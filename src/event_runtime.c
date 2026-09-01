#include "conf.h"
#include "sysdep.h"
#include "event_runtime.h"
#include "perfmon.h"

static struct game_scheduler *runtime_scheduler;
static struct event_runtime_type_profile
{
  game_event_handler handler;
  int perf_index;
} *runtime_type_profiles;
static size_t runtime_type_profile_capacity;

static struct event_runtime_type_profile *runtime_type_profile(
    game_event_type_id_t event_type)
{
  if (event_type == 0U || event_type > runtime_type_profile_capacity)
    return NULL;
  return &runtime_type_profiles[event_type - 1U];
}

static struct game_event_result event_runtime_dispatch(
    const struct game_event_context *context)
{
  struct event_runtime_type_profile *profile;
  struct game_event_result result;
  uint64_t started_usec;
  uint64_t finished_usec;

  if (context == NULL ||
      (profile = runtime_type_profile(context->event_type)) == NULL ||
      profile->handler == NULL)
    return game_event_result_failed(GAME_SCHEDULER_INVALID_TYPE);

  started_usec = PERF_monotonic_usec();
  result = profile->handler(context);
  finished_usec = PERF_monotonic_usec();
  if (profile->perf_index >= 0)
  {
    PERF_note_event_callback(profile->perf_index,
                             finished_usec >= started_usec
                                 ? finished_usec - started_usec
                                 : 0U);
    if (result.kind == GAME_EVENT_RESULT_RESCHEDULE_AT ||
        result.kind == GAME_EVENT_RESULT_RESCHEDULE_AFTER)
      PERF_note_event_rescheduled(profile->perf_index, result.value);
  }
  return result;
}

static enum game_scheduler_status runtime_required(void)
{
  return runtime_scheduler != NULL ? GAME_SCHEDULER_OK : GAME_SCHEDULER_INVALID_ARGUMENT;
}

static void assign_handle(struct event_runtime_handle *handle, game_event_id_t event_id)
{
  if (handle != NULL)
    handle->id = event_id;
}

static void note_scheduled(game_event_type_id_t event_type,
                           game_tick_t delay_ticks)
{
  struct event_runtime_type_profile *profile;

  profile = runtime_type_profile(event_type);
  if (profile != NULL && profile->perf_index >= 0)
    PERF_note_event_scheduled(profile->perf_index, delay_ticks);
}

static game_tick_t delay_until(game_tick_t deadline_tick)
{
  game_tick_t current_tick;

  current_tick = game_scheduler_current_tick(runtime_scheduler);
  return deadline_tick > current_tick
             ? deadline_tick - current_tick
             : 1U;
}

enum game_scheduler_status
event_runtime_init(const struct game_scheduler_config *config)
{
  enum game_scheduler_status status;
  size_t profile_capacity;

  if (config == NULL)
    return GAME_SCHEDULER_INVALID_ARGUMENT;
  if (runtime_scheduler != NULL)
    return GAME_SCHEDULER_BUSY;
  profile_capacity = config->max_event_types > 0U
                         ? config->max_event_types
                         : GAME_SCHEDULER_DEFAULT_MAX_EVENT_TYPES;
  runtime_type_profiles = calloc(profile_capacity, sizeof(*runtime_type_profiles));
  if (runtime_type_profiles == NULL)
    return GAME_SCHEDULER_ALLOCATION_FAILED;
  runtime_type_profile_capacity = profile_capacity;
  runtime_scheduler = game_scheduler_create(config, &status);
  if (runtime_scheduler == NULL)
  {
    free(runtime_type_profiles);
    runtime_type_profiles = NULL;
    runtime_type_profile_capacity = 0U;
  }
  return runtime_scheduler != NULL ? GAME_SCHEDULER_OK : status;
}

enum game_scheduler_status event_runtime_shutdown(void)
{
  enum game_scheduler_status status;

  if (runtime_scheduler == NULL)
    return GAME_SCHEDULER_OK;
  status = game_scheduler_destroy(runtime_scheduler);
  if (status == GAME_SCHEDULER_OK)
  {
    runtime_scheduler = NULL;
    free(runtime_type_profiles);
    runtime_type_profiles = NULL;
    runtime_type_profile_capacity = 0U;
  }
  return status;
}

bool event_runtime_is_initialized(void)
{
  return runtime_scheduler != NULL;
}

