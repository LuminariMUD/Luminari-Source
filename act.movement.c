/**************************************************************************
 *  File: act.movement.c                               Part of LuminariMUD *
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
#include "trails.h"
#include "assign_wpn_armor.h"
#include "encounters.h"
#include "hunts.h"
#include "class.h"
#include "transport.h"

/* do_gen_door utility functions */
static int find_door(struct char_data *ch, const char *type, char *dir,
                     const char *cmdname);
static void do_doorcmd(struct char_data *ch, struct obj_data *obj, int door,
                       int scmd);
static int ok_pick(struct char_data *ch, obj_vnum keynum, int pickproof,
                   int scmd, int door);

#define DOOR_IS_OPENABLE(ch, obj, door) ((obj) ? (((GET_OBJ_TYPE(obj) ==                    \
                                                    ITEM_CONTAINER) ||                      \
                                                   GET_OBJ_TYPE(obj) == ITEM_AMMO_POUCH) && \
                                                  OBJVAL_FLAGGED(obj, CONT_CLOSEABLE))      \
                                               : (EXIT_FLAGGED(EXIT(ch, door), EX_ISDOOR)))
#define DOOR_IS_OPEN(ch, obj, door) ((obj) ? (!OBJVAL_FLAGGED(obj,          \
                                                              CONT_CLOSED)) \
                                           : (!EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED)))
#define DOOR_IS_UNLOCKED(ch, obj, door) ((obj) ? (!OBJVAL_FLAGGED(obj,                                                                         \
                                                                  CONT_LOCKED))                                                                \
                                               : (!EXIT_FLAGGED(EXIT(ch, door), EX_LOCKED) && !EXIT_FLAGGED(EXIT(ch, door), EX_LOCKED_EASY) && \
                                                  !EXIT_FLAGGED(EXIT(ch, door), EX_LOCKED_MEDIUM) && !EXIT_FLAGGED(EXIT(ch, door), EX_LOCKED_HARD)))
#define DOOR_IS_PICKPROOF(ch, obj, door) ((obj) ? (OBJVAL_FLAGGED(obj,             \
                                                                  CONT_PICKPROOF)) \
                                                : (EXIT_FLAGGED(EXIT(ch, door), EX_PICKPROOF)))
#define DOOR_IS_CLOSED(ch, obj, door) (!(DOOR_IS_OPEN(ch, obj, door)))
#define DOOR_IS_LOCKED(ch, obj, door) (!(DOOR_IS_UNLOCKED(ch, obj, door)))
#define DOOR_KEY(ch, obj, door) ((obj) ? (GET_OBJ_VAL(obj, 2)) : (EXIT(ch, door)->key))

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

  check_trap(ch, TRAP_TYPE_OPEN_DOOR, ch->in_room, 0, door);

  return;
}
 */

/* checks if target ch is first-in-line in singlefile room */
bool is_top_of_room_for_singlefile(struct char_data *ch, int dir)
{
  bool exit = FALSE;
  int i;

  for (i = 0; i < 6; i++)
  {
    if (EXIT(ch, i))
    {
      if (exit == FALSE && dir == i)
        return TRUE;
      exit = TRUE;
    }
  }
  return FALSE;
}

/* function to find which char is ahead of the next
   in a singlefile room */
struct char_data *get_char_ahead_of_me(struct char_data *ch, int dir)
{
  struct char_data *tmp;

  if (is_top_of_room_for_singlefile(ch, dir))
  {
    tmp = world[ch->in_room].people;
    while (tmp)
    {
      if (tmp->next_in_room == ch)
        return tmp;
      tmp = tmp->next_in_room;
    }
    return 0;
  }
  return ch->next_in_room;
}

/* falling system */

/* TODO objects */

/* this function will check whether a obj should fall or not based on
   circumstances and whether the obj is floating */
bool obj_should_fall(struct obj_data *obj)
{
  int falling = FALSE;

  if (!obj)
    return FALSE;

  if (ROOM_FLAGGED(obj->in_room, ROOM_FLY_NEEDED) && EXIT_OBJ(obj, DOWN))
    falling = TRUE;

  if (OBJ_FLAGGED(obj, ITEM_FLOAT))
  {
    act("You watch as $p floats gracefully in the air!",
        FALSE, 0, obj, 0, TO_ROOM);
    return FALSE;
  }

  return falling;
}

/* this function will check whether a char should fall or not based on
   circumstances and whether the ch is flying / levitate */
bool char_should_fall(struct char_data *ch, bool silent)
{
  int falling = FALSE;

  if (!ch)
    return FALSE;

  if (IN_ROOM(ch) == NOWHERE)
    return FALSE;

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_FLY_NEEDED) && EXIT(ch, DOWN))
    falling = TRUE;

  if (RIDING(ch) && is_flying(RIDING(ch)))
  {
    if (!silent)
      send_to_char(ch, "Your mount flies gracefully through the air...\r\n");
    return FALSE;
  }

  if (is_flying(ch))
  {
    if (!silent)
      send_to_char(ch, "You fly gracefully through the air...\r\n");
    return FALSE;
  }

  if (AFF_FLAGGED(ch, AFF_LEVITATE))
  {
    if (!silent)
      send_to_char(ch, "You levitate above the ground...\r\n");
    return FALSE;
  }

  return falling;
}

EVENTFUNC(event_falling)
{
  struct mud_event_data *pMudEvent = NULL;
  struct char_data *ch = NULL;
  int height_fallen = 0;
  char buf[50] = {'\0'};

  /* This is just a dummy check, but we'll do it anyway */
  if (event_obj == NULL)
    return 0;

  /* For the sake of simplicity, we will place the event data in easily
   * referenced pointers */
  pMudEvent = (struct mud_event_data *)event_obj;

  /* nab char data */
  ch = (struct char_data *)pMudEvent->pStruct;

  /* dummy checks */
  if (!ch)
    return 0;
  if (!IS_NPC(ch) && !IS_PLAYING(ch->desc))
    return 0;

  /* retrieve svariables and convert it */
  height_fallen += atoi((char *)pMudEvent->sVariables);
  send_to_char(ch, "AIYEE!!!  You have fallen %d feet!\r\n", height_fallen);

  /* already checked if there is a down exit, lets move the char down */
  do_simple_move(ch, DOWN, FALSE);
  send_to_char(ch, "You fall into a new area!\r\n");
  act("$n appears from above, arms flailing helplessly as $e falls...",
      FALSE, ch, 0, 0, TO_ROOM);
  height_fallen += 20; // 20 feet per room right now

  /* can we continue this fall? */
  if (!ROOM_FLAGGED(ch->in_room, ROOM_FLY_NEEDED) || !CAN_GO(ch, DOWN))
  {

    if (AFF_FLAGGED(ch, AFF_SAFEFALL))
    {
      send_to_char(ch, "Moments before slamming into the ground, a 'safefall'"
                       " enchantment stops you!\r\n");
      act("Moments before $n slams into the ground, some sort of magical force"
          " stops $s from the impact.",
          FALSE, ch, 0, 0, TO_ROOM);
      REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_SAFEFALL);
      return 0;
    }

    /* potential damage */
    int dam = dice((height_fallen / 5), 6) + 20;

    /* check for slow-fall! */
    if (!IS_NPC(ch) && HAS_FEAT(ch, FEAT_SLOW_FALL))
    {
      dam -= 21;
      dam -= dice((HAS_FEAT(ch, FEAT_SLOW_FALL) * 4), 6);
    }

    if (dam <= 0)
    { /* woo! avoided damage */
      send_to_char(ch, "You gracefully land on your feet from your perilous fall!\r\n");
      act("$n comes falling in from above, but at the last minute, pulls off an acrobatic flip and lands gracefully on $s feet!", FALSE, ch, 0, 0, TO_ROOM);
      return 0; // end event
    }
    else
    { /* ok we know damage is going to be suffered at this stage */
      send_to_char(ch, "You fall headfirst to the ground!  OUCH!\r\n");
      act("$n crashes into the ground headfirst, OUCH!", FALSE, ch, 0, 0, TO_ROOM);
      change_position(ch, POS_RECLINING);
      start_action_cooldown(ch, atSTANDARD, 12 RL_SEC);

      /* we have a special situation if you die, the event will get cleared */
      if (dam >= GET_HIT(ch) + 9)
      {
        GET_HIT(ch) = -999;
        send_to_char(ch, "You attempt to scream in horror as your skull slams "
                         "into the ground, the very brief sensation of absolute pain "
                         "strikes you as all your upper-body bones shatter and your "
                         "head splatters all over the area!\r\n");
        act("$n attempts to scream in horror as $s skull slams "
            "into the ground.  There is the sound like the cracking of a "
            "ripe melon.  You watch as all of $s upper-body bones shatter and $s "
            "head splatters all over the area!\r\n",
            FALSE, ch, 0, 0, TO_ROOM);
        return 0;
      }
      else
      {
        damage(ch, ch, dam, TYPE_UNDEFINED, DAM_FORCE, FALSE);
        return 0; // end event
      }
    }
  }

  /* hitting ground or fixing your falling situation is the only way to stop
   *  this event :P
   * theoritically the player now could try to cast a spell, use an item, hop
   * on a mount to fix his falling situation, so we gotta check if he's still
   * falling every event call
   *  */
  if (char_should_fall(ch, FALSE))
  {
    send_to_char(ch, "You fall tumbling down!\r\n");
    act("$n drops from sight.", FALSE, ch, 0, 0, TO_ROOM);

    /* are we falling more?  then we gotta increase the heigh fallen */
    snprintf(buf, sizeof(buf), "%d", height_fallen);
    /* Need to free the memory, if we are going to change it. */
    if (pMudEvent->sVariables)
      free(pMudEvent->sVariables);
    pMudEvent->sVariables = strdup(buf);
    return (1 * PASSES_PER_SEC);
  }
  else
  { // stop falling!
    send_to_char(ch, "You put a stop to your fall!\r\n");
    act("$n turns on the air-brakes from a plummet!", FALSE, ch, 0, 0, TO_ROOM);
    return 0;
  }

  return 0;
}
/*  END falling system */

