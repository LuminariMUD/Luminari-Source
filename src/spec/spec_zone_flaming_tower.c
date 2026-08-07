/**************************************************************************
 *  File: spec/spec_zone_flaming_tower.c               Part of LuminariMUD *
 *  Usage: Flaming Tower zone procedures.                                 *
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
#include "movement/movement.h"
#include "spec_zone_flaming_tower.h"

/* from homeland */
SPECIAL(wallach)
{
  if (cmd || GET_POS(ch) == POS_DEAD)
    return FALSE;

  if (GET_ROOM_VNUM(GET_MOB_LOADROOM(ch)) != 112638)
    GET_MOB_LOADROOM(ch) = real_room(112638);

  return FALSE;
}

/* from homeland */
SPECIAL(beltush)
{
  struct char_data *i;

  if (cmd || GET_POS(ch) == POS_DEAD || GET_ROOM_VNUM(ch->in_room) != 112648)
    return FALSE;

  for (i = character_list; i; i = i->next)
    if (!IS_NPC(i) && GET_ROOM_VNUM(i->in_room) == 112602)
    {
      do_enter(ch, "mirror", 0, 0);
      act("Beltush says, 'FOOLS!! How dare you attempt to enter the flaming "
          "tower!!",
          FALSE, ch, 0, 0, TO_ROOM);
      return TRUE;
    }

  return FALSE;
}
