#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "dotenv.h"
#include "event_runtime.h"
#include "mudlim.h"
#include "dgscript/dg_scripts.h"
#include "point_update_periodic.h"

#define POINT_UPDATE_CADENCE ((long)(SECS_PER_MUD_HOUR * PASSES_PER_SEC))
#define POINT_UPDATE_SERVICE_ID 0x50555044U

static bool initialized;
static bool scheduled;
static bool shutting_down;
static bool dispatch_due;
static bool character_iteration_active;
static bool object_iteration_active;
static struct event_runtime_handle service_event_handle;
static game_event_type_id_t point_update_event_type;
static struct char_data *character_owners;
static struct char_data *character_iteration_next;
static struct obj_data *object_owners;
static struct obj_data *object_iteration_next;
static size_t character_count;
static size_t object_count;
static uint64_t service_callbacks;
static uint64_t dispatches;
static uint64_t character_executions;
static uint64_t object_executions;
#ifdef LUMINARI_CUTEST
static bool test_selection_set;
static bool test_scheduled_selection;
#endif

static struct game_event_result
point_update_service_event(const struct game_event_context *context);

static bool configured_scheduled(void)
{
#ifdef LUMINARI_CUTEST
  if (test_selection_set)
    return test_scheduled_selection;
#endif
  return true;
}

static long boundary_delay(void)
{
  unsigned long remainder = pulse % (unsigned long)POINT_UPDATE_CADENCE;

  return remainder == 0U ? POINT_UPDATE_CADENCE : POINT_UPDATE_CADENCE - (long)remainder;
}

static struct game_event_owner service_owner(void)
{
  struct game_event_owner owner = game_event_owner_none();

  owner.kind = GAME_EVENT_OWNER_SERVICE;
  owner.runtime_id = POINT_UPDATE_SERVICE_ID;
  owner.generation = 1U;
  return owner;
}

static bool character_eligible(const struct char_data *ch)
{
  return ch != NULL && !IS_NPC(ch);
}

static bool object_eligible(const struct obj_data *obj)
{
  int slot;

  if (obj == NULL || !obj->object_list_member)
    return false;
  for (slot = 0; slot < SPEC_TIMER_MAX; slot++)
    if (GET_OBJ_SPECTIMER(obj, slot) > 0)
      return true;
  return GET_OBJ_TIMER(obj) > 0 || SCRIPT_CHECK(obj, OTRIG_TIMER) ||
         (GET_OBJ_TYPE(obj) == ITEM_MISSILE && GET_OBJ_VAL(obj, 1) != 0) ||
         OBJ_FLAGGED(obj, ITEM_DECAY) || IS_CORPSE(obj);
}

static void character_add(struct char_data *ch)
{
  if (ch == NULL || ch->point_update_registered)
    return;
  ch->point_update_prev = NULL;
  ch->point_update_next = character_owners;
  if (character_owners != NULL)
    character_owners->point_update_prev = ch;
  character_owners = ch;
  ch->point_update_registered = true;
  character_count++;
}

static void character_remove(struct char_data *ch)
{
  if (ch == NULL || !ch->point_update_registered)
    return;
  if (character_iteration_active && character_iteration_next == ch)
    character_iteration_next = ch->point_update_next;
  if (ch->point_update_prev != NULL)
    ch->point_update_prev->point_update_next = ch->point_update_next;
  else if (character_owners == ch)
    character_owners = ch->point_update_next;
  if (ch->point_update_next != NULL)
    ch->point_update_next->point_update_prev = ch->point_update_prev;
  ch->point_update_next = NULL;
  ch->point_update_prev = NULL;
  ch->point_update_registered = false;
  if (character_count > 0U)
    character_count--;
}

static void object_add(struct obj_data *obj)
{
  if (obj == NULL || obj->point_update_registered)
    return;
  obj->point_update_prev = NULL;
  obj->point_update_next = object_owners;
  if (object_owners != NULL)
    object_owners->point_update_prev = obj;
  object_owners = obj;
  obj->point_update_registered = true;
  object_count++;
}

