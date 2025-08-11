/**************************************************************************
 *  File: movement_validation.c                       Part of LuminariMUD *
 *  Usage: Movement validation functions                                  *
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
#include "fight.h"
#include "feats.h"
#include "mud_event.h"
#include "actions.h"
#include "class.h"
#include "spec_procs.h"
#include "movement_validation.h"

/* Functions moved from movement.c */

#define ZONE_MINLVL(rnum) (zone_table[(rnum)].min_level)

int has_boat(struct char_data *ch, room_rnum going_to)
{
  struct obj_data *obj;
  int i;
  int skill_roll, skill_val;

  if (GET_LEVEL(ch) >= LVL_IMMORT)
    return (1);

  if (AFF_FLAGGED(ch, AFF_WATERWALK) || is_flying(ch) ||
      AFF_FLAGGED(ch, AFF_LEVITATE))
    return (1);

  if (RIDING(ch))
  {
    if (AFF_FLAGGED(RIDING(ch), AFF_WATERWALK))
      return TRUE;
    if (AFF_FLAGGED(RIDING(ch), AFF_LEVITATE))
      return TRUE;
  }

  /* non-wearable boats in inventory will do it */
  for (obj = ch->carrying; obj; obj = obj->next_content)
    if (GET_OBJ_TYPE(obj) == ITEM_BOAT && (find_eq_pos(ch, obj, NULL) < 0))
      return (1);

  /* and any boat you're wearing will do it too */
  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i) && GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_BOAT)
      return (1);

  // they can't swim here, so no need for a skill check.
  if (SECT(going_to) == SECT_WATER_NOSWIM || SECT(going_to) == SECT_UD_WATER_NOSWIM || SECT(going_to) == SECT_UD_NOSWIM)
    return 0;

  /* we should do a swim check now */
  int swim_dc = 13 + ZONE_MINLVL(GET_ROOM_ZONE(going_to));
  skill_roll = d20(ch);
  skill_val = compute_ability(ch, ABILITY_ATHLETICS);
  send_to_char(ch, "Swimming: Athletics Skill (%d) + d20 roll (%d) = Total (%d) vs. Swim DC (%d) ", skill_val, skill_roll, skill_val + skill_roll, swim_dc);
  if ((skill_roll + skill_val) < swim_dc)
  {
    send_to_char(ch, "You attempt to swim, but fail!\r\n");
    USE_MOVE_ACTION(ch);
    return 0;
  }
  else
  { /*success!*/
    send_to_char(ch, "You successfully swim!: ");
    return 1;
  }

  return (0);
}

/* Simple function to determine if char can fly. */
int has_flight(struct char_data *ch)
{
  struct obj_data *obj;
  int i;

  if (GET_LEVEL(ch) >= LVL_IMMORT)
    return (1);

  if (is_flying(ch))
    return (1);

  /* Non-wearable flying items in inventory will do it. */
  for (obj = ch->carrying; obj; obj = obj->next_content)
    if (OBJAFF_FLAGGED(obj, AFF_FLYING) && (find_eq_pos(ch, obj, NULL) < 0))
      return (1);

  /* Any equipped objects with AFF_FLYING will do it too. */
  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i) && OBJAFF_FLAGGED(GET_EQ(ch, i), AFF_FLYING))
      return (1);

  return (0);
}

/* Simple function to determine if char can scuba. */
int has_scuba(struct char_data *ch, room_rnum destination)
{
  struct obj_data *obj;
  int i;

  if (GET_LEVEL(ch) >= LVL_IMMORT)
    return (1);

  if (AFF_FLAGGED(ch, AFF_SCUBA))
    return (1);

  /* Non-wearable scuba items in inventory will do it. */
  for (obj = ch->carrying; obj; obj = obj->next_content)
    if (OBJAFF_FLAGGED(obj, AFF_SCUBA) && (find_eq_pos(ch, obj, NULL) < 0))
      return (1);

  /* Any equipped objects with AFF_SCUBA will do it too. */
  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i) && OBJAFF_FLAGGED(GET_EQ(ch, i), AFF_SCUBA))
      return (1);

  if (IS_SET_AR(ROOM_FLAGS(destination), ROOM_AIRY))
    return (1);

  return (0);
}

