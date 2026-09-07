#include "conf.h"
#include "sysdep.h"

#include "activity_manager.h"

#include "actions.h"
#include "comm.h"
#include "db.h"
#include "dgscript/dg_event.h"
#include "domain_event_types.h"
#include "domain_event_world.h"
#include "event_runtime.h"
#include "interpreter.h"
#include "utils.h"

#define ACTIVITY_NAME_SIZE 64U

struct activity_timer_payload
{
  struct domain_entity_handle actor;
  uint64_t activity_id;
  struct event_runtime_handle event_handle;
};

struct primary_activity
{
  uint64_t id;
  struct domain_entity_handle actor;
  struct domain_entity_handle target;
  enum primary_activity_type type;
  enum primary_activity_state state;
  char display_name[ACTIVITY_NAME_SIZE];
  uint32_t capabilities;
  uint32_t traits;
  enum primary_activity_progress_model progress_model;
  enum primary_activity_progress_owner progress_owner;
  uint32_t completed_steps;
  uint32_t total_steps;
  long step_interval;
  long remaining_delay;
  int combat_actions_required;
  unsigned int delayed_combat_turns;
  enum primary_activity_response movement_response;
  enum primary_activity_response damage_response;
  enum primary_activity_response combat_response;
  enum primary_activity_response target_loss_response;
  enum primary_activity_response command_response;
  long delay_pulses;
  bool wall_clock;
  bool cannot_pause;
  primary_activity_timed_step timed_step;
  primary_activity_damage_check damage_check;
  primary_activity_recheck recheck;
  primary_activity_progress progress;
  primary_activity_completion complete;
  primary_activity_ended ended;
  primary_activity_context_cleanup cleanup_context;
  void *context;
  struct domain_event_subscription_handle target_moved;
  struct domain_event_subscription_handle target_died;
  struct event_runtime_handle timer_handle;
  bool timer_dispatching;
  bool combat_clock;
  bool paused_by_combat;
  struct primary_activity *previous;
  struct primary_activity *next;
};

static struct domain_event_bus *activity_bus;
static struct primary_activity *activity_head;
static struct primary_activity_stats activity_stats;
static uint64_t next_activity_id = 1U;
static bool initialized;
static bool shutting_down;
static bool managed_camp = true;
static game_event_type_id_t primary_activity_event_type;
#ifdef LUMINARI_CUTEST
static bool test_camp_selection;
static bool test_managed_camp;
#endif

static bool camp_mode_is_managed(const char *value)
{
  return value == NULL || *value == '\0' ||
         (strcasecmp(value, "legacy") != 0 && strcasecmp(value, "off") != 0 &&
          strcmp(value, "0") != 0);
}

static struct char_data *resolve_actor(struct domain_entity_handle handle)
{
  if (activity_bus == NULL)
    return NULL;
  return domain_event_resolve(activity_bus, handle, DOMAIN_ENTITY_CHARACTER);
}

static void *resolve_target(struct domain_entity_handle handle)
{
  if (activity_bus == NULL || !domain_entity_handle_is_valid(handle))
    return NULL;
  return domain_event_resolve(activity_bus, handle, handle.kind);
}

static struct game_event_owner activity_owner(struct domain_entity_handle actor)
{
  struct game_event_owner owner = game_event_owner_none();

  if (!domain_entity_handle_is_valid(actor) || actor.kind != DOMAIN_ENTITY_CHARACTER)
    return owner;
  owner.kind = GAME_EVENT_OWNER_CHARACTER;
  owner.runtime_id = actor.runtime_id;
  owner.generation = actor.generation;
  return owner;
}

static void activity_list_add(struct primary_activity *activity)
{
  activity->previous = NULL;
  activity->next = activity_head;
  if (activity_head != NULL)
    activity_head->previous = activity;
  activity_head = activity;
  activity_stats.active++;
  if (activity_stats.active > activity_stats.high_water)
    activity_stats.high_water = activity_stats.active;
}

static void activity_list_remove(struct primary_activity *activity)
{
  if (activity->previous != NULL)
    activity->previous->next = activity->next;
  else if (activity_head == activity)
    activity_head = activity->next;
  if (activity->next != NULL)
    activity->next->previous = activity->previous;
  activity->previous = NULL;
  activity->next = NULL;
  if (activity_stats.active > 0U)
    activity_stats.active--;
}

static struct primary_activity *find_activity_by_id(uint64_t id)
{
  struct primary_activity *activity;

  for (activity = activity_head; activity != NULL; activity = activity->next)
    if (activity->id == id)
      return activity;
  return NULL;
}

static void publish_transition(struct domain_entity_handle actor, enum primary_activity_type type,
                               enum primary_activity_state previous_state,
                               enum primary_activity_state current_state, uint64_t id,
                               enum primary_activity_end_reason reason)
{
  struct domain_activity_transitioned event;
  struct domain_event_topic topic = {DOMAIN_EVENT_TOPIC_SUBJECT, actor};

  if (activity_bus == NULL)
    return;
  event.activity_id = id;
  event.end_reason = (uint32_t)reason;
  event.actor = actor;
  event.activity_type = (uint32_t)type;
  event.previous_state = (uint32_t)previous_state;
  event.current_state = (uint32_t)current_state;
  (void)DOMAIN_EVENT_PUBLISH_ROUTED(activity_bus, DOMAIN_EVENT_ACTIVITY_TRANSITIONED, &topic, 1U,
                                    &event);
}

static void activity_timer_cleanup(void *event_obj)
{
  struct activity_timer_payload *payload;
  struct char_data *actor;
  struct primary_activity *activity;

  payload = event_obj;
  if (payload == NULL)
    return;
  actor = resolve_actor(payload->actor);
  activity = actor != NULL ? actor->primary_activity : NULL;
  if (activity != NULL && activity->id == payload->activity_id &&
      event_runtime_handles_equal(activity->timer_handle, payload->event_handle))
    activity->timer_handle = EVENT_RUNTIME_HANDLE_NONE;
  free(payload);
}

static bool runtime_handle_matches(struct event_runtime_handle handle,
                                   const struct game_event_context *context)
{
  return context != NULL && handle.id == context->event_id;
}

static long activity_timer_remaining(struct event_runtime_handle handle)
{
  game_tick_t remaining;

  if (event_runtime_remaining(handle, &remaining) != GAME_SCHEDULER_OK)
    return 1L;
  return remaining > (game_tick_t)LONG_MAX ? LONG_MAX : (long)remaining;
}

