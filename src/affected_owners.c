#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "dotenv.h"
#include "handler.h"
#include "affected_owners.h"
#include "dgscript/dg_event.h"
#include "magic/spells.h"
#include "mudlim.h"

#define AFFECTED_CHARACTER_MAX_OWNERS 32768U
#define AFFECTED_ROOM_MAX_OWNERS 16384U
#define AFFECTED_REJECTION_LOG_INTERVAL 100U

static bool initialized;
static bool scheduled;
static bool shutting_down;
static bool refilling;
static struct room_data *affected_room_list;
static size_t character_scheduled_count;
static size_t room_owner_count;
static size_t room_scheduled_count;
static size_t character_limit = AFFECTED_CHARACTER_MAX_OWNERS;
static size_t room_limit = AFFECTED_ROOM_MAX_OWNERS;
static uint64_t admission_rejections;
static uint64_t character_callback_count;
static uint64_t room_callback_count;
static uint64_t character_nodes_processed;
static uint64_t room_nodes_processed;
static uint64_t room_behavior_executions;
static uint64_t room_behavior_nodes_processed;
static uint64_t next_generation = 1U;
#ifdef LUMINARI_CUTEST
static bool test_selection_set;
static bool test_scheduled_selection;
#endif

static void affected_room_schedule(struct room_data *room);

static bool configured_scheduled(void)
{
  const char *value;

  value = getenv("LUMINARI_AFFECT_EVENTS");
  if (value == NULL || *value == '\0')
    value = get_env_value("LUMINARI_AFFECT_EVENTS");
  if (value == NULL || *value == '\0' || !strcasecmp(value, "scheduled") ||
      !strcasecmp(value, "active") || !strcasecmp(value, "event"))
    return true;
  if (!strcasecmp(value, "legacy") || !strcasecmp(value, "heartbeat") ||
      !strcasecmp(value, "off"))
    return false;
  log("WARNING: Unknown LUMINARI_AFFECT_EVENTS '%s'; using scheduled owner events.", value);
  return true;
}

static uint64_t ensure_generation(uint64_t *generation)
{
  if (generation == NULL || *generation != 0U)
    return generation != NULL ? *generation : 0U;
  if (next_generation == 0U)
    return 0U;
  *generation = next_generation;
  if (next_generation == UINT64_MAX)
    next_generation = 0U;
  else
    next_generation++;
  return *generation;
}

static struct game_event_owner character_owner(struct char_data *ch)
{
  struct game_event_owner owner = game_event_owner_none();

  if (ch == NULL)
    return owner;
  owner.kind = GAME_EVENT_OWNER_CHARACTER;
  owner.runtime_id = (uint64_t)(uintptr_t)ch;
  owner.generation = ensure_generation(&ch->periodic_event_generation);
  return owner;
}

static struct game_event_owner room_owner(struct room_data *room)
{
  struct game_event_owner owner = game_event_owner_none();

  if (room == NULL)
    return owner;
  owner.kind = GAME_EVENT_OWNER_ROOM;
  owner.runtime_id = (uint64_t)(uint32_t)room->number + 1U;
  owner.generation = ensure_generation(&room->periodic_event_generation);
  return owner;
}

static long next_round_delay(void)
{
  unsigned long remainder = pulse % PULSE_VIOLENCE;

  return remainder == 0U ? PULSE_VIOLENCE : (long)(PULSE_VIOLENCE - remainder);
}

static long next_room_delay(void)
{
  unsigned long luminari_remainder = pulse % PULSE_LUMINARI;
  long luminari_delay = luminari_remainder == 0U
                            ? PULSE_LUMINARI
                            : (long)(PULSE_LUMINARI - luminari_remainder);
  long round_delay = next_round_delay();

  return luminari_delay < round_delay ? luminari_delay : round_delay;
}

static void affected_character_cleanup(event_handle_t handle, void *event_obj)
{
  struct char_data *ch = event_obj;

  if (ch == NULL || ch->affected_event_handle != handle)
    return;
  ch->affected_event_handle = EVENT_HANDLE_NONE;
  if (character_scheduled_count > 0U)
    character_scheduled_count--;
}

