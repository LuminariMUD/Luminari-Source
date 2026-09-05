/**************************************************************************
 *  File: vessels/vessels_legacy.c                     Part of LuminariMUD *
 *  Usage: Legacy route and Greyhawk vessel special procedures.            *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "constants.h"
#include "act.h"
#include "vessels_legacy.h"
#include "modify.h"
#include "graph.h"
#include "vessels/vessels.h"
#include "point_update_periodic.h"

/* External vessel data for ship boarding. */
extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];

/* very simple ship code system */
#define NUM_OF_SHIPS 4

// shiproom, shipobject, room_seq_start, roomseq_end
int ship_info[NUM_OF_SHIPS][4] = {
    // chionthar ferry
    {104072, 104072, 104262, 104266},
    {104073, 104072, 104262, 104266},

    // alanthor ferry
    {126429, 126429, 126423, 126428},

    // md carpet
    {120013, 120010, 120036, 120040},
};

struct obj_data *find_ship(room_rnum room)
{
  int i, j;
  room_rnum ship_room;
  room_rnum route_room;
  struct obj_data *obj;

  for (i = 0; i < NUM_OF_SHIPS; i++)
  {
    ship_room = real_room(ship_info[i][0]);
    if (ship_room != NOWHERE && room == ship_room)
    {
      for (j = ship_info[i][2]; j <= ship_info[i][3]; j++)
      {
        route_room = real_room(j);
        if (route_room == NOWHERE)
          continue;
        for (obj = world[route_room].contents; obj; obj = obj->next_content)
        {
          if (GET_OBJ_VNUM(obj) == (obj_vnum)ship_info[i][1])
            return obj;
        }
      }
      return NULL;
    }
  }
  return NULL;
}

void move_ship(struct obj_data *ship, int dir)
{
  room_rnum new_room;
  room_rnum ship_room;
  const char *msg = NULL;
  int i;
  char buf2[MAX_INPUT_LENGTH] = {'\0'};

  if (!ship || ship->in_room == NOWHERE || ship->in_room > top_of_world || dir < 0 || dir >= 6)
    return;

  if (!world[ship->in_room].dir_option[dir])
    return;

  new_room = world[ship->in_room].dir_option[dir]->to_room;

  if (new_room == NOWHERE || new_room > top_of_world)
    return;

  snprintf(buf2, sizeof(buf2), "$p floats %s.", dirs[dir]);
  act(buf2, TRUE, 0, ship, 0, TO_ROOM);

  obj_from_room(ship);
  obj_to_room(ship, new_room);

  snprintf(buf2, sizeof(buf2), "The ship moves %s.\r\n", dirs[dir]);

  if (world[ship->in_room].sector_type == SECT_ZONE_START)
    msg = "Your ship docks here.\r\n";

  for (i = 0; i < NUM_OF_SHIPS; i++)
  {
    if (GET_OBJ_VNUM(ship) == (obj_vnum)ship_info[i][1])
    {
      ship_room = real_room(ship_info[i][0]);
      if (ship_room == NOWHERE)
        continue;
      send_to_room(ship_room, "%s", buf2);
      if (msg)
        send_to_room(ship_room, "%s", msg);
    }
  }

  snprintf(buf2, sizeof(buf2), "$p floats in from the %s.", dirs[rev_dir[dir]]);
  act(buf2, TRUE, 0, ship, 0, TO_ROOM);
}

// use timer for count.
// weight is wether towards start or end.

void update_ship(struct obj_data *ship, room_vnum start, room_vnum end, int movedelay,
                 int waitdelay)
{
  room_rnum dest = real_room(end);

  if (!ship->obj_flags.weight)
    dest = real_room(start);

  if (dest == NOWHERE)
  {
    log("SYSERR: update_ship could not resolve destination room.");
    return;
  }

  ship->obj_flags.timer--;

  if (ship->obj_flags.timer >= 0)
  {
    point_update_object_sync(ship);
    return;
  }

  ship->obj_flags.timer = movedelay;

  if (dest != ship->in_room)
    move_ship(ship, find_first_step(ship->in_room, dest));

  if (ship->in_room == dest)
  {
    // turn around ship
    ship->obj_flags.weight = !ship->obj_flags.weight;
    ship->obj_flags.timer = waitdelay;
  }
  point_update_object_sync(ship);
}

void ship_lookout(struct char_data *ch)
{
  struct obj_data *ship = find_ship(ch->in_room);
  if (ship == 0)
  {
    send_to_char(ch, "But you are not at a ship to look out from!\r\n");
    return;
  }
  look_at_room_number(ch, 1, ship->in_room);
}

ACMD(do_disembark)
{
  struct obj_data *ship;
  ship = find_ship(ch->in_room);

  if (!ship)
  {
    send_to_char(ch, "But you are not on any ship.\r\n");
    return;
  }
  if (world[ship->in_room].sector_type != SECT_ZONE_START)
  {
    send_to_char(ch, "You can only disembark when the ship is docked.\r\n");
    return;
  }

  // int was_in = ch->in_room;
  act("$n disembarks.", TRUE, ch, 0, 0, TO_ROOM);
  char_from_room(ch);

  if (ZONE_FLAGGED(GET_ROOM_ZONE(ship->in_room), ZONE_WILDERNESS))
  {
    X_LOC(ch) = world[ship->in_room].coords[0];
    Y_LOC(ch) = world[ship->in_room].coords[1];
  }

  char_to_room(ch, ship->in_room);
  act("$n disembarks from $p.", TRUE, ch, ship, 0, TO_ROOM);
  look_at_room(ch, 0);
}