static void detach_timer(struct primary_activity *activity, bool preserve_delay)
{
  struct event_runtime_handle handle;
  long remaining;

  if (activity == NULL || event_runtime_handle_is_none(activity->timer_handle))
    return;
  handle = activity->timer_handle;
  remaining = activity_timer_remaining(handle);
  if (preserve_delay)
    activity->remaining_delay = MAX(1L, remaining);
  activity->timer_handle = EVENT_RUNTIME_HANDLE_NONE;
  (void)event_runtime_cancel(handle);
}

static void finish_activity(struct primary_activity *activity,
                            enum primary_activity_state terminal_state,
                            enum primary_activity_end_reason reason, bool notify)
{
  struct char_data *actor;
  void *target;
  primary_activity_completion complete;
  primary_activity_ended ended;
  primary_activity_context_cleanup cleanup_context;
  void *context;
  char name[ACTIVITY_NAME_SIZE];
  struct domain_entity_handle actor_handle;
  enum primary_activity_type type;
  enum primary_activity_state previous_state;
  uint64_t id;

  if (activity == NULL)
    return;
  id = activity->id;
  actor_handle = activity->actor;
  type = activity->type;
  previous_state = activity->state;
  actor = resolve_actor(activity->actor);
  target = resolve_target(activity->target);
  complete = activity->complete;
  ended = activity->ended;
  cleanup_context = activity->cleanup_context;
  context = activity->context;
  strlcpy(name, activity->display_name, sizeof(name));

  detach_timer(activity, false);
  (void)domain_event_unsubscribe(activity_bus, activity->target_moved);
  (void)domain_event_unsubscribe(activity_bus, activity->target_died);
  activity->state = terminal_state;
  if (actor != NULL && actor->primary_activity == activity)
    actor->primary_activity = NULL;
  activity_list_remove(activity);
  if (terminal_state == PRIMARY_ACTIVITY_STATE_COMPLETED)
    activity_stats.completed++;
  else
    activity_stats.cancelled++;
  free(activity);

  if (notify && actor != NULL && terminal_state == PRIMARY_ACTIVITY_STATE_CANCELLED)
    send_to_char(actor, "You stop %s (%s).\r\n", name, primary_activity_end_reason_name(reason));
  if (terminal_state == PRIMARY_ACTIVITY_STATE_COMPLETED && complete != NULL && actor != NULL)
    complete(actor, target, context);
  else if (terminal_state == PRIMARY_ACTIVITY_STATE_CANCELLED && ended != NULL && actor != NULL)
    ended(actor, reason, context);
  if (cleanup_context != NULL)
    cleanup_context(context);
  publish_transition(actor_handle, type, previous_state, terminal_state, id, reason);
}

static bool activity_recheck_now(struct primary_activity *activity,
                                 enum primary_activity_end_reason reason)
{
  struct char_data *actor;
  void *target;
  uint64_t id;
  struct domain_entity_handle actor_handle;
  bool allowed;

  if (activity == NULL)
    return false;
  id = activity->id;
  actor_handle = activity->actor;
  actor = resolve_actor(activity->actor);
  target = resolve_target(activity->target);
  if (actor == NULL || target == NULL)
  {
    finish_activity(activity, PRIMARY_ACTIVITY_STATE_CANCELLED, reason, actor != NULL);
    return false;
  }
  allowed = activity->recheck == NULL || activity->recheck(actor, target, activity->context);
  actor = resolve_actor(actor_handle);
  activity = actor != NULL ? actor->primary_activity : NULL;
  if (activity == NULL || activity->id != id)
    return false;
  if (actor == NULL || actor->primary_activity != activity || actor->primary_activity->id != id ||
      activity->state != PRIMARY_ACTIVITY_STATE_ACTIVE)
    return false;
  if (!allowed)
  {
    finish_activity(activity, PRIMARY_ACTIVITY_STATE_CANCELLED, reason, true);
    return false;
  }
  return true;
}

static bool schedule_timer(struct primary_activity *activity, long delay);

static bool pause_activity_internal(struct primary_activity *activity, bool notify,
                                    bool combat_pause)
{
  struct char_data *actor;
  struct domain_entity_handle actor_handle;
  enum primary_activity_type type;
  enum primary_activity_state previous_state;
  char name[ACTIVITY_NAME_SIZE];

  if (activity == NULL || activity->cannot_pause ||
      activity->state != PRIMARY_ACTIVITY_STATE_ACTIVE)
    return false;
  actor_handle = activity->actor;
  type = activity->type;
  previous_state = activity->state;
  strlcpy(name, activity->display_name, sizeof(name));
  actor = resolve_actor(activity->actor);
  detach_timer(activity, true);
  activity->combat_clock = false;
  activity->paused_by_combat = combat_pause;
  activity->state = PRIMARY_ACTIVITY_STATE_PAUSED;
  activity_stats.paused++;
  if (notify && actor != NULL)
    send_to_char(actor, "You pause %s.\r\n", name);
  publish_transition(actor_handle, type, previous_state, PRIMARY_ACTIVITY_STATE_PAUSED,
                     activity->id, PRIMARY_ACTIVITY_END_INTERNAL);
  return true;
}

static bool resume_activity_internal(struct primary_activity *activity, bool notify)
{
  struct char_data *actor;
  struct domain_entity_handle actor_handle;
  enum primary_activity_type type;
  enum primary_activity_state previous_state;
  char name[ACTIVITY_NAME_SIZE];
  long delay;

  if (activity == NULL || activity->state != PRIMARY_ACTIVITY_STATE_PAUSED)
    return false;
  actor_handle = activity->actor;
  type = activity->type;
  previous_state = activity->state;
  strlcpy(name, activity->display_name, sizeof(name));
  actor = resolve_actor(activity->actor);
  if (actor == NULL)
    return false;
  if (FIGHTING(actor) != NULL && activity->combat_response == PRIMARY_ACTIVITY_RESPONSE_PAUSE)
  {
    if (notify)
      send_to_char(actor, "You cannot resume %s during combat.\r\n", name);
    return false;
  }
  activity->paused_by_combat = false;
  activity->state = PRIMARY_ACTIVITY_STATE_ACTIVE;
  if (FIGHTING(actor) != NULL && !activity->wall_clock)
  {
    activity->combat_clock = true;
  }
  else
  {
    delay = activity->remaining_delay > 0 ? activity->remaining_delay : activity->step_interval;
    if (!schedule_timer(activity, delay))
    {
      activity->state = previous_state;
      finish_activity(activity, PRIMARY_ACTIVITY_STATE_CANCELLED, PRIMARY_ACTIVITY_END_INTERNAL,
                      true);
      return false;
    }
  }
  activity_stats.resumed++;
  if (notify)
    send_to_char(actor, "You resume %s.\r\n", name);
  publish_transition(actor_handle, type, previous_state, PRIMARY_ACTIVITY_STATE_ACTIVE,
                     activity->id, PRIMARY_ACTIVITY_END_INTERNAL);
  return true;
}