static void affected_room_cleanup(event_handle_t handle, void *event_obj)
{
  struct room_data *room = event_obj;

  if (room == NULL || room->affected_event_handle != handle)
    return;
  room->affected_event_handle = EVENT_HANDLE_NONE;
  if (room_scheduled_count > 0U)
    room_scheduled_count--;
}

static void note_rejection(const char *kind, size_t limit)
{
  admission_rejections++;
  if (admission_rejections == 1U ||
      admission_rejections % AFFECTED_REJECTION_LOG_INTERVAL == 0U)
    log("WARNING: affected %s owner limit reached (%zu); rejected=%llu.", kind, limit,
        (unsigned long long)admission_rejections);
}

void affected_character_owner_refill(void)
{
  struct char_data *ch;

  if (!initialized || !scheduled || shutting_down || refilling ||
      character_scheduled_count >= character_limit)
    return;
  if (affected_registry_iteration_in_progress())
    return;
  refilling = true;
  for (ch = affected_registry_iteration_begin(); ch != NULL &&
                                                  character_scheduled_count < character_limit;
       ch = affected_registry_iteration_next())
    affected_character_owner_sync(ch);
  affected_registry_iteration_end();
  refilling = false;
}

static void refill_room_capacity(void)
{
  struct room_data *room;

  if (!initialized || !scheduled || shutting_down || refilling ||
      room_scheduled_count >= room_limit)
    return;
  refilling = true;
  for (room = affected_room_list;
       room != NULL && room_scheduled_count < room_limit; room = room->affected_next)
    affected_room_schedule(room);
  refilling = false;
}

static EVENTFUNC(affected_character_event)
{
  struct char_data *ch = event_obj;

  if (ch == NULL)
    return 0;
  character_callback_count++;
  if (!ch->affected_registered || !ch->affected_registry_live || ch->affected == NULL)
  {
    ch->affected_event_handle = EVENT_HANDLE_NONE;
    if (character_scheduled_count > 0U)
      character_scheduled_count--;
    affected_character_owner_refill();
    return 0;
  }
  character_nodes_processed += affect_update_character_one(ch);
  return PULSE_VIOLENCE;
}

static EVENTFUNC(affected_room_event)
{
  struct room_data *room = event_obj;

  if (room == NULL)
    return 0;
  room_callback_count++;
  if (!room->affected_registered || room->affected_head == NULL)
  {
    room->affected_event_handle = EVENT_HANDLE_NONE;
    if (room_scheduled_count > 0U)
      room_scheduled_count--;
    refill_room_capacity();
    return 0;
  }
  if (pulse % PULSE_VIOLENCE == 0U)
  {
    room_nodes_processed += affect_update_room_one(room);
    if (!room->affected_registered || room->affected_head == NULL ||
        room->affected_event_handle == EVENT_HANDLE_NONE)
    {
      refill_room_capacity();
      return 0;
    }
  }
  if (pulse % PULSE_LUMINARI == 0U)
  {
    room_behavior_executions++;
    room_behavior_nodes_processed += process_room_affect_activity(room);
    if (!room->affected_registered || room->affected_head == NULL ||
        room->affected_event_handle == EVENT_HANDLE_NONE)
    {
      refill_room_capacity();
      return 0;
    }
  }
  return next_room_delay();
}

void affected_character_owner_sync(struct char_data *ch)
{
  struct game_event_owner owner;

  if (!initialized || !scheduled || ch == NULL || !ch->affected_registered ||
      !ch->affected_registry_live || ch->affected == NULL ||
      ch->affected_event_handle != EVENT_HANDLE_NONE)
    return;
  if (character_scheduled_count >= character_limit)
  {
    note_rejection("character", character_limit);
    return;
  }
  owner = character_owner(ch);
  if (!game_event_owner_is_valid(owner))
    return;
  ch->affected_event_handle = event_schedule_owned_named_with_cleanup(
      affected_character_event, ch, next_round_delay(), "affected_character",
      affected_character_cleanup, owner);
  if (ch->affected_event_handle == EVENT_HANDLE_NONE)
  {
    note_rejection("character", character_limit);
    return;
  }
  character_scheduled_count++;
}

