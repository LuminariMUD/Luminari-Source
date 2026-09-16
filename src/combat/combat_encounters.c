#include "conf.h"
#include "events/ready_action.h"
#include "core/sysdep.h"
#include "core/structs.h"
#include "core/utils.h"
#include "tactical_effects.h"
#include "core/comm.h"
#include "config/dotenv.h"
#include "events/activity_manager.h"
#include "combat/combat_encounters.h"
#include "combat/fight.h"
#include "dgscript/dg_event.h"
#include "events/domain_event_runtime.h"
#include "events/domain_event_types.h"
#include "events/domain_event_world.h"
#include "events/event_runtime.h"

#define COMBAT_ENCOUNTER_MAX_ACTIVE 32768U
#define COMBAT_ENCOUNTER_PHASE_DELAY ((uint64_t)(2 RL_SEC))
#define COMBAT_ENCOUNTER_ROUND_DELAY ((uint64_t)(6 RL_SEC))

struct combat_encounter_participant
{
  struct combat_encounter_data *encounter;
  struct char_data *character;
  struct domain_entity_handle character_handle;
  struct combat_encounter_participant *previous;
  struct combat_encounter_participant *next;
  struct combat_encounter_participant *due_previous;
  struct combat_encounter_participant *due_next;
  uint64_t next_due;
  uint64_t next_turn_due;
  uint64_t due_sequence;
  int initiative;
  unsigned int phase;
  bool active;
  bool pending_add;
  bool pending_activation;
  bool in_due_list;
  bool dispatching;
  bool departing;
};

struct combat_encounter_data
{
  uint64_t id;
  uint64_t generation;
  struct event_runtime_handle event_handle;
  struct combat_encounter_participant *participants;
  struct combat_encounter_participant *participants_tail;
  struct combat_encounter_participant *pending_additions;
  struct combat_encounter_participant *pending_additions_tail;
  struct combat_encounter_participant *due_head;
  struct combat_encounter_participant *due_tail;
  struct combat_encounter_data *registry_previous;
  struct combat_encounter_data *registry_next;
  struct combat_encounter_data *pending_merge_head;
  struct combat_encounter_data *pending_merge_tail;
  struct combat_encounter_data *pending_merge_next;
  struct combat_encounter_data *pending_into;
  uint64_t semantic_round;
  uint64_t next_round_due;
  bool resolving;
  bool terminal;
};

struct combat_encounter_slot
{
  struct combat_encounter_data *encounter;
  uint64_t generation;
  uint32_t next_free;
};

struct combat_encounter_event_payload
{
  uint64_t id;
  uint64_t generation;
};

static struct combat_encounter_slot encounter_slots[COMBAT_ENCOUNTER_MAX_ACTIVE];
static uint32_t free_slot_head;
static struct combat_encounter_data *encounter_registry;
static bool initialized;
static bool encounter_mode;
static bool semantic_rounds;
static bool shutting_down;
static size_t active_encounter_count;
static size_t active_participant_count;
static size_t scheduled_event_count;
static uint64_t next_due_sequence = 1U;
static struct combat_encounter_stats cumulative_stats;
static struct domain_event_bus *encounter_bus;
static game_event_type_id_t encounter_round_event_type;

#ifdef LUMINARI_CUTEST
static bool test_selection_set;
static bool test_encounter_mode;
static bool test_semantic_selection_set;
static bool test_semantic_rounds;
static combat_encounter_test_phase_callback test_phase_callback;
static void *test_phase_context;
#endif

static bool configured_encounter_mode(void)
{
#ifdef LUMINARI_CUTEST
  if (test_selection_set)
    return test_encounter_mode;
#endif
  return true;
}

static bool configured_semantic_rounds(void)
{
#ifdef LUMINARI_CUTEST
  if (test_semantic_selection_set)
    return test_semantic_rounds;
#endif
  return true;
}

static void counter_increment(uint64_t *counter)
{
  if (counter != NULL && *counter < UINT64_MAX)
    (*counter)++;
}

static uint64_t allocate_due_sequence(void)
{
  uint64_t sequence = next_due_sequence;

  if (next_due_sequence == UINT64_MAX)
    next_due_sequence = 1U;
  else
    next_due_sequence++;
  return sequence;
}

static struct combat_encounter_data *resolve_encounter(uint64_t id, uint64_t generation)
{
  struct combat_encounter_slot *slot;

  if (id == 0U || id > COMBAT_ENCOUNTER_MAX_ACTIVE)
    return NULL;
  slot = &encounter_slots[id - 1U];
  if (slot->encounter == NULL || slot->generation != generation)
    return NULL;
  return slot->encounter;
}

static bool allocate_encounter_slot(struct combat_encounter_data *encounter)
{
  struct combat_encounter_slot *slot;
  uint32_t index;

  if (encounter == NULL || free_slot_head == UINT32_MAX)
    return false;
  index = free_slot_head;
  slot = &encounter_slots[index];
  free_slot_head = slot->next_free;
  if (slot->generation == 0U)
    slot->generation = 1U;
  slot->encounter = encounter;
  slot->next_free = UINT32_MAX;
  encounter->id = (uint64_t)index + 1U;
  encounter->generation = slot->generation;
  return true;
}

static void release_encounter_slot(struct combat_encounter_data *encounter)
{
  struct combat_encounter_slot *slot;
  uint32_t index;

  if (encounter == NULL || encounter->id == 0U || encounter->id > COMBAT_ENCOUNTER_MAX_ACTIVE)
    return;
  index = (uint32_t)(encounter->id - 1U);
  slot = &encounter_slots[index];
  if (slot->encounter != encounter)
    return;
  slot->encounter = NULL;
  if (slot->generation == UINT64_MAX)
    slot->generation = 1U;
  else
    slot->generation++;
  slot->next_free = free_slot_head;
  free_slot_head = index;
}

