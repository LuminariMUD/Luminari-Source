#ifndef GAME_SCHEDULER_H
#define GAME_SCHEDULER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define GAME_SCHEDULER_TICK_MILLISECONDS 100U
#define GAME_SCHEDULER_WHEEL_LEVELS 5U
#define GAME_SCHEDULER_WHEEL_SLOTS 64U
#define GAME_SCHEDULER_DEFAULT_MAX_EVENTS 10000U
#define GAME_SCHEDULER_DEFAULT_MAX_EVENT_TYPES 256U
#define GAME_SCHEDULER_LARGE_ADVANCE_TICKS UINT64_C(4096)

typedef uint64_t game_tick_t;
typedef uint64_t game_event_id_t;
typedef uint32_t game_event_type_id_t;

struct game_scheduler;

enum game_scheduler_status
{
  GAME_SCHEDULER_OK = 0,
  GAME_SCHEDULER_INVALID_ARGUMENT,
  GAME_SCHEDULER_INVALID_TYPE,
  GAME_SCHEDULER_INVALID_PAYLOAD,
  GAME_SCHEDULER_INVALID_DEADLINE,
  GAME_SCHEDULER_CLOCK_REVERSED,
  GAME_SCHEDULER_CAPACITY_REACHED,
  GAME_SCHEDULER_TYPE_CAPACITY_REACHED,
  GAME_SCHEDULER_ALLOCATION_FAILED,
  GAME_SCHEDULER_SHUTTING_DOWN,
  GAME_SCHEDULER_NOT_FOUND,
  GAME_SCHEDULER_BUSY,
  GAME_SCHEDULER_ID_EXHAUSTED,
  GAME_SCHEDULER_INVALID_OWNER,
  GAME_SCHEDULER_OWNER_CAPACITY_REACHED,
  GAME_SCHEDULER_OWNER_TYPE_CAPACITY_REACHED
};

enum game_event_owner_kind
{
  GAME_EVENT_OWNER_NONE = 0,
  GAME_EVENT_OWNER_WORLD,
  GAME_EVENT_OWNER_DESCRIPTOR,
  GAME_EVENT_OWNER_CHARACTER,
  GAME_EVENT_OWNER_ROOM,
  GAME_EVENT_OWNER_REGION,
  GAME_EVENT_OWNER_OBJECT,
  GAME_EVENT_OWNER_ZONE,
  GAME_EVENT_OWNER_ENCOUNTER,
  GAME_EVENT_OWNER_VESSEL,
  GAME_EVENT_OWNER_SERVICE,
  GAME_EVENT_OWNER_KIND_COUNT
};

struct game_event_owner
{
  enum game_event_owner_kind kind;
  uint64_t runtime_id;
  uint64_t generation;
};

enum game_event_state
{
  GAME_EVENT_STATE_CREATED = 0,
  GAME_EVENT_STATE_QUEUED,
  GAME_EVENT_STATE_READY,
  GAME_EVENT_STATE_DISPATCHING,
  GAME_EVENT_STATE_CANCEL_PENDING,
  GAME_EVENT_STATE_COMPLETED,
  GAME_EVENT_STATE_CANCELLED,
  GAME_EVENT_STATE_FAILED
};

enum game_event_location
{
  GAME_EVENT_LOCATION_NONE = 0,
  GAME_EVENT_LOCATION_WHEEL,
  GAME_EVENT_LOCATION_OVERFLOW,
  GAME_EVENT_LOCATION_READY,
  GAME_EVENT_LOCATION_DISPATCHING
};

enum game_event_lateness_policy
{
  GAME_EVENT_LATENESS_RUN_ONCE = 0,
  GAME_EVENT_LATENESS_COALESCE,
  GAME_EVENT_LATENESS_SKIP_MISSED,
  GAME_EVENT_LATENESS_CATCH_UP_BOUNDED
};

enum game_event_result_kind
{
  GAME_EVENT_RESULT_COMPLETE = 0,
  GAME_EVENT_RESULT_RESCHEDULE_AT,
  GAME_EVENT_RESULT_RESCHEDULE_AFTER,
  GAME_EVENT_RESULT_FAILED
};

enum game_event_cancel_result
{
  GAME_EVENT_CANCELLED = 0,
  GAME_EVENT_CANCEL_PENDING,
  GAME_EVENT_CANCEL_NOT_FOUND
};

struct game_event_result
{
  enum game_event_result_kind kind;
  game_tick_t value;
  uint32_t diagnostic_code;
};

struct game_event_context
{
  struct game_scheduler *scheduler;
  game_event_id_t event_id;
  game_event_type_id_t event_type;
  game_tick_t deadline_tick;
  game_tick_t now_tick;
  uint64_t missed_occurrences;
  struct game_event_owner owner;
  void *payload;
};

typedef struct game_event_result (*game_event_handler)(const struct game_event_context *context);
typedef void (*game_event_cleanup)(void *payload);
typedef game_tick_t (*game_scheduler_tick_source)(void *context);
typedef uint64_t (*game_scheduler_usec_source)(void *context);

/*
 * The scheduler takes ownership of a non-NULL payload only after successful admission. Event
 * types that accept owned payloads must provide a cleanup function. Cleanup runs exactly once.
 */

struct game_scheduler_config
{
  size_t max_events;
  size_t max_event_types;
  size_t max_events_per_owner;
  game_scheduler_tick_source tick_now;
  game_scheduler_usec_source monotonic_usec_now;
  void *clock_context;
};

struct game_event_type_config
{
  const char *name;
  game_event_handler handler;
  game_event_cleanup cleanup;
  enum game_event_lateness_policy lateness_policy;
  uint32_t catch_up_limit;
  size_t max_events;
  size_t max_events_per_owner;
  bool requires_owner;
};

