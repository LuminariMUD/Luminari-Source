/**************************************************************************
 *  File: movement_cost.c                             Part of LuminariMUD *
 *  Usage: Movement cost and speed calculation functions                  *
 *                                                                         *
 *  All rights reserved.  See license complete information.               *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.              *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "constants.h"
#include "act.h"
#include "class.h"
#include "feats.h"
#include "race.h"
#include "assign_wpn_armor.h"
#include "movement_cost.h"

/* Functions moved from movement.c */

int get_speed(struct char_data *ch, sbyte to_display)
{

  if (!ch) return 30;

  int speed = 30;

  if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_MOUNTABLE))
    speed = 50;

  // if mounted, we'll use the mount's speed instead.
  if (RIDING(ch))
    return get_speed(RIDING(ch), to_display);

  if (!IS_NPC(ch))
  {
    switch (GET_RACE(ch))
    {
    case RACE_DWARF:
    case RACE_DUERGAR:
    case RACE_CRYSTAL_DWARF:
    case RACE_HALFLING:
    case RACE_GNOME:
      speed = 25;
      break;
    }
  }

  if (is_flying(ch))
  {
    if (HAS_FEAT(ch, FEAT_FAE_FLIGHT))
      speed = 60;
    else
      speed = 50;
  }

  // yes, 400 is intentional :)
  if (affected_by_spell(ch, SPELL_SHADOW_WALK))
    speed = 400;

  // haste and exp. retreat don't stack for balance reasons
  if (AFF_FLAGGED(ch, AFF_HASTE))
    speed += 30;
  else if (affected_by_spell(ch, SPELL_EXPEDITIOUS_RETREAT))
    speed += 30;

  // likewise, monk speed and fast movement don't stack for balance reasons
  if (monk_gear_ok(ch))
    speed += MIN(60, CLASS_LEVEL(ch, CLASS_MONK) / 3 * 10);
  else if (HAS_FEAT(ch, FEAT_FAST_MOVEMENT))
    if (compute_gear_armor_type(ch) <= ARMOR_TYPE_MEDIUM || affected_by_spell(ch, SPELL_EFFORTLESS_ARMOR))
      speed += 10;

  if (affected_by_spell(ch, SPELL_GREASE))
    speed -= 10;

  if (affected_by_spell(ch, AFFECT_RALLYING_CRY))
    speed += 5;

  // if they're slowed, it's half regardless.  Same with entangled.
  // if they're blind, they can make an acrobatics check against dc 10
  // to avoid halving their speed, but we only want to do this is the
  // function is called to apply their speed.  If to_display is true,
  // we won't worry about the blind effect, because it's only showing
  // the person's base speed for display purposes (ie. score)
  if (AFF_FLAGGED(ch, AFF_SLOW))
    speed /= 2;
  else if (AFF_FLAGGED(ch, AFF_ENTANGLED))
    speed /= 2;
  else if (!to_display && AFF_FLAGGED(ch, AFF_BLIND) && skill_roll(ch, ABILITY_ACROBATICS) < 10)
    speed /= 2;
  else if (affected_by_spell(ch, PSIONIC_DECELERATION))
    speed /= 2;
  else if (affected_by_spell(ch, PSIONIC_OAK_BODY))
    speed /= 2;
  else if (affected_by_spell(ch, PSIONIC_BODY_OF_IRON))
    speed /= 2;

  return speed;
}

/**
 * Calculate the movement point cost for moving from one room to another
 * 
 * @param ch Character who is moving
 * @param from_room Room moving from
 * @param to_room Room moving to
 * @param riding TRUE if character is mounted
 * @return Movement point cost
 */
