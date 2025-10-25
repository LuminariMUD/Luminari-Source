/**************************************************************************
 *  File: movement.c                                   Part of LuminariMUD *
 *  Usage: Movement commands, door handling, & sleep/rest/etc state.       *
 *                                                                         *
 *  All rights reserved.  See license complete information.                *
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
#include "db.h"
#include "spells.h"
#include "house.h"
#include "constants.h"
#include "dg_scripts.h"
#include "act.h"
#include "fight.h"
#include "oasis.h" /* for buildwalk */
#include "spec_procs.h"
#include "mud_event.h"
#include "hlquest.h"
#include "mudlim.h"
#include "wilderness.h" /* Wilderness! */
#include "actions.h"
#include "traps.h" /* for check_traps() */
#include "spell_prep.h"
/* trails.h merged into movement_tracks.h */
#include "assign_wpn_armor.h"
#include "encounters.h"
#include "hunts.h"
#include "class.h"
#include "transport.h"
#include "routing.h"

/* Include movement system header */
#include "movement.h"

#define ZONE_MINLVL(rnum) (zone_table[(rnum)].min_level)


/***** start file body *****/

/* doorbash - unfinished */
/*
ACMD(do_doorbash) {
  bool failure = FALSE;
  int door;
  struct room_direction_data *back = 0;
  int other_room;
  struct obj_data *obj = 0;

  if (!INN_FLAGGED(ch, INNATE_DOORBASH)) {
    send_to_char("But you are way too small to attempt that.\r\n", ch);
    return;
  }
  one_argument(argument, arg);

  if (!*arg) {
    send_to_char("Doorbash which direction?\r\n", ch);
    return;
  }
  door = search_block(arg, dirs, FALSE);
  if (door < 0) {
    send_to_char("That is not a direction!\r\n", ch);
    return;
  }

  if (!EXIT(ch, door) || EXIT_FLAGGED(EXIT(ch, door), EX_HIDDEN)) {
    send_to_char("There is nothing to doorbash in that direction.\r\n", ch);
    return;
  }

  if (!EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED)) {
    send_to_char("But that direction does not need to be doorbashed.\r\n", ch);
    return;
  }

  if ((other_room = EXIT(ch, door)->to_room) != NOWHERE) {
    back = world[other_room].dir_option[rev_dir[door]];
    if (back && back->to_room != ch->in_room)
      back = 0;
  }

  if (EXIT_FLAGGED(EXIT(ch, door), EX_PICKPROOF) || dice(1, 300) > GET_R_STR(ch) + GET_LEVEL(ch)
          || EXIT_FLAGGED(EXIT(ch, door), EX_LOCKED2) || EXIT_FLAGGED(EXIT(ch, door), EX_LOCKED3)
          )
    failure = TRUE;

  act("$n charges straight into the door with $s entire body.", FALSE, ch, 0, 0, TO_ROOM);
  act("You throw your entire body at the door.", FALSE, ch, 0, 0, TO_CHAR);

  if (failure) {
    act("But it holds steady against the onslaught.", FALSE, ch, 0, 0, TO_ROOM);
    act("But it holds steady against the onslaught.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  act("and it shatters into a million pieces!!", FALSE, ch, 0, 0, TO_ROOM);
  act("and it shatters into a million pieces!!.", FALSE, ch, 0, 0, TO_CHAR);

  UNLOCK_DOOR(ch->in_room, obj, door);
  OPEN_DOOR(ch->in_room, obj, door);
  if (back) {
    UNLOCK_DOOR(other_room, obj, rev_dir[door]);
    OPEN_DOOR(other_room, obj, rev_dir[door]);
    REMOVE_BIT(back->exit_info, EX_HIDDEN);
  }
  WAIT_STATE(ch, 1 * PULSE_VIOLENCE);

  check_trap(ch, TRAP_TRIGGER_OPEN_DOOR, ch->in_room, 0, door);

  return;
}
 */


/** Move a PC/NPC character from their current location to a new location. This
 * is the standard movement locomotion function that all normal walking
 * movement by characters should be sent through. This function also defines
 * the move cost of normal locomotion as:
 * ( (move cost for source room) + (move cost for destination) ) / 2
 *
 * @pre Function assumes that ch has no master controlling character, that
 * ch has no followers (in other words followers won't be moved by this
 * function) and that the direction traveled in is one of the valid, enumerated
 * direction.
 * @param ch The character structure to attempt to move.
 * @param dir The defined direction (NORTH, SOUTH, etc...) to attempt to
 * move into.
 * @param need_specials_check If TRUE will cause
 * @retval int 1 for a successful move (ch is now in a new location)
 * or 0 for a failed move (ch is still in the original location). */
