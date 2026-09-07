#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "activity_manager.h"
#include "domain_event_runtime.h"
#include "domain_event_world.h"
#include "event_runtime.h"
#include "spells.h"
#include "psionics.h"
#include "character/perks.h"
#include "buff_sequence.h"

struct buff_sequence
{
  struct domain_entity_handle actor, target;
  struct event_runtime_handle timer;
  struct domain_event_subscription_handle watches[4];
  struct buff_sequence *previous, *next;
  uint64_t id, casting_id;
  bool issuing;
};

struct buff_wakeup
{
  struct domain_entity_handle actor;
  uint64_t sequence_id;
};

static uint64_t next_sequence_id;
static struct buff_sequence *sequences;
static game_event_type_id_t next_cast_type;
static bool enabled;

static struct buff_sequence *sequence_for(struct char_data *ch)
{
  return ch != NULL && !IS_NPC(ch) && ch->player_specials != NULL
             ? ch->player_specials->buff_sequence
             : NULL;
}

static void release_sequence(struct buff_sequence *sequence)
{
  size_t i;

  if (sequence->previous != NULL)
    sequence->previous->next = sequence->next;
  else
    sequences = sequence->next;
  if (sequence->next != NULL)
    sequence->next->previous = sequence->previous;
  (void)event_runtime_cancel(sequence->timer);
  for (i = 0; i < 4; i++)
    (void)domain_event_unsubscribe(domain_event_runtime_bus(), sequence->watches[i]);
  free(sequence);
}

void buff_sequence_cancel(struct char_data *ch)
{
  struct buff_sequence *sequence = sequence_for(ch);
  bool was_active;

  if (ch == NULL || IS_NPC(ch) || ch->player_specials == NULL)
    return;
  was_active = IS_BUFFING(ch);
  ch->player_specials->buff_sequence = NULL;
  IS_BUFFING(ch) = false;
  GET_CURRENT_BUFF_SLOT(ch) = 0;
  if (sequence == NULL)
    return;
  release_sequence(sequence);
  if (!was_active)
    return;
  affect_from_char(ch, SPELL_MINOR_RAPID_BUFF);
  affect_from_char(ch, SPELL_RAPID_BUFF);
  affect_from_char(ch, SPELL_GREATER_RAPID_BUFF);
}

static bool schedule_next(struct buff_sequence *sequence, int seconds)
{
  struct buff_wakeup *payload = malloc(sizeof(*payload));
  struct game_event_owner owner = game_event_owner_none();

  if (payload == NULL)
    return false;
  payload->actor = sequence->actor;
  payload->sequence_id = sequence->id;
  owner.kind = GAME_EVENT_OWNER_CHARACTER;
  owner.runtime_id = payload->actor.runtime_id;
  owner.generation = payload->actor.generation;
  if (event_runtime_schedule_owned_after(next_cast_type, owner,
                                         (game_tick_t)MAX(1, seconds) * PASSES_PER_SEC, payload,
                                         &sequence->timer) != GAME_SCHEDULER_OK)
  {
    free(payload);
    return false;
  }
  return true;
}

static void participant_changed(const struct domain_event_context *context, void *data)
{
  const struct buff_wakeup *watch = data;
  struct char_data *ch = domain_event_world_resolve_character(watch->actor);
  struct buff_sequence *sequence = sequence_for(ch);

  (void)context;
  if (sequence != NULL && sequence->id == watch->sequence_id)
  {
    send_to_char(ch,
                 "Your buff sequence stops because a participant moved or became unavailable.\r\n");
    buff_sequence_cancel(ch);
  }
}

static bool watch_participant(struct buff_sequence *sequence, size_t index,
                              struct domain_entity_handle participant, domain_event_type_id_t type)
{
  struct domain_event_subscription_config config = {0};
  struct buff_wakeup *actor = malloc(sizeof(*actor));

  if (actor == NULL)
    return false;
  actor->actor = sequence->actor;
  actor->sequence_id = sequence->id;
  config.type = type;
  config.topic.role = DOMAIN_EVENT_TOPIC_SUBJECT;
  config.topic.entity = participant;
  config.owner = sequence->actor;
  config.identity = "buff.sequence.participant";
  config.handler = participant_changed;
  config.handler_context = actor;
  config.cleanup = free;
  if (domain_event_subscribe(domain_event_runtime_bus(), &config, &sequence->watches[index]) !=
      DOMAIN_EVENT_OK)
  {
    free(actor);
    return false;
  }
  return true;
}

