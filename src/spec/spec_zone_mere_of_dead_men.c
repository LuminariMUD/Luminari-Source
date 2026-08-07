/**************************************************************************
 *  File: spec/spec_zone_mere_of_dead_men.c            Part of LuminariMUD *
 *  Usage: Mere of Dead Men zone procedures.                              *
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
#include "spec_zone_mere_of_dead_men.h"

/* from homeland */
SPECIAL(mereshaman)
{
  if (cmd)
    return FALSE;

  if (FIGHTING(ch) && !PROC_FIRED(ch))
  {
    PROC_FIRED(ch) = TRUE;
    send_to_room(ch->in_room,
                 "\tLThe \tglizardman \tLshaman chants loudly, '\tGUktha slithiss "
                 "Semuanya! Ssithlarss sunggar uk!\tL'\tn\r\n"
                 "\tLThe monitor lizard statues shudder and vibrate then take on \tn\r\n"
                 "\tLa \tGbright green glow\tL. Each opens up like a cocoon releasing the\tn\r\n"
                 "\tLreptilian beast contained within.\tn\r\n");

    char_to_room(read_mobile(126725, VIRTUAL), ch->in_room);
    char_to_room(read_mobile(126725, VIRTUAL), ch->in_room);
    char_to_room(read_mobile(126725, VIRTUAL), ch->in_room);
    char_to_room(read_mobile(126725, VIRTUAL), ch->in_room);
    return TRUE;
  }
  return FALSE;
}

/* from homeland */
SPECIAL(willowisp)
{
  room_rnum room = real_room(126899);

  if (cmd)
    return FALSE;

  if (FIGHTING(ch))
    return FALSE;

  if (ch->in_room != room && weather_info.sunlight == SUN_LIGHT)
  {
    act("$n fades away in the sunlight!", FALSE, ch, 0, 0, TO_ROOM);
    ch->mob_specials.temp_room_data = ch->in_room;
    char_from_room(ch);
    char_to_room(ch, room);

    return TRUE;
  }

  if (ch->in_room == room && weather_info.sunlight != SUN_LIGHT)
  {
    char_from_room(ch);
    char_to_room(ch, ch->mob_specials.temp_room_data);
    act("$n appears with the dark of the night!", FALSE, ch, 0, 0, TO_ROOM);
    return TRUE;
  }

  return FALSE;
}