static void registry_link(struct combat_encounter_data *encounter)
{
  encounter->registry_previous = NULL;
  encounter->registry_next = encounter_registry;
  if (encounter_registry != NULL)
    encounter_registry->registry_previous = encounter;
  encounter_registry = encounter;
  active_encounter_count++;
  if (active_encounter_count > cumulative_stats.high_water_encounters)
    cumulative_stats.high_water_encounters = active_encounter_count;
}

static void registry_unlink(struct combat_encounter_data *encounter)
{
  if (encounter->registry_previous != NULL)
    encounter->registry_previous->registry_next = encounter->registry_next;
  else if (encounter_registry == encounter)
    encounter_registry = encounter->registry_next;
  if (encounter->registry_next != NULL)
    encounter->registry_next->registry_previous = encounter->registry_previous;
  encounter->registry_previous = NULL;
  encounter->registry_next = NULL;
  if (active_encounter_count > 0U)
    active_encounter_count--;
}

static void member_append(struct combat_encounter_data *encounter,
                          struct combat_encounter_participant *participant)
{
  participant->previous = encounter->participants_tail;
  participant->next = NULL;
  if (encounter->participants_tail != NULL)
    encounter->participants_tail->next = participant;
  else
    encounter->participants = participant;
  encounter->participants_tail = participant;
  participant->pending_add = false;
}

static void pending_append(struct combat_encounter_data *encounter,
                           struct combat_encounter_participant *participant)
{
  participant->previous = encounter->pending_additions_tail;
  participant->next = NULL;
  if (encounter->pending_additions_tail != NULL)
    encounter->pending_additions_tail->next = participant;
  else
    encounter->pending_additions = participant;
  encounter->pending_additions_tail = participant;
  participant->pending_add = true;
}

static void member_remove(struct combat_encounter_data *encounter,
                          struct combat_encounter_participant *participant)
{
  struct combat_encounter_participant **head;
  struct combat_encounter_participant **tail;

  if (participant->pending_add)
  {
    head = &encounter->pending_additions;
    tail = &encounter->pending_additions_tail;
  }
  else
  {
    head = &encounter->participants;
    tail = &encounter->participants_tail;
  }
  if (participant->previous != NULL)
    participant->previous->next = participant->next;
  else if (*head == participant)
    *head = participant->next;
  if (participant->next != NULL)
    participant->next->previous = participant->previous;
  else if (*tail == participant)
    *tail = participant->previous;
  participant->previous = NULL;
  participant->next = NULL;
}

static bool participant_due_before(const struct combat_encounter_participant *left,
                                   const struct combat_encounter_participant *right)
{
  if (left->next_due != right->next_due)
    return left->next_due < right->next_due;
  if (left->due_sequence != right->due_sequence)
    return left->due_sequence > right->due_sequence;
  return left->character_handle.runtime_id < right->character_handle.runtime_id;
}

static void due_remove(struct combat_encounter_data *encounter,
                       struct combat_encounter_participant *participant)
{
  if (encounter == NULL || participant == NULL || !participant->in_due_list)
    return;
  if (participant->due_previous != NULL)
    participant->due_previous->due_next = participant->due_next;
  else
    encounter->due_head = participant->due_next;
  if (participant->due_next != NULL)
    participant->due_next->due_previous = participant->due_previous;
  else
    encounter->due_tail = participant->due_previous;
  participant->due_previous = NULL;
  participant->due_next = NULL;
  participant->in_due_list = false;
}

static void due_insert(struct combat_encounter_data *encounter,
                       struct combat_encounter_participant *participant)
{
  struct combat_encounter_participant *cursor;

  if (encounter == NULL || participant == NULL || !participant->active ||
      participant->pending_add || participant->pending_activation)
    return;
  due_remove(encounter, participant);
  for (cursor = encounter->due_head; cursor != NULL; cursor = cursor->due_next)
    if (participant_due_before(participant, cursor))
      break;
  if (cursor == NULL)
  {
    participant->due_previous = encounter->due_tail;
    participant->due_next = NULL;
    if (encounter->due_tail != NULL)
      encounter->due_tail->due_next = participant;
    else
      encounter->due_head = participant;
    encounter->due_tail = participant;
  }
  else
  {
    participant->due_previous = cursor->due_previous;
    participant->due_next = cursor;
    if (cursor->due_previous != NULL)
      cursor->due_previous->due_next = participant;
    else
      encounter->due_head = participant;
    cursor->due_previous = participant;
  }
  participant->in_due_list = true;
}

/* Merge logical clocks without moving any pending attack phase. */
static void align_semantic_participants(struct combat_encounter_data *encounter,
                                        uint64_t next_round_due)
{
  struct combat_encounter_participant *participant;

  for (participant = encounter->participants; participant != NULL; participant = participant->next)
    if (participant->active && !participant->pending_activation &&
        participant->next_turn_due < next_round_due)
      participant->next_turn_due = next_round_due;
}

static struct combat_encounter_participant *semantic_participant(struct char_data *character)
{
  struct combat_encounter_participant *participant;

  if (!initialized || !encounter_mode || !semantic_rounds || shutting_down || character == NULL)
    return NULL;
  participant = character->combat_encounter_participant;
  if (participant == NULL || participant->character != character || participant->departing ||
      (!participant->active && !participant->pending_activation))
    return NULL;
  return participant;
}

bool combat_encounter_get_turn(struct char_data *character,
                               struct combat_encounter_turn_snapshot *snapshot)
{
  struct combat_encounter_participant *participant;

