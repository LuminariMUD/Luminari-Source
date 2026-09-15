#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "actions.h"
#include "tactical_effects.h"
#include "combat/combat_encounters.h"
#include "domain_event_runtime.h"
#include "domain_event_world.h"
#include "event_runtime.h"
#include "handler.h"
#include "db.h"
#include "combat/fight.h"
#include "magic/spells.h"
#include "magic/domains_schools.h"

#define DEFENSE_ROUND_PULSES (6 * PASSES_PER_SEC)
#define HAZARD_EXPOSURE_DEFAULT_LIMIT 1024U
#define HAZARD_REJECTION_LOG_INTERVAL 100U

struct tactical_hazard_exposure
{
  struct domain_entity_handle subject;
  uint64_t next_due;
  struct event_runtime_handle event;
  struct tactical_hazard_exposure *next;
};

struct tactical_hazard_event_payload
{
  struct domain_entity_handle subject;
  room_vnum room;
  uint64_t room_generation;
  uint64_t source_identity;
};

static game_event_type_id_t defense_event_type;
static game_event_type_id_t bleeding_event_type;
static game_event_type_id_t hazard_event_type;
static bool bleeding_clock_init(void);
static bool hazard_clock_init(void);
static void remove_hazard_exposure(struct raff_node *source,
                                   struct tactical_hazard_exposure *previous,
                                   struct tactical_hazard_exposure *exposure);
static uint64_t next_hazard_source_identity = 1U;
static size_t hazard_exposure_limit = HAZARD_EXPOSURE_DEFAULT_LIMIT;
static uint64_t hazard_exposure_count;
static uint64_t hazard_exposure_rejections;
#ifdef LUMINARI_CUTEST
static tactical_hazard_test_callback hazard_test_callback;
static void *hazard_test_context;
#endif

static bool defense_character(struct char_data *ch)
{
  return ch != NULL && !IS_NPC(ch) && ch->player_specials != NULL;
}

static void clear_defense(struct char_data *ch, bool notify)
{
  struct event_runtime_handle handle = ch->defensive_casting_event;

  ch->defensive_casting_event = EVENT_RUNTIME_HANDLE_NONE;
  ch->defensive_casting_due = 0U;
  ch->defensive_casting_turn = 0U;
  GET_DEFENSIVE_CASTING_TIMER(ch) = 0;
  ch->player_specials->saved.defensive_casting_pulses = 0;
  if (!event_runtime_handle_is_none(handle))
    (void)event_runtime_cancel(handle);
  if (notify)
    send_to_char(ch, "Your defensive casting bonus fades.\r\n");
}

static struct game_event_result defense_expire(const struct game_event_context *context)
{
  struct domain_entity_handle *identity = context->payload;
  struct char_data *ch = domain_event_world_resolve_character(*identity);

  if (defense_character(ch) && ch->defensive_casting_event.id == context->event_id)
  {
    ch->defensive_casting_event = EVENT_RUNTIME_HANDLE_NONE;
    clear_defense(ch, GET_DEFENSIVE_CASTING_TIMER(ch) > 0);
  }
  return game_event_result_complete();
}

bool tactical_effects_init(void)
{
  struct game_event_type_config config = {0};
  const char *registered = event_runtime_type_name(defense_event_type);

  if (registered != NULL && !strcmp(registered, "tactical.defensive-casting.expiry"))
    return bleeding_clock_init();
  defense_event_type = 0U;
  config.name = "tactical.defensive-casting.expiry";
  config.handler = defense_expire;
  config.cleanup = free;
  config.lateness_policy = GAME_EVENT_LATENESS_RUN_ONCE;
  config.max_events = 32768U;
  config.max_events_per_owner = 1U;
  config.requires_owner = true;
  return event_runtime_register_type(&config, &defense_event_type) == GAME_SCHEDULER_OK &&
         bleeding_clock_init();
}

void tactical_effects_shutdown(void)
{
  struct raff_node *raff;

  for (raff = raff_list; raff != NULL; raff = raff->next)
    while (raff->hazard_exposures != NULL)
      remove_hazard_exposure(raff, NULL, raff->hazard_exposures);
  /* Character-periodic teardown pauses owners before the event types disappear. */
  defense_event_type = 0U;
  bleeding_event_type = 0U;
  hazard_event_type = 0U;
}