int do_simple_move(struct char_data *ch, int dir, int need_specials_check)
{
  /* Begin Local variable definitions */
  /*---------------------------------------------------------------------*/
  /* Used in our special proc check. By default, we pass a NULL argument
   * when checking for specials */
  char spec_proc_args[MAX_INPUT_LENGTH] = {'\0'};
  /* The room the character is currently in and will move from... */
  room_rnum was_in = NOWHERE;
  /* ... and the room the character will move into. */
  room_rnum going_to = NOWHERE;
  /* How many movement points are required to travel from was_in to going_to.
   * We redefine this later when we need it. */
  int need_movement = 0;
  /* NOTE: tch, next_tch, and buf2 were used in old message code - now handled in movement_messages.c */
  // for mount code
  int same_room = 0, riding = 0, ridden_by = 0;
  /* singlefile variables */
  struct char_data *other;
  struct char_data **prev;
  bool was_top = TRUE;

  /* Wilderness variables */
  int new_x = 0, new_y = 0;

  /* added some dummy checks to deal with a fairly mysterious crash */
  if (!ch)
    return 0;

  if (IN_ROOM(ch) == NOWHERE)
    return 0;

  if (dir < 0 || dir >= NUM_OF_DIRS)
    return 0;

  /* dummy check, if you teleport while in a falling event, BOOM otherwise :P */
  if (!EXIT(ch, dir))
    return 0;

  /* The following is to support the wilderness code. */
  if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_WILDERNESS) && (EXIT(ch, dir)->to_room == real_room(1000000)))
  {
    new_x = X_LOC(ch); // world[IN_ROOM(ch)].coords[0];
    new_y = Y_LOC(ch); // world[IN_ROOM(ch)].coords[1];

    /* This is a wilderness movement!  Find out which coordinates we need
     * to check, based on the dir and local coordinates. */
    switch (dir)
    {
    case NORTH:
      new_y++;
      break;
    case SOUTH:
      new_y--;
      break;
    case EAST:
      new_x++;
      break;
    case WEST:
      new_x--;
      break;
    default:
      /* Bad direction for wilderness travel.*/
      return 0;
    }
    going_to = find_room_by_coordinates(new_x, new_y);
    if (going_to == NOWHERE)
    {
      going_to = find_available_wilderness_room();
      if (going_to == NOWHERE)
      {
        log("SYSERR: Wilderness movement failed from (%d, %d) to (%d, %d)", X_LOC(ch), Y_LOC(ch), new_x, new_y);
        return 0;
      }
      /* Must set the coords, etc in the going_to room. */

      assign_wilderness_room(going_to, new_x, new_y);
    }
  }
  else if (world[IN_ROOM(ch)].dir_option[dir])
  {

    going_to = EXIT(ch, dir)->to_room;

    /* Since we are in non-wilderness moving to wilderness, set up the coords. */
    if (ZONE_FLAGGED(GET_ROOM_ZONE(going_to), ZONE_WILDERNESS))
    {
      new_x = world[going_to].coords[0];
      new_y = world[going_to].coords[1];
    }
  }

  if (going_to == NOWHERE)
    return 0;

  was_in = IN_ROOM(ch);
  /* end dummy checks */

  /*---------------------------------------------------------------------*/
  /* End Local variable definitions */

  /* Begin checks that can prevent a character from leaving the was_in room. */
  /* Future checks should be implemented within this section and return 0.   */
  /*---------------------------------------------------------------------*/
  /* Check for special routines that might activate because of the move and
   * also might prevent the movement. Special requires commands, so we pass
   * in the "command" equivalent of the direction (ie. North is '1' in the
   * command list, but NORTH is defined as '0').
   * Note -- only check if following; this avoids 'double spec-proc' bug */
  if (need_specials_check && special(ch, dir + 1, spec_proc_args))
    return 0;

  /* Leave Trigger Checks: Does a leave trigger block exit from the room? */
  /* next 3 if blocks prevent teleport crashes */
  if (!leave_mtrigger(ch, dir) || IN_ROOM(ch) != was_in)
    return 0;
  if (!leave_wtrigger(&world[IN_ROOM(ch)], ch, dir) || IN_ROOM(ch) != was_in)
    return 0;
  if (!leave_otrigger(&world[IN_ROOM(ch)], ch, dir) || IN_ROOM(ch) != was_in)
    return 0;

  if (AFF_FLAGGED(ch, AFF_GRAPPLED) || AFF_FLAGGED(ch, AFF_ENTANGLED))
  {
    send_to_char(ch, "You struggle to move but you are unable to leave the area!\r\n");
    act("$n struggles to move, but can't!", FALSE, ch, 0, 0, TO_ROOM);
    return 0;
  }
  
  /* Crippled characters have a 50% chance to fail movement */
  if (AFF_FLAGGED(ch, AFF_CRIPPLED))
  {
    if (rand_number(1, 100) <= 50)
    {
      send_to_char(ch, "Your crippled legs fail you and you stumble!\r\n");
      act("$n tries to move but stumbles on $s crippled legs!", FALSE, ch, 0, 0, TO_ROOM);
      return 0;
    }
  }
  
  if (affected_by_spell(ch, SKILL_DEFENSIVE_STANCE) &&
      !HAS_FEAT(ch, FEAT_MOBILE_DEFENSE))
  {
    send_to_char(ch, "You can't move while in defensive stance!\r\n");
    return 0;
  }

  /* check if they're mounted */
  if (RIDING(ch))
    riding = 1;
  if (RIDDEN_BY(ch))
    ridden_by = 1;

  /* if they're mounted, are they in the same room w/ their mount(ee)? */
  if (riding && RIDING(ch)->in_room == ch->in_room)
    same_room = 1;
  else if (ridden_by && RIDDEN_BY(ch)->in_room == ch->in_room)
    same_room = 1;

  /* tamed mobiles cannot move about */
  if (ridden_by && same_room && AFF_FLAGGED(ch, AFF_TAMED))
  {
    send_to_char(ch, "You've been tamed.  Now act it!\r\n");
    return 0;
  }

  /* begin singlefile mechanic */
  if (ROOM_FLAGGED(ch->in_room, ROOM_SINGLEFILE))
  {
    other = get_char_ahead_of_me(ch, dir);
    if (other && RIDING(other) != ch && RIDDEN_BY(other) != ch)
    {
      if (GET_POS(other) == POS_RECLINING)
      {
        was_top = is_top_of_room_for_singlefile(ch, dir);
        prev = &world[ch->in_room].people;
        while (*prev)
        {
          if (*prev == ch)
          {
            *prev = ch->next_in_room;
            ch->next_in_room = 0;
          }
          if (*prev)
            prev = &((*prev)->next_in_room);
        }

        prev = &world[ch->in_room].people;
        if (was_top)
        {
          while (*prev)
          {
            if (*prev == other)
            {
              *prev = ch;
              ch->next_in_room = other;
            }
            prev = &((*prev)->next_in_room);
          }
        }
        else
        {
          ch->next_in_room = other->next_in_room;
          other->next_in_room = ch;
        }
        act("You squeeze by the prone body of $N.", FALSE, ch, 0, other, TO_CHAR);
        act("$n squeezes by YOU.", FALSE, ch, 0, other, TO_VICT);
        act("$n squeezes by the prone body of $N.", FALSE, ch, 0, other, TO_NOTVICT);
        return 0;
      }
      else if (GET_POS(ch) == POS_RECLINING && GET_POS(other) >= POS_FIGHTING && FIGHTING(ch) != other && FIGHTING(other) != ch)
      {
        was_top = is_top_of_room_for_singlefile(ch, dir);
        prev = &world[ch->in_room].people;
        while (*prev)
        {
          if (*prev == ch)
          {
            *prev = ch->next_in_room;
            ch->next_in_room = 0;
          }
          if (*prev)
            prev = &((*prev)->next_in_room);
        }

        prev = &world[ch->in_room].people;
        if (was_top)
        {
          while (*prev)
          {
            if (*prev == other)
            {
              *prev = ch;
              ch->next_in_room = other;
            }
            prev = &((*prev)->next_in_room);
          }
        }
        else
        {
          ch->next_in_room = other->next_in_room;
          other->next_in_room = ch;
        }
        act("You crawl by $N.", FALSE, ch, 0, other, TO_CHAR);
        act("$n crawls by YOU.", FALSE, ch, 0, other, TO_VICT);
        act("$n crawls by $N.", FALSE, ch, 0, other, TO_NOTVICT);
        return 0;
      }
      else
      {
        act("You bump into $N.", FALSE, ch, 0, other, TO_CHAR);
        return 0;
      }
    }
  }
  /* end singlefile mechanic */

  /* Charm effect: Does it override the movement?
     for now it is cut out of the code */

  // dummy check
  if (going_to == NOWHERE)
    return 0;

  /* druid spell */
  if (IS_EVIL(ch) && IS_HOLY(going_to))
  {
    send_to_char(ch, "The sanctity of the area prevents "
                     "you from entering.\r\n");
    return 0;
  }
  if (IS_GOOD(ch) && IS_UNHOLY(going_to))
  {
    send_to_char(ch, "The corruption of the area prevents "
                     "you from entering.\r\n");
    return 0;
  }

  /* Water, No Swimming Rooms: Does the deep water prevent movement? */
  if ((SECT(was_in) == SECT_WATER_NOSWIM) || (SECT(was_in) == SECT_UD_NOSWIM) ||
      (SECT(going_to) == SECT_WATER_NOSWIM) || (SECT(going_to) == SECT_UD_NOSWIM))
  {
    if ((riding && !has_boat(RIDING(ch), going_to)) || !has_boat(ch, going_to))
    {
      send_to_char(ch, "You need a boat to go there.\r\n");
      if (GET_WALKTO_LOC(ch))
      {
        send_to_char(ch, "You stop walking to the '%s' landmark.\r\n", get_walkto_landmark_name(walkto_vnum_to_list_row(GET_WALKTO_LOC(ch))));
        GET_WALKTO_LOC(ch) = 0;
      }
      return (0);
    }
  }

  if (SECT(was_in) == SECT_WATER_SWIM || SECT(was_in) == SECT_UD_WATER || SECT(was_in) == SECT_UNDERWATER ||
      SECT(going_to) == SECT_WATER_SWIM || SECT(going_to) == SECT_UD_WATER || SECT(going_to) == SECT_UNDERWATER)
  {
    if ((riding && !has_boat(RIDING(ch), going_to)) || !has_boat(ch, going_to))
    {
      if (GET_MOVE(ch) < 20)
      {
        send_to_char(ch, "You don't have the energy to try and swim there.\r\n");
        return 0;
      }
      send_to_char(ch, "You try to swim there, but fail.\r\n");
      GET_MOVE(ch) -= 20;
      return (0);
    }
  }

  /* Flying Required: Does lack of flying prevent movement? */
  if ((SECT(was_in) == SECT_FLYING) || (SECT(going_to) == SECT_FLYING) ||
      (SECT(was_in) == SECT_UD_NOGROUND) || (SECT(going_to) == SECT_UD_NOGROUND))
  {
    if (char_has_mud_event(ch, eFALLING))
      ; /* zusuk's cheesy falling code */
    else if ((riding && !has_flight(RIDING(ch))) || !has_flight(ch))
    {
      send_to_char(ch, "You need to be flying to go there!\r\n");
      return (0);
    }
  }

  /* Underwater Room: Does lack of underwater breathing prevent movement? */
  if ((SECT(was_in) == SECT_UNDERWATER) ||
      (SECT(going_to) == SECT_UNDERWATER))
  {
    if (!has_scuba(ch, going_to) && (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_NOHASSLE)))
    {
      send_to_char(ch,
                   "You need to be able to breathe water to go there!\r\n");
      return (0);
    }
  }

  /* High Mountain (and any other climb rooms) */
  if ((SECT(was_in) == SECT_HIGH_MOUNTAIN) ||
      (SECT(going_to) == SECT_HIGH_MOUNTAIN))
  {
    if ((riding && !can_climb(RIDING(ch))) || !can_climb(ch))
    {
      send_to_char(ch, "You need to be able to climb to go there!\r\n");
      if (GET_WALKTO_LOC(ch))
      {
        send_to_char(ch, "You stop walking to the '%s' landmark.\r\n", get_walkto_landmark_name(walkto_vnum_to_list_row(GET_WALKTO_LOC(ch))));
        GET_WALKTO_LOC(ch) = 0;
      }
      return (0);
    }
  }

  /* Ocean restriction */
  if (SECT(was_in) == SECT_OCEAN || SECT(going_to) == SECT_OCEAN)
  {
    if (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_NOHASSLE))
    {
      send_to_char(ch, "You need an ocean-ready ship to go there!\r\n");
      return (0);
    }
  }

  /* flight restricted to enter that room */
  if (ROOM_FLAGGED(going_to, ROOM_NOFLY) && is_flying(ch))
  {
    send_to_char(ch, "It is not possible to fly in that direction\r\n");
    return 0;
  }

  /* size restricted to enter that room */
  if (ROOM_FLAGGED(going_to, ROOM_SIZE_TINY) && GET_SIZE(ch) > SIZE_TINY)
  {
    send_to_char(ch, "You'd have to be tiny or smaller to go there.\r\n");
    return 0;
  }
  if (ROOM_FLAGGED(going_to, ROOM_SIZE_DIMINUTIVE) && GET_SIZE(ch) > SIZE_DIMINUTIVE)
  {
    send_to_char(ch, "You'd have to be diminutive or smaller to go there.\r\n");
    return 0;
  }

  /* Houses: Can the player walk into the house? */
  if (ROOM_FLAGGED(was_in, ROOM_ATRIUM))
  {
    if (!House_can_enter(ch, GET_ROOM_VNUM(going_to)))
    {
      send_to_char(ch, "That's private property -- no trespassing!\r\n");
      return (0);
    }
  }

  /* Check zone flag restrictions */
  if (ZONE_FLAGGED(GET_ROOM_ZONE(going_to), ZONE_CLOSED))
  {
    send_to_char(ch, "A mysterious barrier forces you back! That area is "
                     "off-limits.\r\n");
    return (0);
  }

  if (ZONE_FLAGGED(GET_ROOM_ZONE(going_to), ZONE_NOIMMORT) &&
      (GET_LEVEL(ch) >= LVL_IMMORT) && (GET_LEVEL(ch) < LVL_GRSTAFF))
  {
    send_to_char(ch, "A mysterious barrier forces you back! That area is off-limits.\r\n");
    return (0);
  }

  /* Room Size Capacity: Is the room full of people already? */
  if (riding && ROOM_FLAGGED(going_to, ROOM_TUNNEL))
  {
    send_to_char(ch, "There isn't enough space to enter mounted!\r\n");
    return 0;
  }

  if (ROOM_FLAGGED(going_to, ROOM_TUNNEL) &&
      num_pc_in_room(&(world[going_to])) >= CONFIG_TUNNEL_SIZE)
  {
    if (CONFIG_TUNNEL_SIZE > 1)
      send_to_char(ch, "There isn't enough room for you to go there!\r\n");
    else
      send_to_char(ch, "There isn't enough room there for more than"
                       " one person!\r\n");
    return (0);
  }

  /* Room Level Requirements: Is ch privileged enough to enter the room? */
  if (ROOM_FLAGGED(going_to, ROOM_STAFFROOM) && GET_LEVEL(ch) < LVL_STAFF)
  {
    send_to_char(ch, "You aren't godly enough to use that room!\r\n");
    return (0);
  }

  /* climb is needed to get to going_to, skill required is based on min-level of zone */
  if (ROOM_FLAGGED(going_to, ROOM_CLIMB_NEEDED))
  {
    /* do a climb check */
    int climb_dc = 13 + ZONE_MINLVL(GET_ROOM_ZONE(going_to));
    send_to_char(ch, "Climb DC: %d - ", climb_dc);
    if (!skill_check(ch, ABILITY_CLIMB, climb_dc) && !AFF_FLAGGED(ch, AFF_FLYING))
    {
      send_to_char(ch, "You attempt to climb to that area, but fall and get hurt!\r\n");
      damage((ch), (ch), dice(climb_dc - 10, 4), -1, -1, -1);
      update_pos(ch);
      return 0;
    }
    else if (AFF_FLAGGED(ch, AFF_FLYING))
    {
      send_to_char(ch, "You fly up beyond the steep obstacle.: ");
    }
    else
    { /*success!*/
      send_to_char(ch, "You successfully climb!: ");
    }
  }

  /* Check for leave traps using modular function */
  process_leave_traps(ch);

  /* check for magical walls, such as wall of force (also death from wall damage) */
  if (check_wall(ch, dir)) /* true = wall stopped ch somehow */
    return (0);

  /* a silly zusuk dummy check */
  update_pos(ch);
  if (GET_POS(ch) <= POS_STUNNED)
  {
    send_to_char(ch, "You are in no condition to move!\r\n");
    return (0);
  }

  /* Check zone level recommendations */
  if (GET_LEVEL(ch) <= NEWBIE_LEVEL && (ZONE_MINLVL(GET_ROOM_ZONE(going_to)) != -1) &&
      ZONE_MINLVL(GET_ROOM_ZONE(going_to)) > GET_LEVEL(ch))
  {
    send_to_char(ch, "(OOC)  This zone is above your recommended level.\r\n");
  }

  struct char_data *mob;
  sbyte block = FALSE;

  for (mob = world[IN_ROOM(ch)].people; mob; mob = mob->next_in_room)
  {
    if (!IS_NPC(mob))
      continue;

    if (dir == NORTH && MOB_FLAGGED(mob, MOB_BLOCK_N))
      block = TRUE;
    else if (dir == EAST && MOB_FLAGGED(mob, MOB_BLOCK_E))
      block = TRUE;
    else if (dir == SOUTH && MOB_FLAGGED(mob, MOB_BLOCK_S))
      block = TRUE;
    else if (dir == WEST && MOB_FLAGGED(mob, MOB_BLOCK_W))
      block = TRUE;
    else if (dir == NORTHEAST && MOB_FLAGGED(mob, MOB_BLOCK_NE))
      block = TRUE;
    else if (dir == SOUTHEAST && MOB_FLAGGED(mob, MOB_BLOCK_SE))
      block = TRUE;
    else if (dir == SOUTHWEST && MOB_FLAGGED(mob, MOB_BLOCK_SW))
      block = TRUE;
    else if (dir == NORTHWEST && MOB_FLAGGED(mob, MOB_BLOCK_NW))
      block = TRUE;
    else if (dir == UP && MOB_FLAGGED(mob, MOB_BLOCK_U))
      block = TRUE;
    else if (dir == DOWN && MOB_FLAGGED(mob, MOB_BLOCK_D))
      block = TRUE;

    if (block && MOB_FLAGGED(mob, MOB_BLOCK_RACE) && GET_RACE(ch) == GET_RACE(mob))
      block = FALSE;
    if (block && MOB_FLAGGED(mob, MOB_BLOCK_CLASS) && GET_CLASS(ch) == GET_CLASS(mob))
      block = FALSE;
    // if (block && MOB_FLAGGED(mob, MOB_BLOCK_RACE_FAMILY) && race_list[GET_RACE(ch)].family == race_list[GET_RACE(mob)].family)
    //   block = FALSE;
    if (block && MOB_FLAGGED(mob, MOB_BLOCK_LEVEL) && GET_LEVEL(ch) > GET_LEVEL(mob))
      block = FALSE;
    if (block && MOB_FLAGGED(mob, MOB_BLOCK_ALIGN) && IS_GOOD(ch) > IS_GOOD(mob))
      block = FALSE;
    if (block && MOB_FLAGGED(mob, MOB_BLOCK_ALIGN) && IS_EVIL(ch) > IS_EVIL(mob))
      block = FALSE;
    if (block && MOB_FLAGGED(mob, MOB_BLOCK_ALIGN) && IS_NEUTRAL(ch) > IS_NEUTRAL(mob))
      block = FALSE;
    if (block && MOB_FLAGGED(mob, MOB_BLOCK_EVIL) && (IS_NEUTRAL(ch) || IS_GOOD(ch)))
      block = FALSE;
    if (block && MOB_FLAGGED(mob, MOB_BLOCK_NEUTRAL) && (IS_EVIL(ch) || IS_GOOD(ch)))
      block = FALSE;
    if (block && MOB_FLAGGED(mob, MOB_BLOCK_GOOD) && (IS_NEUTRAL(ch) || IS_EVIL(ch)))
      block = FALSE;

    if (block)
      break;
  }

  if (block && !IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_NOHASSLE))
  {
    act("$N blocks your from travelling in that direction.", FALSE, ch, 0, mob, TO_CHAR);
    act("$n tries to leave the room, but $N blocks $m from travelling in their direction.", FALSE, ch, 0, mob, TO_ROOM);
    if (GET_WALKTO_LOC(ch))
    {
      send_to_char(ch, "You stop walking to the '%s' landmark.\r\n", get_walkto_landmark_name(walkto_vnum_to_list_row(GET_WALKTO_LOC(ch))));
      GET_WALKTO_LOC(ch) = 0;
    }
    
    return 0;
  }


  /* fleeing: no retreat feat offers free AOO */
  if (need_specials_check == 3 && GET_POS(ch) > POS_DEAD)
  {
    /* loop room */
    struct char_data *tch, *next_tch;
    for (tch = world[IN_ROOM(ch)].people; tch; tch = next_tch)
    {
      next_tch = tch->next_in_room; /* next value in linked list */

      if (FIGHTING(tch) && FIGHTING(tch) == ch && HAS_FEAT(tch, FEAT_NO_RETREAT))
      {
        attack_of_opportunity(tch, ch, 4);
      }
    }
  }

  /* All checks passed, nothing will prevent movement now other than lack of
   * move points. */
  /* move points needed is avg. move loss for src and destination sect type */
  if (OUTDOORS(ch) && HAS_FEAT(riding ? RIDING(ch) : ch, FEAT_WOODLAND_STRIDE))
  {
    need_movement = 1;
  }
  else
  {
    need_movement = (movement_loss[SECT(was_in)] + movement_loss[SECT(going_to)]) / 2;
  }

  if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_FAST_MOVEMENT))
    need_movement--;

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_ROAD))
    need_movement--;

  if (ROOM_AFFECTED(going_to, RAFF_DIFFICULT_TERRAIN))
    need_movement *= 2;
  /* if in "spot-mode" double cost of movement */
  if (AFF_FLAGGED(ch, AFF_SPOT))
    need_movement *= 2;
  /* if in "listen-mode" double cost of movement */
  if (AFF_FLAGGED(ch, AFF_LISTEN))
    need_movement *= 2;
  /* if reclined quadruple movement cost */
  if (GET_POS(ch) <= POS_RECLINING)
    need_movement *= 4;

  // new movement system requires we multiply this
  need_movement *= 10;

  // Now let's reduce based on skills, since this can
  // be a fraction of 10.

  int skill_bonus = skill_roll(ch, riding ? MAX(ABILITY_RIDE, ABILITY_SURVIVAL) : ABILITY_SURVIVAL);
  if (SECT(going_to) == SECT_HILLS || SECT(going_to) == SECT_MOUNTAIN || SECT(going_to) == SECT_HIGH_MOUNTAIN)
    skill_bonus += skill_roll(ch, ABILITY_CLIMB) / 2;

  need_movement -= skill_bonus;

  // we're using a base speed of 30.  If it's 30, then speed won't have an effect on
  // movement.  If the speed is less than 30, it'll cost more movement.  If it's more
  // it will reduce the movement points required.
  // if we're mounted, we'll use the mount's speed instead.
  int speed_mod = get_speed(riding ? RIDING(ch) : ch, FALSE);
  speed_mod = speed_mod - 30;
  need_movement -= speed_mod;

  // regardless of bonuses, we'll never use less than 5 moves per room, unless using shadow walk
  if (affected_by_spell(riding ? RIDING(ch) : ch, SPELL_SHADOW_WALK))
    need_movement = MAX(1, need_movement);
  else
    need_movement = MAX(5, need_movement);

  /* Move Point Requirement Check */
  if (riding)
  {
    /* do NOT touch this until we re-evaluate the movement system and NPC movements points
       -zusuk */
    need_movement = 0;
  }
  else
  {
    if (GET_MOVE(ch) < need_movement && !IS_NPC(ch))
    {
      if (need_specials_check && ch->master)
        send_to_char(ch, "You are too exhausted to follow.\r\n");
      else
        send_to_char(ch, "You are too exhausted.\r\n");

      if (GET_WALKTO_LOC(ch))
      {
        send_to_char(ch, "You stop walking to the '%s' landmark.\r\n", get_walkto_landmark_name(walkto_vnum_to_list_row(GET_WALKTO_LOC(ch))));
        GET_WALKTO_LOC(ch) = 0;
      }

      return (0);
    }
  }

  /* chance of being thrown off mount */
  if (riding && !HAS_FEAT(ch, FEAT_MOUNTED_COMBAT) && 
      (compute_ability(ch, ABILITY_RIDE) + d20(ch)) < rand_number(1, GET_LEVEL(RIDING(ch))) - rand_number(-4, need_movement))
  {
    act("$N rears backwards, throwing you to the ground.",
        FALSE, ch, 0, RIDING(ch), TO_CHAR);
    act("You rear backwards, throwing $n to the ground.",
        FALSE, ch, 0, RIDING(ch), TO_VICT);
    act("$N rears backwards, throwing $n to the ground.",
        FALSE, ch, 0, RIDING(ch), TO_NOTVICT);
    dismount_char(ch);
    damage(ch, ch, dice(1, 6), -1, -1, -1);
    return 0;
  }

  /*---------------------------------------------------------------------*/
  /* End checks that can prevent a character from leaving the was_in room. */

  /* Begin: the leave operation. */
  /*---------------------------------------------------------------------*/

  if (GET_LEVEL(ch) < LVL_IMMORT && !IS_NPC(ch) && !(riding || ridden_by))
    GET_MOVE(ch) -= need_movement;
  /* artificial inflation of mount movement points */
  else if (riding && !rand_number(0, 9))
    GET_MOVE(RIDING(ch)) -= need_movement;
  else if (ridden_by)
    GET_MOVE(RIDDEN_BY(ch)) -= need_movement;

  /* Display leave messages using modular function */
  display_leave_messages(ch, dir, riding, ridden_by);