  if (snapshot == NULL)
    return false;
  memset(snapshot, 0, sizeof(*snapshot));
  participant = semantic_participant(character);
  if (participant == NULL || character->combat_turn_serial == UINT64_MAX)
    return false;
  snapshot->turn_serial = character->combat_turn_serial;
  snapshot->dispatching = participant->dispatching;
  snapshot->pulses_until_next_turn = participant->dispatching ? COMBAT_ENCOUNTER_ROUND_DELAY
                                     : participant->next_turn_due > (uint64_t)pulse
                                         ? participant->next_turn_due - (uint64_t)pulse
                                         : 0U;
  return true;
}

static bool participant_character_is_live(const struct combat_encounter_participant *participant)
{
  return participant->character != NULL &&
         (encounter_bus == NULL ||
          domain_event_resolve(encounter_bus, participant->character_handle,
                               DOMAIN_ENTITY_CHARACTER) == participant->character);
}

static void leave_tactical_clocks(struct combat_encounter_participant *participant)
{
  if (participant == NULL || shutting_down || !participant_character_is_live(participant))
    return;
  tactical_room_hazards_leave_combat(participant->character);
  tactical_defense_leave_combat(participant->character);
  tactical_bleeding_leave_combat(participant->character);
}

static void free_participant(struct combat_encounter_participant *participant)
{
  if (participant == NULL)
    return;
  if (participant_character_is_live(participant) &&
      participant->character->combat_encounter_participant == participant)
  {
    participant->character->combat_encounter = NULL;
    participant->character->combat_encounter_participant = NULL;
  }
  free(participant);
  if (active_participant_count > 0U)
    active_participant_count--;
}

static void detach_participant(struct combat_encounter_data *encounter,
                               struct combat_encounter_participant *participant)
{
  due_remove(encounter, participant);
  member_remove(encounter, participant);
  free_participant(participant);
}

static struct combat_encounter_participant *add_participant(struct combat_encounter_data *encounter,
                                                            struct char_data *character)
{
  struct combat_encounter_participant *participant;

  if (encounter == NULL || character == NULL || character->combat_encounter != NULL)
    return NULL;
  participant = calloc(1U, sizeof(*participant));
  if (participant == NULL)
    return NULL;
  participant->encounter = encounter;
  participant->character = character;
  participant->character_handle = domain_event_character_handle(character);
  participant->phase = 1U;
  if (encounter->resolving)
    pending_append(encounter, participant);
  else
    member_append(encounter, participant);
  character->combat_encounter = encounter;
  character->combat_encounter_participant = participant;
  active_participant_count++;
  if (active_participant_count > cumulative_stats.high_water_participants)
    cumulative_stats.high_water_participants = active_participant_count;
  counter_increment(&cumulative_stats.participants_joined);
  return participant;
}

static void activate_participant(struct combat_encounter_participant *participant,
                                 long initial_delay)
{
  uint64_t delay;
  uint64_t due;

  if (participant == NULL || participant->active || participant->pending_activation)
    return;
  participant->initiative = GET_INITIATIVE(participant->character);
  delay = initial_delay > 0L ? (uint64_t)initial_delay : 1U;
  due = (uint64_t)pulse + delay;
  participant->next_turn_due = participant->encounter->next_round_due > (uint64_t)pulse
                                   ? participant->encounter->next_round_due
                                   : (uint64_t)pulse + COMBAT_ENCOUNTER_ROUND_DELAY;
  if (participant->encounter->resolving)
    participant->pending_activation = true;
  else
    participant->active = true;
  participant->phase = 1U;
  participant->next_due = due;
  participant->due_sequence = allocate_due_sequence();
  if (participant->active)
    due_insert(participant->encounter, participant);
}

static struct combat_encounter_data *create_encounter(void)
{
  struct combat_encounter_data *encounter;

  encounter = calloc(1U, sizeof(*encounter));
  if (encounter == NULL || !allocate_encounter_slot(encounter))
  {
    free(encounter);
    counter_increment(&cumulative_stats.admission_failures);
    return NULL;
  }
  registry_link(encounter);
  encounter->semantic_round = 0U;
  encounter->next_round_due = semantic_rounds ? (uint64_t)pulse + COMBAT_ENCOUNTER_ROUND_DELAY : 0U;
  counter_increment(&cumulative_stats.encounters_created);
  return encounter;
}

static struct event_runtime_handle create_round_event(struct combat_encounter_data *encounter,
                                                      uint64_t delay);
static void destroy_encounter(struct combat_encounter_data *encounter, bool dispatching);

/* The encounter owns one wakeup for the earliest attack or retained turn clock. */
static uint64_t next_encounter_delay(const struct combat_encounter_data *encounter)
{
  uint64_t due = encounter->due_head->next_due;

  if (semantic_rounds && encounter->next_round_due < due)
    due = encounter->next_round_due;
  return due > (uint64_t)pulse ? due - (uint64_t)pulse : 1U;
}

static bool ensure_round_event(struct combat_encounter_data *encounter)
{
  enum game_scheduler_status status;
  game_tick_t remaining;
  uint64_t delay;

  if (encounter == NULL || encounter->terminal || encounter->due_head == NULL)
    return false;
  delay = next_encounter_delay(encounter);
  if (event_runtime_handle_is_none(encounter->event_handle))
  {
    encounter->event_handle = create_round_event(encounter, delay);
    if (event_runtime_handle_is_none(encounter->event_handle))
      return false;
    scheduled_event_count++;
    return true;
  }
  if (encounter->resolving)
    return true;
  status = event_runtime_remaining(encounter->event_handle, &remaining);
  if (status == GAME_SCHEDULER_OK && remaining <= delay)
    return true;
  if (status == GAME_SCHEDULER_OK)
  {
    (void)event_runtime_reschedule_after(encounter->event_handle, (game_tick_t)delay);
    return true;
  }
  encounter->event_handle = EVENT_RUNTIME_HANDLE_NONE;
  if (scheduled_event_count > 0U)
    scheduled_event_count--;
  encounter->event_handle = create_round_event(encounter, delay);
  if (event_runtime_handle_is_none(encounter->event_handle))
    return false;
  scheduled_event_count++;
  return true;
}

