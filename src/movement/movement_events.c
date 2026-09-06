/**************************************************************************
 *  File: movement_events.c                            Part of LuminariMUD *
 *  Usage: Post-movement event processing.                                *
 *                                                                         *
 *  All rights reserved.  See license for complete information.           *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "domain_event_world.h"
#include "db.h"
#include "magic/spells.h"
#include "obj/house.h"
#include "constants.h"
#include "dgscript/dg_scripts.h"
#include "act.h"
#include "combat/fight.h"
#include "mud_event.h"
#include "quest/hlquest.h"
#include "mudlim.h"
#include "wilderness/wilderness.h"
#include "actions.h"
#include "combat/traps.h"
#include "magic/spell_prep.h"
#include "character/class.h"
#include "vessels/transport.h"
#include "combat/encounters.h"
#include "quest/hunts.h"
#include "character/feats.h"

/* Include movement system header */
#include "movement.h"
#include "movement_events.h"

/* External functions */
int perform_detecttrap(struct char_data *ch, bool silent);
void quest_room(struct char_data *ch);
void set_expire_cooldown(room_rnum room);
void reset_expire_cooldown(room_rnum room);
bool is_road_room(room_rnum room, int type);
void check_hunt_room(room_rnum room);
void check_random_encounter(struct char_data *ch);
bool is_covered(struct char_data *ch);

/**
 * Process all post-movement events
 * This is called after a character has successfully moved to a new room
 *
 * @param ch Character who moved
 * @param was_in Room they moved from
 * @param going_to Room they moved to
 * @param dir Direction they moved
 * @return TRUE if movement succeeds, FALSE if an event prevents it
 */
bool process_movement_events(struct char_data *ch, room_rnum was_in, room_rnum going_to, int dir)
{
  int riding = RIDING(ch) ? 1 : 0;
  int ridden_by = RIDDEN_BY(ch) ? 1 : 0;
  int same_room = 0;
  bool accepted;
  struct domain_entity_handle identity = domain_event_character_handle(ch);

  /* Check mount/rider same room status */
  if (riding && RIDING(ch)->in_room == ch->in_room)
    same_room = 1;
  else if (ridden_by && RIDDEN_BY(ch)->in_room == ch->in_room)
    same_room = 1;

  /* Process room damage effects */
  process_room_damage(ch, going_to, riding, same_room);
  ch = domain_event_world_resolve_character(identity);
  if (ch == NULL || IN_ROOM(ch) != going_to)
    return FALSE;

  /* Fire memory and greet triggers */
  entry_memory_mtrigger(ch);
  ch = domain_event_world_resolve_character(identity);
  if (ch == NULL || IN_ROOM(ch) != going_to)
    return FALSE;

  accepted = greet_mtrigger(ch, dir);
  ch = domain_event_world_resolve_character(identity);
  if (ch == NULL || IN_ROOM(ch) != going_to)
    return FALSE;
  if (!accepted)
  {
    /* Greet trigger prevents entry - send them back */
    char_from_room(ch);

    if (ZONE_FLAGGED(GET_ROOM_ZONE(was_in), ZONE_WILDERNESS))
    {
      X_LOC(ch) = world[was_in].coords[0];
      Y_LOC(ch) = world[was_in].coords[1];
    }

    char_to_room(ch, was_in);
    look_at_room(ch, 0);

    return FALSE;
  }
  else
  {
    greet_memory_mtrigger(ch);
    ch = domain_event_world_resolve_character(identity);
    if (ch == NULL || IN_ROOM(ch) != going_to)
      return FALSE;
  }

  /* Handle NPC quest rooms */
  if (IS_NPC(ch))
    quest_room(ch);
  ch = domain_event_world_resolve_character(identity);
  if (ch == NULL || IN_ROOM(ch) != going_to)
    return FALSE;

  /* Check for traps */
  process_trap_detection(ch);
  ch = domain_event_world_resolve_character(identity);
  if (ch == NULL || IN_ROOM(ch) != going_to)
    return FALSE;

  /* Process wilderness events */
  process_wilderness_events(ch, was_in);
  ch = domain_event_world_resolve_character(identity);
  if (ch == NULL || IN_ROOM(ch) != going_to)
    return FALSE;

  /* Process vampire weaknesses */
  process_vampire_weaknesses(ch);
  ch = domain_event_world_resolve_character(identity);
  if (ch == NULL || IN_ROOM(ch) != going_to)
    return FALSE;

  return TRUE;
}

