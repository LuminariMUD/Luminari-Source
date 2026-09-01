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
#include "quest/quest.h"
#include "vessels/transport.h"
#include "event_runtime.h"

#define CHARACTER_PERIODIC_MAX_OWNERS 32768U
#define CHARACTER_PERIODIC_REJECTION_LOG_INTERVAL 100U
#define CHARACTER_WALK_CADENCE ((long)(PASSES_PER_SEC * 3 / 4))
#define CHARACTER_PSP_CADENCE ((long)(PASSES_PER_SEC * 5))
#define CHARACTER_DEVICE_CADENCE ((long)(PASSES_PER_SEC * 30))
#define CHARACTER_QUEST_CADENCE ((long)(SECS_PER_MUD_HOUR * PASSES_PER_SEC))

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
static uint64_t d20_round_executions;
static uint64_t device_executions;
static uint64_t timed_quest_executions;
static uint64_t next_generation = 1U;
static game_event_type_id_t character_maintenance_event_type;
#ifdef LUMINARI_CUTEST
static bool test_selection_set;
static bool test_scheduled_selection;
#endif

static void refill_capacity(void);
static struct game_event_result character_periodic_event(
    const struct game_event_context *context);

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

static bool npc_has_periodic_work(const struct char_data *ch)
{
  int index;

  if (ch == NULL || !IS_NPC(ch) || !is_in_world(ch))
    return false;
  if (ch->affected != NULL || GET_HIT(ch) != GET_MAX_HIT(ch) ||
      GET_MOVE(ch) != GET_MAX_MOVE(ch) || GET_PSP(ch) != GET_MAX_PSP(ch) ||
      RIDING(ch) != NULL || RIDDEN_BY(ch) != NULL ||
      (GET_POS(ch) == POS_FIGHTING && FIGHTING(ch) == NULL))
    return true;
  if (GET_NODAZE_COOLDOWN(ch) > 0 || ch->char_specials.terror_cooldown > 0 ||
      ch->char_specials.has_been_pushed > 0 || ch->char_specials.sickening_aura_timer > 0 ||
      ch->char_specials.frightful_presence_timer > 0 || ch->char_specials.swindle_cooldown > 0 ||
      ch->char_specials.entertain_cooldown > 0 || ch->char_specials.tribute_cooldown > 0 ||
      ch->char_specials.recently_slammed > 0 || ch->char_specials.recently_kicked > 0 ||
      ch->char_specials.banishing_blade_procced_this_round ||
      ch->sticky_bomb[0] != 0)
    return true;
  for (index = 0; index < NUM_ELDRITCH_BLAST_COOLDOWNS; index++)
    if (ch->char_specials.eldritch_blast_cooldowns[index] > 0)
      return true;
  if (MOB_FLAGGED(ch, MOB_HUNTS_TARGET))
    return true;
  return MOB_FLAGGED(ch, MOB_ENCOUNTER) &&
         (ch->mob_specials.extract_timer > 0 || ch->mob_specials.peaceful_timer > 0 ||
          ch->mob_specials.aggro_timer > 0);
}

static bool is_owner_eligible(const struct char_data *ch)
{
  if (ch == NULL || DEAD(ch))
    return false;
  if (IS_NPC(ch))
    return npc_has_periodic_work(ch) || IS_PERFORMING(ch);
  return is_in_world(ch) || ch->desc != NULL || has_walk_state(ch) || IS_PERFORMING(ch);
}

static unsigned long owner_cadence_phase(struct char_data *ch, long cadence)
{
  uint64_t spread;

  if (ch == NULL || cadence <= 0 || !IS_NPC(ch))
    return 0U;
  spread = ensure_generation(ch);
  spread ^= spread >> 30U;
  spread *= UINT64_C(0xbf58476d1ce4e5b9);
  spread ^= spread >> 27U;
  spread *= UINT64_C(0x94d049bb133111eb);
  spread ^= spread >> 31U;
  return (unsigned long)(spread % (uint64_t)cadence);
}

static bool cadence_due(struct char_data *ch, long cadence, unsigned long earliest_due)
{
  unsigned long deadline;
  unsigned long interval;
  unsigned long phase;
  unsigned long remainder;

  if (cadence <= 0)
    return false;
  interval = (unsigned long)cadence;
  phase = owner_cadence_phase(ch, cadence);
  remainder = pulse % interval;
  if (remainder >= phase)
    deadline = pulse - (remainder - phase);
  else
    deadline = pulse - (interval - (phase - remainder));
  return (long)(deadline - earliest_due) >= 0L;
}