static void object_remove(struct obj_data *obj)
{
  if (obj == NULL || !obj->point_update_registered)
    return;
  if (object_iteration_active && object_iteration_next == obj)
    object_iteration_next = obj->point_update_next;
  if (obj->point_update_prev != NULL)
    obj->point_update_prev->point_update_next = obj->point_update_next;
  else if (object_owners == obj)
    object_owners = obj->point_update_next;
  if (obj->point_update_next != NULL)
    obj->point_update_next->point_update_prev = obj->point_update_prev;
  obj->point_update_next = NULL;
  obj->point_update_prev = NULL;
  obj->point_update_registered = false;
  if (object_count > 0U)
    object_count--;
}

void point_update_character_sync(struct char_data *ch)
{
  if (!initialized || !scheduled || ch == NULL)
    return;
  if (character_eligible(ch))
    character_add(ch);
  else
    character_remove(ch);
}

void point_update_character_forget(struct char_data *ch)
{
  character_remove(ch);
}

void point_update_object_sync(struct obj_data *obj)
{
  if (!initialized || !scheduled || obj == NULL)
    return;
  if (object_eligible(obj))
    object_add(obj);
  else
    object_remove(obj);
}

void point_update_object_forget(struct obj_data *obj)
{
  object_remove(obj);
}

void point_update_object_spec_timer_set(struct obj_data *obj, int slot, int duration)
{
  if (obj == NULL || slot < 0 || slot >= SPEC_TIMER_MAX)
    return;
  GET_OBJ_SPECTIMER(obj, slot) = duration;
  point_update_object_sync(obj);
}

static bool register_point_update_event_type(void)
{
  struct game_event_type_config config;
  const char *registered_name;
  enum game_scheduler_status status;

  if (!event_runtime_is_initialized())
    return false;
  registered_name = event_runtime_type_name(point_update_event_type);
  if (registered_name != NULL && !strcmp(registered_name, "world.mud_hour_update"))
    return true;
  point_update_event_type = 0U;
  memset(&config, 0, sizeof(config));
  config.name = "world.mud_hour_update";
  config.handler = point_update_service_event;
  config.lateness_policy = GAME_EVENT_LATENESS_RUN_ONCE;
  config.max_events = 1U;
  config.max_events_per_owner = 1U;
  config.requires_owner = true;
  status = event_runtime_register_type(&config, &point_update_event_type);
  if (status != GAME_SCHEDULER_OK)
  {
    log("SYSERR: unable to register native event type 'world.mud_hour_update' (status %d).",
        status);
    return false;
  }
  return true;
}

static struct game_event_result point_update_service_event(const struct game_event_context *context)
{
  if (!initialized || !scheduled)
  {
    if (context != NULL && service_event_handle.id == context->event_id)
      service_event_handle = EVENT_RUNTIME_HANDLE_NONE;
    return game_event_result_complete();
  }
  service_callbacks++;
  dispatch_due = true;
  return game_event_result_reschedule_after(POINT_UPDATE_CADENCE);
}

static bool schedule_service(void)
{
  if (!event_runtime_handle_is_none(service_event_handle))
    return true;
  if (!initialized || !scheduled || shutting_down)
    return false;
  return event_runtime_schedule_owned_after(point_update_event_type, service_owner(),
                                            (game_tick_t)boundary_delay(), NULL,
                                            &service_event_handle) == GAME_SCHEDULER_OK;
}

bool point_update_periodic_dispatch_due(void)
{
  struct char_data *ch;
  struct obj_data *obj;

  if (!initialized || !scheduled || !dispatch_due)
    return false;
  dispatch_due = false;
  dispatches++;
  point_update_global_one();

  character_iteration_active = true;
  character_iteration_next = character_owners;
  while ((ch = character_iteration_next) != NULL)
  {
    character_iteration_next = ch->point_update_next;
    character_executions++;
    /* Idle handling may extract ch; the extraction hook already repairs this iteration. */
    point_update_character_one(ch);
  }
  character_iteration_next = NULL;
  character_iteration_active = false;

  object_iteration_active = true;
  object_iteration_next = object_owners;
  while ((obj = object_iteration_next) != NULL)
  {
    object_iteration_next = obj->point_update_next;
    object_executions++;
    if (point_update_object_one(obj))
      point_update_object_sync(obj);
  }
  object_iteration_next = NULL;
  object_iteration_active = false;
  return true;
}

