#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "domain_event_runtime.h"
#include "domain_event_world.h"
#include "event_runtime.h"
#include "dgscript/dg_scripts.h"
#include "transport.h"
#include "transport_jobs.h"
#include "routing.h"

struct transport_job
{
  struct domain_entity_handle passenger;
  struct domain_entity_handle transit;
  struct domain_entity_handle destination;
  struct event_runtime_handle timer;
  struct transport_job *next;
  struct transport_job *previous;
  bool listed;
};

static struct transport_job *jobs;
static game_event_type_id_t arrival_type;
static bool enabled;

bool transport_is_transit_room(room_rnum room)
{
  return world != NULL && room != NOWHERE && room <= top_of_world && world[room].number >= 66700 &&
         world[room].number <= 66799;
}

static void clear_trip(struct char_data *ch)
{
  ch->player_specials->destination = NOWHERE;
  ch->player_specials->travel_timer = 0;
  ch->player_specials->travel_type = 0;
  ch->player_specials->travel_locale = 0;
}

static void unlink_job(struct transport_job *job)
{
  if (!job->listed)
    return;
  if (job->previous != NULL)
    job->previous->next = job->next;
  else
    jobs = job->next;
  if (job->next != NULL)
    job->next->previous = job->previous;
  job->listed = false;
}

static void cleanup_job(void *payload)
{
  struct transport_job *job = payload;
  struct char_data *ch;

  if (job == NULL)
    return;
  ch = domain_event_world_resolve_character(job->passenger);
  if (ch != NULL && ch->player_specials != NULL && ch->player_specials->transport_job == job)
    ch->player_specials->transport_job = NULL;
  unlink_job(job);
  free(job);
}

int transport_remaining_seconds(const struct char_data *ch)
{
  game_tick_t ticks;
  struct transport_job *job;

  if (ch == NULL || IS_NPC(ch) || ch->player_specials == NULL)
    return 0;
  job = ch->player_specials->transport_job;
  if (job != NULL && event_runtime_remaining(job->timer, &ticks) == GAME_SCHEDULER_OK)
    return (int)MIN((game_tick_t)INT_MAX, ticks / PASSES_PER_SEC + (ticks % PASSES_PER_SEC != 0));
  return MAX(0, ch->player_specials->travel_timer);
}

void transport_job_cancel(struct char_data *ch, bool preserve_remaining)
{
  struct transport_job *job;

  if (ch == NULL || IS_NPC(ch) || ch->player_specials == NULL)
    return;
  job = ch->player_specials->transport_job;
  if (preserve_remaining)
    ch->player_specials->travel_timer = transport_remaining_seconds(ch);
  else
    clear_trip(ch);
  ch->player_specials->transport_job = NULL;
  if (job != NULL)
  {
    unlink_job(job);
    (void)event_runtime_cancel(job->timer);
  }
}