#if !defined(CAMPAIGN_DL) && !defined(CAMPAIGN_FR)
  /* Create tracks using new modular function */
  if (should_create_tracks(ch))
    create_movement_tracks(ch, dir, TRACKS_OUT);
#endif

  /* the actual technical moving of the char */
  char_from_room(ch);

  X_LOC(ch) = new_x;
  Y_LOC(ch) = new_y;

  char_to_room(ch, going_to);
  /* end the actual technical moving of the char */
  
  /* Autosearch: Check for traps automatically if enabled */
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOSEARCH))
  {
    /* Perform autosearch at half perception skill */
    perform_autosearch(ch);
  }

  /* move the mount too */
  if (riding && same_room && RIDING(ch)->in_room != ch->in_room)
  {
    char_from_room(RIDING(ch));

    X_LOC(RIDING(ch)) = new_x;
    Y_LOC(RIDING(ch)) = new_y;

    char_to_room(RIDING(ch), ch->in_room);
  }
  else if (ridden_by && same_room && RIDDEN_BY(ch)->in_room != ch->in_room)
  {
    char_from_room(RIDDEN_BY(ch));

    X_LOC(RIDDEN_BY(ch)) = new_x;
    Y_LOC(RIDDEN_BY(ch)) = new_y;

    char_to_room(RIDDEN_BY(ch), ch->in_room);
  }
  /*---------------------------------------------------------------------*/
  /* End: the leave operation. The character is now in the new room. */

  /* Begin: Post-move operations. */
  /*---------------------------------------------------------------------*/
  /* Post Move Trigger Checks: Check the new room for triggers.
   * Assumptions: The character has already truly left the was_in room. If
   * the entry trigger "prevents" movement into the room, it is the triggers
   * job to provide a message to the original was_in room. */
  if (!entry_mtrigger(ch) || !enter_wtrigger(&world[going_to], ch, dir))
  {
    char_from_room(ch);

    if (ZONE_FLAGGED(GET_ROOM_ZONE(was_in), ZONE_WILDERNESS))
    {
      X_LOC(ch) = world[was_in].coords[0];
      Y_LOC(ch) = world[was_in].coords[1];
    }

    char_to_room(ch, was_in);
    if (riding && same_room && RIDING(ch)->in_room != ch->in_room)
    {
      char_from_room(RIDING(ch));

      if (ZONE_FLAGGED(GET_ROOM_ZONE(ch->in_room), ZONE_WILDERNESS))
      {
        X_LOC(RIDING(ch)) = world[ch->in_room].coords[0];
        Y_LOC(RIDING(ch)) = world[ch->in_room].coords[1];
      }

      char_to_room(RIDING(ch), ch->in_room);
    }
    else if (ridden_by && same_room &&
             RIDDEN_BY(ch)->in_room != ch->in_room)
    {
      char_from_room(RIDDEN_BY(ch));

      if (ZONE_FLAGGED(GET_ROOM_ZONE(ch->in_room), ZONE_WILDERNESS))
      {
        X_LOC(RIDDEN_BY(ch)) = world[ch->in_room].coords[0];
        Y_LOC(RIDDEN_BY(ch)) = world[ch->in_room].coords[1];
      }

      char_to_room(RIDDEN_BY(ch), ch->in_room);
    }
    return 0;
  }

  /* char moved from room, so shift everything around */
  if (ROOM_FLAGGED(ch->in_room, ROOM_SINGLEFILE) &&
      !is_top_of_room_for_singlefile(ch, rev_dir[dir]))
  {
    world[ch->in_room].people = ch->next_in_room;
    prev = &world[ch->in_room].people;
    while (*prev)
      prev = &((*prev)->next_in_room);
    *prev = ch;
    ch->next_in_room = NULL;
  }

  /* ... and the room description to the character. */
  if (ch->desc != NULL)
  {
    look_at_room(ch, 0);
    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOSCAN))
      do_scan(ch, 0, 0, 0);
  }
  if (ridden_by)
  {
    if (RIDDEN_BY(ch)->desc != NULL)
    {
      look_at_room(RIDDEN_BY(ch), 0);
      if (!IS_NPC(RIDDEN_BY(ch)) && PRF_FLAGGED(RIDDEN_BY(ch), PRF_AUTOSCAN))
        do_scan(RIDDEN_BY(ch), 0, 0, 0);
    }
  }
  if (riding)
  {
    if (RIDING(ch)->desc != NULL)
    {
      look_at_room(RIDING(ch), 0);
      if (!IS_NPC(RIDING(ch)) && PRF_FLAGGED(RIDING(ch), PRF_AUTOSCAN))
        do_scan(RIDING(ch), 0, 0, 0);
    }
  }

  /* Display enter messages using modular function */
  display_enter_messages(ch, dir, riding, ridden_by);


  /* Process all post-movement events (walls, damage, triggers, etc) */
  if (!process_movement_events(ch, was_in, going_to, dir))
  {
    /* Event prevented movement */
    return 0;
  }

  /* At this point, the character is safe and in the room. */

  return (1);
}
/* END do_simple_move()*/

