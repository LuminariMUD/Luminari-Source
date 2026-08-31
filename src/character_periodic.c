#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "dotenv.h"
#include "act.h"
#include "bardic_performance.h"
#include "character_periodic.h"
#include "domain_event_types.h"
#include "domain_event_world.h"
#include "mudlim.h"
#include "vessels/transport.h"
#include "dgscript/dg_event.h"

#define CHARACTER_PERIODIC_MAX_OWNERS 32768U
#define CHARACTER_PERIODIC_REJECTION_LOG_INTERVAL 100U
#define CHARACTER_WALK_CADENCE ((long)(PASSES_PER_SEC * 3 / 4))
#define CHARACTER_PSP_CADENCE ((long)(PASSES_PER_SEC * 5))

static bool initialized;
static bool scheduled;
static bool shutting_down;
static bool refilling;
static struct char_data *owner_list;
static struct char_data *dispatching_owner;
static bool dispatching_owner_forgotten;
static size_t owner_count;
static size_t scheduled_count;
static size_t admission_limit = CHARACTER_PERIODIC_MAX_OWNERS;
static uint64_t admission_rejections;
static uint64_t callback_count;
static uint64_t walk_executions;
static uint64_t psp_executions;
static uint64_t bardic_executions;
static uint64_t hint_executions;
static uint64_t luminari_executions;
static uint64_t damage_effect_executions;
static uint64_t player_misc_executions;
static uint64_t next_generation = 1U;
#ifdef LUMINARI_CUTEST
static bool test_selection_set;
static bool test_scheduled_selection;
#endif

static void refill_capacity(void);

static bool configured_scheduled(void)
{
  const char *value;

  value = getenv("LUMINARI_CHARACTER_EVENTS");
  if (value == NULL || *value == '\0')
    value = get_env_value("LUMINARI_CHARACTER_EVENTS");
  if (value == NULL || *value == '\0' || !strcasecmp(value, "scheduled") ||
      !strcasecmp(value, "active") || !strcasecmp(value, "event"))
    return true;
  if (!strcasecmp(value, "legacy") || !strcasecmp(value, "heartbeat") ||
      !strcasecmp(value, "off"))
    return false;
  log("WARNING: Unknown LUMINARI_CHARACTER_EVENTS '%s'; using scheduled owner events.", value);
  return true;
}

static uint64_t ensure_generation(struct char_data *ch)
{
  if (ch == NULL || ch->periodic_event_generation != 0U)
    return ch != NULL ? ch->periodic_event_generation : 0U;
  if (next_generation == 0U)
    return 0U;
  ch->periodic_event_generation = next_generation;
  if (next_generation == UINT64_MAX)
    next_generation = 0U;
  else
    next_generation++;
  return ch->periodic_event_generation;
}

static struct game_event_owner character_owner(struct char_data *ch)
{
  struct game_event_owner owner = game_event_owner_none();

  if (ch == NULL)
    return owner;
  owner.kind = GAME_EVENT_OWNER_CHARACTER;
  owner.runtime_id = (uint64_t)(uintptr_t)ch;
  owner.generation = ensure_generation(ch);
  return owner;
}

static bool has_walk_state(const struct char_data *ch)
{
  return ch != NULL && ch->desc != NULL && !IS_NPC(ch) && ch->player_specials != NULL &&
         GET_WALKTO_LOC(ch) != 0;
}

static bool is_in_world(const struct char_data *ch)
{
  return ch != NULL && world != NULL && IN_ROOM(ch) != NOWHERE && IN_ROOM(ch) <= top_of_world;
}

static bool is_owner_eligible(const struct char_data *ch)
{
  return ch != NULL &&
         (is_in_world(ch) || ch->desc != NULL || has_walk_state(ch) || IS_PERFORMING(ch));
}

static long boundary_delay(long cadence)
{
  unsigned long remainder;

  if (cadence <= 0)
    return 1L;
  remainder = pulse % (unsigned long)cadence;
  return remainder == 0U ? cadence : cadence - (long)remainder;
}