static bool has_pending_hostility(const struct combat_encounter_data *encounter)
{
  const struct combat_encounter_participant *participant;

  for (participant = encounter->participants; participant != NULL; participant = participant->next)
    if (participant->active && participant_character_is_live(participant) &&
        FIGHTING(participant->character) != NULL)
      return true;
  for (participant = encounter->pending_additions; participant != NULL;
       participant = participant->next)
    if ((participant->active || participant->pending_activation) &&
        participant_character_is_live(participant) && FIGHTING(participant->character) != NULL)
      return true;
  return false;
}

static void maybe_end_encounter(struct combat_encounter_data *encounter)
{
  if (encounter == NULL || encounter->terminal || has_pending_hostility(encounter))
    return;
  encounter->terminal = true;
  if (!encounter->resolving && encounter->pending_into == NULL)
    destroy_encounter(encounter, false);
}

static void destroy_encounter(struct combat_encounter_data *encounter, bool dispatching)
{
  struct combat_encounter_participant *participant;
  struct combat_encounter_participant *next;

  if (encounter == NULL)
    return;
  if (!event_runtime_handle_is_none(encounter->event_handle))
  {
    if (!dispatching)
      (void)event_runtime_cancel(encounter->event_handle);
    encounter->event_handle = EVENT_RUNTIME_HANDLE_NONE;
    if (scheduled_event_count > 0U)
      scheduled_event_count--;
  }
  for (participant = encounter->participants; participant != NULL; participant = next)
  {
    next = participant->next;
    leave_tactical_clocks(participant);
    free_participant(participant);
  }
  for (participant = encounter->pending_additions; participant != NULL; participant = next)
  {
    next = participant->next;
    leave_tactical_clocks(participant);
    free_participant(participant);
  }
  encounter->participants = NULL;
  encounter->participants_tail = NULL;
  encounter->pending_additions = NULL;
  encounter->pending_additions_tail = NULL;
  encounter->due_head = NULL;
  encounter->due_tail = NULL;
  registry_unlink(encounter);
  release_encounter_slot(encounter);
  counter_increment(&cumulative_stats.encounters_ended);
  free(encounter);
}

static struct combat_encounter_data *merge_root(struct combat_encounter_data *encounter)
{
  while (encounter != NULL && encounter->pending_into != NULL)
    encounter = encounter->pending_into;
  return encounter;
}

static void detach_terminal_membership(struct char_data *character)
{
  struct combat_encounter_participant *participant;
  struct combat_encounter_data *encounter;

  if (character == NULL || character->combat_encounter_participant == NULL ||
      character->combat_encounter == NULL || !character->combat_encounter->terminal)
    return;
  participant = character->combat_encounter_participant;
  encounter = participant->encounter;
  leave_tactical_clocks(participant);
  participant->active = false;
  participant->pending_activation = false;
  participant->departing = true;
  due_remove(encounter, participant);
  character->combat_encounter = NULL;
  character->combat_encounter_participant = NULL;
  counter_increment(&cumulative_stats.participants_left);
  counter_increment(&cumulative_stats.departure_counts[COMBAT_ENCOUNTER_DEPARTURE_ADMINISTRATIVE]);
  if (!encounter->resolving)
    detach_participant(encounter, participant);
}

static void queue_merge(struct combat_encounter_data *survivor,
                        struct combat_encounter_data *absorbed)
{
  if (survivor == NULL || absorbed == NULL || survivor == absorbed ||
      absorbed->pending_into == survivor)
    return;
  absorbed->pending_into = survivor;
  absorbed->pending_merge_next = NULL;
  if (survivor->pending_merge_tail != NULL)
    survivor->pending_merge_tail->pending_merge_next = absorbed;
  else
    survivor->pending_merge_head = absorbed;
  survivor->pending_merge_tail = absorbed;
}

static void transfer_participant(struct combat_encounter_data *survivor,
                                 struct combat_encounter_data *absorbed,
                                 struct combat_encounter_participant *participant)
{
  due_remove(absorbed, participant);
  member_remove(absorbed, participant);
  participant->encounter = survivor;
  if (participant_character_is_live(participant) &&
      participant->character->combat_encounter_participant == participant)
    participant->character->combat_encounter = survivor;
  member_append(survivor, participant);
  if (participant->active)
  {
    if (semantic_rounds && survivor->resolving)
      participant->next_turn_due =
          u64_max(participant->next_turn_due, (uint64_t)pulse + COMBAT_ENCOUNTER_ROUND_DELAY);
    due_insert(survivor, participant);
  }
}

static void merge_now(struct combat_encounter_data *survivor,
                      struct combat_encounter_data *absorbed)
{
  struct combat_encounter_participant *participant;
  struct combat_encounter_participant *next;