int tactical_defense_remaining(struct char_data *ch)
{
  struct combat_encounter_turn_snapshot snapshot;
  uint64_t remaining;

  if (!defense_character(ch) || GET_DEFENSIVE_CASTING_TIMER(ch) <= 0)
    return 0;
  if (ch->defensive_casting_turn != 0U && combat_encounter_get_turn(ch, &snapshot))
    remaining = snapshot.pulses_until_next_turn;
  else if (ch->defensive_casting_due != 0U)
    remaining = ch->defensive_casting_due > (uint64_t)pulse
                    ? ch->defensive_casting_due - (uint64_t)pulse
                    : 0U;
  else if (ch->player_specials->saved.defensive_casting_pulses > 0)
    remaining = ch->player_specials->saved.defensive_casting_pulses;
  else
    remaining = (uint64_t)GET_DEFENSIVE_CASTING_TIMER(ch) * DEFENSE_ROUND_PULSES;
  return remaining > INT_MAX ? INT_MAX : (int)remaining;
}

static bool schedule_defense(struct char_data *ch, int remaining)
{
  struct domain_entity_handle *identity;
  struct game_event_owner owner = game_event_owner_none();

  if (defense_event_type == 0U || remaining <= 0)
    return false;
  identity = malloc(sizeof(*identity));
  if (identity == NULL)
    return false;
  *identity = domain_event_character_handle(ch);
  owner.kind = GAME_EVENT_OWNER_CHARACTER;
  owner.runtime_id = identity->runtime_id;
  owner.generation = identity->generation;
  if (event_runtime_schedule_owned_after(defense_event_type, owner, remaining, identity,
                                         &ch->defensive_casting_event) != GAME_SCHEDULER_OK)
  {
    free(identity);
    return false;
  }
  ch->defensive_casting_due = (uint64_t)pulse + (unsigned int)remaining;
  return true;
}

bool tactical_defense_start(struct char_data *ch)
{
  struct combat_encounter_turn_snapshot snapshot;

  if (!defense_character(ch) || defense_event_type == 0U)
    return false;
  clear_defense(ch, false);
  GET_DEFENSIVE_CASTING_TIMER(ch) = 1;
  ch->player_specials->saved.defensive_casting_pulses = DEFENSE_ROUND_PULSES;
  if (combat_encounter_get_turn(ch, &snapshot) && snapshot.turn_serial < UINT64_MAX - 1U)
  {
    ch->defensive_casting_turn = snapshot.turn_serial + 1U;
    ch->defensive_casting_due = (uint64_t)pulse + snapshot.pulses_until_next_turn;
    return true;
  }
  if (schedule_defense(ch, DEFENSE_ROUND_PULSES))
    return true;
  clear_defense(ch, false);
  return false;
}

void tactical_defense_resume(struct char_data *ch)
{
  int remaining;

  if (!defense_character(ch) || defense_event_type == 0U || GET_DEFENSIVE_CASTING_TIMER(ch) <= 0 ||
      ch->defensive_casting_turn != 0U ||
      !event_runtime_handle_is_none(ch->defensive_casting_event))
    return;
  remaining = tactical_defense_remaining(ch);
  if (!schedule_defense(ch, remaining))
  {
    log("SYSERR: unable to resume Defensive Casting expiry.");
    clear_defense(ch, false);
  }
}

void tactical_defense_pause(struct char_data *ch)
{
  int remaining;
  struct event_runtime_handle handle;

  if (!defense_character(ch))
    return;
  remaining = tactical_defense_remaining(ch);
  handle = ch->defensive_casting_event;
  ch->defensive_casting_event = EVENT_RUNTIME_HANDLE_NONE;
  ch->defensive_casting_due = 0U;
  ch->defensive_casting_turn = 0U;
  ch->player_specials->saved.defensive_casting_pulses = remaining;
  if (remaining == 0)
    GET_DEFENSIVE_CASTING_TIMER(ch) = 0;
  if (!event_runtime_handle_is_none(handle))
    (void)event_runtime_cancel(handle);
}

void tactical_defense_on_turn(struct char_data *ch)
{
  if (defense_character(ch) && ch->defensive_casting_turn != 0U &&
      ch->combat_turn_serial >= ch->defensive_casting_turn)
    clear_defense(ch, GET_DEFENSIVE_CASTING_TIMER(ch) > 0);
}

void tactical_defense_leave_combat(struct char_data *ch)
{
  if (!defense_character(ch) || ch->defensive_casting_turn == 0U)
    return;
  tactical_defense_pause(ch);
  tactical_defense_resume(ch);
}

/* Bleeding Critical is one ordinary affect: affect_join adds its damage and
 * replaces its duration. Other bleeding sources keep their existing policies. */
bool tactical_bleeding_affect(const struct affected_type *af)
{
  return af != NULL && af->spell == ABILITY_BLEEDING_CRITICAL && af->location == APPLY_NONE &&
         af->source_id == 0 && af->duration >= 0 && IS_SET_AR(af->bitvector, AFF_BLEED);
}

static struct affected_type *bleeding_affect(struct char_data *ch)
{
  struct affected_type *af;

