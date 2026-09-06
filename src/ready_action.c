#include "conf.h"
#include "sysdep.h"

#include "ready_action.h"
#include "activity_manager.h"
#include "combat/fight.h"
#include "combat/combat_encounters.h"

#include "structs.h"
#include "actions.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "domain_event_runtime.h"
#include "domain_event_types.h"
#include "domain_event_world.h"
#include "event_runtime.h"
#include "handler.h"
#include "interpreter.h"
#include "constants.h"
#include "character/abilities.h"
#include "magic/spells.h"
#include "magic/spell_prep.h"
#include "mud_event.h"
#include "movement/door_state.h"

#define READY_COMMAND_MAX (MAX_INPUT_LENGTH - 1)
#define READY_TARGET_MAX 80

struct ready_action
{
  struct domain_entity_handle owner;
  struct domain_entity_handle room;
  struct domain_event_subscription_handle entry_subscription;
  struct domain_event_subscription_handle movement_subscription;
  struct domain_event_subscription_handle death_subscription;
  struct domain_event_subscription_handle combat_subscription;
  struct event_runtime_handle execution_handle;
  struct event_runtime_handle expiry_handle;
  struct domain_event_subscription_handle target_subscriptions[3];
  struct domain_entity_handle attack_target;
  bool attack;
  bool counterspell;
  int counter_spellnum;
  bool on_casting;
  bool on_ally;
  struct domain_entity_handle protected_ally;
  uint64_t cast_id;
  unsigned int references;
  int door_direction; /* -1 for entry readiness. */
  uint64_t exit_identity;
  struct domain_entity_handle destination;
  char *command;
  char *target;
};

struct ready_execution
{
  struct domain_entity_handle owner;
  struct domain_entity_handle room;
  char command[READY_COMMAND_MAX + 1];
};

static game_event_type_id_t ready_execution_event_type;
static game_event_type_id_t ready_expiry_event_type;
static struct game_event_owner ready_owner(struct domain_entity_handle owner);
static bool watch_ready_target(struct ready_action *action);

static bool schedule_ready_expiry(struct ready_action *action)
{
  struct ready_execution *expiry = calloc(1U, sizeof(*expiry));

  if (expiry == NULL)
    return false;
  expiry->owner = action->owner;
  if (event_runtime_schedule_owned_after(ready_expiry_event_type, ready_owner(action->owner),
                                         6 * PASSES_PER_SEC, expiry,
                                         &action->expiry_handle) != GAME_SCHEDULER_OK)
  {
    free(expiry);
    return false;
  }
  return true;
}


#define READY_LATENCY_CAPACITY 1024U
static uint64_t ready_lateness[READY_LATENCY_CAPACITY];
static uint64_t ready_callbacks;

void ready_action_latency_reset(void)
{
  ready_callbacks = 0U;
  memset(ready_lateness, 0, sizeof(ready_lateness));
}

static int compare_lateness(const void *left, const void *right)
{
  uint64_t a = *(const uint64_t *)left;
  uint64_t b = *(const uint64_t *)right;

  return (a > b) - (a < b);
}

void ready_action_latency_read(struct ready_action_latency *stats)
{
  uint64_t sorted[READY_LATENCY_CAPACITY];
  size_t count = MIN(ready_callbacks, READY_LATENCY_CAPACITY);

  if (stats == NULL)
    return;
  memset(stats, 0, sizeof(*stats));
  stats->callbacks = ready_callbacks;
  stats->samples = count;
  if (count == 0U)
    return;
  memcpy(sorted, ready_lateness, count * sizeof(*sorted));
  qsort(sorted, count, sizeof(*sorted), compare_lateness);
  stats->p50 = sorted[(count * 50U + 99U) / 100U - 1U];
  stats->p95 = sorted[(count * 95U + 99U) / 100U - 1U];
  stats->p99 = sorted[(count * 99U + 99U) / 100U - 1U];
  stats->maximum = sorted[count - 1U];
}


static struct game_event_owner ready_owner(struct domain_entity_handle owner)
{
  struct game_event_owner result = game_event_owner_none();

  if (owner.kind != DOMAIN_ENTITY_CHARACTER || !domain_entity_handle_is_valid(owner))
    return result;
  result.kind = GAME_EVENT_OWNER_CHARACTER;
  result.runtime_id = owner.runtime_id;
  result.generation = owner.generation;
  return result;
}

static struct char_data *resolve_character(struct domain_event_bus *bus,
                                           struct domain_entity_handle handle)
{
  return domain_event_resolve(bus, handle, DOMAIN_ENTITY_CHARACTER);
}

static void ready_subscription_cleanup(void *handler_context)
{
  struct ready_action *action = handler_context;
  struct domain_event_bus *bus;
  struct char_data *ch;

  if (action == NULL || action->references == 0U)
    return;
  action->references--;
  if (action->references != 0U)
    return;
  bus = domain_event_runtime_bus();
  ch = bus != NULL ? resolve_character(bus, action->owner) : NULL;
  if (ch != NULL && ch->ready_action == action)
    ch->ready_action = NULL;
  if (!event_runtime_handle_is_none(action->execution_handle))
    (void)event_runtime_cancel(action->execution_handle);
  if (!event_runtime_handle_is_none(action->expiry_handle))
    (void)event_runtime_cancel(action->expiry_handle);
  free(action->command);
  free(action->target);
  free(action);
}

