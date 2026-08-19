/**************************************************************************
 *  File: craft/crafting_molds.c                       Part of LuminariMUD *
 *  Usage: Crafting-mold construction and vendor procedure.                *
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
#include "magic/spells.h"
#include "constants.h"
#include "act.h"
#include "crafting_molds.h"
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

#define MOLD_TYPE_WEAPON 1
#define MOLD_TYPE_ARMOR 2
#define MOLD_TYPE_ACCESSORY 3

#define MOLD_CRAFT_RING 0
#define MOLD_CRAFT_BRACER 1
#define MOLD_CRAFT_BELT 2
#define MOLD_CRAFT_BOOTS 3
#define MOLD_CRAFT_GLOVES 4
#define MOLD_CRAFT_NECKLACE 5
#define MOLD_CRAFT_CLOAK 6

#define MOLD_OBJ_VNUM 208
#define MOLD_OBJ_COST 100

void create_crafting_mold(struct char_data *ch, int selection, int type)
{
  if (!ch)
    return;

  if (GET_GOLD(ch) < MOLD_OBJ_COST)
  {
    send_to_char(ch, "You need to have %d gold on you to purchase a crafting mold.\r\n",
                 MOLD_OBJ_COST);
    return;
  }

  struct obj_data *obj = NULL;
  char buf[MEDIUM_STRING];

  obj = read_object(MOLD_OBJ_VNUM, VIRTUAL);
  if (!(obj)) // more error checking.
  {
    send_to_char(
        ch, "There seems to be a problem with the mold object vnum.  Please inform staff.\r\n");
    return;
  }

  switch (type)
  {
  case MOLD_TYPE_WEAPON:
    if (selection < 0 ||
        selection >=
            NUM_WEAPON_TYPES) // redundant, but we'll do it in case it gets called elsewhere in the future
    {
      send_to_char(ch, "That is not a valid weapon mold type\r\n");
      return;
    }
    set_weapon_object(obj, selection);
    SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_MOLD);
    snprintf(buf, sizeof(buf), "mold %s %s", a_or_an(weapon_list[selection].name),
             weapon_list[selection].name);
    obj->name = strdup(buf);
    snprintf(buf, sizeof(buf), "a crafting mold for %s %s", a_or_an(weapon_list[selection].name),
             weapon_list[selection].name);
    obj->short_description = strdup(buf);
    snprintf(buf, sizeof(buf), "A crafting mold for %s %s lies here.",
             a_or_an(weapon_list[selection].name), weapon_list[selection].name);
    obj->description = strdup(buf);
    obj_to_char(obj, ch);
    GET_GOLD(ch) -= MOLD_OBJ_COST;
    send_to_char(ch, "You purchase %s for %d gold coins.\r\n", obj->short_description,
                 MOLD_OBJ_COST);
    GET_OBJ_COST(obj) = 0;
    return;
  case MOLD_TYPE_ARMOR:
    if (selection < 0 ||
        selection >=
            NUM_SPEC_ARMOR_TYPES) // redundant, but we'll do it in case it gets called elsewhere in the future
    {
      send_to_char(ch, "That is not a valid armor mold type\r\n");
      return;
    }
    set_armor_object(obj, selection);
    SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_MOLD);
    snprintf(buf, sizeof(buf), "mold %s %s", a_or_an(armor_list[selection].name),
             armor_list[selection].name);
    obj->name = strdup(buf);
    snprintf(buf, sizeof(buf), "a crafting mold for %s %s", a_or_an(armor_list[selection].name),
             armor_list[selection].name);
    obj->short_description = strdup(buf);
    snprintf(buf, sizeof(buf), "A crafting mold for %s %s lies here.",
             a_or_an(armor_list[selection].name), armor_list[selection].name);
    obj->description = strdup(buf);
    obj_to_char(obj, ch);
    GET_GOLD(ch) -= MOLD_OBJ_COST;
    send_to_char(ch, "You purchase %s for %d gold coins.\r\n", obj->short_description,
                 MOLD_OBJ_COST);
    GET_OBJ_COST(obj) = 0;
    return;
  case MOLD_TYPE_ACCESSORY:
    switch (selection)
    {
    case MOLD_CRAFT_RING:
      SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_FINGER);
      break;
    case MOLD_CRAFT_BRACER:
      SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_WRIST);
      break;
    case MOLD_CRAFT_BELT:
      SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_WAIST);
      break;
    case MOLD_CRAFT_BOOTS:
      SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_FEET);
      break;
    case MOLD_CRAFT_GLOVES:
      SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_HANDS);
      break;
    case MOLD_CRAFT_NECKLACE:
      SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_NECK);
      break;
    case MOLD_CRAFT_CLOAK:
      SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_ABOUT);
      break;
    }

    SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_MOLD);

    snprintf(buf, sizeof(buf), "mold %s %s", a_or_an(mold_accessories[selection]),
             mold_accessories[selection]);
    obj->name = strdup(buf);
    snprintf(buf, sizeof(buf), "a crafting mold for %s %s", a_or_an(mold_accessories[selection]),
             mold_accessories[selection]);
    obj->short_description = strdup(buf);
    snprintf(buf, sizeof(buf), "A crafting mold for %s %s lies here.",
             a_or_an(mold_accessories[selection]), mold_accessories[selection]);
    obj->description = strdup(buf);
    obj_to_char(obj, ch);
    GET_GOLD(ch) -= MOLD_OBJ_COST;
    send_to_char(ch, "You purchase %s for %d gold coins.\r\n", obj->short_description,
                 MOLD_OBJ_COST);
    GET_OBJ_COST(obj) = 0;
    return;
  }
}

SPECIAL(buymolds)
{
  if (!CMD_IS("buy") && !CMD_IS("list"))
    return 0;

  int i = 0;
  skip_spaces(&argument);

  if (CMD_IS("list"))
  {
    if (!*argument)
    {
      send_to_char(ch, "Please specify either: weapons, armor or accessories.\r\n");
      return 1;
    }
    if (is_abbrev(argument, "weapons"))
    {
      for (i = 1; i < NUM_WEAPON_TYPES; i++)
      {
        send_to_char(ch, "%-25s ", weapon_list[i].name);
        if ((i % 3) == 2)
          send_to_char(ch, "\r\n");
      }
      if ((i % 3) != 2)
        send_to_char(ch, "\r\n");
      send_to_char(ch, "Type: buy (weapon name) to purchase a weapon mold.\r\n");
    }
    else if (is_abbrev(argument, "armor"))
    {
      for (i = 1; i < NUM_SPEC_ARMOR_TYPES; i++)
      {
        send_to_char(ch, "%-25s ", armor_list[i].name);
        if ((i % 3) == 2)
          send_to_char(ch, "\r\n");
      }
      if ((i % 3) != 2)
        send_to_char(ch, "\r\n");
      send_to_char(ch, "Type: buy (armor piece name) to purchase an armor mold.\r\n");
    }
    else if (is_abbrev(argument, "accessories"))
    {
      send_to_char(ch, "%-25s %-25s %-25s\r\n", "ring", "bracer", "belt");
      send_to_char(ch, "%-25s %-25s %-25s\r\n", "gloves", "necklace", "cloak");
      send_to_char(ch, "%-25s\r\n", "boots");
      send_to_char(ch, "\r\nType: buy (accessory name) to purchase an accessory mold.\r\n");
    }
    else
    {
      send_to_char(ch, "Please specify either: weapons, armor or accessories.\r\n");
    }
    return 1;
  }
  else if (CMD_IS("buy"))
  {
    int selection = -1;
    if (!*argument)
    {
      send_to_char(ch, "Please specify the full name of the weapon, armor piece or accessory mold "
                       "you wish to purchase.  Type 'list' to see options.\r\n");
      return 1;
    }
    for (i = 1; i < NUM_WEAPON_TYPES; i++)
    {
      if (!strcmp(argument, weapon_list[i].name))
      {
        selection = i;
        break;
      }
    }
    if (selection != -1)
    {
      create_crafting_mold(ch, selection, MOLD_TYPE_WEAPON);
      return 1;
    }
    for (i = 1; i < NUM_SPEC_ARMOR_TYPES; i++)
    {
      if (!strcmp(argument, armor_list[i].name))
      {
        selection = i;
        break;
      }
    }
    if (selection != -1)
    {
      create_crafting_mold(ch, selection, MOLD_TYPE_ARMOR);
      return 1;
    }
    if (!strcmp(argument, "ring"))
    {
      create_crafting_mold(ch, MOLD_CRAFT_RING, MOLD_TYPE_ACCESSORY);
      return 1;
    }
    if (!strcmp(argument, "bracer"))
    {
      create_crafting_mold(ch, MOLD_CRAFT_BRACER, MOLD_TYPE_ACCESSORY);
      return 1;
    }
    if (!strcmp(argument, "belt"))
    {
      create_crafting_mold(ch, MOLD_CRAFT_BELT, MOLD_TYPE_ACCESSORY);
      return 1;
    }
    if (!strcmp(argument, "boots"))
    {
      create_crafting_mold(ch, MOLD_CRAFT_BOOTS, MOLD_TYPE_ACCESSORY);
      return 1;
    }
    if (!strcmp(argument, "gloves"))
    {
      create_crafting_mold(ch, MOLD_CRAFT_GLOVES, MOLD_TYPE_ACCESSORY);
      return 1;
    }
    if (!strcmp(argument, "necklace"))
    {
      create_crafting_mold(ch, MOLD_CRAFT_NECKLACE, MOLD_TYPE_ACCESSORY);
      return 1;
    }
    if (!strcmp(argument, "cloak"))
    {
      create_crafting_mold(ch, MOLD_CRAFT_CLOAK, MOLD_TYPE_ACCESSORY);
      return 1;
    }
    else
    {
      send_to_char(ch, "Please specify the full name of the weapon, armor piece or accessory mold "
                       "you wish to purchase.  Type 'list' to see options.\r\n");
      return 1;
    }
  }
  return 1;
}

#undef MOLD_TYPE_WEAPON
#undef MOLD_TYPE_ARMOR
#undef MOLD_TYPE_ACCESSORY

#undef MOLD_CRAFT_RING
#undef MOLD_CRAFT_BRACER
#undef MOLD_CRAFT_BELT
#undef MOLD_CRAFT_BOOTS
#undef MOLD_CRAFT_GLOVES
#undef MOLD_CRAFT_NECKLACE
#undef MOLD_CRAFT_CLOAK

#undef MOLD_OBJ_VNUM