  if (survivor == NULL || absorbed == NULL || survivor == absorbed)
    return;
  if (!event_runtime_handle_is_none(absorbed->event_handle))
  {
    (void)event_runtime_cancel(absorbed->event_handle);
    absorbed->event_handle = EVENT_RUNTIME_HANDLE_NONE;
    if (scheduled_event_count > 0U)
      scheduled_event_count--;
  }
  for (participant = absorbed->participants; participant != NULL; participant = next)
  {
    next = participant->next;
    transfer_participant(survivor, absorbed, participant);
  }
  for (participant = absorbed->pending_additions; participant != NULL; participant = next)
  {
    next = participant->next;
    member_remove(absorbed, participant);
    participant->encounter = survivor;
    if (participant_character_is_live(participant) &&
        participant->character->combat_encounter_participant == participant)
      participant->character->combat_encounter = survivor;
    if (semantic_rounds && survivor->resolving)
      participant->next_turn_due =
          u64_max(participant->next_turn_due, (uint64_t)pulse + COMBAT_ENCOUNTER_ROUND_DELAY);
    pending_append(survivor, participant);
  }
  absorbed->participants = NULL;
  absorbed->participants_tail = NULL;
  absorbed->pending_additions = NULL;
  absorbed->pending_additions_tail = NULL;
  absorbed->due_head = NULL;
  absorbed->due_tail = NULL;
  absorbed->pending_into = NULL;
  registry_unlink(absorbed);
  release_encounter_slot(absorbed);
  counter_increment(&cumulative_stats.encounters_merged);
  free(absorbed);
  (void)ensure_round_event(survivor);
}

static struct combat_encounter_data *merge_encounters(struct combat_encounter_data *left,
                                                      struct combat_encounter_data *right)
{
  struct combat_encounter_data *survivor;
  struct combat_encounter_data *absorbed;

  left = merge_root(left);
  right = merge_root(right);
  if (left == NULL)
    return right;
  if (right == NULL || left == right)
    return left;
  if (left->resolving)
  {
    queue_merge(left, right);
    return left;
  }
  if (right->resolving)
  {
    queue_merge(right, left);
    return right;
  }
  if (left->due_head == NULL ||
      (right->due_head != NULL &&
       (semantic_rounds ? right->next_round_due < left->next_round_due
                        : participant_due_before(right->due_head, left->due_head))))
  {
    survivor = right;
    absorbed = left;
  }
  else
  {
    survivor = left;
    absorbed = right;
  }
  merge_now(survivor, absorbed);
  return survivor;
}

static void apply_pending_additions(struct combat_encounter_data *encounter)
{
  struct combat_encounter_participant *participant;
  struct combat_encounter_participant *next;

  for (participant = encounter->pending_additions; participant != NULL; participant = next)
  {
    next = participant->next;
    member_remove(encounter, participant);
    member_append(encounter, participant);
    if (participant->pending_activation)
    {
      participant->pending_activation = false;
      participant->active = true;
      due_insert(encounter, participant);
    }
  }
  for (participant = encounter->participants; participant != NULL; participant = participant->next)
  {
    if (!participant->pending_activation)
      continue;
    participant->pending_activation = false;
    participant->active = true;
    due_insert(encounter, participant);
  }
}

static void compact_inactive_participants(struct combat_encounter_data *encounter)
{
  struct combat_encounter_participant *participant;
  struct combat_encounter_participant *next;

  for (participant = encounter->participants; participant != NULL; participant = next)
  {
    next = participant->next;
    if (participant->departing)
      detach_participant(encounter, participant);
  }
  for (participant = encounter->pending_additions; participant != NULL; participant = next)
  {
    next = participant->next;
    if (participant->departing)
      detach_participant(encounter, participant);
  }
}

static bool apply_pending_merges(struct combat_encounter_data *encounter)
{
  struct combat_encounter_data *absorbed;
  bool merged = false;

  while (encounter->pending_merge_head != NULL)
  {
    absorbed = encounter->pending_merge_head;
    encounter->pending_merge_head = absorbed->pending_merge_next;
    if (encounter->pending_merge_head == NULL)
      encounter->pending_merge_tail = NULL;
    absorbed->pending_merge_next = NULL;
    if (absorbed->terminal)
      destroy_encounter(absorbed, false);
    else
      merge_now(encounter, absorbed);
    merged = true;
  }
  return merged;
}

static bool run_attack_phase(struct char_data *character, unsigned int phase)
{
#ifdef LUMINARI_CUTEST
  if (test_phase_callback != NULL)
    return test_phase_callback(character, phase, test_phase_context);
#endif
  return combat_run_phase(character, phase);
}

static bool participant_live(const struct combat_encounter_participant *participant)
{
  return participant->active && !participant->departing &&
         participant_character_is_live(participant);
}

/* These hooks belong to logical six-second turns, independently of attacks. */
static void begin_semantic_round(struct combat_encounter_data *encounter)
{
  struct combat_encounter_participant *participant;

  for (participant = encounter->participants; participant != NULL; participant = participant->next)
  {
    if (!participant_live(participant) || participant->pending_activation ||
        participant->next_turn_due > (uint64_t)pulse)
      continue;
    participant->dispatching = true;
    ready_action_on_semantic_turn(participant->character);
    if (participant->character->combat_turn_serial != UINT64_MAX)
      participant->character->combat_turn_serial++;
    tactical_defense_on_turn(participant->character);
    primary_activity_on_semantic_turn(participant->character);
    counter_increment(&cumulative_stats.semantic_turns_resolved);
  }
}

static void end_semantic_round(struct combat_encounter_data *encounter)
{
  struct combat_encounter_participant *participant;

  for (participant = encounter->participants; participant != NULL; participant = participant->next)
  {
    if (!participant->dispatching)
      continue;
    if (participant_live(participant) && tactical_bleeding_on_turn_end(participant->character) &&
        participant_live(participant))
      (void)tactical_room_hazards_on_turn_end(participant->character);
    participant->dispatching = false;
  }
  counter_increment(&encounter->semantic_round);
  counter_increment(&cumulative_stats.semantic_rounds_resolved);
  encounter->next_round_due = (uint64_t)pulse + COMBAT_ENCOUNTER_ROUND_DELAY;
  align_semantic_participants(encounter, encounter->next_round_due);
}