enum game_scheduler_status event_runtime_register_type(
    const struct game_event_type_config *config, game_event_type_id_t *event_type)
{
  struct event_runtime_type_profile *profile;
  struct game_event_type_config registered_config;
  enum game_scheduler_status status;

  if (runtime_required() != GAME_SCHEDULER_OK)
    return GAME_SCHEDULER_INVALID_ARGUMENT;
  if (config == NULL || config->handler == NULL || event_type == NULL)
    return GAME_SCHEDULER_INVALID_ARGUMENT;
  registered_config = *config;
  registered_config.handler = event_runtime_dispatch;
  status = game_scheduler_register_type(runtime_scheduler, &registered_config,
                                        event_type);
  if (status != GAME_SCHEDULER_OK)
    return status;
  profile = runtime_type_profile(*event_type);
  if (profile == NULL)
    return GAME_SCHEDULER_CAPACITY_REACHED;
  profile->handler = config->handler;
  /* The rollback adapter records the concrete callback name itself. */
  profile->perf_index = !strcmp(config->name, "legacy_event")
                            ? -1
                            : PERF_register_event_callback(config->name);
  return GAME_SCHEDULER_OK;
}

enum game_scheduler_status event_runtime_seal_types(void)
{
  if (runtime_required() != GAME_SCHEDULER_OK)
    return GAME_SCHEDULER_INVALID_ARGUMENT;
  return game_scheduler_seal_types(runtime_scheduler);
}

bool event_runtime_types_are_sealed(void)
{
  return game_scheduler_types_are_sealed(runtime_scheduler);
}

const char *event_runtime_type_name(game_event_type_id_t event_type)
{
  return game_scheduler_type_name(runtime_scheduler, event_type);
}

enum game_scheduler_status event_runtime_find_type(
    const char *name, game_event_type_id_t *event_type)
{
  game_event_type_id_t candidate;
  const char *candidate_name;

  if (runtime_required() != GAME_SCHEDULER_OK || name == NULL ||
      *name == '\0' || event_type == NULL)
    return GAME_SCHEDULER_INVALID_ARGUMENT;
  for (candidate = 1U; candidate <= runtime_type_profile_capacity;
       candidate++)
  {
    candidate_name = game_scheduler_type_name(runtime_scheduler, candidate);
    if (candidate_name == NULL)
      break;
    if (!strcmp(candidate_name, name))
    {
      *event_type = candidate;
      return GAME_SCHEDULER_OK;
    }
  }
  return GAME_SCHEDULER_INVALID_TYPE;
}

size_t event_runtime_event_count(void)
{
  return game_scheduler_event_count(runtime_scheduler);
}

enum game_scheduler_status event_runtime_type_live_count(
    game_event_type_id_t event_type, size_t *live_count)
{
  if (runtime_required() != GAME_SCHEDULER_OK)
    return GAME_SCHEDULER_INVALID_ARGUMENT;
  return game_scheduler_type_live_count(runtime_scheduler, event_type,
                                        live_count);
}

enum game_scheduler_status event_runtime_schedule_at(
    game_event_type_id_t event_type, game_tick_t deadline_tick, void *payload,
    struct event_runtime_handle *handle)
{
  enum game_scheduler_status status;
  game_event_id_t event_id;

  if (runtime_required() != GAME_SCHEDULER_OK || handle == NULL)
    return GAME_SCHEDULER_INVALID_ARGUMENT;
  assign_handle(handle, 0);
  status = game_scheduler_schedule_at(runtime_scheduler, event_type, deadline_tick,
                                      payload, &event_id);
  if (status == GAME_SCHEDULER_OK)
  {
    assign_handle(handle, event_id);
    note_scheduled(event_type, delay_until(deadline_tick));
  }
  return status;
}

enum game_scheduler_status event_runtime_schedule_after(
    game_event_type_id_t event_type, game_tick_t delay_ticks, void *payload,
    struct event_runtime_handle *handle)
{
  enum game_scheduler_status status;
  game_event_id_t event_id;

  if (runtime_required() != GAME_SCHEDULER_OK || handle == NULL)
    return GAME_SCHEDULER_INVALID_ARGUMENT;
  assign_handle(handle, 0);
  status = game_scheduler_schedule_after(runtime_scheduler, event_type, delay_ticks,
                                         payload, &event_id);
  if (status == GAME_SCHEDULER_OK)
  {
    assign_handle(handle, event_id);
    note_scheduled(event_type, delay_ticks);
  }
  return status;
}

enum game_scheduler_status event_runtime_schedule_owned_at(
    game_event_type_id_t event_type, struct game_event_owner owner,
    game_tick_t deadline_tick, void *payload, struct event_runtime_handle *handle)
{
  enum game_scheduler_status status;
  game_event_id_t event_id;

  if (runtime_required() != GAME_SCHEDULER_OK || handle == NULL)
    return GAME_SCHEDULER_INVALID_ARGUMENT;
  assign_handle(handle, 0);
  status = game_scheduler_schedule_owned_at(runtime_scheduler, event_type, owner,
                                            deadline_tick, payload, &event_id);
  if (status == GAME_SCHEDULER_OK)
  {
    assign_handle(handle, event_id);
    note_scheduled(event_type, delay_until(deadline_tick));
  }
  return status;
}