/* simple function to determine if char can walk on water (or swim through it )*/
int has_boat(struct char_data *ch, room_rnum going_to)
{
  struct obj_data *obj;
  int i;

  if (GET_LEVEL(ch) >= LVL_IMMORT)
    return (1);

  if (AFF_FLAGGED(ch, AFF_WATERWALK) || is_flying(ch) ||
      AFF_FLAGGED(ch, AFF_LEVITATE))
    return (1);

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
  send_to_char(ch, "Swim DC: %d - ", swim_dc);
  if (!skill_check(ch, ABILITY_SWIM, swim_dc))
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

  /* Non-wearable 'climb' items in inventory will do it. */
  for (obj = ch->carrying; obj; obj = obj->next_content)
    if (OBJAFF_FLAGGED(obj, AFF_CLIMB) && (find_eq_pos(ch, obj, NULL) < 0))
      return (1);

  /* Any equipped objects with AFF_CLIMB will do it too. */
  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i) && OBJAFF_FLAGGED(GET_EQ(ch, i), AFF_CLIMB))
      return (1);

  return (0);
}

/** Leave tracks in the current room
 * */

#define TRACKS_UNDEFINED 0
#define TRACKS_IN 1
#define TRACKS_OUT 2
#define DIR_NONE -1

void create_tracks(struct char_data *ch, int dir, int flag)
{
  struct room_data *room = NULL;
  struct trail_data *cur = NULL;
  struct trail_data *prev = NULL;
  struct trail_data *new_trail = NULL;

  if (IN_ROOM(ch) != NOWHERE)
  {
    room = &world[ch->in_room];
  }
  else
  {
    log("SYSERR: Char at location NOWHERE trying to create tracks.");
    return;
  }

  /*
    Here we create the track structure, set the values and assign it to the room.
    At the same time, we can prune off any really old trails.  Threshold is set,
    in seconds, in trails.h.  Eventually this cna be adjusted based on weather -
    rain/show/wind can all obscure trails.
  */

  CREATE(new_trail, struct trail_data, 1);
  new_trail->name = strdup(GET_NAME(ch));
  new_trail->race = (IS_NPC(ch) ? strdup(race_family_types[GET_NPC_RACE(ch)]) : strdup(race_list[GET_RACE(ch)].name));
  new_trail->from = (flag == TRACKS_IN ? dir : DIR_NONE);
  new_trail->to = (flag == TRACKS_OUT ? dir : DIR_NONE);
  new_trail->age = time(NULL);

  new_trail->next = room->trail_tracks->head;
  new_trail->prev = NULL;

  if (new_trail->next != NULL)
  {
    room->trail_tracks->head->prev = new_trail;
  }

  room->trail_tracks->head = new_trail;

  prev = NULL;
  for (cur = room->trail_tracks->head; cur != NULL; cur = cur->next)
  {
    if (time(NULL) - cur->age >= TRAIL_PRUNING_THRESHOLD)
    {
      if (prev != NULL)
      {
        // if (prev->next != NULL) DISPOSE(prev->next);
        prev->next = cur->next;
        if (cur->next != NULL)
        {
          cur->next->prev = prev;
        }
      }
      else
      {
        room->trail_tracks->head = cur->next;
        if (cur->next != NULL)
        {
          // if (cur->next->prev != NULL) DISPOSE(cur->next->prev);
          cur->next->prev = NULL;
        }
      }
    }
    prev = cur;
  }

  /*
    struct trail_data_list *trail_scent;
    struct trail_data_list *trail_blood;
*/
}

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
  /* used for looping the room for sneak-checks */
  struct char_data *tch = NULL, *next_tch = NULL;
  // for mount code
  int same_room = 0, riding = 0, ridden_by = 0;
  /* extra buffers */
  char buf2[MAX_STRING_LENGTH] = {'\0'};
  //  char buf3[MAX_STRING_LENGTH] = {'\0'};
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
    send_to_char(ch, "The sanctity of the area prevents you from entering.\r\n");
    return 0;
  }
  if (IS_GOOD(ch) && IS_UNHOLY(going_to))
  {
    send_to_char(ch, "The corruption of the area prevents you from entering.\r\n");
    return 0;
  }

  /* Water, No Swimming Rooms: Does the deep water prevent movement? */
  if ((SECT(was_in) == SECT_WATER_NOSWIM) || (SECT(was_in) == SECT_UD_NOSWIM) ||
      (SECT(going_to) == SECT_WATER_NOSWIM) || (SECT(going_to) == SECT_UD_NOSWIM))
  {
    if ((riding && !has_boat(RIDING(ch), going_to)) || !has_boat(ch, going_to))
    {
      send_to_char(ch, "You need a boat to go there.\r\n");
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
                   "You need to be able to breathe underwater to go there!\r\n");
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
    send_to_char(ch, "A mysterious barrier forces you back! That area is off-limits.\r\n");
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
      send_to_char(ch, "There isn't enough room there for more than one person!\r\n");
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

  /* check for traps (leave room) */
  check_trap(ch, TRAP_TYPE_LEAVE_ROOM, ch->in_room, 0, 0);

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
    else if (dir == SOUTH && MOB_FLAGGED(mob, MOB_BLOCK_E))
      block = TRUE;
    else if (dir == WEST && MOB_FLAGGED(mob, MOB_BLOCK_E))
      block = TRUE;
    else if (dir == NORTHEAST && MOB_FLAGGED(mob, MOB_BLOCK_E))
      block = TRUE;
    else if (dir == SOUTHEAST && MOB_FLAGGED(mob, MOB_BLOCK_E))
      block = TRUE;
    else if (dir == SOUTHWEST && MOB_FLAGGED(mob, MOB_BLOCK_E))
      block = TRUE;
    else if (dir == NORTHWEST && MOB_FLAGGED(mob, MOB_BLOCK_E))
      block = TRUE;
    else if (dir == UP && MOB_FLAGGED(mob, MOB_BLOCK_E))
      block = TRUE;
    else if (dir == DOWN && MOB_FLAGGED(mob, MOB_BLOCK_E))
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

    if (block)
      break;
  }

  if (block && !PRF_FLAGGED(ch, PRF_NOHASSLE))
  {
    act("$N blocks you from travelling in that direction.", FALSE, ch, 0, mob, TO_CHAR);
    act("$n tries to leave the room, but $N blocks $m from travelling in that direction.", FALSE, ch, 0, mob, TO_ROOM);
    return 0;
  }

  // acrobatics check

  /* for now acrobatics check disabled */
  /*************************************
  int cantFlee = 0;

  if (affected_by_spell(ch, SPELL_EXPEDITIOUS_RETREAT))
    cantFlee--;
  if (affected_by_spell(ch, SPELL_GREASE))
    cantFlee++;

  if (need_specials_check == 3 &&
          GET_POS(ch) > POS_DEAD && FIGHTING(ch) &&
          IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)) && cantFlee >= 0) {

    //able to flee away with acrobatics check?
    if (!IS_NPC(ch)) { //player
      if (((HAS_FEAT(ch, FEAT_MOBILITY)) || (HAS_FEAT(ch, FEAT_ENHANCED_MOBILITY)) ||
              dice(1, 20) + compute_ability(ch, ABILITY_ACROBATICS) > 15) &&
              cantFlee <= 0) {
        send_to_char(ch, "\tW*Acrobatics Success\tn*");
        send_to_char(FIGHTING(ch), "\tR*Opp Acrobatics Success*\tn");
      } else {
        // failed
        send_to_char(ch, "\tR*Acrobatics Fail\tn*");
        send_to_char(FIGHTING(ch), "\tW*Opp Acrobatics Fail*\tn");
        return 0;
      }
      //npc
    } else {
      if (dice(1, 20) > 10 && cantFlee <= 0) {
        send_to_char(ch, "\tW*Acrobatics Success\tn*");
        send_to_char(FIGHTING(ch), "\tR*Opp Acrobatics Success*\tn");
      } else {
        // failed
        send_to_char(ch, "\tR*Acrobatics Fail\tn*");
        send_to_char(FIGHTING(ch), "\tW*Opp Acrobatics Fail*\tn");
        return 0;
      }
    }
  }

   *****************************/

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
    /*if (GET_MOVE(RIDING(ch)) < need_movement)
    {
      send_to_char(ch, "Your mount is too exhausted.\r\n");
      return 0;
    }*/
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
        send_to_char(ch, "You stop walking to the %s", get_walkto_location_name(GET_WALKTO_LOC(ch)));
        GET_WALKTO_LOC(ch) = 0;
      }

      return (0);
    }
  }

  /* chance of being thrown off mount */
  if (riding && (compute_ability(ch, ABILITY_RIDE) + d20(ch)) <
                    rand_number(1, GET_LEVEL(RIDING(ch))) - rand_number(-4, need_movement))
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

  /*****/
  /* Generate the leave message(s) and display to others in the was_in room. */
  /*****/

  /* silly to keep people reclining when they leave a room */
  /* actually this mechanic is a necessary one, single file room code, sorry folks -zusuk */
  /*
  if (GET_POS(ch) == POS_RECLINING) {
    send_to_char(ch, "You move from a crawling position to standing as you leave the area.\r\n");
    change_position(ch, POS_STANDING);
  }
   */

  /* scenario:  mounted char */
  if (riding)
  {

    /* riding, mount is -not- attempting to sneak */
    if (!IS_AFFECTED(RIDING(ch), AFF_SNEAK))
    {
      /* character is attempting to sneak (mount is not) */
      if (IS_AFFECTED(ch, AFF_SNEAK))
      {
        /* we know the player is trying to sneak, we have to do the
           sneak-check with all the observers in the room */
        for (tch = world[IN_ROOM(ch)].people; tch; tch = next_tch)
        {
          next_tch = tch->next_in_room;

          /* skip self and mount of course */
          if (tch == ch || tch == RIDING(ch))
            continue;

          /* sneak versus listen check */
          if (!can_hear_sneaking(tch, ch))
          {
            /* message:  mount not sneaking, rider is sneaking */
            snprintf(buf2, sizeof(buf2), "$n leaves %s.", dirs[dir]);
            act(buf2, TRUE, RIDING(ch), 0, tch, TO_VICT);
          }
          else
          {
            /* rider detected ! */
            snprintf(buf2, sizeof(buf2), "$n rides %s %s.",
                     GET_NAME(RIDING(ch)), dirs[dir]);
            act(buf2, TRUE, ch, 0, tch, TO_VICT);
          }
        }

        /* character is -not- attempting to sneak (mount not too) */
      }
      else
      {
        snprintf(buf2, sizeof(buf2), "$n rides $N %s.", dirs[dir]);
        act(buf2, TRUE, ch, 0, RIDING(ch), TO_NOTVICT);
      }
    } /* riding, mount -is- attempting to sneak, if succesful, the whole
       * "package" is sneaking, if not ch might be able to sneak still */
    else
    {
      if (!IS_AFFECTED(ch, AFF_SNEAK))
      {
        /* we know the mount (and not ch) is trying to sneak, we have to do the
           sneak-check with all the observers in the room */
        for (tch = world[IN_ROOM(RIDING(ch))].people; tch; tch = next_tch)
        {
          next_tch = tch->next_in_room;

          /* skip self (mount) of course and ch */
          if (tch == RIDING(ch) || tch == ch)
            continue;

          /* sneak versus listen check */
          if (can_hear_sneaking(tch, RIDING(ch)))
          {
            /* mount detected! */
            snprintf(buf2, sizeof(buf2), "$n rides %s %s.",
                     GET_NAME(RIDING(ch)), dirs[dir]);
            act(buf2, TRUE, ch, 0, tch, TO_VICT);
          } /* if we pass this check, the rider/mount are both sneaking */
        }
      } /* ch is still trying to sneak (mount too) */
      else
      {
        /* we know the mount (and ch) is trying to sneak, we have to do the
           sneak-check with all the observers in the room, mount
         * success in this case is free pass for sneak */
        for (tch = world[IN_ROOM(RIDING(ch))].people; tch; tch = next_tch)
        {
          next_tch = tch->next_in_room;

          /* skip self (mount) of course, skipping ch too */
          if (tch == RIDING(ch) || tch == ch)
            continue;

          /* sneak versus listen check */
          if (!can_hear_sneaking(tch, RIDING(ch)))
          {
            /* mount success!  "package" is sneaking */
          }
          else if (!can_hear_sneaking(tch, ch))
          {
            /* mount failed, player succeeded */
            /* message:  mount not sneaking, rider is sneaking */
            snprintf(buf2, sizeof(buf2), "$n leaves %s.", dirs[dir]);
            act(buf2, TRUE, RIDING(ch), 0, tch, TO_VICT);
          }
          else
          {
            /* mount failed, player failed */
            snprintf(buf2, sizeof(buf2), "$n rides %s %s.",
                     GET_NAME(RIDING(ch)), dirs[dir]);
            act(buf2, TRUE, ch, 0, tch, TO_VICT);
          }
        }
      }
    }
    /* message to self */
    send_to_char(ch, "You ride %s %s.\r\n", GET_NAME(RIDING(ch)), dirs[dir]);
    /* message to mount */
    send_to_char(RIDING(ch), "You carry %s %s.\r\n",
                 GET_NAME(ch), dirs[dir]);
  } /* end:  mounted char */

  /* scenario:  char is mount */
  else if (ridden_by)
  {

    /* ridden and mount-char is -not- attempting to sneak
       will either see whole 'package' move or just the mounted-char */
    if (!IS_AFFECTED(ch, AFF_SNEAK))
    {

      /* char's rider is attempting to sneak (mount-char is not)
         either going to see mount or 'package' move */
      if (IS_AFFECTED(RIDDEN_BY(ch), AFF_SNEAK))
      {
        /* we know the rider is trying to sneak, we have to do the
           sneak-check with all the observers in the room */
        for (tch = world[IN_ROOM(ch)].people; tch; tch = next_tch)
        {
          next_tch = tch->next_in_room;

          /* skip self and rider of course */
          if (tch == ch || tch == RIDDEN_BY(ch))
            continue;

          /* sneak versus listen check */
          if (!can_hear_sneaking(tch, RIDDEN_BY(ch)))
          {
            /* message:  mount not sneaking, rider is sneaking */
            snprintf(buf2, sizeof(buf2), "$n leaves %s.", dirs[dir]);
            act(buf2, TRUE, ch, 0, tch, TO_VICT);
          }
          else
          {
            /* rider detected ! */
            snprintf(buf2, sizeof(buf2), "$n rides %s %s.",
                     GET_NAME(ch), dirs[dir]);
            act(buf2, TRUE, RIDDEN_BY(ch), 0, tch, TO_VICT);
          }
        }

        /* rider is -not- attempting to sneak (mount-char not too) */
      }
      else
      {
        snprintf(buf2, sizeof(buf2), "$n rides $N %s.", dirs[dir]);
        act(buf2, TRUE, RIDDEN_BY(ch), 0, ch, TO_NOTVICT);
      }
    } /* ridden and mount-char -is- attempting to sneak */
    else
    {

      /* both are attempt to sneak */
      if (IS_AFFECTED(RIDDEN_BY(ch), AFF_SNEAK))
      {
        /* we know the mount and rider is trying to sneak, we have to do the
           sneak-check with all the observers in the room, mount (ch)
         * success in this case is free pass for sneak */
        for (tch = world[IN_ROOM(RIDDEN_BY(ch))].people; tch; tch = next_tch)
        {
          next_tch = tch->next_in_room;

          /* skip rider of course, skipping mount-ch too */
          if (tch == RIDDEN_BY(ch) || tch == ch)
            continue;

          /* sneak versus listen check */
          if (!can_hear_sneaking(tch, ch))
          {
            /* mount success!  "package" is sneaking */
          }
          else if (!can_hear_sneaking(tch, RIDDEN_BY(ch)))
          {
            /* mount failed, rider succeeded */
            /* message:  mount not sneaking, rider is sneaking */
            snprintf(buf2, sizeof(buf2), "$n leaves %s.", dirs[dir]);
            act(buf2, TRUE, RIDDEN_BY(ch), 0, tch, TO_VICT);
          }
          else
          {
            /* mount failed, rider failed */
            /* 3.23.18 Ornir Bugfix. */
            snprintf(buf2, sizeof(buf2), "$n rides %s %s.",
                     GET_NAME(RIDING(ch)), dirs[dir]);
            act(buf2, TRUE, RIDDEN_BY(ch), 0, tch, TO_VICT);
          }
        }

        /* ridden and mount-char -is- attempt to sneak, rider -not- */
      }
      else
      {
        /* we know the mount (rider no) is trying to sneak, we have to do the
           sneak-check with all the observers in the room */
        for (tch = world[IN_ROOM(ch)].people; tch; tch = next_tch)
        {
          next_tch = tch->next_in_room;

          /* skip self (mount) and rider */
          if (tch == RIDDEN_BY(ch) || tch == ch)
            continue;

          /* sneak versus listen check */
          if (can_hear_sneaking(tch, ch))
          {
            /* mount detected! */
            snprintf(buf2, sizeof(buf2), "$n rides %s %s.",
                     GET_NAME(RIDING(ch)), dirs[dir]);
            act(buf2, TRUE, ch, 0, tch, TO_VICT);
          } /* if we pass this check, the rider/mount are both sneaking */
        }
      }
    }
    /* message to self */
    send_to_char(ch, "You carry %s %s.\r\n",
                 GET_NAME(RIDDEN_BY(ch)), dirs[dir]);
    /* message to rider */
    send_to_char(RIDDEN_BY(ch), "You are carried %s by %s.\r\n",
                 dirs[dir], GET_NAME(ch));
  } /* end char is mounted */

  /* ch is on foot */
  else if (IS_AFFECTED(ch, AFF_SNEAK))
  {
    /* sneak attempt vs the room content */
    for (tch = world[IN_ROOM(ch)].people; tch; tch = next_tch)
    {
      next_tch = tch->next_in_room;

      /* skip self */
      if (tch == ch)
        continue;

      /* sneak versus listen check */
      if (can_hear_sneaking(tch, ch))
      {
        /* detected! */
        if (IS_NPC(ch) && (ch->player.walkout != NULL))
        {
          // if they have a walk-out message, display that instead of the boring default one
          snprintf(buf2, sizeof(buf2), "%s %s.", ch->player.walkout, dirs[dir]);
          act(buf2, TRUE, ch, 0, tch, TO_VICT);
        }
        else
        {
          snprintf(buf2, sizeof(buf2), "$n leaves %s.", dirs[dir]);
          act(buf2, TRUE, ch, 0, tch, TO_VICT);
        }
      } /* if we pass this check, we are sneaking */
    }
    /* message to self */
    send_to_char(ch, "You sneak %s.\r\n", dirs[dir]);
  } /* not attempting to sneak */
  else if (!IS_AFFECTED(ch, AFF_SNEAK))
  {
    if (IS_NPC(ch) && (ch->player.walkout != NULL))
    {
      // if they have a walk-out message, display that instead of the boring default one
      snprintf(buf2, sizeof(buf2), "%s %s.", ch->player.walkout, dirs[dir]);
      act(buf2, TRUE, ch, 0, tch, TO_ROOM);
    }
    else
    {
      snprintf(buf2, sizeof(buf2), "$n leaves %s.", dirs[dir]);
      act(buf2, TRUE, ch, 0, 0, TO_ROOM);
    }
    /* message to self */
    send_to_char(ch, "You leave %s.\r\n", dirs[dir]);
  }
  /*****/
  /* end leave-room message code */
  /*****/

  /* Leave tracks, if not riding. */
  if (!riding && (IS_NPC(ch) || !PRF_FLAGGED(ch, PRF_NOHASSLE)))
  {
    /*snprintf(buf3, sizeof(buf3), "%d \"%s\" \"%s\" %s", 6,
                                   (IS_NPC(ch) ? race_family_types[GET_NPC_RACE(ch)] : race_list[GET_RACE(ch)].type),
                                   GET_NAME(ch),
                                   dirs[dir]);
      */
    create_tracks(ch, dir, TRACKS_OUT);
  }

  /* the actual technical moving of the char */
  char_from_room(ch);

  X_LOC(ch) = new_x;
  Y_LOC(ch) = new_y;

  char_to_room(ch, going_to);

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

  /*****/
  /* Generate the enter message(s) and display to others in the arrive room. */
  /* This changes stock behavior: it doesn't work                            *
   *          with in/out/enter/exit as dirs                                 */
  /*****/
  if (!riding && !ridden_by)
  {
    /* simplest case, not riding or being ridden-by */
    for (tch = world[IN_ROOM(ch)].people; tch; tch = next_tch)
    {
      next_tch = tch->next_in_room;

      /* skip self */
      if (tch == RIDDEN_BY(ch) || tch == ch)
        continue;

      /* sneak versus listen check */
      if (can_hear_sneaking(tch, ch))
      {
        /* failed sneak attempt (if valid) */
        if (IS_NPC(ch) && ch->player.walkin)
        {
          snprintf(buf2, sizeof(buf2), "%s %s%s.", ch->player.walkin,
                   ((dir == UP || dir == DOWN) ? "" : "the "),
                   (dir == UP ? "below" : dir == DOWN ? "above"
                                                      : dirs[rev_dir[dir]]));
        }
        else
        {
          snprintf(buf2, sizeof(buf2), "$n arrives from %s%s.",
                   ((dir == UP || dir == DOWN) ? "" : "the "),
                   (dir == UP ? "below" : dir == DOWN ? "above"
                                                      : dirs[rev_dir[dir]]));
        }
        act(buf2, TRUE, ch, 0, tch, TO_VICT);
      }
    }
  }
  else if (riding)
  {
    for (tch = world[IN_ROOM(RIDING(ch))].people; tch; tch = next_tch)
    {
      next_tch = tch->next_in_room;

      /* skip rider of course, and mount */
      if (tch == RIDING(ch) || tch == ch)
        continue;

      /* sneak versus listen check */
      if (!can_hear_sneaking(tch, RIDING(ch)))
      {
        /* mount success!  "package" is sneaking */
      }
      else if (!can_hear_sneaking(tch, ch))
      {
        /* mount failed, rider succeeded */
        /* message:  mount not sneaking, rider is sneaking */
        snprintf(buf2, sizeof(buf2), "$n arrives from %s%s.",
                 ((dir == UP || dir == DOWN) ? "" : "the "),
                 (dir == UP ? "below" : dir == DOWN ? "above"
                                                    : dirs[rev_dir[dir]]));
        act(buf2, TRUE, RIDING(ch), 0, tch, TO_VICT);
      }
      else
      {
        /* mount failed, rider failed */
        snprintf(buf2, sizeof(buf2), "$n arrives from %s%s, riding %s.",
                 ((dir == UP || dir == DOWN) ? "" : "the "),
                 (dir == UP ? "below" : dir == DOWN ? "above"
                                                    : dirs[rev_dir[dir]]),
                 GET_NAME(RIDING(ch)));
        act(buf2, TRUE, ch, 0, tch, TO_VICT);
      }
    }
  }
  else if (ridden_by)
  {
    for (tch = world[IN_ROOM(RIDDEN_BY(ch))].people; tch; tch = next_tch)
    {
      next_tch = tch->next_in_room;

      /* skip rider of course, and mount */
      if (tch == RIDDEN_BY(ch) || tch == ch)
        continue;

      /* sneak versus listen check, remember ch = mount right now  */
      if (!can_hear_sneaking(tch, ch))
      {
        /* mount success!  "package" is sneaking */
      }
      else if (!can_hear_sneaking(tch, RIDDEN_BY(ch)))
      {
        /* mount failed, rider succeeded */
        /* message:  mount not sneaking, rider is sneaking */
        snprintf(buf2, sizeof(buf2), "$n arrives from %s%s.",
                 ((dir == UP || dir == DOWN) ? "" : "the "),
                 (dir == UP ? "below" : dir == DOWN ? "above"
                                                    : dirs[rev_dir[dir]]));
        act(buf2, TRUE, ch, 0, tch, TO_VICT);
      }
      else
      {
        /* mount failed, rider failed */
        snprintf(buf2, sizeof(buf2), "$n arrives from %s%s, ridden by %s.",
                 ((dir == UP || dir == DOWN) ? "" : "the "),
                 (dir == UP ? "below" : dir == DOWN ? "above"
                                                    : dirs[rev_dir[dir]]),
                 GET_NAME(RIDDEN_BY(ch)));
        act(buf2, TRUE, RIDDEN_BY(ch), 0, tch, TO_VICT);
      }
    }
  }

  /*****/
  /* end enter-room message code */
  /*****/

  /* Maybe a wall will stop them? */
  if (check_wall(ch, rev_dir[dir]))
  {
    /* send them back! */
    char_from_room(ch);
    char_to_room(ch, was_in);
    return 0;
  }

  /* spike growth damages upon entering the room */
  if (ROOM_AFFECTED(going_to, RAFF_SPIKE_STONES))
  {
    /* only damage the character if they're not mounted (mount takes damage) */
    if (riding && same_room)
    {
      // mount will take the damage, don't hurt rider
      /* damage characters upon entering spike growth room */
      damage(RIDING(ch), RIDING(ch), dice(4, 4), SPELL_SPIKE_STONES, DAM_EARTH, FALSE);
      send_to_char(RIDING(ch), "You are impaled by large stone spikes as you enter the room.\r\n");
    }
    else
    {
      // mount is not there, or not mounted
      damage(ch, ch, dice(4, 4), SPELL_SPIKE_STONES, DAM_EARTH, FALSE);
      send_to_char(ch, "You are impaled by large stone spikes as you enter the room.\r\n");
    }
  }
  if (ROOM_AFFECTED(going_to, RAFF_SPIKE_GROWTH))
  {
    /* only damage the character if they're not mounted (mount takes damage) */
    if (riding && same_room)
    {
      // mount will take the damage, don't hurt rider
      /* damage characters upon entering spike growth room */
      damage(RIDING(ch), RIDING(ch), dice(2, 4), SPELL_SPIKE_GROWTH, DAM_EARTH, FALSE);
      send_to_char(RIDING(ch), "You are impaled by large spikes as you enter the room.\r\n");
    }
    else
    {
      // mount is not there, or not mounted
      damage(ch, ch, dice(2, 4), SPELL_SPIKE_GROWTH, DAM_EARTH, FALSE);
      send_to_char(ch, "You are impaled by large spikes as you enter the room.\r\n");
    }
  }

  /************Death traps have been taken out*************/
  /* ... and Kill the player if the room is a death trap.
  if (ROOM_FLAGGED(going_to, ROOM_DEATH)) {
    if (GET_LEVEL(ch) < LVL_IMMORT) {
      mudlog(BRF, LVL_IMMORT, TRUE, "%s hit death trap #%d (%s)",
   * GET_NAME(ch), GET_ROOM_VNUM(going_to), world[going_to].name);
      death_cry(ch);
      extract_char(ch);
    }

    if (riding && GET_LEVEL(RIDING(ch)) < LVL_IMMORT) {
      mudlog(BRF, LVL_IMMORT, TRUE, "%s hit death trap #%d (%s)",
     GET_NAME(RIDING(ch)), GET_ROOM_VNUM(going_to), world[going_to].name);
      death_cry(RIDING(ch));
      extract_char(RIDING(ch));
    }

    if (ridden_by && GET_LEVEL(RIDDEN_BY(ch)) < LVL_IMMORT) {
      mudlog(BRF, LVL_IMMORT, TRUE, "%s hit death trap #%d (%s)",
     GET_NAME(RIDDEN_BY(ch)), GET_ROOM_VNUM(going_to), world[going_to].name);
      death_cry(RIDDEN_BY(ch));
      extract_char(RIDDEN_BY(ch));
    }
    return (0);
  }
   ************end death trap code**********/

  /* At this point, the character is safe and in the room. */

  /* Fire memory and greet triggers, check and see if the greet trigger
   * prevents movement, and if so, move the player back to the previous room. */
  entry_memory_mtrigger(ch);

  if (!greet_mtrigger(ch, dir))
  {
    char_from_room(ch);

    if (ZONE_FLAGGED(GET_ROOM_ZONE(was_in), ZONE_WILDERNESS))
    {
      X_LOC(ch) = world[was_in].coords[0];
      Y_LOC(ch) = world[was_in].coords[1];
    }

    char_to_room(ch, was_in);
    look_at_room(ch, 0);

    /* Failed move, return a failure */
    return (0);
  }
  else
    greet_memory_mtrigger(ch);
  /*---------------------------------------------------------------------*/
  /* End: Post-move operations. */

  /* Only here is the move successful *and* complete. Return success for
   * calling functions to handle post move operations. */

  /* homeland-port */
  if (IS_NPC(ch))
    quest_room(ch);

  /* trap sense will allow a rogue/berserker to auto detect traps if they
   make a successful check vs DC xx (defined right below ) */
  bool sensed_trap = FALSE;
  if (!IS_NPC(ch))
  {
    int trap_check = 0;
    int dc = 21;
    if ((trap_check = HAS_FEAT(ch, FEAT_TRAP_SENSE)))
    {
      if (skill_check(ch, ABILITY_PERCEPTION, (dc - trap_check)))
        sensed_trap = perform_detecttrap(ch, TRUE); /* silent */
    }
  }

  /* check for traps (enter room) */
  if (!sensed_trap)
    check_trap(ch, TRAP_TYPE_ENTER_ROOM, ch->in_room, 0, 0);

  if (!ch->master)
  {
    // set cooldown timer on old room
    if (was_in != NOWHERE && ZONE_FLAGGED(GET_ROOM_ZONE(was_in), ZONE_WILDERNESS))
    {
      set_expire_cooldown(was_in);
    }
    if (ZONE_FLAGGED(GET_ROOM_ZONE(ch->in_room), ZONE_WILDERNESS))
    {
      check_random_encounter(ch);
      reset_expire_cooldown(ch->in_room);
      check_hunt_room(ch->in_room);
    }
  }

  if (HAS_FEAT(ch, FEAT_VAMPIRE_WEAKNESSES) && GET_LEVEL(ch) < LVL_IMMORT &&
      !affected_by_spell(ch, AFFECT_RECENTLY_DIED) && !affected_by_spell(ch, AFFECT_RECENTLY_RESPECED))
  {
    if (IN_SUNLIGHT(ch) && !is_covered(ch))
    {
      damage(ch, ch, dice(1, 6), TYPE_SUN_DAMAGE, DAM_SUNLIGHT, FALSE);
    }
    if (IN_MOVING_WATER(ch))
    {
      damage(ch, ch, GET_MAX_HIT(ch) / 3, TYPE_MOVING_WATER, DAM_MOVING_WATER, FALSE);
    }
  }

  return (1);
}

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

