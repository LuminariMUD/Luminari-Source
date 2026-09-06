#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "act.h"
#include "domain_event_runtime.h"
#include "domain_event_types.h"
#include "domain_event_world.h"
#include "event_runtime.h"
#include "movement/movement.h"
#include "phenomenon_response.h"

#define PHENOMENON_INTEREST_PULSES (30 * PASSES_PER_SEC)
#define PHENOMENON_RESPONSE_MAX_EVENTS 65536U
#define PHENOMENON_RESPONSE_REJECTION_LOG_INTERVAL 100U

struct phenomenon_response_payload
{
  struct domain_phenomenon_perceived perceived;
  struct event_runtime_handle event;
  bool responded;
  bool set_cover;
};

static game_event_type_id_t response_event_type;
static uint64_t admission_rejections;
#ifdef LUMINARI_CUTEST
static phenomenon_response_test_callback test_callback;
static void *test_context;
#endif

static bool response_candidate(const struct char_data *observer,
                               const struct domain_phenomenon_perceived *perceived)
{
  if (observer == NULL || perceived == NULL || !IS_NPC(observer) ||
      MOB_FLAGGED(observer, MOB_NO_AI) || GET_POS(observer) <= POS_STUNNED)
    return false;
  return MOB_FLAGGED(observer, MOB_LISTEN) || MOB_FLAGGED(observer, MOB_GUARD) ||
         MOB_FLAGGED(observer, MOB_HELPER) || MOB_FLAGGED(observer, MOB_MOB_ASSIST);
}

static void clear_interest(struct char_data *observer, struct phenomenon_response_payload *payload)
{
  if (observer == NULL || payload == NULL ||
      observer->phenomenon_interest_event.id != payload->event.id)
    return;
  observer->phenomenon_interest_event = EVENT_RUNTIME_HANDLE_NONE;
  observer->phenomenon_interest_id = 0U;
  if (payload->set_cover)
    REMOVE_BIT_AR(AFF_FLAGS(observer), AFF_TOTAL_DEFENSE);
#ifdef LUMINARI_CUTEST
  if (test_callback != NULL)
    test_callback(observer, false, test_context);
#endif
}

static void response_cleanup(void *event_obj)
{
  struct phenomenon_response_payload *payload = event_obj;
  struct char_data *observer;

  if (payload == NULL)
    return;
  observer = domain_event_world_resolve_character(payload->perceived.observer);
  clear_interest(observer, payload);
  free(payload);
}

static int adjacent_direction(room_rnum from_room, room_rnum to_room)
{
  int direction;

  if (from_room == NOWHERE || to_room == NOWHERE || from_room > top_of_world ||
      to_room > top_of_world)
    return -1;
  for (direction = 0; direction < NUM_OF_DIRS; direction++)
    if (world[from_room].dir_option[direction] != NULL &&
        world[from_room].dir_option[direction]->to_room == to_room)
      return direction;
  return -1;
}

static struct game_event_result run_response(const struct game_event_context *context)
{
  struct phenomenon_response_payload *payload = context != NULL ? context->payload : NULL;
  struct char_data *observer;
  struct room_data *source_room;
  int direction;

  if (payload == NULL)
    return game_event_result_complete();
  observer = domain_event_world_resolve_character(payload->perceived.observer);
  if (observer == NULL || observer->phenomenon_interest_event.id != context->event_id)
    return game_event_result_complete();
  if (payload->responded)
  {
    clear_interest(observer, payload);
    return game_event_result_complete();
  }
  payload->responded = true;
  if (payload->perceived.kind == DOMAIN_PHENOMENON_MAGIC_IMPACT ||
      payload->perceived.kind == DOMAIN_PHENOMENON_FIRE)
  {
    if (!AFF_FLAGGED(observer, AFF_TOTAL_DEFENSE))
    {
      SET_BIT_AR(AFF_FLAGS(observer), AFF_TOTAL_DEFENSE);
      payload->set_cover = true;
    }
    act("$n takes cover from the danger!", FALSE, observer, NULL, NULL, TO_ROOM);
  }
  act(payload->perceived.source_known ? "$n calls out a warning about the source!"
                                      : "$n calls out a warning about danger nearby!",
      FALSE, observer, NULL, NULL, TO_ROOM);
#ifdef LUMINARI_CUTEST
  if (test_callback != NULL)
    test_callback(observer, true, test_context);
#endif
  source_room = domain_event_resolve(domain_event_runtime_bus(), payload->perceived.source_room,
                                     DOMAIN_ENTITY_ROOM);
  direction = source_room != NULL
                  ? adjacent_direction(IN_ROOM(observer), (room_rnum)(source_room - world))
                  : -1;
  if (direction >= 0 && MOB_FLAGGED(observer, MOB_LISTEN) &&
      !MOB_FLAGGED(observer, MOB_SENTINEL) && CAN_GO(observer, direction))
    (void)perform_move(observer, direction, TRUE);
  observer = domain_event_world_resolve_character(payload->perceived.observer);
  if (observer == NULL || observer->phenomenon_interest_event.id != context->event_id)
    return game_event_result_complete();
  return game_event_result_reschedule_after(PHENOMENON_INTEREST_PULSES);
}

