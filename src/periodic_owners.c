#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "dotenv.h"
#include "event_runtime.h"
#include "periodic_owners.h"
#include "dgscript/dg_scripts.h"

#define PERIODIC_AUTOPROC_MAX_OWNERS 16384U
#define PERIODIC_DG_RANDOM_MAX_OWNERS 32768U
#define PERIODIC_REJECTION_LOG_INTERVAL 100U

static bool initialized;
static bool autoproc_scheduled;
static bool dg_random_scheduled;
static size_t autoproc_count;
static size_t dg_random_counts[3];
static uint64_t autoproc_rejections;
static uint64_t dg_random_rejections;
static uint64_t autoproc_callback_count;
static uint64_t dg_random_callback_counts[3];
static uint64_t dg_random_execution_counts[3];
static size_t autoproc_limit = PERIODIC_AUTOPROC_MAX_OWNERS;
static size_t dg_random_limit = PERIODIC_DG_RANDOM_MAX_OWNERS;
static uint64_t next_owner_generation = 1U;
static game_event_type_id_t autoproc_event_type;
static game_event_type_id_t dg_random_event_type;
#ifdef LUMINARI_CUTEST
static bool test_selection_set;
static bool test_autoproc_selection;
static bool test_dg_random_selection;
#endif

static struct game_event_result periodic_autoproc_event(const struct game_event_context *context);
static struct game_event_result periodic_dg_random_event(const struct game_event_context *context);

static uint64_t ensure_owner_generation(uint64_t *generation)
{
  if (generation == NULL || *generation != 0U)
    return generation != NULL ? *generation : 0U;
  if (next_owner_generation == 0U)
    return 0U;
  *generation = next_owner_generation;
  if (next_owner_generation == UINT64_MAX)
    next_owner_generation = 0U;
  else
    next_owner_generation++;
  return *generation;
}

static struct game_event_owner object_owner(struct obj_data *obj)
{
  struct game_event_owner owner = game_event_owner_none();

  if (obj == NULL)
    return owner;
  owner.kind = GAME_EVENT_OWNER_OBJECT;
  owner.runtime_id = (uint64_t)(uintptr_t)obj;
  owner.generation = ensure_owner_generation(&obj->periodic_event_generation);
  return owner;
}

static struct game_event_owner script_owner(struct script_data *script)
{
  struct game_event_owner owner = game_event_owner_none();
  struct char_data *ch;
  struct obj_data *obj;
  struct room_data *room;

  if (script == NULL || script->owner == NULL)
    return owner;
  switch (script->owner_type)
  {
  case MOB_TRIGGER:
    ch = script->owner;
    owner.kind = GAME_EVENT_OWNER_CHARACTER;
    owner.runtime_id = (uint64_t)(uintptr_t)ch;
    owner.generation = ensure_owner_generation(&ch->periodic_event_generation);
    break;
  case OBJ_TRIGGER:
    obj = script->owner;
    owner = object_owner(obj);
    break;
  case WLD_TRIGGER:
    room = script->owner;
    owner.kind = GAME_EVENT_OWNER_ROOM;
    owner.runtime_id = (uint64_t)(uint32_t)room->number + 1U;
    owner.generation = ensure_owner_generation(&room->periodic_event_generation);
    break;
  }
  return owner;
}

static long spread_delay(struct game_event_owner owner, long cadence, uint64_t salt)
{
  uint64_t spread;

  spread = owner.runtime_id ^ (owner.generation + salt);
  spread ^= spread >> 30U;
  spread *= UINT64_C(0xbf58476d1ce4e5b9);
  spread ^= spread >> 27U;
  spread *= UINT64_C(0x94d049bb133111eb);
  spread ^= spread >> 31U;
  return (long)(spread % (uint64_t)cadence) + 1L;
}

static bool runtime_handle_matches(struct event_runtime_handle handle,
                                   const struct game_event_context *context)
{
  return context != NULL && handle.id == context->event_id;
}

static void borrowed_owner_cleanup(void *payload)
{
  (void)payload;
}