static void cancel_action(struct ready_action *action, bool notify)
{
  struct domain_event_subscription_handle handles[7];
  struct domain_event_bus *bus;
  struct char_data *ch;
  size_t index;

  if (action == NULL)
    return;
  bus = domain_event_runtime_bus();
  if (bus == NULL)
    return;
  ch = resolve_character(bus, action->owner);
  if (notify && ch != NULL)
    send_to_char(ch, "Your readied action is cancelled.\r\n");
  if (!event_runtime_handle_is_none(action->execution_handle))
  {
    (void)event_runtime_cancel(action->execution_handle);
    action->execution_handle = EVENT_RUNTIME_HANDLE_NONE;
  }
  if (ch != NULL && ch->ready_action == action)
    ch->ready_action = NULL;
  if (!event_runtime_handle_is_none(action->expiry_handle))
  {
    (void)event_runtime_cancel(action->expiry_handle);
    action->expiry_handle = EVENT_RUNTIME_HANDLE_NONE;
  }
  for (index = 0U; index < 3U; index++)
  {
    handles[index + 3U] = action->target_subscriptions[index];
    action->target_subscriptions[index] = domain_event_subscription_handle_none();
  }
  handles[6] = action->combat_subscription;
  action->combat_subscription = domain_event_subscription_handle_none();
  handles[0] = action->entry_subscription;
  handles[1] = action->movement_subscription;
  handles[2] = action->death_subscription;
  action->entry_subscription = domain_event_subscription_handle_none();
  action->movement_subscription = domain_event_subscription_handle_none();
  action->death_subscription = domain_event_subscription_handle_none();
  for (index = 0U; index < 7U; index++)
    if (!domain_event_subscription_handle_is_none(handles[index]))
      (void)domain_event_unsubscribe(bus, handles[index]);
}

void ready_action_cancel(struct char_data *ch, bool notify)
{
  if (ch != NULL)
    cancel_action(ch->ready_action, notify);
}

static void ready_execution_cleanup(void *payload)
{
  free(payload);
}

static bool ready_door_is_live(const struct ready_action *action, struct char_data *ch)
{
  struct room_direction_data *exit;

  if (IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) > top_of_world ||
      !domain_entity_handle_equal(domain_event_room_handle(IN_ROOM(ch)), action->room))
    return false;
  exit = world[IN_ROOM(ch)].dir_option[action->door_direction];
  return exit != NULL &&
         door_state_identity(IN_ROOM(ch), action->door_direction) == action->exit_identity &&
         domain_entity_handle_equal(domain_event_room_handle(exit->to_room), action->destination) &&
         (exit->exit_info & EX_ISDOOR) != 0 && !is_exit_hidden(ch, action->door_direction);
}

/* Countering uses speech and hands; it is neither a weapon strike nor a new cast. */
static bool counterspell_allowed(struct char_data *owner, struct char_data *caster)
{
  if (owner == NULL || IS_NPC(owner) || !IS_CASTER(owner) || IN_ROOM(owner) == NOWHERE ||
      DEAD(owner) || GET_POS(owner) < POS_FIGHTING || owner->primary_activity != NULL ||
      IS_CASTING(owner) || HAS_WAIT(owner) || AFF_FLAGGED(owner, AFF_STUN) ||
      AFF_FLAGGED(owner, AFF_DAZED) || AFF_FLAGGED(owner, AFF_PARALYZED) ||
      AFF_FLAGGED(owner, AFF_NAUSEATED) || AFF_FLAGGED(owner, AFF_SILENCED) ||
      AFF_FLAGGED(owner, AFF_GRAPPLED) || AFF_FLAGGED(owner, AFF_PINNED) ||
      char_has_mud_event(owner, eSTUNNED) || ROOM_FLAGGED(IN_ROOM(owner), ROOM_SOUNDPROOF) ||
      ROOM_FLAGGED(IN_ROOM(owner), ROOM_PEACEFUL))
    return false;
  if (caster == NULL)
    return true;
  return caster != owner && !DEAD(caster) && IN_ROOM(owner) == IN_ROOM(caster) &&
         CAN_SEE(owner, caster) && !(AFF_FLAGGED(owner, AFF_CHARM) && owner->master == caster) &&
         pvp_ok(owner, caster, false);
}

static bool counterspell_observable(struct char_data *owner, struct char_data *caster)
{
  int metamagic;

  if (!counterspell_allowed(owner, caster) || !IS_CASTING(caster) ||
      is_spellnum_psionic(CASTING_SPELLNUM(caster)))
    return false;
  metamagic = CASTING_METAMAGIC(caster);
  return !IS_SET(metamagic, METAMAGIC_STILL) ||
         (!IS_SET(metamagic, METAMAGIC_SILENT) && !AFF_FLAGGED(caster, AFF_SILENCED));
}