int perform_move(struct char_data *ch, int dir, int need_specials_check)
{
  return perform_move_full(ch, dir, need_specials_check, true);
}

int perform_move_full(struct char_data *ch, int dir, int need_specials_check, bool recursive)
{
  room_rnum was_in;
  struct follow_type *k, *next;
  char open_cmd[MEDIUM_STRING] = {'\0'};

  if (ch == NULL || dir < 0 || dir >= NUM_OF_DIRS)
    return (0);
  else if (FIGHTING(ch))
    send_to_char(ch, "You are too busy fighting to move!\r\n");
  else if (char_has_mud_event(ch, eFISTED))
    send_to_char(ch, "You can't move!  You are being held in place by a large clenched fist!\r\n");
  else if (!CONFIG_DIAGONAL_DIRS && IS_DIAGONAL(dir))
    send_to_char(ch, "Alas, you cannot go that way...\r\n");
  else if ((!EXIT(ch, dir) && !buildwalk(ch, dir)) || EXIT(ch, dir)->to_room == NOWHERE)
    send_to_char(ch, "Alas, you cannot go that way...\r\n");
  else if (EXIT_FLAGGED(EXIT(ch, dir), EX_HIDDEN) && GET_LEVEL(ch) < LVL_IMMORT)
    send_to_char(ch, "Alas, you cannot go that way...\r\n");
  else if (char_has_mud_event(ch, eFALLING))
    send_to_char(ch, "You can't, you are falling!!!\r\n");
  else if (EXIT_FLAGGED(EXIT(ch, dir), EX_CLOSED) && (GET_LEVEL(ch) < LVL_IMMORT || (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_NOHASSLE))))
  {
    if ((!IS_NPC(ch)) && (PRF_FLAGGED(ch, PRF_AUTODOOR)) && recursive)
    {
      ch->char_specials.autodoor_message = false;
      snprintf(open_cmd, sizeof(open_cmd), " %s", dirs[dir]);
      // send_to_char(ch, "CMD: %s\r\n", open_cmd);
      do_gen_door(ch, open_cmd, 0, SCMD_OPEN);
      if (ch->char_specials.autodoor_message)
      {
        snprintf(open_cmd, sizeof(open_cmd), "You open the %s to the %s.", EXIT(ch, dir)->keyword, dirs[dir]);
        act(open_cmd, FALSE, ch, 0, 0, TO_CHAR);
        ch->char_specials.autodoor_message = false;
      }
      // return perform_move_full(ch, dir, need_specials_check, false);
    }
    else
    {
      if (EXIT(ch, dir)->keyword)
        send_to_char(ch, "The %s seems to be closed.\r\n", fname(EXIT(ch, dir)->keyword));
      else
        send_to_char(ch, "It seems to be closed.\r\n");
    }
  }
  else if (in_encounter_room(ch) && !is_peaceful_encounter(ch))
  {
    send_to_char(ch, "You must deal with this encounter before proceeding.\r\n");
  }
  else
  {
    /* This was tricky - buildwalk for normal rooms is only activated above.
     * for wilderness rooms we need to activate it here. */
    if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_WILDERNESS))
      buildwalk(ch, dir);

    if (!ch->followers)
      return (do_simple_move(ch, dir, need_specials_check));

    was_in = IN_ROOM(ch);
    if (!do_simple_move(ch, dir, need_specials_check))
      return (0);

    for (k = ch->followers; k; k = next)
    {
      next = k->next;
      if ((IN_ROOM(k->follower) == was_in) &&
          (GET_POS(k->follower) >= POS_STANDING))
      {
        act("You follow $N.\r\n", FALSE, k->follower, 0, ch, TO_CHAR);
        perform_move(k->follower, dir, 1);
      }
    }

    return (1);
  }
  return (0);
}