void affected_character_owner_forget(struct char_data *ch)
{
  event_handle_t handle;

  if (ch == NULL || ch->affected_event_handle == EVENT_HANDLE_NONE)
    return;
  handle = ch->affected_event_handle;
  ch->affected_event_handle = EVENT_HANDLE_NONE;
  if (character_scheduled_count > 0U)
    character_scheduled_count--;
  (void)event_handle_cancel(handle);
  affected_character_owner_refill();
}

static void affected_room_schedule(struct room_data *room)
{
  struct game_event_owner owner;

  if (!initialized || !scheduled || room == NULL || !room->affected_registered ||
      room->affected_head == NULL || room->affected_event_handle != EVENT_HANDLE_NONE)
    return;
  if (room_scheduled_count >= room_limit)
  {
    note_rejection("room", room_limit);
    return;
  }
  owner = room_owner(room);
  if (!game_event_owner_is_valid(owner))
    return;
  room->affected_event_handle = event_schedule_owned_named_with_cleanup(
      affected_room_event, room, next_room_delay(), "affected_room", affected_room_cleanup,
      owner);
  if (room->affected_event_handle == EVENT_HANDLE_NONE)
  {
    note_rejection("room", room_limit);
    return;
  }
  room_scheduled_count++;
}

static void affected_room_forget(struct room_data *room)
{
  event_handle_t handle;

  if (room == NULL || room->affected_event_handle == EVENT_HANDLE_NONE)
    return;
  handle = room->affected_event_handle;
  room->affected_event_handle = EVENT_HANDLE_NONE;
  if (room_scheduled_count > 0U)
    room_scheduled_count--;
  (void)event_handle_cancel(handle);
  refill_room_capacity();
}

void affected_room_owner_add(struct raff_node *raff)
{
  struct room_data *room;

  if (raff == NULL || world == NULL || raff->room_registered || raff->room == NOWHERE ||
      raff->room > top_of_world)
    return;
  room = &world[raff->room];
  raff->room_prev = NULL;
  raff->room_next = room->affected_head;
  if (raff->room_next != NULL)
    raff->room_next->room_prev = raff;
  room->affected_head = raff;
  raff->room_registered = true;
  room->affected_count++;
  if (!room->affected_registered)
  {
    room->affected_prev = NULL;
    room->affected_next = affected_room_list;
    if (affected_room_list != NULL)
      affected_room_list->affected_prev = room;
    affected_room_list = room;
    room->affected_registered = true;
    room_owner_count++;
  }
  affected_room_schedule(room);
}

void affected_room_owner_remove(struct raff_node *raff)
{
  struct room_data *room;

  if (raff == NULL || !raff->room_registered || raff->room == NOWHERE ||
      raff->room > top_of_world)
    return;
  room = &world[raff->room];
  if (raff->room_prev != NULL)
    raff->room_prev->room_next = raff->room_next;
  else if (room->affected_head == raff)
    room->affected_head = raff->room_next;
  if (raff->room_next != NULL)
    raff->room_next->room_prev = raff->room_prev;
  raff->room_next = NULL;
  raff->room_prev = NULL;
  raff->room_registered = false;
  if (room->affected_count > 0U)
    room->affected_count--;
  if (room->affected_head != NULL)
    return;

  affected_room_forget(room);
  if (room->affected_prev != NULL)
    room->affected_prev->affected_next = room->affected_next;
  else if (affected_room_list == room)
    affected_room_list = room->affected_next;
  if (room->affected_next != NULL)
    room->affected_next->affected_prev = room->affected_prev;
  room->affected_next = NULL;
  room->affected_prev = NULL;
  room->affected_registered = false;
  if (room_owner_count > 0U)
    room_owner_count--;
}

void affected_room_owners_remove_room(uint32_t room_index)
{
  struct raff_node *raff;
  struct raff_node *next;

  for (raff = raff_list; raff != NULL; raff = next)
  {
    next = raff->next;
    if (raff->room == (room_rnum)room_index)
      rem_room_aff(raff);
  }
}