static bool delay_activity(struct primary_activity *activity, long delay, bool notify)
{
  struct char_data *actor;
  long remaining;

  if (activity == NULL || delay < 1)
    return false;
  actor = resolve_actor(activity->actor);
  if (activity->state == PRIMARY_ACTIVITY_STATE_PAUSED)
  {
    activity->remaining_delay = MIN(LONG_MAX - delay, activity->remaining_delay) + delay;
  }
  else if (activity->combat_clock)
  {
    activity->delayed_combat_turns++;
  }
  else if (!event_runtime_handle_is_none(activity->timer_handle))
  {
    remaining = activity_timer_remaining(activity->timer_handle);
    if (remaining > LONG_MAX - delay)
      remaining = LONG_MAX;
    else
      remaining += delay;
    detach_timer(activity, false);
    if (!schedule_timer(activity, remaining))
    {
      finish_activity(activity, PRIMARY_ACTIVITY_STATE_CANCELLED, PRIMARY_ACTIVITY_END_INTERNAL,
                      true);
      return false;
    }
  }
  activity_stats.delayed++;
  if (notify && actor != NULL)
    send_to_char(actor, "The interruption delays %s.\r\n", activity->display_name);
  return true;
}

static bool apply_response(struct primary_activity *activity,
                           enum primary_activity_response response,
                           enum primary_activity_end_reason reason, bool notify)
{
  switch (response)
  {
  case PRIMARY_ACTIVITY_RESPONSE_IGNORE:
    return true;
  case PRIMARY_ACTIVITY_RESPONSE_CANCEL:
    finish_activity(activity, PRIMARY_ACTIVITY_STATE_CANCELLED, reason, notify);
    return false;
  case PRIMARY_ACTIVITY_RESPONSE_PAUSE:
    return pause_activity_internal(activity, notify, false);
  case PRIMARY_ACTIVITY_RESPONSE_DELAY:
    return delay_activity(activity, MAX(1L, activity->delay_pulses), notify);
  case PRIMARY_ACTIVITY_RESPONSE_RECHECK:
    return activity_recheck_now(activity, PRIMARY_ACTIVITY_END_RECHECK_FAILED);
  case PRIMARY_ACTIVITY_RESPONSE_REJECT:
  default:
    return false;
  }
}

static bool consume_combat_actions(struct char_data *actor, int actions_required)
{
  if (!command_actions_available(actor, actions_required))
    return false;
  if (IS_SET(actions_required, ACTION_STANDARD))
    start_action_cooldown(actor, atSTANDARD, 6 RL_SEC);
  if (IS_SET(actions_required, ACTION_MOVE))
    start_action_cooldown(actor, atMOVE, 6 RL_SEC);
  if (IS_SET(actions_required, ACTION_SWIFT))
    start_action_cooldown(actor, atSWIFT, 6 RL_SEC);
  return true;
}

static bool advance_activity(struct primary_activity *activity, bool recheck)
{
  struct char_data *actor;
  void *target;
  uint64_t id;
  struct domain_entity_handle actor_handle;

  if (recheck && !activity_recheck_now(activity, PRIMARY_ACTIVITY_END_RECHECK_FAILED))
    return false;
  actor = resolve_actor(activity->actor);
  target = resolve_target(activity->target);
  if (actor == NULL || target == NULL)
    return false;
  id = activity->id;
  actor_handle = activity->actor;
  if (activity->timed_step != NULL)
  {
    long delay = activity->timed_step(actor, target, activity->context);

    actor = resolve_actor(actor_handle);
    activity = actor != NULL ? actor->primary_activity : NULL;
    if (activity == NULL || activity->id != id)
      return false;
    if (delay <= 0)
    {
      finish_activity(activity, PRIMARY_ACTIVITY_STATE_COMPLETED, PRIMARY_ACTIVITY_END_COMPLETED,
                      false);
      return false;
    }
    activity->step_interval = delay;
    if (activity->completed_steps + 1U < activity->total_steps)
      activity->completed_steps++;
    return true;
  }
  if (activity->completed_steps < activity->total_steps)
    activity->completed_steps++;
  if (activity->progress != NULL && activity->completed_steps < activity->total_steps)
    activity->progress(actor, target, activity->completed_steps, activity->total_steps,
                       activity->context);
  actor = resolve_actor(actor_handle);
  if (actor == NULL || actor->primary_activity == NULL || actor->primary_activity->id != id ||
      actor->primary_activity->state != PRIMARY_ACTIVITY_STATE_ACTIVE)
    return false;
  if (activity->completed_steps >= activity->total_steps)
  {
    finish_activity(activity, PRIMARY_ACTIVITY_STATE_COMPLETED, PRIMARY_ACTIVITY_END_COMPLETED,
                    false);
    return false;
  }
  return true;
}

