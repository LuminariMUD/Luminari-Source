#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "dotenv.h"
#include "mudlim.h"
#include "dgscript/dg_scripts.h"
#include "dgscript/dg_event.h"
#include "point_update_periodic.h"

#define POINT_UPDATE_CADENCE ((long)(SECS_PER_MUD_HOUR * PASSES_PER_SEC))
#define POINT_UPDATE_SERVICE_ID 0x50555044U

static bool initialized;
static bool scheduled;
static bool shutting_down;
static bool dispatch_due;
static bool character_iteration_active;
static bool object_iteration_active;
static struct event *service_event;
static struct char_data *character_owners;
static struct char_data *character_iteration_next;
static struct obj_data *object_owners;
static struct obj_data *object_iteration_next;
static size_t character_count;
static size_t object_count;
static uint64_t service_callbacks;
static uint64_t dispatches;
static uint64_t character_executions;
static uint64_t object_executions;
#ifdef LUMINARI_CUTEST
static bool test_selection_set;
static bool test_scheduled_selection;
#endif

static bool configured_scheduled(void)
{
  const char *value;

#ifdef LUMINARI_CUTEST
  if (test_selection_set)
    return test_scheduled_selection;
#endif
  value = getenv("LUMINARI_POINT_UPDATE_EVENTS");
  if (value == NULL || *value == '\0')
    value = get_env_value("LUMINARI_POINT_UPDATE_EVENTS");
  if (value == NULL || *value == '\0' || !strcasecmp(value, "scheduled") ||
      !strcasecmp(value, "active") || !strcasecmp(value, "event"))
    return true;
  if (!strcasecmp(value, "legacy") || !strcasecmp(value, "heartbeat") ||
      !strcasecmp(value, "off"))
    return false;
  log("WARNING: Unknown LUMINARI_POINT_UPDATE_EVENTS '%s'; using scheduled events.", value);
  return true;
}

static long boundary_delay(void)
{
  unsigned long remainder = pulse % (unsigned long)POINT_UPDATE_CADENCE;

  return remainder == 0U ? POINT_UPDATE_CADENCE : POINT_UPDATE_CADENCE - (long)remainder;
}

static struct game_event_owner service_owner(void)
{
  struct game_event_owner owner = game_event_owner_none();

  owner.kind = GAME_EVENT_OWNER_SERVICE;
  owner.runtime_id = POINT_UPDATE_SERVICE_ID;
  owner.generation = 1U;
  return owner;
}

static bool character_eligible(const struct char_data *ch)
{
  return ch != NULL && !IS_NPC(ch);
}

static bool object_eligible(const struct obj_data *obj)
{
  int slot;

  if (obj == NULL || !obj->object_list_member)
    return false;
  for (slot = 0; slot < SPEC_TIMER_MAX; slot++)
    if (GET_OBJ_SPECTIMER(obj, slot) > 0)
      return true;
  return GET_OBJ_TIMER(obj) > 0 || SCRIPT_CHECK(obj, OTRIG_TIMER) ||
         (GET_OBJ_TYPE(obj) == ITEM_MISSILE && GET_OBJ_VAL(obj, 1) != 0) ||
         OBJ_FLAGGED(obj, ITEM_DECAY) || IS_CORPSE(obj);
}

static void character_add(struct char_data *ch)
{
  if (ch == NULL || ch->point_update_registered)
    return;
  ch->point_update_prev = NULL;
  ch->point_update_next = character_owners;
  if (character_owners != NULL)
    character_owners->point_update_prev = ch;
  character_owners = ch;
  ch->point_update_registered = true;
  character_count++;
}

static void character_remove(struct char_data *ch)
{
  if (ch == NULL || !ch->point_update_registered)
    return;
  if (character_iteration_active && character_iteration_next == ch)
    character_iteration_next = ch->point_update_next;
  if (ch->point_update_prev != NULL)
    ch->point_update_prev->point_update_next = ch->point_update_next;
  else if (character_owners == ch)
    character_owners = ch->point_update_next;
  if (ch->point_update_next != NULL)
    ch->point_update_next->point_update_prev = ch->point_update_prev;
  ch->point_update_next = NULL;
  ch->point_update_prev = NULL;
  ch->point_update_registered = false;
  if (character_count > 0U)
    character_count--;
}