void point_update_periodic_init(void)
{
  struct char_data *ch;
  struct obj_data *obj;
  bool requested;

  if (initialized)
    return;
  requested = configured_scheduled();
  scheduled = requested && register_point_update_event_type();
  initialized = true;
  shutting_down = false;
  dispatch_due = false;
  if (requested && !scheduled)
    log("SYSERR: native mud-hour point-update event type is unavailable.");
  else if (scheduled && !schedule_service())
  {
    log("SYSERR: unable to schedule required native point-update service.");
    scheduled = false;
  }
  if (scheduled)
  {
    for (ch = character_list; ch != NULL; ch = ch->next)
      point_update_character_sync(ch);
    for (obj = object_list; obj != NULL; obj = obj->next)
      point_update_object_sync(obj);
  }
  log("Point-update scheduling: %s.", scheduled ? "scheduled (one service event)" : "unavailable");
}

void point_update_periodic_shutdown(void)
{
  struct char_data *ch;
  struct char_data *next_ch;
  struct obj_data *obj;
  struct obj_data *next_obj;
  struct event_runtime_handle handle;

  if (!initialized)
    return;
  shutting_down = true;
  if (!event_runtime_handle_is_none(service_event_handle))
  {
    handle = service_event_handle;
    service_event_handle = EVENT_RUNTIME_HANDLE_NONE;
    (void)event_runtime_cancel(handle);
  }
  for (ch = character_owners; ch != NULL; ch = next_ch)
  {
    next_ch = ch->point_update_next;
    character_remove(ch);
  }
  for (obj = object_owners; obj != NULL; obj = next_obj)
  {
    next_obj = obj->point_update_next;
    object_remove(obj);
  }
  character_owners = NULL;
  object_owners = NULL;
  character_iteration_next = NULL;
  object_iteration_next = NULL;
  character_iteration_active = false;
  object_iteration_active = false;
  character_count = 0U;
  object_count = 0U;
  initialized = false;
  scheduled = false;
  shutting_down = false;
  dispatch_due = false;
}

bool point_update_events_enabled(void)
{
  return initialized && scheduled;
}
size_t point_update_character_count(void)
{
  return character_count;
}
struct char_data *point_update_character_first(void)
{
  return character_owners;
}
size_t point_update_object_count(void)
{
  return object_count;
}
uint64_t point_update_service_callbacks(void)
{
  return service_callbacks;
}
uint64_t point_update_dispatches(void)
{
  return dispatches;
}
uint64_t point_update_character_executions(void)
{
  return character_executions;
}
uint64_t point_update_object_executions(void)
{
  return object_executions;
}

size_t point_update_character_registry_validate(void)
{
  struct char_data *ch;
  struct char_data *previous = NULL;
  size_t members = 0U;

  for (ch = character_owners; ch != NULL; ch = ch->point_update_next)
  {
    if (!ch->point_update_registered || ch->point_update_prev != previous ||
        !character_eligible(ch))
      return character_count + 1U;
    members++;
    previous = ch;
    if (members > character_count)
      return members;
  }
  return members == character_count
             ? 0U
             : (members > character_count ? members - character_count : character_count - members);
}

size_t point_update_object_registry_validate(void)
{
  struct obj_data *obj;
  struct obj_data *previous = NULL;
  size_t members = 0U;

  for (obj = object_owners; obj != NULL; obj = obj->point_update_next)
  {
    if (!obj->point_update_registered || obj->point_update_prev != previous ||
        !object_eligible(obj))
      return object_count + 1U;
    members++;
    previous = obj;
    if (members > object_count)
      return members;
  }
  return members == object_count
             ? 0U
             : (members > object_count ? members - object_count : object_count - members);
}

void point_update_periodic_reset_telemetry(void)
{
  service_callbacks = 0U;
  dispatches = 0U;
  character_executions = 0U;
  object_executions = 0U;
}

#ifdef LUMINARI_CUTEST
void point_update_periodic_reset_for_test(void)
{
  point_update_periodic_shutdown();
  point_update_periodic_reset_telemetry();
  test_selection_set = false;
  test_scheduled_selection = false;
}

void point_update_periodic_select_for_test(bool use_scheduled)
{
  test_selection_set = true;
  test_scheduled_selection = use_scheduled;
}
#endif