/* Simple function to determine if char can climb */
int can_climb(struct char_data *ch)
{

  struct obj_data *obj;
  int i;

  if (GET_LEVEL(ch) >= LVL_IMMORT)
    return (1);

  if (has_flight(ch))
    return 1;

  if (AFF_FLAGGED(ch, AFF_CLIMB))
    return (1);

  if (affected_by_spell(ch, SPELL_SPIDER_CLIMB))
    return (1);

  /* Non-wearable 'climb' items in inventory will do it. */
  for (obj = ch->carrying; obj; obj = obj->next_content)
    if (OBJAFF_FLAGGED(obj, AFF_CLIMB) && (find_eq_pos(ch, obj, NULL) < 0))
      return (1);

  /* Any equipped objects with AFF_CLIMB will do it too. */
  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i) && OBJAFF_FLAGGED(GET_EQ(ch, i), AFF_CLIMB))
      return (1);

  /* we should do a climb check now since everything else failed */
  int climb_dc = 10 + movement_loss[SECT(IN_ROOM(ch))];
  int skill_roll = d20(ch);
  int skill_val = compute_ability(ch, ABILITY_ATHLETICS);
  send_to_char(ch, "Climbing: Athletics Skill (%d) + d20 roll (%d) = Total (%d) vs. Climb DC (%d) ", skill_val, skill_roll, skill_val + skill_roll, climb_dc);
  if ((skill_roll + skill_val) < climb_dc)
  {
    send_to_char(ch, "You attempt to climb, but fail!\r\n");
    act("$n attempts to climb, but fails.", TRUE, ch, 0, 0, TO_ROOM);
    
    // they fell, check for damage.
    int acro_dc = 10 + movement_loss[SECT(IN_ROOM(ch))];
    skill_roll = d20(ch);
    skill_val = compute_ability(ch, ABILITY_ACROBATICS);
    send_to_char(ch, "Falling! Acrobatics Skill (%d) + d20 roll (%d) = Total (%d) vs. Climb DC (%d) ", skill_val, skill_roll, skill_val + skill_roll, climb_dc);
    if ((skill_roll + skill_val) < acro_dc)
    {
      send_to_char(ch, "You're falling... ");
      act("$n begins to fall.", TRUE, ch, 0, 0, TO_ROOM);
      int dam = movement_loss[SECT(IN_ROOM(ch))];
      dam = dice(3, dam);
      dam -= HAS_FEAT(ch, FEAT_SLOW_FALL) * 3;

      if (affected_by_spell(ch, PSIONIC_CATFALL))
      {
        send_to_char(ch, "Your psionic catfall ability allows you to float down gently.\r\n");
        act("$n falls down gently.", TRUE, ch, 0, 0, TO_ROOM);
      }
      else if (dam <= 0)
      {
        send_to_char(ch, "Your ability to 'slow fall' allows you to float down more gently.\r\n");
        act("$n falls down gently.", TRUE, ch, 0, 0, TO_ROOM);
      }
      else
      {
        send_to_char(ch, "You fall and take %d damage! %s\r\n", dam, 
                     HAS_FEAT(ch, FEAT_SLOW_FALL) ? "Your slow fall ability has reduced the damage." : "");
        act("$n falls down hard!", TRUE, ch, 0, 0, TO_ROOM);
        damage(ch, ch, dam, AFFECT_FALLING_DAMAGE, DAM_BLUDGEON, FALSE);
      }
    }
    else
    {
      send_to_char(ch, "You manage to catch yourself before falling!\r\n");
      act("$n manages to catch $mself before falling.", TRUE, ch, 0, 0, TO_ROOM);
    }
    USE_MOVE_ACTION(ch);
    return 0;
  }
  else
  { /*success!*/
    send_to_char(ch, "You successfully climb!\r\n");
    act("$n climbs the incline.", TRUE, ch, 0, 0, TO_ROOM);
    return 1;
  }

  return (0);
}