static int find_door(struct char_data *ch, const char *type, char *dir, const char *cmdname)
{
  int door;

  if (*dir)
  { /* a direction was specified */
    if ((door = search_block(dir, dirs, FALSE)) == -1)
    { /* Partial Match */
      if ((door = search_block(dir, autoexits, FALSE)) == -1)
      { /* Check 'short' dirs too */
        send_to_char(ch, "That's not a direction.\r\n");
        return (-1);
      }
    }
    if (EXIT(ch, door))
    { /* Braces added according to indent. -gg */
      if (EXIT(ch, door)->keyword)
      {
        if (is_name(type, EXIT(ch, door)->keyword))
          return (door);
        else
        {
          send_to_char(ch, "I see no %s there.\r\n", type);
          return (-1);
        }
      }
      else
        return (door);
    }
    else
    {
      send_to_char(ch, "I really don't see how you can %s anything there.\r\n", cmdname);
      return (-1);
    }
  }
  else
  { /* try to locate the keyword */
    if (!*type)
    {
      send_to_char(ch, "What is it you want to %s?\r\n", cmdname);
      return (-1);
    }
    for (door = 0; door < DIR_COUNT; door++)
    {
      if (EXIT(ch, door))
      {
        if (EXIT(ch, door)->keyword)
        {
          if (isname(type, EXIT(ch, door)->keyword) || is_abbrev(type, dirs[door]))
          {
            if (EXIT_FLAGGED(EXIT(ch, door), EX_HIDDEN) && GET_LEVEL(ch) < LVL_IMMORT)
              ;
            else if ((!IS_NPC(ch)) && (!PRF_FLAGGED(ch, PRF_AUTODOOR)))
              return door;
            else if (is_abbrev(cmdname, "open"))
            {
              if (IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))
                return door;
              else if (IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED))
                return door;
            }
            else if ((is_abbrev(cmdname, "close")) && (!(IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))))
              return door;
            else if ((is_abbrev(cmdname, "lock")) && (!(IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED))))
              return door;
            else if ((is_abbrev(cmdname, "unlock")) && (IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED)))
              return door;
            else if ((is_abbrev(cmdname, "pick")) && (IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED)))
              return door;
          }
        }
      }
    }

    if ((!IS_NPC(ch)) && (!PRF_FLAGGED(ch, PRF_AUTODOOR)))
      send_to_char(ch, "There doesn't seem to be %s %s here.\r\n", AN(type), type);
    else if (is_abbrev(cmdname, "open"))
      send_to_char(ch, "There doesn't seem to be %s %s that can be opened.\r\n", AN(type), type);
    else if (is_abbrev(cmdname, "close"))
      send_to_char(ch, "There doesn't seem to be %s %s that can be closed.\r\n", AN(type), type);
    else if (is_abbrev(cmdname, "lock"))
      send_to_char(ch, "There doesn't seem to be %s %s that can be locked.\r\n", AN(type), type);
    else if (is_abbrev(cmdname, "unlock"))
      send_to_char(ch, "There doesn't seem to be %s %s that can be unlocked.\r\n", AN(type), type);
    else
      send_to_char(ch, "There doesn't seem to be %s %s that can be picked.\r\n", AN(type), type);

    return (-1);
  }
}