static void execute_counterspell(struct ready_action *action, struct char_data *owner)
{
  struct domain_entity_handle owner_handle = action->owner;
  struct domain_entity_handle caster_handle = action->attack_target;
  uint64_t cast_id = action->cast_id;
  int spellnum = action->counter_spellnum;
  struct primary_activity_snapshot cast;
  struct char_data *caster;
  struct domain_event_bus *bus = domain_event_runtime_bus();

  /* Release the reservation before any terminal cast observers can run. */
  cancel_action(action, false);
  owner = resolve_character(bus, owner_handle);
  caster = resolve_character(bus, caster_handle);
  if (owner == NULL || caster == NULL || !counterspell_observable(owner, caster) ||
      !primary_activity_snapshot(caster, &cast) || cast.type != PRIMARY_ACTIVITY_CASTING ||
      cast.id != cast_id || CASTING_SPELLNUM(caster) != spellnum ||
      spell_prep_base_resource_check(owner, spellnum) == CLASS_UNDEFINED)
    return;
  /* Base extraction modifies owned resources and output buffers, with no world
   * callbacks. The exact-cast cancellation follows in the same execution step. */
  if (spell_prep_gen_extract(owner, spellnum, METAMAGIC_NONE) == CLASS_UNDEFINED)
    return;
  send_to_char(owner, "You counter %s's %s.\r\n", GET_NAME(caster), spell_name(spellnum));
  (void)primary_activity_cancel_id(caster, cast_id, PRIMARY_ACTIVITY_END_COUNTERED, true);
}

static struct game_event_result ready_execution_callback(const struct game_event_context *context)
{
  struct ready_execution *execution = context != NULL ? context->payload : NULL;
  struct domain_event_bus *bus = domain_event_runtime_bus();
  struct char_data *ch;
  struct char_data *target;
  struct primary_activity_snapshot cast;

  if (context != NULL)
  {
    ready_lateness[ready_callbacks % READY_LATENCY_CAPACITY] =
        context->now_tick > context->deadline_tick ? context->now_tick - context->deadline_tick
                                                   : 0U;
    ready_callbacks++;
  }
  if (execution == NULL || bus == NULL)
    return game_event_result_complete();
  ch = resolve_character(bus, execution->owner);
  if (ch == NULL || ch->ready_action == NULL ||
      ch->ready_action->execution_handle.id != context->event_id)
    return game_event_result_complete();
  ch->ready_action->execution_handle = EVENT_RUNTIME_HANDLE_NONE;
  if (ch->ready_action->door_direction >= 0 &&
      (!ready_door_is_live(ch->ready_action, ch) ||
       EXIT_FLAGGED(EXIT(ch, ch->ready_action->door_direction), EX_CLOSED)))
  {
    send_to_char(ch, "Your readied action is cancelled: the watched door is no longer open.\r\n");
    cancel_action(ch->ready_action, false);
    return game_event_result_complete();
  }
  if (ch->ready_action->counterspell)
  {
    execute_counterspell(ch->ready_action, ch);
    return game_event_result_complete();
  }
  if (ch->ready_action->attack)
  {
    target = resolve_character(bus, ch->ready_action->attack_target);
    if (target == NULL ||
        (ch->ready_action->on_casting &&
         (!primary_activity_snapshot(target, &cast) || cast.type != PRIMARY_ACTIVITY_CASTING ||
          cast.id != ch->ready_action->cast_id)))
    {
      cancel_action(ch->ready_action, true);
      return game_event_result_complete();
    }
    cancel_action(ch->ready_action, false);
    (void)combat_readied_attack(ch, target);
    return game_event_result_complete();
  }
  cancel_action(ch->ready_action, false);
  if (ch == NULL || GET_POS(ch) <= POS_DEAD || IN_ROOM(ch) == NOWHERE ||
      !domain_entity_handle_equal(domain_event_room_handle(IN_ROOM(ch)), execution->room))
    return game_event_result_complete();
  command_interpreter(ch, execution->command);
  return game_event_result_complete();
}

void ready_action_on_semantic_turn(struct char_data *ch)
{
  if (ch != NULL && ch->ready_action != NULL &&
      (ch->ready_action->attack || ch->ready_action->counterspell))
  {
    send_to_char(ch, "Your readied action expires as your next turn begins.\r\n");
    cancel_action(ch->ready_action, false);
  }
}

static struct game_event_result ready_expiry_callback(const struct game_event_context *context)
{
  struct ready_execution *payload = context->payload;
  struct char_data *ch = resolve_character(domain_event_runtime_bus(), payload->owner);

  if (ch != NULL && ch->ready_action != NULL &&
      ch->ready_action->expiry_handle.id == context->event_id)
  {
    ch->ready_action->expiry_handle = EVENT_RUNTIME_HANDLE_NONE;
    send_to_char(ch, "Your readied action expires.\r\n");
    cancel_action(ch->ready_action, false);
  }
  return game_event_result_complete();
}