static bool register_event_type(const char *name, game_event_handler handler, size_t max_events,
                                game_event_type_id_t *event_type)
{
  struct game_event_type_config config;
  const char *registered_name;
  enum game_scheduler_status status;

  if (!event_runtime_is_initialized() || event_type == NULL)
    return false;
  registered_name = event_runtime_type_name(*event_type);
  if (registered_name != NULL && !strcmp(registered_name, name))
    return true;
  *event_type = 0U;
  memset(&config, 0, sizeof(config));
  config.name = name;
  config.handler = handler;
  config.cleanup = borrowed_owner_cleanup;
  config.lateness_policy = GAME_EVENT_LATENESS_RUN_ONCE;
  config.max_events = max_events;
  config.max_events_per_owner = 1U;
  config.requires_owner = true;
  status = event_runtime_register_type(&config, event_type);
  if (status != GAME_SCHEDULER_OK)
  {
    log("SYSERR: unable to register native event type '%s' (status %d).", name, status);
    return false;
  }
  return true;
}

static struct game_event_result periodic_autoproc_event(const struct game_event_context *context)
{
  struct obj_data *obj = context != NULL ? context->payload : NULL;

  if (obj == NULL)
    return game_event_result_complete();
  autoproc_callback_count++;
  if (!obj->autoproc_registered || !OBJ_FLAGGED(obj, ITEM_AUTOPROC))
  {
    if (runtime_handle_matches(obj->autoproc_event_handle, context))
    {
      obj->autoproc_event_handle = EVENT_RUNTIME_HANDLE_NONE;
      if (autoproc_count > 0U)
        autoproc_count--;
    }
    return game_event_result_complete();
  }
  object_auto_proc_run_one(obj);
  return game_event_result_reschedule_after(PULSE_MOBILE);
}

static struct game_event_result periodic_dg_random_event(const struct game_event_context *context)
{
  struct script_data *script = context != NULL ? context->payload : NULL;
  void *owner;
  int owner_type;

  if (script == NULL)
    return game_event_result_complete();
  owner_type = script->owner_type;
  if (owner_type < MOB_TRIGGER || owner_type > WLD_TRIGGER || !script->random_registered)
  {
    if (runtime_handle_matches(script->random_event_handle, context))
    {
      script->random_event_handle = EVENT_RUNTIME_HANDLE_NONE;
      if (owner_type >= MOB_TRIGGER && owner_type <= WLD_TRIGGER &&
          dg_random_counts[owner_type] > 0U)
        dg_random_counts[owner_type]--;
    }
    return game_event_result_complete();
  }
  dg_random_callback_counts[owner_type]++;
  owner = dg_random_registry_resolve_owner(script);
  if (owner == NULL)
  {
    if (runtime_handle_matches(script->random_event_handle, context))
    {
      script->random_event_handle = EVENT_RUNTIME_HANDLE_NONE;
      if (dg_random_counts[owner_type] > 0U)
        dg_random_counts[owner_type]--;
    }
    return game_event_result_complete();
  }
  if (dg_random_trigger_run_one(owner, owner_type))
    dg_random_execution_counts[owner_type]++;
  return game_event_result_reschedule_after(PULSE_DG_SCRIPT);
}

void periodic_autoproc_sync(struct obj_data *obj)
{
  struct game_event_owner owner;

  if (!initialized || !autoproc_scheduled || obj == NULL || !obj->autoproc_registered ||
      !event_runtime_handle_is_none(obj->autoproc_event_handle))
    return;
  if (autoproc_count >= autoproc_limit)
  {
    autoproc_rejections++;
    if (autoproc_rejections == 1U || autoproc_rejections % PERIODIC_REJECTION_LOG_INTERVAL == 0U)
      log("WARNING: ITEM_AUTOPROC owner limit reached (%zu); rejected=%llu.", autoproc_limit,
          (unsigned long long)autoproc_rejections);
    return;
  }
  owner = object_owner(obj);
  if (!game_event_owner_is_valid(owner))
    return;
  if (event_runtime_schedule_owned_after(
          autoproc_event_type, owner,
          (game_tick_t)spread_delay(owner, PULSE_MOBILE, UINT64_C(0xa17f0c)), obj,
          &obj->autoproc_event_handle) != GAME_SCHEDULER_OK)
  {
    autoproc_rejections++;
    return;
  }
  autoproc_count++;
}

void periodic_autoproc_forget(struct obj_data *obj)
{
  struct event_runtime_handle handle;

  if (obj == NULL || event_runtime_handle_is_none(obj->autoproc_event_handle))
    return;
  handle = obj->autoproc_event_handle;
  obj->autoproc_event_handle = EVENT_RUNTIME_HANDLE_NONE;
  if (autoproc_count > 0U)
    autoproc_count--;
  (void)event_runtime_cancel(handle);
}