#define PRISONER_KEY_1 132130
#define PRISONER_KEY_2 132129
#define PRISONER_KEY_3 132150
/* this function will destroy the keyvnums that go through it so they can't be horded, called by has_key() */
int is_evaporating_key(struct char_data *ch, obj_vnum key)
{
  if (!IS_NPC(ch) && GET_LEVEL(ch) >= LVL_IMMORT && PRF_FLAGGED(ch, PRF_NOHASSLE))
    return (TRUE);

  struct obj_data *o = NULL;

  for (o = ch->carrying; o; o = o->next_content)
  {
    if (GET_OBJ_VNUM(o) == key)
    {
      act("$n breaks $p, unlocking the path!", FALSE, ch, o, 0, TO_ROOM);
      act("You break $p, unlocking the path!", FALSE, ch, o, 0, TO_CHAR);
      extract_obj(o);
      return TRUE;
    }
  }

  if (GET_EQ(ch, WEAR_HOLD_1))
  {
    if (GET_OBJ_VNUM(GET_EQ(ch, WEAR_HOLD_1)) == key)
    {
      act("$n breaks $p, unlocking the path!", FALSE, ch, GET_EQ(ch, WEAR_HOLD_1), 0, TO_ROOM);
      act("You break $p, unlocking the path!", FALSE, ch, GET_EQ(ch, WEAR_HOLD_1), 0, TO_CHAR);
      extract_obj(unequip_char(ch, WEAR_HOLD_1));
      return TRUE;
    }
  }

  if (GET_EQ(ch, WEAR_HOLD_2))
  {
    if (GET_OBJ_VNUM(GET_EQ(ch, WEAR_HOLD_2)) == key)
    {
      act("$n breaks $p, unlocking the path!", FALSE, ch, GET_EQ(ch, WEAR_HOLD_2), 0, TO_ROOM);
      act("You break $p, unlocking the path!", FALSE, ch, GET_EQ(ch, WEAR_HOLD_2), 0, TO_CHAR);
      extract_obj(unequip_char(ch, WEAR_HOLD_2));
      return TRUE;
    }
  }

  return FALSE;
}