static struct game_event_result
primary_activity_timer(const struct game_event_context *event_context)
{
  struct activity_timer_payload *payload = event_context != NULL ? event_context->payload : NULL;
  struct char_data *actor;
  struct primary_activity *activity;
  long next_delay;

  if (payload == NULL)
    return game_event_result_complete();
  actor = resolve_actor(payload->actor);
  activity = actor != NULL ? actor->primary_activity : NULL;
  if (activity == NULL || activity->id != payload->activity_id ||
      activity->state != PRIMARY_ACTIVITY_STATE_ACTIVE || activity->combat_clock)
  {
    activity_stats.stale_callbacks++;
    event_note_stale_owner_outcome();
    return game_event_result_complete();
  }
  if (!runtime_handle_matches(activity->timer_handle, event_context))
    return game_event_result_complete();
  activity->timer_handle = EVENT_RUNTIME_HANDLE_NONE;
  activity->timer_dispatching = true;
  if (!advance_activity(activity, true))
  {
    actor = resolve_actor(payload->actor);
    activity = actor != NULL ? actor->primary_activity : NULL;
    if (activity != NULL && activity->id == payload->activity_id)
      activity->timer_dispatching = false;
    return game_event_result_complete();
  }
  actor = resolve_actor(payload->actor);
  activity = actor != NULL ? actor->primary_activity : NULL;
  if (activity != NULL && activity->id == payload->activity_id)
    activity->timer_dispatching = false;
  if (activity == NULL || activity->id != payload->activity_id ||
      activity->state != PRIMARY_ACTIVITY_STATE_ACTIVE || activity->combat_clock)
    return game_event_result_complete();
  next_delay = activity->step_interval;
  activity->remaining_delay = next_delay;
  activity->timer_handle = payload->event_handle;
  return game_event_result_reschedule_after((game_tick_t)next_delay);
}

static bool register_primary_activity_event_type(void)
{
  struct game_event_type_config config;
  const char *registered_name;
  enum game_scheduler_status status;

  if (!event_runtime_is_initialized())
    return false;
  registered_name = event_runtime_type_name(primary_activity_event_type);
  if (registered_name != NULL && !strcmp(registered_name, "activity.primary.step"))
    return true;
  primary_activity_event_type = 0U;
  memset(&config, 0, sizeof(config));
  config.name = "activity.primary.step";
  config.handler = primary_activity_timer;
  config.cleanup = activity_timer_cleanup;
  config.lateness_policy = GAME_EVENT_LATENESS_RUN_ONCE;
  config.max_events = 65536U;
  config.max_events_per_owner = 1U;
  config.requires_owner = true;
  status = event_runtime_register_type(&config, &primary_activity_event_type);
  if (status != GAME_SCHEDULER_OK)
  {
    log("SYSERR: unable to register native event type 'activity.primary.step' (status %d).",
        status);
    return false;
  }
  return true;
}

static bool schedule_timer(struct primary_activity *activity, long delay)
{
  struct activity_timer_payload *payload;
  struct game_event_owner owner;
  struct event_runtime_handle handle = EVENT_RUNTIME_HANDLE_NONE;

  if (activity == NULL || !event_runtime_handle_is_none(activity->timer_handle) ||
      activity->timer_dispatching || activity->state != PRIMARY_ACTIVITY_STATE_ACTIVE)
    return false;
  owner = activity_owner(activity->actor);
  if (!game_event_owner_is_valid(owner))
    return false;
  payload = calloc(1U, sizeof(*payload));
  if (payload == NULL)
    return false;
  payload->actor = activity->actor;
  payload->activity_id = activity->id;
  if (event_runtime_schedule_owned_after(primary_activity_event_type, owner,
                                         (game_tick_t)MAX(1L, delay), payload,
                                         &handle) != GAME_SCHEDULER_OK)
  {
    free(payload);
    return false;
  }
  payload->event_handle = handle;
  activity->timer_handle = handle;
  activity->remaining_delay = MAX(1L, delay);
  return true;
}

static void handle_character_moved(const struct domain_event_context *context,
                                   void *handler_context)
{
  const struct domain_character_moved *event = context->payload;
  struct char_data *actor;
  struct primary_activity *activity;

  (void)handler_context;
  if (domain_entity_handle_equal(event->from_room, event->to_room))
    return;
  actor = resolve_actor(event->character);
  activity = actor != NULL ? actor->primary_activity : NULL;
  if (activity != NULL)
    (void)apply_response(activity, activity->movement_response, PRIMARY_ACTIVITY_END_MOVED, true);
}

static void handle_character_damaged(const struct domain_event_context *context,
                                     void *handler_context)
{
  const struct domain_character_damaged *event = context->payload;
  struct char_data *actor;
  struct primary_activity *activity;

  (void)handler_context;
  if (event->amount <= 0)
    return;
  actor = resolve_actor(event->target);
  activity = actor != NULL ? actor->primary_activity : NULL;
  if (activity != NULL && activity->damage_check != NULL)
  {
    uint64_t id = activity->id;
    bool allowed = activity->damage_check(actor, event, activity->context);

    actor = resolve_actor(event->target);
    if (actor == NULL || actor->primary_activity == NULL || actor->primary_activity->id != id)
      return;
    if (!allowed)
      finish_activity(activity, PRIMARY_ACTIVITY_STATE_CANCELLED, PRIMARY_ACTIVITY_END_DAMAGED,
                      true);
  }
  else if (activity != NULL)
    (void)apply_response(activity, activity->damage_response, PRIMARY_ACTIVITY_END_DAMAGED, true);
}

static void handle_character_died(const struct domain_event_context *context, void *handler_context)
{
  const struct domain_character_died *event = context->payload;
  struct char_data *actor;

  (void)handler_context;
  actor = resolve_actor(event->character);
  if (actor != NULL && actor->primary_activity != NULL)
    finish_activity(actor->primary_activity, PRIMARY_ACTIVITY_STATE_CANCELLED,
                    PRIMARY_ACTIVITY_END_DIED, false);
}

static void handle_entity_extracted(const struct domain_event_context *context,
                                    void *handler_context)
{
  const struct domain_entity_extracted *event = context->payload;
  struct primary_activity *activity;
  uint64_t *ids;
  size_t count = 0U;
  size_t index;

  (void)handler_context;
  ids = calloc(MAX(1U, activity_stats.active), sizeof(*ids));
  if (ids == NULL)
    return;
  for (activity = activity_head; activity != NULL; activity = activity->next)
  {
    if (domain_entity_handle_equal(activity->actor, event->entity) ||
        domain_entity_handle_equal(activity->target, event->entity))
      ids[count++] = activity->id;
  }
  for (index = 0U; index < count; index++)
  {
    activity = find_activity_by_id(ids[index]);
    if (activity == NULL)
      continue;
    if (domain_entity_handle_equal(activity->actor, event->entity))
      finish_activity(activity, PRIMARY_ACTIVITY_STATE_CANCELLED, PRIMARY_ACTIVITY_END_EXTRACTED,
                      false);
    else if (domain_entity_handle_equal(activity->target, event->entity))
      (void)apply_response(activity, activity->target_loss_response,
                           PRIMARY_ACTIVITY_END_TARGET_LOST, true);
  }
  free(ids);
}