bool ready_action_runtime_init(void)
{
  struct game_event_type_config config;
  enum game_scheduler_status status;
  const char *registered_name;

  if (!event_runtime_is_initialized())
    return false;
  registered_name = event_runtime_type_name(ready_execution_event_type);
  if (registered_name != NULL && !strcmp(registered_name, "action.ready.execute"))
    return true;
  ready_execution_event_type = 0U;
  memset(&config, 0, sizeof(config));
  config.name = "action.ready.execute";
  config.handler = ready_execution_callback;
  config.cleanup = ready_execution_cleanup;
  config.lateness_policy = GAME_EVENT_LATENESS_RUN_ONCE;
  config.max_events = 65536U;
  config.max_events_per_owner = 1U;
  config.requires_owner = true;
  status = event_runtime_register_type(&config, &ready_execution_event_type);
  if (status != GAME_SCHEDULER_OK)
  {
    log("SYSERR: unable to register native event type 'action.ready.execute' (status %d).", status);
    return false;
  }
  config.name = "action.ready.expire";
  config.handler = ready_expiry_callback;
  return event_runtime_register_type(&config, &ready_expiry_event_type) == GAME_SCHEDULER_OK;
}

void ready_action_runtime_shutdown(void)
{
  ready_action_latency_reset();
  ready_execution_event_type = 0U;
  ready_expiry_event_type = 0U;
}

static bool target_matches(const struct ready_action *action, struct char_data *entrant)
{
  if (action->target == NULL || *action->target == '\0')
    return true;
  if (entrant == NULL || entrant->player.name == NULL)
    return false;
  return isname(action->target, entrant->player.name) != 0;
}

static bool queue_ready_execution(struct ready_action *action, struct char_data *owner)
{
  struct ready_execution *execution;
  struct event_runtime_handle handle = EVENT_RUNTIME_HANDLE_NONE;

  if (!event_runtime_handle_is_none(action->execution_handle))
    return false;
  execution = calloc(1U, sizeof(*execution));
  if (execution == NULL)
  {
    send_to_char(owner, "You cannot hold that readied action.\r\n");
    cancel_action(action, false);
    return false;
  }
  execution->owner = action->owner;
  execution->room = action->room;
  snprintf(execution->command, sizeof(execution->command), "%s", action->command);
  if (event_runtime_schedule_owned_after(ready_execution_event_type, ready_owner(action->owner), 1U,
                                         execution, &handle) != GAME_SCHEDULER_OK)
  {
    free(execution);
    send_to_char(owner, "Your readied action could not be triggered.\r\n");
    cancel_action(action, false);
    return false;
  }
  action->execution_handle = handle;
  return true;
}

static void ready_entry_handler(const struct domain_event_context *context, void *handler_context)
{
  const struct domain_character_moved *moved = context->payload;
  struct ready_action *action = handler_context;
  struct char_data *owner = resolve_character(context->bus, action->owner);
  struct char_data *entrant = resolve_character(context->bus, moved->character);

  if (owner == NULL || owner->ready_action != action || entrant == NULL || entrant == owner ||
      !domain_entity_handle_equal(moved->to_room, action->room) || !target_matches(action, entrant))
    return;
  if (action->attack)
  {
    if (!event_runtime_handle_is_none(action->execution_handle) || !CAN_SEE(owner, entrant))
      return;
    action->attack_target = moved->character;
  }
  if (queue_ready_execution(action, owner))
    send_to_char(owner, "Your readied action triggers as %s enters.\r\n", GET_NAME(entrant));
}

static void ready_casting_handler(const struct domain_event_context *context, void *data)
{
  const struct domain_casting_started *event = context->payload;
  struct ready_action *action = data;
  struct char_data *owner = resolve_character(context->bus, action->owner);
  struct char_data *caster = resolve_character(context->bus, event->caster);

  if (owner == NULL || owner->ready_action != action || caster == NULL ||
      !domain_entity_handle_equal(action->attack_target, event->caster) ||
      !domain_entity_handle_equal(action->room, event->room) || !CAN_SEE(owner, caster) ||
      !event_runtime_handle_is_none(action->execution_handle))
    return;
  if (action->counterspell)
  {
    struct primary_activity_snapshot cast;

    if (!counterspell_observable(owner, caster) || !primary_activity_snapshot(caster, &cast) ||
        cast.type != PRIMARY_ACTIVITY_CASTING || cast.id != event->cast_id ||
        CASTING_SPELLNUM(caster) != event->spellnum)
      return;
    if (compute_ability(owner, ABILITY_SPELLCRAFT) + d20(owner) <= 20)
    {
      send_to_char(owner, "You cannot identify the spell in time to counter it.\r\n");
      cancel_action(action, false);
      return;
    }
    if (spell_prep_base_resource_check(owner, event->spellnum) == CLASS_UNDEFINED)
    {
      send_to_char(owner, "You have no matching spell resource available to counter it.\r\n");
      cancel_action(action, false);
      return;
    }
    action->counter_spellnum = event->spellnum;
  }
  action->cast_id = event->cast_id;
  if (queue_ready_execution(action, owner))
    send_to_char(owner, "Your readied %s triggers as %s begins casting.\r\n",
                 action->counterspell ? "counterspell" : "attack", GET_NAME(caster));
}

static bool ready_ally_matches(struct char_data *owner, struct char_data *ally)
{
  return owner != NULL && ally != NULL && owner != ally && !DEAD(ally) &&
         IN_ROOM(owner) == IN_ROOM(ally) && CAN_SEE(owner, ally) &&
         ((GROUP(owner) != NULL && GROUP(owner) == GROUP(ally)) ||
          (IS_NPC(ally) && ally->master == owner));
}

