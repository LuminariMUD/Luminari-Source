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

struct combat_encounter_stats
{
  bool initialized;
  bool encounter_mode;
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
  uint64_t admission_failures;
  uint64_t stale_encounter_callbacks;
  uint64_t departure_counts[COMBAT_ENCOUNTER_DEPARTURE_COUNT];
};

enum domain_event_status combat_encounter_runtime_init(struct domain_event_bus *bus);
void combat_encounter_runtime_shutdown(void);
bool combat_encounter_events_enabled(void);
bool combat_encounter_join(struct char_data *character, struct char_data *opponent,
                           long initial_delay);
void combat_encounter_leave(struct char_data *character,
                            enum combat_encounter_departure_reason reason);
void combat_encounter_forget_character(struct char_data *character,
                                       enum combat_encounter_departure_reason reason);
void combat_encounter_get_stats(struct combat_encounter_stats *stats);

#ifdef LUMINARI_CUTEST
typedef bool (*combat_encounter_test_phase_callback)(struct char_data *character,
                                                     unsigned int phase, void *context);
void combat_encounter_test_select(bool encounter_mode);
void combat_encounter_test_set_phase_callback(combat_encounter_test_phase_callback callback,
                                              void *context);
#endif

#endif /* COMBAT_COMBAT_ENCOUNTERS_H */
