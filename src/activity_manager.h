#ifndef ACTIVITY_MANAGER_H
#define ACTIVITY_MANAGER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "act.h"
#include "domain_events.h"
#include "structs.h"

enum primary_activity_type
{
  PRIMARY_ACTIVITY_NONE = 0,
  PRIMARY_ACTIVITY_CAMP,
  PRIMARY_ACTIVITY_TEST,
  PRIMARY_ACTIVITY_TYPE_COUNT
};

enum primary_activity_state
{
  PRIMARY_ACTIVITY_STATE_NONE = 0,
  PRIMARY_ACTIVITY_STATE_ACTIVE,
  PRIMARY_ACTIVITY_STATE_PAUSED,
  PRIMARY_ACTIVITY_STATE_COMPLETED,
  PRIMARY_ACTIVITY_STATE_CANCELLED
};

enum primary_activity_capability
{
  PRIMARY_ACTIVITY_CAP_MOVEMENT = (1U << 0),
  PRIMARY_ACTIVITY_CAP_HANDS = (1U << 1),
  PRIMARY_ACTIVITY_CAP_ATTENTION = (1U << 2),
  PRIMARY_ACTIVITY_CAP_VISION = (1U << 3),
  PRIMARY_ACTIVITY_CAP_SPEECH = (1U << 4),
  PRIMARY_ACTIVITY_CAP_STANDARD = (1U << 5),
  PRIMARY_ACTIVITY_CAP_MOVE = (1U << 6),
  PRIMARY_ACTIVITY_CAP_SWIFT = (1U << 7),
  PRIMARY_ACTIVITY_CAP_IMMEDIATE = (1U << 8)
};

enum primary_activity_trait
{
  PRIMARY_ACTIVITY_TRAIT_STATIONARY = (1U << 0),
  PRIMARY_ACTIVITY_TRAIT_DISTRACTED = (1U << 1),
  PRIMARY_ACTIVITY_TRAIT_HANDS_OCCUPIED = (1U << 2),
  PRIMARY_ACTIVITY_TRAIT_FINE_MANIPULATION = (1U << 3),
  PRIMARY_ACTIVITY_TRAIT_OBVIOUS = (1U << 4)
};

enum primary_activity_progress_model
{
  PRIMARY_ACTIVITY_PROGRESS_ATOMIC = 0,
  PRIMARY_ACTIVITY_PROGRESS_PROGRESSIVE,
  PRIMARY_ACTIVITY_PROGRESS_CONTINUOUS
};

enum primary_activity_progress_owner
{
  PRIMARY_ACTIVITY_PROGRESS_CHARACTER = 0,
  PRIMARY_ACTIVITY_PROGRESS_TARGET
};

enum primary_activity_response
{
  PRIMARY_ACTIVITY_RESPONSE_IGNORE = 0,
  PRIMARY_ACTIVITY_RESPONSE_CANCEL,
  PRIMARY_ACTIVITY_RESPONSE_PAUSE,
  PRIMARY_ACTIVITY_RESPONSE_DELAY,
  PRIMARY_ACTIVITY_RESPONSE_RECHECK,
  PRIMARY_ACTIVITY_RESPONSE_REJECT
};

enum primary_activity_end_reason
{
  PRIMARY_ACTIVITY_END_COMPLETED = 0,
  PRIMARY_ACTIVITY_END_PLAYER_CANCELLED,
  PRIMARY_ACTIVITY_END_MOVED,
  PRIMARY_ACTIVITY_END_DIED,
  PRIMARY_ACTIVITY_END_EXTRACTED,
  PRIMARY_ACTIVITY_END_TARGET_LOST,
  PRIMARY_ACTIVITY_END_RECHECK_FAILED,
  PRIMARY_ACTIVITY_END_COMMAND,
  PRIMARY_ACTIVITY_END_SHUTDOWN,
  PRIMARY_ACTIVITY_END_INTERNAL
};

typedef bool (*primary_activity_recheck)(struct char_data *actor, void *target, void *context);
typedef void (*primary_activity_progress)(struct char_data *actor, void *target,
                                          uint32_t completed_steps, uint32_t total_steps,
                                          void *context);
typedef void (*primary_activity_completion)(struct char_data *actor, void *target, void *context);
typedef void (*primary_activity_ended)(struct char_data *actor,
                                       enum primary_activity_end_reason reason, void *context);
typedef void (*primary_activity_context_cleanup)(void *context);

struct primary_activity_definition
{
  enum primary_activity_type type;
  const char *display_name;
  uint32_t capabilities;
  uint32_t traits;
  enum primary_activity_progress_model progress_model;
  enum primary_activity_progress_owner progress_owner;
  uint32_t total_steps;
  long step_interval;
  int combat_actions_required;
  enum primary_activity_response movement_response;
  enum primary_activity_response damage_response;
  enum primary_activity_response combat_response;
  enum primary_activity_response target_loss_response;
  enum primary_activity_response command_response;
  long delay_pulses;
  primary_activity_recheck recheck;
  primary_activity_progress progress;
  primary_activity_completion complete;
  primary_activity_ended ended;
  primary_activity_context_cleanup cleanup_context;
  void *context;
};

struct primary_activity_snapshot
{
  enum primary_activity_type type;
  enum primary_activity_state state;
  char display_name[64];
  uint32_t capabilities;
  uint32_t traits;
  uint32_t completed_steps;
  uint32_t total_steps;
  long next_step_pulses;
  bool combat_clock;
};

struct primary_activity_stats
{
  size_t active;
  size_t high_water;
  uint64_t started;
  uint64_t completed;
  uint64_t cancelled;
  uint64_t paused;
  uint64_t resumed;
  uint64_t delayed;
  uint64_t rejected_commands;
  uint64_t stale_callbacks;
};

enum domain_event_status primary_activity_manager_init(struct domain_event_bus *bus);
void primary_activity_manager_shutdown(void);
bool primary_activity_start(struct char_data *actor, struct domain_entity_handle target,
                            const struct primary_activity_definition *definition);
bool primary_activity_cancel(struct char_data *actor, enum primary_activity_end_reason reason,
                             bool notify);
bool primary_activity_pause(struct char_data *actor, bool notify);
bool primary_activity_resume(struct char_data *actor, bool notify);
bool primary_activity_command_admit(struct char_data *actor, const char *command,
                                    uint32_t command_capabilities, bool informational,
                                    bool activity_control);
void primary_activity_on_semantic_turn(struct char_data *actor);
void primary_activity_forget_character(struct char_data *actor);
bool primary_activity_snapshot(const struct char_data *actor,
                               struct primary_activity_snapshot *snapshot);
void primary_activity_get_stats(struct primary_activity_stats *stats);
bool primary_activity_feature_enabled(enum primary_activity_type type);
const char *primary_activity_state_name(enum primary_activity_state state);
const char *primary_activity_end_reason_name(enum primary_activity_end_reason reason);

ACMD_DECL(do_activity);

#ifdef LUMINARI_CUTEST
void primary_activity_test_select_camp(bool managed);
bool primary_activity_test_camp_value_is_managed(const char *value);
#endif

#endif /* ACTIVITY_MANAGER_H */
