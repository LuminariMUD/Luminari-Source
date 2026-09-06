#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "event_runtime.h"
#include "staff_events.h"
#include "staff_event_agenda.h"

#define STAFF_HOUR_TICKS ((game_tick_t)SECS_PER_MUD_HOUR * PASSES_PER_SEC)
#define STAFF_AGENDA_OWNER 0x535441464645564eULL

enum agenda_kind
{
  AGENDA_EXPIRY,
  AGENDA_DELAY,
  AGENDA_POPULATION,
  AGENDA_PRISONER,
  AGENDA_COUNT
};
struct staff_wakeup
{
  uint64_t incarnation;
  int event_num;
  enum agenda_kind kind;
};
static game_event_type_id_t types[AGENDA_COUNT];
static struct event_runtime_handle timers[AGENDA_COUNT];
static uint64_t serial, active_incarnation, delay_incarnation;
static int active_event = UNDEFINED_EVENT;
static bool enabled;

uint64_t staff_event_agenda_incarnation(void)
{
  return active_incarnation;
}

static game_tick_t next_hour(void)
{
  return STAFF_HOUR_TICKS - ((game_tick_t)pulse % STAFF_HOUR_TICKS);
}

static bool schedule(enum agenda_kind kind, int event_num, uint64_t incarnation, game_tick_t ticks,
                     struct event_runtime_handle *timer)
{
  struct staff_wakeup *wakeup = malloc(sizeof(*wakeup));
  struct game_event_owner owner = game_event_owner_none();

  if (wakeup == NULL)
    return false;
  *wakeup = (struct staff_wakeup){incarnation, event_num, kind};
  owner.kind = GAME_EVENT_OWNER_SERVICE;
  owner.runtime_id = STAFF_AGENDA_OWNER;
  owner.generation = incarnation;
  if (event_runtime_schedule_owned_after(types[kind], owner, ticks, wakeup, timer) !=
      GAME_SCHEDULER_OK)
  {
    free(wakeup);
    return false;
  }
  return true;
}

void staff_event_agenda_cancel(void)
{
  int kind;

  active_incarnation = 0;
  active_event = UNDEFINED_EVENT;
  for (kind = 0; kind < AGENDA_COUNT; kind++)
    if (kind != AGENDA_DELAY)
    {
      struct event_runtime_handle timer = timers[kind];

      timers[kind] = EVENT_RUNTIME_HANDLE_NONE;
      (void)event_runtime_cancel(timer);
    }
}

int staff_event_agenda_hours(bool delay)
{
  game_tick_t ticks;

  if (event_runtime_remaining(timers[delay ? AGENDA_DELAY : AGENDA_EXPIRY], &ticks) !=
      GAME_SCHEDULER_OK)
    return 0;
  return (int)MIN((game_tick_t)INT_MAX, ticks / STAFF_HOUR_TICKS + (ticks % STAFF_HOUR_TICKS != 0));
}

int staff_event_agenda_seconds(void)
{
  game_tick_t ticks;

  if (event_runtime_remaining(timers[AGENDA_EXPIRY], &ticks) != GAME_SCHEDULER_OK)
    return 0;
  return (int)MIN((game_tick_t)INT_MAX, ticks / PASSES_PER_SEC + (ticks % PASSES_PER_SEC != 0));
}

bool staff_event_agenda_start(int event_num, int hours)
{
  uint64_t incarnation;
  struct event_runtime_handle expiry = EVENT_RUNTIME_HANDLE_NONE,
                              agenda = EVENT_RUNTIME_HANDLE_NONE;
  enum agenda_kind kind;

  if (!enabled || active_incarnation != 0 || event_num < 0 || event_num >= NUM_STAFF_EVENTS ||
      hours < 1)
    return false;
  incarnation = ++serial;
  kind = event_num == JACKALOPE_HUNT ? AGENDA_POPULATION : AGENDA_PRISONER;
  if (!schedule(AGENDA_EXPIRY, event_num, incarnation,
                next_hour() + (game_tick_t)(hours - 1) * STAFF_HOUR_TICKS, &expiry))
    return false;
  if (!schedule(kind, event_num, incarnation, next_hour(), &agenda))
  {
    (void)event_runtime_cancel(expiry);
    return false;
  }
  timers[AGENDA_EXPIRY] = expiry;
  timers[kind] = agenda;
  active_incarnation = incarnation;
  active_event = event_num;
  return true;
}