static void object_add(struct obj_data *obj)
{
  if (obj == NULL || obj->point_update_registered)
    return;
  obj->point_update_prev = NULL;
  obj->point_update_next = object_owners;
  if (object_owners != NULL)
    object_owners->point_update_prev = obj;
  object_owners = obj;
  obj->point_update_registered = true;
  object_count++;
}

static void object_remove(struct obj_data *obj)
{
  if (obj == NULL || !obj->point_update_registered)
    return;
  if (object_iteration_active && object_iteration_next == obj)
    object_iteration_next = obj->point_update_next;
  if (obj->point_update_prev != NULL)
    obj->point_update_prev->point_update_next = obj->point_update_next;
  else if (object_owners == obj)
    object_owners = obj->point_update_next;
  if (obj->point_update_next != NULL)
    obj->point_update_next->point_update_prev = obj->point_update_prev;
  obj->point_update_next = NULL;
  obj->point_update_prev = NULL;
  obj->point_update_registered = false;
  if (object_count > 0U)
    object_count--;
}

void point_update_character_sync(struct char_data *ch)
{
  if (!initialized || !scheduled || ch == NULL)
    return;
  if (character_eligible(ch))
    character_add(ch);
  else
    character_remove(ch);
}

void point_update_character_forget(struct char_data *ch) { character_remove(ch); }

void point_update_object_sync(struct obj_data *obj)
{
  if (!initialized || !scheduled || obj == NULL)
    return;
  if (object_eligible(obj))
    object_add(obj);
  else
    object_remove(obj);
}

void point_update_object_forget(struct obj_data *obj) { object_remove(obj); }

void point_update_object_spec_timer_set(struct obj_data *obj, int slot, int duration)
{
  if (obj == NULL || slot < 0 || slot >= SPEC_TIMER_MAX)
    return;
  GET_OBJ_SPECTIMER(obj, slot) = duration;
  point_update_object_sync(obj);
}

static void service_cleanup(struct event *event)
{
  if (event != NULL && service_event == event)
    service_event = NULL;
  if (event != NULL)
    event->event_obj = NULL;
}

static EVENTFUNC(point_update_service_event)
{
  (void)event_obj;

  if (!initialized || !scheduled)
  {
    service_event = NULL;
    return 0;
  }
  service_callbacks++;
  dispatch_due = true;
  return POINT_UPDATE_CADENCE;
}

static bool schedule_service(void)
{
  if (service_event != NULL)
    return true;
  if (!initialized || !scheduled || shutting_down ||
      event_backend_current() == EVENT_BACKEND_UNINITIALIZED)
    return false;
  service_event = event_create_owned_named_with_cleanup(
      point_update_service_event, NULL, boundary_delay(), "point_update_service", service_cleanup,
      service_owner());
  return service_event != NULL;
}

bool point_update_periodic_dispatch_due(void)
{
  struct char_data *ch;
  struct obj_data *obj;

  if (!initialized || !scheduled || !dispatch_due)
    return false;
  dispatch_due = false;
  dispatches++;
  point_update_global_one();

  character_iteration_active = true;
  character_iteration_next = character_owners;
  while ((ch = character_iteration_next) != NULL)
  {
    character_iteration_next = ch->point_update_next;
    character_executions++;
    /* Idle handling may extract ch; the extraction hook already repairs this iteration. */
    point_update_character_one(ch);
  }
  character_iteration_next = NULL;
  character_iteration_active = false;

  object_iteration_active = true;
  object_iteration_next = object_owners;
  while ((obj = object_iteration_next) != NULL)
  {
    object_iteration_next = obj->point_update_next;
    object_executions++;
    if (point_update_object_one(obj))
      point_update_object_sync(obj);
  }
  object_iteration_next = NULL;
  object_iteration_active = false;
  return true;
}