static void ready_ally_attacked(const struct domain_event_context *context, void *data)
{
  const struct domain_attack_committed *event = context->payload;
  struct ready_action *action = data;
  struct char_data *owner = resolve_character(context->bus, action->owner);
  struct char_data *ally;
  struct char_data *attacker;

  if (owner == NULL || owner->ready_action != action ||
      !event_runtime_handle_is_none(action->execution_handle) ||
      !domain_entity_handle_equal(action->protected_ally, event->defender) ||
      !domain_entity_handle_equal(action->room, event->origin_room))
    return;
  ally = resolve_character(context->bus, action->protected_ally);
  attacker = resolve_character(context->bus, event->attacker);
  if (!ready_ally_matches(owner, ally) || attacker == NULL || attacker == ally ||
      !combat_readied_attack_allowed(owner, attacker) || !pvp_ok(owner, attacker, false))
    return;

  /* Room-scoped watches were admitted before publication. Claim the attacker
   * by identity; no subscription mutation or combat occurs inside dispatch. */
  action->attack_target = event->attacker;
  if (queue_ready_execution(action, owner))
    send_to_char(owner, "Your readied strike triggers as %s attacks your ally.\r\n",
                 GET_NAME(attacker));
}

static void ready_target_lost(const struct domain_event_context *context, void *data)
{
  struct ready_action *action = data;

  if (context->type == DOMAIN_EVENT_CHARACTER_MOVED)
  {
    const struct domain_character_moved *moved = context->payload;
    if (!domain_entity_handle_equal(moved->character, action->attack_target) ||
        domain_entity_handle_equal(moved->from_room, moved->to_room))
      return;
  }
  else if (context->type == DOMAIN_EVENT_CHARACTER_DIED)
  {
    const struct domain_character_died *died = context->payload;
    if (!domain_entity_handle_equal(died->character, action->attack_target))
      return;
  }
  else if (context->type == DOMAIN_EVENT_ENTITY_EXTRACTED)
  {
    const struct domain_entity_extracted *extracted = context->payload;
    if (!domain_entity_handle_equal(extracted->entity, action->attack_target))
      return;
  }
  cancel_action(action, true);
}

static void ready_door_handler(const struct domain_event_context *context, void *handler_context)
{
  const struct domain_door_state_changed *changed = context->payload;
  struct ready_action *action = handler_context;
  struct char_data *owner = resolve_character(context->bus, action->owner);

  if (owner == NULL || owner->ready_action != action ||
      changed->direction != action->door_direction ||
      !domain_entity_handle_equal(changed->room, action->room))
    return;
  if (!ready_door_is_live(action, owner) || changed->cause == DOMAIN_DOOR_EDIT ||
      changed->exit_identity != action->exit_identity)
  {
    cancel_action(action, true);
    return;
  }
  /* Administrative changes cannot trigger a command, including a reset after
   * a gameplay open has queued one. A close cancels pending execution even if
   * another open arrives before the callback. */
  if (changed->cause != DOMAIN_DOOR_GAMEPLAY || (changed->current_state & EX_CLOSED) != 0)
  {
    if (!event_runtime_handle_is_none(action->execution_handle))
      cancel_action(action, true);
    return;
  }
  if ((changed->previous_state & EX_CLOSED) != 0 &&
      !EXIT_FLAGGED(EXIT(owner, action->door_direction), EX_CLOSED) &&
      queue_ready_execution(action, owner))
    send_to_char(owner, "Your readied action triggers as the door opens.\r\n");
}

static void ready_movement_handler(const struct domain_event_context *context,
                                   void *handler_context)
{
  const struct domain_character_moved *moved = context->payload;
  struct ready_action *action = handler_context;

  if (domain_entity_handle_equal(moved->character, action->owner) &&
      !domain_entity_handle_equal(moved->from_room, moved->to_room))
    cancel_action(action, true);
}

static void ready_death_handler(const struct domain_event_context *context, void *handler_context)
{
  const struct domain_character_died *died = context->payload;
  struct ready_action *action = handler_context;

  if (domain_entity_handle_equal(died->character, action->owner))
    cancel_action(action, false);
}

static void ready_combat_changed(const struct domain_event_context *context, void *data)
{
  const struct domain_combat_state_changed *event = context->payload;
  struct ready_action *action = data;

  if (event->in_combat && combat_encounter_semantic_rounds_enabled())
  {
    if (!event_runtime_handle_is_none(action->expiry_handle))
      (void)event_runtime_cancel(action->expiry_handle);
    action->expiry_handle = EVENT_RUNTIME_HANDLE_NONE;
  }
  else if (!event->in_combat && event_runtime_handle_is_none(action->expiry_handle) &&
           !schedule_ready_expiry(action))
    cancel_action(action, true);
}

static char *trimmed_copy(const char *start, size_t length, size_t maximum)
{
  char *copy;

  while (length > 0U && isspace((unsigned char)*start))
  {
    start++;
    length--;
  }
  while (length > 0U && isspace((unsigned char)start[length - 1U]))
    length--;
  if (length == 0U || length > maximum)
    return NULL;
  copy = malloc(length + 1U);
  if (copy == NULL)
    return NULL;
  memcpy(copy, start, length);
  copy[length] = '\0';
  return copy;
}

