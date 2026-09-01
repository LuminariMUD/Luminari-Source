/**************************************************************************
 *  File: mob_act.h                                   Part of LuminariMUD *
 *  Usage: Mobile agenda behavior execution and legacy rollback           *
 *                                                                         *
 *  All rights reserved.  See license for complete information.           *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.              *
 **************************************************************************/

#ifndef _MOB_ACT_H_
#define _MOB_ACT_H_

#include <stdint.h>

/* Include all mob module headers */
#include "mob_memory.h"
#include "mob_utils.h"
#include "mob_race.h"
#include "mob_psionic.h"
#include "mob_class.h"
#include "mob_spells.h"

/* Concrete autonomous-work reasons used by owner-local agendas. */
typedef uint32_t mobile_work_mask;

enum mobile_work_reason
{
  MOBILE_WORK_NONE = 0U,
  MOBILE_WORK_SPEC_ACTIVITY = (1U << 0),
  MOBILE_WORK_ECHO = (1U << 1),
  MOBILE_WORK_SCAVENGE = (1U << 2),
  MOBILE_WORK_PATROL = (1U << 3),
  MOBILE_WORK_HUNT = (1U << 4),
  MOBILE_WORK_WANDER = (1U << 5),
  MOBILE_WORK_POSTURE = (1U << 6),
  MOBILE_WORK_ROOM_REACTION = (1U << 7),
  MOBILE_WORK_COMBAT_REACTION = (1U << 8),
  MOBILE_WORK_RESOURCE_RECOVERY = (1U << 9)
};

#define MOBILE_WORK_RECURRING_MASK                                                         \
  (MOBILE_WORK_SPEC_ACTIVITY | MOBILE_WORK_ECHO | MOBILE_WORK_SCAVENGE |                  \
   MOBILE_WORK_PATROL | MOBILE_WORK_HUNT | MOBILE_WORK_WANDER | MOBILE_WORK_POSTURE |      \
   MOBILE_WORK_RESOURCE_RECOVERY)
#define MOBILE_WORK_REACTION_MASK (MOBILE_WORK_ROOM_REACTION | MOBILE_WORK_COMBAT_REACTION)
#define MOBILE_WORK_FIXED_CADENCE_MASK                                                     \
  (MOBILE_WORK_RECURRING_MASK & ~(MOBILE_WORK_WANDER | MOBILE_WORK_RESOURCE_RECOVERY))

#if (defined(LUMINARI_ENABLE_EVENT_ROLLBACK) && LUMINARI_ENABLE_EVENT_ROLLBACK) ||                 \
    defined(LUMINARI_EVENT_ROLLBACK_TESTS)
void mobile_activity_run_legacy_cycle(void);
#endif
void mobile_activity_run_one(struct char_data *ch);
void mobile_activity_run_scheduled(struct char_data *ch, mobile_work_mask reasons);
mobile_work_mask mobile_activity_recurring_reasons(struct char_data *ch);
mobile_work_mask mobile_activity_room_reaction_reasons(const struct char_data *ch);
mobile_work_mask mobile_activity_combat_reaction_reasons(const struct char_data *ch);
long mobile_activity_next_wander_delay(void);
long mobile_activity_next_resource_recovery_delay(const struct char_data *ch);
#if (defined(LUMINARI_ENABLE_EVENT_ROLLBACK) && LUMINARI_ENABLE_EVENT_ROLLBACK) ||                 \
    defined(LUMINARI_EVENT_ROLLBACK_TESTS)
void mobile_activity_run_legacy_slice(int heart_pulse);
#endif
void mobile_activity_reset(void);
void mobile_activity_forget_character(struct char_data *ch);

#endif /* _MOB_ACT_H_ */