/**
 * Process room damage effects like spike growth
 *
 * @param ch Character entering the room
 * @param room Room being entered
 * @param riding TRUE if mounted
 * @param same_room TRUE if mount is in same room
 */
void process_room_damage(struct char_data *ch, room_rnum room, int riding, int same_room)
{
  struct domain_entity_handle actor = domain_event_character_handle(ch);
  struct domain_entity_handle victim;
  struct char_data *target;
  int i;
  static const int effects[] = {RAFF_SPIKE_STONES, RAFF_SPIKE_GROWTH};
  static const int spells[] = {SPELL_SPIKE_STONES, SPELL_SPIKE_GROWTH};
  static const int dice_count[] = {4, 2};

  for (i = 0; i < 2; i++)
  {
    ch = domain_event_world_resolve_character(actor);
    if (ch == NULL || IN_ROOM(ch) != room)
      return;
    if (!ROOM_AFFECTED(room, effects[i]))
      continue;
    target = riding && same_room && RIDING(ch) != NULL ? RIDING(ch) : ch;
    victim = domain_event_character_handle(target);
    damage(target, target, dice(dice_count[i], 4), spells[i], DAM_EARTH, FALSE);
    target = domain_event_world_resolve_character(victim);
    if (target != NULL)
      send_to_char(target, "You are impaled by large %sspikes as you enter the room.\r\n",
                   i == 0 ? "stone " : "");
  }
}

/**
 * Process trap detection for characters with trap sense
 *
 * @param ch Character to check for trap detection
 */
void process_trap_detection(struct char_data *ch)
{
  /* Trap sense feat allows automatic detection */
  if (!IS_NPC(ch))
  {
    int trap_check = 0;
    int dc = 21;

    if ((trap_check = HAS_FEAT(ch, FEAT_TRAP_SENSE)))
    {
      if (skill_check(ch, ABILITY_PERCEPTION, (dc - trap_check)))
        perform_detecttrap(ch, TRUE); /* silent */
    }
  }

  /* Room entry traps removed - use autosearch system instead */
  /* Players can enable autosearch for automatic trap detection */
}

/**
 * Process wilderness-specific events
 *
 * @param ch Character who moved
 * @param was_in Previous room
 */
void process_wilderness_events(struct char_data *ch, room_rnum was_in)
{
  if (!ch->master)
  {
    /* Set cooldown timer on old wilderness room */
    if (was_in != NOWHERE && ZONE_FLAGGED(GET_ROOM_ZONE(was_in), ZONE_WILDERNESS))
    {
      set_expire_cooldown(was_in);
    }

    /* Check for encounters and hunts */
    if (ZONE_FLAGGED(GET_ROOM_ZONE(ch->in_room), ZONE_WILDERNESS))
    {
      check_random_encounter(ch);
      reset_expire_cooldown(ch->in_room);
      check_hunt_room(ch->in_room);
    }
  }
}

/**
 * Process vampire weakness effects
 *
 * @param ch Character to check for vampire weaknesses
 */
void process_vampire_weaknesses(struct char_data *ch)
{
  /* Skip if not a vampire or protected */
  if (!HAS_FEAT(ch, FEAT_VAMPIRE_WEAKNESSES))
    return;

  if (GET_LEVEL(ch) >= LVL_IMMORT)
    return;

  if (affected_by_spell(ch, AFFECT_RECENTLY_DIED))
    return;

  if (affected_by_spell(ch, AFFECT_RECENTLY_RESPECED))
    return;

  /* Sunlight damage */
  if (IN_SUNLIGHT(ch) && !is_covered(ch))
  {
    damage(ch, ch, dice(1, 6), TYPE_SUN_DAMAGE, DAM_SUNLIGHT, FALSE);
  }

  /* Moving water damage */
  if (IN_MOVING_WATER(ch))
  {
    damage(ch, ch, GET_MAX_HIT(ch) / 3, TYPE_MOVING_WATER, DAM_MOVING_WATER, FALSE);
  }
}

/**
 * Process pre-movement trap checks
 *
 * @param ch Character leaving a room
 */
void process_leave_traps(struct char_data *ch)
{
  check_trap(ch, TRAP_TRIGGER_LEAVE_ROOM, ch->in_room, 0, 0);
}
