/**
 * @file spec/spec_rol_lavatubes.c
 * Converted Realms of Luminari Lavatubes special procedures.
 */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"

#include "act.h"
#include "comm.h"
#include "constants.h"
#include "db.h"
#include "handler.h"
#include "helpers.h"
#include "interpreter.h"
#include "movement/movement.h"
#include "spec_dispatch.h"
#include "spec_rol_conversion.h"
#include "spec_rol_lavatubes.h"

#define ROL_LAVATUBES_SNOWVULTURE_VNUM 2012001
#define ROL_LAVATUBES_CRYSTAL_SPIKE_VNUM 2012000
#define ROL_LAVATUBES_SKELETON_KEY_VNUM 2012025
#define ROL_LAVATUBES_AUTOMATON_VNUM 2012027
#define ROL_LAVATUBES_LEVER_VNUM 2012027
#define ROL_LAVATUBES_UPPER_ROOM_VNUM 2012158
#define ROL_LAVATUBES_LOWER_ROOM_VNUM 2012159

static bool rol_lavatubes_command_is(int cmd, const char *name)
{
  return cmd > 0 && name != NULL && complete_cmd_info != NULL &&
         !strcmp(complete_cmd_info[cmd].command, name);
}

static int rol_lavatubes_held_slot(const struct char_data *ch, const struct obj_data *obj)
{
  static const int held_slots[] = {WEAR_HOLD_1, WEAR_HOLD_2, WEAR_HOLD_2H};
  size_t index;

  if (ch == NULL || obj == NULL || obj->worn_by != ch)
    return -1;

  for (index = 0; index < sizeof(held_slots) / sizeof(held_slots[0]); index++)
    if (GET_EQ(ch, held_slots[index]) == obj)
      return held_slots[index];

  return -1;
}

static bool rol_lavatubes_actor_has_key(const struct char_data *ch, obj_vnum key_vnum)
{
  const struct obj_data *obj;

  if (ch == NULL || key_vnum == NOTHING)
    return false;
  if (!IS_NPC(ch) && GET_LEVEL(ch) >= LVL_IMMORT && PRF_FLAGGED(ch, PRF_NOHASSLE))
    return true;

  for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
    if (GET_OBJ_VNUM(obj) == key_vnum)
      return true;

  if (GET_EQ(ch, WEAR_HOLD_1) != NULL && GET_OBJ_VNUM(GET_EQ(ch, WEAR_HOLD_1)) == key_vnum)
    return true;
  if (GET_EQ(ch, WEAR_HOLD_2) != NULL && GET_OBJ_VNUM(GET_EQ(ch, WEAR_HOLD_2)) == key_vnum)
    return true;
  return false;
}

static bool rol_lavatubes_room_pair(room_rnum *upper_room, room_rnum *lower_room)
{
  room_rnum upper;
  room_rnum lower;

  upper = real_room(ROL_LAVATUBES_UPPER_ROOM_VNUM);
  lower = real_room(ROL_LAVATUBES_LOWER_ROOM_VNUM);
  if (!VALID_ROOM_RNUM(upper) || !VALID_ROOM_RNUM(lower) || world[upper].dir_option[DOWN] == NULL ||
      world[lower].dir_option[UP] == NULL || world[upper].dir_option[DOWN]->to_room != lower ||
      world[lower].dir_option[UP]->to_room != upper)
  {
    log("SYSERR: RoL Lavatubes trapdoor rooms %d/%d do not form a valid exit pair",
        ROL_LAVATUBES_UPPER_ROOM_VNUM, ROL_LAVATUBES_LOWER_ROOM_VNUM);
    return false;
  }

  if (upper_room != NULL)
    *upper_room = upper;
  if (lower_room != NULL)
    *lower_room = lower;
  return true;
}

int rol_lavatubes_skeleton_key_break_chance(int dexterity_bonus)
{
  return MAX(0, MIN(100, 50 - dexterity_bonus * 5));
}

enum rol_lavatubes_snowvulture_outcome rol_lavatubes_snowvulture_outcome(int roll)
{
  switch (roll)
  {
  case 3:
    return ROL_LAVATUBES_SNOWVULTURE_SQUEAK;
  case 4:
    return ROL_LAVATUBES_SNOWVULTURE_FLAP;
  case 5:
    return ROL_LAVATUBES_SNOWVULTURE_DEVOUR;
  default:
    return ROL_LAVATUBES_SNOWVULTURE_NONE;
  }
}