void point_update_periodic_init(void)
{
  struct char_data *ch;
  struct obj_data *obj;

  if (initialized)
    return;
  scheduled = configured_scheduled();
  initialized = true;
  shutting_down = false;
  dispatch_due = false;
  if (scheduled && !schedule_service())
  {
    log("WARNING: unable to schedule point-update service; using the legacy heartbeat.");
    scheduled = false;
  }
  if (scheduled)
  {
    for (ch = character_list; ch != NULL; ch = ch->next)
      point_update_character_sync(ch);
    for (obj = object_list; obj != NULL; obj = obj->next)
      point_update_object_sync(obj);
  }
  log("Point-update scheduling: %s.",
      scheduled ? "scheduled (one service event)" : "legacy heartbeat");
}

void point_update_periodic_shutdown(void)
{
  struct char_data *ch;
  struct char_data *next_ch;
  struct obj_data *obj;
  struct obj_data *next_obj;
  struct event *event;

  if (!initialized)
    return;
  shutting_down = true;
  if (service_event != NULL)
  {
    event = service_event;
    service_event = NULL;
    event_cancel(event);
  }
  for (ch = character_owners; ch != NULL; ch = next_ch)
  {
    next_ch = ch->point_update_next;
    character_remove(ch);
  }
  for (obj = object_owners; obj != NULL; obj = next_obj)
  {
    next_obj = obj->point_update_next;
    object_remove(obj);
  }
  character_owners = NULL;
  object_owners = NULL;
  character_iteration_next = NULL;
  object_iteration_next = NULL;
  character_iteration_active = false;
  object_iteration_active = false;
  character_count = 0U;
  object_count = 0U;
  initialized = false;
  scheduled = false;
  shutting_down = false;
  dispatch_due = false;
}

bool point_update_events_enabled(void) { return initialized && scheduled; }
size_t point_update_character_count(void) { return character_count; }
size_t point_update_object_count(void) { return object_count; }
uint64_t point_update_service_callbacks(void) { return service_callbacks; }
uint64_t point_update_dispatches(void) { return dispatches; }
uint64_t point_update_character_executions(void) { return character_executions; }
uint64_t point_update_object_executions(void) { return object_executions; }

size_t point_update_character_registry_validate(void)
{
  struct char_data *ch;
  struct char_data *previous = NULL;
  size_t members = 0U;

  for (ch = character_owners; ch != NULL; ch = ch->point_update_next)
  {
    if (!ch->point_update_registered || ch->point_update_prev != previous ||
        !character_eligible(ch))
      return character_count + 1U;
    members++;
    previous = ch;
    if (members > character_count)
      return members;
  }
  return members == character_count ? 0U :
                                      (members > character_count ? members - character_count
                                                                 : character_count - members);
}

size_t point_update_object_registry_validate(void)
{
  struct obj_data *obj;
  struct obj_data *previous = NULL;
  size_t members = 0U;

  for (obj = object_owners; obj != NULL; obj = obj->point_update_next)
  {
    if (!obj->point_update_registered || obj->point_update_prev != previous ||
        !object_eligible(obj))
      return object_count + 1U;
    members++;
    previous = obj;
    if (members > object_count)
      return members;
  }
  return members == object_count ? 0U :
                                   (members > object_count ? members - object_count
                                                           : object_count - members);
}

void point_update_periodic_reset_telemetry(void)
{
  service_callbacks = 0U;
  dispatches = 0U;
  character_executions = 0U;
  object_executions = 0U;
}

#ifdef LUMINARI_CUTEST
void point_update_periodic_reset_for_test(void)
{
  point_update_periodic_shutdown();
  point_update_periodic_reset_telemetry();
  test_selection_set = false;
  test_scheduled_selection = false;
}

void point_update_periodic_select_for_test(bool use_scheduled)
{
  test_selection_set = true;
  test_scheduled_selection = use_scheduled;
}
#endif
