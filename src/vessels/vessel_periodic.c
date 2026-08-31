#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "dotenv.h"
#include "dgscript/dg_event.h"
#include "vessel_periodic.h"
#include "vessels.h"
#include "vessels_rol.h"

#define VESSEL_PERIODIC_MAX_OWNERS GREYHAWK_MAXSHIPS
#define VESSEL_PERIODIC_REJECTION_LOG_INTERVAL 100U
#define VESSEL_PERIODIC_FAST_CADENCE ((long)AUTOPILOT_TICK_INTERVAL)
#define VESSEL_PERIODIC_SCHEDULE_CADENCE ((long)(SECS_PER_MUD_HOUR * PASSES_PER_SEC))
#define VESSEL_PERIODIC_SERVICE_ID 0x5653534cU

extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];

static bool initialized;
static bool scheduled;
static bool shutting_down;
static bool refilling;
static struct greyhawk_ship_data *owner_list;
static struct greyhawk_ship_data *dispatching_owner;
static bool dispatching_owner_forgotten;
static struct event *service_event;
static size_t owner_count;
static size_t scheduled_count;
static size_t admission_limit = VESSEL_PERIODIC_MAX_OWNERS;
static uint64_t next_generation = 1U;
static uint64_t admission_rejections;
static uint64_t callback_count;
static uint64_t service_callback_count;
static uint64_t fast_executions;
static uint64_t schedule_executions;
static unsigned int narrative_ticks;
static unsigned int hazard_ticks;
static unsigned int encounter_ticks;
static unsigned long prepared_pulse = ULONG_MAX;
static int current_wage_batch;
static bool narrative_due;
static bool hazard_due;
static bool encounter_due;
#ifdef LUMINARI_CUTEST
static bool test_selection_set;
static bool test_scheduled_selection;
#endif

static void refill_capacity(void);
static void cancel_owner_registry(void);

static bool configured_scheduled(void)
{
  const char *value;

#ifdef LUMINARI_CUTEST
  if (test_selection_set)
    return test_scheduled_selection;
#endif
  value = getenv("LUMINARI_VESSEL_EVENTS");
  if (value == NULL || *value == '\0')
    value = get_env_value("LUMINARI_VESSEL_EVENTS");
  if (value == NULL || *value == '\0' || !strcasecmp(value, "scheduled") ||
      !strcasecmp(value, "active") || !strcasecmp(value, "event"))
    return true;
  if (!strcasecmp(value, "legacy") || !strcasecmp(value, "heartbeat") ||
      !strcasecmp(value, "off"))
    return false;
  log("WARNING: Unknown LUMINARI_VESSEL_EVENTS '%s'; using scheduled owner events.", value);
  return true;
}

static long boundary_delay(long cadence)
{
  unsigned long remainder;

  if (cadence <= 0L)
    return 1L;
  remainder = pulse % (unsigned long)cadence;
  return remainder == 0U ? cadence : cadence - (long)remainder;
}

static uint64_t ensure_generation(struct greyhawk_ship_data *ship)
{
  if (ship == NULL || ship->periodic_generation != 0U)
    return ship != NULL ? ship->periodic_generation : 0U;
  if (next_generation == 0U)
    return 0U;
  ship->periodic_generation = next_generation;
  if (next_generation == UINT64_MAX)
    next_generation = 0U;
  else
    next_generation++;
  return ship->periodic_generation;
}

static struct game_event_owner vessel_owner(struct greyhawk_ship_data *ship)
{
  struct game_event_owner owner = game_event_owner_none();

  if (ship == NULL || ship->shipnum < 0)
    return owner;
  owner.kind = GAME_EVENT_OWNER_VESSEL;
  owner.runtime_id = (uint64_t)ship->shipnum + 1U;
  owner.generation = ensure_generation(ship);
  return owner;
}

static struct game_event_owner service_owner(void)
{
  struct game_event_owner owner = game_event_owner_none();

  owner.kind = GAME_EVENT_OWNER_SERVICE;
  owner.runtime_id = VESSEL_PERIODIC_SERVICE_ID;
  owner.generation = 1U;
  return owner;
}

static bool owner_is_live(const struct greyhawk_ship_data *ship)
{
  return ship != NULL && CONFIG_VESSEL_SYSTEM && is_valid_ship(ship);
}