static int rol_lavatubes_snowvulture(struct char_data *mob, const char *argument)
{
  enum rol_lavatubes_snowvulture_outcome outcome;

  if (mob == NULL || FIGHTING(mob) != NULL || !VALID_ROOM_RNUM(IN_ROOM(mob)))
    return FALSE;

  outcome = rol_lavatubes_snowvulture_outcome(dice(3, 3));
  switch (outcome)
  {
  case ROL_LAVATUBES_SNOWVULTURE_SQUEAK:
    act("$n squeaks, \"Skaaa? reet.\"", FALSE, mob, NULL, NULL, TO_ROOM);
    break;
  case ROL_LAVATUBES_SNOWVULTURE_FLAP:
    act("$n flaps about.", FALSE, mob, NULL, NULL, TO_ROOM);
    break;
  case ROL_LAVATUBES_SNOWVULTURE_DEVOUR:
    (void)rol_corpse_devourer(mob, mob, 0, argument);
    break;
  default:
    break;
  }

  /* Source activity never consumed the pulse, including after devouring. */
  return FALSE;
}

static int rol_lavatubes_automaton_unblock(struct char_data *mob)
{
  struct char_data *occupant;
  room_rnum upper_room;
  room_rnum lower_room;

  if (mob == NULL || !VALID_ROOM_RNUM(IN_ROOM(mob)) ||
      GET_ROOM_VNUM(IN_ROOM(mob)) != ROL_LAVATUBES_LOWER_ROOM_VNUM)
    return FALSE;

  for (occupant = world[IN_ROOM(mob)].people; occupant != NULL; occupant = occupant->next_in_room)
    if (occupant != mob)
      return FALSE;

  if (!rol_lavatubes_room_pair(&upper_room, &lower_room))
    return FALSE;

  REMOVE_BIT(world[upper_room].dir_option[DOWN]->exit_info, EX_BLOCKED);
  REMOVE_BIT(world[lower_room].dir_option[UP]->exit_info, EX_BLOCKED);
  return TRUE;
}

static void rol_lavatubes_extract_held_object(struct spec_event_context *context,
                                              struct char_data *ch, struct obj_data *obj, int slot)
{
  struct obj_data *removed;

  removed = unequip_char(ch, slot);
  if (removed != obj)
  {
    log("SYSERR: RoL Lavatubes object changed held slot during command dispatch");
    if (removed != NULL)
      equip_char(ch, removed, slot);
    return;
  }

  extract_obj(removed);
  context->invalidation |= SPEC_INVALIDATE_OWNER;
}

static int rol_lavatubes_crystal_spike(struct spec_event_context *context, struct char_data *ch,
                                       struct obj_data *obj)
{
  int slot;

  if (!rol_lavatubes_command_is(context->command, "cast") ||
      (slot = rol_lavatubes_held_slot(ch, obj)) < 0)
    return FALSE;

  GET_OBJ_VAL(obj, 0)--;
  if (GET_OBJ_VAL(obj, 0) != 0)
    return FALSE;

  act("Your $p fades slowly, and disappears into nothing.", FALSE, ch, obj, NULL, TO_CHAR);
  act("$n's $p fades slowly, and disappears into nothing.", FALSE, ch, obj, NULL, TO_ROOM);
  rol_lavatubes_extract_held_object(context, ch, obj, slot);
  return FALSE;
}

static int rol_lavatubes_find_door(struct char_data *ch, const char *type, const char *direction)
{
  char direction_copy[MAX_INPUT_LENGTH];
  int door;

  if (ch == NULL || type == NULL || direction == NULL || !VALID_ROOM_RNUM(IN_ROOM(ch)))
    return -1;

  if (*direction != '\0')
  {
    strlcpy(direction_copy, direction, sizeof(direction_copy));
    door = search_block(direction_copy, dirs, FALSE);
    if (door < 0 || door >= DIR_COUNT || EXIT(ch, door) == NULL)
      return -1;
    if (EXIT(ch, door)->keyword != NULL && !is_name(type, EXIT(ch, door)->keyword))
      return -1;
    return door;
  }

  for (door = 0; door < DIR_COUNT; door++)
  {
    if (EXIT(ch, door) == NULL || EXIT(ch, door)->keyword == NULL)
      continue;
    if (!isname(type, EXIT(ch, door)->keyword) && !is_abbrev(type, dirs[door]))
      continue;
    if (EXIT_FLAGGED(EXIT(ch, door), EX_HIDDEN) && GET_LEVEL(ch) < LVL_IMMORT)
      continue;
    return door;
  }

  return -1;
}