static const char *find_ready_clause(const char *argument, const char *clause)
{
  const char *found = NULL;
  const char *cursor;
  size_t clause_length = strlen(clause);

  for (cursor = argument; *cursor != '\0'; cursor++)
    if (!strncasecmp(cursor, clause, clause_length) &&
        (cursor[clause_length] == '\0' || isspace((unsigned char)cursor[clause_length])))
      found = cursor;
  return found;
}

static bool add_subscription(struct ready_action *action,
                             struct domain_event_subscription_handle *handle,
                             domain_event_type_id_t type, struct domain_event_topic topic,
                             const char *identity, domain_event_handler handler)
{
  struct domain_event_subscription_config config;

  memset(&config, 0, sizeof(config));
  config.type = type;
  config.topic = topic;
  config.owner = action->owner;
  config.identity = identity;
  config.handler = handler;
  config.handler_context = action;
  config.cleanup = ready_subscription_cleanup;
  if (domain_event_subscribe(domain_event_runtime_bus(), &config, handle) != DOMAIN_EVENT_OK)
    return false;
  action->references++;
  return true;
}

static bool watch_ready_target(struct ready_action *action)
{
  static const domain_event_type_id_t types[3] = {
      DOMAIN_EVENT_CHARACTER_MOVED, DOMAIN_EVENT_CHARACTER_DIED, DOMAIN_EVENT_ENTITY_EXTRACTED};
  static const char *identities[3] = {"ready.target.moved", "ready.target.died",
                                      "ready.target.extracted"};
  struct domain_event_topic topic = {DOMAIN_EVENT_TOPIC_SUBJECT, action->attack_target};
  size_t index;

  if (action->on_ally)
    topic = (struct domain_event_topic){DOMAIN_EVENT_TOPIC_SOURCE, action->room};
  for (index = 0U; index < 3U; index++)
    if (!add_subscription(action, &action->target_subscriptions[index], types[index], topic,
                          identities[index], ready_target_lost))
      return false;
  return true;
}

