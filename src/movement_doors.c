/**************************************************************************
 *  File: movement_doors.c                            Part of LuminariMUD *
 *  Usage: Door handling functions                                        *
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
#include "dg_scripts.h"
#include "mud_event.h"
#include "actions.h"
#include "config.h"
#include "traps.h"
#include "spec_procs.h"
#include "psionics.h"
#include "movement_doors.h"

/* External functions */
extern int skill_check(struct char_data *ch, int ability, int dc);
extern const char *get_walkto_landmark_name(int number);
extern int walkto_vnum_to_list_row(room_vnum vnum);

#define NEED_OPEN (1 << 0)
#define NEED_CLOSED (1 << 1)
#define NEED_UNLOCKED (1 << 2)
#define NEED_LOCKED (1 << 3)

#define PRISONER_KEY_1 132130
#define PRISONER_KEY_2 132129
#define PRISONER_KEY_3 132150

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

/* Static (internal) function prototypes - will be made non-static after transition */
static int find_door(struct char_data *ch, const char *type, char *dir, const char *cmdname);
static void do_doorcmd(struct char_data *ch, struct obj_data *obj, int door, int scmd);

/* Find door function - locates a door by keyword and/or direction */
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
    {
      send_to_char(ch, "There doesn't seem to be %s %s here.\r\n", AN(type), type);
      send_to_char(ch, "Try turning on the autokey toggle in prefedit, or supply a direction name as well. Eg. %s %s north.", cmdname, type);
    }
    else if (is_abbrev(cmdname, "open"))
    {
      send_to_char(ch, "There doesn't seem to be %s %s that can be opened.\r\n", AN(type), type);
      send_to_char(ch, "Try turning on the autokey toggle in prefedit, or supply a direction name as well. Eg. %s %s north.", cmdname, type);
    }
    else if (is_abbrev(cmdname, "close"))
    {
      send_to_char(ch, "There doesn't seem to be %s %s that can be closed.\r\n", AN(type), type);
      send_to_char(ch, "Try turning on the autokey toggle in prefedit, or supply a direction name as well. Eg. %s %s north.", cmdname, type);
    }
    else if (is_abbrev(cmdname, "lock"))
    {
      send_to_char(ch, "There doesn't seem to be %s %s that can be locked.\r\n", AN(type), type);
      send_to_char(ch, "Try turning on the autokey toggle in prefedit, or supply a direction name as well. Eg. %s %s north.", cmdname, type);
    }
    else if (is_abbrev(cmdname, "unlock"))
    {
      send_to_char(ch, "There doesn't seem to be %s %s that can be unlocked.\r\n", AN(type), type);
      send_to_char(ch, "Try turning on the autokey toggle in prefedit, or supply a direction name as well. Eg. %s %s north.", cmdname, type);
    }
    else
    {
      send_to_char(ch, "There doesn't seem to be %s %s that can be picked.\r\n", AN(type), type);
      send_to_char(ch, "Try turning on the autokey toggle in prefedit, or supply a direction name as well. Eg. %s %s north.", cmdname, type);
    }

    return (-1);
  }
}

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
      act("$n breaks $p unlocking the path!", FALSE, ch, o, 0, TO_ROOM);
      act("You break $p unlocking the path!", FALSE, ch, o, 0, TO_CHAR);
      extract_obj(o);
      return TRUE;
    }
  }

  if (GET_EQ(ch, WEAR_HOLD_1))
  {
    if (GET_OBJ_VNUM(GET_EQ(ch, WEAR_HOLD_1)) == key)
    {
      act("$n breaks $p unlocking the path!", FALSE, ch, GET_EQ(ch, WEAR_HOLD_1), 0, TO_ROOM);
      act("You break $p unlocking the path!", FALSE, ch, GET_EQ(ch, WEAR_HOLD_1), 0, TO_CHAR);
      extract_obj(unequip_char(ch, WEAR_HOLD_1));
      return TRUE;
    }
  }

  if (GET_EQ(ch, WEAR_HOLD_2))
  {
    if (GET_OBJ_VNUM(GET_EQ(ch, WEAR_HOLD_2)) == key)
    {
      act("$n breaks $p unlocking the path!", FALSE, ch, GET_EQ(ch, WEAR_HOLD_2), 0, TO_ROOM);
      act("You break $p unlocking the path!", FALSE, ch, GET_EQ(ch, WEAR_HOLD_2), 0, TO_CHAR);
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

// This will attempt to remove the key from the character
// if the key is found and the key is flagged EXTRACT_ON_USE
void extract_key(struct char_data *ch, obj_vnum key)
{

  /* players were using corpses to open doors */
  if (key == NOTHING || key <= 0)
    return;

  struct obj_data *o = NULL;

  for (o = ch->carrying; o; o = o->next_content)
  {
    if (GET_OBJ_VNUM(o) == key && OBJ_FLAGGED(o, ITEM_EXTRACT_AFTER_USE))
    {
      act("After using $p, it crumbles to dust.", false, ch, o, 0, TO_CHAR);
      act("After $n uses $p, it crumbles to dust.", false, ch, o, 0, TO_ROOM);
      obj_from_char(o);
      extract_obj(o);
      return;
    }
  }

  if (GET_EQ(ch, WEAR_HOLD_1))
    if (GET_OBJ_VNUM(GET_EQ(ch, WEAR_HOLD_1)) == key && OBJ_FLAGGED(GET_EQ(ch, WEAR_HOLD_1), ITEM_EXTRACT_AFTER_USE))
    {
      act("After using $p, it crumbles to dust.", false, ch, GET_EQ(ch, WEAR_HOLD_1), 0, TO_CHAR);
      act("After $n uses $p, it crumbles to dust.", false, ch, GET_EQ(ch, WEAR_HOLD_1), 0, TO_ROOM);
      extract_obj(unequip_char(ch, WEAR_HOLD_1));
      return;
    }

  if (GET_EQ(ch, WEAR_HOLD_2))
    if (GET_OBJ_VNUM(GET_EQ(ch, WEAR_HOLD_2)) == key && OBJ_FLAGGED(GET_EQ(ch, WEAR_HOLD_2), ITEM_EXTRACT_AFTER_USE))
    {
      act("After using $p, it crumbles to dust.", false, ch, GET_EQ(ch, WEAR_HOLD_2), 0, TO_CHAR);
      act("After $n uses $p, it crumbles to dust.", false, ch, GET_EQ(ch, WEAR_HOLD_2), 0, TO_ROOM);
      extract_obj(unequip_char(ch, WEAR_HOLD_2));
      return;
    }

}

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
      if (GET_ABILITY(ch, ABILITY_DISABLE_DEVICE) <= 0)
      {
        send_to_char(ch, "You do not know how to pick locks.\r\n");
        return;
      }
      if (skill_check(ch, ABILITY_DISABLE_DEVICE, 15))
      {
        TOGGLE_LOCK(IN_ROOM(ch), obj, door);
        send_to_char(ch, "The lock quickly yields to your skills.\r\n");
        len = strlcpy(buf, "$n skillfully picks the lock on ", sizeof(buf));
      }
      else
      {
        send_to_char(ch, "You fail to pick the lock.\r\n");
        len = strlcpy(buf, "$n fails picks the lock on ", sizeof(buf));
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
  int skill_lvl, roll;
  int lock_dc = 10;
  // struct obj_data *tools = NULL;

  if (scmd != SCMD_PICK)
    return (1);

  if (GET_ABILITY(ch, ABILITY_DISABLE_DEVICE) <= 0)
  {
    send_to_char(ch, "You do not know how to pick locks.\r\n");
    return 0;
  }

  skill_lvl = compute_ability(ch, ABILITY_DISABLE_DEVICE);
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
    send_to_char(ch, "You have no idea how (train disable device)!\r\n");
    return (0);
  }

  roll = d20(ch);

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
  else if (lock_dc <= (skill_lvl + roll))
  {
    send_to_char(ch, "Success! [%d dc vs. %d skill + %d roll]\r\n", lock_dc, skill_lvl, roll);
    USE_MOVE_ACTION(ch);
    return (1);
  }

  /* failed */
  send_to_char(ch, "Failure! [%d dc vs. %d skill + %d roll]\r\n", lock_dc, skill_lvl, roll);
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
                GET_OBJ_TYPE(obj) != ITEM_AMMO_POUCH &&
                GET_OBJ_TYPE(obj) != ITEM_TREASURE_CHEST))
  {
    obj = NULL;
    door = find_door(ch, type, dir, cmd_door[subcmd]);
  }

  if ((obj) || (door >= 0))
  {
    keynum = DOOR_KEY(ch, obj, door);
    if (obj && GET_OBJ_TYPE(obj) == ITEM_TREASURE_CHEST)
    {
      if (GET_OBJ_VAL(obj, 4) <= 0)
      {
        act("$p is not locked.", TRUE, ch, obj, 0, TO_CHAR);
        return;
      }
      if (skill_check(ch, ABILITY_SLEIGHT_OF_HAND, GET_OBJ_VAL(obj, 4)))
      {
        GET_OBJ_VAL(obj, 4) = 0;
        act("You have picked the lock on $p!", FALSE, ch, obj, 0, TO_CHAR);
        WAIT_STATE(ch, PULSE_VIOLENCE * 1);
      }
      else
      {
        act("You have failed to pick the lock on $p.", FALSE, ch, obj, 0, TO_CHAR);
        WAIT_STATE(ch, PULSE_VIOLENCE * 3);
      }
      send_to_char(ch, "Your next action will be delayed.\r\n");
      USE_FULL_ROUND_ACTION(ch);
      return;
    }
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
      extract_key(ch, keynum);

    }
    else if (!(DOOR_IS_UNLOCKED(ch, obj, door)) && IS_SET(flags_door[subcmd], NEED_UNLOCKED) &&
             ((!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOKEY))) && (!has_key(ch, keynum)))
    {
      send_to_char(ch, "It is locked, and you do not have the key!\r\n");
      if (GET_WALKTO_LOC(ch))
      {
        send_to_char(ch, "You stop walking to the '%s' landmark.\r\n", get_walkto_landmark_name(walkto_vnum_to_list_row(GET_WALKTO_LOC(ch))));
        GET_WALKTO_LOC(ch) = 0;
      }
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
      extract_key(ch, keynum);
      return;
    }
  }
  return;
}