ACMD(do_move)
{

  /* this test added for newer reclining position */
  if (GET_POS(ch) == POS_SITTING || GET_POS(ch) == POS_RESTING)
  {
    send_to_char(ch, "You have to be standing or reclining to move.\r\n");
    return;
  }

  /* These subcmd defines are mapped precisely to the direction defines. */
  perform_move(ch, subcmd, 0);
}

ACMD(do_enter)
{
  char buf[MAX_INPUT_LENGTH] = {'\0'};
  int door = 0, portal_type = 0, count = 0, diff = 0;
  room_vnum portal_dest = 1;
  room_rnum was_in = NOWHERE, real_dest = NOWHERE;
  struct follow_type *k = NULL;
  struct obj_data *portal = NULL;
  // room_vnum vClanhall = NOWHERE;
  // int iPlayerClan = 0;
  // room_rnum rClanhall = 0;

  if (FIGHTING(ch))
  {
    send_to_char(ch, "You are too busy fighting to enter!\r\n");
    return;
  }

  was_in = IN_ROOM(ch);

  one_argument(argument, buf, sizeof(buf));

  /* an argument was supplied, search for door keyword */
  if (*buf)
  {
    /* Portals first */
    portal = get_obj_in_list_vis(ch, buf, NULL, world[IN_ROOM(ch)].contents);
    if ((portal) && (GET_OBJ_TYPE(portal) == ITEM_PORTAL))
    {
      portal_type = portal->obj_flags.value[0];

      /* Perform checks and get destination */
      switch (portal_type)
      {
      case PORTAL_NORMAL:
        portal_dest = portal->obj_flags.value[1];
        break;

      case PORTAL_CHECKFLAGS:
        if (is_class_anti_object(ch, portal, false) && !is_class_req_object(ch, portal, false))
        {
          act("You try to enter $p, but a mysterious power "
              "forces you back!  (class restriction)",
              FALSE, ch, portal, 0, TO_CHAR);
          act("\tRA booming voice in your head shouts '\tWNot "
              "for your class!\tR'\tn",
              FALSE, ch, portal, 0, TO_CHAR);
          act("$n tries to enter $p, but a mysterious power "
              "forces $m back!",
              FALSE, ch, portal, 0, TO_ROOM);
          return;
        }

        if (IS_EVIL(ch) && OBJ_FLAGGED(portal, ITEM_ANTI_EVIL))
        {
          act("You try to enter $p, but a mysterious power "
              "forces you back!  (alignment restriction)",
              FALSE, ch, portal, 0, TO_CHAR);
          act("\tRA booming voice in your head shouts '\tWBEGONE "
              "EVIL-DOER!\tR'\tn",
              FALSE, ch, portal, 0, TO_CHAR);
          act("$n tries to enter $p, but a mysterious power "
              "forces $m back!",
              FALSE, ch, portal, 0, TO_ROOM);
          return;
        }

        if (IS_GOOD(ch) && OBJ_FLAGGED(portal, ITEM_ANTI_GOOD))
        {
          act("You try to enter $p, but a mysterious power "
              "forces you back!  (alignment restriction)",
              FALSE, ch, portal, 0, TO_CHAR);
          act("\tRA booming voice in your head shouts '\tWBEGONE "
              "DO-GOODER!\tR'\tn",
              FALSE, ch, portal, 0, TO_CHAR);
          act("$n tries to enter $p, but a mysterious power "
              "forces $m back!",
              FALSE, ch, portal, 0, TO_ROOM);
          return;
        }

        if (((!IS_EVIL(ch)) && !(IS_GOOD(ch))) &&
            OBJ_FLAGGED(portal, ITEM_ANTI_NEUTRAL))
        {
          act("You try to enter $p, but a mysterious power "
              "forces you back!  (alignment restriction)",
              FALSE, ch, portal, 0, TO_CHAR);
          act("\tRA booming voice in your head shouts '\tWBEGONE!"
              "\tR'\tn",
              FALSE, ch, portal, 0, TO_CHAR);
          act("$n tries to enter $p, but a mysterious power "
              "forces $m back!",
              FALSE, ch, portal, 0, TO_ROOM);
          return;
        }

        portal_dest = portal->obj_flags.value[1];
        break;

        /*
                  case PORTAL_CLANHALL:
                    iPlayerClan = GET_CLAN(ch);

                    if (iPlayerClan <= 0) {
                      send_to_char(ch, "You try to enter the portal, but it returns you back to the same room!\n\r");
                      return;
                    }

                    if (GET_CLANHALL_ZONE(ch) == NOWHERE) {
                      send_to_char(ch, "Your clan does not have a clanhall!\n\r");
                      log("[PORTAL] Clan Portal - No clanhall (Player: %s, Clan ID: %d)", GET_NAME(ch), iPlayerClan);
                      return;
                    }

                    vClanhall = (GET_CLANHALL_ZONE(ch) * 100) + 1;
                    rClanhall = real_room(vClanhall);

                    if (rClanhall == NOWHERE ) {
                      send_to_char(ch, "Your clanhall is currently broken - contact an Imm!\n\r");
                      log("[PORTAL] Clan Portal failed (Player: %s, Clan ID: %d)", GET_NAME(ch), iPlayerClan);
                      return;
                    }

                    portal_dest = vClanhall;
                    break;
           */

      case PORTAL_RANDOM:
        if (real_room(portal->obj_flags.value[1]) == NOWHERE)
        {
          send_to_char(ch, "The portal leads nowhere.  (please tell a staff member)\r\n");
          return;
        }

        if (real_room(portal->obj_flags.value[2]) == NOWHERE)
        {
          send_to_char(ch, "The portal leads nowhere.  (pleaes tell a staff member)\r\n");
          return;
        }

        count = 0;
        if (portal->obj_flags.value[1] > portal->obj_flags.value[2])
        {
          diff = portal->obj_flags.value[1] - portal->obj_flags.value[2];
          do
          {
            portal_dest =
                (portal->obj_flags.value[2]) + rand_number(0, diff);
          } while ((real_room(portal_dest) == NOWHERE) && (++count < 150));
        }
        else
        {
          diff = portal->obj_flags.value[2] - portal->obj_flags.value[1];
          do
          {
            portal_dest = (portal->obj_flags.value[1]) + rand_number(0, diff);
          } while ((real_room(portal_dest) == NOWHERE) && (++count < 150));
        }

        log("Random Portal: Sending %s to vnum %d", GET_NAME(ch), portal_dest);
        break;

      default:
        mudlog(NRM, LVL_STAFF, TRUE, "SYSERR: Invalid portal type (%d) in room %d", portal->obj_flags.value[0], world[IN_ROOM(ch)].number);
        send_to_char(ch, "This portal is broken, please tell an Imm.\r\n");
        return;
        break;
      }

      if ((real_dest = real_room(portal_dest)) == NOWHERE)
      {
        send_to_char(ch, "The portal appears to be a vacuum!  (tell a staff member please)\r\n");
        return;
      }

      /* All checks passed, except checking the destination, so let's do that now */
      /* this function needs a vnum, not rnum */
      if (ch && !House_can_enter(ch, portal_dest))
      {
        send_to_char(ch, "As you try to enter the portal, it flares "
                         "brightly, pushing you back!  (someone's private house)\r\n");
        return;
      }

      if (ROOM_FLAGGED(real_dest, ROOM_PRIVATE))
      {
        send_to_char(ch, "As you try to enter the portal, it flares "
                         "brightly, pushing you back!!  (private area)\r\n");
        return;
      }

      if (ROOM_FLAGGED(real_dest, ROOM_DEATH))
      {
        send_to_char(ch, "As you try to enter the portal, it flares "
                         "brightly, pushing you back!!!  (death room, eek!)\r\n");
        return;
      }

      if (ROOM_FLAGGED(real_dest, ROOM_STAFFROOM))
      {
        send_to_char(ch, "As you try to enter the portal, it flares "
                         "brightly, pushing you back!!!!  (destination is staff only)\r\n");
        return;
      }

      if (ZONE_FLAGGED(GET_ROOM_ZONE(real_dest), ZONE_CLOSED))
      {
        send_to_char(ch, "As you try to enter the portal, it flares "
                         "brightly, pushing you back!!!!!  (destination zone is closed for construction/repairs)\r\n");
        return;
      }

      /* ok NOW we are good to go */

      act("$n enters $p, and vanishes!", FALSE, ch, portal, 0, TO_ROOM);
      act("You enter $p, and you are transported elsewhere.", FALSE, ch, portal, 0, TO_CHAR);
      char_from_room(ch);

      if (ZONE_FLAGGED(GET_ROOM_ZONE(real_dest), ZONE_WILDERNESS))
      {
        X_LOC(ch) = world[real_dest].coords[0];
        Y_LOC(ch) = world[real_dest].coords[1];
      }

      char_to_room(ch, real_dest);
      look_at_room(ch, 0);
      act("$n appears from thin air!", FALSE, ch, 0, 0, TO_ROOM);

      /* Then, any followers should auto-follow (Jamdog 19th June 2006) */
      for (k = ch->followers; k; k = k->next)
      {
        if ((IN_ROOM(k->follower) == was_in) &&
            (GET_POS(k->follower) >= POS_STANDING))
        {
          act("You follow $N.\r\n", FALSE, k->follower, 0, ch, TO_CHAR);
          act("$n enters $p, and vanishes!", FALSE, k->follower, portal, 0, TO_ROOM);
          char_from_room(k->follower);

          if (ZONE_FLAGGED(GET_ROOM_ZONE(real_dest), ZONE_WILDERNESS))
          {
            X_LOC(k->follower) = world[real_dest].coords[0];
            Y_LOC(k->follower) = world[real_dest].coords[1];
          }

          char_to_room(k->follower, real_dest);
          look_at_room(k->follower, 0);
          act("$n appears from thin air!", FALSE, k->follower, 0, 0, TO_ROOM);
        }
      }
      return;
      /* Must be a door */
    }
    else
    {
      for (door = 0; door < NUM_OF_DIRS; door++)
      {
        if (EXIT(ch, door) && (EXIT(ch, door)->keyword))
        {
          if (!str_cmp(EXIT(ch, door)->keyword, buf))
          {
            perform_move(ch, door, 1);
            return;
          }
        }
      }
      send_to_char(ch, "There is no %s here.\r\n", buf);
    }
  }
  else if (!OUTSIDE(ch))
  {
    send_to_char(ch, "You are already indoors.\r\n");
  }
  else
  {
    /* try to locate an entrance */
    for (door = 0; door < NUM_OF_DIRS; door++)
    {
      if (EXIT(ch, door))
      {
        if (EXIT(ch, door)->to_room != NOWHERE)
        {
          if (!EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED) &&
              ROOM_OUTSIDE(EXIT(ch, door)->to_room))
          {
            perform_move(ch, door, 1);
            return;
          }
        }
      }
    }
    /*fail!!*/
    send_to_char(ch, "You can't seem to find anything to enter.\r\n");
  }
}