ACMD(do_ready)
{
  struct ready_action *action;
  struct domain_event_topic entry_topic;
  struct domain_event_topic owner_topic;
  const char *clause;
  const char *target_start;
  const char *argument_end;
  char first_word[MAX_INPUT_LENGTH];
  size_t clause_length = strlen(" on entry");
  const char *door_clause;
  const char *cast_clause;
  const char *ally_clause;
  char ally_name[MAX_INPUT_LENGTH];
  const char *attack_name;
  char attack_lookup[MAX_INPUT_LENGTH];
  struct char_data *victim = NULL;
  bool on_casting = false;
  bool on_ally = false;
  bool attack;
  bool counterspell;
  int direction = -1;
  char direction_name[MAX_INPUT_LENGTH];
  char extra[MAX_INPUT_LENGTH];

  (void)cmd;
  (void)subcmd;
  while (isspace((unsigned char)*argument))
    argument++;
  argument_end = argument + strlen(argument);
  while (argument_end > argument && isspace((unsigned char)argument_end[-1]))
    argument_end--;
  if (*argument == '\0')
  {
    if (ch->ready_action == NULL)
      send_to_char(ch, "You have no readied action.\r\n");
    else if (ch->ready_action->on_ally)
      send_to_char(ch, "Readied: attack on ally %s attacked\r\n", ch->ready_action->target);
    else if (ch->ready_action->on_casting)
      send_to_char(ch, "Readied: %s on casting\r\n", ch->ready_action->command);
    else if (ch->ready_action->door_direction >= 0)
      send_to_char(ch, "Readied: %s on door open %s\r\n", ch->ready_action->command,
                   dirs[ch->ready_action->door_direction]);
    else if (ch->ready_action->target != NULL)
      send_to_char(ch, "Readied: %s on entry %s\r\n", ch->ready_action->command,
                   ch->ready_action->target);
    else
      send_to_char(ch, "Readied: %s on entry\r\n", ch->ready_action->command);
    return;
  }
  if ((size_t)(argument_end - argument) == strlen("cancel") &&
      !strncasecmp(argument, "cancel", strlen("cancel")))
  {
    if (ch->ready_action == NULL)
      send_to_char(ch, "You have no readied action.\r\n");
    else
      ready_action_cancel(ch, true);
    return;
  }
  if (IS_NPC(ch) || IN_ROOM(ch) == NOWHERE || domain_event_runtime_bus() == NULL)
  {
    send_to_char(ch, "You cannot ready an action here.\r\n");
    return;
  }
  clause = find_ready_clause(argument, " on entry");
  door_clause = find_ready_clause(argument, " on door open");
  if (door_clause != NULL && (clause == NULL || door_clause > clause))
  {
    clause = door_clause;
    clause_length = strlen(" on door open");
    two_arguments((char *)clause + clause_length, direction_name, sizeof(direction_name), extra,
                  sizeof(extra));
    for (direction = 0; direction < DIR_COUNT; direction++)
      if (*direction_name != '\0' && is_abbrev(direction_name, dirs[direction]))
        break;
    if (direction >= DIR_COUNT || *extra != '\0' || EXIT(ch, direction) == NULL ||
        !EXIT_FLAGGED(EXIT(ch, direction), EX_ISDOOR) || EXIT(ch, direction)->to_room == NOWHERE ||
        EXIT(ch, direction)->to_room > top_of_world || is_exit_hidden(ch, direction) ||
        !EXIT_FLAGGED(EXIT(ch, direction), EX_CLOSED))
    {
      send_to_char(ch, "You must name a visible, closed door in this room.\r\n");
      return;
    }
  }
  cast_clause = find_ready_clause(argument, " on casting");
  if (cast_clause != NULL && (clause == NULL || cast_clause > clause))
  {
    clause = cast_clause;
    clause_length = strlen(" on casting");
    direction = -1;
    on_casting = true;
  }
  ally_clause = find_ready_clause(argument, " on ally");
  if (ally_clause != NULL && (clause == NULL || ally_clause > clause))
  {
    const char *tail;

    clause = ally_clause;
    clause_length = strlen(" on ally");
    direction = -1;
    on_casting = false;
    on_ally = true;
    tail = one_argument((char *)clause + clause_length, ally_name, sizeof(ally_name));
    tail = one_argument((char *)tail, extra, sizeof(extra));
    while (isspace((unsigned char)*tail))
      tail++;
    if (*ally_name == '\0' || strcasecmp(extra, "attacked") || *tail != '\0')
    {
      send_to_char(ch, "Usage: ready attack on ally <ally> attacked\r\n");
      return;
    }
  }
  if (clause == NULL)
  {
    send_to_char(ch, "Usage: ready <command> on entry [target]\r\n       ready <command> on door "
                     "open <direction>\r\n       ready attack <target> on casting\r\n       ready "
                     "counterspell <target> on casting\r\n");
    return;
  }
  action = calloc(1U, sizeof(*action));
  if (action == NULL)
  {
    send_to_char(ch, "You cannot ready that action right now.\r\n");
    return;
  }
  action->door_direction = direction;
  action->command = trimmed_copy(argument, (size_t)(clause - argument), READY_COMMAND_MAX);
  target_start = clause + clause_length;
  while (isspace((unsigned char)*target_start))
    target_start++;
  if (direction < 0 && *target_start != '\0')
    action->target = trimmed_copy(target_start, strlen(target_start), READY_TARGET_MAX);
  if (action->command == NULL ||
      (direction < 0 && *target_start != '\0' && action->target == NULL) ||
      strchr(action->command, '\n') != NULL || strchr(action->command, '\r') != NULL)
  {
    free(action->command);
    free(action->target);
    free(action);
    send_to_char(ch, "Usage: ready <command> on entry [target]\r\n       ready <command> on door "
                     "open <direction>\r\n       ready attack <target> on casting\r\n       ready "
                     "counterspell <target> on casting\r\n");
    return;
  }
  attack_name = one_argument(action->command, first_word, sizeof(first_word));
  attack = !strcasecmp(first_word, "attack") || !strcasecmp(first_word, "hit") ||
           !strcasecmp(first_word, "kill");
  counterspell = !strcasecmp(first_word, "counterspell");
  if (on_ally)
  {
    if (!attack || *attack_name != '\0')
      goto invalid_attack;
    attack_name = ally_name;
    free(action->target);
    action->target = trimmed_copy(ally_name, strlen(ally_name), READY_TARGET_MAX);
    if (action->target == NULL)
      goto invalid_attack;
  }
  if (counterspell && !on_casting)
    goto invalid_attack;
  /* Only explicit noncombat commands retain command readiness. Arbitrary
   * aliases, spells and special attacks cannot bypass action reservation. */
  if (!attack && !counterspell &&
      (on_casting || (strcasecmp(first_word, "say") && strcasecmp(first_word, "emote") &&
                      strcasecmp(first_word, "look") && strcasecmp(first_word, "rest") &&
                      strcasecmp(first_word, "stand") && strcasecmp(first_word, "sit") &&
                      strcasecmp(first_word, "open") && strcasecmp(first_word, "close"))))
  {
    free(action->command);
    free(action->target);
    free(action);
    send_to_char(ch, "Ready an attack, or say, emote, look, rest, stand, sit, open or close.\r\n");
    return;
  }
  if (attack || counterspell)
  {
    while (isspace((unsigned char)*attack_name))
      attack_name++;
    if (*attack_name == '\0' || (on_casting && action->target != NULL) ||
        !(counterspell ? counterspell_allowed(ch, NULL)
                       : combat_readied_attack_allowed(ch, NULL)) ||
        !is_action_available(ch, atSTANDARD, true))
      goto invalid_attack;
    if (on_casting || on_ally || direction >= 0)
    {
      strlcpy(attack_lookup, attack_name, sizeof(attack_lookup));
      victim = get_char_vis(ch, attack_lookup, NULL, FIND_CHAR_ROOM);
      if (victim == NULL || !(on_ally        ? ready_ally_matches(ch, victim)
                              : counterspell ? counterspell_allowed(ch, victim)
                                             : combat_readied_attack_allowed(ch, victim)))
        goto invalid_attack;
      action->attack_target = domain_event_character_handle(victim);
    }
    else
    {
      /* Entry targets may not yet exist locally. Bind the matching entrant's
       * stable handle at the event, never look up the name at execution. */
      if (action->target != NULL && strcasecmp(action->target, attack_name))
        goto invalid_attack;
      free(action->target);
      action->target = trimmed_copy(attack_name, strlen(attack_name), READY_TARGET_MAX);
      if (action->target == NULL)
        goto invalid_attack;
    }
  }
  action->attack = attack;
  action->counterspell = counterspell;
  action->on_casting = on_casting;
  action->on_ally = on_ally;
  if (on_ally)
    action->protected_ally = action->attack_target;
  if (ch->ready_action != NULL)
    ready_action_cancel(ch, false);
  action->owner = domain_event_character_handle(ch);
  action->room = domain_event_room_handle(IN_ROOM(ch));
  if (direction >= 0)
  {
    action->exit_identity = door_state_identity(IN_ROOM(ch), direction);
    action->destination = domain_event_room_handle(EXIT(ch, direction)->to_room);
    if (action->exit_identity == 0U)
    {
      free(action->command);
      free(action);
      send_to_char(ch, "You cannot ready that action right now.\r\n");
      return;
    }
  }
  ch->ready_action = action;
  entry_topic.role = direction >= 0 ? DOMAIN_EVENT_TOPIC_SUBJECT : DOMAIN_EVENT_TOPIC_DESTINATION;
  entry_topic.entity = action->room;
  if (on_casting || on_ally)
  {
    entry_topic.role = DOMAIN_EVENT_TOPIC_SUBJECT;
    entry_topic.entity = action->attack_target;
  }
  owner_topic.role = DOMAIN_EVENT_TOPIC_SUBJECT;
  owner_topic.entity = action->owner;
  if (!add_subscription(action, &action->entry_subscription,
                        on_ally          ? DOMAIN_EVENT_ATTACK_COMMITTED
                        : on_casting     ? DOMAIN_EVENT_CASTING_STARTED
                        : direction >= 0 ? DOMAIN_EVENT_DOOR_STATE_CHANGED
                                         : DOMAIN_EVENT_CHARACTER_MOVED,
                        entry_topic,
                        on_ally          ? "ready.ally-attacked"
                        : on_casting     ? "ready.casting"
                        : direction >= 0 ? "ready.door-open"
                                         : "ready.entry",
                        on_ally          ? ready_ally_attacked
                        : on_casting     ? ready_casting_handler
                        : direction >= 0 ? ready_door_handler
                                         : ready_entry_handler) ||
      !add_subscription(action, &action->movement_subscription, DOMAIN_EVENT_CHARACTER_MOVED,
                        owner_topic, "ready.owner-moved", ready_movement_handler) ||
      !add_subscription(action, &action->death_subscription, DOMAIN_EVENT_CHARACTER_DIED,
                        owner_topic, "ready.owner-died", ready_death_handler))
  {
    if (action->references == 0U)
    {
      ch->ready_action = NULL;
      free(action->command);
      free(action->target);
      free(action);
    }
    else
      cancel_action(action, false);
    send_to_char(ch, "You cannot ready that action right now.\r\n");
    return;
  }
  if (attack || counterspell)
  {
    struct domain_event_topic departure_topic = {DOMAIN_EVENT_TOPIC_SOURCE, action->room};

    if ((victim == NULL &&
         !add_subscription(action, &action->target_subscriptions[0], DOMAIN_EVENT_CHARACTER_MOVED,
                           departure_topic, "ready.entry.target-left", ready_target_lost)) ||
        (victim != NULL && !watch_ready_target(action)) ||
        !add_subscription(action, &action->combat_subscription, DOMAIN_EVENT_COMBAT_STATE_CHANGED,
                          owner_topic, "ready.owner.combat", ready_combat_changed) ||
        (!combat_encounter_semantic_manages(ch) && !schedule_ready_expiry(action)))
    {
      cancel_action(action, true);
      return;
    }
    USE_STANDARD_ACTION(ch);
    send_to_char(ch, "You spend a standard action to ready one %s.\r\n",
                 counterspell ? "counterspell" : "attack");
  }
  if (on_ally)
    send_to_char(ch, "You ready one strike against the first eligible attacker of %s.\r\n",
                 action->target);
  else if (on_casting)
    send_to_char(ch, "You ready '%s' for when that target begins casting.\r\n", action->command);
  else if (direction >= 0)
    send_to_char(ch, "You ready '%s' for when the door %s opens.\r\n", action->command,
                 dirs[direction]);
  else if (action->target != NULL)
    send_to_char(ch, "You ready '%s' for when %s enters.\r\n", action->command, action->target);
  else
    send_to_char(ch, "You ready '%s' for the next arrival.\r\n", action->command);
  return;

invalid_attack:
  free(action->command);
  free(action->target);
  free(action);
  send_to_char(
      ch, "Ready attack <target> on casting, on entry, or on door open <direction>.\r\n"
          "Or ready counterspell <target> on casting, or ready attack on ally <ally> attacked.\r\n"
          "Casting and door triggers require an eligible, visible target here.\r\n");
}