  for (af = ch != NULL ? ch->affected : NULL; af != NULL; af = af->next)
    if (tactical_bleeding_affect(af))
      return af;
  return NULL;
}

int tactical_bleeding_remaining(struct char_data *ch)
{
  struct combat_encounter_turn_snapshot snapshot;
  uint64_t remaining;
  uint64_t extra_turns;

  if (ch == NULL || bleeding_affect(ch) == NULL)
    return 0;
  if (ch->bleeding_critical_turn != 0U && combat_encounter_get_turn(ch, &snapshot))
  {
    if (ch->bleeding_critical_turn <= snapshot.turn_serial)
      remaining = 0U;
    else
    {
      extra_turns = ch->bleeding_critical_turn - snapshot.turn_serial - 1U;
      if (extra_turns > INT_MAX / DEFENSE_ROUND_PULSES)
        return INT_MAX;
      remaining = snapshot.pulses_until_next_turn + extra_turns * DEFENSE_ROUND_PULSES;
    }
  }
  else if (ch->bleeding_critical_due != 0U)
    remaining = ch->bleeding_critical_due > (uint64_t)pulse
                    ? ch->bleeding_critical_due - (uint64_t)pulse
                    : 0U;
  else
    remaining =
        ch->bleeding_critical_pulses > 0 ? ch->bleeding_critical_pulses : DEFENSE_ROUND_PULSES;
  /* A due damage tick is retained across removal/save, not reset to a round. */
  return remaining > INT_MAX ? INT_MAX : MAX(1, (int)remaining);
}

void tactical_bleeding_pause(struct char_data *ch)
{
  struct event_runtime_handle handle;

  if (ch == NULL)
    return;
  ch->bleeding_critical_pulses = tactical_bleeding_remaining(ch);
  handle = ch->bleeding_critical_event;
  ch->bleeding_critical_event = EVENT_RUNTIME_HANDLE_NONE;
  ch->bleeding_critical_due = 0U;
  ch->bleeding_critical_turn = 0U;
  if (!event_runtime_handle_is_none(handle))
    (void)event_runtime_cancel(handle);
}

static bool schedule_bleeding(struct char_data *ch, int remaining)
{
  struct domain_entity_handle *identity = malloc(sizeof(*identity));
  struct game_event_owner owner = game_event_owner_none();

  if (identity == NULL)
    return false;
  *identity = domain_event_character_handle(ch);
  owner.kind = GAME_EVENT_OWNER_CHARACTER;
  owner.runtime_id = identity->runtime_id;
  owner.generation = identity->generation;
  if (event_runtime_schedule_owned_after(bleeding_event_type, owner, remaining, identity,
                                         &ch->bleeding_critical_event) != GAME_SCHEDULER_OK)
  {
    free(identity);
    return false;
  }
  ch->bleeding_critical_due = (uint64_t)pulse + (unsigned int)remaining;
  return true;
}

void tactical_bleeding_sync(struct char_data *ch)
{
  struct combat_encounter_turn_snapshot snapshot;
  struct affected_type *af = bleeding_affect(ch);
  int remaining;

  if (af == NULL)
  {
    tactical_bleeding_pause(ch);
    return;
  }
  if (bleeding_event_type == 0U || !ch->affected_registry_live || world == NULL ||
      IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) > top_of_world || GET_POS(ch) <= POS_DEAD ||
      ch->bleeding_critical_turn != 0U ||
      !event_runtime_handle_is_none(ch->bleeding_critical_event))
    return;
  if (ch->bleeding_critical_version == UINT64_MAX)
  {
    log("SYSERR: exhausted Bleeding Critical clock identities.");
    return;
  }
  ch->bleeding_critical_version++;
  if (af->duration > 0 && ch->bleeding_critical_pulses == 0 &&
      combat_encounter_get_turn(ch, &snapshot) && snapshot.turn_serial < UINT64_MAX - 1U)
  {
    ch->bleeding_critical_turn = snapshot.turn_serial + 1U;
    ch->bleeding_critical_due = (uint64_t)pulse + snapshot.pulses_until_next_turn;
    return;
  }
  remaining = af->duration == 0 ? 1 : tactical_bleeding_remaining(ch);
  if (!schedule_bleeding(ch, remaining))
  {
    log("SYSERR: unable to admit Bleeding Critical clock; affected owner will retry.");
  }
}

/* Do not carry an affect pointer across damage: DG/death callbacks may cure or
 * replace it, or extract the subject. A replacement gets a new clock version. */
static struct char_data *bleeding_step(struct char_data *ch)
{
  struct domain_entity_handle identity = domain_event_character_handle(ch);
  struct affected_type *af = bleeding_affect(ch);
  uint64_t version = ch->bleeding_critical_version;
  int amount;
  const char *wearoff;