static void enter_combat_clock(struct primary_activity *activity)
{
  if (activity == NULL || activity->state != PRIMARY_ACTIVITY_STATE_ACTIVE)
    return;
  detach_timer(activity, true);
  activity->combat_clock = true;
}

static void handle_combat_state_changed(const struct domain_event_context *context,
                                        void *handler_context)
{
  const struct domain_combat_state_changed *event = context->payload;
  struct char_data *actor;
  struct primary_activity *activity;

  (void)handler_context;
  actor = resolve_actor(event->character);
  activity = actor != NULL ? actor->primary_activity : NULL;
  if (activity == NULL || activity->wall_clock)
    return;
  if (event->in_combat)
  {
    if (activity->combat_response == PRIMARY_ACTIVITY_RESPONSE_PAUSE)
    {
      activity->paused_by_combat = true;
      (void)pause_activity_internal(activity, true, true);
    }
    else
    {
      uint64_t id = activity->id;

      if (apply_response(activity, activity->combat_response, PRIMARY_ACTIVITY_END_COMMAND, true) &&
          actor->primary_activity == activity && actor->primary_activity->id == id)
        enter_combat_clock(activity);
    }
  }
  else if (activity->state == PRIMARY_ACTIVITY_STATE_PAUSED && activity->paused_by_combat)
  {
    (void)resume_activity_internal(activity, true);
  }
  else if (activity->state == PRIMARY_ACTIVITY_STATE_ACTIVE && activity->combat_clock)
  {
    activity->combat_clock = false;
    if (!schedule_timer(activity, activity->remaining_delay > 0 ? activity->remaining_delay
                                                                : activity->step_interval))
      finish_activity(activity, PRIMARY_ACTIVITY_STATE_CANCELLED, PRIMARY_ACTIVITY_END_INTERNAL,
                      true);
  }
}

enum domain_event_status primary_activity_manager_init(struct domain_event_bus *bus)
{
  static const struct domain_event_handler_config handlers[] = {
      {DOMAIN_EVENT_CHARACTER_MOVED, "activity.movement", 100, handle_character_moved, NULL},
      {DOMAIN_EVENT_CHARACTER_DAMAGED, "activity.damage", 100, handle_character_damaged, NULL},
      {DOMAIN_EVENT_CHARACTER_DIED, "activity.death", 100, handle_character_died, NULL},
      {DOMAIN_EVENT_ENTITY_EXTRACTED, "activity.extraction", 100, handle_entity_extracted, NULL},
      {DOMAIN_EVENT_COMBAT_STATE_CHANGED, "activity.combat", 100, handle_combat_state_changed,
       NULL},
  };
  const char *value;
  size_t index;
  enum domain_event_status status;

  if (bus == NULL)
    return DOMAIN_EVENT_INVALID_ARGUMENT;
  if (initialized)
    return DOMAIN_EVENT_BUSY;
  memset(&activity_stats, 0, sizeof(activity_stats));
  activity_bus = bus;
  shutting_down = false;
  value = getenv("LUMINARI_CAMP_ACTIVITY");
  managed_camp = camp_mode_is_managed(value);
#ifdef LUMINARI_CUTEST
  if (test_camp_selection)
    managed_camp = test_managed_camp;
#endif
  if (event_backend_current() == EVENT_BACKEND_GAME_SCHEDULER &&
      !register_primary_activity_event_type())
    return DOMAIN_EVENT_BUSY;
  for (index = 0U; index < sizeof(handlers) / sizeof(handlers[0]); index++)
  {
    status = domain_event_register_handler(bus, &handlers[index]);
    if (status != DOMAIN_EVENT_OK)
    {
      activity_bus = NULL;
      return status;
    }
  }
  initialized = true;
  log("Primary activity manager initialized; camp mode: %s.", managed_camp ? "managed" : "legacy");
  return DOMAIN_EVENT_OK;
}

void primary_activity_manager_shutdown(void)
{
  struct primary_activity *activity;

  if (!initialized && activity_head == NULL)
  {
    activity_bus = NULL;
    return;
  }
  shutting_down = true;
  while ((activity = activity_head) != NULL)
    finish_activity(activity, PRIMARY_ACTIVITY_STATE_CANCELLED, PRIMARY_ACTIVITY_END_SHUTDOWN,
                    false);
  initialized = false;
  activity_bus = NULL;
  shutting_down = false;
}

static void activity_target_changed(const struct domain_event_context *context, void *data)
{
  struct primary_activity *activity = data;

  if (context->type == DOMAIN_EVENT_CHARACTER_DIED)
    finish_activity(activity, PRIMARY_ACTIVITY_STATE_CANCELLED, PRIMARY_ACTIVITY_END_TARGET_LOST,
                    true);
  else if (activity->state == PRIMARY_ACTIVITY_STATE_ACTIVE)
    (void)activity_recheck_now(activity, PRIMARY_ACTIVITY_END_TARGET_LOST);
}

static bool watch_activity_target(struct primary_activity *activity)
{
  struct domain_event_subscription_config config = {0};

  if (activity->target.kind != DOMAIN_ENTITY_CHARACTER ||
      domain_entity_handle_equal(activity->target, activity->actor))
    return true;
  config.topic.role = DOMAIN_EVENT_TOPIC_SUBJECT;
  config.topic.entity = activity->target;
  config.owner = activity->actor;
  config.handler = activity_target_changed;
  config.handler_context = activity;
  config.type = DOMAIN_EVENT_CHARACTER_MOVED;
  config.identity = "activity.target.moved";
  if (domain_event_subscribe(activity_bus, &config, &activity->target_moved) != DOMAIN_EVENT_OK)
    return false;
  config.type = DOMAIN_EVENT_CHARACTER_DIED;
  config.identity = "activity.target.died";
  return domain_event_subscribe(activity_bus, &config, &activity->target_died) == DOMAIN_EVENT_OK;
}