static void registry_add(struct greyhawk_ship_data *ship)
{
  if (ship == NULL || ship->periodic_registered)
    return;
  ship->periodic_prev = NULL;
  ship->periodic_next = owner_list;
  if (owner_list != NULL)
    owner_list->periodic_prev = ship;
  owner_list = ship;
  ship->periodic_registered = true;
  owner_count++;
}

static void registry_remove(struct greyhawk_ship_data *ship)
{
  if (ship == NULL || !ship->periodic_registered)
    return;
  if (ship->periodic_prev != NULL)
    ship->periodic_prev->periodic_next = ship->periodic_next;
  else if (owner_list == ship)
    owner_list = ship->periodic_next;
  if (ship->periodic_next != NULL)
    ship->periodic_next->periodic_prev = ship->periodic_prev;
  ship->periodic_prev = NULL;
  ship->periodic_next = NULL;
  ship->periodic_registered = false;
  if (owner_count > 0U)
    owner_count--;
}

static void note_rejection(void)
{
  admission_rejections++;
  if (admission_rejections == 1U ||
      admission_rejections % VESSEL_PERIODIC_REJECTION_LOG_INTERVAL == 0U)
    log("WARNING: vessel periodic owner limit reached (%zu); rejected=%llu.", admission_limit,
        (unsigned long long)admission_rejections);
}

static void vessel_owner_cleanup(struct event *event)
{
  struct greyhawk_ship_data *ship;

  if (event == NULL)
    return;
  ship = event->event_obj;
  if (ship != NULL && ship->periodic_event == event)
  {
    ship->periodic_event = NULL;
    if (scheduled_count > 0U)
      scheduled_count--;
  }
  event->event_obj = NULL;
}

static void vessel_service_cleanup(struct event *event)
{
  if (event == NULL)
    return;
  if (service_event == event)
    service_event = NULL;
  event->event_obj = NULL;
}

static void prepare_tick(void)
{
  if (prepared_pulse == pulse)
    return;
  prepared_pulse = pulse;
  current_wage_batch = vessel_crew_wage_begin_tick();
  narrative_due = ++narrative_ticks >= VESSEL_NARRATIVE_INTERVAL;
  hazard_due = ++hazard_ticks >= VESSEL_HAZARD_INTERVAL;
  encounter_due = ++encounter_ticks >= VESSEL_ENCOUNTER_INTERVAL;
  if (narrative_due)
    narrative_ticks = 0U;
  if (hazard_due)
    hazard_ticks = 0U;
  if (encounter_due)
  {
    encounter_ticks = 0U;
    vessel_encounter_tick_begin();
  }
}

static bool callback_owner_still_live(struct greyhawk_ship_data *ship)
{
  return !dispatching_owner_forgotten && owner_is_live(ship);
}

static EVENTFUNC(vessel_owner_event)
{
  struct greyhawk_ship_data *ship = event_obj;
  int departed_position;

  if (ship == NULL)
    return 0;
  callback_count++;
  if (!ship->periodic_registered || !owner_is_live(ship))
  {
    ship->periodic_event = NULL;
    if (scheduled_count > 0U)
      scheduled_count--;
    registry_remove(ship);
    refill_capacity();
    return 0;
  }

  prepare_tick();
  dispatching_owner = ship;
  dispatching_owner_forgotten = false;
  if (pulse % (unsigned long)VESSEL_PERIODIC_FAST_CADENCE == 0U)
  {
    fast_executions++;
    autopilot_tick_one(ship);
    if (callback_owner_still_live(ship))
      vessel_hunter_tick_one(ship);
    if (callback_owner_still_live(ship))
      vessel_combat_tick_one(ship);
    if (callback_owner_still_live(ship))
    {
      departed_position = vessel_crew_wage_tick_one(ship, current_wage_batch);
      if (departed_position >= 0)
        vessel_crew_delete_departure(ship->shipnum, departed_position);
    }
    if (callback_owner_still_live(ship))
      vessel_upkeep_tick_one(ship);
    if (callback_owner_still_live(ship) && narrative_due)
      vessel_narrative_tick_one(ship);
    if (callback_owner_still_live(ship) && hazard_due)
      vessel_weather_tick_one(ship);
    if (callback_owner_still_live(ship) && encounter_due)
      vessel_encounter_tick_one(ship);
  }
  if (callback_owner_still_live(ship) &&
      pulse % (unsigned long)VESSEL_PERIODIC_SCHEDULE_CADENCE == 0U)
  {
    schedule_executions++;
    schedule_tick_one(ship);
  }

  if (!callback_owner_still_live(ship))
  {
    dispatching_owner = NULL;
    return 0;
  }
  dispatching_owner = NULL;
  return VESSEL_PERIODIC_FAST_CADENCE;
}

