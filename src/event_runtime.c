#include "conf.h"
#include "sysdep.h"
#include "event_runtime.h"

static struct game_scheduler *runtime_scheduler;

static enum game_scheduler_status runtime_required(void)
{
  return runtime_scheduler != NULL ? GAME_SCHEDULER_OK : GAME_SCHEDULER_INVALID_ARGUMENT;
}

static void assign_handle(struct event_runtime_handle *handle, game_event_id_t event_id)
{
  if (handle != NULL)
    handle->id = event_id;
}

enum game_scheduler_status
event_runtime_init(const struct game_scheduler_config *config)
{
  enum game_scheduler_status status;

  if (config == NULL)
    return GAME_SCHEDULER_INVALID_ARGUMENT;
  if (runtime_scheduler != NULL)
    return GAME_SCHEDULER_BUSY;
  runtime_scheduler = game_scheduler_create(config, &status);
  return runtime_scheduler != NULL ? GAME_SCHEDULER_OK : status;
}

enum game_scheduler_status event_runtime_shutdown(void)
{
  enum game_scheduler_status status;

  if (runtime_scheduler == NULL)
    return GAME_SCHEDULER_OK;
  status = game_scheduler_destroy(runtime_scheduler);
  if (status == GAME_SCHEDULER_OK)
    runtime_scheduler = NULL;
  return status;
}

bool event_runtime_is_initialized(void)
{
  return runtime_scheduler != NULL;
}

enum game_scheduler_status event_runtime_register_type(
    const struct game_event_type_config *config, game_event_type_id_t *event_type)
{
  if (runtime_required() != GAME_SCHEDULER_OK)
    return GAME_SCHEDULER_INVALID_ARGUMENT;
  return game_scheduler_register_type(runtime_scheduler, config, event_type);
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
    assign_handle(handle, event_id);
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
    assign_handle(handle, event_id);
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
    assign_handle(handle, event_id);
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
    assign_handle(handle, event_id);
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
