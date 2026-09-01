#ifndef COMBAT_COMBAT_ENCOUNTERS_H
#define COMBAT_COMBAT_ENCOUNTERS_H

#include "domain_events.h"
#include "structs.h"

enum combat_encounter_departure_reason
{
  COMBAT_ENCOUNTER_DEPARTURE_STOPPED = 0,
  COMBAT_ENCOUNTER_DEPARTURE_MOVED,
  COMBAT_ENCOUNTER_DEPARTURE_FLED,
  COMBAT_ENCOUNTER_DEPARTURE_TELEPORTED,
  COMBAT_ENCOUNTER_DEPARTURE_DIED,
  COMBAT_ENCOUNTER_DEPARTURE_EXTRACTED,
  COMBAT_ENCOUNTER_DEPARTURE_DISCONNECTED,
  COMBAT_ENCOUNTER_DEPARTURE_ADMINISTRATIVE,
  COMBAT_ENCOUNTER_DEPARTURE_COUNT
};

enum combat_encounter_round_flag
{
  COMBAT_ENCOUNTER_ROUND_PERFECT_TEMPO_HIT = 0,
  COMBAT_ENCOUNTER_ROUND_DEFLECTIVE_SCREEN_USED,
  COMBAT_ENCOUNTER_ROUND_SMASH_DEFENSE_USED,
  COMBAT_ENCOUNTER_ROUND_RELENTLESS_ASSAULT_USED,
  COMBAT_ENCOUNTER_ROUND_FLAG_COUNT
};

struct combat_encounter_stats
{
  bool initialized;
  bool encounter_mode;
  bool semantic_rounds;
  size_t active_encounters;
  size_t active_participants;
  size_t scheduled_events;
  uint64_t high_water_encounters;
  uint64_t high_water_participants;
  uint64_t encounters_created;
  uint64_t encounters_ended;
  uint64_t participants_joined;
  uint64_t participants_left;
  uint64_t encounters_merged;
  uint64_t encounter_callbacks;
  uint64_t compatibility_attempts;
  uint64_t compatibility_phases;
  uint64_t compatibility_terminal;
  uint64_t compatibility_mismatches;
  uint64_t semantic_rounds_resolved;
  uint64_t semantic_turns_resolved;
  uint64_t intents_dispatched;
  uint64_t intent_dispatch_blocks;
  uint64_t action_budgets_spent;
  uint64_t reactions_spent;
  uint64_t admission_failures;
  uint64_t stale_encounter_callbacks;
  uint64_t departure_counts[COMBAT_ENCOUNTER_DEPARTURE_COUNT];
};

struct combat_encounter_initiative_entry
{
  struct char_data *character;
  int initiative;
};

struct combat_encounter_initiative_snapshot
{
  uint64_t round_number;
  uint64_t pulses_until_round;
  size_t total_participants;
  size_t entry_count;
  bool semantic_rounds;
};

enum domain_event_status combat_encounter_runtime_init(struct domain_event_bus *bus);
void combat_encounter_runtime_shutdown(void);
bool combat_encounter_events_enabled(void);
bool combat_encounter_semantic_rounds_enabled(void);
bool combat_encounter_semantic_manages(const struct char_data *character);
bool combat_encounter_join(struct char_data *character, struct char_data *opponent,
                           long initial_delay);
void combat_encounter_leave(struct char_data *character,
                            enum combat_encounter_departure_reason reason);
void combat_encounter_forget_character(struct char_data *character,
                                       enum combat_encounter_departure_reason reason);
void combat_encounter_get_stats(struct combat_encounter_stats *stats);
bool combat_encounter_get_initiative(
    const struct char_data *viewer, struct combat_encounter_initiative_entry *entries,
    size_t capacity, struct combat_encounter_initiative_snapshot *snapshot);
bool combat_encounter_action_query(struct char_data *character, action_type action,
                                   bool *available);
bool combat_encounter_action_consume(struct char_data *character, action_type action,
                                     int duration);
bool combat_encounter_intent_claim(struct char_data *character);
bool combat_encounter_reaction_try_use(struct char_data *character, unsigned int limit,
                                       bool *managed);
bool combat_encounter_reaction_refund(struct char_data *character);
bool combat_encounter_round_flag_query(struct char_data *character,
                                       enum combat_encounter_round_flag flag, bool *used);
bool combat_encounter_round_flag_mark(struct char_data *character,
                                      enum combat_encounter_round_flag flag);

#ifdef LUMINARI_CUTEST
typedef bool (*combat_encounter_test_phase_callback)(struct char_data *character,
                                                     unsigned int phase, void *context);
void combat_encounter_test_select(bool encounter_mode);
void combat_encounter_test_select_semantic(bool semantic_rounds);
void combat_encounter_test_set_phase_callback(combat_encounter_test_phase_callback callback,
                                              void *context);
#endif

#endif /* COMBAT_COMBAT_ENCOUNTERS_H */
