#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "dotenv.h"
#include "actions.h"
#include "combat/combat_encounters.h"
#include "combat/fight.h"
#include "dgscript/dg_event.h"
#include "domain_event_runtime.h"
#include "domain_event_types.h"
#include "domain_event_world.h"
#include "event_runtime.h"

#define COMBAT_ENCOUNTER_MAX_ACTIVE 32768U
#define COMBAT_ENCOUNTER_PHASE_DELAY ((uint64_t)(2 RL_SEC))
#define COMBAT_ENCOUNTER_JOIN_GUARD ((uint64_t)(6 RL_SEC))
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
  uint64_t due_sequence;
  uint64_t turns_started;
  uint64_t action_ready_turn[NUM_ACTIONS];
  int initiative;
  int dexterity_tiebreak;
  unsigned int phase;
  int reactions_used;
  uint32_t round_flags;
  bool action_notice_pending[NUM_ACTIONS];
  bool semantic_state_imported;
  bool perfect_tempo_was_hit;
  bool intent_dispatched;
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
  uint64_t compatibility_phase;
  uint64_t compatibility_round;
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
  const char *value;

#ifdef LUMINARI_CUTEST
  if (test_selection_set)
    return test_encounter_mode;
#endif
  value = getenv("LUMINARI_COMBAT_EVENTS");
  if (value == NULL || *value == '\0')
    value = get_env_value("LUMINARI_COMBAT_EVENTS");
  if (value == NULL || *value == '\0' || !strcasecmp(value, "encounter") ||
      !strcasecmp(value, "scheduled") || !strcasecmp(value, "event"))
    return true;
  if (!strcasecmp(value, "legacy") || !strcasecmp(value, "character") ||
      !strcasecmp(value, "off"))
    return false;
  log("WARNING: Unknown LUMINARI_COMBAT_EVENTS '%s'; using encounter events.", value);
  return true;
}

