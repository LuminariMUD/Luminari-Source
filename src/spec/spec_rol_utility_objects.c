/**
 * @file spec/spec_rol_utility_objects.c
 * Converted Realms of Luminari utility-object special procedures.
 */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"

#include "act.h"
#include "combat/fight.h"
#include "comm.h"
#include "db.h"
#include "dgscript/dg_scripts.h"
#include "handler.h"
#include "interpreter.h"
#include "magic/spells.h"
#include "spec_dispatch.h"
#include "spec_rol_utility_objects.h"

#define ROL_GOODBERRY_VNUM 2000876
#define ROL_BLOODSTONE_CHILD_VNUM 2007151
#define ROL_NECROMANCER_CHILD_VNUM 2046991
#define ROL_MENDEN_FIGURINE_VNUM 2088825
#define ROL_RUBY_MONOCLE_VNUM 2090004
#define ROL_RUBY_MONOCLE_ROOM_MIN 2090124
#define ROL_RUBY_MONOCLE_ROOM_MAX 2090142

static const char *rol_utility_command_name(int cmd)
{
  if (cmd <= 0 || complete_cmd_info == NULL)
    return NULL;
  return complete_cmd_info[cmd].command;
}

static bool rol_utility_command_is(int cmd, const char *name)
{
  const char *command;

  command = rol_utility_command_name(cmd);
  return command != NULL && name != NULL && !strcmp(command, name);
}

static int rol_utility_held_slot(const struct char_data *ch, const struct obj_data *obj)
{
  static const int slots[] = {WEAR_WIELD_1, WEAR_HOLD_1,   WEAR_WIELD_OFFHAND,
                              WEAR_HOLD_2,  WEAR_WIELD_2H, WEAR_HOLD_2H};
  size_t index;

  if (ch == NULL || obj == NULL || obj->worn_by != ch)
    return -1;

  for (index = 0; index < sizeof(slots) / sizeof(slots[0]); index++)
    if (GET_EQ(ch, slots[index]) == obj)
      return slots[index];

  return -1;
}

bool rol_utility_sacrifice_keyword(const char *argument)
{
  char keyword[MAX_INPUT_LENGTH];

  if (argument == NULL)
    return false;

  one_argument(argument, keyword, sizeof(keyword));
  return !strcasecmp(keyword, "child") || !strcasecmp(keyword, "all");
}

bool rol_utility_sacrifice_command_name(const char *command)
{
  return command != NULL &&
         (!strcmp(command, "get") || !strcmp(command, "take") || !strcmp(command, "drag"));
}

const char *rol_utility_necro_child_message(int roll)
{
  static const char *const messages[] = {
      "The necromancer's child screams at the top of his lungs!\r\n",
      "The necromancer's child throws a fit!\r\n",
      "The necromancer's child shrieks in anger!\r\n",
      "The necromancer's child struggles and screams!\r\n",
      "The necromancer's child kicks and spits in fury!\r\n",
      "The necromancer's child glares at you evilly!\r\n",
      "The necromancer's child tries to bite you!\r\n",
  };

  if (roll < 0 || roll >= (int)(sizeof(messages) / sizeof(messages[0])))
    return NULL;
  return messages[roll];
}

bool rol_utility_monocle_room(int room_vnum)
{
  return room_vnum >= ROL_RUBY_MONOCLE_ROOM_MIN && room_vnum <= ROL_RUBY_MONOCLE_ROOM_MAX;
}

static int rol_utility_child_sacrifice(struct spec_event_context *context, struct char_data *ch,
                                       struct obj_data *obj)
{
  int damage_amount;

  if (!rol_utility_sacrifice_command_name(rol_utility_command_name(context->command)) ||
      !AWAKE(ch) || !VALID_ROOM_RNUM(IN_ROOM(obj)) || IN_ROOM(ch) != IN_ROOM(obj) ||
      !rol_utility_sacrifice_keyword(context->argument))
    return FALSE;

  act("You attempt to pick up $p...\r\n"
      "Suddenly, a wave of revulsion rushes through you!\r\n"
      "You drop $p, leaving your hands reddened and sore.",
      FALSE, ch, obj, NULL, TO_CHAR);
  act("As $n attempts to pick up $p, $e shudders and suddenly drops it.", TRUE, ch, obj, NULL,
      TO_ROOM);

  damage_amount = rand_number(1, 9);
  if (GET_HIT(ch) - damage_amount < -10)
  {
    act("Your wounds prove too much for you!", FALSE, ch, NULL, NULL, TO_CHAR);
    act("$n's wounds prove too much for $m!", TRUE, ch, NULL, NULL, TO_ROOM);
    GET_HIT(ch) = -11;
    update_pos(ch);
    die(ch, ch);
    context->invalidation |= SPEC_INVALIDATE_ACTOR;
  }
  else
  {
    GET_HIT(ch) -= damage_amount;
    update_pos(ch);
  }

  return TRUE;
}

static int rol_utility_goodberry(struct char_data *ch, struct obj_data *obj, int cmd,
                                 const char *argument)
{
  struct char_data *dummy_character;
  struct obj_data *selected;

  if (!rol_utility_command_is(cmd, "eat") || !AWAKE(ch) || obj->carried_by != ch ||
      GET_COND(ch, HUNGER) > 20 || argument == NULL ||
      !generic_find(argument, FIND_OBJ_INV, ch, &dummy_character, &selected) || selected != obj)
    return FALSE;

  /* The source schedules this cure one tick after the native eat. Selecting
   * the first matching carried object prevents duplicate healing while the
   * target's native eat command consumes the same berry. */
  call_magic(ch, ch, NULL, SPELL_CURE_LIGHT, 0, 50, CAST_INNATE);
  return FALSE;
}

