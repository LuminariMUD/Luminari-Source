#include "conf.h"
#include "sysdep.h"

#include "ready_action.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "domain_event_runtime.h"
#include "domain_event_types.h"
#include "domain_event_world.h"
#include "event_runtime.h"
#include "handler.h"
#include "interpreter.h"

#define READY_COMMAND_MAX (MAX_INPUT_LENGTH - 1)
#define READY_TARGET_MAX 80

struct ready_action
{
  struct domain_entity_handle owner;
  struct domain_entity_handle room;
  struct domain_event_subscription_handle entry_subscription;
  struct domain_event_subscription_handle movement_subscription;
  struct domain_event_subscription_handle death_subscription;
  struct event_runtime_handle execution_handle;
  unsigned int references;
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
  free(action->command);
  free(action->target);
  free(action);
}

static void cancel_action(struct ready_action *action, bool notify)
{
  struct domain_event_subscription_handle handles[3];
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
  handles[0] = action->entry_subscription;
  handles[1] = action->movement_subscription;
  handles[2] = action->death_subscription;
  action->entry_subscription = domain_event_subscription_handle_none();
  action->movement_subscription = domain_event_subscription_handle_none();
  action->death_subscription = domain_event_subscription_handle_none();
  for (index = 0U; index < 3U; index++)
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

static struct game_event_result ready_execution_callback(const struct game_event_context *context)
{
  struct ready_execution *execution = context != NULL ? context->payload : NULL;
  struct domain_event_bus *bus = domain_event_runtime_bus();
  struct char_data *ch;

  if (execution == NULL || bus == NULL)
    return game_event_result_complete();
  ch = resolve_character(bus, execution->owner);
  if (ch == NULL || ch->ready_action == NULL)
    return game_event_result_complete();
  ch->ready_action->execution_handle = EVENT_RUNTIME_HANDLE_NONE;
  cancel_action(ch->ready_action, false);
  if (ch == NULL || GET_POS(ch) <= POS_DEAD || IN_ROOM(ch) == NOWHERE ||
      !domain_entity_handle_equal(domain_event_room_handle(IN_ROOM(ch)), execution->room))
    return game_event_result_complete();
  command_interpreter(ch, execution->command);
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
  return true;
}

void ready_action_runtime_shutdown(void)
{
  ready_execution_event_type = 0U;
}

static bool target_matches(const struct ready_action *action, struct char_data *entrant)
{
  if (action->target == NULL || *action->target == '\0')
    return true;
  if (entrant == NULL || entrant->player.name == NULL)
    return false;
  return isname(action->target, entrant->player.name) != 0;
}

static void ready_entry_handler(const struct domain_event_context *context, void *handler_context)
{
  const struct domain_character_moved *moved = context->payload;
  struct ready_action *action = handler_context;
  struct ready_execution *execution;
  struct char_data *owner;
  struct char_data *entrant;
  struct game_event_owner event_owner;
  struct event_runtime_handle event_handle;

  owner = resolve_character(context->bus, action->owner);
  entrant = resolve_character(context->bus, moved->character);
  if (!event_runtime_handle_is_none(action->execution_handle))
    return;
  if (owner == NULL || owner->ready_action != action || entrant == NULL || entrant == owner ||
      !domain_entity_handle_equal(moved->to_room, action->room) || !target_matches(action, entrant))
    return;
  execution = calloc(1U, sizeof(*execution));
  if (execution == NULL)
  {
    send_to_char(owner, "You cannot hold that readied action.\r\n");
    cancel_action(action, false);
    return;
  }
  execution->owner = action->owner;
  execution->room = action->room;
  snprintf(execution->command, sizeof(execution->command), "%s", action->command);
  event_owner = ready_owner(action->owner);
  event_handle = EVENT_RUNTIME_HANDLE_NONE;
  if (event_runtime_schedule_owned_after(ready_execution_event_type, event_owner, 1U, execution,
                                         &event_handle) != GAME_SCHEDULER_OK)
  {
    free(execution);
    send_to_char(owner, "Your readied action could not be triggered.\r\n");
    cancel_action(action, false);
    return;
  }
  send_to_char(owner, "Your readied action triggers as %s enters.\r\n", GET_NAME(entrant));
  action->execution_handle = event_handle;
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

static const char *find_entry_clause(const char *argument)
{
  static const char clause[] = " on entry";
  const char *found = NULL;
  const char *cursor;
  size_t clause_length = sizeof(clause) - 1U;

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
  clause = find_entry_clause(argument);
  if (clause == NULL)
  {
    send_to_char(ch, "Usage: ready <command> on entry [target]\r\n");
    return;
  }
  action = calloc(1U, sizeof(*action));
  if (action == NULL)
  {
    send_to_char(ch, "You cannot ready that action right now.\r\n");
    return;
  }
  action->command = trimmed_copy(argument, (size_t)(clause - argument), READY_COMMAND_MAX);
  target_start = clause + clause_length;
  while (isspace((unsigned char)*target_start))
    target_start++;
  if (*target_start != '\0')
    action->target = trimmed_copy(target_start, strlen(target_start), READY_TARGET_MAX);
  if (action->command == NULL || (*target_start != '\0' && action->target == NULL) ||
      strchr(action->command, '\n') != NULL || strchr(action->command, '\r') != NULL)
  {
    free(action->command);
    free(action->target);
    free(action);
    send_to_char(ch, "Usage: ready <command> on entry [target]\r\n");
    return;
  }
  one_argument(action->command, first_word, sizeof(first_word));
  if (!strcasecmp(first_word, "ready"))
  {
    free(action->command);
    free(action->target);
    free(action);
    send_to_char(ch, "You cannot ready the ready command.\r\n");
    return;
  }
  if (ch->ready_action != NULL)
    ready_action_cancel(ch, false);
  action->owner = domain_event_character_handle(ch);
  action->room = domain_event_room_handle(IN_ROOM(ch));
  ch->ready_action = action;
  entry_topic.role = DOMAIN_EVENT_TOPIC_DESTINATION;
  entry_topic.entity = action->room;
  owner_topic.role = DOMAIN_EVENT_TOPIC_SUBJECT;
  owner_topic.entity = action->owner;
  if (!add_subscription(action, &action->entry_subscription, DOMAIN_EVENT_CHARACTER_MOVED,
                        entry_topic, "ready.entry", ready_entry_handler) ||
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
  if (action->target != NULL)
    send_to_char(ch, "You ready '%s' for when %s enters.\r\n", action->command, action->target);
  else
    send_to_char(ch, "You ready '%s' for the next arrival.\r\n", action->command);
}
