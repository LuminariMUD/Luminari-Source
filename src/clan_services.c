/**************************************************************************
 *  File: clan_services.c                              Part of LuminariMUD *
 *  Usage: Clan hall mobile services and access control.                   *
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
#include "act.h"
#include "mudlim.h"
#include "magic/spells.h"
#include "clan_services.h"
#include "clan.h"

SPECIAL(clan_cleric)
{
  int i;
  char buf[MAX_STRING_LENGTH] = {'\0'};
  zone_vnum clanhall;
  clan_vnum clan;
  struct char_data *this_mob = (struct char_data *)me;

  struct price_info
  {
    short int number;
    char name[25];
    short int price;
  } clan_prices[] = {/* Spell Num (defined)      Name shown        Price  */
                     {SPELL_SHIELD_OF_FAITH, "shield of faith  ", 75},
                     {SPELL_BLESS, "bless            ", 150},
                     {SPELL_REMOVE_POISON, "remove poison    ", 525},
                     {SPELL_CURE_BLIND, "cure blindness   ", 375},
                     {SPELL_CURE_CRITIC, "critical         ", 525},
                     {SPELL_SANCTUARY, "sanctuary       ", 3000},
                     {SPELL_HEAL, "heal            ", 3500},

                     /* The next line must be last, add new spells above. */
                     {-1, "\r\n", -1}};

  if (CMD_IS("buy") || CMD_IS("list"))
  {
    argument = one_argument_u(argument, buf);

    /* Which clanhall is this cleric in? */
    clanhall = zone_table[(GET_ROOM_ZONE(IN_ROOM(this_mob)))].number;
    if ((clan = zone_is_clanhall(clanhall)) == NO_CLAN)
    {
      log("SYSERR: clan_cleric spec (%s) not in a known clanhall (room %d)", GET_NAME(this_mob),
          world[(IN_ROOM(this_mob))].number);
      return FALSE;
    }
    if (clan != GET_CLAN(ch))
    {
      snprintf(buf, sizeof(buf), "$n will only serve members of %s", CLAN_NAME(real_clan(clan)));
      act(buf, TRUE, this_mob, 0, ch, TO_VICT);
      return TRUE;
    }

    if (FIGHTING(ch))
    {
      send_to_char(ch, "You can't do that while fighting!\r\n");
      return TRUE;
    }

    if (*buf)
    {
      for (i = 0; clan_prices[i].number > SPELL_RESERVED_DBC; i++)
      {
        if (is_abbrev(buf, clan_prices[i].name))
        {
          if (GET_GOLD(ch) < clan_prices[i].price)
          {
            act("$n tells you, 'You don't have enough gold for that spell!'", FALSE, this_mob, 0,
                ch, TO_VICT);
            return TRUE;
          }
          else
          {
            act("$N gives $n some money.", FALSE, this_mob, 0, ch, TO_NOTVICT);
            send_to_char(ch, "You give %s %d coins.\r\n", GET_NAME(this_mob), clan_prices[i].price);
            decrease_gold(ch, clan_prices[i].price);
            /* Uncomment the next line to make the mob get RICH! */
            /* increase_gold(this_mob, clan_prices[i].price); */

            cast_spell(this_mob, ch, NULL, clan_prices[i].number, 0);
            return TRUE;
          }
        }
      }
      act("$n tells you, 'I do not know of that spell!"
          "  Type 'buy' for a list.'",
          FALSE, this_mob, 0, ch, TO_VICT);

      return TRUE;
    }
    else
    {
      act("$n tells you, 'Here is a listing of the prices for my services.'", FALSE, this_mob, 0,
          ch, TO_VICT);
      for (i = 0; clan_prices[i].number > SPELL_RESERVED_DBC; i++)
      {
        send_to_char(ch, "%s%d\r\n", clan_prices[i].name, clan_prices[i].price);
      }
      return TRUE;
    }
  }
  return FALSE;
}

SPECIAL(clan_guard)
{
  zone_vnum clanhall, to_zone;
  clan_vnum clan;
  struct char_data *guard = (struct char_data *)me;
  const char *buf = "The guard humiliates you, and blocks your way.\r\n";
  const char *buf2 = "The guard humiliates $n, and blocks $s way.";

  if (!IS_MOVE(cmd) || IS_AFFECTED(guard, AFF_BLIND))
    return FALSE;

  if (GET_LEVEL(ch) >= LVL_IMMORT)
    return FALSE;

  /* Which clanhall is this cleric in? */
  clanhall = zone_table[(GET_ROOM_ZONE(IN_ROOM(guard)))].number;
  if ((clan = zone_is_clanhall(clanhall)) == NO_CLAN)
  {
    log("SYSERR: clan_guard spec (%s) not in a known clanhall (room %d)", GET_NAME(guard),
        world[(IN_ROOM(guard))].number);
    return FALSE;
  }

  /* This is the player's clanhall, allow them to pass */
  if (GET_CLAN(ch) == clan)
  {
    return FALSE;
  }

  /* If the exit leads to another clanhall room, block it */
  /* NOTE: cmd equals the direction for directional commands */
  if (EXIT(ch, cmd) && EXIT(ch, cmd)->to_room && EXIT(ch, cmd)->to_room != NOWHERE)
  {
    to_zone = zone_table[(GET_ROOM_ZONE(EXIT(ch, cmd)->to_room))].number;
    if (to_zone == clanhall)
    {
      act(buf, FALSE, ch, 0, 0, TO_CHAR);
      act(buf2, FALSE, ch, 0, 0, TO_ROOM);
      return TRUE;
    }
  }

  /* If we get here, player is allowed to leave */
  return FALSE;
}