static void rol_lavatubes_break_skeleton_key(struct spec_event_context *context,
                                             struct char_data *ch, struct obj_data *key, int slot)
{
  act("Your clumsiness caused $p to break into several pieces!", TRUE, ch, key, NULL, TO_CHAR);
  act("$n's clumsiness caused $p to break into several pieces!", TRUE, ch, key, NULL, TO_ROOM);
  rol_lavatubes_extract_held_object(context, ch, key, slot);
}

static bool rol_lavatubes_skeleton_key_breaks(struct char_data *ch, bool pickproof)
{
  if (pickproof)
    return true;
  return rand_number(1, 100) <= rol_lavatubes_skeleton_key_break_chance(GET_DEX_BONUS(ch));
}

static int rol_lavatubes_unlock_object(struct spec_event_context *context, struct char_data *ch,
                                       struct obj_data *key, struct obj_data *target, int slot)
{
  if ((GET_OBJ_TYPE(target) != ITEM_CONTAINER && GET_OBJ_TYPE(target) != ITEM_AMMO_POUCH) ||
      !OBJVAL_FLAGGED(target, CONT_CLOSED) || GET_OBJ_VAL(target, 2) < 0 ||
      rol_lavatubes_actor_has_key(ch, (obj_vnum)GET_OBJ_VAL(target, 2)) ||
      !OBJVAL_FLAGGED(target, CONT_LOCKED))
    return FALSE;

  if (rol_lavatubes_skeleton_key_breaks(ch, OBJVAL_FLAGGED(target, CONT_PICKPROOF)))
  {
    rol_lavatubes_break_skeleton_key(context, ch, key, slot);
    return TRUE;
  }

  act("You successfully used $p to unlock $P!", TRUE, ch, key, target, TO_CHAR);
  act("$n successfully used $p to unlock $P!", TRUE, ch, key, target, TO_ROOM);
  REMOVE_BIT(GET_OBJ_VAL(target, 1), CONT_LOCKED);
  return TRUE;
}

static int rol_lavatubes_unlock_door(struct spec_event_context *context, struct char_data *ch,
                                     struct obj_data *key, int door, int slot)
{
  struct room_direction_data *back;
  room_rnum other_room;

  if (door < 0 || EXIT(ch, door) == NULL || !EXIT_FLAGGED(EXIT(ch, door), EX_ISDOOR) ||
      !EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED) || EXIT(ch, door)->key == NOTHING ||
      rol_lavatubes_actor_has_key(ch, EXIT(ch, door)->key) ||
      (!EXIT_FLAGGED(EXIT(ch, door), EX_LOCKED) && !EXIT_FLAGGED(EXIT(ch, door), EX_LOCKED_EASY) &&
       !EXIT_FLAGGED(EXIT(ch, door), EX_LOCKED_MEDIUM) &&
       !EXIT_FLAGGED(EXIT(ch, door), EX_LOCKED_HARD)))
    return FALSE;

  if (rol_lavatubes_skeleton_key_breaks(ch, EXIT_FLAGGED(EXIT(ch, door), EX_PICKPROOF)))
  {
    rol_lavatubes_break_skeleton_key(context, ch, key, slot);
    return TRUE;
  }

  if (EXIT(ch, door)->keyword != NULL)
  {
    act("You successfully used $p to unlock the $F!", TRUE, ch, key, EXIT(ch, door)->keyword,
        TO_CHAR);
    act("$n successfully used $p to unlock the $F!", TRUE, ch, key, EXIT(ch, door)->keyword,
        TO_ROOM);
  }
  else
  {
    act("You successfully used $p to unlock the door!", TRUE, ch, key, NULL, TO_CHAR);
    act("$n successfully used $p to unlock the door!", TRUE, ch, key, NULL, TO_ROOM);
  }

  other_room = EXIT(ch, door)->to_room;
  remove_locked_door_flags(IN_ROOM(ch), door);
  if (VALID_ROOM_RNUM(other_room) && (back = world[other_room].dir_option[rev_dir[door]]) != NULL &&
      back->to_room == IN_ROOM(ch))
    remove_locked_door_flags(other_room, rev_dir[door]);
  return TRUE;
}

