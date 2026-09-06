#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "tactical_effects.h"
#include "combat/combat_encounters.h"
#include "domain_event_runtime.h"
#include "domain_event_world.h"
#include "event_runtime.h"
#include "handler.h"
#include "db.h"
#include "combat/fight.h"
#include "magic/spells.h"

#define DEFENSE_ROUND_PULSES (6 * PASSES_PER_SEC)

static game_event_type_id_t defense_event_type;
static game_event_type_id_t bleeding_event_type;
static bool bleeding_clock_init(void);

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
  /* Character-periodic teardown pauses owners before the event types disappear. */
  defense_event_type = 0U;
  bleeding_event_type = 0U;
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
    return true;
  bleeding_event_type = 0U;
  config.name = "tactical.bleeding-critical.tick";
  config.handler = bleeding_tick;
  config.cleanup = free;
  config.lateness_policy = GAME_EVENT_LATENESS_RUN_ONCE;
  config.max_events = 32768U;
  /* One queued event and one cancelled, currently dispatching predecessor. */
  config.max_events_per_owner = 2U;
  config.requires_owner = true;
  return event_runtime_register_type(&config, &bleeding_event_type) == GAME_SCHEDULER_OK;
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