static long cadence_delay(struct char_data *ch, long cadence)
{
  unsigned long interval;
  unsigned long phase;
  unsigned long remainder;

  if (cadence <= 0)
    return 1L;
  interval = (unsigned long)cadence;
  phase = owner_cadence_phase(ch, cadence);
  remainder = pulse % interval;
  if (remainder == phase)
    return cadence;
  if (remainder < phase)
    return (long)(phase - remainder);
  return (long)(interval - (remainder - phase));
}

static long next_owner_delay(struct char_data *ch)
{
  long delay = LONG_MAX;
  long candidate;

  if (ch == NULL)
    return 0L;
  if (ch->desc != NULL)
  {
    candidate = cadence_delay(ch, CHARACTER_PSP_CADENCE);
    if (candidate < delay)
      delay = candidate;
    if (!IS_NPC(ch))
    {
      candidate = cadence_delay(ch, PULSE_HINTS);
      if (candidate < delay)
        delay = candidate;
    }
  }
  if (is_in_world(ch))
  {
    candidate = cadence_delay(ch, PULSE_LUMINARI);
    if (candidate < delay)
      delay = candidate;
    candidate = cadence_delay(ch, PULSE_VIOLENCE);
    if (candidate < delay)
      delay = candidate;
  }
  if (is_in_world(ch) || ch->desc != NULL)
  {
    candidate = cadence_delay(ch, CHARACTER_DEVICE_CADENCE);
    if (candidate < delay)
      delay = candidate;
    candidate = cadence_delay(ch, CHARACTER_QUEST_CADENCE);
    if (candidate < delay)
      delay = candidate;
  }
  if (has_walk_state(ch))
  {
    candidate = cadence_delay(ch, CHARACTER_WALK_CADENCE);
    if (candidate < delay)
      delay = candidate;
  }
  if (IS_PERFORMING(ch))
  {
    candidate = cadence_delay(ch, PULSE_VERSE_INTERVAL);
    if (candidate < delay)
      delay = candidate;
  }
  return delay == LONG_MAX ? 0L : delay;
}

static bool runtime_handle_matches(struct event_runtime_handle handle,
                                   const struct game_event_context *context)
{
  return context != NULL && handle.id == context->event_id;
}

static void borrowed_owner_cleanup(void *payload)
{
  struct char_data *ch = payload;

  if (ch == NULL || event_runtime_handle_is_none(ch->character_periodic_event_handle) ||
      event_runtime_handle_is_live(ch->character_periodic_event_handle))
    return;
  ch->character_periodic_event_handle = EVENT_RUNTIME_HANDLE_NONE;
  ch->character_periodic_due_pulse = 0U;
  if (scheduled_count > 0U)
    scheduled_count--;
}

