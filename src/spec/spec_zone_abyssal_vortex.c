/**************************************************************************
 *  File: spec/spec_zone_abyssal_vortex.c                Part of LuminariMUD *
 *  Usage: Abyssal Vortex zone procedures.                                  *
 *                                                                           *
 *  All rights reserved.  See license for complete information.              *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "spec_zone_abyssal_vortex.h"

/* from homeland */
SPECIAL(abyssal_vortex)
{
  int temp;

  if (cmd)
    return FALSE;

  if (IS_NPC(ch) && !IS_PET(ch))
    return FALSE;

  if (!rand_number(0, 7))
  {
    temp = world[ch->in_room].dir_option[0]->to_room;
    world[ch->in_room].dir_option[0]->to_room = world[ch->in_room].dir_option[1]->to_room;
    world[ch->in_room].dir_option[1]->to_room = world[ch->in_room].dir_option[4]->to_room;
    world[ch->in_room].dir_option[4]->to_room = world[ch->in_room].dir_option[3]->to_room;
    world[ch->in_room].dir_option[3]->to_room = world[ch->in_room].dir_option[5]->to_room;
    world[ch->in_room].dir_option[5]->to_room = world[ch->in_room].dir_option[2]->to_room;
    world[ch->in_room].dir_option[2]->to_room = temp;

    send_to_room(ch->in_room,
                 "\tLThe reality seems to \tCshift\tL as madness descends in the \tcvortex\tn\r\n");

    return TRUE;
  }
  return FALSE;
}