  if (af == NULL)
    return ch;
  amount = af->modifier;
  if (af->duration > 0)
  {
    af->duration--;
    /* Publish/save callbacks must see the next interval, never a due tick that
     * has already been charged. */
    ch->bleeding_critical_due = (uint64_t)pulse + DEFENSE_ROUND_PULSES;
    if (ch->bleeding_critical_turn != 0U)
      ch->bleeding_critical_turn = ch->combat_turn_serial + 1U;
    damage(ch, ch, amount, TYPE_SUFFERING, DAM_BLEEDING, TYPE_SPECAB_BLEEDING);
  }
  ch = domain_event_world_resolve_character(identity);
  if (ch == NULL || ch->bleeding_critical_version != version)
    return ch;
  af = bleeding_affect(ch);
  if (af != NULL && af->duration == 0)
  {
    affect_remove(ch, af);
    wearoff = get_wearoff(ABILITY_BLEEDING_CRITICAL);
    if (wearoff != NULL)
      send_to_char(ch, "%s\r\n", wearoff);
  }
  return ch;
}

static struct game_event_result bleeding_tick(const struct game_event_context *context)
{
  struct domain_entity_handle *identity = context->payload;
  struct char_data *ch = domain_event_world_resolve_character(*identity);
  struct combat_encounter_turn_snapshot snapshot;

  if (ch == NULL || ch->bleeding_critical_event.id != context->event_id)
    return game_event_result_complete();
  ch = bleeding_step(ch);
  if (ch == NULL || ch->bleeding_critical_event.id != context->event_id)
    return game_event_result_complete();
  if (GET_POS(ch) <= POS_DEAD || bleeding_affect(ch) == NULL)
  {
    tactical_bleeding_pause(ch);
    return game_event_result_complete();
  }
  if (combat_encounter_get_turn(ch, &snapshot) && snapshot.turn_serial < UINT64_MAX - 2U)
  {
    ch->bleeding_critical_event = EVENT_RUNTIME_HANDLE_NONE;
    /* A native tick immediately before a due semantic turn has already paid
     * this interval. Skip that end boundary rather than damage twice. */
    ch->bleeding_critical_turn =
        snapshot.turn_serial + (snapshot.pulses_until_next_turn == 0U ? 2U : 1U);
    ch->bleeding_critical_due = (uint64_t)pulse + (unsigned int)tactical_bleeding_remaining(ch);
    ch->bleeding_critical_pulses = 0;
    return game_event_result_complete();
  }
  ch->bleeding_critical_due = (uint64_t)pulse + DEFENSE_ROUND_PULSES;
  return game_event_result_reschedule_after(DEFENSE_ROUND_PULSES);
}

bool tactical_bleeding_on_turn_end(struct char_data *ch)
{
  uint64_t version;

  if (ch == NULL || ch->bleeding_critical_turn == 0U ||
      ch->combat_turn_serial < ch->bleeding_critical_turn)
    return true;
  version = ch->bleeding_critical_version;
  ch = bleeding_step(ch);
  if (ch == NULL)
    return false;
  if (ch->bleeding_critical_version == version && bleeding_affect(ch) != NULL)
  {
    if (combat_encounter_semantic_manages(ch) && ch->combat_turn_serial < UINT64_MAX - 1U)
    {
      ch->bleeding_critical_turn = ch->combat_turn_serial + 1U;
      ch->bleeding_critical_due = (uint64_t)pulse + DEFENSE_ROUND_PULSES;
    }
    else
    {
      tactical_bleeding_pause(ch);
      tactical_bleeding_sync(ch);
    }
  }
  return GET_POS(ch) > POS_DEAD;
}

void tactical_bleeding_leave_combat(struct char_data *ch)
{
  if (ch == NULL || ch->bleeding_critical_turn == 0U)
    return;
  tactical_bleeding_pause(ch);
  tactical_bleeding_sync(ch);
}

static bool bleeding_clock_init(void)
{
  struct game_event_type_config config = {0};
  const char *registered = event_runtime_type_name(bleeding_event_type);

  if (registered != NULL && !strcmp(registered, "tactical.bleeding-critical.tick"))
    return hazard_clock_init();
  bleeding_event_type = 0U;
  config.name = "tactical.bleeding-critical.tick";
  config.handler = bleeding_tick;
  config.cleanup = free;
  config.lateness_policy = GAME_EVENT_LATENESS_RUN_ONCE;
  config.max_events = 32768U;
  /* One queued event and one cancelled, currently dispatching predecessor. */
  config.max_events_per_owner = 2U;
  config.requires_owner = true;
  return event_runtime_register_type(&config, &bleeding_event_type) == GAME_SCHEDULER_OK &&
         hazard_clock_init();
}