/* this function checks that ch has in inventory or held an object with matching vnum as the door-key value
     note - added a check for NOTHING or -1 vnum key value so corpses can't be used to open these doors -zusuk */
int has_key(struct char_data *ch, obj_vnum key)
{

  /* special key handling */
  switch (key)
  {

    /* these keys will break upon usage so they can't be horded */
  case PRISONER_KEY_1:
  /*fallthrough*/
  case PRISONER_KEY_2:
  /*fallthrough*/
  case PRISONER_KEY_3:
    return (is_evaporating_key(ch, key));

  default:
    break;
  }

  if (!IS_NPC(ch) && GET_LEVEL(ch) >= LVL_IMMORT && PRF_FLAGGED(ch, PRF_NOHASSLE))
    return (1);

  /* debug */
  /* send_to_char(ch, "key vnum: %d", key); */

  /* players were using corpses to open doors */
  if (key == NOTHING || key <= 0)
    return (0);

  struct obj_data *o = NULL;

  for (o = ch->carrying; o; o = o->next_content)
    if (GET_OBJ_VNUM(o) == key)
      return (1);

  if (GET_EQ(ch, WEAR_HOLD_1))
    if (GET_OBJ_VNUM(GET_EQ(ch, WEAR_HOLD_1)) == key)
      return (1);

  if (GET_EQ(ch, WEAR_HOLD_2))
    if (GET_OBJ_VNUM(GET_EQ(ch, WEAR_HOLD_2)) == key)
      return (1);

  return (0);
}

#define NEED_OPEN (1 << 0)
#define NEED_CLOSED (1 << 1)
#define NEED_UNLOCKED (1 << 2)
#define NEED_LOCKED (1 << 3)

/* cmd_door is required external from act.movement.c */
const char *const cmd_door[] = {
    "open",
    "close",
    "unlock",
    "lock",
    "pick"};

static const int flags_door[] = {
    NEED_CLOSED | NEED_UNLOCKED,
    NEED_OPEN,
    NEED_CLOSED | NEED_LOCKED,
    NEED_CLOSED | NEED_UNLOCKED,
    NEED_CLOSED | NEED_LOCKED};

static void do_doorcmd(struct char_data *ch, struct obj_data *obj, int door, int scmd)
{
  char buf[MAX_STRING_LENGTH] = {'\0'};
  size_t len;
  room_rnum other_room = NOWHERE;
  struct room_direction_data *back = NULL;

  if (!door_mtrigger(ch, scmd, door))
    return;

  if (!door_wtrigger(ch, scmd, door))
    return;

  len = snprintf(buf, sizeof(buf), "$n %ss ", cmd_door[scmd]);
  if (!obj && ((other_room = EXIT(ch, door)->to_room) != NOWHERE))
    if ((back = world[other_room].dir_option[rev_dir[door]]) != NULL)
      if (back->to_room != IN_ROOM(ch))
        back = NULL;

  switch (scmd)
  {
  case SCMD_OPEN:
    if (obj)
    {
      if (check_trap(ch, TRAP_TYPE_OPEN_CONTAINER, ch->in_room, obj, 0))
        return;
    }
    else
    {
      if (check_trap(ch, TRAP_TYPE_OPEN_DOOR, ch->in_room, 0, door))
        return;
    }
    OPEN_DOOR(IN_ROOM(ch), obj, door);
    if (back)
      OPEN_DOOR(other_room, obj, rev_dir[door]);
    send_to_char(ch, "%s", CONFIG_OK);
    break;

  case SCMD_CLOSE:
    if (obj)
    {
      if (check_trap(ch, TRAP_TYPE_OPEN_CONTAINER, ch->in_room, obj, 0))
        return;
    }
    else
    {
      if (check_trap(ch, TRAP_TYPE_OPEN_DOOR, ch->in_room, 0, door))
        return;
    }
    CLOSE_DOOR(IN_ROOM(ch), obj, door);
    if (back)
      CLOSE_DOOR(other_room, obj, rev_dir[door]);
    send_to_char(ch, "%s", CONFIG_OK);
    break;

  case SCMD_LOCK:
    LOCK_DOOR(IN_ROOM(ch), obj, door);
    if (back)
      LOCK_DOOR(other_room, obj, rev_dir[door]);
    send_to_char(ch, "*Click*\r\n");
    break;

  case SCMD_UNLOCK:
    if (obj)
    {
      if (check_trap(ch, TRAP_TYPE_UNLOCK_CONTAINER, ch->in_room, obj, 0))
        return;
    }
    else
    {
      if (check_trap(ch, TRAP_TYPE_UNLOCK_DOOR, ch->in_room, 0, door))
        return;
    }
    UNLOCK_DOOR(IN_ROOM(ch), obj, door);
    if (back)
      UNLOCK_DOOR(other_room, obj, rev_dir[door]);
    send_to_char(ch, "*Click*\r\n");
    break;

  case SCMD_PICK:
    if (obj)
    {
      if (GET_OBJ_TYPE(obj) != ITEM_CONTAINER)
      {
        send_to_char(ch, "That item cannot be picked.\r\n");
        return;
      }
      if (check_trap(ch, TRAP_TYPE_UNLOCK_CONTAINER, ch->in_room, obj, 0))
        return;
      if (DOOR_IS_PICKPROOF(ch, obj, door))
      {
        send_to_char(ch, "That item cannot be picked.\r\n");
        return;
      }
      if (GET_SKILL(ch, ABILITY_DISABLE_DEVICE) > 0 && skill_check(ch, ABILITY_DISABLE_DEVICE, 15))
      {
        TOGGLE_LOCK(IN_ROOM(ch), obj, door);
        send_to_char(ch, "The lock quickly yields to your skills.\r\n");
        len = strlcpy(buf, "$n skillfully picks the lock on ", sizeof(buf));
      }
      else
      {
        send_to_char(ch, "You fail to pick the lock.\r\n");
        len = strlcpy(buf, "$n fails to pick the lock on ", sizeof(buf));
        send_to_char(ch, "Your next action will be delayed up to 6 seconds.\r\n");
      }
    }
    else
    {
      // if (check_trap(ch, TRAP_TYPE_UNLOCK_DOOR, ch->in_room, 0, door))
      //   return;

      if (!ok_pick(ch, 0, EXIT_FLAGGED(EXIT(ch, door), EX_PICKPROOF), SCMD_PICK, door))
      {
        send_to_char(ch, "Your next action will be delayed up to 6 seconds.\r\n");
        WAIT_STATE(ch, PULSE_VIOLENCE * 1);
        return;
      }
      TOGGLE_LOCK(IN_ROOM(ch), obj, door);
      if (back)
        TOGGLE_LOCK(other_room, obj, rev_dir[door]);
      send_to_char(ch, "The lock quickly yields to your skills.\r\n");
      len = strlcpy(buf, "$n skillfully picks the lock on ", sizeof(buf));
    }
    break;
  }

  /* Notify the room. */
  if (len < sizeof(buf))
    snprintf(buf + len, sizeof(buf) - len, "%s%s.",
             obj ? "" : "the ", obj ? "$p" : EXIT(ch, door)->keyword ? "$F"
                                                                     : "door");
  if (!obj || IN_ROOM(obj) != NOWHERE)
    act(buf, FALSE, ch, obj, obj ? 0 : EXIT(ch, door)->keyword, TO_ROOM);

  /* Notify the other room */
  if (back && (scmd == SCMD_OPEN || scmd == SCMD_CLOSE))
    send_to_room(EXIT(ch, door)->to_room, "The %s is %s%s from the other side.\r\n",
                 back->keyword ? fname(back->keyword) : "door", cmd_door[scmd],
                 scmd == SCMD_CLOSE ? "d" : "ed");

  /* Door actions are a move action. */
  USE_MOVE_ACTION(ch);
}

