/**************************************************************************
 *  File: spec/spec_zone_neverwinter.c                 Part of LuminariMUD *
 *  Usage: Cohesive Neverwinter control-puzzle procedures.                  *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "movement/door_state.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "magic/spells.h"
#include "constants.h"
#include "act.h"
#include "spec_zone_neverwinter.h"
#include "character/class.h"
#include "combat/fight.h"
#include "modify.h"
#include "obj/house.h"
#include "clan.h"
#include "mudlim.h"
#include "graph.h"
#include "dgscript/dg_scripts.h"
#include "mud_event.h"
#include "actions.h"
#include "combat/assign_wpn_armor.h"
#include "magic/domains_schools.h"
#include "character/feats.h"
#include "magic/spell_prep.h"
#include "obj/item.h"
#include "craft/alchemy.h"
#include "obj/treasure.h"
#include "mob/mob_utils.h"
#include "character/evolutions.h"
#include "olc/oasis.h"
#include "quest/quest.h"
#include "character/backgrounds.h"
#include "character/perks.h"
#include "point_update_periodic.h"

SPECIAL(neverwinter_button_control)
{
  struct door_state_operation operations[2] = {0};
  size_t door_index;
  struct obj_data *dummy = NULL;
  struct obj_data *obj = (struct obj_data *)me;
  struct char_data *i = NULL;
  bool change = FALSE;

  skip_spaces(&argument);

  if (cmd)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "This appears to be a button control...\r\n");
    return TRUE;
  }

  door_state_begin(&operations[0], real_room(123637), DOWN, false, DOMAIN_DOOR_GAMEPLAY);
  door_state_begin(&operations[1], real_room(123641), UP, false, DOMAIN_DOOR_GAMEPLAY);
  if (!CAN_GO(obj, EAST) && !CAN_GO(obj, SOUTH) && !CAN_GO(obj, WEST))
  {
    if (IS_CLOSED(real_room(123637), DOWN))
    {
      OPEN_DOOR(real_room(123637), dummy, DOWN);
      OPEN_DOOR(real_room(123641), dummy, UP);
      change = TRUE;
    }
  }

  if (change && GET_OBJ_SPECTIMER(obj, 0) == 0)
  {
    for (i = character_list; i; i = i->next)
      if (world[obj->in_room].zone == world[i->in_room].zone)
        send_to_char(i, "\tLYou hear a slight rumbling.\tn\r\n");
    point_update_object_spec_timer_set(obj, 0, 9999);
  }

  for (door_index = 0U; door_index < 2U; door_index++)
    door_state_finish(&operations[door_index]);
  return FALSE;
}

/* from homeland */
SPECIAL(neverwinter_valve_control)
{
  struct door_state_operation operations[6] = {0};
  size_t door_index;
  /* A- North
     B- East
     C- South
     D- West
   */
  struct char_data *i = NULL;
  struct obj_data *dummy = 0;
  struct obj_data *obj = (struct obj_data *)me;
  bool avalve = FALSE, bvalve = FALSE, cvalve = FALSE, dvalve = FALSE, change = FALSE;

  skip_spaces(&argument);

  if (cmd)
    return FALSE;

  if (!cmd && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "This appears to be a valve control...\r\n");
    return TRUE;
  }

  door_state_begin(&operations[0], real_room(123440), EAST, false, DOMAIN_DOOR_GAMEPLAY);
  door_state_begin(&operations[1], real_room(123441), WEST, false, DOMAIN_DOOR_GAMEPLAY);
  door_state_begin(&operations[2], real_room(123469), EAST, false, DOMAIN_DOOR_GAMEPLAY);
  door_state_begin(&operations[3], real_room(123470), WEST, false, DOMAIN_DOOR_GAMEPLAY);
  door_state_begin(&operations[4], real_room(123533), EAST, false, DOMAIN_DOOR_GAMEPLAY);
  door_state_begin(&operations[5], real_room(123534), WEST, false, DOMAIN_DOOR_GAMEPLAY);
  if (!CAN_GO(obj, NORTH))
    avalve = TRUE;

  if (!CAN_GO(obj, EAST))
    bvalve = TRUE;

  if (!CAN_GO(obj, SOUTH))
    cvalve = TRUE;

  if (!CAN_GO(obj, WEST))
    dvalve = TRUE;

  if (avalve && bvalve && !cvalve && dvalve)
  {
    if (!IS_CLOSED(real_room(123440), EAST))
      OPEN_DOOR(real_room(123440), dummy, EAST);
    if (!IS_CLOSED(real_room(123441), WEST))
      OPEN_DOOR(real_room(123441), dummy, WEST);
    if (!IS_CLOSED(real_room(123469), EAST))
      OPEN_DOOR(real_room(123469), dummy, EAST);
    if (!IS_CLOSED(real_room(123470), WEST))
      OPEN_DOOR(real_room(123470), dummy, WEST);
    if (IS_CLOSED(real_room(123533), EAST))
    {
      OPEN_DOOR(real_room(123533), dummy, EAST);
      change = TRUE;
    }
    if (IS_CLOSED(real_room(123534), WEST))
      OPEN_DOOR(real_room(123534), dummy, WEST);
  }
  else if (avalve && !bvalve && cvalve && !dvalve)
  {
    if (IS_CLOSED(real_room(123440), EAST))
    {
      OPEN_DOOR(real_room(123440), dummy, EAST);
      change = TRUE;
    }
    if (IS_CLOSED(real_room(123441), WEST))
      OPEN_DOOR(real_room(123441), dummy, WEST);
    if (!IS_CLOSED(real_room(123469), EAST))
      OPEN_DOOR(real_room(123469), dummy, EAST);
    if (!IS_CLOSED(real_room(123470), WEST))
      OPEN_DOOR(real_room(123470), dummy, WEST);
    if (!IS_CLOSED(real_room(123533), EAST))
      OPEN_DOOR(real_room(123533), dummy, EAST);
    if (!IS_CLOSED(real_room(123534), WEST))
      OPEN_DOOR(real_room(123534), dummy, WEST);
  }
  else if (!avalve && bvalve && cvalve && dvalve)
  {
    if (!IS_CLOSED(real_room(123440), EAST))
      OPEN_DOOR(real_room(123440), dummy, EAST);
    if (!IS_CLOSED(real_room(123441), WEST))
      OPEN_DOOR(real_room(123441), dummy, WEST);
    if (IS_CLOSED(real_room(123469), EAST))
    {
      OPEN_DOOR(real_room(123469), dummy, EAST);
      change = TRUE;
    }
    if (IS_CLOSED(real_room(123470), WEST))
      OPEN_DOOR(real_room(123470), dummy, WEST);
    if (!IS_CLOSED(real_room(123533), EAST))
      OPEN_DOOR(real_room(123533), dummy, EAST);
    if (!IS_CLOSED(real_room(123534), WEST))
      OPEN_DOOR(real_room(123534), dummy, WEST);
  }
  else
  {
    if (!IS_CLOSED(real_room(123440), EAST))
      OPEN_DOOR(real_room(123440), dummy, EAST);
    if (!IS_CLOSED(real_room(123441), WEST))
      OPEN_DOOR(real_room(123441), dummy, WEST);
    if (!IS_CLOSED(real_room(123469), EAST))
      OPEN_DOOR(real_room(123469), dummy, EAST);
    if (!IS_CLOSED(real_room(123470), WEST))
      OPEN_DOOR(real_room(123470), dummy, WEST);
    if (!IS_CLOSED(real_room(123533), EAST))
      OPEN_DOOR(real_room(123533), dummy, EAST);
    if (!IS_CLOSED(real_room(123534), WEST))
      OPEN_DOOR(real_room(123534), dummy, WEST);
  }

  if (change)
    for (i = character_list; i; i = i->next)
      if (world[obj->in_room].zone == world[i->in_room].zone)
        send_to_char(i, "\tgYou hear the flow of rushing sewage somewhere.\tn\r\n");

  for (door_index = 0U; door_index < 6U; door_index++)
    door_state_finish(&operations[door_index]);
  return FALSE;
}