/* Arrival is committed before notification; every callback can invalidate a passenger. */
void transport_arrival(struct char_data *ch, room_rnum destination, int type, int locale)
{
  struct domain_entity_handle passenger, room;

  if (ch == NULL || destination == NOWHERE || destination > top_of_world)
    return;
  passenger = domain_event_character_handle(ch);
  room = domain_event_room_handle(destination);
  transport_job_cancel(ch, false);
  char_from_room(ch);
  X_LOC(ch) = world[destination].coords[0];
  Y_LOC(ch) = world[destination].coords[1];
  char_to_room_cause(ch, destination, ch, DOMAIN_RELOCATION_TRANSPORT, -1);
  ch = domain_event_world_resolve_character(passenger);
  if (ch == NULL || !domain_entity_handle_equal(domain_event_room_handle(IN_ROOM(ch)), room))
    return;
  char_pets_to_char_loc(ch);
  ch = domain_event_world_resolve_character(passenger);
  if (ch == NULL || !domain_entity_handle_equal(domain_event_room_handle(IN_ROOM(ch)), room))
    return;
  look_at_room(ch, 0);
  entry_memory_mtrigger(ch);
  ch = domain_event_world_resolve_character(passenger);
  if (ch == NULL || !domain_entity_handle_equal(domain_event_room_handle(IN_ROOM(ch)), room))
    return;
  greet_mtrigger(ch, -1);
  ch = domain_event_world_resolve_character(passenger);
  if (ch == NULL || !domain_entity_handle_equal(domain_event_room_handle(IN_ROOM(ch)), room))
    return;
  greet_memory_mtrigger(ch);
  ch = domain_event_world_resolve_character(passenger);
  if (ch == NULL || !domain_entity_handle_equal(domain_event_room_handle(IN_ROOM(ch)), room))
    return;
  if (!transport_locale_valid(type, locale))
    return;
  if (type == TRAVEL_CARRIAGE)
  {
    act("$n disembarks a horse-drawn carriage that grinds to a halt before you.", FALSE, ch, NULL,
        NULL, TO_ROOM);
    send_to_char(ch, "You hop out of your carriage arriving at the %s.\r\n\r\n",
                 get_transport_carriage_name(locale));
  }
  else if (type == TRAVEL_SAILING)
  {
    act("$n disembarks a caravel that just docked here.", FALSE, ch, NULL, NULL, TO_ROOM);
    send_to_char(ch, "You disembark the caravel arriving at %s.\r\n\r\n",
                 get_transport_sailing_name(locale));
  }
}

static struct game_event_result arrive(const struct game_event_context *context)
{
  struct transport_job *job = context->payload;
  struct char_data *ch = domain_event_world_resolve_character(job->passenger);
  struct room_data *destination;
  int type, locale;

  if (ch == NULL || ch->player_specials == NULL || ch->player_specials->transport_job != job)
    return game_event_result_complete();
  if (!domain_entity_handle_equal(domain_event_room_handle(IN_ROOM(ch)), job->transit))
  {
    clear_trip(ch);
    return game_event_result_complete();
  }
  ch->player_specials->travel_timer = 0;
  if (ch->desc == NULL || STATE(ch->desc) != CON_PLAYING)
    return game_event_result_complete();
  destination =
      domain_event_resolve(domain_event_runtime_bus(), job->destination, DOMAIN_ENTITY_ROOM);
  if (destination == NULL)
  {
    send_to_char(ch, "Your destination is no longer available. Please contact a staff member.\r\n");
    return game_event_result_complete();
  }
  type = ch->player_specials->travel_type;
  locale = ch->player_specials->travel_locale;
  /* The running callback owns job until terminal cleanup. Do not cancel it from arrival. */
  ch->player_specials->transport_job = NULL;
  transport_arrival(ch, (room_rnum)(destination - world), type, locale);
  return game_event_result_complete();
}

bool transport_job_start(struct char_data *ch, room_rnum transit, room_rnum destination,
                         int seconds, int type, int locale)
{
  struct transport_job *job;
  struct game_event_owner owner = game_event_owner_none();
  enum game_scheduler_status status;

  if (!enabled || ch == NULL || IS_NPC(ch) || ch->player_specials == NULL ||
      ch->player_specials->transport_job != NULL || !transport_is_transit_room(transit) ||
      destination == NOWHERE || destination > top_of_world || seconds < 0 ||
      !transport_locale_valid(type, locale) || !event_runtime_is_initialized())
    return false;
  job = calloc(1, sizeof(*job));
  if (job == NULL)
    return false;
  job->passenger = domain_event_character_handle(ch);
  job->transit = domain_event_room_handle(transit);
  job->destination = domain_event_room_handle(destination);
  owner.kind = GAME_EVENT_OWNER_CHARACTER;
  owner.runtime_id = job->passenger.runtime_id;
  owner.generation = job->passenger.generation;
  status = event_runtime_schedule_owned_after(
      arrival_type, owner, MAX((game_tick_t)1, (game_tick_t)seconds * PASSES_PER_SEC), job,
      &job->timer);
  if (status != GAME_SCHEDULER_OK)
  {
    free(job);
    return false;
  }
  job->next = jobs;
  if (jobs != NULL)
    jobs->previous = job;
  jobs = job;
  job->listed = true;
  ch->player_specials->transport_job = job;
  ch->player_specials->destination = GET_ROOM_VNUM(destination);
  ch->player_specials->travel_timer = seconds;
  ch->player_specials->travel_type = type;
  ch->player_specials->travel_locale = locale;
  return true;
}