static int rol_lavatubes_skeleton_key(struct spec_event_context *context, struct char_data *ch,
                                      struct obj_data *key)
{
  struct char_data *dummy_character;
  struct obj_data *target;
  char type[MAX_INPUT_LENGTH];
  char direction[MAX_INPUT_LENGTH];
  int door;
  int slot;

  if (!rol_lavatubes_command_is(context->command, "unlock") || ch == NULL || !AWAKE(ch) ||
      context->argument == NULL || (slot = rol_lavatubes_held_slot(ch, key)) < 0)
    return FALSE;

  two_arguments(context->argument, type, sizeof(type), direction, sizeof(direction));
  if (*type == '\0')
    return FALSE;

  if (generic_find(type, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &dummy_character, &target))
    return rol_lavatubes_unlock_object(context, ch, key, target, slot);

  door = rol_lavatubes_find_door(ch, type, direction);
  return rol_lavatubes_unlock_door(context, ch, key, door, slot);
}

static int rol_lavatubes_automaton_lever(struct char_data *ch, struct obj_data *lever, int cmd,
                                         const char *argument)
{
  struct char_data *dummy_character;
  struct obj_data *selected;
  room_rnum upper_room;
  room_rnum lower_room;

  if (ch == NULL || lever == NULL || argument == NULL || !rol_lavatubes_command_is(cmd, "pull") ||
      !VALID_ROOM_RNUM(IN_ROOM(ch)) ||
      GET_ROOM_VNUM(IN_ROOM(ch)) != ROL_LAVATUBES_LOWER_ROOM_VNUM ||
      !generic_find(argument, FIND_OBJ_ROOM, ch, &dummy_character, &selected) || selected != lever)
    return FALSE;

  if (!rol_lavatubes_room_pair(&upper_room, &lower_room))
  {
    send_to_char(ch, "The mechanism is broken. Please tell a staff member.\r\n");
    return TRUE;
  }

  if (!EXIT_FLAGGED(world[lower_room].dir_option[UP], EX_BLOCKED))
  {
    send_to_char(ch, "Nothing seems to happen.\r\n");
    return TRUE;
  }

  act("You pull $p.", FALSE, ch, lever, NULL, TO_CHAR);
  act("$n pulls $p.", TRUE, ch, lever, NULL, TO_ROOM);
  send_to_room(IN_ROOM(ch), "A loud metallic scraping sounds, followed by a clunk.\r\n");
  send_to_room(IN_ROOM(ch), "The trapdoor appears to hang ever so slightly lower.\r\n");
  REMOVE_BIT(world[upper_room].dir_option[DOWN]->exit_info, EX_BLOCKED);
  REMOVE_BIT(world[lower_room].dir_option[UP]->exit_info, EX_BLOCKED);
  return TRUE;
}

