/**************************************************************************
 *  File: spec/spec_zone_abyss.c                       Part of LuminariMUD *
 *  Usage: Abyss zone procedures.                                         *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "spec_zone_abyss.h"

#define ZONE_VNUM 1423

/* just made this to help facilitate switching of zone vnums if needed */
static room_vnum calc_room_num(int value)
{
  return (ZONE_VNUM * 100) + value;
}

/* this proc swaps exits in the rooms in a given area */
SPECIAL(abyss_randomizer)
{
  struct char_data *i = NULL;
  char buf[MAX_INPUT_LENGTH] = {'\0'};
  room_vnum room;
  room_rnum current_room, temp1, temp2;

  if (cmd)
    return 0;

  if (rand_number(0, 9))
    return 0;

  for (room = calc_room_num(1); room <= calc_room_num(18); room++)
  {
    current_room = real_room(room);
    if (current_room == NOWHERE)
      continue;

    /* Swapping North and South */
    if (world[current_room].dir_option[NORTH] &&
        world[current_room].dir_option[NORTH]->to_room != NOWHERE)
      temp1 = world[current_room].dir_option[NORTH]->to_room;
    else
      temp1 = NOWHERE;
    if (world[current_room].dir_option[SOUTH] &&
        world[current_room].dir_option[SOUTH]->to_room != NOWHERE)
      temp2 = world[current_room].dir_option[SOUTH]->to_room;
    else
      temp2 = NOWHERE;
    if (temp2 != NOWHERE)
    {
      if (!world[current_room].dir_option[NORTH])
        CREATE(world[current_room].dir_option[NORTH], struct room_direction_data, 1);
      world[current_room].dir_option[NORTH]->to_room = temp2;
    }
    else if (world[current_room].dir_option[NORTH])
    {
      free(world[current_room].dir_option[NORTH]);
      world[current_room].dir_option[NORTH] = NULL;
    }
    if (temp1 != NOWHERE)
    {
      if (!world[current_room].dir_option[SOUTH])
        CREATE(world[current_room].dir_option[SOUTH], struct room_direction_data, 1);
      world[current_room].dir_option[SOUTH]->to_room = temp1;
    }
    else if (world[current_room].dir_option[SOUTH])
    {
      free(world[current_room].dir_option[SOUTH]);
      world[current_room].dir_option[SOUTH] = NULL;
    }

    /* Swapping East and West */
    if (world[current_room].dir_option[EAST] &&
        world[current_room].dir_option[EAST]->to_room != NOWHERE)
      temp1 = world[current_room].dir_option[EAST]->to_room;
    else
      temp1 = NOWHERE;
    if (world[current_room].dir_option[WEST] &&
        world[current_room].dir_option[WEST]->to_room != NOWHERE)
      temp2 = world[current_room].dir_option[WEST]->to_room;
    else
      temp2 = NOWHERE;
    if (temp2 != NOWHERE)
    {
      if (!world[current_room].dir_option[EAST])
        CREATE(world[current_room].dir_option[EAST], struct room_direction_data, 1);
      world[current_room].dir_option[EAST]->to_room = temp2;
    }
    else if (world[current_room].dir_option[EAST])
    {
      free(world[current_room].dir_option[EAST]);
      world[current_room].dir_option[EAST] = NULL;
    }
    if (temp1 != NOWHERE)
    {
      if (!world[current_room].dir_option[WEST])
        CREATE(world[current_room].dir_option[WEST], struct room_direction_data, 1);
      world[current_room].dir_option[WEST]->to_room = temp1;
    }
    else if (world[current_room].dir_option[WEST])
    {
      free(world[current_room].dir_option[WEST]);
      world[current_room].dir_option[WEST] = NULL;
    }

    /* Swapping Up and Down */
    if (world[current_room].dir_option[UP] &&
        world[current_room].dir_option[UP]->to_room != NOWHERE)
      temp1 = world[current_room].dir_option[UP]->to_room;
    else
      temp1 = NOWHERE;
    if (world[current_room].dir_option[DOWN] &&
        world[current_room].dir_option[DOWN]->to_room != NOWHERE)
      temp2 = world[current_room].dir_option[DOWN]->to_room;
    else
      temp2 = NOWHERE;
    if (temp2 != NOWHERE)
    {
      if (!world[current_room].dir_option[UP])
        CREATE(world[current_room].dir_option[UP], struct room_direction_data, 1);
      world[current_room].dir_option[UP]->to_room = temp2;
    }
    else if (world[current_room].dir_option[UP])
    {
      free(world[current_room].dir_option[UP]);
      world[current_room].dir_option[UP] = NULL;
    }
    if (temp1 != NOWHERE)
    {
      if (!world[current_room].dir_option[DOWN])
        CREATE(world[current_room].dir_option[DOWN], struct room_direction_data, 1);
      world[current_room].dir_option[DOWN]->to_room = temp1;
    }
    else if (world[current_room].dir_option[DOWN])
    {
      free(world[current_room].dir_option[DOWN]);
      world[current_room].dir_option[DOWN] = NULL;
    }
  }

  snprintf(buf, sizeof(buf), "\tpThe world seems to shift.\tn\r\n");

  for (i = character_list; i; i = i->next)
    if (world[ch->in_room].zone == world[i->in_room].zone)
      send_to_char(i, "%s", buf);

  return 0;
}

#undef ZONE_VNUM