static bool register_character_maintenance_type(void)
{
  struct game_event_type_config config;
  const char *registered_name;
  enum game_scheduler_status status;

  if (!event_runtime_is_initialized())
    return false;
  registered_name = event_runtime_type_name(character_maintenance_event_type);
  if (registered_name != NULL && !strcmp(registered_name, "character.maintenance"))
    return true;
  character_maintenance_event_type = 0U;
  memset(&config, 0, sizeof(config));
  config.name = "character.maintenance";
  config.handler = character_periodic_event;
  config.cleanup = borrowed_owner_cleanup;
  config.lateness_policy = GAME_EVENT_LATENESS_RUN_ONCE;
  config.max_events = CHARACTER_PERIODIC_MAX_OWNERS;
  config.max_events_per_owner = 1U;
  config.requires_owner = true;
  status = event_runtime_register_type(&config, &character_maintenance_event_type);
  if (status != GAME_SCHEDULER_OK)
  {
    log("SYSERR: unable to register native event type 'character.maintenance' (status %d).",
        status);
    return false;
  }
  return true;
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

static bool dispatch_due_work(struct char_data *ch, unsigned long earliest_due)
{
  dispatching_owner = ch;
  dispatching_owner_forgotten = false;
  if (has_walk_state(ch) && cadence_due(ch, CHARACTER_WALK_CADENCE, earliest_due))
  {
    walk_executions++;
    process_walkto_action(ch);
  }
  if (callback_owner_still_live() && ch->desc != NULL &&
      cadence_due(ch, CHARACTER_PSP_CADENCE, earliest_due))
  {
    psp_executions++;
    regen_psp_one(ch);
  }
  if (callback_owner_still_live() && is_in_world(ch) &&
      cadence_due(ch, PULSE_LUMINARI, earliest_due))
  {
    luminari_executions++;
    process_character_environment_and_recovery(ch);
  }
  if (callback_owner_still_live() && IS_PERFORMING(ch) &&
      cadence_due(ch, PULSE_VERSE_INTERVAL, earliest_due))
  {
    bardic_executions++;
    advance_bardic_performance(ch);
  }
  if (callback_owner_still_live() && ch->desc != NULL && !IS_NPC(ch) &&
      cadence_due(ch, PULSE_HINTS, earliest_due))
  {
    hint_executions++;
    show_hint_one(ch);
  }
  if (callback_owner_still_live() && is_in_world(ch) &&
      cadence_due(ch, PULSE_VIOLENCE, earliest_due))
  {
    d20_round_executions++;
    proc_d20_round_one(ch);
    if (DEAD(ch))
      character_periodic_forget(ch);
  }
  if (callback_owner_still_live() && is_in_world(ch) &&
      cadence_due(ch, PULSE_VIOLENCE, earliest_due))
  {
    damage_effect_executions++;
    update_damage_and_effects_over_time_one(ch);
  }
  if (callback_owner_still_live() && ch->desc != NULL && is_in_world(ch) &&
      cadence_due(ch, PULSE_VIOLENCE, earliest_due))
  {
    player_misc_executions++;
    update_player_misc_one(ch);
  }
  if (callback_owner_still_live() && (is_in_world(ch) || ch->desc != NULL) &&
      cadence_due(ch, CHARACTER_DEVICE_CADENCE, earliest_due))
  {
    device_executions++;
    check_device_one(ch);
  }
  if (callback_owner_still_live() && (is_in_world(ch) || ch->desc != NULL) &&
      cadence_due(ch, CHARACTER_QUEST_CADENCE, earliest_due))
  {
    timed_quest_executions++;
    check_timed_quests_one(ch);
  }

  if (!callback_owner_still_live())
  {
    dispatching_owner = NULL;
    return false;
  }
  dispatching_owner = NULL;
  return is_owner_eligible(ch);
}

static struct game_event_result character_periodic_event(
    const struct game_event_context *context)
{
  struct char_data *ch = context != NULL ? context->payload : NULL;
  unsigned long earliest_due;
  long delay;

  if (ch == NULL)
    return game_event_result_complete();
  callback_count++;
  if (!ch->character_periodic_registered || !is_owner_eligible(ch))
  {
    if (runtime_handle_matches(ch->character_periodic_event_handle, context))
    {
      ch->character_periodic_event_handle = EVENT_RUNTIME_HANDLE_NONE;
      ch->character_periodic_due_pulse = 0U;
      if (scheduled_count > 0U)
        scheduled_count--;
    }
    registry_remove(ch);
    refill_capacity();
    return game_event_result_complete();
  }

  earliest_due = ch->character_periodic_due_pulse;
  if (!dispatch_due_work(ch, earliest_due))
  {
    character_periodic_forget(ch);
    return game_event_result_complete();
  }
  if (!is_owner_eligible(ch))
  {
    if (runtime_handle_matches(ch->character_periodic_event_handle, context))
    {
      ch->character_periodic_event_handle = EVENT_RUNTIME_HANDLE_NONE;
      ch->character_periodic_due_pulse = 0U;
      if (scheduled_count > 0U)
        scheduled_count--;
    }
    registry_remove(ch);
    refill_capacity();
    return game_event_result_complete();
  }
  if (!runtime_handle_matches(ch->character_periodic_event_handle, context))
    return game_event_result_complete();
  delay = next_owner_delay(ch);
  ch->character_periodic_due_pulse = delay > 0L ? pulse + (unsigned long)delay : 0U;
  return delay > 0L ? game_event_result_reschedule_after((game_tick_t)delay)
                    : game_event_result_complete();
}

static bool schedule_owner(struct char_data *ch)
{
  struct game_event_owner owner;
  long delay;

  if (!initialized || !scheduled || shutting_down || ch == NULL ||
      !ch->character_periodic_registered ||
      !event_runtime_handle_is_none(ch->character_periodic_event_handle))
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
  if (event_runtime_schedule_owned_after(character_maintenance_event_type, owner,
                                         (game_tick_t)delay, ch,
                                         &ch->character_periodic_event_handle) !=
      GAME_SCHEDULER_OK)
  {
    ch->character_periodic_due_pulse = 0U;
    note_rejection();
    return false;
  }
  ch->character_periodic_due_pulse = pulse + (unsigned long)delay;
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
    if (event_runtime_handle_is_none(ch->character_periodic_event_handle))
      schedule_owner(ch);
  }
  refilling = false;
}