static long next_owner_delay(const struct char_data *ch)
{
  long delay = LONG_MAX;
  long candidate;

  if (ch == NULL)
    return 0L;
  if (ch->desc != NULL)
  {
    candidate = boundary_delay(CHARACTER_PSP_CADENCE);
    if (candidate < delay)
      delay = candidate;
    if (!IS_NPC(ch))
    {
      candidate = boundary_delay(PULSE_HINTS);
      if (candidate < delay)
        delay = candidate;
    }
  }
  if (is_in_world(ch))
  {
    candidate = boundary_delay(PULSE_LUMINARI);
    if (candidate < delay)
      delay = candidate;
    candidate = boundary_delay(PULSE_VIOLENCE);
    if (candidate < delay)
      delay = candidate;
  }
  if (has_walk_state(ch))
  {
    candidate = boundary_delay(CHARACTER_WALK_CADENCE);
    if (candidate < delay)
      delay = candidate;
  }
  if (IS_PERFORMING(ch))
  {
    candidate = boundary_delay(PULSE_VERSE_INTERVAL);
    if (candidate < delay)
      delay = candidate;
  }
  return delay == LONG_MAX ? 0L : delay;
}

static void borrowed_owner_cleanup(struct event *event)
{
  if (event != NULL)
    event->event_obj = NULL;
}

static void registry_add(struct char_data *ch)
{
  if (ch == NULL || ch->character_periodic_registered)
    return;
  ch->character_periodic_prev = NULL;
  ch->character_periodic_next = owner_list;
  if (owner_list != NULL)
    owner_list->character_periodic_prev = ch;
  owner_list = ch;
  ch->character_periodic_registered = true;
  owner_count++;
}

static void registry_remove(struct char_data *ch)
{
  if (ch == NULL || !ch->character_periodic_registered)
    return;
  if (ch->character_periodic_prev != NULL)
    ch->character_periodic_prev->character_periodic_next = ch->character_periodic_next;
  else if (owner_list == ch)
    owner_list = ch->character_periodic_next;
  if (ch->character_periodic_next != NULL)
    ch->character_periodic_next->character_periodic_prev = ch->character_periodic_prev;
  ch->character_periodic_next = NULL;
  ch->character_periodic_prev = NULL;
  ch->character_periodic_registered = false;
  if (owner_count > 0U)
    owner_count--;
}

static void note_rejection(void)
{
  admission_rejections++;
  if (admission_rejections == 1U ||
      admission_rejections % CHARACTER_PERIODIC_REJECTION_LOG_INTERVAL == 0U)
    log("WARNING: character periodic owner limit reached (%zu); rejected=%llu.",
        admission_limit, (unsigned long long)admission_rejections);
}

static bool callback_owner_still_live(void)
{
  return !dispatching_owner_forgotten;
}

static EVENTFUNC(character_periodic_event)
{
  struct char_data *ch = event_obj;
  long delay;

  if (ch == NULL)
    return 0;
  callback_count++;
  if (!ch->character_periodic_registered || !is_owner_eligible(ch))
  {
    ch->character_periodic_event = NULL;
    if (scheduled_count > 0U)
      scheduled_count--;
    registry_remove(ch);
    refill_capacity();
    return 0;
  }

  dispatching_owner = ch;
  dispatching_owner_forgotten = false;
  if (has_walk_state(ch) && pulse % (unsigned long)CHARACTER_WALK_CADENCE == 0U)
  {
    walk_executions++;
    process_walkto_action(ch);
  }
  if (callback_owner_still_live() && ch->desc != NULL &&
      pulse % (unsigned long)CHARACTER_PSP_CADENCE == 0U)
  {
    psp_executions++;
    regen_psp_one(ch);
  }
  if (callback_owner_still_live() && is_in_world(ch) &&
      pulse % (unsigned long)PULSE_LUMINARI == 0U)
  {
    luminari_executions++;
    pulse_luminari_character_one(ch);
  }
  if (callback_owner_still_live() && IS_PERFORMING(ch) &&
      pulse % (unsigned long)PULSE_VERSE_INTERVAL == 0U)
  {
    bardic_executions++;
    pulse_bardic_performance_one(ch);
  }
  if (callback_owner_still_live() && ch->desc != NULL && !IS_NPC(ch) &&
      pulse % (unsigned long)PULSE_HINTS == 0U)
  {
    hint_executions++;
    show_hint_one(ch);
  }
  if (callback_owner_still_live() && is_in_world(ch) &&
      pulse % (unsigned long)PULSE_VIOLENCE == 0U)
  {
    damage_effect_executions++;
    update_damage_and_effects_over_time_one(ch);
  }
  if (callback_owner_still_live() && ch->desc != NULL && is_in_world(ch) &&
      pulse % (unsigned long)PULSE_VIOLENCE == 0U)
  {
    player_misc_executions++;
    update_player_misc_one(ch);
  }

  if (!callback_owner_still_live())
  {
    dispatching_owner = NULL;
    return 0;
  }
  dispatching_owner = NULL;
  if (!is_owner_eligible(ch))
  {
    ch->character_periodic_event = NULL;
    if (scheduled_count > 0U)
      scheduled_count--;
    registry_remove(ch);
    refill_capacity();
    return 0;
  }
  delay = next_owner_delay(ch);
  return delay > 0L ? delay : 0L;
}