static int rol_utility_menden_figurine(struct spec_event_context *context, struct char_data *ch,
                                       struct obj_data *obj)
{
  struct char_data *summoned;
  struct obj_data *removed;
  char name[MAX_INPUT_LENGTH];
  int slot;

  if (!rol_utility_command_is(context->command, "flex") || !AWAKE(ch) ||
      (slot = rol_utility_held_slot(ch, obj)) < 0 || context->argument == NULL)
    return FALSE;

  one_argument(context->argument, name, sizeof(name));
  if (*name == '\0' || !isname(name, obj->name))
    return FALSE;

  summoned = read_mobile(GET_OBJ_VAL(obj, 0), VIRTUAL);
  if (summoned == NULL)
  {
    log("SYSERR: RoL Menden figurine %d cannot load mobile %d", GET_OBJ_VNUM(obj),
        GET_OBJ_VAL(obj, 0));
    send_to_char(ch, "The figurine fails to awaken. Please tell a staff member.\r\n");
    return TRUE;
  }

  char_to_room(summoned, IN_ROOM(ch));
  load_mtrigger(summoned);
  SET_BIT_AR(AFF_FLAGS(summoned), AFF_CHARM);

  removed = unequip_char(ch, slot);
  if (removed != obj)
  {
    log("SYSERR: RoL Menden figurine changed held slot during command dispatch");
    if (removed != NULL)
      equip_char(ch, removed, slot);
    extract_char(summoned);
    return TRUE;
  }

  act("You flex $p several times, until it finally snaps!", TRUE, ch, obj, NULL, TO_CHAR);
  act("$n flexes $p several times, until it finally snaps!", TRUE, ch, obj, NULL, TO_ROOM);
  act("From the $o come swirling vapors, which solidify to form $n.", TRUE, summoned, obj, NULL,
      TO_ROOM);
  act("The $o rapidly disintegrates to powder, only to be borne away by a sudden wind.", TRUE,
      summoned, obj, NULL, TO_ROOM);
  add_follower(summoned, ch);
  extract_obj(removed);
  context->invalidation |= SPEC_INVALIDATE_OWNER;
  return TRUE;
}

static int rol_utility_necro_child(struct char_data *ch, struct obj_data *obj)
{
  const char *message;

  if (ch == NULL || obj->carried_by != ch || !VALID_ROOM_RNUM(IN_ROOM(ch)) ||
      ROOM_FLAGGED(IN_ROOM(ch), ROOM_SOUNDPROOF) || AFF_FLAGGED(ch, AFF_SILENCED))
    return FALSE;

  message = rol_utility_necro_child_message(rand_number(0, 10));
  if (message != NULL)
    send_to_char(ch, "%s", message);
  return FALSE;
}

static int rol_utility_ruby_monocle(struct obj_data *obj)
{
  room_rnum destination;
  room_rnum room;
  zone_rnum zone;
  int destination_vnum;

  room = IN_ROOM(obj);
  if (!VALID_ROOM_RNUM(room) || !rol_utility_monocle_room(GET_ROOM_VNUM(room)))
    return FALSE;

  zone = world[room].zone;
  if (zone > top_of_zone_table || zone_table[zone].age != 0)
    return FALSE;

  destination_vnum = rand_number(ROL_RUBY_MONOCLE_ROOM_MIN, ROL_RUBY_MONOCLE_ROOM_MAX);
  destination = real_room(destination_vnum);
  if (!VALID_ROOM_RNUM(destination))
  {
    log("SYSERR: RoL ruby monocle selected unavailable room %d", destination_vnum);
    return FALSE;
  }

  obj_from_room(obj);
  obj_to_room(obj, destination);
  return TRUE;
}

int rol_utility_object(struct char_data *ch, void *me, int cmd, const char *argument)
{
  UNUSED(ch);
  UNUSED(me);
  UNUSED(cmd);
  UNUSED(argument);

  /* Typed dispatch supplies the exact owner, event, and invalidation contract. */
  return FALSE;
}

int rol_utility_object_typed(struct spec_event_context *context)
{
  struct char_data *ch;
  struct obj_data *obj;

  if (context == NULL || context->owner_type != SPEC_OWNER_OBJECT || context->owner == NULL)
    return FALSE;

  obj = context->owner;
  if (!VALID_OBJ_RNUM(obj))
    return FALSE;

  if (context->event == SPEC_EVENT_OBJECT_AUTO_PULSE)
  {
    switch (GET_OBJ_VNUM(obj))
    {
    case ROL_NECROMANCER_CHILD_VNUM:
      return rol_utility_necro_child(context->actor, obj);
    case ROL_RUBY_MONOCLE_VNUM:
      return rol_utility_ruby_monocle(obj);
    default:
      return FALSE;
    }
  }

  if (context->event != SPEC_EVENT_COMMAND || context->actor == NULL)
    return FALSE;

  ch = context->actor;
  switch (GET_OBJ_VNUM(obj))
  {
  case ROL_GOODBERRY_VNUM:
    return rol_utility_goodberry(ch, obj, context->command, context->argument);
  case ROL_BLOODSTONE_CHILD_VNUM:
    return rol_utility_child_sacrifice(context, ch, obj);
  case ROL_MENDEN_FIGURINE_VNUM:
    return rol_utility_menden_figurine(context, ch, obj);
  default:
    return FALSE;
  }
}