void tactical_bleeding_restore_clock(struct char_data *ch, int remaining, uint64_t turn)
{
  struct combat_encounter_turn_snapshot snapshot;

  if (ch == NULL || bleeding_affect(ch) == NULL)
    return;
  tactical_bleeding_pause(ch);
  ch->bleeding_critical_pulses = MAX(1, remaining);
  if (turn != 0U && combat_encounter_get_turn(ch, &snapshot) && turn > snapshot.turn_serial)
  {
    ch->bleeding_critical_pulses = 0;
    ch->bleeding_critical_turn = turn;
    ch->bleeding_critical_due = (uint64_t)pulse + (unsigned int)MAX(1, remaining);
  }
  else
    tactical_bleeding_sync(ch);
}

static bool billowing_source(const struct raff_node *source)
{
  return source != NULL && source->spell == SPELL_BILLOWING_CLOUD && source->source_identity != 0U;
}

static bool billowing_source_active(const struct raff_node *source)
{
  uint64_t round;
  uint64_t elapsed;

  if (!billowing_source(source) || source->timer <= 0)
    return false;
  if (!source->lifetime_initialized)
    return true;
  round = (uint64_t)pulse / PULSE_VIOLENCE;
  elapsed = round > source->lifetime_round ? round - source->lifetime_round : 0U;
  return elapsed < (uint64_t)source->timer;
}

static void note_hazard_rejection(const char *reason)
{
  hazard_exposure_rejections++;
  if (hazard_exposure_rejections == 1U ||
      hazard_exposure_rejections % HAZARD_REJECTION_LOG_INTERVAL == 0U)
    log("WARNING: billowing-cloud exposure rejected (%s); rejected=%llu.", reason,
        (unsigned long long)hazard_exposure_rejections);
}

static void remove_hazard_exposure(struct raff_node *source,
                                   struct tactical_hazard_exposure *previous,
                                   struct tactical_hazard_exposure *exposure)
{
  if (previous != NULL)
    previous->next = exposure->next;
  else
    source->hazard_exposures = exposure->next;
  if (!event_runtime_handle_is_none(exposure->event))
    (void)event_runtime_cancel(exposure->event);
  if (source->hazard_exposure_count > 0U)
    source->hazard_exposure_count--;
  if (hazard_exposure_count > 0U)
    hazard_exposure_count--;
  free(exposure);
}

static struct tactical_hazard_exposure *hazard_exposure(struct raff_node *source,
                                                        struct char_data *subject, bool admit)
{
  struct domain_entity_handle identity;
  struct tactical_hazard_exposure *exposure;
  struct tactical_hazard_exposure *next;
  struct tactical_hazard_exposure *previous = NULL;

  if (!billowing_source(source) || subject == NULL)
    return NULL;
  identity = domain_event_character_handle(subject);
  if (!domain_entity_handle_is_valid(identity))
    return NULL;
  for (exposure = source->hazard_exposures; exposure != NULL; exposure = next)
  {
    next = exposure->next;
    if (domain_entity_handle_equal(exposure->subject, identity))
      return exposure;
    if (domain_event_world_resolve_character(exposure->subject) == NULL)
    {
      remove_hazard_exposure(source, previous, exposure);
      continue;
    }
    previous = exposure;
  }
  if (!admit)
    return NULL;
  if (source->hazard_exposure_count >= hazard_exposure_limit)
  {
    note_hazard_rejection("source capacity");
    return NULL;
  }
  exposure = calloc(1U, sizeof(*exposure));
  if (exposure == NULL)
  {
    note_hazard_rejection("allocation");
    return NULL;
  }
  exposure->subject = identity;
  exposure->event = EVENT_RUNTIME_HANDLE_NONE;
  exposure->next = source->hazard_exposures;
  source->hazard_exposures = exposure;
  source->hazard_exposure_count++;
  hazard_exposure_count++;
  return exposure;
}

static struct raff_node *hazard_source_in_room(room_rnum room, uint64_t identity)
{
  struct raff_node *source;

  if (world == NULL || room == NOWHERE || room > top_of_world || identity == 0U)
    return NULL;
  for (source = world[room].affected_head; source != NULL; source = source->room_next)
    if (source->source_identity == identity && billowing_source_active(source))
      return source;
  return NULL;
}

static void consume_billowing_action(struct char_data *subject, bool continued_turn)
{
  action_type action;
  int duration;

  action = is_action_available(subject, atMOVE, false) ? atMOVE : atSTANDARD;
  duration = continued_turn && combat_encounter_semantic_manages(subject) ? DEFENSE_ROUND_PULSES + 1
                                                                          : DEFENSE_ROUND_PULSES;
  start_action_cooldown(subject, action, duration);
}