static struct game_event_result
combat_encounter_round_event(const struct game_event_context *context)
{
  struct combat_encounter_event_payload *payload = context != NULL ? context->payload : NULL;
  struct combat_encounter_data *encounter;
  struct combat_encounter_participant *participant;
  bool completed;
  bool merged;
  bool semantic_round_started = false;
  uint64_t delay;

  if (payload == NULL)
    return game_event_result_complete();
  encounter = resolve_encounter(payload->id, payload->generation);
  if (encounter == NULL)
  {
    counter_increment(&cumulative_stats.stale_encounter_callbacks);
    event_note_stale_owner_outcome();
    return game_event_result_complete();
  }
  if (encounter->event_handle.id != context->event_id)
    return game_event_result_complete();
  counter_increment(&cumulative_stats.encounter_callbacks);
  encounter->resolving = true;
  if (semantic_rounds && encounter->due_head != NULL &&
      encounter->next_round_due <= (uint64_t)pulse)
  {
    begin_semantic_round(encounter);
    semantic_round_started = true;
  }
  do
  {
    while (encounter->due_head != NULL && encounter->due_head->next_due <= (uint64_t)pulse)
    {
      participant = encounter->due_head;
      due_remove(encounter, participant);
      if (!participant_character_is_live(participant))
      {
        event_note_stale_owner_outcome();
        counter_increment(&cumulative_stats.stale_encounter_callbacks);
        participant->active = false;
        participant->departing = true;
        continue;
      }
      counter_increment(&cumulative_stats.phase_attempts);
      completed = run_attack_phase(participant->character, participant->phase);
      if (completed)
        counter_increment(&cumulative_stats.phases_resolved);
      else
        counter_increment(&cumulative_stats.phase_terminal);
      if (participant->active && participant->encounter == encounter && completed)
      {
        participant->phase = participant->phase < 3U ? participant->phase + 1U : 1U;
        participant->next_due = (uint64_t)pulse + COMBAT_ENCOUNTER_PHASE_DELAY;
        participant->due_sequence = allocate_due_sequence();
        due_insert(encounter, participant);
      }
      else if (participant->active && participant->encounter == encounter)
      {
        participant->active = false;
        participant->departing = true;
      }
    }
    apply_pending_additions(encounter);
    merged = apply_pending_merges(encounter);
    if (!merged && semantic_round_started)
    {
      end_semantic_round(encounter);
      semantic_round_started = false;
      /* Bleeding and hazard callbacks can also change encounter membership. */
      apply_pending_additions(encounter);
      merged = apply_pending_merges(encounter);
    }
  } while (merged);
  compact_inactive_participants(encounter);
  maybe_end_encounter(encounter);
  encounter->resolving = false;
  if (encounter->terminal || encounter->due_head == NULL)
  {
    destroy_encounter(encounter, true);
    return game_event_result_complete();
  }
  delay = next_encounter_delay(encounter);
  return game_event_result_reschedule_after(delay);
}

static void combat_encounter_event_cleanup(void *payload)
{
  free(payload);
}

static bool register_encounter_round_event_type(void)
{
  struct game_event_type_config config;
  const char *registered_name;
  enum game_scheduler_status status;

  if (!event_runtime_is_initialized())
    return false;
  registered_name = event_runtime_type_name(encounter_round_event_type);
  if (registered_name != NULL && !strcmp(registered_name, "combat.encounter.round"))
    return true;
  encounter_round_event_type = 0U;
  memset(&config, 0, sizeof(config));
  config.name = "combat.encounter.round";
  config.handler = combat_encounter_round_event;
  config.cleanup = combat_encounter_event_cleanup;
  config.lateness_policy = GAME_EVENT_LATENESS_RUN_ONCE;
  config.max_events = COMBAT_ENCOUNTER_MAX_ACTIVE;
  config.max_events_per_owner = 1U;
  config.requires_owner = true;
  status = event_runtime_register_type(&config, &encounter_round_event_type);
  if (status != GAME_SCHEDULER_OK)
  {
    log("SYSERR: unable to register native event type 'combat.encounter.round' (status %u).",
        status);
    return false;
  }
  return true;
}

static struct event_runtime_handle create_round_event(struct combat_encounter_data *encounter,
                                                      uint64_t delay)
{
  struct combat_encounter_event_payload *payload;
  struct game_event_owner owner;
  struct event_runtime_handle handle = EVENT_RUNTIME_HANDLE_NONE;

  payload = calloc(1U, sizeof(*payload));
  if (payload == NULL)
  {
    counter_increment(&cumulative_stats.admission_failures);
    return EVENT_RUNTIME_HANDLE_NONE;
  }
  payload->id = encounter->id;
  payload->generation = encounter->generation;
  owner.kind = GAME_EVENT_OWNER_ENCOUNTER;
  owner.runtime_id = encounter->id;
  owner.generation = encounter->generation;
  if (event_runtime_schedule_owned_after(encounter_round_event_type, owner, (game_tick_t)delay,
                                         payload, &handle) != GAME_SCHEDULER_OK)
  {
    free(payload);
    counter_increment(&cumulative_stats.admission_failures);
  }
  return handle;
}