static void note_admission_rejection(void)
{
  admission_rejections++;
  if (admission_rejections == 1U ||
      admission_rejections % PHENOMENON_RESPONSE_REJECTION_LOG_INTERVAL == 0U)
    log("WARNING: NPC phenomenon response admission failed; rejected=%llu.",
        (unsigned long long)admission_rejections);
}

static void handle_perceived(const struct domain_event_context *context, void *handler_context)
{
  const struct domain_phenomenon_perceived *perceived = context->payload;
  struct phenomenon_response_payload *payload;
  struct game_event_owner owner;
  struct event_runtime_handle old_event;
  struct char_data *observer;

  (void)handler_context;
  observer = domain_event_resolve(context->bus, perceived->observer, DOMAIN_ENTITY_CHARACTER);
  if (!response_candidate(observer, perceived) || perceived->phenomenon_id == 0U ||
      observer->phenomenon_interest_id == perceived->phenomenon_id)
    return;
  old_event = observer->phenomenon_interest_event;
  if (!event_runtime_handle_is_none(old_event))
    (void)event_runtime_cancel(old_event);
  observer->phenomenon_interest_event = EVENT_RUNTIME_HANDLE_NONE;
  observer->phenomenon_interest_id = 0U;
  payload = calloc(1, sizeof(*payload));
  if (payload == NULL)
  {
    note_admission_rejection();
    return;
  }
  payload->perceived = *perceived;
  owner = game_event_owner_none();
  owner.kind = GAME_EVENT_OWNER_CHARACTER;
  owner.runtime_id = perceived->observer.runtime_id;
  owner.generation = perceived->observer.generation;
  if (event_runtime_schedule_owned_after(response_event_type, owner, 1U, payload,
                                         &observer->phenomenon_interest_event) !=
      GAME_SCHEDULER_OK)
  {
    free(payload);
    note_admission_rejection();
    return;
  }
  payload->event = observer->phenomenon_interest_event;
  observer->phenomenon_interest_id = perceived->phenomenon_id;
}

enum domain_event_status phenomenon_response_init(struct domain_event_bus *bus)
{
  struct game_event_type_config event_config = {0};
  struct domain_event_handler_config handler_config = {
      DOMAIN_EVENT_PHENOMENON_PERCEIVED, "npc.phenomenon-response", 100, handle_perceived, NULL};
  const char *registered;

  if (bus == NULL)
    return DOMAIN_EVENT_INVALID_ARGUMENT;
  registered = event_runtime_type_name(response_event_type);
  if (registered == NULL || strcmp(registered, "npc.phenomenon-interest") != 0)
  {
    response_event_type = 0U;
    event_config.name = "npc.phenomenon-interest";
    event_config.handler = run_response;
    event_config.cleanup = response_cleanup;
    event_config.lateness_policy = GAME_EVENT_LATENESS_RUN_ONCE;
    event_config.max_events = PHENOMENON_RESPONSE_MAX_EVENTS;
    event_config.max_events_per_owner = 2U;
    event_config.requires_owner = true;
    if (event_runtime_register_type(&event_config, &response_event_type) != GAME_SCHEDULER_OK)
      return DOMAIN_EVENT_ALLOCATION_FAILED;
  }
  return domain_event_register_handler(bus, &handler_config);
}

void phenomenon_response_shutdown(void)
{
  struct char_data *observer;
  struct event_runtime_handle event;

  for (observer = character_list; observer != NULL; observer = observer->next)
  {
    event = observer->phenomenon_interest_event;
    if (!event_runtime_handle_is_none(event))
      (void)event_runtime_cancel(event);
    observer->phenomenon_interest_event = EVENT_RUNTIME_HANDLE_NONE;
    observer->phenomenon_interest_id = 0U;
  }
  response_event_type = 0U;
}

uint64_t phenomenon_response_admission_rejections(void)
{
  return admission_rejections;
}

#ifdef LUMINARI_CUTEST
void phenomenon_response_set_test_callback(phenomenon_response_test_callback callback,
                                           void *context)
{
  test_callback = callback;
  test_context = context;
}
#endif