static void apply_billowing_exposure(struct raff_node *source, struct char_data *subject,
                                     bool continued_turn)
{
  struct tactical_hazard_exposure *exposure;
  struct event_runtime_handle old_event;
  bool failed;

  if (!billowing_source_active(source) || subject == NULL || GET_LEVEL(subject) >= 13 ||
      IN_ROOM(subject) != source->room)
    return;
  exposure = hazard_exposure(source, subject, true);
  if (exposure == NULL)
    return;
  if (exposure->next_due > (uint64_t)pulse)
    return;
  old_event = exposure->event;
  exposure->event = EVENT_RUNTIME_HANDLE_NONE;
  exposure->next_due = (uint64_t)pulse + DEFENSE_ROUND_PULSES;
  if (!event_runtime_handle_is_none(old_event))
    (void)event_runtime_cancel(old_event);
#ifdef LUMINARI_CUTEST
  if (hazard_test_callback != NULL)
    failed = hazard_test_callback(source, subject, hazard_test_context);
  else
#endif
    /* Preserve the established self-attributed save formula. The casting level
     * is captured on the source instead of read from mutable caster state. */
    failed = !savingthrow(subject, subject, SAVING_FORT, 0, CAST_SPELL, source->source_level,
                          CONJURATION);
  if (failed)
  {
    send_to_char(subject, "You are bogged down by the billowing cloud!\r\n");
    act("$n is bogged down by the billowing cloud.", TRUE, subject, 0, NULL, TO_ROOM);
    consume_billowing_action(subject, continued_turn);
  }
}

static struct raff_node *resolve_hazard_source(const struct tactical_hazard_event_payload *payload)
{
  room_rnum room;

  if (payload == NULL)
    return NULL;
  room = real_room(payload->room);
  if (room == NOWHERE || world[room].event_owner_generation != payload->room_generation)
    return NULL;
  return hazard_source_in_room(room, payload->source_identity);
}

static bool schedule_hazard_exposure(struct raff_node *source,
                                     struct tactical_hazard_exposure *exposure,
                                     struct char_data *subject, bool leaving_combat)
{
  struct tactical_hazard_event_payload *payload;
  struct domain_entity_handle room;
  struct game_event_owner owner = game_event_owner_none();
  game_tick_t delay;

  if (!billowing_source_active(source) || exposure == NULL || subject == NULL ||
      hazard_event_type == 0U || (!leaving_combat && combat_encounter_semantic_manages(subject)) ||
      IN_ROOM(subject) != source->room || GET_POS(subject) <= POS_DEAD)
    return false;
  if (!event_runtime_handle_is_none(exposure->event))
  {
    if (event_runtime_handle_is_live(exposure->event))
      return true;
    exposure->event = EVENT_RUNTIME_HANDLE_NONE;
  }
  payload = calloc(1U, sizeof(*payload));
  if (payload == NULL)
  {
    note_hazard_rejection("event allocation");
    return false;
  }
  room = domain_event_room_handle(source->room);
  payload->subject = exposure->subject;
  payload->room = GET_ROOM_VNUM(source->room);
  payload->room_generation = room.generation;
  payload->source_identity = source->source_identity;
  owner.kind = GAME_EVENT_OWNER_CHARACTER;
  owner.runtime_id = exposure->subject.runtime_id;
  owner.generation = exposure->subject.generation;
  delay = exposure->next_due > (uint64_t)pulse ? exposure->next_due - (uint64_t)pulse : 1U;
  if (event_runtime_schedule_owned_after(hazard_event_type, owner, delay, payload,
                                         &exposure->event) != GAME_SCHEDULER_OK)
  {
    exposure->event = EVENT_RUNTIME_HANDLE_NONE;
    free(payload);
    note_hazard_rejection("event capacity");
    return false;
  }
  return true;
}

static struct game_event_result hazard_tick(const struct game_event_context *context)
{
  struct tactical_hazard_event_payload *payload = context != NULL ? context->payload : NULL;
  struct tactical_hazard_exposure *exposure;
  struct raff_node *source;
  struct char_data *subject;
  uint64_t source_identity;
  room_vnum room;