void periodic_dg_random_sync(struct script_data *script)
{
  struct game_event_owner owner;
  size_t total;
  int owner_type;

  if (!initialized || !dg_random_scheduled || script == NULL || !script->random_registered ||
      !event_runtime_handle_is_none(script->random_event_handle))
    return;
  owner_type = script->owner_type;
  if (owner_type < MOB_TRIGGER || owner_type > WLD_TRIGGER)
    return;
  total =
      dg_random_counts[MOB_TRIGGER] + dg_random_counts[OBJ_TRIGGER] + dg_random_counts[WLD_TRIGGER];
  if (total >= dg_random_limit)
  {
    dg_random_rejections++;
    if (dg_random_rejections == 1U || dg_random_rejections % PERIODIC_REJECTION_LOG_INTERVAL == 0U)
      log("WARNING: DG random-trigger owner limit reached (%zu); rejected=%llu.", dg_random_limit,
          (unsigned long long)dg_random_rejections);
    return;
  }
  owner = script_owner(script);
  if (!game_event_owner_is_valid(owner))
    return;
  if (event_runtime_schedule_owned_after(
          dg_random_event_type, owner,
          (game_tick_t)spread_delay(owner, PULSE_DG_SCRIPT,
                                    UINT64_C(0xd672a9) + (uint64_t)owner_type),
          script, &script->random_event_handle) != GAME_SCHEDULER_OK)
  {
    dg_random_rejections++;
    return;
  }
  dg_random_counts[owner_type]++;
}

void periodic_dg_random_forget(struct script_data *script)
{
  struct event_runtime_handle handle;
  int owner_type;

  if (script == NULL || event_runtime_handle_is_none(script->random_event_handle))
    return;
  owner_type = script->owner_type;
  handle = script->random_event_handle;
  script->random_event_handle = EVENT_RUNTIME_HANDLE_NONE;
  if (owner_type >= MOB_TRIGGER && owner_type <= WLD_TRIGGER && dg_random_counts[owner_type] > 0U)
    dg_random_counts[owner_type]--;
  (void)event_runtime_cancel(handle);
}

void periodic_owners_init(void)
{
  struct obj_data *obj;
  struct script_data *script;
  void *owner;
  bool autoproc_ready;
  bool autoproc_requested;
  bool dg_random_ready;
  bool dg_random_requested;
  int owner_type;

  if (initialized)
    return;
#ifdef LUMINARI_CUTEST
  if (test_selection_set)
  {
    autoproc_requested = test_autoproc_selection;
    dg_random_requested = test_dg_random_selection;
  }
  else
#endif
  {
    autoproc_requested = true;
    dg_random_requested = true;
  }
  autoproc_ready = register_event_type("object.automatic_procedure", periodic_autoproc_event,
                                       PERIODIC_AUTOPROC_MAX_OWNERS, &autoproc_event_type);
  dg_random_ready = register_event_type("dg.random_trigger", periodic_dg_random_event,
                                        PERIODIC_DG_RANDOM_MAX_OWNERS, &dg_random_event_type);
  autoproc_scheduled = autoproc_requested && autoproc_ready;
  dg_random_scheduled = dg_random_requested && dg_random_ready;
  initialized = true;
  if (autoproc_requested && !autoproc_ready)
    log("SYSERR: native automatic-procedure event type is unavailable.");
  if (dg_random_requested && !dg_random_ready)
    log("SYSERR: native DG random-trigger event type is unavailable.");
  log("ITEM_AUTOPROC scheduling: %s (owner limit %zu).",
      autoproc_scheduled ? "scheduled" : "unavailable", autoproc_limit);
  log("DG random-trigger scheduling: %s (combined owner limit %zu).",
      dg_random_scheduled ? "scheduled" : "unavailable", dg_random_limit);

  if (autoproc_scheduled)
  {
    for (obj = autoproc_registry_iteration_begin(); obj != NULL;
         obj = autoproc_registry_iteration_next())
      periodic_autoproc_sync(obj);
    autoproc_registry_iteration_end();
  }

  if (!dg_random_scheduled)
    return;
  for (owner_type = MOB_TRIGGER; owner_type <= WLD_TRIGGER; owner_type++)
  {
    for (owner = dg_random_registry_iteration_begin(owner_type); owner != NULL;
         owner = dg_random_registry_iteration_next())
    {
      if (owner_type == MOB_TRIGGER)
        script = SCRIPT((struct char_data *)owner);
      else if (owner_type == OBJ_TRIGGER)
        script = SCRIPT((struct obj_data *)owner);
      else
        script = SCRIPT((struct room_data *)owner);
      periodic_dg_random_sync(script);
    }
    dg_random_registry_iteration_end();
  }
}