void affected_room_owners_prepare_world_reindex(void)
{
  struct room_data *room;
  struct room_data *next_room;
  struct raff_node *raff;
  struct raff_node *next_raff;
  bool was_refilling = refilling;

  refilling = true;
  for (room = affected_room_list; room != NULL; room = next_room)
  {
    next_room = room->affected_next;
    if (room->affected_event_handle != EVENT_HANDLE_NONE)
    {
      event_handle_t handle = room->affected_event_handle;

      room->affected_event_handle = EVENT_HANDLE_NONE;
      (void)event_handle_cancel(handle);
    }
    for (raff = room->affected_head; raff != NULL; raff = next_raff)
    {
      next_raff = raff->room_next;
      raff->room_next = NULL;
      raff->room_prev = NULL;
      raff->room_registered = false;
    }
    room->affected_head = NULL;
    room->affected_next = NULL;
    room->affected_prev = NULL;
    room->affected_count = 0U;
    room->affected_registered = false;
  }
  affected_room_list = NULL;
  room_owner_count = 0U;
  room_scheduled_count = 0U;
  for (raff = raff_list; raff != NULL; raff = raff->next)
  {
    raff->room_next = NULL;
    raff->room_prev = NULL;
    raff->room_registered = false;
  }
  refilling = was_refilling;
}

void affected_room_owners_finish_world_reindex(uint32_t pivot, bool inserted)
{
  struct raff_node *raff;

  for (raff = raff_list; raff != NULL; raff = raff->next)
  {
    if (raff->room == NOWHERE)
      continue;
    if (inserted && raff->room >= (room_rnum)pivot)
      raff->room++;
    else if (!inserted && raff->room > (room_rnum)pivot)
      raff->room--;
    else if (!inserted && raff->room == (room_rnum)pivot)
    {
      log("SYSERR: affected room owner survived deletion at room index %u.", pivot);
      continue;
    }
    affected_room_owner_add(raff);
  }
}

void affected_owners_init(void)
{
  struct char_data *ch;
  struct room_data *room;

  if (initialized)
    return;
#ifdef LUMINARI_CUTEST
  scheduled = test_selection_set ? test_scheduled_selection : configured_scheduled();
#else
  scheduled = configured_scheduled();
#endif
  initialized = true;
  shutting_down = false;
  log("Affected-owner scheduling: %s (character limit %zu, room limit %zu).",
      scheduled ? "scheduled" : "legacy heartbeat", character_limit, room_limit);
  if (!scheduled)
    return;
  for (ch = affected_registry_iteration_begin(); ch != NULL;
       ch = affected_registry_iteration_next())
    affected_character_owner_sync(ch);
  affected_registry_iteration_end();
  for (room = affected_room_list; room != NULL; room = room->affected_next)
    affected_room_schedule(room);
}

void affected_owners_shutdown(void)
{
  struct char_data *ch;
  struct room_data *room;

  if (!initialized)
    return;
  shutting_down = true;
  for (ch = affected_registry_iteration_begin(); ch != NULL;
       ch = affected_registry_iteration_next())
    affected_character_owner_forget(ch);
  affected_registry_iteration_end();
  for (room = affected_room_list; room != NULL; room = room->affected_next)
    affected_room_forget(room);
  character_scheduled_count = 0U;
  room_scheduled_count = 0U;
  initialized = false;
  scheduled = false;
  shutting_down = false;
  refilling = false;
}

bool affected_owner_events_enabled(void)
{
  return initialized && scheduled;
}

size_t affected_character_scheduled_count(void) { return character_scheduled_count; }
size_t affected_room_owner_count(void) { return room_owner_count; }
size_t affected_room_scheduled_count(void) { return room_scheduled_count; }
size_t affected_character_admission_limit(void) { return character_limit; }
size_t affected_room_admission_limit(void) { return room_limit; }
uint64_t affected_owner_admission_rejections(void) { return admission_rejections; }
uint64_t affected_character_callbacks(void) { return character_callback_count; }
uint64_t affected_room_callbacks(void) { return room_callback_count; }
uint64_t affected_character_nodes_processed(void) { return character_nodes_processed; }
uint64_t affected_room_nodes_processed(void) { return room_nodes_processed; }
uint64_t affected_room_behavior_executions(void) { return room_behavior_executions; }
uint64_t affected_room_behavior_nodes_processed(void) { return room_behavior_nodes_processed; }