int ok_pick(struct char_data *ch, obj_vnum keynum, int pickproof, int scmd, int door)
{
  int skill_lvl;
  int lock_dc = 10;
  // struct obj_data *tools = NULL;

  if (scmd != SCMD_PICK)
    return (1);

  skill_lvl = compute_ability(ch, ABILITY_SLEIGHT_OF_HAND);
  if (affected_by_spell(ch, PSIONIC_BREACH))
  {
    affect_from_char(ch, PSIONIC_BREACH);
  }

  /* this is a hack of sorts, we have some abuse of charmies being used to pick
     locks, so we add some penalties and restirctions here */
  if (IS_NPC(ch))
  {
    skill_lvl -= 4; /* wheeeeeee */
    switch (GET_RACE(ch))
    {
    case RACE_TYPE_UNDEAD:
    case RACE_TYPE_ANIMAL:
    case RACE_TYPE_DRAGON:
    case RACE_TYPE_MAGICAL_BEAST:
    case RACE_TYPE_OOZE:
    case RACE_TYPE_PLANT:
    case RACE_TYPE_VERMIN:
      send_to_char(ch, "What makes you think you know how to do this?!\r\n");
      return (0);
    default:
      /* we will let them pick */
      break;
    }
  }
  /* end npc hack */

  if (skill_lvl <= 0)
  { /* not an untrained skill */
    send_to_char(ch, "You have no idea how (train sleight of hand)!\r\n");
    return (0);
  }

  if (FIGHTING(ch))
    skill_lvl += d20(ch);
  else
    skill_lvl += MAX(20, d20(ch)); // take 20

  /* thief tools */
  /*
  if ((tools = get_obj_in_list_vis(ch, "thieves,tools", NULL, ch->carrying))) {
    if (GET_OBJ_VNUM(tools) == 105)
      skill_lvl += 2;
  }
   */

  if (EXIT(ch, door))
  {
    if (EXIT_FLAGGED(EXIT(ch, door), EX_LOCKED_EASY))
      lock_dc += 14;
    else if (EXIT_FLAGGED(EXIT(ch, door), EX_LOCKED_MEDIUM))
      lock_dc += 25;
    else if (EXIT_FLAGGED(EXIT(ch, door), EX_LOCKED_HARD))
      lock_dc += 38;
  }

  if (keynum == NOTHING)
  {
    send_to_char(ch, "Odd - you can't seem to find a keyhole.\r\n");
  }
  else if (pickproof)
  {
    send_to_char(ch, "It resists your attempts to pick it.\r\n");
  }
  else if (lock_dc <= skill_lvl)
  {
    send_to_char(ch, "Success! [%d dc vs. %d skill]\r\n", lock_dc, skill_lvl);
    USE_MOVE_ACTION(ch);
    return (1);
  }

  /* failed */
  USE_MOVE_ACTION(ch);
  return (0);
}

