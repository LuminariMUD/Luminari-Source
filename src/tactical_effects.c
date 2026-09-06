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

#define DEFENSE_ROUND_PULSES (6 * PASSES_PER_SEC)

static game_event_type_id_t defense_event_type;

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
    return true;
  defense_event_type = 0U;
  config.name = "tactical.defensive-casting.expiry";
  config.handler = defense_expire;
  config.cleanup = free;
  config.lateness_policy = GAME_EVENT_LATENESS_RUN_ONCE;
  config.max_events = 32768U;
  config.max_events_per_owner = 1U;
  config.requires_owner = true;
  return event_runtime_register_type(&config, &defense_event_type) == GAME_SCHEDULER_OK;
}

void tactical_effects_shutdown(void)
{
  /* Character-periodic teardown pauses owners before the event types disappear. */
  defense_event_type = 0U;
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