ACMD(do_leave)
{
  int door;

  if (OUTSIDE(ch))
    send_to_char(ch, "You are outside.. where do you want to go?\r\n");
  else
  {
    for (door = 0; door < DIR_COUNT; door++)
      if (EXIT(ch, door))
        if (EXIT(ch, door)->to_room != NOWHERE)
          if (!EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED) &&
              !ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_INDOORS))
          {
            perform_move(ch, door, 1);
            return;
          }
    send_to_char(ch, "I see no obvious exits to the outside.\r\n");
  }
}

/* put together a simple check to see if someone can stand, at time of writing its for AUTO_STAND */

ACMD(do_follow)
{
  char buf[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *leader = NULL;

  one_argument(argument, buf, sizeof(buf));

  if (*buf)
  {
    if (!(leader = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM)))
    {
      send_to_char(ch, "%s", CONFIG_NOPERSON);
      return;
    }
  }
  else
  {
    send_to_char(ch, "Whom do you wish to follow?\r\n");
    return;
  }

  /* we now have the 'leader' aka the person we are trying to follow */

  /* easy out */
  if (ch->master == leader)
  {
    act("You are already following $M.", FALSE, ch, 0, leader, TO_CHAR);
    return;
  }

  /* easy out */
  if (!IS_NPC(leader) && PRF_FLAGGED(leader, PRF_NO_FOLLOW))
  {
    act("$N has $S nofollow toggled.", FALSE, ch, 0, leader, TO_CHAR);
    return;
  }

  if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master))
  {
    act("But you only feel like following $N!", FALSE, ch, 0, ch->master, TO_CHAR);
  }
  else
  { /* Not Charmed follow person */
    if (leader == ch)
    {
      if (!ch->master)
      {
        send_to_char(ch, "You are already following yourself.\r\n");
        return;
      }
      stop_follower(ch);
    }
    else
    {
      if (circle_follow(ch, leader))
      {
        send_to_char(ch, "Sorry, but following in loops is not allowed.\r\n");
        return;
      }
      if (ch->master)
        stop_follower(ch);

      add_follower(ch, leader);
    }
  }
}