ACMD(do_gen_door)
{
  int door = -1;
  obj_vnum keynum;
  char type[MAX_INPUT_LENGTH] = {'\0'}, dir[MAX_INPUT_LENGTH] = {'\0'};
  struct obj_data *obj = NULL;
  struct char_data *victim = NULL;

  skip_spaces_c(&argument);

  if (!*argument)
  {
    send_to_char(ch, "%c%s what?\r\n", UPPER(*cmd_door[subcmd]), cmd_door[subcmd] + 1);
    return;
  }

  two_arguments(argument, type, sizeof(type), dir, sizeof(dir));

  if (!generic_find(type, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &victim, &obj))
    door = find_door(ch, type, dir, cmd_door[subcmd]);

  if ((obj) && (GET_OBJ_TYPE(obj) != ITEM_CONTAINER &&
                GET_OBJ_TYPE(obj) != ITEM_AMMO_POUCH))
  {
    obj = NULL;
    door = find_door(ch, type, dir, cmd_door[subcmd]);
  }

  if ((obj) || (door >= 0))
  {
    keynum = DOOR_KEY(ch, obj, door);
    if (!(DOOR_IS_OPENABLE(ch, obj, door)))
      send_to_char(ch, "You can't %s that!\r\n", cmd_door[subcmd]);
    else if (!DOOR_IS_OPEN(ch, obj, door) &&
             IS_SET(flags_door[subcmd], NEED_OPEN))
      send_to_char(ch, "But it's already closed!\r\n");
    else if (!DOOR_IS_CLOSED(ch, obj, door) &&
             IS_SET(flags_door[subcmd], NEED_CLOSED))
      send_to_char(ch, "But it's currently open!\r\n");
    else if (!(DOOR_IS_LOCKED(ch, obj, door)) &&
             IS_SET(flags_door[subcmd], NEED_LOCKED))
      send_to_char(ch, "Oh.. it wasn't locked, after all..\r\n");
    else if (!(DOOR_IS_UNLOCKED(ch, obj, door)) && IS_SET(flags_door[subcmd], NEED_UNLOCKED) &&
             ((!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOKEY))) && (has_key(ch, keynum)))
    {
      send_to_char(ch, "It is locked, but you have the key.\r\n");
      do_doorcmd(ch, obj, door, SCMD_UNLOCK);
      send_to_char(ch, "*Click*\r\n");
      do_doorcmd(ch, obj, door, subcmd);
      ch->char_specials.autodoor_message = true;
    }
    else if (!(DOOR_IS_UNLOCKED(ch, obj, door)) && IS_SET(flags_door[subcmd], NEED_UNLOCKED) &&
             ((!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOKEY))) && (!has_key(ch, keynum)))
    {
      send_to_char(ch, "It is locked, and you do not have the key!\r\n");
    }
    else if (!(DOOR_IS_UNLOCKED(ch, obj, door)) &&
             IS_SET(flags_door[subcmd], NEED_UNLOCKED) &&
             (GET_LEVEL(ch) < LVL_IMMORT || (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_NOHASSLE))))
      send_to_char(ch, "It seems to be locked.\r\n");
    else if (!has_key(ch, keynum) && (GET_LEVEL(ch) < LVL_STAFF) &&
             ((subcmd == SCMD_LOCK) || (subcmd == SCMD_UNLOCK)))
      send_to_char(ch, "You don't seem to have the proper key.\r\n");
    else if (ok_pick(ch, keynum, DOOR_IS_PICKPROOF(ch, obj, door), subcmd, door))
    {
      do_doorcmd(ch, obj, door, subcmd);
      ch->char_specials.autodoor_message = true;
    }
  }
  return;
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
          act("You try to enter $p, but a mysterious power forces you back!  (class restriction)",
              FALSE, ch, portal, 0, TO_CHAR);
          act("\tRA booming voice in your head shouts '\tWNot for your class!\tR'\tn",
              FALSE, ch, portal, 0, TO_CHAR);
          act("$n tries to enter $p, but a mysterious power forces $m back!",
              FALSE, ch, portal, 0, TO_ROOM);
          return;
        }

        if (IS_EVIL(ch) && OBJ_FLAGGED(portal, ITEM_ANTI_EVIL))
        {
          act("You try to enter $p, but a mysterious power forces you back!  (alignment restriction)",
              FALSE, ch, portal, 0, TO_CHAR);
          act("\tRA booming voice in your head shouts '\tWBEGONE EVIL-DOER!\tR'\tn",
              FALSE, ch, portal, 0, TO_CHAR);
          act("$n tries to enter $p, but a mysterious power forces $m back!",
              FALSE, ch, portal, 0, TO_ROOM);
          return;
        }

        if (IS_GOOD(ch) && OBJ_FLAGGED(portal, ITEM_ANTI_GOOD))
        {
          act("You try to enter $p, but a mysterious power forces you back!  (alignment restriction)",
              FALSE, ch, portal, 0, TO_CHAR);
          act("\tRA booming voice in your head shouts '\tWBEGONE DO-GOODER!\tR'\tn",
              FALSE, ch, portal, 0, TO_CHAR);
          act("$n tries to enter $p, but a mysterious power forces $m back!",
              FALSE, ch, portal, 0, TO_ROOM);
          return;
        }

        if (((!IS_EVIL(ch)) && !(IS_GOOD(ch))) &&
            OBJ_FLAGGED(portal, ITEM_ANTI_NEUTRAL))
        {
          act("You try to enter $p, but a mysterious power forces you back!  (alignment restriction)",
              FALSE, ch, portal, 0, TO_CHAR);
          act("\tRA booming voice in your head shouts '\tWBEGONE!\tR'\tn",
              FALSE, ch, portal, 0, TO_CHAR);
          act("$n tries to enter $p, but a mysterious power forces $m back!",
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
      act("You enter $p, and are transported elsewhere.", FALSE, ch, portal, 0, TO_CHAR);
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
bool can_stand(struct char_data *ch)
{

  if (AFF_FLAGGED(ch, AFF_PINNED))
    return FALSE;

  if (AFF_FLAGGED(ch, AFF_ENTANGLED))
    return FALSE;

  if (AFF_FLAGGED(ch, AFF_GRAPPLED))
    return FALSE;

  if (AFF_FLAGGED(ch, AFF_SLEEP))
    return FALSE;

  if (AFF_FLAGGED(ch, AFF_PARALYZED))
    return FALSE;

  if (!is_action_available(ch, atMOVE, FALSE))
    return FALSE;

  switch (GET_POS(ch))
  {
  case POS_STANDING:
  case POS_SLEEPING:
  case POS_FIGHTING:
    return FALSE;
  case POS_SITTING:
  case POS_RESTING:
  case POS_RECLINING:
  default:
    return TRUE;
  }

  /* how did we get here? */
  return FALSE;
}

/* Stand - Standing costs a move action. */
ACMD(do_stand)
{
  if (AFF_FLAGGED(ch, AFF_PINNED))
  {
    send_to_char(ch, "You can't, you are pinned! (try struggle or grapple <target>).\r\n");
    return;
  }
  switch (GET_POS(ch))
  {
  case POS_STANDING:
    send_to_char(ch, "You are already standing.\r\n");
    break;
  case POS_SITTING:
    send_to_char(ch, "You stand up.\r\n");
    act("$n clambers to $s feet.", TRUE, ch, 0, 0, TO_ROOM);
    /* Were they sitting in something? */
    char_from_furniture(ch);
    /* Will be sitting after a successful bash and may still be fighting. */
    GET_POS(ch) = FIGHTING(ch) ? POS_FIGHTING : POS_STANDING;
    USE_MOVE_ACTION(ch);

    if (FIGHTING(ch))
      attacks_of_opportunity(ch, 0);

    break;
  case POS_RESTING:
    send_to_char(ch, "You stop resting, and stand up.\r\n");
    act("$n stops resting, and clambers on $s feet.", TRUE, ch, 0, 0, TO_ROOM);
    change_position(ch, POS_STANDING);
    /* Were they sitting in something. */
    char_from_furniture(ch);
    USE_MOVE_ACTION(ch);

    if (FIGHTING(ch))
      attacks_of_opportunity(ch, 0);

    break;
  case POS_RECLINING:
    send_to_char(ch, "You hop from prone position to standing.\r\n");
    act("$n hops from prone to standing on $s feet.", TRUE, ch, 0, 0, TO_ROOM);
    change_position(ch, POS_STANDING);
    /* Were they sitting in something. */
    char_from_furniture(ch);
    USE_MOVE_ACTION(ch);

    if (FIGHTING(ch))
      attacks_of_opportunity(ch, 0);

    break;
  case POS_SLEEPING:
    send_to_char(ch, "You have to wake up first!\r\n");
    break;
  case POS_FIGHTING:
    send_to_char(ch, "Do you not consider fighting as standing?\r\n");
    break;
  default:
    send_to_char(ch, "You stop floating around, and put your feet on the ground.\r\n");
    act("$n stops floating around, and puts $s feet on the ground.",
        TRUE, ch, 0, 0, TO_ROOM);
    change_position(ch, POS_STANDING);
    break;
  }
}

ACMD(do_sit)
{
  char arg[MAX_STRING_LENGTH] = {'\0'};
  struct obj_data *furniture;
  struct char_data *tempch;
  int found;

  one_argument(argument, arg, sizeof(arg));

  if (!*arg)
    found = 0;
  if (!(furniture = get_obj_in_list_vis(ch, arg, NULL, world[ch->in_room].contents)))
    found = 0;
  else
    found = 1;

  switch (GET_POS(ch))
  {
  case POS_STANDING:
    if (found == 0)
    {
      send_to_char(ch, "You sit down.\r\n");
      act("$n sits down.", FALSE, ch, 0, 0, TO_ROOM);
      change_position(ch, POS_SITTING);
    }
    else
    {
      if (GET_OBJ_TYPE(furniture) != ITEM_FURNITURE)
      {
        send_to_char(ch, "You can't sit on that!\r\n");
        return;
      }
      else if (GET_OBJ_VAL(furniture, 1) > GET_OBJ_VAL(furniture, 0))
      {
        /* Val 1 is current number sitting, 0 is max in sitting. */
        act("$p looks like it's all full.", TRUE, ch, furniture, 0, TO_CHAR);
        log("SYSERR: Furniture %d holding too many people.", GET_OBJ_VNUM(furniture));
        return;
      }
      else if (GET_OBJ_VAL(furniture, 1) == GET_OBJ_VAL(furniture, 0))
      {
        act("There is no where left to sit upon $p.", TRUE, ch, furniture, 0, TO_CHAR);
        return;
      }
      else
      {
        if (OBJ_SAT_IN_BY(furniture) == NULL)
          OBJ_SAT_IN_BY(furniture) = ch;
        for (tempch = OBJ_SAT_IN_BY(furniture); tempch != ch; tempch = NEXT_SITTING(tempch))
        {
          if (NEXT_SITTING(tempch))
            continue;
          NEXT_SITTING(tempch) = ch;
        }
        act("You sit down upon $p.", TRUE, ch, furniture, 0, TO_CHAR);
        act("$n sits down upon $p.", TRUE, ch, furniture, 0, TO_ROOM);
        SITTING(ch) = furniture;
        NEXT_SITTING(ch) = NULL;
        GET_OBJ_VAL(furniture, 1) += 1;
        change_position(ch, POS_SITTING);
      }
    }
    break;
  case POS_SITTING:
    send_to_char(ch, "You're sitting already.\r\n");
    break;
  case POS_RESTING:
    send_to_char(ch, "You stop resting, and sit up.\r\n");
    act("$n stops resting.", TRUE, ch, 0, 0, TO_ROOM);
    change_position(ch, POS_SITTING);
    break;
  case POS_RECLINING:
    send_to_char(ch, "You shift your body from prone to sitting up.\r\n");
    act("$n shifts $s body from prone to sitting up.", TRUE, ch, 0, 0, TO_ROOM);
    change_position(ch, POS_SITTING);
    break;
  case POS_SLEEPING:
    send_to_char(ch, "You have to wake up first.\r\n");
    break;
  case POS_FIGHTING:
    send_to_char(ch, "You drop down in a low squat!\r\n");
    change_position(ch, POS_SITTING);
    break;
  default:
    send_to_char(ch, "You stop floating around, and sit down.\r\n");
    act("$n stops floating around, and sits down.", TRUE, ch, 0, 0, TO_ROOM);
    change_position(ch, POS_SITTING);
    break;
  }
}

ACMD(do_rest)
{

  if (affected_by_spell(ch, SKILL_RAGE))
  {
    send_to_char(ch, "Rest now? No way. PRESS ON!\r\n");
    return;
  }

  switch (GET_POS(ch))
  {
  case POS_STANDING:
    send_to_char(ch, "You sit down and rest your tired bones.\r\n");
    act("$n sits down and rests.", TRUE, ch, 0, 0, TO_ROOM);
    change_position(ch, POS_RESTING);
    break;
  case POS_SITTING:
    send_to_char(ch, "You rest your tired bones.\r\n");
    act("$n rests.", TRUE, ch, 0, 0, TO_ROOM);
    change_position(ch, POS_RESTING);
    break;
  case POS_RESTING:
    send_to_char(ch, "You are already resting.\r\n");
    break;
  case POS_RECLINING:
    send_to_char(ch, "You sit up slowly.\r\n");
    act("$n sits up slowly.", TRUE, ch, 0, 0, TO_ROOM);
    change_position(ch, POS_RESTING);
    break;
  case POS_SLEEPING:
    send_to_char(ch, "You have to wake up first.\r\n");
    break;
  case POS_FIGHTING:
    send_to_char(ch, "Rest while fighting?  Are you MAD?\r\n");
    break;
  default:
    send_to_char(ch, "You stop floating around, and stop to rest your tired bones.\r\n");
    act("$n stops floating around, and rests.", FALSE, ch, 0, 0, TO_ROOM);
    change_position(ch, POS_RESTING);
    break;
  }
}

ACMD(do_recline)
{
  switch (GET_POS(ch))
  {
  case POS_STANDING:
    send_to_char(ch, "You drop down to a prone position.\r\n");
    act("$n drops down to a prone position.", TRUE, ch, 0, 0, TO_ROOM);
    change_position(ch, POS_RECLINING);
    break;
  case POS_SITTING:
    send_to_char(ch, "You shift to a prone position.\r\n");
    act("$n shifts to a prone position.", TRUE, ch, 0, 0, TO_ROOM);
    change_position(ch, POS_RECLINING);
    break;
  case POS_RESTING:
    send_to_char(ch, "You lie down and continue resting.\r\n");
    act("$n lays down.", TRUE, ch, 0, 0, TO_ROOM);
    change_position(ch, POS_RECLINING);
    break;
  case POS_RECLINING:
    send_to_char(ch, "You are already reclining.\r\n");
    break;
  case POS_SLEEPING:
    send_to_char(ch, "You have to wake up first.\r\n");
    break;
  case POS_FIGHTING:
    send_to_char(ch, "You drop down to your stomach!\r\n");
    change_position(ch, POS_RECLINING);
    break;
  default:
    send_to_char(ch, "You stop floating around, and drop prone to the ground.\r\n");
    act("$n stops floating around, and drops prone to the ground.", FALSE, ch, 0, 0, TO_ROOM);
    change_position(ch, POS_RECLINING);
    break;
  }
}

ACMD(do_sleep)
{

  if (affected_by_spell(ch, SKILL_RAGE))
  {
    send_to_char(ch, "You are way too hyper for that right now!\r\n");
    return;
  }

  switch (GET_POS(ch))
  {
  case POS_STANDING:
  case POS_SITTING:
  case POS_RESTING:
  case POS_RECLINING:
    send_to_char(ch, "You go to sleep.\r\n");
    act("$n lies down and falls asleep.", TRUE, ch, 0, 0, TO_ROOM);
    change_position(ch, POS_SLEEPING);
    break;
  case POS_SLEEPING:
    send_to_char(ch, "You are already sound asleep.\r\n");
    break;
  case POS_FIGHTING:
    send_to_char(ch, "Sleep while fighting?  Are you MAD?\r\n");
    break;
  default:
    send_to_char(ch, "You stop floating around, and lie down to sleep.\r\n");
    act("$n stops floating around, and lie down to sleep.",
        TRUE, ch, 0, 0, TO_ROOM);
    change_position(ch, POS_SLEEPING);
    break;
  }
}

ACMD(do_wake)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict;
  int self = 0;

  one_argument(argument, arg, sizeof(arg));
  if (*arg)
  {
    if (GET_POS(ch) == POS_SLEEPING)
      send_to_char(ch, "Maybe you should wake yourself up first.\r\n");
    else if ((vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)) == NULL)
      send_to_char(ch, "%s", CONFIG_NOPERSON);
    else if (vict == ch)
      self = 1;
    else if (AWAKE(vict))
      act("$E is already awake.", FALSE, ch, 0, vict, TO_CHAR);
    else if (AFF_FLAGGED(vict, AFF_SLEEP))
      act("You can't wake $M up!", FALSE, ch, 0, vict, TO_CHAR);
    else if (GET_POS(vict) < POS_SLEEPING)
      act("$E's in pretty bad shape!", FALSE, ch, 0, vict, TO_CHAR);
    else
    {
      act("You wake $M up.", FALSE, ch, 0, vict, TO_CHAR);
      act("You are awakened by $n.", FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);
      change_position(vict, POS_RECLINING);
    }
    if (!self)
      return;
  }
  if (AFF_FLAGGED(ch, AFF_SLEEP))
    send_to_char(ch, "You can't wake up!\r\n");
  else if (GET_POS(ch) > POS_SLEEPING)
    send_to_char(ch, "You are already awake...\r\n");
  else
  {
    send_to_char(ch, "You awaken and are now in a prone position.\r\n");
    act("$n awakens and is now in a prone position.", TRUE, ch, 0, 0, TO_ROOM);
    change_position(ch, POS_RECLINING);
  }
}

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
  if (PRF_FLAGGED(leader, PRF_NO_FOLLOW))
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