bool primary_activity_start(struct char_data *actor, struct domain_entity_handle target,
                            const struct primary_activity_definition *definition)
{
  struct primary_activity *activity;
  struct domain_entity_handle actor_handle;
  void *target_pointer;
  bool starts_in_combat;
  struct domain_casting_started casting = {0};
  struct domain_event_topic casting_topics[2];

  if (!initialized || shutting_down || actor == NULL || definition == NULL ||
      definition->type <= PRIMARY_ACTIVITY_NONE ||
      definition->type >= PRIMARY_ACTIVITY_TYPE_COUNT || definition->display_name == NULL ||
      definition->total_steps == 0U || definition->step_interval < 1 ||
      actor->primary_activity != NULL || !domain_entity_handle_is_valid(target))
    return false;
  target_pointer = resolve_target(target);
  if (target_pointer == NULL)
    return false;
  actor_handle = domain_event_character_handle(actor);
  if (!domain_entity_handle_is_valid(actor_handle) || resolve_actor(actor_handle) != actor)
    return false;
  starts_in_combat = FIGHTING(actor) != NULL && !definition->wall_clock;
  if (starts_in_combat)
  {
    if (definition->combat_response == PRIMARY_ACTIVITY_RESPONSE_CANCEL ||
        definition->combat_response == PRIMARY_ACTIVITY_RESPONSE_REJECT)
      return false;
    if (definition->combat_response == PRIMARY_ACTIVITY_RESPONSE_RECHECK &&
        definition->recheck != NULL &&
        !definition->recheck(actor, target_pointer, definition->context))
      return false;
    if (actor->primary_activity != NULL || resolve_actor(actor_handle) != actor ||
        resolve_target(target) != target_pointer)
      return false;
  }
  activity = calloc(1U, sizeof(*activity));
  if (activity == NULL)
    return false;
  activity->id = next_activity_id++;
  if (next_activity_id == 0U)
    next_activity_id = 1U;
  activity->actor = actor_handle;
  activity->target = target;
  activity->type = definition->type;
  activity->state = PRIMARY_ACTIVITY_STATE_ACTIVE;
  strlcpy(activity->display_name, definition->display_name, sizeof(activity->display_name));
  activity->capabilities = definition->capabilities;
  activity->traits = definition->traits;
  activity->progress_model = definition->progress_model;
  activity->progress_owner = definition->progress_owner;
  activity->total_steps = definition->total_steps;
  activity->step_interval = definition->step_interval;
  activity->remaining_delay = definition->step_interval;
  activity->combat_actions_required = definition->combat_actions_required;
  activity->movement_response = definition->movement_response;
  activity->damage_response = definition->damage_response;
  activity->combat_response = definition->combat_response;
  activity->target_loss_response = definition->target_loss_response;
  activity->command_response = definition->command_response;
  activity->delay_pulses = MAX(1L, definition->delay_pulses);
  activity->wall_clock = definition->wall_clock;
  activity->cannot_pause = definition->cannot_pause;
  activity->timed_step = definition->timed_step;
  activity->damage_check = definition->damage_check;
  activity->recheck = definition->recheck;
  activity->progress = definition->progress;
  activity->complete = definition->complete;
  activity->ended = definition->ended;
  activity->cleanup_context = definition->cleanup_context;
  activity->context = definition->context;
  if (starts_in_combat)
  {
    if (activity->combat_response == PRIMARY_ACTIVITY_RESPONSE_PAUSE)
    {
      activity->state = PRIMARY_ACTIVITY_STATE_PAUSED;
      activity->paused_by_combat = true;
    }
    else
    {
      activity->combat_clock = true;
      if (activity->combat_response == PRIMARY_ACTIVITY_RESPONSE_DELAY)
        activity->delayed_combat_turns = 1U;
    }
  }
  else if (!schedule_timer(activity, activity->step_interval))
  {
    free(activity);
    return false;
  }
  actor->primary_activity = activity;
  activity_list_add(activity);
  if (definition->watch_target && !watch_activity_target(activity))
  {
    finish_activity(activity, PRIMARY_ACTIVITY_STATE_CANCELLED, PRIMARY_ACTIVITY_END_INTERNAL,
                    false);
    return false;
  }
  activity_stats.started++;
  if (activity->type == PRIMARY_ACTIVITY_CASTING)
  {
    casting.cast_id = activity->id;
    casting.caster = actor_handle;
    casting.target = target;
    casting.room = domain_event_room_handle(IN_ROOM(actor));
    casting.spellnum = CASTING_SPELLNUM(actor);
    casting.casting_class = CASTING_CLASS(actor);
  }
  if (activity->state == PRIMARY_ACTIVITY_STATE_PAUSED)
    activity_stats.paused++;
  publish_transition(actor_handle, definition->type, PRIMARY_ACTIVITY_STATE_NONE, activity->state,
                     activity->id, PRIMARY_ACTIVITY_END_INTERNAL);
  /* Transition observers may have cancelled or replaced the activity. */
  actor = resolve_actor(actor_handle);
  if (casting.cast_id != 0U && actor != NULL && actor->primary_activity != NULL &&
      actor->primary_activity->id == casting.cast_id)
  {
    casting_topics[0] = (struct domain_event_topic){DOMAIN_EVENT_TOPIC_SUBJECT, actor_handle};
    casting_topics[1] = (struct domain_event_topic){DOMAIN_EVENT_TOPIC_SOURCE, casting.room};
    (void)DOMAIN_EVENT_PUBLISH_ROUTED(activity_bus, DOMAIN_EVENT_CASTING_STARTED, casting_topics,
                                      2U, &casting);
  }
  return true;
}

bool primary_activity_cancel(struct char_data *actor, enum primary_activity_end_reason reason,
                             bool notify)
{
  if (actor == NULL || actor->primary_activity == NULL)
    return false;
  finish_activity(actor->primary_activity, PRIMARY_ACTIVITY_STATE_CANCELLED, reason, notify);
  return true;
}

bool primary_activity_cancel_id(struct char_data *actor, uint64_t activity_id,
                                enum primary_activity_end_reason reason, bool notify)
{
  if (actor == NULL || actor->primary_activity == NULL || activity_id == 0U ||
      actor->primary_activity->id != activity_id)
    return false;
  return primary_activity_cancel(actor, reason, notify);
}

bool primary_activity_pause(struct char_data *actor, bool notify)
{
  return actor != NULL && pause_activity_internal(actor->primary_activity, notify, false);
}

bool primary_activity_resume(struct char_data *actor, bool notify)
{
  return actor != NULL && resume_activity_internal(actor->primary_activity, notify);
}