static bool schedule_owner(struct char_data *ch)
{
  struct game_event_owner owner;
  long delay;

  if (!initialized || !scheduled || shutting_down || ch == NULL ||
      event_backend_current() == EVENT_BACKEND_UNINITIALIZED ||
      !ch->character_periodic_registered || ch->character_periodic_event != NULL)
    return false;
  if (scheduled_count >= admission_limit)
  {
    note_rejection();
    return false;
  }
  delay = next_owner_delay(ch);
  if (delay <= 0L)
    return false;
  owner = character_owner(ch);
  if (!game_event_owner_is_valid(owner))
    return false;
  ch->character_periodic_event = event_create_owned_named_with_cleanup(
      character_periodic_event, ch, delay, "character_periodic", borrowed_owner_cleanup, owner);
  if (ch->character_periodic_event == NULL)
  {
    note_rejection();
    return false;
  }
  scheduled_count++;
  return true;
}

static void refill_capacity(void)
{
  struct char_data *ch;

  if (!initialized || !scheduled || shutting_down || refilling ||
      scheduled_count >= admission_limit)
    return;
  refilling = true;
  for (ch = owner_list; ch != NULL && scheduled_count < admission_limit;
       ch = ch->character_periodic_next)
  {
    if (ch->character_periodic_event == NULL)
      schedule_owner(ch);
  }
  refilling = false;
}

void character_periodic_sync(struct char_data *ch)
{
  struct event *event;
  long delay;

  if (!initialized || !scheduled || event_backend_current() == EVENT_BACKEND_UNINITIALIZED ||
      ch == NULL)
    return;
  if (dispatching_owner == ch)
    return;
  if (!is_owner_eligible(ch))
  {
    character_periodic_forget(ch);
    return;
  }
  registry_add(ch);
  if (ch->character_periodic_event == NULL)
  {
    schedule_owner(ch);
    return;
  }
  delay = next_owner_delay(ch);
  if (delay <= 0L || event_time(ch->character_periodic_event) <= delay)
    return;
  event = ch->character_periodic_event;
  ch->character_periodic_event = NULL;
  if (scheduled_count > 0U)
    scheduled_count--;
  event_cancel(event);
  schedule_owner(ch);
}

void character_periodic_forget(struct char_data *ch)
{
  struct event *event;

  if (ch == NULL)
    return;
  if (dispatching_owner == ch)
    dispatching_owner_forgotten = true;
  if (ch->character_periodic_event != NULL)
  {
    event = ch->character_periodic_event;
    ch->character_periodic_event = NULL;
    if (scheduled_count > 0U)
      scheduled_count--;
    event_cancel(event);
  }
  registry_remove(ch);
  refill_capacity();
}