bool combat_encounter_join(struct char_data *character, struct char_data *opponent,
                           long initial_delay)
{
  struct combat_encounter_data *character_encounter;
  struct combat_encounter_data *opponent_encounter;
  struct combat_encounter_data *encounter;
  struct combat_encounter_participant *participant;

  if (!combat_encounter_events_enabled() || character == NULL || opponent == NULL ||
      character == opponent || shutting_down)
    return false;
  detach_terminal_membership(character);
  detach_terminal_membership(opponent);
  character_encounter = character->combat_encounter;
  opponent_encounter = opponent->combat_encounter;
  if (character_encounter == NULL && opponent_encounter == NULL)
  {
    encounter = create_encounter();
    if (encounter == NULL)
      return false;
    participant = add_participant(encounter, character);
    if (participant == NULL || add_participant(encounter, opponent) == NULL)
    {
      destroy_encounter(encounter, false);
      counter_increment(&cumulative_stats.admission_failures);
      return false;
    }
  }
  else
  {
    encounter = character_encounter != NULL ? character_encounter : opponent_encounter;
    if (character_encounter != NULL && opponent_encounter != NULL &&
        merge_root(character_encounter) != merge_root(opponent_encounter))
      encounter = merge_encounters(character_encounter, opponent_encounter);
    if (character->combat_encounter == NULL && add_participant(encounter, character) == NULL)
    {
      counter_increment(&cumulative_stats.admission_failures);
      return false;
    }
    if (opponent->combat_encounter == NULL && add_participant(encounter, opponent) == NULL)
    {
      counter_increment(&cumulative_stats.admission_failures);
      return false;
    }
  }
  participant = character->combat_encounter_participant;
  activate_participant(participant, initial_delay);
  tactical_room_hazards_enter_combat(character);
  encounter = merge_root(character->combat_encounter);
  if (encounter != NULL && !encounter->resolving && !ensure_round_event(encounter))
  {
    destroy_encounter(encounter, false);
    return false;
  }
  return true;
}

void combat_encounter_leave(struct char_data *character,
                            enum combat_encounter_departure_reason reason)
{
  struct combat_encounter_participant *participant;
  struct combat_encounter_data *encounter;

  if (character == NULL || character->combat_encounter_participant == NULL)
    return;
  participant = character->combat_encounter_participant;
  encounter = participant->encounter;
  if (reason != COMBAT_ENCOUNTER_DEPARTURE_DIED && reason != COMBAT_ENCOUNTER_DEPARTURE_EXTRACTED)
    leave_tactical_clocks(participant);
  participant->active = false;
  participant->pending_activation = false;
  participant->departing = true;
  due_remove(encounter, participant);
  character->combat_encounter = NULL;
  character->combat_encounter_participant = NULL;
  counter_increment(&cumulative_stats.participants_left);
  if (reason >= COMBAT_ENCOUNTER_DEPARTURE_STOPPED && reason < COMBAT_ENCOUNTER_DEPARTURE_COUNT)
    counter_increment(&cumulative_stats.departure_counts[reason]);
  if (!encounter->resolving)
    detach_participant(encounter, participant);
  maybe_end_encounter(encounter);
}

void combat_encounter_forget_character(struct char_data *character,
                                       enum combat_encounter_departure_reason reason)
{
  combat_encounter_leave(character, reason);
}

bool combat_encounter_semantic_rounds_enabled(void)
{
  return combat_encounter_events_enabled() && semantic_rounds;
}

bool combat_encounter_semantic_manages(const struct char_data *character)
{
  const struct combat_encounter_participant *participant;

  if (!combat_encounter_semantic_rounds_enabled() || character == NULL)
    return false;
  participant = character->combat_encounter_participant;
  return participant != NULL && participant->character == character && !participant->departing &&
         (participant->active || participant->pending_activation);
}

static void handle_character_moved(const struct domain_event_context *context,
                                   void *handler_context)
{
  const struct domain_character_moved *event = context->payload;
  struct char_data *character;

  (void)handler_context;
  character = domain_event_resolve(context->bus, event->character, DOMAIN_ENTITY_CHARACTER);
  if (character == NULL)
  {
    event_note_stale_owner_outcome();
    return;
  }
  if (character != NULL && character->combat_encounter != NULL &&
      (FIGHTING(character) == NULL || IN_ROOM(character) != IN_ROOM(FIGHTING(character))))
    combat_encounter_leave(character, COMBAT_ENCOUNTER_DEPARTURE_MOVED);
}

static void handle_character_died(const struct domain_event_context *context, void *handler_context)
{
  const struct domain_character_died *event = context->payload;
  struct char_data *character;

  (void)handler_context;
  character = domain_event_resolve(context->bus, event->character, DOMAIN_ENTITY_CHARACTER);
  if (character == NULL)
  {
    event_note_stale_owner_outcome();
    return;
  }
  combat_encounter_leave(character, COMBAT_ENCOUNTER_DEPARTURE_DIED);
}

static void handle_entity_extracted(const struct domain_event_context *context,
                                    void *handler_context)
{
  const struct domain_entity_extracted *event = context->payload;
  struct char_data *character;

  (void)handler_context;
  if (event->entity.kind != DOMAIN_ENTITY_CHARACTER)
    return;
  character = domain_event_resolve(context->bus, event->entity, DOMAIN_ENTITY_CHARACTER);
  if (character == NULL)
  {
    event_note_stale_owner_outcome();
    return;
  }
  combat_encounter_leave(character, COMBAT_ENCOUNTER_DEPARTURE_EXTRACTED);
}

enum domain_event_status combat_encounter_runtime_init(struct domain_event_bus *bus)
{
  static const struct domain_event_handler_config handlers[] = {
      {DOMAIN_EVENT_CHARACTER_MOVED, "combat-encounter-character-moved", 20, handle_character_moved,
       NULL},
      {DOMAIN_EVENT_CHARACTER_DIED, "combat-encounter-character-died", 20, handle_character_died,
       NULL},
      {DOMAIN_EVENT_ENTITY_EXTRACTED, "combat-encounter-entity-extracted", 20,
       handle_entity_extracted, NULL},
  };
  enum domain_event_status status;
  size_t index;