static EVENTFUNC(vessel_service_event)
{
  (void)event_obj;

  if (!initialized || !scheduled)
  {
    service_event = NULL;
    return 0;
  }
  service_callback_count++;
  prepare_tick();
  if (CONFIG_VESSEL_SYSTEM)
  {
    vessel_event_tick();
    vessel_trade_restock_tick();
    vessel_msdp_tick();
    if (pulse % (unsigned long)VESSEL_PERIODIC_SCHEDULE_CADENCE == 0U)
      vessel_merchant_tick();
  }
  return VESSEL_PERIODIC_FAST_CADENCE;
}

static bool schedule_service_event(void)
{
  struct game_event_owner owner;

  if (service_event != NULL)
    return true;
  if (!initialized || !scheduled || shutting_down ||
      event_backend_current() == EVENT_BACKEND_UNINITIALIZED)
    return false;
  owner = service_owner();
  service_event = event_create_owned_named_with_cleanup(
      vessel_service_event, NULL, boundary_delay(VESSEL_PERIODIC_FAST_CADENCE), "vessel_service",
      vessel_service_cleanup, owner);
  return service_event != NULL;
}

static void cancel_service_event(void)
{
  struct event *event;

  if (service_event == NULL)
    return;
  event = service_event;
  service_event = NULL;
  event_cancel(event);
}

static bool schedule_owner(struct greyhawk_ship_data *ship)
{
  struct game_event_owner owner;

  if (!initialized || !scheduled || shutting_down || ship == NULL ||
      event_backend_current() == EVENT_BACKEND_UNINITIALIZED || !ship->periodic_registered ||
      ship->periodic_event != NULL || !owner_is_live(ship))
    return false;
  if (scheduled_count >= admission_limit)
  {
    note_rejection();
    return false;
  }
  owner = vessel_owner(ship);
  if (!game_event_owner_is_valid(owner))
    return false;
  ship->periodic_event = event_create_owned_named_with_cleanup(
      vessel_owner_event, ship, boundary_delay(VESSEL_PERIODIC_FAST_CADENCE), "vessel_periodic",
      vessel_owner_cleanup, owner);
  if (ship->periodic_event == NULL)
  {
    note_rejection();
    return false;
  }
  scheduled_count++;
  return true;
}

static void refill_capacity(void)
{
  struct greyhawk_ship_data *ship;

  if (!initialized || !scheduled || shutting_down || refilling ||
      scheduled_count >= admission_limit)
    return;
  refilling = true;
  for (ship = owner_list; ship != NULL && scheduled_count < admission_limit;
       ship = ship->periodic_next)
  {
    if (ship->periodic_event == NULL)
      schedule_owner(ship);
  }
  refilling = false;
}

void vessel_periodic_sync(struct greyhawk_ship_data *ship)
{
  if (!initialized || !scheduled || event_backend_current() == EVENT_BACKEND_UNINITIALIZED ||
      ship == NULL)
    return;
  if (!owner_is_live(ship))
  {
    vessel_periodic_forget(ship);
    return;
  }
  registry_add(ship);
  if (ship->periodic_event == NULL)
    schedule_owner(ship);
}

void vessel_periodic_forget(struct greyhawk_ship_data *ship)
{
  struct event *event;

  if (ship == NULL)
    return;
  if (dispatching_owner == ship)
    dispatching_owner_forgotten = true;
  if (ship->periodic_event != NULL)
  {
    event = ship->periodic_event;
    ship->periodic_event = NULL;
    if (scheduled_count > 0U)
      scheduled_count--;
    event_cancel(event);
  }
  registry_remove(ship);
  ship->periodic_generation = 0U;
  refill_capacity();
}

void vessel_periodic_rebuild(void)
{
  int index;

  if (!initialized || !scheduled)
    return;
  for (index = 0; index < GREYHAWK_MAXSHIPS; index++)
  {
    if (is_valid_ship(&greyhawk_ships[index]))
      vessel_periodic_sync(&greyhawk_ships[index]);
  }
}

static void cancel_owner_registry(void)
{
  struct greyhawk_ship_data *ship;
  struct greyhawk_ship_data *next;

  for (ship = owner_list; ship != NULL; ship = next)
  {
    next = ship->periodic_next;
    vessel_periodic_forget(ship);
  }
}

