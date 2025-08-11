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