/* I put this here for reference for below - Zusuk */
/*
#define POS_DEAD       0	 //Position = dead
#define POS_MORTALLYW  1	 //Position = mortally wounded
#define POS_INCAP      2	 //Position = incapacitated
#define POS_STUNNED    3	 //Position = stunned
#define POS_SLEEPING   4	 //Position = sleeping
#define POS_RECLINING  5	 //Position = reclining
#define POS_RESTING    6	 //Position = resting
#define POS_SITTING    7	 //Position = sitting
#define POS_FIGHTING   8	 //Position = fighting
#define POS_STANDING   9	 //Position = standing
 */

/* in: character, position change
   out: as of this writing, nothing yet
   function:  changing a position use to be just GET_POS(ch) = POS_X, but
              that did not account for dynamic changes that would be connected
              to the change in position..  the classic example is the combat
              maneuver TRIP, which would change your position from POS_STANDING to
              POS_SITTING, if the victim is casting, then they should be -immediately-
              interrupted.  */
int change_position(struct char_data *ch, int new_position)
{
  if (!ch)
    return 0;

  int old_position = GET_POS(ch);

  /* we will put some general checks for having your position changed */

  /* casting */
  if (char_has_mud_event(ch, eCASTING) && new_position <= POS_SITTING)
  {
    act("$n's spell is interrupted!", FALSE, ch, 0, 0,
        TO_ROOM);
    send_to_char(ch, "Your spell is aborted!\r\n");
    resetCastingData(ch);
  }

  /* preparing spells */
  if (char_has_mud_event(ch, ePREPARATION) && new_position != POS_RESTING)
  {
    act("$n's preparations are aborted!", FALSE, ch, 0, 0,
        TO_ROOM);
    send_to_char(ch, "Your preparations are aborted!\r\n");
    stop_all_preparations(ch);
  }

  /* end general checks */

  /* we set up some switches to account for -every- scenario of switches possible
     this can create unique messages, etc */
  switch (new_position)
  {

  case POS_DEAD:
    switch (old_position)
    {
    case POS_DEAD:
      break;
    case POS_MORTALLYW:
      break;
    case POS_INCAP:
      break;
    case POS_STUNNED:
      break;
    case POS_SLEEPING:
      break;
    case POS_RECLINING:
      break;
    case POS_RESTING:
      break;
    case POS_SITTING:
      break;
    case POS_FIGHTING:
      break;
    case POS_STANDING:
      break;
    default:
      break;
    }
    break;

  case POS_MORTALLYW:
    switch (old_position)
    {
    case POS_DEAD:
      break;
    case POS_MORTALLYW:
      break;
    case POS_INCAP:
      break;
    case POS_STUNNED:
      break;
    case POS_SLEEPING:
      break;
    case POS_RECLINING:
      break;
    case POS_RESTING:
      break;
    case POS_SITTING:
      break;
    case POS_FIGHTING:
      break;
    case POS_STANDING:
      break;
    default:
      break;
    }
    break;

  case POS_INCAP:
    switch (old_position)
    {
    case POS_DEAD:
      break;
    case POS_MORTALLYW:
      break;
    case POS_INCAP:
      break;
    case POS_STUNNED:
      break;
    case POS_SLEEPING:
      break;
    case POS_RECLINING:
      break;
    case POS_RESTING:
      break;
    case POS_SITTING:
      break;
    case POS_FIGHTING:
      break;
    case POS_STANDING:
      break;
    default:
      break;
    }
    break;

  case POS_STUNNED:
    switch (old_position)
    {
    case POS_DEAD:
      break;
    case POS_MORTALLYW:
      break;
    case POS_INCAP:
      break;
    case POS_STUNNED:
      break;
    case POS_SLEEPING:
      break;
    case POS_RECLINING:
      break;
    case POS_RESTING:
      break;
    case POS_SITTING:
      break;
    case POS_FIGHTING:
      break;
    case POS_STANDING:
      break;
    default:
      break;
    }
    break;

  case POS_SLEEPING:
    switch (old_position)
    {
    case POS_DEAD:
      break;
    case POS_MORTALLYW:
      break;
    case POS_INCAP:
      break;
    case POS_STUNNED:
      break;
    case POS_SLEEPING:
      break;
    case POS_RECLINING:
      break;
    case POS_RESTING:
      break;
    case POS_SITTING:
      break;
    case POS_FIGHTING:
      break;
    case POS_STANDING:
      break;
    default:
      break;
    }
    break;

  case POS_RECLINING:
    switch (old_position)
    {
    case POS_DEAD:
      break;
    case POS_MORTALLYW:
      break;
    case POS_INCAP:
      break;
    case POS_STUNNED:
      break;
    case POS_SLEEPING:
      break;
    case POS_RECLINING:
      break;
    case POS_RESTING:
      break;
    case POS_SITTING:
      break;
    case POS_FIGHTING:
      break;
    case POS_STANDING:
      break;
    default:
      break;
    }
    break;

  case POS_RESTING:
    switch (old_position)
    {
    case POS_DEAD:
      break;
    case POS_MORTALLYW:
      break;
    case POS_INCAP:
      break;
    case POS_STUNNED:
      break;
    case POS_SLEEPING:
      break;
    case POS_RECLINING:
      break;
    case POS_RESTING:
      break;
    case POS_SITTING:
      break;
    case POS_FIGHTING:
      break;
    case POS_STANDING:
      break;
    default:
      break;
    }
    break;

  case POS_SITTING:
    switch (old_position)
    {
    case POS_DEAD:
      break;
    case POS_MORTALLYW:
      break;
    case POS_INCAP:
      break;
    case POS_STUNNED:
      break;
    case POS_SLEEPING:
      break;
    case POS_RECLINING:
      break;
    case POS_RESTING:
      break;
    case POS_SITTING:
      break;
    case POS_FIGHTING:
      break;
    case POS_STANDING:
      break;
    default:
      break;
    }
    break;

  case POS_FIGHTING:
    switch (old_position)
    {
    case POS_DEAD:
      break;
    case POS_MORTALLYW:
      break;
    case POS_INCAP:
      break;
    case POS_STUNNED:
      break;
    case POS_SLEEPING:
      break;
    case POS_RECLINING:
      break;
    case POS_RESTING:
      break;
    case POS_SITTING:
      break;
    case POS_FIGHTING:
      break;
    case POS_STANDING:
      break;
    default:
      break;
    }
    break;

  case POS_STANDING:
    switch (old_position)
    {
    case POS_DEAD:
      break;
    case POS_MORTALLYW:
      break;
    case POS_INCAP:
      break;
    case POS_STUNNED:
      break;
    case POS_SLEEPING:
      break;
    case POS_RECLINING:
      break;
    case POS_RESTING:
      break;
    case POS_SITTING:
      break;
    case POS_FIGHTING:
      break;
    case POS_STANDING:
      break;
    default:
      break;
    }
    break;
  default:
    break;
  }

  /* this is really all that is going on here :P */
  GET_POS(ch) = new_position;

  /* we don't have a significant return value yet */
  if (old_position == new_position)
    return 0;
  else
    return 1;
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

int get_speed(struct char_data *ch, sbyte to_display)
{

  // if mounted, we'll use the mount's speed instead.
  if (RIDING(ch))
    return get_speed(RIDING(ch), to_display);

  int speed = 30;

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

  if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_MOUNTABLE))
    speed = 50;

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

/* undefines */
#undef PRISONER_KEY_1
#undef PRISONER_KEY_2
#undef PRISONER_KEY_3

/*EOF*/