enum game_scheduler_status event_runtime_schedule_owned_after(
    game_event_type_id_t event_type, struct game_event_owner owner,
    game_tick_t delay_ticks, void *payload, struct event_runtime_handle *handle)
{
  enum game_scheduler_status status;
  game_event_id_t event_id;

  if (runtime_required() != GAME_SCHEDULER_OK || handle == NULL)
    return GAME_SCHEDULER_INVALID_ARGUMENT;
  assign_handle(handle, 0);
  status = game_scheduler_schedule_owned_after(runtime_scheduler, event_type, owner,
                                               delay_ticks, payload, &event_id);
  if (status == GAME_SCHEDULER_OK)
  {
    assign_handle(handle, event_id);
    note_scheduled(event_type, delay_ticks);
  }
  return status;
}

enum game_event_cancel_result event_runtime_cancel(struct event_runtime_handle handle)
{
  if (runtime_scheduler == NULL || handle.id == 0)
    return GAME_EVENT_CANCEL_NOT_FOUND;
  return game_scheduler_cancel(runtime_scheduler, handle.id);
}

enum game_scheduler_status event_runtime_cancel_owner(struct game_event_owner owner,
                                                      size_t *cancelled_count)
{
  if (runtime_required() != GAME_SCHEDULER_OK)
    return GAME_SCHEDULER_INVALID_ARGUMENT;
  return game_scheduler_cancel_owner(runtime_scheduler, owner, cancelled_count);
}

enum game_scheduler_status event_runtime_reschedule_at(struct event_runtime_handle handle,
                                                       game_tick_t deadline_tick)
{
  if (runtime_required() != GAME_SCHEDULER_OK || handle.id == 0)
    return GAME_SCHEDULER_INVALID_ARGUMENT;
  return game_scheduler_reschedule_at(runtime_scheduler, handle.id, deadline_tick);
}

enum game_scheduler_status event_runtime_reschedule_after(struct event_runtime_handle handle,
                                                          game_tick_t delay_ticks)
{
  if (runtime_required() != GAME_SCHEDULER_OK || handle.id == 0)
    return GAME_SCHEDULER_INVALID_ARGUMENT;
  return game_scheduler_reschedule_after(runtime_scheduler, handle.id, delay_ticks);
}

enum game_scheduler_status event_runtime_remaining(struct event_runtime_handle handle,
                                                   game_tick_t *remaining_ticks)
{
  if (runtime_required() != GAME_SCHEDULER_OK || handle.id == 0)
    return GAME_SCHEDULER_INVALID_ARGUMENT;
  return game_scheduler_remaining(runtime_scheduler, handle.id, remaining_ticks);
}

bool event_runtime_handle_is_live(struct event_runtime_handle handle)
{
  struct game_event_snapshot snapshot;

  return handle.id != 0 && event_runtime_inspect(handle, &snapshot) == GAME_SCHEDULER_OK;
}

enum game_scheduler_status event_runtime_advance(
    const struct game_scheduler_budget *budget,
    struct game_scheduler_dispatch_report *report)
{
  if (runtime_required() != GAME_SCHEDULER_OK)
    return GAME_SCHEDULER_INVALID_ARGUMENT;
  return game_scheduler_advance(runtime_scheduler, budget, report);
}

enum game_scheduler_status event_runtime_next_deadline(game_tick_t *deadline_tick,
                                                       bool *has_deadline)
{
  if (runtime_required() != GAME_SCHEDULER_OK)
    return GAME_SCHEDULER_INVALID_ARGUMENT;
  return game_scheduler_next_deadline(runtime_scheduler, deadline_tick, has_deadline);
}

enum game_scheduler_status event_runtime_inspect(struct event_runtime_handle handle,
                                                 struct game_event_snapshot *snapshot)
{
  if (runtime_required() != GAME_SCHEDULER_OK || handle.id == 0)
    return GAME_SCHEDULER_INVALID_ARGUMENT;
  return game_scheduler_inspect(runtime_scheduler, handle.id, snapshot);
}

enum game_scheduler_status event_runtime_inspect_owner(
    struct game_event_owner owner, struct game_event_snapshot *snapshots,
    size_t snapshot_capacity, size_t *event_count)
{
  if (runtime_required() != GAME_SCHEDULER_OK)
    return GAME_SCHEDULER_INVALID_ARGUMENT;
  return game_scheduler_inspect_owner(runtime_scheduler, owner, snapshots,
                                      snapshot_capacity, event_count);
}

enum game_scheduler_status event_runtime_inspect_all(
    struct game_event_snapshot *snapshots, size_t snapshot_capacity,
    size_t *event_count)
{
  if (runtime_required() != GAME_SCHEDULER_OK)
    return GAME_SCHEDULER_INVALID_ARGUMENT;
  return game_scheduler_inspect_all(runtime_scheduler, snapshots, snapshot_capacity,
                                    event_count);
}

void event_runtime_get_stats(struct game_scheduler_stats *stats)
{
  game_scheduler_get_stats(runtime_scheduler, stats);
}