  source = resolve_hazard_source(payload);
  subject = payload != NULL ? domain_event_world_resolve_character(payload->subject) : NULL;
  exposure = source != NULL && subject != NULL ? hazard_exposure(source, subject, false) : NULL;
  if (exposure == NULL || exposure->event.id != context->event_id)
    return game_event_result_complete();
  exposure->event = EVENT_RUNTIME_HANDLE_NONE;
  if (IN_ROOM(subject) != source->room || combat_encounter_semantic_manages(subject))
    return game_event_result_complete();
  source_identity = source->source_identity;
  room = GET_ROOM_VNUM(source->room);
  apply_billowing_exposure(source, subject, false);
  source =
      real_room(room) != NOWHERE ? hazard_source_in_room(real_room(room), source_identity) : NULL;
  subject = domain_event_world_resolve_character(payload->subject);
  exposure = source != NULL && subject != NULL ? hazard_exposure(source, subject, false) : NULL;
  if (exposure != NULL && IN_ROOM(subject) == source->room)
    (void)schedule_hazard_exposure(source, exposure, subject, false);
  return game_event_result_complete();
}

static bool hazard_clock_init(void)
{
  struct game_event_type_config config = {0};
  const char *registered = event_runtime_type_name(hazard_event_type);

  if (registered != NULL && !strcmp(registered, "tactical.room-hazard.exposure"))
    return true;
  hazard_event_type = 0U;
  config.name = "tactical.room-hazard.exposure";
  config.handler = hazard_tick;
  config.cleanup = free;
  config.lateness_policy = GAME_EVENT_LATENESS_RUN_ONCE;
  config.max_events = 65536U;
  config.max_events_per_owner = 64U;
  config.requires_owner = true;
  return event_runtime_register_type(&config, &hazard_event_type) == GAME_SCHEDULER_OK;
}

bool tactical_room_hazard_prepare_source(struct raff_node *source, int level)
{
  if (source == NULL || source->spell != SPELL_BILLOWING_CLOUD)
    return true;
  if (source->source_identity != 0U)
    return true;
  if (next_hazard_source_identity == 0U)
    return false;
  source->source_identity = next_hazard_source_identity++;
  source->source_level = MAX(1, level);
  return true;
}

void tactical_room_hazard_source_created(struct raff_node *source)
{
  struct tactical_hazard_exposure *exposure;
  struct char_data *subject;
  struct char_data *next;

  if (!billowing_source_active(source) || world == NULL || source->room == NOWHERE ||
      source->room > top_of_world)
    return;
  for (subject = world[source->room].people; subject != NULL; subject = next)
  {
    next = subject->next_in_room;
    apply_billowing_exposure(source, subject, false);
    exposure = hazard_exposure(source, subject, false);
    if (exposure != NULL)
      (void)schedule_hazard_exposure(source, exposure, subject, false);
  }
}

void tactical_room_hazards_rebuild(void)
{
  struct tactical_hazard_exposure *exposure;
  struct raff_node *source;
  struct char_data *subject;

  for (source = raff_list; source != NULL; source = source->next)
  {
    while (source->hazard_exposures != NULL)
      remove_hazard_exposure(source, NULL, source->hazard_exposures);
    if (!billowing_source_active(source) || world == NULL || source->room == NOWHERE ||
        source->room > top_of_world)
      continue;
    for (subject = world[source->room].people; subject != NULL; subject = subject->next_in_room)
    {
      exposure = hazard_exposure(source, subject, true);
      if (exposure != NULL)
        (void)schedule_hazard_exposure(source, exposure, subject, false);
    }
  }
}

void tactical_room_hazard_source_removed(struct raff_node *source)
{
  while (source != NULL && source->hazard_exposures != NULL)
    remove_hazard_exposure(source, NULL, source->hazard_exposures);
}

static void handle_hazard_movement(const struct domain_event_context *context, void *data)
{
  const struct domain_character_moved *event = context->payload;
  struct tactical_hazard_exposure *exposure;
  struct raff_node *source;
  struct raff_node *next;
  struct char_data *subject;
  struct domain_entity_handle current_room;
  struct room_data *from_room;
  struct room_data *to_room;

  (void)data;
  subject = domain_event_resolve(context->bus, event->character, DOMAIN_ENTITY_CHARACTER);
  if (subject == NULL || IN_ROOM(subject) == NOWHERE)
    return;
  current_room = domain_event_room_handle(IN_ROOM(subject));
  if (!domain_entity_handle_equal(current_room, event->to_room))
    return;
  from_room = domain_event_resolve(context->bus, event->from_room, DOMAIN_ENTITY_ROOM);
  to_room = domain_event_resolve(context->bus, event->to_room, DOMAIN_ENTITY_ROOM);
  if (from_room != NULL && to_room != NULL)
  {
    apply_wall_crossing(subject, (room_rnum)(from_room - world), (room_rnum)(to_room - world),
                        event->direction);
    subject = domain_event_resolve(context->bus, event->character, DOMAIN_ENTITY_CHARACTER);
    if (subject == NULL || IN_ROOM(subject) == NOWHERE ||
        !domain_entity_handle_equal(domain_event_room_handle(IN_ROOM(subject)), event->to_room))
      return;
  }
  for (source = world[IN_ROOM(subject)].affected_head; source != NULL; source = next)
  {
    next = source->room_next;
    if (!billowing_source_active(source))
      continue;
    apply_billowing_exposure(source, subject, false);
    exposure = hazard_exposure(source, subject, false);
    if (exposure != NULL)
      (void)schedule_hazard_exposure(source, exposure, subject, false);
  }
}