ACMD(do_unlead)
{
  char buf[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *follower;

  one_argument(argument, buf, sizeof(buf));

  if (*buf)
  {
    if (!(follower = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM)))
    {
      send_to_char(ch, "%s", CONFIG_NOPERSON);
      return;
    }
  }
  else
  {
    send_to_char(ch, "Whom do you wish to stop leading?\r\n");
    return;
  }

  if (follower->master != ch)
  {
    act("$E isn't following you!", FALSE, ch, 0, follower, TO_CHAR);
    return;
  }

  // Not replicating the AFF_CHARM check from do_follow here - if you want to unlead a charmee,
  // go right ahead.  We also don't want to call stop_follower() on the follower, or you'd be freeing
  // your charmees instead of just making them stay put.  We'll use stop_follower_engine() instead.
  stop_follower_engine(follower);
  follower->master = NULL;
  act("$N stops following you.", FALSE, ch, 0, follower, TO_CHAR);
  act("You are no longer following $n.", TRUE, ch, 0, follower, TO_VICT);
  act("$N stops following $n.", TRUE, ch, 0, follower, TO_NOTVICT);
}

ACMD(do_sorcerer_draconic_wings)
{

  if (IS_NPC(ch) || !HAS_FEAT(ch, FEAT_DRACONIC_HERITAGE_WINGS))
  {
    send_to_char(ch, "You have no idea how.\r\n");
    return;
  }

  if (affected_by_spell(ch, SKILL_DRHRT_WINGS))
  {
    send_to_char(ch, "You retract your draconic wings.\r\n");
    act("$n retracts a large pair of draconic wings into $s back.", TRUE, ch, 0, 0, TO_ROOM);
    affect_from_char(ch, SKILL_DRHRT_WINGS);
    return;
  }

  struct affected_type af;

  new_affect(&af);

  af.spell = SKILL_DRHRT_WINGS;
  SET_BIT_AR(af.bitvector, AFF_FLYING);
  af.duration = -1;

  affect_to_char(ch, &af);

  send_to_char(ch, "You spread your draconic wings, giving you flight at a speed of 60.\r\n");
  act("$n spreads $s draconic wings.", TRUE, ch, 0, 0, TO_ROOM);
}

ACMD(do_pullswitch)
{
  //- item-switch( command(push/pull), room, dir, unhide/unlock/open)
  int other_room = 0;
  struct room_direction_data *back = 0;
  int door = 0;
  int room = 0;
  struct obj_data *obj;
  struct char_data *tmp_ch;
  struct obj_data *dummy = 0;
  char arg[MAX_INPUT_LENGTH] = {'\0'};

  one_argument(argument, arg, sizeof(arg));

  if (!*arg)
  {
    send_to_char(ch, "You want to do what?!\r\n");
    return;
  }

  generic_find(arg, FIND_OBJ_ROOM, ch, &tmp_ch, &obj);
  if (!obj)
  {
    send_to_char(ch, "You do not see that here.\r\n");
    return;
  }

  if (GET_OBJ_TYPE(obj) != ITEM_SWITCH)
  {
    send_to_char(ch, "But that is not a switch\r\n");
    return;
  }

  if (CMD_IS("pull") && 0 != GET_OBJ_VAL(obj, 0))
  {
    send_to_char(ch, "But you can't pull that.\r\n");
    return;
  }
  if (CMD_IS("push") && 1 != GET_OBJ_VAL(obj, 0))
  {
    send_to_char(ch, "But you can't push that.\r\n");
    return;
  }

  room = real_room(GET_OBJ_VAL(obj, 1));
  door = GET_OBJ_VAL(obj, 2);

  if (room < 0)
  {
    send_to_char(ch, "Bug in switch here, contact an immortal\r\n");
    log("SYSERR: Broken switch: real_room() for %s (VNUM %ld) evaluated to -1", obj->name, obj->id);
    return;
  }
  if (!world[room].dir_option[door])
  {
    send_to_char(ch, "Bug in switch here, contact an immortal\r\n");
    return;
  }

  if ((other_room = EXITN(room, door)->to_room) != NOWHERE)
  {
    if ((back = world[other_room].dir_option[rev_dir[door]]))
    {
      if (back->to_room != ch->in_room)
        back = 0;
    }
  }

  switch (GET_OBJ_VAL(obj, 3))
  {
  case SWITCH_UNHIDE:
    break;
  case SWITCH_UNLOCK:
    UNLOCK_DOOR(room, dummy, door);
    if (back)
      UNLOCK_DOOR(other_room, dummy, rev_dir[door]);
    break;
  case SWITCH_OPEN:
    if (EXIT_FLAGGED(world[room].dir_option[door], EX_LOCKED))
      UNLOCK_DOOR(room, dummy, door);
    OPEN_DOOR(room, dummy, door);
    if (back)
    {
      if (EXIT_FLAGGED(world[other_room].dir_option[rev_dir[door]], EX_LOCKED))
        UNLOCK_DOOR(other_room, dummy, rev_dir[door]);
      OPEN_DOOR(other_room, dummy, rev_dir[door]);
    }
    break;
  }

  if (obj->action_description != NULL)
    send_to_room(ch->in_room, "%s", obj->action_description);
  else
    send_to_room(ch->in_room, "*ka-ching*\r\n");
}