static void casting_transitioned(const struct domain_event_context *context, void *data)
{
  const struct domain_activity_transitioned *event = context->payload;
  struct char_data *ch = domain_event_world_resolve_character(event->actor);
  struct buff_sequence *sequence = sequence_for(ch);

  (void)data;
  if (sequence == NULL || event->activity_type != PRIMARY_ACTIVITY_CASTING)
    return;
  if (sequence->issuing && event->previous_state == PRIMARY_ACTIVITY_STATE_NONE &&
      event->current_state == PRIMARY_ACTIVITY_STATE_ACTIVE)
    sequence->casting_id = event->activity_id;
  else if (sequence->casting_id == event->activity_id)
  {
    if (event->current_state == PRIMARY_ACTIVITY_STATE_COMPLETED)
    {
      sequence->casting_id = 0;
      if (!schedule_next(sequence, 1))
        buff_sequence_cancel(ch);
    }
    else if (event->current_state == PRIMARY_ACTIVITY_STATE_CANCELLED)
    {
      send_to_char(ch, "Your interrupted cast stops the buff sequence.\r\n");
      buff_sequence_cancel(ch);
    }
  }
}

static struct game_event_result cast_next(const struct game_event_context *context)
{
  const struct buff_wakeup *wakeup = context->payload;
  struct domain_entity_handle actor = wakeup->actor;
  uint64_t sequence_id = wakeup->sequence_id;
  struct char_data *ch = domain_event_world_resolve_character(actor), *target;
  struct buff_sequence *sequence = sequence_for(ch);
  int slot, spell, kind, augment = 0, delay, ordinal = 1, candidates = 0, number;
  struct char_data *candidate;
  char command[MAX_INPUT_LENGTH], target_name[MAX_INPUT_LENGTH];
  size_t i;
  game_tick_t remaining;

  if (sequence == NULL || sequence->id != sequence_id)
    return game_event_result_complete();
  sequence->timer = (struct event_runtime_handle){0};
  target = domain_event_world_resolve_character(sequence->target);
  if (ch->desc == NULL || STATE(ch->desc) != CON_PLAYING || DEAD(ch) || target == NULL ||
      DEAD(target) || IN_ROOM(ch) == NOWHERE || IN_ROOM(target) != IN_ROOM(ch) ||
      GET_POS(ch) < POS_FIGHTING || ch->primary_activity != NULL)
  {
    buff_sequence_cancel(ch);
    return game_event_result_complete();
  }
  slot = GET_CURRENT_BUFF_SLOT(ch);
  while (slot >= 0 && slot < MAX_BUFFS && GET_BUFF(ch, slot, 0) == 0)
    slot++;
  if (slot < 0 || slot >= MAX_BUFFS)
  {
    send_to_char(ch, "You finish your buff sequence.\r\n");
    buff_sequence_cancel(ch);
    return game_event_result_complete();
  }
  spell = GET_BUFF(ch, slot, 0);
  if (spell <= 0 || spell > TOP_SPELL_DEFINE || spell_info[spell].name == NULL)
  {
    send_to_char(ch, "Your buff list contains an invalid spell or power.\r\n");
    buff_sequence_cancel(ch);
    return game_event_result_complete();
  }
  GET_CURRENT_BUFF_SLOT(ch) = slot + 1;
  kind = is_spell_or_power(spell);
  delay = IS_AFFECTED(ch, AFF_TIME_STOPPED) || IS_AFFECTED(ch, AFF_RAPID_BUFF)
              ? 1
              : MAX(1, spell_info[spell].time + 1);
  if (target == ch && has_perk(ch, PERK_CLERIC_BATTLE_BLESSING))
    delay = MAX(1, delay - 1);
  strlcpy(target_name,
          IS_NPC(target) && target->player.name != NULL ? target->player.name : GET_NAME(target),
          sizeof(target_name));
  for (i = 0; target_name[i] != '\0'; i++)
    if (target_name[i] == ' ')
      target_name[i] = '-';
  if (target == ch)
    strlcpy(target_name, "self", sizeof(target_name));
  else
  {
    for (candidate = world[IN_ROOM(ch)].people; candidate != NULL;
         candidate = candidate->next_in_room)
      candidates++;
    for (ordinal = 1; ordinal <= candidates; ordinal++)
    {
      number = ordinal;
      if (get_char_vis(ch, target_name, &number, FIND_CHAR_ROOM) == target)
        break;
    }
    if (ordinal > candidates)
    {
      send_to_char(ch, "Your selected buff target is no longer visible.\r\n");
      buff_sequence_cancel(ch);
      return game_event_result_complete();
    }
  }
  sequence->issuing = true;
  send_to_char(ch, "You continue buffing... (buff cancel to stop)\r\n");
  if (kind >= 2)
  {
    snprintf(command, sizeof(command), " '%s' %d.%.*s", spell_info[spell].name, ordinal,
             (int)(sizeof(command) / 2), target_name);
    do_gen_cast(ch, command, 0, SCMD_CAST_SPELL);
  }
  else
  {
    if (PRF_FLAGGED(ch, PRF_AUGMENT_BUFFS))
      augment = max_augment_psp_allowed(ch, spell);
    snprintf(command, sizeof(command), " %d '%s' %d.%.*s", augment, spell_info[spell].name, ordinal,
             (int)(sizeof(command) / 2), target_name);
    do_manifest(ch, command, 0, SCMD_CAST_PSIONIC);
  }
  ch = domain_event_world_resolve_character(actor);
  sequence = sequence_for(ch);
  if (sequence != NULL && sequence->id == sequence_id)
  {
    sequence->issuing = false;
    if (sequence->casting_id == 0 &&
        event_runtime_remaining(sequence->timer, &remaining) != GAME_SCHEDULER_OK &&
        !schedule_next(sequence, delay))
      buff_sequence_cancel(ch);
  }
  return game_event_result_complete();
}