void character_periodic_sync(struct char_data *ch)
{
  struct event_runtime_handle handle;
  game_tick_t remaining;
  long delay;

  if (!initialized || !scheduled || ch == NULL)
    return;
  if (dispatching_owner == ch)
    return;
  if (!is_owner_eligible(ch))
  {
    character_periodic_forget(ch);
    return;
  }
  registry_add(ch);
  if (event_runtime_handle_is_none(ch->character_periodic_event_handle))
  {
    schedule_owner(ch);
    return;
  }
  delay = next_owner_delay(ch);
  if (delay <= 0L)
    return;
  remaining = 0U;
  if (event_runtime_remaining(ch->character_periodic_event_handle, &remaining) ==
          GAME_SCHEDULER_OK &&
      remaining <= (game_tick_t)delay)
    return;
  handle = ch->character_periodic_event_handle;
  ch->character_periodic_event_handle = EVENT_RUNTIME_HANDLE_NONE;
  ch->character_periodic_due_pulse = 0U;
  if (scheduled_count > 0U)
    scheduled_count--;
  (void)event_runtime_cancel(handle);
  schedule_owner(ch);
}

void character_periodic_forget(struct char_data *ch)
{
  struct event_runtime_handle handle;

  if (ch == NULL)
    return;
  if (dispatching_owner == ch)
    dispatching_owner_forgotten = true;
  if (!event_runtime_handle_is_none(ch->character_periodic_event_handle))
  {
    handle = ch->character_periodic_event_handle;
    ch->character_periodic_event_handle = EVENT_RUNTIME_HANDLE_NONE;
    ch->character_periodic_due_pulse = 0U;
    if (scheduled_count > 0U)
      scheduled_count--;
    (void)event_runtime_cancel(handle);
  }
  else
    ch->character_periodic_due_pulse = 0U;
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

static void handle_character_damaged(const struct domain_event_context *context,
                                     void *handler_context)
{
  const struct domain_character_damaged *event;
  struct char_data *ch;

  (void)handler_context;
  event = context->payload;
  ch = domain_event_resolve(context->bus, event->target, DOMAIN_ENTITY_CHARACTER);
  character_periodic_sync(ch);
}

enum domain_event_status character_periodic_register_handlers(struct domain_event_bus *bus)
{
  struct domain_event_handler_config handlers[] = {
      {DOMAIN_EVENT_CHARACTER_MOVED, "character-periodic-moved", 90,
       handle_character_moved, NULL},
      {DOMAIN_EVENT_CHARACTER_DAMAGED, "character-periodic-damaged", 90,
       handle_character_damaged, NULL},
  };
  size_t index;
  enum domain_event_status status;

  if (bus == NULL)
    return DOMAIN_EVENT_INVALID_ARGUMENT;
  for (index = 0U; index < sizeof(handlers) / sizeof(handlers[0]); index++)
  {
    status = domain_event_register_handler(bus, &handlers[index]);
    if (status != DOMAIN_EVENT_OK)
      return status;
  }
  return DOMAIN_EVENT_OK;
}

void character_periodic_init(void)
{
  struct char_data *ch;
  bool native_ready;
  bool requested;

  if (initialized)
    return;
#ifdef LUMINARI_CUTEST
  requested = test_selection_set ? test_scheduled_selection : configured_scheduled();
#else
  requested = configured_scheduled();
#endif
  native_ready = register_character_maintenance_type();
  scheduled = requested && native_ready;
  initialized = true;
  shutting_down = false;
  if (requested && !native_ready)
    log("WARNING: native character-maintenance event type unavailable; using legacy heartbeat.");
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
uint64_t character_periodic_d20_round_executions(void) { return d20_round_executions; }
uint64_t character_periodic_device_executions(void) { return device_executions; }
uint64_t character_periodic_timed_quest_executions(void) { return timed_quest_executions; }

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
    if (!event_runtime_handle_is_none(ch->character_periodic_event_handle))
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
  d20_round_executions = 0U;
  device_executions = 0U;
  timed_quest_executions = 0U;
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
