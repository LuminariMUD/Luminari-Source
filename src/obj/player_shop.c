/**************************************************************************
 *  File: obj/player_shop.c                             Part of LuminariMUD *
 *  Usage: Player-owned shop special procedure and helpers.                 *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 **************************************************************************/


#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "magic/spells.h"
#include "constants.h"
#include "act.h"
#include "player_shop.h"
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

/* Player-shop state is owned by the house subsystem. */
extern struct house_control_rec house_control[];
extern int num_of_houses;

/* Player owned shops - Original created by Jamdog - 22nd February 2007
 * Zusuk had to completely re-write it for usage in LuminariMUD and to add
 *   the requested functionality from staff/players
 * Mob Special Function: the mob must be in the ATRIUM of the house
 * Items to be sold will be everything on the ground of the house */

/* do we have a valid player-shop item?  currently checking:
     1) shopper can see the item [unless have holy light]
     2) the item is not hidden (desc including a period .) [unless have holy light]
     3) item type is not ITEM_MONEY
 */

/* debug, comment out to disable */ //#define PLAYER_SHOP_DEBUG

bool valid_player_shop_item(struct char_data *ch, struct obj_data *obj)
{
  if (!obj)
    return FALSE;

  if (!CAN_SEE_OBJ(ch, obj))
    return FALSE;

  if (!*obj->description)
    return FALSE;

  if (*obj->description == '.')
    return FALSE;

  if (GET_OBJ_TYPE(obj) == ITEM_MONEY)
    return FALSE;

  /* made it! */
  return TRUE;
}

/* given shopper (ch), 'private room' and hopefully an argument
   return object based on argument
   will accept an index value that corresponds to the order of the items
     in the shop storage room (for argument) */
struct obj_data *find_player_shop_obj(struct char_data *ch, char *argument, room_rnum private_room)
{
  bool is_number = FALSE;
  int index = 0, num = 1 /*starting index*/;
  struct obj_data *obj = NULL;

  skip_spaces(&argument);

  /* we need to identify if the shopper used a number (reference) to buy -Zusuk */
  if (isdigit(*argument))
  {
    is_number = TRUE;
    index = atoi(argument);
  }

  if (is_number)
  {
#ifdef PLAYER_SHOP_DEBUG
    send_to_char(ch, "player_shops: %s looking for item index (%d) in room %d\r\n", GET_NAME(ch),
                 index, world[private_room].number);
#endif

    for (obj = world[private_room].contents; obj; obj = obj->next_content)
    {
      if (valid_player_shop_item(ch, obj))
      {
        if (num == index) /* found the item obj */
          break;
        num++;
      }
    }

    if (num != index) /* reached end of list without finding index */
      obj = NULL;
  }
  else
  { /* ARGUMENT */

#ifdef PLAYER_SHOP_DEBUG
    send_to_char(ch, "player_shops: %s looking for %s in room %d\r\n", GET_NAME(ch), argument,
                 world[private_room].number);
#endif

    obj = get_obj_in_list_vis(ch, argument, NULL, world[private_room].contents);

    if (!valid_player_shop_item(ch, obj))
      obj = NULL;
  }

  return obj;
}