static bool configured_semantic_rounds(void)
{
  const char *value;

#ifdef LUMINARI_CUTEST
  if (test_semantic_selection_set)
    return test_semantic_rounds;
#endif
  value = getenv("LUMINARI_COMBAT_ROUNDS");
  if (value == NULL || *value == '\0')
    value = get_env_value("LUMINARI_COMBAT_ROUNDS");
  if (value == NULL || *value == '\0' || !strcasecmp(value, "semantic") ||
      !strcasecmp(value, "round") || !strcasecmp(value, "d20"))
    return true;
  if (!strcasecmp(value, "compatibility") || !strcasecmp(value, "phased") ||
      !strcasecmp(value, "legacy"))
    return false;
  log("WARNING: Unknown LUMINARI_COMBAT_ROUNDS '%s'; using semantic rounds.", value);
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

  if (encounter == NULL || encounter->id == 0U ||
      encounter->id > COMBAT_ENCOUNTER_MAX_ACTIVE)
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
  if (semantic_rounds && left->initiative != right->initiative)
    return left->initiative > right->initiative;
  if (semantic_rounds && left->dexterity_tiebreak != right->dexterity_tiebreak)
    return left->dexterity_tiebreak > right->dexterity_tiebreak;
  if (semantic_rounds &&
      left->character_handle.runtime_id != right->character_handle.runtime_id)
    return left->character_handle.runtime_id < right->character_handle.runtime_id;
  if (left->due_sequence != right->due_sequence)
    return left->due_sequence < right->due_sequence;
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

static void align_semantic_participants(struct combat_encounter_data *encounter,
                                        uint64_t next_round_due)
{
  struct combat_encounter_participant *participant;

  if (!semantic_rounds || encounter == NULL)
    return;
  for (participant = encounter->participants; participant != NULL;
       participant = participant->next)
  {
    if (!participant->active || participant->pending_activation ||
        participant->next_due >= next_round_due)
      continue;
    due_remove(encounter, participant);
    participant->next_due = next_round_due;
    participant->due_sequence = allocate_due_sequence();
    due_insert(encounter, participant);
  }
}

static const event_id action_event_ids[NUM_ACTIONS] = {
    eSTANDARDACTION,
    eMOVEACTION,
    eSWIFTACTION,
};

static const event_id round_flag_event_ids[COMBAT_ENCOUNTER_ROUND_FLAG_COUNT] = {
    ePERFECT_TEMPO_HIT_THIS_ROUND,
    eDEFLECTIVE_SCREEN_HIT_THIS_ROUND,
    eSMASH_DEFENSE,
    eRELENTLESS_ASSAULT,
};

static uint64_t semantic_rounds_for_delay(long delay)
{
  uint64_t positive_delay;

  positive_delay = delay > 0L ? (uint64_t)delay : 1U;
  return (positive_delay + COMBAT_ENCOUNTER_ROUND_DELAY - 1U) /
         COMBAT_ENCOUNTER_ROUND_DELAY;
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

static void import_semantic_state(struct combat_encounter_participant *participant)
{
  struct mud_event_data *event_data;
  uint64_t rounds;
  size_t index;

  if (participant == NULL || participant->semantic_state_imported ||
      participant->character == NULL)
    return;
  participant->semantic_state_imported = true;
  for (index = 0U; index < NUM_ACTIONS; index++)
  {
    event_data = char_has_mud_event(participant->character, action_event_ids[index]);
    if (event_data == NULL)
      continue;
    rounds = semantic_rounds_for_delay(event_handle_time(event_data->event_handle));
    participant->action_ready_turn[index] = participant->turns_started + rounds;
    participant->action_notice_pending[index] = true;
    event_cancel_specific(participant->character, action_event_ids[index]);
  }
  for (index = 0U; index < COMBAT_ENCOUNTER_ROUND_FLAG_COUNT; index++)
  {
    if (char_has_mud_event(participant->character, round_flag_event_ids[index]) == NULL)
      continue;
    participant->round_flags |= (uint32_t)1U << index;
    event_cancel_specific(participant->character, round_flag_event_ids[index]);
  }
}

static void restore_semantic_action_events(struct combat_encounter_participant *participant)
{
  char duration_text[32];
  uint64_t remaining_rounds;
  uint64_t delay;
  size_t index;

  if (participant == NULL || !participant->semantic_state_imported ||
      participant->character == NULL || shutting_down)
    return;
  for (index = 0U; index < NUM_ACTIONS; index++)
  {
    if (participant->action_ready_turn[index] <= participant->turns_started)
      continue;
    remaining_rounds = participant->action_ready_turn[index] - participant->turns_started;
    delay = remaining_rounds > UINT64_MAX / COMBAT_ENCOUNTER_ROUND_DELAY
                ? UINT64_MAX
                : remaining_rounds * COMBAT_ENCOUNTER_ROUND_DELAY;
    snprintf(duration_text, sizeof(duration_text), "%ld",
             delay > LONG_MAX ? LONG_MAX : (long)delay);
    attach_mud_event(
        new_mud_event(action_event_ids[index], participant->character, duration_text),
        delay > LONG_MAX ? LONG_MAX : (long)delay);
  }
}

static void announce_recovered_actions(struct combat_encounter_participant *participant)
{
  static const char *const messages[NUM_ACTIONS] = {
      "You may perform another standard action.\r\n",
      "You may perform another move action.\r\n",
      "You may perform another swift action.\r\n",
  };
  size_t index;

  for (index = 0U; index < NUM_ACTIONS; index++)
  {
    if (!participant->action_notice_pending[index] ||
        participant->action_ready_turn[index] > participant->turns_started)
      continue;
    participant->action_notice_pending[index] = false;
    send_to_char(participant->character, "%s", messages[index]);
  }
  update_msdp_actions(participant->character);
}

static void free_participant(struct combat_encounter_participant *participant)
{
  if (participant == NULL)
    return;
  if (participant->character != NULL &&
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

static struct combat_encounter_participant *add_participant(
    struct combat_encounter_data *encounter, struct char_data *character)
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
  if (semantic_rounds)
    import_semantic_state(participant);
  participant->initiative = GET_INITIATIVE(participant->character);
  participant->dexterity_tiebreak = GET_DEX(participant->character);
  delay = initial_delay > 0L ? (uint64_t)initial_delay : 1U;
  if (semantic_rounds)
    due = participant->encounter->next_round_due > (uint64_t)pulse
              ? participant->encounter->next_round_due
              : (uint64_t)pulse + COMBAT_ENCOUNTER_ROUND_DELAY;
  else
    due = (uint64_t)pulse + delay;
  if (participant->encounter->resolving)
  {
    due = MAX(due, (uint64_t)pulse + (semantic_rounds ? COMBAT_ENCOUNTER_ROUND_DELAY
                                                      : COMBAT_ENCOUNTER_JOIN_GUARD));
    participant->pending_activation = true;
  }
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
  encounter->compatibility_phase = 1U;
  encounter->compatibility_round = 1U;
  encounter->semantic_round = 0U;
  encounter->next_round_due = semantic_rounds
                                  ? (uint64_t)pulse + COMBAT_ENCOUNTER_ROUND_DELAY
                                  : 0U;
  counter_increment(&cumulative_stats.encounters_created);
  return encounter;
}

static struct event_runtime_handle
create_round_event(struct combat_encounter_data *encounter, uint64_t delay);
static void destroy_encounter(struct combat_encounter_data *encounter, bool dispatching);

static bool ensure_round_event(struct combat_encounter_data *encounter)
{
  enum game_scheduler_status status;
  game_tick_t remaining;
  uint64_t delay;

  if (encounter == NULL || encounter->terminal || encounter->due_head == NULL)
    return false;
  if (semantic_rounds)
    delay = encounter->next_round_due > (uint64_t)pulse
                ? encounter->next_round_due - (uint64_t)pulse
                : 1U;
  else
    delay = encounter->due_head->next_due > (uint64_t)pulse
                ? encounter->due_head->next_due - (uint64_t)pulse
                : 1U;
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

  for (participant = encounter->participants; participant != NULL;
       participant = participant->next)
    if (participant->active && participant->character != NULL &&
        FIGHTING(participant->character) != NULL)
      return true;
  for (participant = encounter->pending_additions; participant != NULL;
       participant = participant->next)
    if ((participant->active || participant->pending_activation) &&
        participant->character != NULL && FIGHTING(participant->character) != NULL)
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
    restore_semantic_action_events(participant);
    free_participant(participant);
  }
  for (participant = encounter->pending_additions; participant != NULL; participant = next)
  {
    next = participant->next;
    restore_semantic_action_events(participant);
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
  restore_semantic_action_events(participant);
  participant->active = false;
  participant->pending_activation = false;
  participant->departing = true;
  due_remove(encounter, participant);
  character->combat_encounter = NULL;
  character->combat_encounter_participant = NULL;
  counter_increment(&cumulative_stats.participants_left);
  counter_increment(
      &cumulative_stats.departure_counts[COMBAT_ENCOUNTER_DEPARTURE_ADMINISTRATIVE]);
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
  if (participant->character != NULL &&
      participant->character->combat_encounter_participant == participant)
    participant->character->combat_encounter = survivor;
  member_append(survivor, participant);
  if (participant->active)
  {
    if (semantic_rounds && survivor->resolving)
      participant->next_due =
          MAX(participant->next_due, (uint64_t)pulse + COMBAT_ENCOUNTER_ROUND_DELAY);
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
    if (participant->character != NULL &&
        participant->character->combat_encounter_participant == participant)
      participant->character->combat_encounter = survivor;
    if (semantic_rounds && survivor->resolving)
      participant->next_due =
          MAX(participant->next_due, (uint64_t)pulse + COMBAT_ENCOUNTER_ROUND_DELAY);
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
      (right->due_head != NULL && participant_due_before(right->due_head, left->due_head)))
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
      participant->due_sequence = allocate_due_sequence();
      due_insert(encounter, participant);
    }
  }
  for (participant = encounter->participants; participant != NULL; participant = participant->next)
  {
    if (!participant->pending_activation)
      continue;
    participant->pending_activation = false;
    participant->active = true;
    participant->due_sequence = allocate_due_sequence();
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

static bool run_compatibility_phase(struct char_data *character, unsigned int phase)
{
#ifdef LUMINARI_CUTEST
  if (test_phase_callback != NULL)
    return test_phase_callback(character, phase, test_phase_context);
#endif
  return combat_run_compatibility_phase(character, phase);
}

static void prepare_semantic_round(struct combat_encounter_data *encounter)
{
  struct combat_encounter_participant *participant;

  for (participant = encounter->participants; participant != NULL;
       participant = participant->next)
  {
    if (!participant->active || participant->pending_activation ||
        participant->character == NULL)
      continue;
    if (encounter_bus != NULL &&
        domain_event_resolve(encounter_bus, participant->character_handle,
                             DOMAIN_ENTITY_CHARACTER) != participant->character)
      continue;
    participant->perfect_tempo_was_hit =
        (participant->round_flags &
         ((uint32_t)1U << COMBAT_ENCOUNTER_ROUND_PERFECT_TEMPO_HIT)) != 0U;
    participant->round_flags = 0U;
    participant->reactions_used = 0;
    GET_TOTAL_AOO(participant->character) = 0;
  }
}

static bool run_semantic_round(struct combat_encounter_participant *participant)
{
  participant->turns_started++;
  participant->intent_dispatched = false;
  announce_recovered_actions(participant);
#ifdef LUMINARI_CUTEST
  if (test_phase_callback != NULL)
    return test_phase_callback(participant->character, 0U, test_phase_context);
#endif
  return combat_run_semantic_round(participant->character,
                                   participant->perfect_tempo_was_hit);
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
      encounter->due_head->next_due <= (uint64_t)pulse)
  {
    prepare_semantic_round(encounter);
    semantic_round_started = true;
  }
  do
  {
    while (encounter->due_head != NULL &&
           encounter->due_head->next_due <= (uint64_t)pulse)
    {
      participant = encounter->due_head;
      due_remove(encounter, participant);
      if (encounter_bus != NULL &&
          domain_event_resolve(encounter_bus, participant->character_handle,
                               DOMAIN_ENTITY_CHARACTER) != participant->character)
      {
        event_note_stale_owner_outcome();
        counter_increment(&cumulative_stats.stale_encounter_callbacks);
        participant->active = false;
        participant->departing = true;
        continue;
      }
      participant->dispatching = true;
      if (semantic_rounds)
      {
        completed = run_semantic_round(participant);
        counter_increment(&cumulative_stats.semantic_turns_resolved);
      }
      else
      {
        counter_increment(&cumulative_stats.compatibility_attempts);
        completed = run_compatibility_phase(participant->character, participant->phase);
      }
      participant->dispatching = false;
      if (!semantic_rounds)
      {
        if (completed)
          counter_increment(&cumulative_stats.compatibility_phases);
        else
          counter_increment(&cumulative_stats.compatibility_terminal);
      }
      if (participant->active && participant->encounter == encounter && completed)
      {
        if (!semantic_rounds)
          participant->phase = participant->phase < 3U ? participant->phase + 1U : 1U;
        participant->next_due =
            (uint64_t)pulse +
            (semantic_rounds ? COMBAT_ENCOUNTER_ROUND_DELAY : COMBAT_ENCOUNTER_PHASE_DELAY);
        participant->due_sequence = allocate_due_sequence();
        due_insert(encounter, participant);
      }
      else if (participant->active && participant->encounter == encounter)
      {
        participant->active = false;
        participant->departing = true;
      }
    }
    compact_inactive_participants(encounter);
    apply_pending_additions(encounter);
    merged = apply_pending_merges(encounter);
  } while (merged);
  if (semantic_rounds && semantic_round_started)
  {
    counter_increment(&encounter->semantic_round);
    counter_increment(&cumulative_stats.semantic_rounds_resolved);
    encounter->next_round_due = (uint64_t)pulse + COMBAT_ENCOUNTER_ROUND_DELAY;
    align_semantic_participants(encounter, encounter->next_round_due);
  }
  else if (!semantic_rounds)
  {
    encounter->compatibility_phase = encounter->compatibility_phase < 3U
                                         ? encounter->compatibility_phase + 1U
                                         : 1U;
    if (encounter->compatibility_phase == 1U)
      counter_increment(&encounter->compatibility_round);
  }
  maybe_end_encounter(encounter);
  encounter->resolving = false;
  if (encounter->terminal || encounter->due_head == NULL)
  {
    destroy_encounter(encounter, true);
    return game_event_result_complete();
  }
  if (semantic_rounds)
    delay = encounter->next_round_due > (uint64_t)pulse
                ? encounter->next_round_due - (uint64_t)pulse
                : 1U;
  else
    delay = encounter->due_head->next_due > (uint64_t)pulse
                ? encounter->due_head->next_due - (uint64_t)pulse
                : 1U;
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
    log("SYSERR: unable to register native event type 'combat.encounter.round' (status %d).",
        status);
    return false;
  }
  return true;
}

static struct event_runtime_handle
create_round_event(struct combat_encounter_data *encounter, uint64_t delay)
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
  if (event_runtime_schedule_owned_after(encounter_round_event_type, owner,
                                         (game_tick_t)delay, payload,
                                         &handle) != GAME_SCHEDULER_OK)
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
  if (reason != COMBAT_ENCOUNTER_DEPARTURE_DIED &&
      reason != COMBAT_ENCOUNTER_DEPARTURE_EXTRACTED)
    restore_semantic_action_events(participant);
  participant->active = false;
  participant->pending_activation = false;
  participant->departing = true;
  due_remove(encounter, participant);
  character->combat_encounter = NULL;
  character->combat_encounter_participant = NULL;
  counter_increment(&cumulative_stats.participants_left);
  if (reason >= COMBAT_ENCOUNTER_DEPARTURE_STOPPED &&
      reason < COMBAT_ENCOUNTER_DEPARTURE_COUNT)
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

bool combat_encounter_action_query(struct char_data *character, action_type action,
                                   bool *available)
{
  struct combat_encounter_participant *participant;

  participant = semantic_participant(character);
  if (participant == NULL || available == NULL || action < atSTANDARD || action > atSWIFT)
    return false;
  *available = participant->action_ready_turn[action] <= participant->turns_started;
  return true;
}

bool combat_encounter_action_consume(struct char_data *character, action_type action,
                                     int duration)
{
  struct combat_encounter_participant *participant;
  uint64_t ready_turn;

  participant = semantic_participant(character);
  if (participant == NULL || action < atSTANDARD || action > atSWIFT)
    return false;
  ready_turn = participant->turns_started + semantic_rounds_for_delay(duration);
  participant->action_ready_turn[action] = MAX(participant->action_ready_turn[action], ready_turn);
  participant->action_notice_pending[action] = true;
  counter_increment(&cumulative_stats.action_budgets_spent);
  return true;
}

bool combat_encounter_intent_claim(struct char_data *character)
{
  struct combat_encounter_participant *participant;

  participant = semantic_participant(character);
  if (participant == NULL)
    return true;
  if (!participant->dispatching || participant->intent_dispatched)
  {
    counter_increment(&cumulative_stats.intent_dispatch_blocks);
    return false;
  }
  participant->intent_dispatched = true;
  counter_increment(&cumulative_stats.intents_dispatched);
  return true;
}

bool combat_encounter_reaction_try_use(struct char_data *character, unsigned int limit,
                                       bool *managed)
{
  struct combat_encounter_participant *participant;

  if (managed != NULL)
    *managed = false;
  participant = semantic_participant(character);
  if (participant == NULL)
    return false;
  if (managed != NULL)
    *managed = true;
  if (participant->reactions_used >= 0 &&
      (unsigned int)participant->reactions_used >= limit)
    return false;
  participant->reactions_used++;
  GET_TOTAL_AOO(character) = participant->reactions_used;
  counter_increment(&cumulative_stats.reactions_spent);
  return true;
}

bool combat_encounter_reaction_refund(struct char_data *character)
{
  struct combat_encounter_participant *participant;

  participant = semantic_participant(character);
  if (participant == NULL)
    return false;
  if (participant->reactions_used > INT_MIN)
    participant->reactions_used--;
  GET_TOTAL_AOO(character) = participant->reactions_used;
  return true;
}

bool combat_encounter_round_flag_query(struct char_data *character,
                                       enum combat_encounter_round_flag flag, bool *used)
{
  struct combat_encounter_participant *participant;

  participant = semantic_participant(character);
  if (participant == NULL || used == NULL || flag < COMBAT_ENCOUNTER_ROUND_PERFECT_TEMPO_HIT ||
      flag >= COMBAT_ENCOUNTER_ROUND_FLAG_COUNT)
    return false;
  *used = (participant->round_flags & ((uint32_t)1U << flag)) != 0U;
  return true;
}

bool combat_encounter_round_flag_mark(struct char_data *character,
                                      enum combat_encounter_round_flag flag)
{
  struct combat_encounter_participant *participant;

  participant = semantic_participant(character);
  if (participant == NULL || flag < COMBAT_ENCOUNTER_ROUND_PERFECT_TEMPO_HIT ||
      flag >= COMBAT_ENCOUNTER_ROUND_FLAG_COUNT)
    return false;
  participant->round_flags |= (uint32_t)1U << flag;
  return true;
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

static void handle_character_died(const struct domain_event_context *context,
                                  void *handler_context)
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
      {DOMAIN_EVENT_CHARACTER_MOVED, "combat-encounter-character-moved", 20,
       handle_character_moved, NULL},
      {DOMAIN_EVENT_CHARACTER_DIED, "combat-encounter-character-died", 20,
       handle_character_died, NULL},
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
    encounter_slots[index].next_free = index + 1U < COMBAT_ENCOUNTER_MAX_ACTIVE
                                           ? (uint32_t)(index + 1U)
                                           : UINT32_MAX;
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
  log("Combat round scheduling: %s.",
      !encounter_mode ? "legacy character events"
                      : semantic_rounds ? "encounter-owned six-second semantic rounds"
                                        : "encounter-owned compatibility phases");
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
      cumulative_stats.compatibility_attempts !=
          cumulative_stats.compatibility_phases + cumulative_stats.compatibility_terminal)
    stats->compatibility_mismatches++;
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