int calculate_movement_cost(struct char_data *ch, room_rnum from_room, room_rnum to_room, int riding)
{
  int need_movement;
  
  /* Special case: woodland stride reduces outdoor movement to 1 */
  if (OUTDOORS(ch) && HAS_FEAT(riding ? RIDING(ch) : ch, FEAT_WOODLAND_STRIDE))
  {
    need_movement = 1;
  }
  else
  {
    /* Average movement cost between source and destination */
    need_movement = (movement_loss[SECT(from_room)] + movement_loss[SECT(to_room)]) / 2;
  }

  /* Fast movement feat reduces cost */
  if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_FAST_MOVEMENT))
    need_movement--;

  /* Roads reduce movement cost */
  if (ROOM_FLAGGED(from_room, ROOM_ROAD))
    need_movement--;

  /* Difficult terrain doubles cost */
  if (ROOM_AFFECTED(to_room, RAFF_DIFFICULT_TERRAIN))
    need_movement *= 2;
    
  /* Spot mode doubles cost */
  if (AFF_FLAGGED(ch, AFF_SPOT))
    need_movement *= 2;
    
  /* Listen mode doubles cost */
  if (AFF_FLAGGED(ch, AFF_LISTEN))
    need_movement *= 2;
    
  /* Reclining quadruples cost */
  if (GET_POS(ch) <= POS_RECLINING)
    need_movement *= 4;

  /* New movement system multiplier */
  need_movement *= 10;

  /* Skill-based reduction */
  int skill_bonus = skill_roll(ch, riding ? MAX(ABILITY_RIDE, ABILITY_SURVIVAL) : ABILITY_SURVIVAL);
  if (SECT(to_room) == SECT_HILLS || SECT(to_room) == SECT_MOUNTAIN || SECT(to_room) == SECT_HIGH_MOUNTAIN)
    skill_bonus += skill_roll(ch, ABILITY_CLIMB) / 2;

  need_movement -= skill_bonus;

  /* Speed modification */
  int speed_mod = get_speed(riding ? RIDING(ch) : ch, FALSE);
  speed_mod = speed_mod - 30;
  need_movement -= speed_mod;

  /* Minimum movement cost */
  if (affected_by_spell(riding ? RIDING(ch) : ch, SPELL_SHADOW_WALK))
    need_movement = MAX(1, need_movement);
  else
    need_movement = MAX(5, need_movement);

  return need_movement;
}

/**
 * Check if character has enough movement points
 * 
 * @param ch Character to check
 * @param need_movement Required movement points
 * @param riding TRUE if mounted
 * @param following TRUE if following someone
 * @return TRUE if has enough movement, FALSE otherwise
 */
bool check_movement_points(struct char_data *ch, int need_movement, int riding, int following)
{
  if (riding)
  {
    /* Mounts currently don't use movement points */
    return TRUE;
  }
  else
  {
    if (GET_MOVE(ch) < need_movement && !IS_NPC(ch))
    {
      if (following && ch->master)
        send_to_char(ch, "You are too exhausted to follow.\r\n");
      else
        send_to_char(ch, "You are too exhausted.\r\n");

      if (GET_WALKTO_LOC(ch))
      {
        send_to_char(ch, "You stop walking to the '%s' landmark.\r\n", 
                     get_walkto_landmark_name(walkto_vnum_to_list_row(GET_WALKTO_LOC(ch))));
        GET_WALKTO_LOC(ch) = 0;
      }

      return FALSE;
    }
  }
  
  return TRUE;
}

/**
 * Deduct movement points from character
 * 
 * @param ch Character to deduct from
 * @param need_movement Amount to deduct
 * @param riding TRUE if mounted
 * @param ridden_by TRUE if being ridden
 */
void deduct_movement_points(struct char_data *ch, int need_movement, int riding, int ridden_by)
{
  if (GET_LEVEL(ch) < LVL_IMMORT && !IS_NPC(ch) && !(riding || ridden_by))
    GET_MOVE(ch) -= need_movement;
  /* Artificial inflation of mount movement points */
  else if (riding && !rand_number(0, 9))
    GET_MOVE(RIDING(ch)) -= need_movement;
  else if (ridden_by)
    GET_MOVE(RIDDEN_BY(ch)) -= need_movement;
}