bool primary_activity_command_admit(struct char_data *actor, const char *command,
                                    uint32_t command_capabilities, bool informational,
                                    bool activity_control)
{
  struct primary_activity *activity;
  uint32_t conflicts;

  if (actor == NULL || (activity = actor->primary_activity) == NULL || informational ||
      activity_control)
    return true;
  conflicts = activity->capabilities & command_capabilities;
  if (conflicts == 0U)
    return true;
  if (conflicts & PRIMARY_ACTIVITY_CAP_MOVEMENT)
  {
    if (activity->movement_response == PRIMARY_ACTIVITY_RESPONSE_REJECT)
    {
      activity_stats.rejected_commands++;
      send_to_char(actor, "You cannot move while %s.\r\n", activity->display_name);
      return false;
    }
    return true;
  }
  switch (activity->command_response)
  {
  case PRIMARY_ACTIVITY_RESPONSE_IGNORE:
    return true;
  case PRIMARY_ACTIVITY_RESPONSE_CANCEL:
    finish_activity(activity, PRIMARY_ACTIVITY_STATE_CANCELLED, PRIMARY_ACTIVITY_END_COMMAND, true);
    return true;
  case PRIMARY_ACTIVITY_RESPONSE_PAUSE:
    (void)pause_activity_internal(activity, true, false);
    return true;
  case PRIMARY_ACTIVITY_RESPONSE_DELAY:
    return delay_activity(activity, activity->delay_pulses, true);
  case PRIMARY_ACTIVITY_RESPONSE_RECHECK:
    return activity_recheck_now(activity, PRIMARY_ACTIVITY_END_RECHECK_FAILED);
  case PRIMARY_ACTIVITY_RESPONSE_REJECT:
  default:
    activity_stats.rejected_commands++;
    send_to_char(actor, "You cannot '%s' while %s. Use 'activity' for status.\r\n",
                 command != NULL ? command : "do that", activity->display_name);
    return false;
  }
}

void primary_activity_on_semantic_turn(struct char_data *actor)
{
  struct primary_activity *activity;

  if (actor == NULL || (activity = actor->primary_activity) == NULL ||
      activity->state != PRIMARY_ACTIVITY_STATE_ACTIVE || !activity->combat_clock)
    return;
  if (activity->delayed_combat_turns > 0U)
  {
    activity->delayed_combat_turns--;
    return;
  }
  if (!activity_recheck_now(activity, PRIMARY_ACTIVITY_END_RECHECK_FAILED))
    return;
  activity = actor->primary_activity;
  if (activity == NULL || !consume_combat_actions(actor, activity->combat_actions_required))
    return;
  (void)advance_activity(activity, false);
}

void primary_activity_forget_character(struct char_data *actor)
{
  struct domain_entity_handle handle;
  struct primary_activity *activity;
  uint64_t *ids;
  size_t count = 0U;
  size_t index;

  if (actor == NULL)
    return;
  handle = domain_event_character_handle(actor);
  if (actor->primary_activity != NULL)
    finish_activity(actor->primary_activity, PRIMARY_ACTIVITY_STATE_CANCELLED,
                    PRIMARY_ACTIVITY_END_EXTRACTED, false);
  if (!domain_entity_handle_is_valid(handle) || activity_stats.active == 0U)
    return;
  ids = calloc(activity_stats.active, sizeof(*ids));
  if (ids == NULL)
    return;
  for (activity = activity_head; activity != NULL; activity = activity->next)
    if (domain_entity_handle_equal(activity->target, handle))
      ids[count++] = activity->id;
  for (index = 0U; index < count; index++)
  {
    activity = find_activity_by_id(ids[index]);
    if (activity != NULL && domain_entity_handle_equal(activity->target, handle))
      (void)apply_response(activity, activity->target_loss_response,
                           PRIMARY_ACTIVITY_END_TARGET_LOST, false);
  }
  free(ids);
}

bool primary_activity_snapshot(const struct char_data *actor,
                               struct primary_activity_snapshot *snapshot)
{
  const struct primary_activity *activity;

  if (snapshot == NULL)
    return false;
  memset(snapshot, 0, sizeof(*snapshot));
  if (actor == NULL || (activity = actor->primary_activity) == NULL)
    return false;
  snapshot->id = activity->id;
  snapshot->type = activity->type;
  snapshot->state = activity->state;
  strlcpy(snapshot->display_name, activity->display_name, sizeof(snapshot->display_name));
  snapshot->capabilities = activity->capabilities;
  snapshot->traits = activity->traits;
  snapshot->completed_steps = activity->completed_steps;
  snapshot->total_steps = activity->total_steps;
  snapshot->combat_clock = activity->combat_clock;
  if (!event_runtime_handle_is_none(activity->timer_handle))
    snapshot->next_step_pulses = activity_timer_remaining(activity->timer_handle);
  else
    snapshot->next_step_pulses = activity->remaining_delay;
  return true;
}

void primary_activity_get_stats(struct primary_activity_stats *stats)
{
  if (stats != NULL)
    *stats = activity_stats;
}

bool primary_activity_feature_enabled(enum primary_activity_type type)
{
  return type != PRIMARY_ACTIVITY_CAMP || managed_camp;
}

const char *primary_activity_state_name(enum primary_activity_state state)
{
  switch (state)
  {
  case PRIMARY_ACTIVITY_STATE_ACTIVE:
    return "active";
  case PRIMARY_ACTIVITY_STATE_PAUSED:
    return "paused";
  case PRIMARY_ACTIVITY_STATE_COMPLETED:
    return "completed";
  case PRIMARY_ACTIVITY_STATE_CANCELLED:
    return "cancelled";
  case PRIMARY_ACTIVITY_STATE_NONE:
  default:
    return "none";
  }
}

const char *primary_activity_end_reason_name(enum primary_activity_end_reason reason)
{
  switch (reason)
  {
  case PRIMARY_ACTIVITY_END_PLAYER_CANCELLED:
    return "cancelled";
  case PRIMARY_ACTIVITY_END_MOVED:
    return "you moved";
  case PRIMARY_ACTIVITY_END_DIED:
    return "you died";
  case PRIMARY_ACTIVITY_END_EXTRACTED:
    return "you left the world";
  case PRIMARY_ACTIVITY_END_TARGET_LOST:
    return "the target was lost";
  case PRIMARY_ACTIVITY_END_RECHECK_FAILED:
    return "conditions changed";
  case PRIMARY_ACTIVITY_END_COUNTERED:
    return "the spell was countered";
  case PRIMARY_ACTIVITY_END_DAMAGED:
    return "damage interrupted it";
  case PRIMARY_ACTIVITY_END_COMMAND:
    return "another action interrupted it";
  case PRIMARY_ACTIVITY_END_SHUTDOWN:
    return "the world is shutting down";
  case PRIMARY_ACTIVITY_END_INTERNAL:
    return "it could not continue";
  case PRIMARY_ACTIVITY_END_COMPLETED:
  default:
    return "completed";
  }
}