enum domain_event_status tactical_effects_register_handlers(struct domain_event_bus *bus)
{
  struct domain_event_handler_config config = {
      DOMAIN_EVENT_CHARACTER_MOVED, "tactical.room-hazard-entry", 70, handle_hazard_movement, NULL};

  if (bus == NULL)
    return DOMAIN_EVENT_INVALID_ARGUMENT;
  return domain_event_register_handler(bus, &config);
}

void tactical_room_hazards_enter_combat(struct char_data *subject)
{
  struct tactical_hazard_exposure *exposure;
  struct raff_node *source;

  if (subject == NULL || world == NULL || IN_ROOM(subject) == NOWHERE ||
      IN_ROOM(subject) > top_of_world)
    return;
  for (source = world[IN_ROOM(subject)].affected_head; source != NULL; source = source->room_next)
  {
    exposure = hazard_exposure(source, subject, false);
    if (exposure != NULL && !event_runtime_handle_is_none(exposure->event))
    {
      (void)event_runtime_cancel(exposure->event);
      exposure->event = EVENT_RUNTIME_HANDLE_NONE;
    }
  }
}

void tactical_room_hazards_leave_combat(struct char_data *subject)
{
  struct tactical_hazard_exposure *exposure;
  struct raff_node *source;

  if (subject == NULL || world == NULL || IN_ROOM(subject) == NOWHERE ||
      IN_ROOM(subject) > top_of_world)
    return;
  for (source = world[IN_ROOM(subject)].affected_head; source != NULL; source = source->room_next)
  {
    exposure = hazard_exposure(source, subject, false);
    if (exposure != NULL)
      (void)schedule_hazard_exposure(source, exposure, subject, true);
  }
}

void tactical_room_hazards_forget(struct char_data *subject)
{
  struct domain_entity_handle identity;
  struct tactical_hazard_exposure *exposure;
  struct tactical_hazard_exposure *next;
  struct tactical_hazard_exposure *previous;
  struct raff_node *source;

  if (subject == NULL)
    return;
  identity = domain_event_character_handle(subject);
  if (!domain_entity_handle_is_valid(identity))
    return;
  for (source = raff_list; source != NULL; source = source->next)
  {
    previous = NULL;
    for (exposure = source->hazard_exposures; exposure != NULL; exposure = next)
    {
      next = exposure->next;
      if (domain_entity_handle_equal(exposure->subject, identity))
      {
        remove_hazard_exposure(source, previous, exposure);
        continue;
      }
      previous = exposure;
    }
  }
}

bool tactical_room_hazards_on_turn_end(struct char_data *subject)
{
  struct domain_entity_handle identity;
  struct tactical_hazard_exposure *exposure;
  struct raff_node *source;
  struct raff_node *next;
  room_rnum room;

  if (subject == NULL || world == NULL || IN_ROOM(subject) == NOWHERE ||
      IN_ROOM(subject) > top_of_world)
    return subject != NULL;
  identity = domain_event_character_handle(subject);
  room = IN_ROOM(subject);
  for (source = world[room].affected_head; source != NULL; source = next)
  {
    next = source->room_next;
    if (!billowing_source_active(source))
      continue;
    exposure = hazard_exposure(source, subject, false);
    if (exposure != NULL && !event_runtime_handle_is_none(exposure->event))
    {
      (void)event_runtime_cancel(exposure->event);
      exposure->event = EVENT_RUNTIME_HANDLE_NONE;
    }
    apply_billowing_exposure(source, subject, true);
    subject = domain_event_world_resolve_character(identity);
    if (subject == NULL || GET_POS(subject) <= POS_DEAD || IN_ROOM(subject) != room)
      return false;
  }
  return true;
}

uint64_t tactical_room_hazard_exposures(void)
{
  return hazard_exposure_count;
}

uint64_t tactical_room_hazard_exposure_rejections(void)
{
  return hazard_exposure_rejections;
}

#ifdef LUMINARI_CUTEST
void tactical_effects_set_hazard_test_callback(tactical_hazard_test_callback callback,
                                               void *context)
{
  hazard_test_callback = callback;
  hazard_test_context = context;
}

void tactical_effects_set_hazard_exposure_limit_for_test(size_t limit)
{
  hazard_exposure_limit = limit;
}
#endif