  if (initialized)
    return DOMAIN_EVENT_BUSY;
  memset(&cumulative_stats, 0, sizeof(cumulative_stats));
  memset(encounter_slots, 0, sizeof(encounter_slots));
  for (index = 0; index < COMBAT_ENCOUNTER_MAX_ACTIVE; index++)
  {
    encounter_slots[index].generation = 1U;
    encounter_slots[index].next_free =
        index + 1U < COMBAT_ENCOUNTER_MAX_ACTIVE ? (uint32_t)(index + 1U) : UINT32_MAX;
  }
  free_slot_head = 0U;
  encounter_registry = NULL;
  active_encounter_count = 0U;
  active_participant_count = 0U;
  scheduled_event_count = 0U;
  next_due_sequence = 1U;
  encounter_bus = bus;
  shutting_down = false;
  encounter_mode = configured_encounter_mode();
  semantic_rounds = encounter_mode && configured_semantic_rounds();
  if (encounter_mode && !register_encounter_round_event_type())
    return DOMAIN_EVENT_BUSY;
  initialized = true;
  log("Combat scheduling: %s.", !encounter_mode ? "test rollback character events"
                                : semantic_rounds
                                    ? "encounter attack phases with six-second effect turns"
                                    : "test encounter phases without effect turns");
  if (!encounter_mode || bus == NULL)
    return DOMAIN_EVENT_OK;
  for (index = 0; index < sizeof(handlers) / sizeof(handlers[0]); index++)
  {
    status = domain_event_register_handler(bus, &handlers[index]);
    if (status != DOMAIN_EVENT_OK)
      return status;
  }
  return DOMAIN_EVENT_OK;
}

void combat_encounter_runtime_shutdown(void)
{
  struct combat_encounter_data *encounter;

  if (!initialized)
    return;
  for (encounter = encounter_registry; encounter != NULL; encounter = encounter->registry_next)
  {
    struct combat_encounter_participant *participant;

    for (participant = encounter->participants; participant != NULL;
         participant = participant->next)
      if (participant_character_is_live(participant))
      {
        tactical_defense_pause(participant->character);
        tactical_bleeding_pause(participant->character);
        tactical_room_hazards_forget(participant->character);
      }
    for (participant = encounter->pending_additions; participant != NULL;
         participant = participant->next)
      if (participant_character_is_live(participant))
      {
        tactical_defense_pause(participant->character);
        tactical_bleeding_pause(participant->character);
        tactical_room_hazards_forget(participant->character);
      }
  }
  shutting_down = true;
  while ((encounter = encounter_registry) != NULL)
    destroy_encounter(encounter, false);
  initialized = false;
  encounter_mode = false;
  semantic_rounds = false;
  encounter_bus = NULL;
  shutting_down = false;
#ifdef LUMINARI_CUTEST
  test_selection_set = false;
  test_encounter_mode = true;
  test_semantic_selection_set = false;
  test_semantic_rounds = true;
  test_phase_callback = NULL;
  test_phase_context = NULL;
#endif
}

bool combat_encounter_events_enabled(void)
{
  return initialized && encounter_mode && !shutting_down;
}

void combat_encounter_get_stats(struct combat_encounter_stats *stats)
{
  if (stats == NULL)
    return;
  *stats = cumulative_stats;
  stats->initialized = initialized;
  stats->encounter_mode = encounter_mode;
  stats->semantic_rounds = semantic_rounds;
  stats->active_encounters = active_encounter_count;
  stats->active_participants = active_participant_count;
  stats->scheduled_events = scheduled_event_count;
  if (active_encounter_count != scheduled_event_count ||
      cumulative_stats.phase_attempts !=
          cumulative_stats.phases_resolved + cumulative_stats.phase_terminal)
    stats->phase_mismatches++;
}

bool combat_encounter_get_initiative(const struct char_data *viewer,
                                     struct combat_encounter_initiative_entry *entries,
                                     size_t capacity,
                                     struct combat_encounter_initiative_snapshot *snapshot)
{
  struct combat_encounter_data *encounter;
  struct combat_encounter_participant *participant;

  if (snapshot == NULL)
    return false;
  memset(snapshot, 0, sizeof(*snapshot));
  if (!initialized || viewer == NULL || viewer->combat_encounter == NULL ||
      viewer->combat_encounter_participant == NULL)
    return false;
  encounter = merge_root(viewer->combat_encounter);
  if (encounter == NULL || encounter->terminal)
    return false;
  snapshot->semantic_rounds = semantic_rounds;
  if (!semantic_rounds)
    return true;
  snapshot->round_number = encounter->semantic_round + 1U;
  snapshot->pulses_until_round = encounter->next_round_due > (uint64_t)pulse
                                     ? encounter->next_round_due - (uint64_t)pulse
                                     : 0U;
  for (participant = encounter->due_head; participant != NULL; participant = participant->due_next)
  {
    if (!participant_live(participant))
      continue;
    if (snapshot->entry_count < capacity && entries != NULL)
    {
      entries[snapshot->entry_count].character = participant->character;
      entries[snapshot->entry_count].initiative = participant->initiative;
      entries[snapshot->entry_count].phase = participant->phase;
      entries[snapshot->entry_count].pulses_until_phase =
          participant->next_due > (uint64_t)pulse ? participant->next_due - (uint64_t)pulse : 0U;
      snapshot->entry_count++;
    }
    snapshot->total_participants++;
  }
  return true;
}

#ifdef LUMINARI_CUTEST
void combat_encounter_test_select(bool selected_encounter_mode)
{
  test_selection_set = true;
  test_encounter_mode = selected_encounter_mode;
}

void combat_encounter_test_select_semantic(bool selected_semantic_rounds)
{
  test_semantic_selection_set = true;
  test_semantic_rounds = selected_semantic_rounds;
}

void combat_encounter_test_set_phase_callback(combat_encounter_test_phase_callback callback,
                                              void *context)
{
  test_phase_callback = callback;
  test_phase_context = context;
}
#endif