bool staff_event_agenda_delay(int hours)
{
  struct event_runtime_handle old = timers[AGENDA_DELAY], replacement = EVENT_RUNTIME_HANDLE_NONE;
  uint64_t incarnation = ++serial;

  if (hours > 0 && (!enabled || !schedule(AGENDA_DELAY, UNDEFINED_EVENT, incarnation,
                                          next_hour() + (game_tick_t)(hours - 1) * STAFF_HOUR_TICKS,
                                          &replacement)))
    return false;
  timers[AGENDA_DELAY] = replacement;
  delay_incarnation = hours > 0 ? incarnation : 0;
  (void)event_runtime_cancel(old);
  return true;
}

static struct game_event_result dispatch(const struct game_event_context *context)
{
  const struct staff_wakeup wakeup = *(struct staff_wakeup *)context->payload;

  if (!enabled)
    return game_event_result_complete();
  if (wakeup.kind == AGENDA_DELAY)
  {
    if (delay_incarnation == wakeup.incarnation)
    {
      timers[AGENDA_DELAY] = EVENT_RUNTIME_HANDLE_NONE;
      delay_incarnation = 0;
      staffevent_data.delay = 0;
    }
    return game_event_result_complete();
  }
  if (active_incarnation != wakeup.incarnation || active_event != wakeup.event_num)
    return game_event_result_complete();
  if (wakeup.kind == AGENDA_EXPIRY)
  {
    timers[AGENDA_EXPIRY] = EVENT_RUNTIME_HANDLE_NONE;
    (void)end_staff_event(wakeup.event_num);
    return game_event_result_complete();
  }
  /* Expiry wins over maintenance even when both become runnable at the same boundary. */
  if (staff_event_agenda_hours(false) == 0)
    return game_event_result_complete();
  if (wakeup.kind == AGENDA_POPULATION)
    staff_event_maintain_population();
  else
    staff_event_maintain_prisoner();
  if (!enabled || active_incarnation != wakeup.incarnation)
    return game_event_result_complete();
  return game_event_result_reschedule_after(next_hour());
}

enum domain_event_status staff_event_agenda_init(void)
{
  static const char *names[AGENDA_COUNT] = {"staff-event.expiry", "staff-event.delay-ended",
                                            "staff-event.jackalope-population",
                                            "staff-event.prisoner-presence"};
  struct game_event_type_config type = {0};
  int kind;

  for (kind = 0; kind < AGENDA_COUNT; kind++)
    if (event_runtime_find_type(names[kind], &types[kind]) != GAME_SCHEDULER_OK)
    {
      type.name = names[kind];
      type.handler = dispatch;
      type.cleanup = free;
      type.lateness_policy = GAME_EVENT_LATENESS_RUN_ONCE;
      type.requires_owner = true;
      type.max_events = 4;
      type.max_events_per_owner = 1;
      if (event_runtime_register_type(&type, &types[kind]) != GAME_SCHEDULER_OK)
        return DOMAIN_EVENT_ALLOCATION_FAILED;
    }
  enabled = true;
  if (!staff_event_agenda_delay(MAX(0, staffevent_data.delay)))
    return DOMAIN_EVENT_ALLOCATION_FAILED;
  return DOMAIN_EVENT_OK;
}

void staff_event_agenda_shutdown(void)
{
  staff_event_agenda_cancel();
  (void)staff_event_agenda_delay(0);
  enabled = false;
  staffevent_data.event_num = UNDEFINED_EVENT;
  staffevent_data.ticks_left = 0;
  staffevent_data.delay = 0;
}