static int rol_lavatubes_automaton_trapdoor(struct char_data *ch, struct room_data *room, int cmd)
{
  struct obj_data *obj;
  room_rnum upper_room;
  room_rnum lower_room;

  if (ch == NULL || room == NULL || !AWAKE(ch) || !VALID_ROOM_RNUM(IN_ROOM(ch)) ||
      room->number != ROL_LAVATUBES_UPPER_ROOM_VNUM || IN_ROOM(ch) != real_room(room->number) ||
      cmd <= 0 || complete_cmd_info == NULL || !IS_MOVE(cmd) ||
      complete_cmd_info[cmd].subcmd != DOWN)
    return FALSE;

  if (!rol_lavatubes_room_pair(&upper_room, &lower_room))
  {
    send_to_char(ch, "The trapdoor mechanism is broken. Please tell a staff member.\r\n");
    return TRUE;
  }
  if (EXIT_FLAGGED(world[upper_room].dir_option[DOWN], EX_CLOSED) ||
      EXIT_FLAGGED(world[upper_room].dir_option[DOWN], EX_BLOCKED))
    return FALSE;

  SET_BIT(world[upper_room].dir_option[DOWN]->exit_info, EX_CLOSED);
  SET_BIT(world[lower_room].dir_option[UP]->exit_info, EX_CLOSED);
  act("The trapdoor swings shut behind you as you descend!", FALSE, ch, NULL, NULL, TO_CHAR);
  act("The trapdoor swings shut behind $n as $e descends.", TRUE, ch, NULL, NULL, TO_ROOM);

  char_from_room(ch);
  char_to_room(ch, lower_room);
  act("The trapdoor swings shut behind $n as $e enters from above.", TRUE, ch, NULL, NULL, TO_ROOM);

  for (obj = world[lower_room].contents; obj != NULL; obj = obj->next_content)
  {
    if (GET_OBJ_TYPE(obj) != ITEM_SWITCH || GET_OBJ_VAL(obj, 1) != ROL_LAVATUBES_UPPER_ROOM_VNUM)
      continue;

    SET_BIT(world[upper_room].dir_option[DOWN]->exit_info, EX_BLOCKED);
    SET_BIT(world[lower_room].dir_option[UP]->exit_info, EX_BLOCKED);
    act("You hear a metallic clunk from the trapdoor as it closes.", FALSE, ch, NULL, NULL,
        TO_CHAR);
    act("You hear a faint metallic clunk from the ceiling.", TRUE, ch, NULL, NULL, TO_ROOM);
    send_to_room(upper_room, "You hear a faint metallic clunk from underground.\r\n");
    break;
  }

  return TRUE;
}

int rol_lavatubes_mobile(struct char_data *ch, void *me, int cmd, const char *argument)
{
  UNUSED(ch);
  UNUSED(me);
  UNUSED(cmd);
  UNUSED(argument);

  /* Typed dispatch supplies the exact owner and event. */
  return FALSE;
}

int rol_lavatubes_mobile_typed(struct spec_event_context *context)
{
  struct char_data *mob;

  if (context == NULL || context->owner_type != SPEC_OWNER_MOBILE ||
      context->event != SPEC_EVENT_MOBILE_ACTIVITY)
    return FALSE;

  mob = context->owner;
  if (mob == NULL || !IS_MOB(mob))
    return FALSE;

  switch (GET_MOB_VNUM(mob))
  {
  case ROL_LAVATUBES_SNOWVULTURE_VNUM:
    return rol_lavatubes_snowvulture(mob, context->argument);
  case ROL_LAVATUBES_AUTOMATON_VNUM:
    return rol_lavatubes_automaton_unblock(mob);
  default:
    return FALSE;
  }
}

int rol_lavatubes_object(struct char_data *ch, void *me, int cmd, const char *argument)
{
  UNUSED(ch);
  UNUSED(me);
  UNUSED(cmd);
  UNUSED(argument);

  /* Typed dispatch supplies the exact owner and event. */
  return FALSE;
}

int rol_lavatubes_object_typed(struct spec_event_context *context)
{
  struct obj_data *obj;

  if (context == NULL || context->owner_type != SPEC_OWNER_OBJECT ||
      context->event != SPEC_EVENT_COMMAND || context->actor == NULL)
    return FALSE;

  obj = context->owner;
  if (obj == NULL || !VALID_OBJ_RNUM(obj))
    return FALSE;

  switch (GET_OBJ_VNUM(obj))
  {
  case ROL_LAVATUBES_CRYSTAL_SPIKE_VNUM:
    return rol_lavatubes_crystal_spike(context, context->actor, obj);
  case ROL_LAVATUBES_SKELETON_KEY_VNUM:
    return rol_lavatubes_skeleton_key(context, context->actor, obj);
  case ROL_LAVATUBES_LEVER_VNUM:
    return rol_lavatubes_automaton_lever(context->actor, obj, context->command, context->argument);
  default:
    return FALSE;
  }
}

int rol_lavatubes_room(struct char_data *ch, void *me, int cmd, const char *argument)
{
  UNUSED(ch);
  UNUSED(me);
  UNUSED(cmd);
  UNUSED(argument);

  /* Typed dispatch supplies the exact owner and event. */
  return FALSE;
}

int rol_lavatubes_room_typed(struct spec_event_context *context)
{
  if (context == NULL || context->owner_type != SPEC_OWNER_ROOM ||
      context->event != SPEC_EVENT_COMMAND)
    return FALSE;

  return rol_lavatubes_automaton_trapdoor(context->actor, context->owner, context->command);
}
