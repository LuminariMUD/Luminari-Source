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
#include "mob/mob_act.h"

#define ACTIVE_WORLD_MAX_MOBILES 65536U
#define ACTIVE_WORLD_REJECTION_LOG_INTERVAL 100U

static struct char_data *scheduled_mobiles;
static size_t active_mobile_count;
static size_t cooling_mobile_count;
static uint64_t admission_rejections;
static uint64_t mobile_callbacks;
static bool initialized;
static bool enabled;
static size_t admission_limit = ACTIVE_WORLD_MAX_MOBILES;
#ifdef LUMINARI_CUTEST
static bool test_selection_set;
static bool test_selection;
#endif

static bool configured_enabled(void)
{
  const char *value;

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
}

static bool mobile_should_be_active(struct char_data *ch)
{
  return ch != NULL && IS_MOB(ch) && IN_ROOM(ch) != NOWHERE && IN_ROOM(ch) <= top_of_world &&
         !MOB_FLAGGED(ch, MOB_NOTDEADYET) && !MOB_FLAGGED(ch, MOB_NO_AI);
}

static void registry_insert(struct char_data *ch)
{
  ch->active_world_prev = NULL;
  ch->active_world_next = scheduled_mobiles;
  if (scheduled_mobiles != NULL)
    scheduled_mobiles->active_world_prev = ch;
  scheduled_mobiles = ch;
}

static void registry_remove(struct char_data *ch)
{
  if (ch->active_world_prev != NULL)
    ch->active_world_prev->active_world_next = ch->active_world_next;
  else if (scheduled_mobiles == ch)
    scheduled_mobiles = ch->active_world_next;
  if (ch->active_world_next != NULL)
    ch->active_world_next->active_world_prev = ch->active_world_prev;
  ch->active_world_next = NULL;
  ch->active_world_prev = NULL;
}