/* from homeland */
SPECIAL(chionthar_ferry)
{
  if (cmd)
    return FALSE;

  if (!cmd && argument && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "This is a ferry.\r\n");
    return TRUE;
  }

  update_ship((struct obj_data *)me, 104262, 104266, 1, 4);
  return TRUE;
}

/* from homeland */
SPECIAL(alandor_ferry)
{
  if (cmd)
    return FALSE;

  if (!cmd && argument && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "This is a ferry.\r\n");
    return TRUE;
  }

  update_ship((struct obj_data *)me, 126423, 126428, 1, 4);
  return TRUE;
}

/* from homeland */
SPECIAL(md_carpet)
{
  if (cmd)
    return FALSE;

  if (!cmd && argument && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "This is a transport carpet.\r\n");
    return TRUE;
  }

  update_ship((struct obj_data *)me, 120036, 120040, 3, 10);
  return TRUE;
}

/* Vessel/Ship Special Procedures */

/* Special procedure for Greyhawk ship objects - handles boarding */
SPECIAL(greyhawk_ship_object)
{
  struct obj_data *obj = (struct obj_data *)me;
  struct obj_data *target;
  char obj_name[MAX_INPUT_LENGTH];
  int ship_index;
  room_rnum interior_room;

  /* Only handle 'board' command */
  if (!cmd || !CMD_IS("board"))
    return 0;

  one_argument_u(argument, obj_name);
  if (*obj_name)
  {
    target = get_obj_in_list_vis(ch, obj_name, NULL, world[IN_ROOM(ch)].contents);
    if (target != obj)
    {
      return 0;
    }
  }

  if (!CONFIG_VESSEL_SYSTEM)
  {
    send_to_char(ch, "The vessel system is currently disabled.\r\n");
    return 1;
  }

  /* Validate object type */
  if (GET_OBJ_TYPE(obj) != ITEM_GREYHAWK_SHIP)
  {
    send_to_char(ch, "This is not a ship you can board.\r\n");
    return 0;
  }

  /* Get ship index from object value 1 */
  ship_index = GET_OBJ_VAL(obj, 1);
  if (ship_index < 0 || ship_index >= GREYHAWK_MAXSHIPS)
  {
    send_to_char(ch, "This ship seems to be broken.\r\n");
    return 0;
  }

  if (!is_valid_ship(&greyhawk_ships[ship_index]))
  {
    send_to_char(ch, "This ship seems to be broken.\r\n");
    log("SYSERR: Ship object %d points to inactive or mismatched fleet slot %d", GET_OBJ_VNUM(obj),
        ship_index);
    return 0;
  }

  /* Get interior room from object value 0 */
  interior_room = real_room(GET_OBJ_VAL(obj, 0));
  if (interior_room == NOWHERE)
  {
    send_to_char(ch, "You cannot find a way inside this ship.\r\n");
    return 0;
  }

  if (greyhawk_ships[ship_index].shiproom != GET_OBJ_VAL(obj, 0))
  {
    send_to_char(ch, "This ship's entrance is not linked correctly.\r\n");
    log("SYSERR: Ship object %d entrance %d disagrees with fleet slot %d room %d",
        GET_OBJ_VNUM(obj), GET_OBJ_VAL(obj, 0), ship_index, greyhawk_ships[ship_index].shiproom);
    return 0;
  }

  if (!vessel_collect_passenger_fare(ch, &greyhawk_ships[ship_index]))
  {
    return 1;
  }

  /* Link interior room to ship data - required for disembark and ship commands */
  world[interior_room].ship = &greyhawk_ships[ship_index];

  /* Link ship data to ship object - required for coordinate sync to move object */
  greyhawk_ships[ship_index].shipobj = obj;

  /* Move character to ship interior */
  act("$n boards $p.", TRUE, ch, obj, 0, TO_ROOM);
  char_from_room(ch);
  char_to_room(ch, interior_room);
  act("$n arrives from outside.", TRUE, ch, 0, 0, TO_ROOM);

  send_to_char(ch, "You board the ship.\r\n");
  look_at_room(ch, 0);

  return 1;
}

/* Special procedure for ship control rooms - handles ship commands */
SPECIAL(greyhawk_ship_commands)
{
  room_rnum room;

  /* Only process ship-related commands */
  if (!cmd)
    return 0;

  room = ch->in_room;

  /* Find which ship this room belongs to by checking ship data */
  /* This would need to iterate through ships to find matching interior room */
  /* For now, we'll use a simplified approach */

  /* Check if this is a ship control command (bridge-only commands) */
  /* Note: disembark removed - handled by do_greyhawk_disembark from any ship room */
  if (CMD_IS("setsail") || CMD_IS("heading") || CMD_IS("speed") || CMD_IS("anchor") ||
      CMD_IS("tactical"))
  {
    /* Validate this is actually a ship control room */
    if (!ROOM_FLAGGED(room, ROOM_HOUSE))
    { /* Using ROOM_HOUSE as placeholder for ship rooms */
      send_to_char(ch, "You must be in a ship's control room to use that command.\r\n");
      return 0;
    }

    /* Pass command to vessel system for processing */
    /* The actual command implementations are in vessels.c */
    return 0; /* Let the normal command handler process it */
  }

  return 0;
}