ACMD(do_transposition)
{
  if (!HAS_FEAT(ch, FEAT_TRANSPOSITION))
  {
    send_to_char(ch, "You do not have the transposition feat.\r\n");
    return;
  }

  struct follow_type *f = NULL;
  struct char_data *mob = NULL, *eidolon = NULL;
  room_rnum chRoom = NOWHERE, eidolonRoom = NOWHERE;

  for (f = ch->followers; f; f = f->next)
  {
    mob = f->follower;
    if (!mob) continue;
    if (!IS_NPC(mob)) continue;
    if (!MOB_FLAGGED(mob, MOB_EIDOLON)) continue;
    eidolon = mob;
    break;
  }

  if (!eidolon)
  {
    send_to_char(ch, "Your eidolon does not seem to be summoned.\r\n");
    return;
  }

  chRoom = IN_ROOM(ch);
  eidolonRoom = IN_ROOM(eidolon);

  if (chRoom == NOWHERE || eidolonRoom == NOWHERE)
  {
    send_to_char(ch, "You are unable to transposition yourself and your eidolon.\r\n");
    return;
  }

  if (!valid_mortal_tele_dest(eidolon, chRoom, true) || !valid_mortal_tele_dest(ch, eidolonRoom, true))
  {
    send_to_char(ch, "You are unable to transposition yourself and your eidolon.\r\n");
    return;
  }

  // send eidolon to ch's location
  act("$n disappears suddenly.", TRUE, eidolon, 0, 0, TO_ROOM);
  char_from_room(eidolon);
  if (ZONE_FLAGGED(GET_ROOM_ZONE(chRoom), ZONE_WILDERNESS))
  {
    X_LOC(eidolon) = world[chRoom].coords[0];
    Y_LOC(eidolon) = world[chRoom].coords[1];
  }
  char_to_room(eidolon, chRoom);

  act("$n arrives suddenly.", TRUE, eidolon, 0, 0, TO_ROOM);
  act("$n has transpoisitoned locations with you!", FALSE, ch, 0, eidolon, TO_VICT);
  look_at_room(eidolon, 0);
  entry_memory_mtrigger(eidolon);
  greet_mtrigger(eidolon, -1);
  greet_memory_mtrigger(eidolon);

  // send ch to eidolon's location
  act("$n disappears suddenly.", TRUE, ch, 0, 0, TO_ROOM);
  char_from_room(ch);
  if (ZONE_FLAGGED(GET_ROOM_ZONE(eidolonRoom), ZONE_WILDERNESS))
  {
    X_LOC(ch) = world[eidolonRoom].coords[0];
    Y_LOC(ch) = world[eidolonRoom].coords[1];
  }
  char_to_room(ch, eidolonRoom);

  act("$n arrives suddenly.", TRUE, ch, 0, 0, TO_ROOM);
  act("$N has transpoisitoned locations with you!", FALSE, ch, 0, eidolon, TO_VICT);
  look_at_room(ch, 0);
  entry_memory_mtrigger(ch);
  greet_mtrigger(ch, -1);
  greet_memory_mtrigger(ch);

  USE_STANDARD_ACTION(ch);
}

ACMDU(do_unstuck)
{
  int exp = 0, gold = 0;

  exp = GET_LEVEL(ch) * GET_LEVEL(ch) * 500;
  gold = GET_LEVEL(ch) * GET_LEVEL(ch) * 5;

  if (GET_LEVEL(ch) <= 5)
    exp = gold = 0;
  else if (GET_LEVEL(ch) <= 10)
    { exp /= 4; gold /= 4; }
  else if (GET_LEVEL(ch) <= 20)
    { exp /= 2; gold /= 2; }

  if (ch->player_specials->unstuck == NULL)
  {
    ch->player_specials->unstuck = randstring(6);
  }

  skip_spaces(&argument);

  if (!*argument)
  {
    send_to_char(ch, "You didn't supply a confirmation code.\r\n"
                     "Please enter 'unstuck %s' to confirm your desire to become unstuck.\r\n"
                     "You will be transported to the MUD start room", ch->player_specials->unstuck);
    if (exp > 0)
      send_to_char(ch, ", and it will cost you %d experience points and %d coins", exp, gold);
    send_to_char(ch, ".\r\n");
    return;
  }
  if (strcmp(argument, ch->player_specials->unstuck))
  {
    send_to_char(ch, "Please enter 'unstuck %s' to confirm your desire to become unstuck. You typed: %s\r\n"
                     "You will be transported to the MUD start room", ch->player_specials->unstuck, argument);
    if (exp > 0)
      send_to_char(ch, ", and it will cost you %d experience points and %d coins", exp, gold);
    send_to_char(ch, ".\r\n");
    return;
  }
  else
  {
    GET_EXP(ch) -= exp;
    send_to_char(ch, "You lose %d experience points.\r\n", exp);
    if (GET_GOLD(ch) < gold)
    {
      gold -= GET_GOLD(ch);
      send_to_char(ch, "You lose %d gold ", gold);
      GET_GOLD(ch) = 0;
      if (GET_BANK_GOLD(ch) < gold)
      {
        gold = GET_BANK_GOLD(ch);
        send_to_char(ch, "and %d bank gold.\r\n", gold);
        GET_BANK_GOLD(ch) = 0;
      }
      else
      {
        GET_BANK_GOLD(ch) -= gold;
        send_to_char(ch, "and %d bank gold.\r\n", gold);
      }
    }
    else
    {
      GET_GOLD(ch) -= gold;
      send_to_char(ch, "You lose %d gold.\r\n", gold);
    }
  }
  act("$n disappears.", TRUE, ch, 0, 0, TO_ROOM);
  char_from_room(ch);
  char_to_room(ch, real_room(CONFIG_MORTAL_START));
  act("$n appears in the middle of the room.", TRUE, ch, 0, 0, TO_ROOM);
  look_at_room(ch, 0);
  entry_memory_mtrigger(ch);
  greet_mtrigger(ch, -1);
  greet_memory_mtrigger(ch);
  ch->player_specials->unstuck = NULL;
}

ACMD(do_lastroom)
{
  unsigned int last_room = GET_LAST_ROOM(ch);
  unsigned int start_room = (unsigned int) CONFIG_MORTAL_START;

  if (GET_ROOM_VNUM(IN_ROOM(ch)) != start_room)
  {
    send_to_char(ch, "You can't use the lastroom command here.\r\n");
    return;
  }

  if (last_room == 0 || last_room == NOWHERE || last_room == start_room)
  {
    send_to_char(ch, "You don't have a valid 'last room'.\r\n");
    return;
  }

  char_from_room(ch);
  act("$n disappears ina  flash.", TRUE, ch, 0, 0, TO_ROOM);
  act("You teleport to your last location.", FALSE, ch, 0, 0, TO_CHAR);
  char_to_room(ch, real_room(last_room));
  do_look(ch, "", 0, 0);

  char_pets_to_char_loc(ch);

}

/* undefines */
#undef PRISONER_KEY_1
#undef PRISONER_KEY_2
#undef PRISONER_KEY_3
#undef TRACKS_UNDEFINED
#undef TRACKS_IN
#undef TRACKS_OUT
#undef DIR_NONE

/*EOF*/