static void set_state(struct char_data *ch, enum active_world_mobile_state state)
{
  enum active_world_mobile_state previous;

  previous = (enum active_world_mobile_state)ch->active_world_state;
  if (previous == state)
    return;
  if (previous == ACTIVE_WORLD_MOBILE_ACTIVE && active_mobile_count > 0)
    active_mobile_count--;
  else if (previous == ACTIVE_WORLD_MOBILE_COOLING && cooling_mobile_count > 0)
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

static void active_world_mobile_cleanup(struct event *event)
{
  if (event != NULL)
    event->event_obj = NULL;
}

static EVENTFUNC(active_world_mobile_event)
{
  struct char_data *ch;

  ch = event_obj;
  if (ch == NULL)
    return 0;
  mobile_callbacks++;

  if (!mobile_should_be_active(ch))
  {
    if (ch->active_world_state == ACTIVE_WORLD_MOBILE_ACTIVE)
    {
      set_state(ch, ACTIVE_WORLD_MOBILE_COOLING);
      return PULSE_MOBILE;
    }
    if (ch->active_world_state == ACTIVE_WORLD_MOBILE_COOLING)
    {
      registry_remove(ch);
      set_state(ch, ACTIVE_WORLD_MOBILE_DORMANT);
      ch->active_world_event = NULL;
      mobile_activity_run_one(ch);
      return 0;
    }
    ch->active_world_event = NULL;
    return 0;
  }

  set_state(ch, ACTIVE_WORLD_MOBILE_ACTIVE);
  mobile_activity_run_one(ch);
  return PULSE_MOBILE;
}

static bool schedule_mobile(struct char_data *ch)
{
  struct game_event_owner owner;
  uint64_t spread;

  owner = mobile_owner(ch);
  if (!game_event_owner_is_valid(owner))
    return false;
  spread = (owner.runtime_id >> 4U) ^ owner.generation;
  spread *= UINT64_C(11400714819323198485);
  ch->active_world_event = event_create_owned_named_with_cleanup(
      active_world_mobile_event, ch,
      (long)(spread % (uint64_t)PULSE_MOBILE) + 1L,
      "active_world_mobile", active_world_mobile_cleanup, owner);
  return ch->active_world_event != NULL;
}

void active_world_sync_mobile(struct char_data *ch)
{
  enum active_world_mobile_state state;

  if (!initialized || !enabled || ch == NULL || !IS_NPC(ch))
    return;
  state = (enum active_world_mobile_state)ch->active_world_state;
  if (mobile_should_be_active(ch))
  {
    if (state == ACTIVE_WORLD_MOBILE_ACTIVE)
      return;
    if (state == ACTIVE_WORLD_MOBILE_COOLING)
    {
      set_state(ch, ACTIVE_WORLD_MOBILE_ACTIVE);
      return;
    }
    if (active_mobile_count + cooling_mobile_count >= admission_limit)
    {
      admission_rejections++;
      if (admission_rejections == 1 ||
          admission_rejections % ACTIVE_WORLD_REJECTION_LOG_INTERVAL == 0)
        log("WARNING: Active-world mobile admission limit reached (%u); rejected=%llu.",
            (unsigned int)admission_limit, (unsigned long long)admission_rejections);
      return;
    }
    registry_insert(ch);
    set_state(ch, ACTIVE_WORLD_MOBILE_ACTIVE);
    if (!schedule_mobile(ch))
    {
      registry_remove(ch);
      set_state(ch, ACTIVE_WORLD_MOBILE_DORMANT);
      admission_rejections++;
    }
    return;
  }

  if (state == ACTIVE_WORLD_MOBILE_ACTIVE)
    set_state(ch, ACTIVE_WORLD_MOBILE_COOLING);
}

void active_world_forget_character(struct char_data *ch)
{
  enum active_world_mobile_state state;

  if (ch == NULL)
    return;
  state = (enum active_world_mobile_state)ch->active_world_state;
  if (state != ACTIVE_WORLD_MOBILE_DORMANT)
  {
    registry_remove(ch);
    set_state(ch, ACTIVE_WORLD_MOBILE_DORMANT);
  }
  if (ch->active_world_event != NULL)
  {
    struct event *event = ch->active_world_event;
    ch->active_world_event = NULL;
    event_cancel(event);
  }
}

static void sync_room(struct domain_event_bus *bus, struct domain_entity_handle handle)
{
  struct room_data *room;
  struct char_data *ch;
  struct char_data *next;

  room = domain_event_resolve(bus, handle, DOMAIN_ENTITY_ROOM);
  if (room == NULL)
    return;
  for (ch = room->people; ch != NULL; ch = next)
  {
    next = ch->next_in_room;
    active_world_sync_mobile(ch);
  }
}

static void handle_character_moved(const struct domain_event_context *context,
                                   void *handler_context)
{
  const struct domain_character_moved *event;

  (void)handler_context;
  event = context->payload;
  sync_room(context->bus, event->from_room);
  sync_room(context->bus, event->to_room);
}

static void handle_combat_state_changed(const struct domain_event_context *context,
                                        void *handler_context)
{
  const struct domain_combat_state_changed *event;
  struct char_data *ch;

  (void)handler_context;
  event = context->payload;
  ch = domain_event_resolve(context->bus, event->character, DOMAIN_ENTITY_CHARACTER);
  active_world_sync_mobile(ch);
  ch = domain_event_resolve(context->bus, event->opponent, DOMAIN_ENTITY_CHARACTER);
  active_world_sync_mobile(ch);
}

static void handle_entity_extracted(const struct domain_event_context *context,
                                    void *handler_context)
{
  const struct domain_entity_extracted *event;
  struct char_data *ch;

  (void)handler_context;
  event = context->payload;
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
      {DOMAIN_EVENT_ENTITY_EXTRACTED, "active-world-entity-extracted", 100,
       handle_entity_extracted, NULL},
  };
  size_t index;
  enum domain_event_status status;

  if (bus == NULL || initialized)
    return DOMAIN_EVENT_INVALID_ARGUMENT;
  enabled = configured_enabled();
  initialized = true;
  log("Active-world mobile scheduling: %s (autonomous owner limit %u).",
      enabled ? "active" : "legacy heartbeat", (unsigned int)admission_limit);
  if (!enabled)
    return DOMAIN_EVENT_OK;
  for (index = 0; index < sizeof(handlers) / sizeof(handlers[0]); index++)
  {
    status = domain_event_register_handler(bus, &handlers[index]);
    if (status != DOMAIN_EVENT_OK)
      return status;
  }
  return DOMAIN_EVENT_OK;
}

void active_world_shutdown(void)
{
  struct char_data *ch;
  struct char_data *next;

  for (ch = scheduled_mobiles; ch != NULL; ch = next)
  {
    next = ch->active_world_next;
    active_world_forget_character(ch);
  }
  scheduled_mobiles = NULL;
  active_mobile_count = 0;
  cooling_mobile_count = 0;
  initialized = false;
  enabled = false;
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
  return 0;
}

size_t active_world_mobile_admission_limit(void)
{
  return admission_limit;
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
  admission_rejections = 0;
  mobile_callbacks = 0;
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