SPECIAL(player_owned_shops)
{
  room_rnum private_room;
  room_vnum house_vnum;
  struct obj_data *i, *j;
  int num = 1, hse;
  char *temp, shop_owner[32], buf[MAX_STRING_LENGTH] = {'\0'};
  bool found = FALSE;

  if (!cmd)
    return FALSE;

#ifdef PLAYER_SHOP_DEBUG
  send_to_char(ch, "IN_ROOM(ch): %d\r\n", IN_ROOM(ch));
#endif

  /* Grab the name of the shop owner */
  for (hse = 0; hse < num_of_houses; hse++)
  {
#ifdef PLAYER_SHOP_DEBUG
    send_to_char(ch, "House counter: %d, This-atrium: %d\r\n", hse, house_control[hse].atrium);
#endif
    if (real_room(house_control[hse].atrium) == IN_ROOM(ch))
    {
      /* Avoid seeing <UNDEF> entries from self-deleted people. */
      if ((temp = get_name_by_id(house_control[hse].owner)) == NULL)
      {
        snprintf(shop_owner, sizeof(shop_owner), "Someone");
      }
      else
      {
        snprintf(shop_owner, sizeof(shop_owner), "%s",
                 CAP(get_name_by_id(house_control[hse].owner)));
      }
      found = TRUE;
      break;
    }
  }

  if (found == FALSE)
    snprintf(shop_owner, sizeof(shop_owner), "Invalid Shop - Tell an Imp");

  private_room = real_room(house_control[hse].vnum);
  house_vnum = house_control[hse].vnum;

#ifdef PLAYER_SHOP_DEBUG
  send_to_char(ch, "House VNum %d\r\n", house_control[hse].vnum);
#endif

  /** LIST COMMAND **/

  if (CMD_IS("list"))
  {
    if (IS_NPC(ch))
    {
      send_to_char(ch, "Mobiles can't buy from a player-owned shop!\r\n");
      return TRUE;
    }

    snprintf(buf, sizeof(buf), "Owner: \tW%s\tn", shop_owner);
    send_to_char(ch, "Player-owned Shop %*s\r\n", count_color_chars(buf) + 55, buf);
    send_to_char(ch, "###    Item                                                Cost\r\n");
    send_to_char(
        ch, "--------------------------------------------------------------------------------\r\n");

    for (i = world[private_room].contents; i; i = i->next_content)
    {
      if (valid_player_shop_item(ch, i))
      {
        send_to_char(ch, "%3d)   %-*s %11d\r\n", num++,
                     count_color_chars(i->short_description) + 44, i->short_description,
                     GET_OBJ_COST(i));
      }
    }

    return (TRUE);

    /** BUY COMMAND **/
  }
  else if (CMD_IS("buy"))
  {
    /* do we have an item?  accepts an index for the argument */
    i = find_player_shop_obj(ch, argument, private_room);

    if (i == NULL)
    {
      send_to_char(ch, "Can not find that item!  Try the index value in cases "
                       "were some objects have funky keywords.\r\n");
      return (TRUE);
    }

#ifdef PLAYER_SHOP_DEBUG
    send_to_char(ch, "player_shops: found %s (cost: %d)\r\n", i->short_description,
                 GET_OBJ_COST(i));
#endif

    if (GET_GOLD(ch) < GET_OBJ_COST(i))
    {
      send_to_char(ch, "You don't have enough gold!\r\n");
      return (TRUE);
    }

    /* Just to avoid crashes, if the object has no cost, then don't
     * try to make a pile of no gold */
    if (GET_OBJ_COST(i) > 0)
    {
      /* Take gold from player */
      GET_GOLD(ch) -= GET_OBJ_COST(i);

      /* Put gold in stock-room */
      j = create_money(GET_OBJ_COST(i));
      obj_to_room(j, private_room);
    }

    /* Move item from stock-room to player's inventory */
    obj_from_room(i);
    obj_to_char(i, ch);

    /* Let everyone know what's happening */
    send_to_char(ch, "%s hands you %s, and takes your payment.\r\n",
                 CAP(GET_NAME((struct char_data *)me)), i->short_description);
    act("$n buys $p from $N.", FALSE, ch, i, (struct char_data *)me, TO_ROOM);
    send_to_char(ch, "%s thanks you for your business, 'please come again!'\r\n", shop_owner);

#ifdef PLAYER_SHOP_DEBUG
    send_to_char(ch, "player_shops: item bought and paid for\r\n");
#endif

    /* we have to save here to cement the transaction, otherwise a well timed
       crash or whatnot will duplicate the item -Zusuk */
    save_char(ch, 0);
    Crash_crashsave(ch);
    House_crashsave(house_vnum);

    return (TRUE);

    /** IDENTIFY COMMAND **/
  }
  else if (CMD_IS("identify"))
  {
    /* do we have an item?  accepts an index for the argument */
    i = find_player_shop_obj(ch, argument, private_room);

    if (i == NULL)
    {
      send_to_char(ch, "Can not find that item!  Try the index value in cases "
                       "were some objects have funky keywords.\r\n");
      return (TRUE);
    }

    do_stat_object(ch, i, ITEM_STAT_MODE_IDENTIFY_SPELL);

    return (TRUE);
  }

  /* Exit! */
  return (FALSE);
}
#ifdef PLAYER_SHOP_DEBUG
#undef PLAYER_SHOP_DEBUG
#endif