void periodic_owners_shutdown(void)
{
  struct obj_data *obj;
  struct script_data *script;
  void *owner;
  int owner_type;

  if (!initialized)
    return;
  for (obj = autoproc_registry_iteration_begin(); obj != NULL;
       obj = autoproc_registry_iteration_next())
    periodic_autoproc_forget(obj);
  autoproc_registry_iteration_end();
  for (owner_type = MOB_TRIGGER; owner_type <= WLD_TRIGGER; owner_type++)
  {
    for (owner = dg_random_registry_iteration_begin(owner_type); owner != NULL;
         owner = dg_random_registry_iteration_next())
    {
      if (owner_type == MOB_TRIGGER)
        script = SCRIPT((struct char_data *)owner);
      else if (owner_type == OBJ_TRIGGER)
        script = SCRIPT((struct obj_data *)owner);
      else
        script = SCRIPT((struct room_data *)owner);
      periodic_dg_random_forget(script);
    }
    dg_random_registry_iteration_end();
  }
  autoproc_count = 0U;
  memset(dg_random_counts, 0, sizeof(dg_random_counts));
  initialized = false;
  autoproc_scheduled = false;
  dg_random_scheduled = false;
}

bool periodic_autoproc_enabled(void)
{
  return initialized && autoproc_scheduled;
}

size_t periodic_autoproc_scheduled_count(void)
{
  return autoproc_count;
}

size_t periodic_autoproc_admission_limit(void)
{
  return autoproc_limit;
}

uint64_t periodic_autoproc_admission_rejections(void)
{
  return autoproc_rejections;
}

uint64_t periodic_autoproc_callbacks(void)
{
  return autoproc_callback_count;
}

bool periodic_dg_random_enabled(void)
{
  return initialized && dg_random_scheduled;
}

size_t periodic_dg_random_scheduled_count(int owner_type)
{
  if (owner_type < MOB_TRIGGER || owner_type > WLD_TRIGGER)
    return 0U;
  return dg_random_counts[owner_type];
}

size_t periodic_dg_random_admission_limit(void)
{
  return dg_random_limit;
}

uint64_t periodic_dg_random_admission_rejections(void)
{
  return dg_random_rejections;
}

uint64_t periodic_dg_random_callbacks(int owner_type)
{
  if (owner_type < MOB_TRIGGER || owner_type > WLD_TRIGGER)
    return 0U;
  return dg_random_callback_counts[owner_type];
}

uint64_t periodic_dg_random_executions(int owner_type)
{
  if (owner_type < MOB_TRIGGER || owner_type > WLD_TRIGGER)
    return 0U;
  return dg_random_execution_counts[owner_type];
}

void periodic_owners_reset_telemetry(void)
{
  autoproc_rejections = 0U;
  dg_random_rejections = 0U;
  autoproc_callback_count = 0U;
  memset(dg_random_callback_counts, 0, sizeof(dg_random_callback_counts));
  memset(dg_random_execution_counts, 0, sizeof(dg_random_execution_counts));
}

#ifdef LUMINARI_CUTEST
void periodic_owners_reset_for_test(void)
{
  periodic_owners_shutdown();
  periodic_owners_reset_telemetry();
  autoproc_limit = PERIODIC_AUTOPROC_MAX_OWNERS;
  dg_random_limit = PERIODIC_DG_RANDOM_MAX_OWNERS;
  test_selection_set = false;
  test_autoproc_selection = false;
  test_dg_random_selection = false;
}

void periodic_owners_select_for_test(bool use_autoproc_events, bool use_dg_random_events)
{
  test_selection_set = true;
  test_autoproc_selection = use_autoproc_events;
  test_dg_random_selection = use_dg_random_events;
}

void periodic_owners_set_limits_for_test(size_t new_autoproc_limit, size_t new_dg_random_limit)
{
  autoproc_limit = new_autoproc_limit;
  dg_random_limit = new_dg_random_limit;
}
#endif
