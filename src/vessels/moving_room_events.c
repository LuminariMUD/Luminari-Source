#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "domain_event_runtime.h"
#include "domain_event_world.h"
#include "event_runtime.h"
#include "spec/spec_dispatch.h"
#include "moving_room_events.h"

struct moving_room_job
{
  struct domain_entity_handle room;
  struct moving_room_data *definition; /* Identity only until checked against the live room. */
  struct event_runtime_handle timer;
  struct moving_room_job *previous, *next;
  bool listed;
};

static struct moving_room_job *jobs;
static game_event_type_id_t relocation_type;
static bool enabled;

static void unlink_job(struct moving_room_job *job)
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

static struct room_data *resolve_room(struct domain_entity_handle room)
{
  return domain_event_resolve(domain_event_runtime_bus(), room, DOMAIN_ENTITY_ROOM);
}

static void cleanup_job(void *payload)
{
  struct moving_room_job *job = payload;
  struct room_data *room = resolve_room(job->room);

  if (room != NULL && room->moving_room_event.id == job->timer.id)
    room->moving_room_event = EVENT_RUNTIME_HANDLE_NONE;
  unlink_job(job);
  free(job);
}

static game_tick_t period(const struct moving_room_data *mover)
{
  /* The former scanner ran every ten seconds, including nonpositive reset values. */
  return (game_tick_t)MAX(1, mover->resetZonePulse) * 10U * PASSES_PER_SEC;
}

static struct game_event_result relocate(const struct game_event_context *context)
{
  struct moving_room_job *job = context->payload;
  struct room_data *room = resolve_room(job->room);

  if (!enabled || room == NULL || room->mover != job->definition ||
      room->moving_room_event.id != context->event_id || room->mover->destination != room->number)
    return game_event_result_complete();
  spec_gateway_moving_room(room, room->mover, room->number);
  room = resolve_room(job->room);
  if (!enabled || room == NULL || room->mover != job->definition ||
      room->moving_room_event.id != context->event_id || room->mover->destination != room->number)
    return game_event_result_complete();
  return game_event_result_reschedule_after(period(room->mover));
}

void moving_room_event_forget(room_rnum rnum)
{
  struct event_runtime_handle timer;

  if (world == NULL || rnum == NOWHERE || rnum > top_of_world)
    return;
  timer = world[rnum].moving_room_event;
  world[rnum].moving_room_event = EVENT_RUNTIME_HANDLE_NONE;
  (void)event_runtime_cancel(timer);
}

int moving_room_remaining_seconds(room_rnum rnum)
{
  game_tick_t ticks;

  if (world == NULL || rnum == NOWHERE || rnum > top_of_world ||
      event_runtime_remaining(world[rnum].moving_room_event, &ticks) != GAME_SCHEDULER_OK)
    return -1;
  return (int)MIN((game_tick_t)INT_MAX, ticks / PASSES_PER_SEC + (ticks % PASSES_PER_SEC != 0));
}

bool moving_room_event_sync(room_rnum rnum)
{
  struct room_data *room;
  struct moving_room_job *job;
  struct game_event_owner owner = game_event_owner_none();
  game_tick_t remaining;

  if (world == NULL || rnum == NOWHERE || rnum > top_of_world)
    return false;
  room = &world[rnum];
  if (room->mover == NULL)
  {
    moving_room_event_forget(rnum);
    return true;
  }
  if (!enabled)
    return false;
  /* An editor clone cannot schedule the source room's mover under another vnum. */
  if (room->mover->destination != room->number)
  {
    moving_room_event_forget(rnum);
    log("SYSERR: moving room #%d has metadata for room #%d; refusing its deadline.", room->number,
        room->mover->destination);
    return false;
  }
  if (event_runtime_remaining(room->moving_room_event, &remaining) == GAME_SCHEDULER_OK)
    return true;
  job = calloc(1, sizeof(*job));
  if (job == NULL)
    return false;
  job->room = domain_event_room_handle(rnum);
  job->definition = room->mover;
  owner.kind = GAME_EVENT_OWNER_ROOM;
  owner.runtime_id = job->room.runtime_id;
  owner.generation = job->room.generation;
  if (event_runtime_schedule_owned_after(relocation_type, owner, period(room->mover), job,
                                         &job->timer) != GAME_SCHEDULER_OK)
  {
    free(job);
    return false;
  }
  room->moving_room_event = job->timer;
  job->next = jobs;
  if (jobs != NULL)
    jobs->previous = job;
  jobs = job;
  job->listed = true;
  return true;
}

enum domain_event_status moving_room_events_init(void)
{
  struct game_event_type_config type = {0};

  if (event_runtime_find_type("moving-room.relocate", &relocation_type) != GAME_SCHEDULER_OK)
  {
    type.name = "moving-room.relocate";
    type.handler = relocate;
    type.cleanup = cleanup_job;
    type.lateness_policy = GAME_EVENT_LATENESS_RUN_ONCE;
    type.requires_owner = true;
    type.max_events = 65536;
    /* A callback may replace its mover while the cancelled callback is still unwinding. */
    type.max_events_per_owner = 2;
    if (event_runtime_register_type(&type, &relocation_type) != GAME_SCHEDULER_OK)
      return DOMAIN_EVENT_ALLOCATION_FAILED;
  }
  enabled = true;
  return DOMAIN_EVENT_OK;
}

bool moving_room_events_bootstrap(void)
{
  room_rnum room;

  if (!enabled)
    return false;
  if (world == NULL)
    return true;
  for (room = 0; room <= top_of_world; room++)
    if (world[room].mover != NULL && !moving_room_event_sync(room))
    {
      log("SYSERR: unable to admit native moving-room deadline for #%d.", world[room].number);
      moving_room_events_shutdown();
      return false;
    }
  return true;
}

void moving_room_events_shutdown(void)
{
  enabled = false;
  while (jobs != NULL)
  {
    struct moving_room_job *job = jobs;
    struct event_runtime_handle timer = job->timer;

    unlink_job(job);
    (void)event_runtime_cancel(timer);
  }
}