static void append_name(char *buffer, size_t capacity, const char *name, bool *first)
{
  if (!*first)
    strlcat(buffer, ", ", capacity);
  strlcat(buffer, name, capacity);
  *first = false;
}

static void format_capabilities(uint32_t values, char *buffer, size_t capacity)
{
  bool first = true;

  buffer[0] = '\0';
  if (values & PRIMARY_ACTIVITY_CAP_MOVEMENT)
    append_name(buffer, capacity, "movement", &first);
  if (values & PRIMARY_ACTIVITY_CAP_HANDS)
    append_name(buffer, capacity, "hands", &first);
  if (values & PRIMARY_ACTIVITY_CAP_ATTENTION)
    append_name(buffer, capacity, "attention", &first);
  if (values & PRIMARY_ACTIVITY_CAP_VISION)
    append_name(buffer, capacity, "vision", &first);
  if (values & PRIMARY_ACTIVITY_CAP_SPEECH)
    append_name(buffer, capacity, "speech", &first);
  if (values & PRIMARY_ACTIVITY_CAP_STANDARD)
    append_name(buffer, capacity, "standard", &first);
  if (values & PRIMARY_ACTIVITY_CAP_MOVE)
    append_name(buffer, capacity, "move", &first);
  if (values & PRIMARY_ACTIVITY_CAP_SWIFT)
    append_name(buffer, capacity, "swift", &first);
  if (values & PRIMARY_ACTIVITY_CAP_IMMEDIATE)
    append_name(buffer, capacity, "immediate", &first);
}

static void format_traits(uint32_t values, char *buffer, size_t capacity)
{
  bool first = true;

  buffer[0] = '\0';
  if (values & PRIMARY_ACTIVITY_TRAIT_STATIONARY)
    append_name(buffer, capacity, "stationary", &first);
  if (values & PRIMARY_ACTIVITY_TRAIT_DISTRACTED)
    append_name(buffer, capacity, "distracted", &first);
  if (values & PRIMARY_ACTIVITY_TRAIT_HANDS_OCCUPIED)
    append_name(buffer, capacity, "hands occupied", &first);
  if (values & PRIMARY_ACTIVITY_TRAIT_FINE_MANIPULATION)
    append_name(buffer, capacity, "fine manipulation", &first);
  if (values & PRIMARY_ACTIVITY_TRAIT_OBVIOUS)
    append_name(buffer, capacity, "obvious", &first);
}

static void send_wrapped_field(struct char_data *ch, const char *label, const char *values)
{
  char copy[160];
  char line[81];
  char continuation[24];
  char *save = NULL;
  char *value;
  size_t label_length;
  bool first = true;

  if (ch == NULL || label == NULL || values == NULL)
    return;
  label_length = MIN(strlen(label), sizeof(continuation) - 1U);
  memset(continuation, ' ', label_length);
  continuation[label_length] = '\0';
  strlcpy(copy, values, sizeof(copy));
  strlcpy(line, label, sizeof(line));
  for (value = strtok_r(copy, ",", &save); value != NULL; value = strtok_r(NULL, ",", &save))
  {
    size_t separator_length;

    skip_spaces(&value);
    separator_length = first ? 0U : 2U;
    if (strlen(line) + separator_length + strlen(value) > 78U)
    {
      send_to_char(ch, "%s\r\n", line);
      strlcpy(line, continuation, sizeof(line));
      first = true;
      separator_length = 0U;
    }
    if (separator_length > 0U)
      strlcat(line, ", ", sizeof(line));
    strlcat(line, value, sizeof(line));
    first = false;
  }
  send_to_char(ch, "%s\r\n", line);
}

ACMD(do_activity)
{
  char option[MAX_INPUT_LENGTH];
  struct primary_activity_snapshot snapshot;
  char capabilities[160];
  char traits[160];
  unsigned int percent;

  (void)cmd;
  (void)subcmd;
  one_argument(argument, option, sizeof(option));
  if (!strcasecmp(option, "cancel"))
  {
    if (!primary_activity_cancel(ch, PRIMARY_ACTIVITY_END_PLAYER_CANCELLED, true))
      send_to_char(ch, "You are not performing a primary activity.\r\n");
    return;
  }
  if (!strcasecmp(option, "pause"))
  {
    if (!primary_activity_pause(ch, true))
      send_to_char(ch, "You have no active activity to pause.\r\n");
    return;
  }
  if (!strcasecmp(option, "resume"))
  {
    if (!primary_activity_resume(ch, true))
      send_to_char(ch, "You have no paused activity that can resume.\r\n");
    return;
  }
  if (!primary_activity_snapshot(ch, &snapshot))
  {
    send_to_char(ch, "You are not performing a primary activity.\r\n");
    return;
  }
  percent = snapshot.total_steps > 0U
                ? (unsigned int)((uint64_t)snapshot.completed_steps * 100U / snapshot.total_steps)
                : 0U;
  format_capabilities(snapshot.capabilities, capabilities, sizeof(capabilities));
  format_traits(snapshot.traits, traits, sizeof(traits));
  send_to_char(ch, "Activity: %s\r\n", snapshot.display_name);
  send_to_char(ch, "State: %-7s  Progress: %u/%u (%u%%)\r\n",
               primary_activity_state_name(snapshot.state), snapshot.completed_steps,
               snapshot.total_steps, percent);
  send_wrapped_field(ch, "Claims: ", capabilities);
  send_wrapped_field(ch, "Traits: ", traits);
  if (snapshot.state == PRIMARY_ACTIVITY_STATE_ACTIVE)
    send_to_char(ch, "Clock: %s  Next step: %.1f seconds\r\n",
                 snapshot.combat_clock ? "combat turn" : "wall time",
                 (double)snapshot.next_step_pulses / PASSES_PER_SEC);
}

#ifdef LUMINARI_CUTEST
void primary_activity_test_select_camp(bool managed)
{
  test_camp_selection = true;
  test_managed_camp = managed;
  managed_camp = managed;
}

bool primary_activity_test_camp_value_is_managed(const char *value)
{
  return camp_mode_is_managed(value);
}
#endif