static bool room_is_in_current_world(const struct room_data *room)
{
  uintptr_t address;
  uintptr_t first;
  uintptr_t last;

  if (room == NULL || world == NULL)
    return false;
  address = (uintptr_t)room;
  first = (uintptr_t)&world[0];
  last = (uintptr_t)&world[top_of_world];
  return address >= first && address <= last && (address - first) % sizeof(*world) == 0U;
}

size_t affected_room_registry_validate(void)
{
  struct room_data *room;
  struct room_data *previous_room = NULL;
  struct raff_node *raff;
  struct raff_node *previous_raff;
  size_t list_rooms = 0U;
  size_t list_nodes = 0U;
  size_t list_scheduled = 0U;
  size_t room_nodes;
  size_t world_rooms = 0U;
  size_t world_nodes = 0U;
  room_rnum index;

  for (room = affected_room_list; room != NULL; room = room->affected_next)
  {
    if (!room_is_in_current_world(room) || !room->affected_registered ||
        room->affected_prev != previous_room || room->affected_head == NULL)
      return room_owner_count + 1U;
    list_rooms++;
    room_nodes = 0U;
    previous_raff = NULL;
    for (raff = room->affected_head; raff != NULL; raff = raff->room_next)
    {
      if (!raff->room_registered || raff->room == NOWHERE || raff->room > top_of_world ||
          &world[raff->room] != room || raff->room_prev != previous_raff)
        return list_rooms + list_nodes + 1U;
      previous_raff = raff;
      room_nodes++;
      list_nodes++;
      if (room_nodes > room->affected_count)
        return list_rooms + list_nodes + 1U;
    }
    if (room_nodes != room->affected_count)
      return list_rooms + list_nodes + 1U;
    if (room->affected_event_handle != EVENT_HANDLE_NONE)
      list_scheduled++;
    previous_room = room;
  }
  if (world != NULL)
    for (index = 0; index <= top_of_world; index++)
      if (world[index].affected_head != NULL || world[index].affected_registered ||
          world[index].affected_count != 0U ||
          world[index].affected_event_handle != EVENT_HANDLE_NONE)
      {
        world_rooms++;
        world_nodes += world[index].affected_count;
      }
  if (list_rooms == world_rooms && list_rooms == room_owner_count && list_nodes == world_nodes &&
      list_scheduled == room_scheduled_count)
    return 0U;
  return list_rooms + world_rooms + list_nodes + world_nodes + list_scheduled +
         room_scheduled_count;
}

void affected_owners_reset_telemetry(void)
{
  admission_rejections = 0U;
  character_callback_count = 0U;
  room_callback_count = 0U;
  character_nodes_processed = 0U;
  room_nodes_processed = 0U;
  room_behavior_executions = 0U;
  room_behavior_nodes_processed = 0U;
}

#ifdef LUMINARI_CUTEST
void affected_owners_reset_for_test(void)
{
  struct room_data *room;
  struct room_data *next;
  struct raff_node *raff;
  struct raff_node *next_raff;

  affected_owners_shutdown();
  affected_owners_reset_telemetry();
  for (room = affected_room_list; room != NULL; room = next)
  {
    next = room->affected_next;
    for (raff = room->affected_head; raff != NULL; raff = next_raff)
    {
      next_raff = raff->room_next;
      raff->room_next = NULL;
      raff->room_prev = NULL;
      raff->room_registered = false;
    }
    room->affected_head = NULL;
    room->affected_next = NULL;
    room->affected_prev = NULL;
    room->affected_count = 0U;
    room->affected_registered = false;
    room->affected_event_handle = EVENT_HANDLE_NONE;
  }
  affected_room_list = NULL;
  room_owner_count = 0U;
  character_limit = AFFECTED_CHARACTER_MAX_OWNERS;
  room_limit = AFFECTED_ROOM_MAX_OWNERS;
  test_selection_set = false;
  test_scheduled_selection = false;
}

void affected_owners_select_for_test(bool use_scheduled)
{
  test_selection_set = true;
  test_scheduled_selection = use_scheduled;
}

void affected_owners_set_limits_for_test(size_t new_character_limit, size_t new_room_limit)
{
  character_limit = new_character_limit;
  room_limit = new_room_limit;
}
#endif