void vessel_periodic_feature_changed(void)
{
  if (!initialized || !scheduled)
    return;
  if (CONFIG_VESSEL_SYSTEM)
  {
    if (!schedule_service_event())
    {
      log("WARNING: unable to restore the vessel periodic service event; using the legacy "
          "heartbeat.");
      scheduled = false;
      cancel_owner_registry();
      rol_ship_periodic_shutdown();
      rol_ship_periodic_init();
      return;
    }
    vessel_periodic_rebuild();
    return;
  }
  cancel_service_event();
  cancel_owner_registry();
}

void vessel_periodic_init(void)
{
  if (initialized)
    return;
  scheduled = configured_scheduled();
  initialized = true;
  shutting_down = false;
  prepared_pulse = ULONG_MAX;
  if (scheduled && event_backend_current() == EVENT_BACKEND_UNINITIALIZED)
  {
    log("WARNING: vessel periodic scheduling requested before event initialization; using the "
        "legacy heartbeat.");
    scheduled = false;
  }
  if (scheduled && CONFIG_VESSEL_SYSTEM)
  {
    if (!schedule_service_event())
    {
      log("WARNING: unable to schedule the vessel periodic service event; using the legacy "
          "heartbeat.");
      scheduled = false;
    }
  }
  rol_ship_periodic_init();
  log("Vessel periodic scheduling: %s (owner limit %zu).",
      scheduled ? "scheduled" : "legacy heartbeat", admission_limit);
}

void vessel_periodic_shutdown(void)
{
  if (!initialized)
    return;
  shutting_down = true;
  rol_ship_periodic_shutdown();
  cancel_service_event();
  cancel_owner_registry();
  owner_list = NULL;
  owner_count = 0U;
  scheduled_count = 0U;
  dispatching_owner = NULL;
  dispatching_owner_forgotten = false;
  initialized = false;
  scheduled = false;
  shutting_down = false;
  refilling = false;
}

bool vessel_periodic_events_enabled(void) { return initialized && scheduled; }
size_t vessel_periodic_owner_count(void) { return owner_count; }
size_t vessel_periodic_scheduled_count(void) { return scheduled_count; }
uint64_t vessel_periodic_callbacks(void) { return callback_count; }
uint64_t vessel_periodic_service_callbacks(void) { return service_callback_count; }
uint64_t vessel_periodic_fast_executions(void) { return fast_executions; }
uint64_t vessel_periodic_schedule_executions(void) { return schedule_executions; }
uint64_t vessel_periodic_admission_rejections(void) { return admission_rejections; }
size_t vessel_periodic_admission_limit(void) { return admission_limit; }

size_t vessel_periodic_registry_validate(void)
{
  struct greyhawk_ship_data *ship;
  size_t members = 0U;
  size_t events = 0U;
  size_t mismatches = 0U;

  for (ship = owner_list; ship != NULL; ship = ship->periodic_next)
  {
    members++;
    if (!ship->periodic_registered || !owner_is_live(ship))
      mismatches++;
    if (ship->periodic_event != NULL)
      events++;
    if (ship->periodic_next != NULL && ship->periodic_next->periodic_prev != ship)
      mismatches++;
  }
  if (members != owner_count)
    mismatches++;
  if (events != scheduled_count)
    mismatches++;
  return mismatches;
}

void vessel_periodic_reset_telemetry(void)
{
  admission_rejections = 0U;
  callback_count = 0U;
  service_callback_count = 0U;
  fast_executions = 0U;
  schedule_executions = 0U;
}

#ifdef LUMINARI_CUTEST
void vessel_periodic_select_for_test(bool use_scheduled)
{
  test_selection_set = true;
  test_scheduled_selection = use_scheduled;
}

void vessel_periodic_set_admission_limit_for_test(size_t limit) { admission_limit = limit; }

void vessel_periodic_reset_for_test(void)
{
  vessel_periodic_shutdown();
  admission_limit = VESSEL_PERIODIC_MAX_OWNERS;
  next_generation = 1U;
  narrative_ticks = 0U;
  hazard_ticks = 0U;
  encounter_ticks = 0U;
  prepared_pulse = ULONG_MAX;
  current_wage_batch = 0;
  narrative_due = false;
  hazard_due = false;
  encounter_due = false;
  test_selection_set = false;
  test_scheduled_selection = false;
  vessel_periodic_reset_telemetry();
}
#endif