static void handle_character_moved(const struct domain_event_context *context,
                                   void *handler_context)
{
  const struct domain_character_moved *event;
  struct char_data *ch;

  (void)handler_context;
  event = context->payload;
  ch = domain_event_resolve(context->bus, event->character, DOMAIN_ENTITY_CHARACTER);
  character_periodic_sync(ch);
}

enum domain_event_status character_periodic_register_handlers(struct domain_event_bus *bus)
{
  struct domain_event_handler_config handler = {
      DOMAIN_EVENT_CHARACTER_MOVED,
      "character-periodic-moved",
      90,
      handle_character_moved,
      NULL,
  };

  if (bus == NULL)
    return DOMAIN_EVENT_INVALID_ARGUMENT;
  return domain_event_register_handler(bus, &handler);
}

void character_periodic_init(void)
{
  struct char_data *ch;

  if (initialized)
    return;
#ifdef LUMINARI_CUTEST
  scheduled = test_selection_set ? test_scheduled_selection : configured_scheduled();
#else
  scheduled = configured_scheduled();
#endif
  initialized = true;
  shutting_down = false;
  log("Character periodic scheduling: %s (owner limit %zu).",
      scheduled ? "scheduled" : "legacy heartbeat", admission_limit);
  if (!scheduled)
    return;
  for (ch = character_list; ch != NULL; ch = ch->next)
    character_periodic_sync(ch);
}

void character_periodic_shutdown(void)
{
  struct char_data *ch;
  struct char_data *next;

  if (!initialized)
    return;
  shutting_down = true;
  for (ch = owner_list; ch != NULL; ch = next)
  {
    next = ch->character_periodic_next;
    character_periodic_forget(ch);
  }
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

bool character_periodic_events_enabled(void) { return initialized && scheduled; }
size_t character_periodic_owner_count(void) { return owner_count; }
size_t character_periodic_scheduled_count(void) { return scheduled_count; }
size_t character_periodic_admission_limit(void) { return admission_limit; }
uint64_t character_periodic_admission_rejections(void) { return admission_rejections; }
uint64_t character_periodic_callbacks(void) { return callback_count; }
uint64_t character_periodic_walk_executions(void) { return walk_executions; }
uint64_t character_periodic_psp_executions(void) { return psp_executions; }
uint64_t character_periodic_bardic_executions(void) { return bardic_executions; }
uint64_t character_periodic_hint_executions(void) { return hint_executions; }
uint64_t character_periodic_luminari_executions(void) { return luminari_executions; }
uint64_t character_periodic_damage_effect_executions(void) { return damage_effect_executions; }
uint64_t character_periodic_player_misc_executions(void) { return player_misc_executions; }

size_t character_periodic_registry_validate(void)
{
  struct char_data *ch;
  struct char_data *previous = NULL;
  size_t members = 0U;
  size_t events = 0U;

  for (ch = owner_list; ch != NULL; ch = ch->character_periodic_next)
  {
    if (!ch->character_periodic_registered || ch->character_periodic_prev != previous ||
        !is_owner_eligible(ch))
      return owner_count + 1U;
    members++;
    if (ch->character_periodic_event != NULL)
      events++;
    if (members > owner_count)
      return members;
    previous = ch;
  }
  if (members != owner_count || events != scheduled_count)
    return members + events + 1U;
  return 0U;
}

void character_periodic_reset_telemetry(void)
{
  admission_rejections = 0U;
  callback_count = 0U;
  walk_executions = 0U;
  psp_executions = 0U;
  bardic_executions = 0U;
  hint_executions = 0U;
  luminari_executions = 0U;
  damage_effect_executions = 0U;
  player_misc_executions = 0U;
}

#ifdef LUMINARI_CUTEST
void character_periodic_reset_for_test(void)
{
  character_periodic_shutdown();
  character_periodic_reset_telemetry();
  admission_limit = CHARACTER_PERIODIC_MAX_OWNERS;
  test_selection_set = false;
  test_scheduled_selection = false;
}

void character_periodic_select_for_test(bool use_scheduled)
{
  test_selection_set = true;
  test_scheduled_selection = use_scheduled;
}

void character_periodic_set_limit_for_test(size_t limit)
{
  admission_limit = limit;
}
#endif