void transport_job_resume(struct char_data *ch)
{
  room_rnum destination;

  if (ch == NULL || IS_NPC(ch) || ch->player_specials == NULL || ch->desc == NULL ||
      STATE(ch->desc) != CON_PLAYING || ch->player_specials->transport_job != NULL)
    return;
  if (!transport_is_transit_room(IN_ROOM(ch)))
  {
    clear_trip(ch);
    return;
  }
  destination = real_room(ch->player_specials->destination);
  if (destination == NOWHERE ||
      !transport_locale_valid(ch->player_specials->travel_type, ch->player_specials->travel_locale))
  {
    /* Compatibility with saves that predate persisted journey state. */
    destination = real_room(14100);
    if (destination != NOWHERE)
      transport_arrival(ch, destination, 0, 0);
    else
      send_to_char(ch, "Your journey needs staff assistance; its destination is unavailable.\r\n");
    return;
  }
  if (!transport_job_start(ch, IN_ROOM(ch), destination, MAX(0, ch->player_specials->travel_timer),
                           ch->player_specials->travel_type, ch->player_specials->travel_locale))
    send_to_char(ch, "Your journey could not be resumed. Please contact a staff member.\r\n");
}

static void passenger_moved(const struct domain_event_context *context, void *data)
{
  const struct domain_character_moved *event = context->payload;
  struct char_data *ch = domain_event_world_resolve_character(event->character);
  struct transport_job *job;

  (void)data;
  if (ch == NULL || IS_NPC(ch) || ch->player_specials == NULL)
    return;
  job = ch->player_specials->transport_job;
  if (job != NULL && !domain_entity_handle_equal(event->to_room, job->transit))
    transport_job_cancel(ch, false);
}

enum domain_event_status transport_jobs_init(struct domain_event_bus *bus)
{
  struct game_event_type_config type = {0};
  const struct domain_event_handler_config moved = {
      DOMAIN_EVENT_CHARACTER_MOVED, "transport.passenger-relocated", 20, passenger_moved, NULL};

  if (event_runtime_find_type("transport.arrival", &arrival_type) != GAME_SCHEDULER_OK)
  {
    type.name = "transport.arrival";
    type.handler = arrive;
    type.cleanup = cleanup_job;
    type.lateness_policy = GAME_EVENT_LATENESS_RUN_ONCE;
    type.requires_owner = true;
    type.max_events = 32768;
    type.max_events_per_owner = 1;
    if (event_runtime_register_type(&type, &arrival_type) != GAME_SCHEDULER_OK)
      return DOMAIN_EVENT_ALLOCATION_FAILED;
  }
  if (domain_event_register_handler(bus, &moved) != DOMAIN_EVENT_OK)
    return DOMAIN_EVENT_ALLOCATION_FAILED;
  enabled = true;
  return DOMAIN_EVENT_OK;
}

void transport_jobs_shutdown(void)
{
  struct char_data *ch;

  enabled = false;
  while (jobs != NULL)
  {
    ch = domain_event_world_resolve_character(jobs->passenger);
    if (ch != NULL && ch->player_specials != NULL && ch->player_specials->transport_job == jobs)
      transport_job_cancel(ch, true);
    else
    {
      struct event_runtime_handle timer = jobs->timer;

      unlink_job(jobs);
      (void)event_runtime_cancel(timer);
    }
  }
}
