#ifndef EVENT_RUNTIME_H
#define EVENT_RUNTIME_H

#include "event_handle.h"
#include "game_scheduler.h"

/*
 * Process-wide timed gameplay runtime. Event IDs are never reused during a
 * runtime instance, so a stale handle cannot resolve to a later event before
 * that runtime is shut down.
 */
enum game_scheduler_status event_runtime_init(const struct game_scheduler_config *config);
enum game_scheduler_status event_runtime_shutdown(void);
bool event_runtime_is_initialized(void);

enum game_scheduler_status event_runtime_register_type(const struct game_event_type_config *config,
                                                       game_event_type_id_t *event_type);
enum game_scheduler_status event_runtime_registration_checkpoint(size_t *registered_type_count);
enum game_scheduler_status event_runtime_rollback_type_registrations(size_t registered_type_count);
enum game_scheduler_status event_runtime_seal_types(void);
bool event_runtime_types_are_sealed(void);
const char *event_runtime_type_name(game_event_type_id_t event_type);
enum game_scheduler_status event_runtime_find_type(const char *name,
                                                   game_event_type_id_t *event_type);
size_t event_runtime_event_count(void);
enum game_scheduler_status event_runtime_type_live_count(game_event_type_id_t event_type,
                                                         size_t *live_count);

enum game_scheduler_status event_runtime_schedule_at(game_event_type_id_t event_type,
                                                     game_tick_t deadline_tick, void *payload,
                                                     struct event_runtime_handle *handle);
enum game_scheduler_status event_runtime_schedule_after(game_event_type_id_t event_type,
                                                        game_tick_t delay_ticks, void *payload,
                                                        struct event_runtime_handle *handle);
enum game_scheduler_status event_runtime_schedule_owned_at(game_event_type_id_t event_type,
                                                           struct game_event_owner owner,
                                                           game_tick_t deadline_tick, void *payload,
                                                           struct event_runtime_handle *handle);
enum game_scheduler_status event_runtime_schedule_owned_after(game_event_type_id_t event_type,
                                                              struct game_event_owner owner,
                                                              game_tick_t delay_ticks,
                                                              void *payload,
                                                              struct event_runtime_handle *handle);

enum game_event_cancel_result event_runtime_cancel(struct event_runtime_handle handle);
enum game_scheduler_status event_runtime_cancel_owner(struct game_event_owner owner,
                                                      size_t *cancelled_count);
enum game_scheduler_status event_runtime_reschedule_at(struct event_runtime_handle handle,
                                                       game_tick_t deadline_tick);
enum game_scheduler_status event_runtime_reschedule_after(struct event_runtime_handle handle,
                                                          game_tick_t delay_ticks);
enum game_scheduler_status event_runtime_remaining(struct event_runtime_handle handle,
                                                   game_tick_t *remaining_ticks);
bool event_runtime_handle_is_live(struct event_runtime_handle handle);

enum game_scheduler_status event_runtime_advance(const struct game_scheduler_budget *budget,
                                                 struct game_scheduler_dispatch_report *report);
enum game_scheduler_status event_runtime_next_deadline(game_tick_t *deadline_tick,
                                                       bool *has_deadline);
enum game_scheduler_status event_runtime_inspect(struct event_runtime_handle handle,
                                                 struct game_event_snapshot *snapshot);
enum game_scheduler_status event_runtime_inspect_owner(struct game_event_owner owner,
                                                       struct game_event_snapshot *snapshots,
                                                       size_t snapshot_capacity,
                                                       size_t *event_count);
enum game_scheduler_status event_runtime_inspect_all(struct game_event_snapshot *snapshots,
                                                     size_t snapshot_capacity, size_t *event_count);
void event_runtime_get_stats(struct game_scheduler_stats *stats);

#if defined(LUMINARI_CUTEST)
void event_runtime_test_fail_registration_after(size_t successful_registrations);
#endif

#endif /* EVENT_RUNTIME_H */