struct game_scheduler_budget
{
  size_t max_callbacks;
  uint64_t max_usec;
};

struct game_scheduler_dispatch_report
{
  game_tick_t previous_tick;
  game_tick_t current_tick;
  size_t callbacks;
  size_t completed;
  size_t cancelled;
  size_t failed;
  size_t rescheduled;
  size_t late_callbacks;
  uint64_t missed_occurrences;
  uint64_t skipped_occurrences;
  size_t ready_remaining;
  size_t events_remaining;
  bool callback_budget_exhausted;
  bool time_budget_exhausted;
  bool used_large_advance;
  uint64_t ticks_advanced;
  uint64_t cascade_slots;
  uint64_t cascaded_events;
  uint64_t overflow_promotions;
  uint64_t large_advance_events;
};

struct game_event_snapshot
{
  game_event_id_t event_id;
  game_event_type_id_t event_type;
  enum game_event_state state;
  enum game_event_location location;
  game_tick_t deadline_tick;
  game_tick_t interval_ticks;
  uint64_t insertion_sequence;
  uint32_t wheel_level;
  uint32_t wheel_slot;
  struct game_event_owner owner;
};

struct game_scheduler_stats
{
  game_tick_t current_tick;
  size_t event_count;
  size_t ready_count;
  size_t overflow_count;
  size_t registered_type_count;
  size_t owner_count;
  size_t owner_counts[GAME_EVENT_OWNER_KIND_COUNT];
  uint64_t total_scheduled;
  uint64_t total_callbacks;
  uint64_t total_cancelled;
  uint64_t total_completed;
  uint64_t total_failed;
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

struct game_event_result game_event_result_complete(void);
struct game_event_result game_event_result_reschedule_at(game_tick_t deadline_tick);
struct game_event_result game_event_result_reschedule_after(game_tick_t delay_ticks);
struct game_event_result game_event_result_failed(uint32_t diagnostic_code);
struct game_event_owner game_event_owner_none(void);
bool game_event_owner_is_none(struct game_event_owner owner);
bool game_event_owner_is_valid(struct game_event_owner owner);
bool game_event_owner_equal(struct game_event_owner left, struct game_event_owner right);

struct game_scheduler *game_scheduler_create(const struct game_scheduler_config *config,
                                             enum game_scheduler_status *status);
enum game_scheduler_status game_scheduler_shutdown(struct game_scheduler *scheduler);
enum game_scheduler_status game_scheduler_destroy(struct game_scheduler *scheduler);

enum game_scheduler_status game_scheduler_register_type(struct game_scheduler *scheduler,
                                                        const struct game_event_type_config *config,
                                                        game_event_type_id_t *event_type);

enum game_scheduler_status game_scheduler_schedule_at(struct game_scheduler *scheduler,
                                                      game_event_type_id_t event_type,
                                                      game_tick_t deadline_tick, void *payload,
                                                      game_event_id_t *event_id);
enum game_scheduler_status game_scheduler_schedule_after(struct game_scheduler *scheduler,
                                                         game_event_type_id_t event_type,
                                                         game_tick_t delay_ticks, void *payload,
                                                         game_event_id_t *event_id);
enum game_scheduler_status game_scheduler_schedule_owned_at(
    struct game_scheduler *scheduler, game_event_type_id_t event_type,
    struct game_event_owner owner, game_tick_t deadline_tick, void *payload,
    game_event_id_t *event_id);
enum game_scheduler_status game_scheduler_schedule_owned_after(
    struct game_scheduler *scheduler, game_event_type_id_t event_type,
    struct game_event_owner owner, game_tick_t delay_ticks, void *payload,
    game_event_id_t *event_id);
enum game_event_cancel_result game_scheduler_cancel(struct game_scheduler *scheduler,
                                                    game_event_id_t event_id);
enum game_scheduler_status game_scheduler_cancel_owner(struct game_scheduler *scheduler,
                                                       struct game_event_owner owner,
                                                       size_t *cancelled_count);
enum game_scheduler_status game_scheduler_reschedule_at(struct game_scheduler *scheduler,
                                                        game_event_id_t event_id,
                                                        game_tick_t deadline_tick);
enum game_scheduler_status game_scheduler_reschedule_after(struct game_scheduler *scheduler,
                                                           game_event_id_t event_id,
                                                           game_tick_t delay_ticks);
enum game_scheduler_status game_scheduler_remaining(const struct game_scheduler *scheduler,
                                                    game_event_id_t event_id,
                                                    game_tick_t *remaining_ticks);

enum game_scheduler_status game_scheduler_advance(struct game_scheduler *scheduler,
                                                  const struct game_scheduler_budget *budget,
                                                  struct game_scheduler_dispatch_report *report);

enum game_scheduler_status game_scheduler_next_deadline(const struct game_scheduler *scheduler,
                                                        game_tick_t *deadline_tick,
                                                        bool *has_deadline);
enum game_scheduler_status game_scheduler_inspect(const struct game_scheduler *scheduler,
                                                  game_event_id_t event_id,
                                                  struct game_event_snapshot *snapshot);
enum game_scheduler_status game_scheduler_inspect_owner(
    const struct game_scheduler *scheduler, struct game_event_owner owner,
    struct game_event_snapshot *snapshots, size_t snapshot_capacity, size_t *event_count);
void game_scheduler_get_stats(const struct game_scheduler *scheduler,
                              struct game_scheduler_stats *stats);

#ifdef LUMINARI_CUTEST
enum game_scheduler_status game_scheduler_test_set_sequences(struct game_scheduler *scheduler,
                                                             game_event_id_t next_event_id,
                                                             uint64_t next_insertion_sequence);
#endif

#endif /* GAME_SCHEDULER_H */