bool buff_sequence_start(struct char_data *ch)
{
  struct buff_sequence *sequence;
  struct char_data *target;
  bool self;

  if (!enabled || ch == NULL || IS_NPC(ch) || ch->player_specials == NULL ||
      sequence_for(ch) != NULL || ch->primary_activity != NULL || ch->desc == NULL ||
      STATE(ch->desc) != CON_PLAYING || DEAD(ch) || GET_POS(ch) < POS_FIGHTING)
    return false;
  target = GET_BUFF_TARGET(ch) != NULL ? GET_BUFF_TARGET(ch) : ch;
  if (DEAD(target) || IN_ROOM(ch) == NOWHERE || IN_ROOM(target) != IN_ROOM(ch))
    return false;
  sequence = calloc(1, sizeof(*sequence));
  if (sequence == NULL)
    return false;
  sequence->id = ++next_sequence_id;
  sequence->actor = domain_event_character_handle(ch);
  sequence->target = domain_event_character_handle(target);
  self = ch == target;
  sequence->next = sequences;
  if (sequences != NULL)
    sequences->previous = sequence;
  sequences = sequence;
  ch->player_specials->buff_sequence = sequence;
  if (!watch_participant(sequence, 0, sequence->actor, DOMAIN_EVENT_CHARACTER_MOVED) ||
      !watch_participant(sequence, 1, sequence->actor, DOMAIN_EVENT_CHARACTER_DIED) ||
      (!self && (!watch_participant(sequence, 2, sequence->target, DOMAIN_EVENT_CHARACTER_MOVED) ||
                 !watch_participant(sequence, 3, sequence->target, DOMAIN_EVENT_CHARACTER_DIED))) ||
      !schedule_next(sequence, 1))
  {
    buff_sequence_cancel(ch);
    return false;
  }
  IS_BUFFING(ch) = true;
  GET_CURRENT_BUFF_SLOT(ch) = 0;
  return true;
}

enum domain_event_status buff_sequences_init(struct domain_event_bus *bus)
{
  struct game_event_type_config type = {0};
  const struct domain_event_handler_config transitioned = {DOMAIN_EVENT_ACTIVITY_TRANSITIONED,
                                                           "buff.sequence.cast-ended", 20,
                                                           casting_transitioned, NULL};

  if (event_runtime_find_type("buff.sequence.next-cast", &next_cast_type) != GAME_SCHEDULER_OK)
  {
    type.name = "buff.sequence.next-cast";
    type.handler = cast_next;
    type.cleanup = free;
    type.lateness_policy = GAME_EVENT_LATENESS_RUN_ONCE;
    type.requires_owner = true;
    type.max_events = 32768;
    /* An instant cast can enqueue its successor before this callback is retired. */
    type.max_events_per_owner = 2;
    if (event_runtime_register_type(&type, &next_cast_type) != GAME_SCHEDULER_OK)
      return DOMAIN_EVENT_ALLOCATION_FAILED;
  }
  if (domain_event_register_handler(bus, &transitioned) != DOMAIN_EVENT_OK)
    return DOMAIN_EVENT_ALLOCATION_FAILED;
  enabled = true;
  return DOMAIN_EVENT_OK;
}

void buff_sequences_shutdown(void)
{
  enabled = false;
  while (sequences != NULL)
  {
    struct char_data *ch = domain_event_world_resolve_character(sequences->actor);

    if (ch == NULL)
      release_sequence(sequences);
    else
      buff_sequence_cancel(ch);
  }
}
