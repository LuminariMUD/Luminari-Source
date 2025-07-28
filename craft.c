/*/ \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \
\
/  Luminari Crafting System, Inspired by D20mud's Craft System
/  Created By: Zusuk, original d20 code from Gicker
\
/  using craft.h as the header file currently
\
/
\ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ /*/

/*
 * Hard metal -> Mining
 * Leather -> Hunting
 * Wood -> Foresting
 * Cloth -> Knitting
 * Crystals / Essences -> Chemistry
 */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "mysql.h"
#include "utils.h"
#include "comm.h"
#include "spells.h"
#include "interpreter.h"
#include "constants.h"
#include "handler.h"
#include "db.h"
#include "craft.h"
#include "spells.h"
#include "mud_event.h"
#include "modify.h" // for parse_at()
#include "treasure.h"
#include "mudlim.h"
#include "spec_procs.h" /* For compute_ability() */
#include "item.h"
#include "quest.h"
#include "assign_wpn_armor.h"
#include "genolc.h"

extern MYSQL *conn;

/* global variables */
int mining_nodes = 0;
int farming_nodes = 0;
int hunting_nodes = 0;
int foresting_nodes = 0;

/***********************************/
/* crafting local utility functions*/
/***********************************/

/* charts for weapon resize, if weapon dice don't fall on any of these, invalid */
int weapon_damage_a[NUM_SIZES][2] = {
    /* num_dice, siz_dice */
    {
        0,
        0,
    }, /* SIZE_RESERVED */
    {
        1,
        2,
    }, // fine
    {
        1,
        3,
    }, /* diminutive */
    {
        1,
        4,
    }, /* tiny */
    {
        1,
        6,
    }, /* small */
    {
        1,
        8,
    }, /* medium */
    {
        1,
        12,
    }, /* large */
    {
        4,
        4,
    }, /* huge */
    {
        4,
        6,
    }, /* gargantuan */
    {
        6,
        6,
    }, // colossal
};
int weapon_damage_b[NUM_SIZES][2] = {
    /* num_dice, siz_dice */
    {
        0,
        0,
    }, /* SIZE_RESERVED */
    {
        1,
        1,
    }, // fine
    {
        2,
        1,
    }, /* diminutive */
    {
        2,
        3,
    }, /* tiny */
    {
        1,
        7,
    }, /* small */
    {
        2,
        4,
    }, /* medium */
    {
        2,
        6,
    }, /* large */
    {
        3,
        6,
    }, /* huge */
    {
        6,
        4,
    }, /* gargantuan */
    {
        5,
        8,
    }, // colossal
};
int weapon_damage_c[NUM_SIZES][2] = {
    /* num_dice, siz_dice */
    {
        0,
        0,
    }, /* SIZE_RESERVED */
    {
        0,
        0,
    }, /* invalid (fine) */
    {
        3,
        1,
    }, // diminiutive
    {
        2,
        2,
    }, /* tiny */
    {
        3,
        2,
    }, /* small */
    {
        1,
        9,
    }, /* medium */
    {
        1,
        10,
    }, /* large */
    {
        2,
        8,
    }, /* huge */
    {
        3,
        8,
    }, /* gargantuan */
    {
        4,
        8,
    }, // colossal
};

/* the primary use of this function is to modify a weapons damage on resize
 *   weapon:  object, needs to be a weapon
 * we have 3 charts above trying to accomodate most weapons you could
 *   possibly ecnounter
 * returns TRUE if successful, FALSE if failed */
bool scale_damage(struct char_data *ch, struct obj_data *weapon, int new_size)
{
  int num_of_dice = 0;       // number-of-dice rolled for weapon dam
  int size_of_dice = 0;      // size-of-dice rolled for weapon dam
  int size = SIZE_UNDEFINED; // old size of weapon
  int counter = 0;
  int size_shift = SIZE_UNDEFINED;

  /* wha?! no object? */
  if (!weapon)
  {
    send_to_char(ch, "You do not seem to have an object for resizing!\r\n");
    return FALSE;
  }

  /* this only works for weapons */
  if (GET_OBJ_TYPE(weapon) != ITEM_WEAPON)
  {
    send_to_char(ch, "You do not seem to have a weapon for resizing!\r\n");
    return FALSE;
  }

  /* assigned for ease-of-use */
  num_of_dice = GET_OBJ_VAL(weapon, 1);  /* how many dice are we rolling on old size */
  size_of_dice = GET_OBJ_VAL(weapon, 2); /* how big is the current dice roll on old size */
  size = GET_OBJ_SIZE(weapon);           /* what is current size of weapon? */
  size_shift = new_size - size;          /* how many size classes to shift in charts */

  /* first check to make sure we have this value on one of the charts, if
     not we are calling it invalid */
  for (counter = 0; counter < NUM_SIZES; counter++)
  {

    /* check our charts - chart A */
    if (weapon_damage_a[counter][0] == num_of_dice &&
        weapon_damage_a[counter][1] == size_of_dice)
    {
      /* valid shift in chart?  calculate our new location on chart */
      if (counter + size_shift >= NUM_SIZES ||
          counter + size_shift <= SIZE_RESERVED)
      {
        send_to_char(ch, "Invalid resize!\r\n");
        return FALSE;
      }
      /* valid!  set and exit clean */
      GET_OBJ_VAL(weapon, 1) = weapon_damage_a[counter + size_shift][0];
      GET_OBJ_VAL(weapon, 2) = weapon_damage_a[counter + size_shift][1];
      GET_OBJ_SIZE(weapon) = new_size;
      return TRUE;
    }

    /* check our charts - chart B */
    if (weapon_damage_b[counter][0] == num_of_dice &&
        weapon_damage_b[counter][1] == size_of_dice)
    {
      /* valid shift in chart?  calculate our new location on chart */
      if (counter + size_shift >= NUM_SIZES ||
          counter + size_shift <= SIZE_RESERVED)
      {
        send_to_char(ch, "Invalid resize!\r\n");
        return FALSE;
      }
      /* valid!  set and exit clean */
      GET_OBJ_VAL(weapon, 1) = weapon_damage_b[counter + size_shift][0];
      GET_OBJ_VAL(weapon, 2) = weapon_damage_b[counter + size_shift][1];
      GET_OBJ_SIZE(weapon) = new_size;
      return TRUE;
    }

    /* check our charts - chart C */
    if (weapon_damage_c[counter][0] == num_of_dice &&
        weapon_damage_c[counter][1] == size_of_dice)
    {
      /* valid shift in chart?  calculate our new location on chart */
      if (counter + size_shift >= NUM_SIZES ||
          counter + size_shift <= SIZE_FINE)
      { /* no 'fine' value for this weapon */
        send_to_char(ch, "Invalid resize!\r\n");
        return FALSE;
      }
      /* valid!  set and exit clean */
      GET_OBJ_VAL(weapon, 1) = weapon_damage_c[counter + size_shift][0];
      GET_OBJ_VAL(weapon, 2) = weapon_damage_c[counter + size_shift][1];
      GET_OBJ_SIZE(weapon) = new_size;
      save_char(ch, 0);
      Crash_crashsave(ch);
      return TRUE;
    }
  }

  /* couldn't find anything on the charts! */
  send_to_char(ch, "Could not find your weapon on any of the charts!  You "
                   "should turn this weapon in to a builder-staff member to adjust.\r\n");
  return FALSE;
}

/* this function will switch the material of an item based on the
   conversion crafting system
 */
int convert_material(int material)
{
  switch (material)
  {
  case MATERIAL_IRON:
    return MATERIAL_COLD_IRON;
  case MATERIAL_SILVER:
    return MATERIAL_ALCHEMAL_SILVER;
  default:
    return material;
  }

  return material;
}

/* simple function to reset craft data */
void reset_craft(struct char_data *ch)
{
  /* initialize values */
  GET_CRAFTING_TYPE(ch) = 0; // SCMD_ of craft
  GET_CRAFTING_TICKS(ch) = 0;
  GET_CRAFTING_OBJ(ch) = NULL;
  GET_CRAFTING_REPEAT(ch) = 0;
}

/* simple function to reset auto craft data */
void reset_acraft(struct char_data *ch)
{

  /* initialize values */
  GET_AUTOCQUEST_VNUM(ch) = 0;
  GET_AUTOCQUEST_MAKENUM(ch) = 0;
  GET_AUTOCQUEST_QP(ch) = 0;
  GET_AUTOCQUEST_EXP(ch) = 0;
  GET_AUTOCQUEST_GOLD(ch) = 0;
  GET_AUTOCQUEST_MATERIAL(ch) = 0;

  if (GET_AUTOCQUEST_DESC(ch))
    free(GET_AUTOCQUEST_DESC(ch));
  GET_AUTOCQUEST_DESC(ch) = strdup("nothing");
}

/* compartmentalized auto-quest crafting reporting since its done
   a few times in the code */
void cquest_report(struct char_data *ch)
{
  if (GET_AUTOCQUEST_VNUM(ch))
  {
    if (GET_AUTOCQUEST_MAKENUM(ch) <= 0)
      send_to_char(ch, "You have completed your supply order for %s.\r\n",
                   GET_AUTOCQUEST_DESC(ch));
    else
      send_to_char(ch, "You have not yet completed your supply order "
                       "for %s.\r\n"
                       "You still need to make %d more.\r\n",
                   GET_AUTOCQUEST_DESC(ch), GET_AUTOCQUEST_MAKENUM(ch));
    send_to_char(ch, "Once completed/turned-in you will receive the"
                     " following:\r\n"
                     "You will receive %d reputation points.\r\n"
                     "%d gold will be awarded to you.\r\n"
                     "You will receive %d experience points.\r\n"
                     "(type 'supplyorder complete' at the supply office)\r\n",
                 GET_AUTOCQUEST_QP(ch), GET_AUTOCQUEST_GOLD(ch),
                 GET_AUTOCQUEST_EXP(ch));
  }
  else
    send_to_char(ch, "Type 'supplyorder new' for a new supply order, "
                     "'supplyorder complete' to finish your supply "
                     "order and receive your reward or 'supplyorder quit' "
                     "to quit your current supply order.\r\n");
}

/*
 * Our current list of materials distributed in this manner:
 METALS (hard)
 * bronze
 * iron
 * steel
 * cold iron
 * alchemal silver
 * mithril
 * adamantine
 METALS (precious)
 * copper
 * brass
 * silver
 * gold
 * platinum
 LEATHERS
 * leather
 * dragonhide
 WOODS
 * wood
 * darkwood
 CLOTH
 * burlap
 * hemp
 * cotton
 * wool
 * velvet
 * satin
 * silk
 */

/* this function returns an appropriate keyword(s) based on material */
char *node_keywords(int material)
{

  /* reference */
  /* steel      - vein of dull ore */
  /* cold iron  - vein of ore */
  /* mithril    - vein of bright ore */
  /* adamantine - vein of sparkling ore */
  /* silver     - vein of dull speckled ore */
  /* gold       - vein of yellowish ore */

  switch (material)
  {
  case MATERIAL_STEEL:
    return strdup("vein of dull ore");
  case MATERIAL_COLD_IRON:
    return strdup("vein of ore");
  case MATERIAL_MITHRIL:
    return strdup("vein of bright ore");
  case MATERIAL_ADAMANTINE:
    return strdup("vein of sparkling ore");
  case MATERIAL_SILVER:
    return strdup("vein dull speckled ore");
  case MATERIAL_GOLD:
    return strdup("vein of yellowish ore");
  case MATERIAL_WOOD:
    return strdup("wood harvest tree");
  case MATERIAL_DARKWOOD:
    return strdup("wood harvest tree quality");
  case MATERIAL_LEATHER:
    return strdup("game live area");
  case MATERIAL_DRAGONHIDE:
    return strdup("game live area exotic");
  case MATERIAL_HEMP:
    return strdup("cloth raw material basic");
  case MATERIAL_COTTON:
    return strdup("cloth raw material simple");
  case MATERIAL_WOOL:
    return strdup("cloth raw material");
  case MATERIAL_VELVET:
    return strdup("cloth raw material quality");
  case MATERIAL_SATIN:
    return strdup("cloth raw material high quality");
  case MATERIAL_SILK:
    return strdup("cloth raw material rich");
  }
  return strdup("node harvesting");
}

/* this function returns an appropriate short-desc based on material */
char *node_sdesc(int material)
{
  /* reference */
  /* steel      - vein of dull ore */
  /* cold iron  - vein of ore */
  /* mithril    - vein of bright ore */
  /* adamantine - vein of sparkling ore */
  /* silver     - vein of dull speckled ore */
  /* gold       - vein of yellowish ore */
  switch (material)
  {
  case MATERIAL_STEEL:
    return strdup("a vein of dull ore");
  case MATERIAL_COLD_IRON:
    return strdup("a vein of ore");
  case MATERIAL_MITHRIL:
    return strdup("a vein of bright ore");
  case MATERIAL_ADAMANTINE:
    return strdup("a vein of sparkling ore");
  case MATERIAL_SILVER:
    return strdup("a vein of dull speckled ore");
  case MATERIAL_GOLD:
    return strdup("a vein of yellowish ore");
  case MATERIAL_WOOD:
    return strdup("a wood harvest");
  case MATERIAL_DARKWOOD:
    return strdup("a quality wood harvest");
  case MATERIAL_LEATHER:
    return strdup("an area of live game");
  case MATERIAL_DRAGONHIDE:
    return strdup("an area of live exotic game");
  case MATERIAL_HEMP:
    return strdup("raw material for basic cloth");
  case MATERIAL_COTTON:
    return strdup("raw material for simple cloth");
  case MATERIAL_WOOL:
    return strdup("raw material for cloth");
  case MATERIAL_VELVET:
    return strdup("raw material for quality cloth");
  case MATERIAL_SATIN:
    return strdup("raw material for high quality");
  case MATERIAL_SILK:
    return strdup("raw material for rich cloth");
  }
  return strdup("a harvesting node");
}

/* this function returns an appropriate desc based on material */
char *node_desc(int material)
{
  /* reference */
  /* steel      - vein of dull ore */
  /* cold iron  - vein of ore */
  /* mithril    - vein of bright ore */
  /* adamantine - vein of sparkling ore */
  /* silver     - vein of dull speckled ore */
  /* gold       - vein of yellowish ore */
  switch (material)
  {
  case MATERIAL_STEEL:
    return strdup("There are veins of dull ore here. \tn(\tYharvest\tn)");
  case MATERIAL_COLD_IRON:
    return strdup("There are veins of ore here. \tn(\tYharvest\tn)");
  case MATERIAL_MITHRIL:
    return strdup("There are veins of bright ore here. \tn(\tYharvest\tn)");
  case MATERIAL_ADAMANTINE:
    return strdup("There are veins of sparkling ore here. \tn(\tYharvest\tn)");
  case MATERIAL_SILVER:
    return strdup("There are veins of dull speckled ore here. \tn(\tYharvest\tn)");
  case MATERIAL_GOLD:
    return strdup("There are veins of yellowish ore here. \tn(\tYharvest\tn)");
  case MATERIAL_WOOD:
    return strdup("The area here looks ideal for harvesting wood. \tn(\tYharvest\tn)");
  case MATERIAL_DARKWOOD:
    return strdup("The area here looks ideal for harvesting quality wood. \tn(\tYharvest\tn)");
  case MATERIAL_LEATHER:
    return strdup("The area is live with game. \tn(\tYharvest\tn)");
  case MATERIAL_DRAGONHIDE:
    return strdup("The area is live with exotic game. \tn(\tYharvest\tn)");
  case MATERIAL_HEMP:
    return strdup("There is enough raw material for basic cloth here. \tn(\tYharvest\tn)");
  case MATERIAL_COTTON:
    return strdup("There is enough raw material for simple cloth here. \tn(\tYharvest\tn)");
  case MATERIAL_WOOL:
    return strdup("There is enough raw material for cloth here. \tn(\tYharvest\tn)");
  case MATERIAL_VELVET:
    return strdup("There is enough raw material for quality cloth here. \tn(\tYharvest\tn)");
  case MATERIAL_SATIN:
    return strdup("There is enough raw material for high quality cloth here. \tn(\tYharvest\tn)");
  case MATERIAL_SILK:
    return strdup("There is enough raw material for rich cloth here. \tn(\tYharvest\tn)");
  }
  return strdup("A harvesting node is here.  Please inform an imm, this is an error.");
}

/* a function to try and make an intelligent(?) decision
   about what material a harvesting node should be */
int random_node_material(int allowed)
{
  int rand = 0;

  if (mining_nodes >= (allowed * 2) && foresting_nodes >= allowed &&
      farming_nodes >= allowed && hunting_nodes >= allowed)
    return MATERIAL_STEEL;

  rand = rand_number(1, 100);
  /* 34% mining, blacksmithing or goldsmithing */
  if (rand <= 34)
  {

    // mining
    if (mining_nodes >= (allowed * 2))
      return random_node_material(allowed);

    rand = rand_number(1, 100);
    /* 80% chance of blacksmithing (iron/steel/cold-iron/mithril/adamantine */
    if (rand <= 80)
    {

      rand = rand_number(1, 100);
      // blacksmithing

      if (rand <= 85)
        return MATERIAL_STEEL;
      else if (rand <= 93)
        return MATERIAL_COLD_IRON;
      else if (rand <= 98)
        return MATERIAL_MITHRIL;
      else
        return MATERIAL_ADAMANTINE;

      /* 20% of goldsmithing (silver/gold) */
    }
    else
    {

      // goldsmithing

      if (rand_number(1, 100) <= 90)
        return MATERIAL_SILVER;
      else
        return MATERIAL_GOLD;
    }

    /* 33% farming (hemp/cotton/wool/velvet/satin/silk) */
  }
  else if (rand <= 67)
  {

    rand = rand_number(1, 100);
    // farming

    if (farming_nodes >= allowed)
      return random_node_material(allowed);

    if (rand <= 30)
      return MATERIAL_HEMP;
    else if (rand <= 70)
      return MATERIAL_COTTON;
    else if (rand <= 85)
      return MATERIAL_WOOL;
    else if (rand <= 96)
      return MATERIAL_VELVET;
    else if (rand <= 98)
      return MATERIAL_SATIN;
    else
      return MATERIAL_SILK;

    /* 33% foresting (leather/dragonhide/wood/darkwood) */
  }
  else
  {
    // foresting

    if (foresting_nodes >= allowed)
      return random_node_material(allowed);

    rand = dice(1, 100);

    if (rand <= 50)
    {

      rand = dice(1, 100);
      if (rand <= 99)
        return MATERIAL_LEATHER;
      else
        return MATERIAL_DRAGONHIDE;
    }
    else
    {

      rand = dice(1, 100);
      if (rand <= 99)
        return MATERIAL_WOOD;
      else
        return MATERIAL_DARKWOOD;
    }
  }

  /* default steel */
  return MATERIAL_STEEL;
}

/* this is called in db.c on boot-up
   harvesting nodes are placed by this function randomly(?)
   throughout the world
 */
void reset_harvesting_rooms(void)
{
  int cnt = 0;
  int num_rooms = 0;
  int nodes_allowed = 0;
  struct obj_data *obj = NULL;

  for (cnt = 0; cnt <= top_of_world; cnt++)
  {
    if (!VALID_ROOM_RNUM(cnt))
      continue;
    if (ROOM_FLAGGED(cnt, ROOM_HOUSE))
      continue;
    if (ROOM_FLAGGED(cnt, ROOM_FLY_NEEDED))
      continue;
    if (ROOM_FLAGGED(cnt, ROOM_CLIMB_NEEDED))
      continue;
    if (world[cnt].sector_type == SECT_CITY)
      continue;
    if (world[cnt].sector_type == SECT_INSIDE)
      continue;
    if (world[cnt].sector_type == SECT_INSIDE_ROOM)
      continue;
    if (world[cnt].sector_type == SECT_WATER_NOSWIM)
      continue;
    if (world[cnt].sector_type == SECT_OUTTER_PLANES)
      continue;
    if (world[cnt].sector_type == SECT_UD_CITY)
      continue;
    if (world[cnt].sector_type == SECT_UD_INSIDE)
      continue;
    if (world[cnt].sector_type == SECT_UD_WATER_NOSWIM)
      continue;

    num_rooms++;
  }

  nodes_allowed = num_rooms / NODE_CAP_FACTOR;

  if (mining_nodes >= (nodes_allowed * 2) && foresting_nodes >= nodes_allowed &&
      farming_nodes >= nodes_allowed && hunting_nodes >= nodes_allowed)
    return;

  for (cnt = 0; cnt < top_of_world; cnt++)
  {
    if (!VALID_ROOM_RNUM(cnt))
      continue;
    if (ROOM_FLAGGED(cnt, ROOM_HOUSE))
      continue;
    if (ROOM_FLAGGED(cnt, ROOM_FLY_NEEDED))
      continue;
    if (ROOM_FLAGGED(cnt, ROOM_CLIMB_NEEDED))
      continue;
    if (world[cnt].sector_type == SECT_CITY)
      continue;
    if (world[cnt].sector_type == SECT_INSIDE)
      continue;
    if (world[cnt].sector_type == SECT_INSIDE_ROOM)
      continue;
    if (world[cnt].sector_type == SECT_WATER_NOSWIM)
      continue;
    if (world[cnt].sector_type == SECT_OUTTER_PLANES)
      continue;
    if (world[cnt].sector_type == SECT_UD_CITY)
      continue;
    if (world[cnt].sector_type == SECT_UD_INSIDE)
      continue;
    if (world[cnt].sector_type == SECT_UD_WATER_NOSWIM)
      continue;
    if (dice(1, 33) == 1)
    {
      obj = read_object(HARVESTING_NODE, VIRTUAL);
      if (!obj)
        continue;
      GET_OBJ_MATERIAL(obj) = random_node_material(nodes_allowed);
      switch (GET_OBJ_MATERIAL(obj))
      {
      case MATERIAL_STEEL:
      case MATERIAL_COLD_IRON:
      case MATERIAL_MITHRIL:
      case MATERIAL_ADAMANTINE:
      case MATERIAL_SILVER:
      case MATERIAL_GOLD:
        if (mining_nodes >= nodes_allowed)
        {
          obj_to_room(obj, cnt);
          extract_obj(obj);
          continue;
        }
        else
          mining_nodes++;
        break;
      case MATERIAL_WOOD:
      case MATERIAL_DARKWOOD:
      case MATERIAL_LEATHER:
      case MATERIAL_DRAGONHIDE:
        if (foresting_nodes >= nodes_allowed)
        {
          obj_to_room(obj, cnt);
          extract_obj(obj);
          continue;
        }
        else
          foresting_nodes++;
      case MATERIAL_HEMP:
      case MATERIAL_COTTON:
      case MATERIAL_WOOL:
      case MATERIAL_VELVET:
      case MATERIAL_SATIN:
      case MATERIAL_SILK:
        if (farming_nodes >= nodes_allowed)
        {
          obj_to_room(obj, cnt);
          extract_obj(obj);
          continue;
        }
        else
          farming_nodes++;
        break;
      default:
        obj_to_room(obj, cnt);
        extract_obj(obj);
        continue;
        break;
      }
      GET_OBJ_VAL(obj, 0) = dice(2, 3);

      /* strdup()ed in node_foo() functions */
      if (obj->name) free(obj->name);
      obj->name = node_keywords(GET_OBJ_MATERIAL(obj));
      if (obj->short_description) free(obj->short_description);
      obj->short_description = node_sdesc(GET_OBJ_MATERIAL(obj));
      if (obj->description) free(obj->description);
      obj->description = node_desc(GET_OBJ_MATERIAL(obj));
      obj_to_room(obj, cnt);
    }
  }
}

/*************************/
/* start primary engines */
/*************************/

// combine essence to make them stronger

int augment(struct obj_data *kit, struct char_data *ch)
{
  struct obj_data *obj = NULL, *essence_one = NULL, *essence_two = NULL;
  int num_objs = 0, cost = 0, level_diff = 0, success_chance = 0;
  int dice_roll = 0, essence_level = 0;
  int skill_type = SKILL_CHEMISTRY; // change this to change the skill used
  int fast_craft_bonus = GET_SKILL(ch, SKILL_FAST_CRAFTER) / 33;

  // Cycle through contents and categorize
  for (obj = kit->contains; obj != NULL; obj = obj->next_content)
  {
    if (obj)
    {
      num_objs++;
      if (num_objs > 2)
      {
        send_to_char(ch, "Make sure only two items are in the kit.\r\n");
        return 1;
      }
      if (GET_OBJ_TYPE(obj) == ITEM_ESSENCE && !essence_one)
      {
        essence_one = obj;
      }
      else if (GET_OBJ_TYPE(obj) == ITEM_ESSENCE && !essence_two)
      {
        essence_two = obj;
      }
    }
  }

  if (num_objs > 2)
  {
    send_to_char(ch, "Make sure only two items are in the kit.\r\n");
    return 1;
  }
  if (!essence_one || !essence_two)
  {
    send_to_char(ch, "You need two essences to augment.\r\n");
    return 1;
  }
  if (GET_OBJ_LEVEL(essence_one) >= (LVL_IMMORT - 1) ||
      GET_OBJ_LEVEL(essence_two) >= (LVL_IMMORT - 1))
  {
    send_to_char(ch, "You can not further augment that essence!\r\n");
    return 1;
  }
  level_diff = abs(GET_OBJ_LEVEL(essence_one) - GET_OBJ_LEVEL(essence_two));
  /* essence have to be 4 level range of each other */
  if (level_diff > 4)
  {
    send_to_char(ch, "The essence have to be closer in power (level) to each other!\r\n");
    return 1;
  }

  /* what is the level we are adjusting? */
  if (GET_OBJ_LEVEL(essence_one) >= GET_OBJ_LEVEL(essence_two))
    essence_level = GET_OBJ_LEVEL(essence_one);
  else
    essence_level = GET_OBJ_LEVEL(essence_two);

  /* high enough skill? */
  if (essence_level > (GET_SKILL(ch, skill_type) / 3))
  {
    send_to_char(ch, "The essence level is %d but your %s skill is "
                     "only capable of creating level %d crystals.\r\n",
                 essence_level, skill_name(skill_type),
                 (GET_SKILL(ch, skill_type) / 3));
    return 1;
  }

  cost = essence_level * 500 / 3; // expense for augmenting
  if (GET_GOLD(ch) < cost)
  {
    send_to_char(ch, "You need %d coins on hand for supplies to augment this "
                     "crystal.\r\n",
                 cost);
    return 1;
  }

  /* roll the dice! */
  dice_roll = dice(1, 100);

  /* success is level difference divided by 4,  percent */
  success_chance = 100 - (level_diff * 10);
  success_chance -= essence_level; /* minus level */

  /* critical success */
  if (dice_roll >= 95)
    success_chance = 150;

  dice_roll += GET_SKILL(ch, skill_type) / 3;

  /* failed our attempt */
  if (dice_roll > success_chance)
  {
    send_to_char(ch, "There seems to be a flaw in your augmentation...\r\n");
  }
  /* success! */
  else
  {
    essence_level++;
    GET_OBJ_LEVEL(essence_one) = essence_level;
  }

  /* exp bonus for crafting ticks */
  GET_CRAFTING_BONUS(ch) = 10 + MIN(30, essence_level);
  /* cost */
  send_to_char(ch, "It cost you %d coins in supplies to augment this "
                   "essence.\r\n",
               cost);
  GET_GOLD(ch) -= cost;

  GET_CRAFTING_TYPE(ch) = SCMD_AUGMENT;
  GET_CRAFTING_TICKS(ch) = 10 - fast_craft_bonus;
  GET_CRAFTING_OBJ(ch) = essence_one;
  send_to_char(ch, "You begin to augment %s.\r\n",
               essence_one->short_description);
  act("$n begins to augment $p.", FALSE, ch, essence_one, 0, TO_ROOM);

  /* get rid of the items in the kit */
  obj_from_obj(essence_one);
  extract_obj(essence_two);
  obj_to_char(essence_one, ch);
  save_char(ch, 0);
  Crash_crashsave(ch);

  NEW_EVENT(eCRAFTING, ch, NULL, 1 * PASSES_PER_SEC);

  if (!IS_NPC(ch))
    increase_skill(ch, skill_type);

  return 1;
}

// convert one material into another
// requires multiples of exactly 10 of same mat to do the converstion

/*  !! still under construction - zusuk !! */
int convert(struct obj_data *kit, struct char_data *ch)
{
  int cost = 500; /* flat cost */
  int num_mats = 0, material = -1, obj_vnum = 0;
  struct obj_data *new_mat = NULL, *obj = NULL;
  int fast_craft_bonus = GET_SKILL(ch, SKILL_FAST_CRAFTER) / 33;

  /* Cycle through contents and categorize */
  for (obj = kit->contains; obj != NULL; obj = obj->next_content)
  {
    if (obj)
    {
      if (GET_OBJ_TYPE(obj) != ITEM_MATERIAL)
      {
        send_to_char(ch, "Only materials should be inside the kit in"
                         " order to convert.\r\n");
        return 1;
      }
      else if (GET_OBJ_TYPE(obj) == ITEM_MATERIAL)
      {
        if (GET_OBJ_VAL(obj, 0) >= 2)
        {
          send_to_char(ch, "%s is a bundled item, which must first be unbundled before you can use it to craft.\r\n", obj->short_description);
          return 1;
        }
        if (material == -1)
        { /* first item */
          new_mat = obj;
          material = GET_OBJ_MATERIAL(obj);
        }
        else if (GET_OBJ_MATERIAL(obj) != material)
        {
          send_to_char(ch, "You have mixed materials inside the kit, "
                           "put only the exact same materials for "
                           "conversion.\r\n");
          return 1;
        }
        num_mats++; /* we found matching material */
        obj_vnum = GET_OBJ_VNUM(obj);
      }
    }
  }

  if (num_mats)
  {
    if (num_mats % 10)
    {
      send_to_char(ch, "You must convert materials in multiple "
                       "of 10 units exactly.\r\n");
      return 1;
    }
  }
  else
  {
    send_to_char(ch, "There is no material in the kit.\r\n");
    return 1;
  }

  if ((num_mats = convert_material(material)))
    send_to_char(ch, "You are converting the material to:  %s\r\n",
                 material_name[num_mats]);
  else
  {
    send_to_char(ch, "You do not have a valid material in the crafting "
                     "kit.\r\n");
    return 1;
  }

  if (GET_GOLD(ch) < cost)
  {
    send_to_char(ch, "You need %d gold on hand for supplies to covert these "
                     "materials.\r\n",
                 cost);
    return 1;
  }
  send_to_char(ch, "It cost you %d gold in supplies to convert this "
                   "item.\r\n",
               cost);
  GET_GOLD(ch) -= cost;
  // new name
  char buf[MAX_INPUT_LENGTH] = {'\0'};
  snprintf(buf, sizeof(buf), "\tca portion of %s material\tn",
           material_name[num_mats]);
  new_mat->name = strdup(buf);
  new_mat->short_description = strdup(buf);
  snprintf(buf, sizeof(buf), "\tcA portion of %s material lies here.\tn",
           material_name[num_mats]);
  new_mat->description = strdup(buf);
  act("$n begins a conversion of materials into $p.", FALSE, ch,
      new_mat, 0, TO_ROOM);

  GET_CRAFTING_BONUS(ch) = 10 + MIN(60, GET_OBJ_LEVEL(new_mat));
  GET_CRAFTING_TYPE(ch) = SCMD_CONVERT;
  GET_CRAFTING_TICKS(ch) = 5 - fast_craft_bonus;
  GET_CRAFTING_OBJ(ch) = new_mat;
  GET_CRAFTING_REPEAT(ch) = MAX(0, (num_mats / 10) + 1);

  obj_from_obj(new_mat);

  obj_vnum = GET_OBJ_VNUM(kit);
  obj_from_char(kit);
  extract_obj(kit);
  kit = read_object(obj_vnum, VIRTUAL);

  obj_to_char(kit, ch);

  obj_to_char(new_mat, ch);

  save_char(ch, 0);
  Crash_crashsave(ch);
  NEW_EVENT(eCRAFTING, ch, NULL, 1 * PASSES_PER_SEC);

  return 1;
}

/* rename an object */
int restring(char *argument, struct obj_data *kit, struct char_data *ch)
{
  int num_objs = 0, cost;
  struct obj_data *obj = NULL;
  char buf[MAX_INPUT_LENGTH] = {'\0'};
  int fast_craft_bonus = GET_SKILL(ch, SKILL_FAST_CRAFTER) / 33;

  /* Cycle through contents */
  /* restring requires just one item be inside the kit */
  for (obj = kit->contains; obj != NULL; obj = obj->next_content)
  {
    num_objs++;
    break;
  }

  if (num_objs > 1)
  {
    send_to_char(ch, "Only one item should be inside the kit.\r\n");
    return 1;
  }

  if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER ||
      GET_OBJ_TYPE(obj) == ITEM_AMMO_POUCH)
  {
    if (obj->contains)
    {
      send_to_char(ch, "You cannot restring bags that have items in them.\r\n");
      return 1;
    }
  }

  if (GET_OBJ_TYPE(obj) == ITEM_SPELLBOOK)
  {
    send_to_char(ch, "You cannot restring spellbooks.\r\n");
    return 1;
  }

  if (GET_OBJ_MATERIAL(obj))
  {
    if (!strstr(argument, material_name[GET_OBJ_MATERIAL(obj)]))
    {
      send_to_char(ch, "You must include the material name, '%s', in the object "
                       "description somewhere.\r\n",
                   material_name[GET_OBJ_MATERIAL(obj)]);
      return 1;
    }
  }

  /* Thazull wanted very cheap at low level for RP fun */
  switch (GET_OBJ_LEVEL(obj))
  {
  case 0:
  case 1:
  case 2:
  case 3:
  case 4:
  case 5:
  case 6:
    cost = 10;
    break;
  case 7:
  case 8:
  case 9:
  case 10:
  case 11:
  case 12:
    cost = 20 + GET_OBJ_LEVEL(obj);
    break;
  case 13:
  case 14:
  case 15:
  case 16:
    cost = 40 + GET_OBJ_LEVEL(obj) + GET_OBJ_COST(obj) / 6;
    break;
  case 17:
  case 18:
  case 19:
  case 20:
    cost = 150 + GET_OBJ_LEVEL(obj) + GET_OBJ_COST(obj) / 5;
    break;
  case 21:
  case 22:
  case 23:
  case 24:
  case 25:
    cost = 500 + GET_OBJ_LEVEL(obj) + GET_OBJ_COST(obj) / 4;
    break;
  default:
    cost = 2000 + GET_OBJ_LEVEL(obj) + GET_OBJ_COST(obj) / 2;
    break;
  }

  if (GET_GOLD(ch) < cost)
  {
    send_to_char(ch, "You need %d gold on hand for supplies to restring"
                     " this item.\r\n",
                 cost);
    return 1;
  }

  /* you need to parse the @ sign */
  parse_at(argument);

  /* success!! */
  if (obj->name) free(obj->name);
  obj->name = strdup(argument);
  strip_colors(obj->name);
  if (obj->short_description) free(obj->short_description);
  obj->short_description = strdup(argument);
  snprintf(buf, sizeof(buf), "%s lies here.", CAP(argument));
  if (obj->description) free(obj->description);
  if (obj->description) free(obj->description);
  obj->description = strdup(buf);
  if (obj->ex_description)
  {
    free_ex_descriptions(obj->ex_description);
    struct extra_descr_data *new_descr;
    CREATE(new_descr, struct extra_descr_data, 1);
    new_descr->keyword = strdup(argument);
    new_descr->description = strdup("You don't notice any extra details.\n");
    obj->ex_description = new_descr;
  }
  GET_CRAFTING_TYPE(ch) = SCMD_RESTRING;
  GET_CRAFTING_TICKS(ch) = 5 - fast_craft_bonus;
  GET_CRAFTING_OBJ(ch) = obj;

  send_to_char(ch, "It cost you %d gold in supplies to create this item.\r\n",
               cost);
  GET_GOLD(ch) -= cost;
  send_to_char(ch, "You put the item into the crafting kit and wait for it "
                   "to transform into %s.\r\n",
               obj->short_description);

  obj_from_obj(obj);

  obj_to_char(obj, ch);
  save_char(ch, 0);
  Crash_crashsave(ch);
  NEW_EVENT(eCRAFTING, ch, NULL, 1 * PASSES_PER_SEC);

  return 1;
}

/* change extra description of an object */
int redesc(char *argument, struct obj_data *kit, struct char_data *ch)
{
  int num_objs = 0, cost;
  struct obj_data *obj = NULL;
  char buf[MAX_INPUT_LENGTH] = {'\0'};
  int fast_craft_bonus = GET_SKILL(ch, SKILL_FAST_CRAFTER) / 33;

  /* Cycle through contents */
  /* redesc requires just one item be inside the kit */
  for (obj = kit->contains; obj != NULL; obj = obj->next_content)
  {
    num_objs++;
    break;
  }

  if (num_objs > 1)
  {
    send_to_char(ch, "Only one item should be inside the kit.\r\n");
    return 1;
  }

  if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER ||
      GET_OBJ_TYPE(obj) == ITEM_AMMO_POUCH)
  {
    if (obj->contains)
    {
      send_to_char(ch, "You cannot redesc bags that have items in them.\r\n");
      return 1;
    }
  }

  if (GET_OBJ_TYPE(obj) == ITEM_SPELLBOOK)
  {
    send_to_char(ch, "You cannot redesc spellbooks.\r\n");
    return 1;
  }

  /* Thazull wanted very cheap at low level for RP fun */
  switch (GET_OBJ_LEVEL(obj))
  {
  case 0:
  case 1:
  case 2:
  case 3:
  case 4:
  case 5:
  case 6:
    cost = 10;
    break;
  case 7:
  case 8:
  case 9:
  case 10:
  case 11:
  case 12:
    cost = 20 + GET_OBJ_LEVEL(obj);
    break;
  case 13:
  case 14:
  case 15:
  case 16:
    cost = 40 + GET_OBJ_LEVEL(obj) + GET_OBJ_COST(obj) / 6;
    break;
  case 17:
  case 18:
  case 19:
  case 20:
    cost = 150 + GET_OBJ_LEVEL(obj) + GET_OBJ_COST(obj) / 5;
    break;
  case 21:
  case 22:
  case 23:
  case 24:
  case 25:
    cost = 500 + GET_OBJ_LEVEL(obj) + GET_OBJ_COST(obj) / 4;
    break;
  default:
    cost = 2000 + GET_OBJ_LEVEL(obj) + GET_OBJ_COST(obj) / 2;
    break;
  }

  if (GET_GOLD(ch) < cost)
  {
    send_to_char(ch, "You need %d gold on hand for supplies to redesc this item.\r\n", cost);
    return 1;
  }

  /* you need to parse the @ sign */
  parse_at(argument);

  /* success!! */
  if (obj->ex_description)
  {
    free_ex_descriptions(obj->ex_description);
  }

  struct extra_descr_data *new_descr;
  CREATE(new_descr, struct extra_descr_data, 1);
  new_descr->keyword = strdup(obj->name);
  snprintf(buf, sizeof(buf), "%s\n", strfrmt(argument, 80, 1, FALSE, FALSE, FALSE));
  new_descr->description = strdup(buf);
  obj->ex_description = new_descr;

  GET_CRAFTING_TYPE(ch) = SCMD_REDESC;
  GET_CRAFTING_TICKS(ch) = 5 - fast_craft_bonus;
  GET_CRAFTING_OBJ(ch) = obj;

  send_to_char(ch, "It cost you %d gold in supplies to create this item.\r\n", cost);
  GET_GOLD(ch) -= cost;
  send_to_char(ch, "You put the item into the crafting kit and wait for it to transform into %s.\r\n", obj->short_description);

  obj_from_obj(obj);

  obj_to_char(obj, ch);
  save_char(ch, 0);
  Crash_crashsave(ch);
  NEW_EVENT(eCRAFTING, ch, NULL, 1 * PASSES_PER_SEC);

  return 1;
}

/* autocraft - crafting quest command */
int autocraft(struct obj_data *kit, struct char_data *ch)
{
  int material, obj_vnum, num_mats = 0, material_level = 1;
  struct obj_data *obj = NULL;
  int fast_craft_bonus = GET_SKILL(ch, SKILL_FAST_CRAFTER) / 33;

  if (!GET_AUTOCQUEST_MATERIAL(ch))
  {
    send_to_char(ch, "You do not have a supply order active right now. "
                     "(supplyorder new)\r\n");
    return 1;
  }
  if (!GET_AUTOCQUEST_MAKENUM(ch))
  {
    send_to_char(ch, "You have completed your supply order, "
                     "go turn it in (type 'supplyorder complete' in a supplyorder office).\r\n");
    return 1;
  }

  material = GET_AUTOCQUEST_MATERIAL(ch);

  /* Cycle through contents and categorize */
  for (obj = kit->contains; obj != NULL; obj = obj->next_content)
  {
    if (obj)
    {
      if (GET_OBJ_TYPE(obj) != ITEM_MATERIAL)
      {
        send_to_char(ch, "Only materials should be inside the kit in"
                         " order to complete a supplyorder.\r\n");
        return 1;
      }
      else if (GET_OBJ_TYPE(obj) == ITEM_MATERIAL)
      {
        if (GET_OBJ_VAL(obj, 0) >= 2)
        {
          send_to_char(ch, "%s is a bundled item, which must first be unbundled before you can use it to craft.\r\n", obj->short_description);
          return 1;
        }
        if (GET_OBJ_MATERIAL(obj) != material)
        {
          send_to_char(ch, "You need %s to complete this supplyorder.\r\n",
                       material_name[GET_AUTOCQUEST_MATERIAL(ch)]);
          return 1;
        }
        material_level = GET_OBJ_LEVEL(obj); // material level affects gold
        obj_vnum = GET_OBJ_VNUM(obj);
        num_mats++; /* we found matching material */
        if (num_mats > SUPPLYORDER_MATS)
        {
          send_to_char(ch, "You have too much materials in the kit, put "
                           "exactly %d for the supplyorder.\r\n",
                       SUPPLYORDER_MATS);
          return 1;
        }
      }
      else
      { /* must be an essence */
        send_to_char(ch, "Essence items will not work for supplyorders!\r\n");
        return 1;
      }
    }
  }

  if (num_mats < SUPPLYORDER_MATS)
  {
    send_to_char(ch, "You have %d material units in the kit, you will need "
                     "%d more units to complete the supplyorder.\r\n",
                 num_mats, SUPPLYORDER_MATS - num_mats);
    return 1;
  }

  GET_CRAFTING_TYPE(ch) = SCMD_SUPPLYORDER;
  GET_CRAFTING_TICKS(ch) = 5 - fast_craft_bonus;
  GET_AUTOCQUEST_GOLD(ch) += GET_LEVEL(ch);
  send_to_char(ch, "You begin a supply order for %s.\r\n",
               GET_AUTOCQUEST_DESC(ch));
  act("$n begins a supply order.", FALSE, ch, NULL, 0, TO_ROOM);

  obj_vnum = GET_OBJ_VNUM(kit);
  obj_from_char(kit);
  extract_obj(kit);
  kit = read_object(obj_vnum, VIRTUAL);
  obj_to_char(kit, ch);
  save_char(ch, 0);
  Crash_crashsave(ch);
  NEW_EVENT(eCRAFTING, ch, NULL, 1 * PASSES_PER_SEC);

  return 1;
}

/* resize an object, also will change weapon damage */
int resize(char *argument, struct obj_data *kit, struct char_data *ch)
{
  int num_objs = 0, newsize, cost;
  struct obj_data *obj = NULL;
  int num_dice = -1;
  int size_dice = -1;
  int fast_craft_bonus = GET_SKILL(ch, SKILL_FAST_CRAFTER) / 33;

  /* Cycle through contents */
  /* resize requires just one item be inside the kit */
  for (obj = kit->contains; obj != NULL; obj = obj->next_content)
  {
    num_objs++;
    break;
  }

  if (num_objs > 1)
  {
    send_to_char(ch, "Only one item should be inside the kit.\r\n");
    return 1;
  }

  if (is_abbrev(argument, "fine"))
    newsize = SIZE_FINE;
  else if (is_abbrev(argument, "diminutive"))
    newsize = SIZE_DIMINUTIVE;
  else if (is_abbrev(argument, "tiny"))
    newsize = SIZE_TINY;
  else if (is_abbrev(argument, "small"))
    newsize = SIZE_SMALL;
  else if (is_abbrev(argument, "medium"))
    newsize = SIZE_MEDIUM;
  else if (is_abbrev(argument, "large"))
    newsize = SIZE_LARGE;
  else if (is_abbrev(argument, "huge"))
    newsize = SIZE_HUGE;
  else if (is_abbrev(argument, "gargantuan"))
    newsize = SIZE_GARGANTUAN;
  else if (is_abbrev(argument, "colossal"))
    newsize = SIZE_COLOSSAL;
  else
  {
    send_to_char(ch, "That is not a valid size: (fine|diminutive|tiny|small|"
                     "medium|large|huge|gargantuan|colossal)\r\n");
    return 1;
  }

  if (newsize == GET_OBJ_SIZE(obj))
  {
    send_to_char(ch, "The object is already the size you desire.\r\n");
    return 1;
  }

  /* "cost" of resizing */
  cost = GET_OBJ_COST(obj) / 2;
  // if it's a race changing the size to their own so they can use it normally, we don't want to penalize them with a cost in gold
  if (newsize == GET_SIZE(ch))
  {
    cost = 0;
  }

  if (GET_GOLD(ch) < cost)
  {
    send_to_char(ch, "You need %d coins on hand for supplies to resize this "
                     "item.\r\n",
                 cost);
    return 1;
  }

  /* weapon damage adjustment */
  if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
  {
    num_dice = GET_OBJ_VAL(obj, 1);
    size_dice = GET_OBJ_VAL(obj, 2);

    if (scale_damage(ch, obj, newsize))
    {
      /* success, weapon upgraded or downgraded in damage
         corresponding to size change */
      send_to_char(ch, "Weapon change:  %dd%d to %dd%d\r\n",
                   num_dice, size_dice, GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2));
    }
    else
    {
      send_to_char(ch, "You failed to resize this weapon!\r\n");
      return 1;
    }
  }

  if (cost > 0)
  {
    send_to_char(ch, "It cost you %d coins to resize this item.\r\n",
                 cost);
    GET_GOLD(ch) -= cost;
  }
  send_to_char(ch, "You begin to resize %s from %s to %s.\r\n",
               obj->short_description, size_names[GET_OBJ_SIZE(obj)],
               size_names[newsize]);
  act("$n begins resizing $p.", FALSE, ch, obj, 0, TO_ROOM);
  obj_from_obj(obj);

  /* resize object after taking out of kit, otherwise issues */
  /* weight adjustment of object */
  GET_OBJ_SIZE(obj) = newsize;
  GET_OBJ_WEIGHT(obj) += (newsize - GET_OBJ_SIZE(obj)) * GET_OBJ_WEIGHT(obj);
  if (GET_OBJ_WEIGHT(obj) <= 0)
    GET_OBJ_WEIGHT(obj) = 1;

  GET_CRAFTING_OBJ(ch) = obj;
  GET_CRAFTING_TYPE(ch) = SCMD_RESIZE;
  if (cost == 0)
    GET_CRAFTING_TICKS(ch) = 1;
  else
    GET_CRAFTING_TICKS(ch) = 5 - fast_craft_bonus;

  obj_to_char(obj, ch);
  save_char(ch, 0);
  Crash_crashsave(ch);
  NEW_EVENT(eCRAFTING, ch, NULL, 1 * PASSES_PER_SEC);

  return 1;
}


/* change armor from original material to bone material */
int bonearmor(char *argument, struct obj_data *kit, struct char_data *ch)
{
  int num_objs = 0, cost;
  struct obj_data *obj = NULL;
  int fast_craft_bonus = GET_SKILL(ch, SKILL_FAST_CRAFTER) / 33;
  char buf[MAX_STRING_LENGTH];

  if (!HAS_REAL_FEAT(ch, FEAT_BONE_ARMOR))
  {
    send_to_char(ch, "You must have the bone armor feat to convert armor into bone.\r\n");
    return 1;
  }

  /* Cycle through contents */
  /* resize requires just one item be inside the kit */
  for (obj = kit->contains; obj != NULL; obj = obj->next_content)
  {
    num_objs++;
    break;
  }

  if (num_objs > 1)
  {
    send_to_char(ch, "Only one item should be inside the kit.\r\n");
    return 1;
  }

    if (GET_OBJ_TYPE(obj) != ITEM_ARMOR)
  {
    send_to_char(ch, "You can only convert armor and shields to bone.\r\n");
    return 1;
  }
 

  if (GET_OBJ_MATERIAL(obj) == MATERIAL_BONE)
  {
    send_to_char(ch, "The object is already made of bone.\r\n");
    return 1;
  }

  if (!strstr(argument, material_name[MATERIAL_BONE]))
  {
    send_to_char(ch, "You must include the material name, '%s', in the object description somewhere.\r\n", material_name[MATERIAL_BONE]);
    return 1;
  }

  /* "cost" of resizing */
  cost = GET_OBJ_COST(obj) / 3;

  if (GET_GOLD(ch) < cost)
  {
    send_to_char(ch, "You need %d coins on hand for supplies to convert this item into bone.\r\n", cost);
    return 1;
  }

  if (cost > 0)
  {
    send_to_char(ch, "It cost you %d coins to convert this item into bone.\r\n", cost);
    GET_GOLD(ch) -= cost;
  }

  /* you need to parse the @ sign */
  parse_at(argument);

  /* success!! */
  if (obj->name) free(obj->name);
  obj->name = strdup(argument);
  strip_colors(obj->name);
  if (obj->short_description) free(obj->short_description);
  obj->short_description = strdup(argument);
  snprintf(buf, sizeof(buf), "%s lies here.", CAP(argument));
  if (obj->description) free(obj->description);
  if (obj->description) free(obj->description);
  obj->description = strdup(buf);

  send_to_char(ch, "You begin to convert %s into bone.\r\n", obj->short_description);
  act("$n begins convetring $p to bone.", FALSE, ch, obj, 0, TO_ROOM);
  obj_from_obj(obj);

  GET_OBJ_MATERIAL(obj) = MATERIAL_BONE;
  if (GET_OBJ_WEIGHT(obj) <= 0)
    GET_OBJ_WEIGHT(obj) = 1;

  GET_CRAFTING_OBJ(ch) = obj;
  GET_CRAFTING_TYPE(ch) = SCMD_BONEARMOR;
  if (cost == 0)
    GET_CRAFTING_TICKS(ch) = 1;
  else
    GET_CRAFTING_TICKS(ch) = MAX(1, 5 - fast_craft_bonus);

  obj_to_char(obj, ch);
  save_char(ch, 0);
  Crash_crashsave(ch);
  NEW_EVENT(eCRAFTING, ch, NULL, 1 * PASSES_PER_SEC);

  return 1;
}

/* change armor or weapon from one type to another */
int reforge(char *argument, struct obj_data *kit, struct char_data *ch)
{
  int num_objs = 0, cost;
  struct obj_data *obj = NULL;
  int fast_craft_bonus = GET_SKILL(ch, SKILL_FAST_CRAFTER) / 33;
  char buf[MAX_STRING_LENGTH];
  int i = 0;
  char bonus[30];
  int orig_cost = 0, enhancement = 0, material = 0;

  /* Cycle through contents */
  /* resize requires just one item be inside the kit */
  for (obj = kit->contains; obj != NULL; obj = obj->next_content)
  {
    num_objs++;
    break;
  }

  if (num_objs > 1)
  {
    send_to_char(ch, "Only one item should be inside the kit.\r\n");
    return 1;
  }

    if (GET_OBJ_TYPE(obj) != ITEM_ARMOR && GET_OBJ_TYPE(obj) != ITEM_WEAPON)
  {
    send_to_char(ch, "You can only reforge armor, shields and weapons.\r\n");
    return 1;
  }

  skip_spaces(&argument);

  if (!*argument)
  {
    send_to_char(ch, "Please specify the type of weapon, armor or shield you'd like to reforge this item into. Type weaponlist or armorlistfull to see options.\r\n");
    return 1;
  }

  orig_cost = GET_OBJ_COST(obj);
  enhancement = GET_OBJ_VAL(obj, 4);
  material = GET_OBJ_MATERIAL(obj);

  /* "cost" of reforge */
  cost = GET_OBJ_COST(obj) / 2;

  if (GET_GOLD(ch) < cost)
  {
    send_to_char(ch, "You need %d coins on hand for supplies to reforge this item.\r\n", cost);
    return 1;
  }

  switch (GET_OBJ_TYPE(obj))
  {
    case ITEM_WEAPON:
      for (i = 0; i < NUM_WEAPON_TYPES; i++)
      {
        if (is_abbrev(weapon_list[i].name, argument))
          break;
      }
      if (i >= NUM_WEAPON_TYPES)
      {
        send_to_char(ch, "That is not a valid weapon type. Type weaponlist for options.\r\n");
        return 1;
      }
      if (i == GET_OBJ_VAL(obj, 0))
      {
        send_to_char(ch, "The item is already %s %s.\r\n", AN(weapon_list[i].name), weapon_list[i].name);
        return 1;
      }
      set_weapon_object(obj, i);
      break;
    case ITEM_ARMOR:
      if (IS_SHIELD(GET_OBJ_VAL(obj, 1)))
      {
        for (i = 0; i < NUM_SPEC_ARMOR_TYPES; i++)
        {
          if (!IS_SHIELD(i)) continue;
          if (is_abbrev(armor_list[i].name, argument))
            break;
        }
        if (i >= NUM_SPEC_ARMOR_TYPES)
        {
          send_to_char(ch, "That is not a valid shield type. Type armorlistfull for options.\r\n");
          return 1;
        }
        if (i == GET_OBJ_VAL(obj, 1))
        {
          send_to_char(ch, "The item is already %s %s.\r\n", AN(armor_list[i].name), armor_list[i].name);
          return 1;
        }
        GET_OBJ_VAL(obj, 1) = i;
      }
      else
      {
        for (i = 0; i < NUM_SPEC_ARMOR_TYPES; i++)
        {
          if (IS_SHIELD(i)) continue;
          if (CAN_WEAR(obj, ITEM_WEAR_HEAD) && armor_list[i].wear != ITEM_WEAR_HEAD)
          {
            continue;
          }
          else if (CAN_WEAR(obj, ITEM_WEAR_BODY) && armor_list[i].wear != ITEM_WEAR_BODY)
          {
            continue;
          }
          else if (CAN_WEAR(obj, ITEM_WEAR_ARMS) && armor_list[i].wear != ITEM_WEAR_ARMS)
          {
            continue;
          }
          else if (CAN_WEAR(obj, ITEM_WEAR_LEGS) && armor_list[i].wear != ITEM_WEAR_LEGS)
          {
            continue;
          }
          if (is_abbrev(armor_list[i].name, argument))
            break;
        }
        if (i >= NUM_SPEC_ARMOR_TYPES)
        {
          send_to_char(ch, "That is not a valid armor type. Type armorlistfull for options. Please ensure wear types match. Ie. You can only reforge a body wear slot to another body wear slot.\r\n");
          return 1;
        }
        if (i == GET_OBJ_VAL(obj, 1))
        {
          send_to_char(ch, "The item is already %s %s.\r\n", AN(armor_list[i].name), armor_list[i].name);
          return 1;
        }
        GET_OBJ_VAL(obj, 1) = i;
      }
      set_armor_object(obj, i);
      break;
    default:
      send_to_char(ch, "You can only reforge armor, shields and weapons.\r\n");
      return 1;
  }

  GET_OBJ_COST(obj) = orig_cost;
  GET_OBJ_VAL(obj, 4) = enhancement;
  // restore the original material if new item type is of the same material type as orig.
  if (IS_HARD_METAL(GET_OBJ_MATERIAL(obj)) && IS_HARD_METAL(material))
    GET_OBJ_MATERIAL(obj) = material;
  else if (IS_LEATHER(GET_OBJ_MATERIAL(obj)) && IS_LEATHER(material))
    GET_OBJ_MATERIAL(obj) = material;
  else if (IS_CLOTH(GET_OBJ_MATERIAL(obj)) && IS_CLOTH(material))
    GET_OBJ_MATERIAL(obj) = material;
  else if (IS_WOOD(GET_OBJ_MATERIAL(obj)) && IS_WOOD(material))
    GET_OBJ_MATERIAL(obj) = material;

  if (cost > 0)
  {
    send_to_char(ch, "It cost you %d coins to reforge this item.\r\n", cost);
    GET_GOLD(ch) -= cost;
  }

  send_to_char(ch, "You begin to reforge %s into %s %s.\r\n", obj->short_description, 
                    (GET_OBJ_TYPE(obj) == ITEM_WEAPON) ? AN(weapon_list[GET_OBJ_VAL(obj, 0)].name) : AN(armor_list[GET_OBJ_VAL(obj, 1)].name), 
                    (GET_OBJ_TYPE(obj) == ITEM_WEAPON) ? weapon_list[GET_OBJ_VAL(obj, 0)].name : armor_list[GET_OBJ_VAL(obj, 1)].name);
  snprintf(buf, sizeof(buf), "$n begins to reforge %s into %s %s.", obj->short_description, 
                    (GET_OBJ_TYPE(obj) == ITEM_WEAPON) ? AN(weapon_list[GET_OBJ_VAL(obj, 0)].name) : AN(armor_list[GET_OBJ_VAL(obj, 1)].name), 
                    (GET_OBJ_TYPE(obj) == ITEM_WEAPON) ? weapon_list[GET_OBJ_VAL(obj, 0)].name : armor_list[GET_OBJ_VAL(obj, 1)].name);
  act(buf, FALSE, ch, obj, 0, TO_ROOM);

  // new descriptions
  if (GET_OBJ_VAL(obj, 4) > 0)
    snprintf(bonus, sizeof(bonus), "(+%d)", GET_OBJ_VAL(obj, 4));
  else
    snprintf(bonus, sizeof(bonus), "(no enchantment bonus)");

  if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
  {   
    snprintf(buf, sizeof(buf), "a reforged %s %s", weapon_list[GET_OBJ_VAL(obj, 0)].name, bonus);
  }
  else
  {  
    snprintf(buf, sizeof(buf), "a reforged %s %s", armor_list[GET_OBJ_VAL(obj, 1)].name, bonus);
  }

  if (obj->name) free(obj->name);
  obj->name = strdup(buf);
  strip_colors(obj->name);
  if (obj->short_description) free(obj->short_description);
  obj->short_description = strdup(buf);
  
  /* Fix string memory leak - CAP modifies the string in-place, but strdup creates a leak */
  char *temp_str = strdup(obj->short_description);
  snprintf(buf, sizeof(buf), "%s lies here.", CAP(temp_str));
  if (obj->description) free(obj->description);
  obj->description = strdup(buf);
  free(temp_str);

  obj_from_obj(obj);

  if (GET_OBJ_WEIGHT(obj) <= 0)
    GET_OBJ_WEIGHT(obj) = 1;

  GET_CRAFTING_OBJ(ch) = obj;
  GET_CRAFTING_TYPE(ch) = SCMD_REFORGE;
  if (cost == 0)
    GET_CRAFTING_TICKS(ch) = 1;
  else
    GET_CRAFTING_TICKS(ch) = 10 - fast_craft_bonus;

  obj_to_char(obj, ch);
  save_char(ch, 0);
  Crash_crashsave(ch);
  NEW_EVENT(eCRAFTING, ch, NULL, 1 * PASSES_PER_SEC);

  return 1;
}

/* convert magic objects to essence */
int disenchant(struct obj_data *kit, struct char_data *ch)
{
  struct obj_data *obj = NULL;
  int num_objs = 0, essence_level = 0;
  int fast_craft_bonus = GET_SKILL(ch, SKILL_FAST_CRAFTER) / 33;
  int chem_check = GET_SKILL(ch, SKILL_CHEMISTRY) + d20(ch);

  /* Cycle through contents */
  /* disenchant requires just one item be inside the kit */
  for (obj = kit->contains; obj != NULL; obj = obj->next_content)
  {
    num_objs++;
  }
  if (num_objs > 1)
  {
    send_to_char(ch, "Only one item should be inside the kit.\r\n");
    return 1;
  }

  if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
  {
    send_to_char(ch, "You must drop something before you can disenchant anything.\r\n");
    return 1;
  }
  for (obj = kit->contains; obj != NULL; obj = obj->next_content)
  {
    break; // this should be the object
  }

  if (!obj)
  {
    send_to_char(ch, "You do not seem to have a magical item in the kit.\r\n");
    return 1;
  }

  if (!IS_SET_AR(GET_OBJ_EXTRA(obj), ITEM_MAGIC))
  {
    send_to_char(ch, "Only magical items can be disenchanted.\r\n");
    return 1;
  }

  /* You can disenchant object level equal to your: chemistry-skill / 3 + 1 */
  if (GET_OBJ_LEVEL(obj) < 5)
  {
    send_to_char(ch, "You need a more powerful object to have any chance of "
                     "extracting magic essence.\r\n");
    return 1;
  }

  /* determine the level of this essence */
  essence_level = dice(1, ((GET_OBJ_LEVEL(obj) / 2)));

  /* getting complaints it is too slow to notch - zusuk */
  if (!IS_NPC(ch))
  {
    increase_skill(ch, SKILL_CHEMISTRY);
    increase_skill(ch, SKILL_CHEMISTRY);
    increase_skill(ch, SKILL_CHEMISTRY);
  }

  GET_CRAFTING_TYPE(ch) = SCMD_DISENCHANT;
  GET_CRAFTING_TICKS(ch) = MAX(2, 11 - fast_craft_bonus);
  GET_CRAFTING_OBJ(ch) = NULL;

  send_to_char(ch, "You begin to disenchant %s.\r\n", obj->short_description);
  act("$n begins to disenchant $p.", FALSE, ch, obj, 0, TO_ROOM);

  /* clear item that got disenchanted */
  obj_from_obj(obj);
  extract_obj(obj);

  /* make the check! */
  if (chem_check <= (GET_OBJ_LEVEL(obj) * 3 + 10))
  {
    /* fail! */
    send_to_char(ch, "You are having difficulty extracting the magical essence...\r\n");
  }
  else
  {
    /* create the essence */
    obj = read_object(MAGICAL_ESSENCE, VIRTUAL);
    if (!obj)
    {
      log("Failed to load the seence object in disenchant()");
      send_to_char(ch, "Report to staff please: disenchant failed to load essence object.\r\n");
      return 1;
    }
    GET_OBJ_LEVEL(obj) = essence_level;
    obj_to_char(obj, ch);
  }

  save_char(ch, 0);
  Crash_crashsave(ch);
  NEW_EVENT(eCRAFTING, ch, NULL, 1 * PASSES_PER_SEC);
  return 1;
}

/* our create command and craftcheck, mode determines which we're using */
/* mode = 1; create     */
/* mode = 2; craftcheck */

/* As an extra layer of protection, only ITEM_MOLD should be used for
 * crafting.  It should be hard-coded un-wearable, BUT have the exact WEAR_
 * flags you want it to create. Also it should have the raw stats of the
 * item you want it to turn into.  Otherwise you could run into some issues
 * with stacking stats, etc.
 */

/*
 * create is for wearable gear at this stage
 */
#define CREATE_STRING_LIMIT 80

int create(char *argument, struct obj_data *kit, struct char_data *ch, int mode)
{
  char buf[MAX_INPUT_LENGTH] = {'\0'};
  struct obj_data *obj = NULL, *mold = NULL, *crystal = NULL,
                  *material = NULL, *essence = NULL;
  int num_mats = 0, obj_level = 1, skill = ABILITY_CRAFT_WEAPONSMITHING,
      mats_needed = 12345, found = 0, i = 0, l = 0;
  int fast_craft_bonus = 0;
  int chance_of_crit = 0;

  /* weird find, color codes doesn't play nice with the ' character -zusuk */
  if (mode == CREATE_MODE_CREATE && *argument)
  {
    for (l = 0; *(argument + l); l++)
    {
      if (*(argument + l) == '\'')
      {
        send_to_char(ch, "The usage of the character: ' is not allowed in create "
                         "currently (it conflicts with color codes).\r\n");
        return 1;
      }
    }
  }

  /* string length limit  -zusuk */
  if (l > CREATE_STRING_LIMIT)
  {
    send_to_char(ch, "The length (%d) of the name you gave your object is over "
                     "the limit (%d).\r\n",
                 l, CREATE_STRING_LIMIT);
    return 1;
  }

  /* sort through our kit and check if we got everything we need */
  for (obj = kit->contains; obj != NULL; obj = obj->next_content)
  {
    if (obj)
    {
      /* find a mold? */
      if (OBJ_FLAGGED(obj, ITEM_MOLD))
      {
        if (!mold)
        {
          mold = obj;
          found++;
        }
        else
        {
          send_to_char(ch, "You have more than one mold inside the kit, "
                           "please only put one inside.\r\n");
          return 1;
        }
      }

      if (found)
      { // we didn't have a mold and found one, iterate main loop
        found = FALSE;
        continue;
      }

      /* find a crystal? */
      if (GET_OBJ_TYPE(obj) == ITEM_CRYSTAL)
      {
        if (!crystal)
        {
          crystal = obj;
        }
        else
        {
          send_to_char(ch, "You have more than one crystal inside the kit, "
                           "please only put one inside.\r\n");
          return 1;
        }

        /* find a material? */
      }
      else if (GET_OBJ_TYPE(obj) == ITEM_MATERIAL)
      {
        if (GET_OBJ_VAL(obj, 0) >= 2)
        {
          send_to_char(ch, "%s is a bundled item, which must first be unbundled before you can use it to craft.\r\n", obj->short_description);
          return 1;
        }
        if (!material)
        {
          material = obj;
          num_mats++;
        }
        else if (GET_OBJ_MATERIAL(obj) != GET_OBJ_MATERIAL(material))
        {
          send_to_char(ch, "You have mixed materials in the kit, please "
                           "make sure to use only the required materials.\r\n");
          return 1;
        }
        else
        { /* this should be good */
          num_mats++;
        }

        /* find an essence? */
      }
      else if (GET_OBJ_TYPE(obj) == ITEM_ESSENCE)
      {
        if (!essence)
        {
          essence = obj;
        }
        else
        {
          send_to_char(ch, "You have more than one essence inside the kit, "
                           "please only put one inside.\r\n");
          return 1;
        }
      }
      else
      { /* didn't find anything we need */
        send_to_char(ch, "There is an unnecessary item in the kit, please "
                         "remove it.\r\n");
        return 1;
      }
    }
  } /* end our sorting loop */

  /** check we have all the ingredients we need **/
  if (!mold)
  {
    send_to_char(ch, "The creation process requires a mold to continue.\r\n");
    return 1;
  }

  /* set base level, crystal should be ultimate determinant */
  obj_level = GET_OBJ_LEVEL(mold);
  if (crystal)
    obj_level = GET_OBJ_LEVEL(crystal);

  if (!material)
  {
    send_to_char(ch, "You need to put materials into the kit.\r\n");
    return 1;
  }

  /* right material? */
  if (HAS_FEAT(ch, FEAT_BONE_ARMOR) &&
      (GET_OBJ_MATERIAL(material) == MATERIAL_BONE || GET_OBJ_MATERIAL(material) == MATERIAL_DRAGONBONE))
  {
    send_to_char(ch, "You use your mastery in bone-crafting to substitutue "
                     "bone for the normal material needed...\r\n");
  }
  else if (IS_CLOTH(GET_OBJ_MATERIAL(mold)) &&
           !IS_CLOTH(GET_OBJ_MATERIAL(material)))
  {
    send_to_char(ch, "You need cloth for this mold pattern.\r\n");
    return 1;
  }
  else if (IS_LEATHER(GET_OBJ_MATERIAL(mold)) &&
           !IS_LEATHER(GET_OBJ_MATERIAL(material)))
  {
    send_to_char(ch, "You need leather for this mold pattern.\r\n");
    return 1;
  }
  else if (IS_WOOD(GET_OBJ_MATERIAL(mold)) &&
           !IS_WOOD(GET_OBJ_MATERIAL(material)))
  {
    send_to_char(ch, "You need wood for this mold pattern.\r\n");
    return 1;
  }
  else if (IS_HARD_METAL(GET_OBJ_MATERIAL(mold)) &&
           !IS_HARD_METAL(GET_OBJ_MATERIAL(material)))
  {
    send_to_char(ch, "You need hard metal for this mold pattern.\r\n");
    return 1;
  }
  else if (IS_PRECIOUS_METAL(GET_OBJ_MATERIAL(mold)) &&
           !IS_PRECIOUS_METAL(GET_OBJ_MATERIAL(material)))
  {
    send_to_char(ch, "You need precious metal for this mold pattern.\r\n");
    return 1;
  }
  else if (GET_OBJ_TYPE(mold) == ITEM_WEAPON && GET_OBJ_MATERIAL(material) == MATERIAL_DRAGONSCALE)
  {
    send_to_char(ch, "You can't use dragonscale to make weapons.\r\n");
    return 1;
  }
  else if (GET_OBJ_TYPE(mold) == ITEM_ARMOR && GET_OBJ_MATERIAL(material) == MATERIAL_DRAGONBONE)
  {
    send_to_char(ch, "You can't use dragonbone to make armor or shields.\r\n");
    return 1;
  }
  /* we should be OK at this point with material validity, */
  /* although more error checking might be good */
  /* valid_misc_item_material_type(mold, material)) */
  /* expansion here or above to other miscellaneous materials, etc */

  /* determine how much material is needed
   * [mold weight divided by weight_factor]
   */
  mats_needed = MAX(MIN_MATS, (GET_OBJ_WEIGHT(mold) / WEIGHT_FACTOR));

  /* elven crafting reduces material needed */
  if (HAS_FEAT(ch, FEAT_ELVEN_CRAFTING))
    mats_needed = MAX(MIN_ELF_MATS, mats_needed / 2);

  if (num_mats < mats_needed)
  {
    send_to_char(ch, "You do not have enough materials to make that item.  "
                     "You need %d more units of the same type.\r\n",
                 mats_needed - num_mats);
    return 1;
  }
  else if (num_mats > mats_needed)
  {
    send_to_char(ch, "You put too much material in the kit, please "
                     "take out %d units.\r\n",
                 num_mats - mats_needed);
    return 1;
  }

  /** check for other disqualifiers */
  /* valid name */
  if (mode == CREATE_MODE_CREATE && !strstr(argument,
                                            material_name[GET_OBJ_MATERIAL(material)]))
  {
    send_to_char(ch, "You must include the material name, '%s', in the object "
                     "description somewhere.\r\n",
                 material_name[GET_OBJ_MATERIAL(material)]);

    return 1;
  }

  /* calculate chance for master work */
  if (essence)
  {
    chance_of_crit += (GET_OBJ_LEVEL(essence) * 2);
    /* feat, etc bonuses */
    if (HAS_FEAT(ch, FEAT_MASTERWORK_CRAFTING))
    {
      send_to_char(ch, "Your masterwork-crafting skill increases the chance of creating a master-piece!\r\n");
      chance_of_crit += 10;
    }
    if (HAS_FEAT(ch, FEAT_DWARVEN_CRAFTING))
    {
      send_to_char(ch, "Your dwarven-crafting skill increases the chance of creating a master-piece!\r\n");
      chance_of_crit += 10;
    }
    if (HAS_FEAT(ch, FEAT_DRACONIC_CRAFTING))
    {
      send_to_char(ch, "Your draconic-crafting skill increases the chance of creating a master-piece!\r\n");
      chance_of_crit += 10;
    }
  }

  /* which skill is used for this crafting session? */
  /* we determine crafting skill by wear-flag */

  /* jewel making (finger, */
  if (CAN_WEAR(mold, ITEM_WEAR_FINGER) ||
      CAN_WEAR(mold, ITEM_WEAR_ANKLE) ||
      CAN_WEAR(mold, ITEM_WEAR_NECK) ||
      CAN_WEAR(mold, ITEM_WEAR_HOLD))
  {
    skill = SKILL_JEWELRY_MAKING;
  } /* body armor pieces: either armor-smith/leather-worker/or knitting */
  else if (CAN_WEAR(mold, ITEM_WEAR_BODY) ||
           CAN_WEAR(mold, ITEM_WEAR_ARMS) ||
           CAN_WEAR(mold, ITEM_WEAR_LEGS) ||
           CAN_WEAR(mold, ITEM_WEAR_HEAD) ||
           CAN_WEAR(mold, ITEM_WEAR_FEET) ||
           CAN_WEAR(mold, ITEM_WEAR_HANDS) ||
           CAN_WEAR(mold, ITEM_WEAR_WRIST) ||
           CAN_WEAR(mold, ITEM_WEAR_WAIST))
  {
    if (IS_HARD_METAL(GET_OBJ_MATERIAL(mold)))
      skill = SKILL_ARMOR_SMITHING;
    else if (IS_LEATHER(GET_OBJ_MATERIAL(mold)))
      skill = SKILL_LEATHER_WORKING;
    else
      skill = SKILL_KNITTING;
  } /* about body */
  else if (CAN_WEAR(mold, ITEM_WEAR_ABOUT))
  {
    skill = SKILL_KNITTING;
  } /* weapon-smithing:  weapons and shields */
  else if (CAN_WEAR(mold, ITEM_WEAR_WIELD) ||
           CAN_WEAR(mold, ITEM_WEAR_SHIELD))
  {
    skill = SKILL_WEAPON_SMITHING;
  }

  /* skill restriction */
  if (GET_SKILL(ch, skill) / 3 < obj_level)
  {
    send_to_char(ch, "Your skill in %s (%d) is too low to create that item, you need %d.\r\n",
                 spell_info[skill].name, GET_SKILL(ch, skill), obj_level * 3);
    return 1;
  }

  int cost = obj_level * obj_level * 100 / 3;

  /** passed all the tests, time to check or create the item **/
  if (CREATE_MODE_CHECK == mode)
  { /* checkcraft */
    send_to_char(ch, "This crafting session will create the following "
                     "item:\r\n\r\n");
    do_stat_object(ch, mold, ITEM_STAT_MODE_IDENTIFY_SPELL);
    if (crystal)
    {
      send_to_char(ch, "You will be enhancing it with this crystal:\r\n");
      do_stat_object(ch, crystal, ITEM_STAT_MODE_IDENTIFY_SPELL);
    }
    /* calculate chance for master work */
    if (essence)
    {
      send_to_char(ch, "Basic essence chance of critical (masterwork): %d.  ",
                   GET_OBJ_LEVEL(essence) * 2);
      /* feat, etc bonuses */
      if (HAS_FEAT(ch, FEAT_MASTERWORK_CRAFTING))
        send_to_char(ch, "Masterwork Crafting feat bonus: 10.  ");
      if (HAS_FEAT(ch, FEAT_DWARVEN_CRAFTING))
        send_to_char(ch, "Dwarven Crafting feat bonus: 10.  ");
      if (HAS_FEAT(ch, FEAT_DRACONIC_CRAFTING))
        send_to_char(ch, "Draconic Crafting feat bonus: 10.  ");
      send_to_char(ch, "\r\n");
      send_to_char(ch, "You have a %d percent chance of creating a masterwork item.\r\n",
                   chance_of_crit);
    }
    send_to_char(ch, "The item will be level: %d.\r\n", obj_level);
    send_to_char(ch, "It will make use of your %s skill, which has a value "
                     "of %d.\r\n",
                 spell_info[skill].name, GET_SKILL(ch, skill));
    send_to_char(ch, "This crafting session will take 60 seconds.\r\n");
    send_to_char(ch, "You need %d gold on hand to make this item.\r\n", cost);

    return 1;
  }

  /* not enough gold? */
  else if (GET_GOLD(ch) < cost)
  {
    send_to_char(ch, "You need %d coins on hand for supplies to make"
                     "this item.\r\n",
                 cost);
    return 1;
  }
  /* CREATE! */
  else
  {
    REMOVE_BIT_AR(GET_OBJ_EXTRA(mold), ITEM_MOLD);
    if (essence || crystal)
      SET_BIT_AR(GET_OBJ_EXTRA(mold), ITEM_MAGIC);
    GET_OBJ_LEVEL(mold) = obj_level;
    GET_OBJ_MATERIAL(mold) = GET_OBJ_MATERIAL(material);

    /* transfer crystal over to item */
    if (crystal)
    {
      for (i = 0; i < MAX_OBJ_AFFECT; i++)
      {
        if (crystal->affected[i].modifier && crystal->affected[i].location)
        {
          mold->affected[i].location = crystal->affected[i].location;
          mold->affected[i].modifier = crystal->affected[i].modifier;
          if (!crystal->affected[i].bonus_type)
            mold->affected[i].bonus_type = BONUS_TYPE_ENHANCEMENT;
          else
            mold->affected[i].bonus_type = crystal->affected[i].bonus_type;
        }
      }
      /* enhancement bonus */
      if (CAN_WEAR(mold, ITEM_WEAR_WIELD) || CAN_WEAR(mold, ITEM_WEAR_SHIELD) ||
          CAN_WEAR(mold, ITEM_WEAR_HEAD) || CAN_WEAR(mold, ITEM_WEAR_BODY) ||
          CAN_WEAR(mold, ITEM_WEAR_LEGS) || CAN_WEAR(mold, ITEM_WEAR_ARMS) ||
          GET_OBJ_TYPE(mold) == ITEM_MISSILE)
      {
        GET_OBJ_VAL(mold, 4) = MIN(CRAFT_MAX_BONUS, ((GET_OBJ_LEVEL(mold) + 5) / 5));
      }
    }

    /* try for master-work craft! */
    if (essence)
    {
      /*debug*/
      if (GET_LEVEL(ch) >= LVL_IMMORT)
      {
        send_to_char(ch, "Staff override on crit chance (real chance: %d)\r\n",
                     chance_of_crit);
        chance_of_crit = 101;
      }
      /*debug*/
      if (dice(1, 100) <= chance_of_crit)
      {
        /* did it! we assumed [3rd] value is available for this bonus */
        mold->affected[3].location = random_apply_value();
        mold->affected[3].modifier = adjust_bonus_value(mold->affected[3].location, 1);
        mold->affected[3].bonus_type = BONUS_TYPE_INHERENT;
        send_to_char(ch, "You feel a sense of inspiration as you begin your craft!\r\n");
      }
    }

    GET_OBJ_COST(mold) = 100 + GET_OBJ_LEVEL(mold) * 50 * MAX(1, GET_OBJ_LEVEL(mold) - 1) + GET_OBJ_COST(mold);
    GET_CRAFTING_BONUS(ch) = 10 + MIN(60, GET_OBJ_LEVEL(mold));

    send_to_char(ch, "It cost you %d gold in supplies to create this item.\r\n",
                 cost);
    GET_GOLD(ch) -= cost;

    /* gotta convert @ sign */
    parse_at(argument);

    /* restringing aspect */
    if (mold->short_description) free(mold->short_description);
    mold->short_description = strdup(argument);
    snprintf(buf, sizeof(buf), "%s lies here.", CAP(argument));
    if (mold->description) free(mold->description);
    mold->description = strdup(buf);
    strip_colors(argument);
    if (mold->name) free(mold->name);
    mold->name = strdup(argument); /*keywords, leave last*/

    send_to_char(ch, "You begin to craft %s.\r\n", mold->short_description);
    act("$n begins to craft $p.", FALSE, ch, mold, 0, TO_ROOM);

    GET_CRAFTING_OBJ(ch) = mold;
    obj_from_obj(mold); /* extracting this causes issues, solution? */
    GET_CRAFTING_TYPE(ch) = SCMD_CRAFT;
    fast_craft_bonus = GET_SKILL(ch, SKILL_FAST_CRAFTER) / 33;
    GET_CRAFTING_TICKS(ch) = 11 - fast_craft_bonus;
    int kit_obj_vnum = GET_OBJ_VNUM(kit);
    obj_from_room(kit);
    extract_obj(kit);
    kit = read_object(kit_obj_vnum, VIRTUAL);

    obj_to_char(kit, ch);

    obj_to_char(mold, ch);

    /*save here just in case*/
    save_char(ch, 0);
    Crash_crashsave(ch);

    if (!IS_NPC(ch))
      increase_skill(ch, skill);
    NEW_EVENT(eCRAFTING, ch, NULL, 1 * PASSES_PER_SEC);
  }
  return 1;
}
#undef CREATE_STRING_LIMIT

SPECIAL(crafting_kit)
{
  if (!CMD_IS("resize") && !CMD_IS("create") && !CMD_IS("checkcraft") &&
      !CMD_IS("restring") && !CMD_IS("redesc") && !CMD_IS("augment") && !CMD_IS("convert") &&
      !CMD_IS("autocraft") && !CMD_IS("disenchant") && !CMD_IS("bonearmor") &&
      !CMD_IS("reforge"))
    return 0;

  if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
  {
    send_to_char(ch, "You cannot craft anything until you've made some "
                     "room in your inventory.\r\n");
    return 1;
  }

  if (GET_CRAFTING_OBJ(ch) || char_has_mud_event(ch, eCRAFTING))
  {
    send_to_char(ch, "You are already doing something.  Please wait until "
                     "your current task ends.\r\n");
    return 1;
  }

  struct obj_data *kit = (struct obj_data *)me;
  skip_spaces(&argument);

  /* Some of the commands require argument */
  if (!*argument && !CMD_IS("checkcraft") && !CMD_IS("augment") &&
      !CMD_IS("autocraft") && !CMD_IS("convert") && !CMD_IS("disenchant"))
  {
    if (CMD_IS("create") || CMD_IS("restring") || CMD_IS("bonearmor") || CMD_IS("redesc"))
      send_to_char(ch, "Please provide an item description containing the item name in the string.\r\n");
    else if (CMD_IS("resize"))
      send_to_char(ch, "What would you like the new size to be?"
                       " (fine|diminutive|tiny|small|"
                       "medium|large|huge|gargantuan|colossal)\r\n");
    else if (CMD_IS("reforge"))
    {
      send_to_char(ch, "Please specify the type of weapon, armor of shield you'd like to reforge this item into. See weaponlist and armorlistfull for options.\r\n");
    }
    return 1;
  }

  if (!kit->contains)
  {
    if (CMD_IS("augment"))
      send_to_char(ch, "You must place at least two crystals of the same "
                       "type into the kit in order to augment.\r\n");
    else if (CMD_IS("autocraft"))
    {
      if (GET_AUTOCQUEST_MATERIAL(ch))
        send_to_char(ch, "You must place %d units of %s or a similar type of "
                         "material (all the same type) into the kit to continue.\r\n",
                     SUPPLYORDER_MATS,
                     material_name[GET_AUTOCQUEST_MATERIAL(ch)]);
      else
        send_to_char(ch, "You do not have a supply order active "
                         "right now.\r\n");
    }
    else if (CMD_IS("create"))
      send_to_char(ch, "You must place an item to use as the mold pattern, "
                       "a crystal and your crafting resource materials in the "
                       "kit and then type 'create <optional item "
                       "description>'\r\n");
    else if (CMD_IS("restring"))
      send_to_char(ch, "You must place the item to restring and in the "
                       "crafting kit.\r\n");
    else if (CMD_IS("redesc"))
      send_to_char(ch, "You must place the item to redesc and in the "
                       "crafting kit.\r\n"); 
    else if (CMD_IS("resize"))
      send_to_char(ch, "You must place the item in the kit to resize it.\r\n");
    else if (CMD_IS("bonearmor"))
      send_to_char(ch, "You must place the item in the kit to convert it to bone armor.\r\n");
    else if (CMD_IS("reforge"))
      send_to_char(ch, "You must place the item in the kit to reforge it.\r\n");
    else if (CMD_IS("checkcraft"))
      send_to_char(ch, "You must place an item to use as the mold pattern, a "
                       "crystal and your crafting resource materials in the kit and "
                       "then type 'checkcraft'\r\n");
    else if (CMD_IS("convert"))
      send_to_char(ch, "You must place exact multiples of 10, of a material "
                       "to being the conversion process.\r\n");
    else if (CMD_IS("disenchant"))
      send_to_char(ch, "You must place the item you want to disenchant "
                       "in the kit.\r\n");
    else
      send_to_char(ch, "Unrecognized crafting-kit command!\r\n");
    return 1;
  }

  if (kit->carried_by != ch)
  {
    send_to_char(ch, "You must be holding your kit to perform any crafting tasks.\r\n");
    return 1;
  }

  if (CMD_IS("resize"))
    return resize(argument, kit, ch);
  if (CMD_IS("bonearmor"))
    return bonearmor(argument, kit, ch);
  if (CMD_IS("reforge"))
    return reforge(argument, kit, ch);
  else if (CMD_IS("restring"))
    return restring(argument, kit, ch);
  else if (CMD_IS("redesc"))
    return redesc(argument, kit, ch);
  else if (CMD_IS("augment"))
    return augment(kit, ch);
  else if (CMD_IS("convert"))
    return convert(kit, ch);
  else if (CMD_IS("autocraft"))
    return autocraft(kit, ch);
  else if (CMD_IS("create"))
    return create(argument, kit, ch, CREATE_MODE_CREATE);
  else if (CMD_IS("checkcraft"))
    return create(NULL, kit, ch, CREATE_MODE_CHECK);
  else if (CMD_IS("disenchant"))
    return disenchant(kit, ch);
  else
  {
    send_to_char(ch, "Invalid command.\r\n");
    return 0;
  }
  return 0;
}

/* here is our room-spec for crafting quest */
SPECIAL(crafting_quest)
{
  char desc[MAX_INPUT_LENGTH] = {'\0'};
  char arg[MAX_INPUT_LENGTH] = {'\0'}, arg2[MAX_INPUT_LENGTH] = {'\0'};
  int roll = 0;
#if defined(CAMPAIGN_DL)
  int avail = 0;
#endif

  if (!CMD_IS("supplyorder"))
  {
    return 0;
  }

  if (IS_NPC(ch))
  {
    send_to_char(ch, "Mobiles can't craft.\r\n");
    return 1;
  }

  two_arguments(argument, arg, sizeof(arg), arg2, sizeof(arg2));

  if (!*arg)
    cquest_report(ch);
  else if (!strcmp(arg, "new"))
  {
    if (GET_AUTOCQUEST_VNUM(ch) && GET_AUTOCQUEST_MAKENUM(ch) <= 0)
    {
      send_to_char(ch, "You can't take a new supply order until you've "
                       "handed in the one you've completed (supplyorder complete).\r\n");
      return 1;
    }
#if defined(CAMPAIGN_DL)
    avail = get_mysql_supply_orders_available(ch);

    if (avail <= 0)
    {
      send_to_char(ch, "You must wait until tomorrow to perform more supply orders.\r\n");
      return 1;
    }
#endif

    /* initialize values */
    reset_acraft(ch);
    GET_AUTOCQUEST_VNUM(ch) = AUTOCQUEST_VNUM;

    switch (dice(1, 5))
    {
    case 1:
      snprintf(desc, sizeof(desc), "a shield");
      GET_AUTOCQUEST_MATERIAL(ch) = MATERIAL_WOOD;
      break;
    case 2:
      snprintf(desc, sizeof(desc), "a sword");
      GET_AUTOCQUEST_MATERIAL(ch) = MATERIAL_STEEL;
      break;
    case 3:
      if ((roll = dice(1, 7)) == 1)
      {
        snprintf(desc, sizeof(desc), "a necklace");
        GET_AUTOCQUEST_MATERIAL(ch) = MATERIAL_COPPER;
      }
      else if (roll == 2)
      {
        snprintf(desc, sizeof(desc), "a bracer");
        GET_AUTOCQUEST_MATERIAL(ch) = MATERIAL_COPPER;
      }
      else if (roll == 3)
      {
        snprintf(desc, sizeof(desc), "a cloak");
        GET_AUTOCQUEST_MATERIAL(ch) = MATERIAL_WOOL;
      }
      else if (roll == 4)
      {
        snprintf(desc, sizeof(desc), "a cape");
        GET_AUTOCQUEST_MATERIAL(ch) = MATERIAL_HEMP;
      }
      else if (roll == 5)
      {
        snprintf(desc, sizeof(desc), "a belt");
        GET_AUTOCQUEST_MATERIAL(ch) = MATERIAL_BURLAP;
      }
      else if (roll == 6)
      {
        snprintf(desc, sizeof(desc), "a pair of gloves");
        GET_AUTOCQUEST_MATERIAL(ch) = MATERIAL_COTTON;
      }
      else
      {
        snprintf(desc, sizeof(desc), "a pair of boots");
        GET_AUTOCQUEST_MATERIAL(ch) = MATERIAL_LEATHER;
      }
      break;
    case 4:
      if ((roll = dice(1, 2)) == 1)
      {
        snprintf(desc, sizeof(desc), "a suit of ringmail");
        GET_AUTOCQUEST_MATERIAL(ch) = MATERIAL_IRON;
      }
      else
      {
        snprintf(desc, sizeof(desc), "a cloth robe");
        GET_AUTOCQUEST_MATERIAL(ch) = MATERIAL_SATIN;
      }
      break;
    default:
      snprintf(desc, sizeof(desc), "some war supplies");
      GET_AUTOCQUEST_MATERIAL(ch) = MATERIAL_BRONZE;
      break;
    }

    GET_AUTOCQUEST_DESC(ch) = strdup(desc);
    GET_AUTOCQUEST_MAKENUM(ch) = AUTOCQUEST_MAKENUM;
#if defined(CAMPAIGN_DL)
    GET_AUTOCQUEST_QP(ch) = MAX(5, GET_LEVEL(ch) / 2);
#else
    if (!rand_number(0, 20))
      GET_AUTOCQUEST_QP(ch) = 1;
    else
      GET_AUTOCQUEST_QP(ch) = 0;
#endif
    if (GET_LEVEL(ch) <= 5)
    {
      GET_AUTOCQUEST_GOLD(ch) = 50;
      GET_AUTOCQUEST_EXP(ch) = 100;
    }
    else if (GET_LEVEL(ch) <= 10)
    {
      GET_AUTOCQUEST_GOLD(ch) = 100;
      GET_AUTOCQUEST_EXP(ch) = 200;
    }
    else if (GET_LEVEL(ch) <= 15)
    {
      GET_AUTOCQUEST_GOLD(ch) = 300;
      GET_AUTOCQUEST_EXP(ch) = 400;
    }
    else if (GET_LEVEL(ch) <= 20)
    {
      GET_AUTOCQUEST_GOLD(ch) = 500;
      GET_AUTOCQUEST_EXP(ch) = 800;
    }
    else if (GET_LEVEL(ch) <= 25)
    {
      GET_AUTOCQUEST_GOLD(ch) = 800;
      GET_AUTOCQUEST_EXP(ch) = 1000;
    }
    else
    {
      GET_AUTOCQUEST_GOLD(ch) = 1000;
      GET_AUTOCQUEST_EXP(ch) = 1500;
    };

    send_to_char(ch, "You have been commissioned for a supply order to "
                     "make %s.  We expect you to make %d before you can collect your "
                     "reward.  Good luck!  Once completed you will receive the "
                     "following:  You will receive %d quest points."
                     "  %d gold will be given to you.  You will receive %d "
                     "experience points.\r\n",
                 desc, GET_AUTOCQUEST_MAKENUM(ch), GET_AUTOCQUEST_QP(ch),
                 GET_AUTOCQUEST_GOLD(ch), GET_AUTOCQUEST_EXP(ch));
#if defined(CAMPAIGN_DL)
    put_mysql_supply_orders_available(ch, --avail);
#endif

  }
  else if (!strcmp(arg, "complete"))
  {
    if (GET_AUTOCQUEST_VNUM(ch) && GET_AUTOCQUEST_MAKENUM(ch) <= 0)
    {
      send_to_char(ch, "You have completed your supply order contract"
                       " for %s.\r\n"
                       "You receive %d reputation points.\r\n"
                       "%d gold has been given to you.\r\n"
                       "You receive %d experience points.\r\n",
                   GET_AUTOCQUEST_DESC(ch), GET_AUTOCQUEST_QP(ch),
                   GET_AUTOCQUEST_GOLD(ch), GET_AUTOCQUEST_EXP(ch));
      GET_QUESTPOINTS(ch) += GET_AUTOCQUEST_QP(ch);
      GET_GOLD(ch) += GET_AUTOCQUEST_GOLD(ch);
      GET_EXP(ch) += GET_AUTOCQUEST_EXP(ch);

      reset_acraft(ch);
    }
    else
      cquest_report(ch);
  }
  else if (!strcmp(arg, "quit"))
  {
    send_to_char(ch, "You abandon your supply order to make %d %s.\r\n",
                 GET_AUTOCQUEST_MAKENUM(ch), GET_AUTOCQUEST_DESC(ch));
    reset_acraft(ch);
  }
  else
    cquest_report(ch);

  return 1;
}

/* the event driver for crafting */
EVENTFUNC(event_crafting)
{
  struct char_data *ch;
  struct mud_event_data *pMudEvent;
  struct obj_data *obj2 = NULL;
  char buf[MAX_INPUT_LENGTH] = {'\0'}, buf2[MAX_INPUT_LENGTH] = {'\0'};
  int exp = 0;
  int skill = -1, roll = -1;

  // initialize everything and dummy checks
  if (event_obj == NULL)
    return 0;
  pMudEvent = (struct mud_event_data *)event_obj;
  ch = (struct char_data *)pMudEvent->pStruct;

  if (!ch || !ch->desc)
    return 0;
  if (!IS_NPC(ch) && !IS_PLAYING(ch->desc))
    return 0;

  if (GET_CRAFTING_TYPE(ch) == SCMD_DISENCHANT)
  {
    ; /* disenchant is unique - we do not bring an object along */
  }
  else if (!GET_AUTOCQUEST_VNUM(ch) && GET_CRAFTING_OBJ(ch) == NULL)
  {
    log("SYSERR: crafting - null object");
    return 0;
  }
  if (GET_CRAFTING_TYPE(ch) == 0)
  {
    log("SYSERR: crafting - invalid type");
    return 0;
  }

  if (FIGHTING(ch))
  {
    send_to_char(ch, "You are too busy fighting to do continue!\r\n");
    return 0;
  }

  if (GET_CRAFTING_TICKS(ch))
  { /* still working! */

    /* disenchant.   disenchant has no OBJ so we handle separate */
    if (GET_CRAFTING_TYPE(ch) == SCMD_DISENCHANT)
    {
      send_to_char(ch, "You continue to %s.\r\n",
                   craft_type[GET_CRAFTING_TYPE(ch)]);
      exp = 10 * GET_LEVEL(ch) + GET_LEVEL(ch);

      /* should be everything that is not disenchant/supplyorder */
    }
    else if (GET_CRAFTING_OBJ(ch))
    {
      send_to_char(ch, "You continue to %s and work to create %s.\r\n",
                   craft_type[GET_CRAFTING_TYPE(ch)],
                   GET_CRAFTING_OBJ(ch)->short_description);
      exp = GET_OBJ_LEVEL(GET_CRAFTING_OBJ(ch)) * GET_LEVEL(ch) + GET_LEVEL(ch);

      /* supply orders */
    }
    else
    {
      send_to_char(ch, "You continue your supply order for %s.\r\n",
                   GET_AUTOCQUEST_DESC(ch));
      exp = GET_LEVEL(ch) * 2;
    }

    if (GET_CRAFTING_TYPE(ch) == SCMD_RESIZE)
      exp = 0;
    if (exp > 0)
    {
      gain_exp(ch, exp, GAIN_EXP_MODE_CRAFT);
      send_to_char(ch, "You gained %d exp for crafting...\r\n", exp);
    }
    send_to_char(ch, "You have approximately %d seconds "
                     "left to go.\r\n",
                 GET_CRAFTING_TICKS(ch) * 6);

    GET_CRAFTING_TICKS(ch)--;

    /* skill notch */
    if (GET_SKILL(ch, SKILL_FAST_CRAFTER) < 99)
      increase_skill(ch, SKILL_FAST_CRAFTER);

    if (GET_LEVEL(ch) >= LVL_IMMORT)
      return 1;
    else
      return (6 * PASSES_PER_SEC); // come back in x time to the event
  }
  else
  { /* should be completed */

    switch (GET_CRAFTING_TYPE(ch))
    {

    case SCMD_RESIZE:
      // no skill association
      snprintf(buf, sizeof(buf), "You resize $p.");
      act(buf, false, ch, GET_CRAFTING_OBJ(ch), 0, TO_CHAR);
      snprintf(buf, sizeof(buf), "$n resizes $p.");
      act(buf, false, ch, GET_CRAFTING_OBJ(ch), 0, TO_ROOM);

      /* resize system check point -Zusuk */
      autoquest_trigger_check(ch, NULL, NULL, 0, AQ_CRAFT_RESIZE);

      break;

    case SCMD_BONEARMOR:
      skill = SKILL_ARMOR_SMITHING;
      snprintf(buf, sizeof(buf), "You finish converting $p into bone.");
      act(buf, false, ch, GET_CRAFTING_OBJ(ch), 0, TO_CHAR);
      snprintf(buf, sizeof(buf), "$n finishes converting $p into bone.");
      act(buf, false, ch, GET_CRAFTING_OBJ(ch), 0, TO_ROOM);

      /* resize system check point -Zusuk */
      autoquest_trigger_check(ch, NULL, NULL, 0, AQ_CRAFT_RESIZE);
      break;

      case SCMD_REFORGE:
      if (GET_OBJ_TYPE(GET_CRAFTING_OBJ(ch)) == ITEM_WEAPON)
        skill = SKILL_WEAPON_SMITHING;
      else
        skill = SKILL_ARMOR_SMITHING;

      snprintf(buf, sizeof(buf), "You finish reforging $p. It is recommended you use the restring command on this object.");
      act(buf, false, ch, GET_CRAFTING_OBJ(ch), 0, TO_CHAR);
      snprintf(buf, sizeof(buf), "$n finishes reforging $p. It is recommended you use the restring command on this object.");
      act(buf, false, ch, GET_CRAFTING_OBJ(ch), 0, TO_ROOM);

      /* resize system check point -Zusuk */
      autoquest_trigger_check(ch, NULL, NULL, 0, AQ_CRAFT_RESIZE);

      break;

    case SCMD_DIVIDE:
      // no skill association
      snprintf(buf, sizeof(buf), "You create $p (x%d).",
               GET_CRAFTING_REPEAT(ch));
      act(buf, false, ch, GET_CRAFTING_OBJ(ch), 0, TO_CHAR);
      snprintf(buf, sizeof(buf), "$n creates $p (x%d).", GET_CRAFTING_REPEAT(ch));
      act(buf, false, ch, GET_CRAFTING_OBJ(ch), 0, TO_ROOM);

      int i = 0;
      for (i = 1; i < GET_CRAFTING_REPEAT(ch); i++)
      {
        obj2 = read_object(GET_OBJ_VNUM(GET_CRAFTING_OBJ(ch)), VIRTUAL);
        obj_to_char(obj2, ch);
      }

      /* divide system check point -Zusuk */
      autoquest_trigger_check(ch, NULL, NULL, 0, AQ_CRAFT_DIVIDE);

      break;

    case SCMD_MINE:
      skill = SKILL_MINING;

      snprintf(buf, sizeof(buf), "Your efforts in the area result in: $p.");
      act(buf, false, ch, GET_CRAFTING_OBJ(ch), 0, TO_CHAR);
      snprintf(buf, sizeof(buf), "$n's efforts in the area result in: $p.");
      act(buf, false, ch, GET_CRAFTING_OBJ(ch), 0, TO_ROOM);

      /* mine system check point -Zusuk */
      autoquest_trigger_check(ch, NULL, NULL, 0, AQ_CRAFT_MINE);

      break;

    case SCMD_HUNT:
      skill = SKILL_FORESTING;

      snprintf(buf, sizeof(buf), "Your efforts in the area result in: $p.");
      act(buf, false, ch, GET_CRAFTING_OBJ(ch), 0, TO_CHAR);
      snprintf(buf, sizeof(buf), "$n's efforts in the area result in: $p.");
      act(buf, false, ch, GET_CRAFTING_OBJ(ch), 0, TO_ROOM);

      /* hunt system check point -Zusuk */
      autoquest_trigger_check(ch, NULL, NULL, 0, AQ_CRAFT_HUNT);

      break;

    case SCMD_KNIT:
      skill = SKILL_KNITTING;

      snprintf(buf, sizeof(buf), "Your efforts in the area result in: $p.");
      act(buf, false, ch, GET_CRAFTING_OBJ(ch), 0, TO_CHAR);
      snprintf(buf, sizeof(buf), "$n's efforts in the area result in: $p.");
      act(buf, false, ch, GET_CRAFTING_OBJ(ch), 0, TO_ROOM);

      /* knit system check point -Zusuk */
      autoquest_trigger_check(ch, NULL, NULL, 0, AQ_CRAFT_KNIT);

      break;

    case SCMD_FOREST:
      skill = SKILL_FORESTING;

      snprintf(buf, sizeof(buf), "Your efforts in the area result in: $p.");
      act(buf, false, ch, GET_CRAFTING_OBJ(ch), 0, TO_CHAR);
      snprintf(buf, sizeof(buf), "$n's efforts in the area result in: $p.");
      act(buf, false, ch, GET_CRAFTING_OBJ(ch), 0, TO_ROOM);

      /* foresting system check point -Zusuk */
      autoquest_trigger_check(ch, NULL, NULL, 0, AQ_CRAFT_FOREST);

      break;

    case SCMD_DISENCHANT:
      skill = SKILL_CHEMISTRY;

      snprintf(buf, sizeof(buf), "You complete the disenchantment process.");
      act(buf, false, ch, 0, 0, TO_CHAR);
      snprintf(buf, sizeof(buf), "$n finishes the disenchanting process.");
      act(buf, false, ch, 0, 0, TO_ROOM);

      /* disenchant system check point -Zusuk */
      autoquest_trigger_check(ch, NULL, NULL, 0, AQ_CRAFT_DISENCHANT);

      break;

    case SCMD_SYNTHESIZE:
      // synthesizing here, incomplete

      /* syntheize system check point -Zusuk */
      // autoquest_trigger_check(ch, NULL, NULL, 0, AQ_CRAFT_SYNTHESIZE);

      break;

    case SCMD_CRAFT:
      if (GET_CRAFTING_REPEAT(ch))
      {
        snprintf(buf2, sizeof(buf2), " (x%d)", GET_CRAFTING_REPEAT(ch) + 1);
        for (i = 0; i < MAX(0, GET_CRAFTING_REPEAT(ch)); i++)
        {
          obj2 = GET_CRAFTING_OBJ(ch);
          obj_to_char(obj2, ch);
        }
        GET_CRAFTING_REPEAT(ch) = 0;
      }
      else
        snprintf(buf2, sizeof(buf2), "\tn");

      snprintf(buf, sizeof(buf), "You create $p%s.", buf2);
      act(buf, false, ch, GET_CRAFTING_OBJ(ch), 0, TO_CHAR);
      snprintf(buf, sizeof(buf), "$n creates $p%s.", buf2);
      act(buf, false, ch, GET_CRAFTING_OBJ(ch), 0, TO_ROOM);
      /*
        if (GET_GOLD(ch) < (GET_OBJ_COST(GET_CRAFTING_OBJ(ch)) / 4)) {
          GET_BANK_GOLD(ch) -= GET_OBJ_COST(GET_CRAFTING_OBJ(ch)) / 4;
        } else {
          GET_GOLD(ch) -= GET_OBJ_COST(GET_CRAFTING_OBJ(ch)) / 4;
        }
         */

      /* autoquest system check point -Zusuk */
      autoquest_trigger_check(ch, NULL, NULL, 0, AQ_CRAFT);

      break;

    case SCMD_AUGMENT:
      // use to be part of crafting
      skill = SKILL_CHEMISTRY;

      if (GET_CRAFTING_REPEAT(ch))
      {
        snprintf(buf2, sizeof(buf2), " (x%d)", GET_CRAFTING_REPEAT(ch) + 1);
        for (i = 0; i < MAX(0, GET_CRAFTING_REPEAT(ch)); i++)
        {
          obj2 = GET_CRAFTING_OBJ(ch);
          obj_to_char(obj2, ch);
        }
        GET_CRAFTING_REPEAT(ch) = 0;
      }
      else
        snprintf(buf2, sizeof(buf2), "\tn");

      snprintf(buf, sizeof(buf), "You augment $p%s.", buf2);
      act(buf, false, ch, GET_CRAFTING_OBJ(ch), 0, TO_CHAR);
      snprintf(buf, sizeof(buf), "$n augments $p%s.", buf2);
      act(buf, false, ch, GET_CRAFTING_OBJ(ch), 0, TO_ROOM);

      /* augment system check point -Zusuk */
      autoquest_trigger_check(ch, NULL, NULL, 0, AQ_CRAFT_AUGMENT);

      break;

    case SCMD_CONVERT:
      skill = SKILL_CHEMISTRY;
      // use to be part of crafting

      if (GET_CRAFTING_REPEAT(ch))
      {
        snprintf(buf2, sizeof(buf2), " (x%d)", GET_CRAFTING_REPEAT(ch) + 1);
        for (i = 0; i < MAX(0, GET_CRAFTING_REPEAT(ch)); i++)
        {
          obj2 = GET_CRAFTING_OBJ(ch);
          obj_to_char(obj2, ch);
        }
        GET_CRAFTING_REPEAT(ch) = 0;
      }
      else
        snprintf(buf2, sizeof(buf2), "\tn");

      snprintf(buf, sizeof(buf), "You convert $p%s.", buf2);
      act(buf, false, ch, GET_CRAFTING_OBJ(ch), 0, TO_CHAR);
      snprintf(buf, sizeof(buf), "$n converts $p%s.", buf2);
      act(buf, false, ch, GET_CRAFTING_OBJ(ch), 0, TO_ROOM);

      /* convert system check point -Zusuk */
      autoquest_trigger_check(ch, NULL, NULL, 0, AQ_CRAFT_CONVERT);

      break;

    case SCMD_RESTRING:
      // no skill association
      snprintf(buf2, sizeof(buf2), "\tn");
      snprintf(buf, sizeof(buf), "You rename $p%s.", buf2);
      act(buf, false, ch, GET_CRAFTING_OBJ(ch), 0, TO_CHAR);
      snprintf(buf, sizeof(buf), "$n renames $p%s.", buf2);
      act(buf, false, ch, GET_CRAFTING_OBJ(ch), 0, TO_ROOM);

      /* hunt system check point -Zusuk */
      autoquest_trigger_check(ch, NULL, NULL, 0, AQ_CRAFT_RESTRING);

      break;

    case SCMD_REDESC:
      // no skill association
      snprintf(buf2, sizeof(buf2), "\tn");
      snprintf(buf, sizeof(buf), "You redesc $p%s.", buf2);
      act(buf, false, ch, GET_CRAFTING_OBJ(ch), 0, TO_CHAR);
      snprintf(buf, sizeof(buf), "$n redescs $p%s.", buf2);
      act(buf, false, ch, GET_CRAFTING_OBJ(ch), 0, TO_ROOM);

      break;

    case SCMD_SUPPLYORDER:
      /* picking a random trade to notch */
      roll = dice(1, 10);
      switch (roll)
      {
      case 1:
        skill = SKILL_ARMOR_SMITHING;
        break;
      case 2:
        skill = SKILL_WEAPON_SMITHING;
        break;
      case 3:
        skill = SKILL_JEWELRY_MAKING;
        break;
      case 4:
        skill = SKILL_CHEMISTRY;
        break;
      case 5:
        skill = SKILL_BONE_ARMOR;
        break;
      case 6:
        skill = SKILL_ELVEN_CRAFTING;
        break;
      case 7:
        skill = SKILL_MASTERWORK_CRAFTING;
        break;
      case 8:
        skill = SKILL_DRACONIC_CRAFTING;
        break;
      case 9:
        skill = SKILL_DWARVEN_CRAFTING;
        break;

      default:
        skill = SKILL_LEATHER_WORKING;
        break;
      }
      GET_AUTOCQUEST_MAKENUM(ch)
      --;
      if (GET_AUTOCQUEST_MAKENUM(ch) <= 0)
      {
        snprintf(buf, sizeof(buf), "$n completes an item for a supply order.");
        act(buf, false, ch, NULL, 0, TO_ROOM);
        send_to_char(ch, "You have completed your supply order! Go turn"
                         " it in for more exp, quest points and "
                         "gold!\r\n");

        /* autoquest system check point -Zusuk */
        autoquest_trigger_check(ch, NULL, NULL, 0, AQ_AUTOCRAFT);
      }
      else
      {
        snprintf(buf, sizeof(buf), "$n completes a supply order.");
        act(buf, false, ch, NULL, 0, TO_ROOM);
        send_to_char(ch, "You have completed another item in your supply "
                         "order and have %d more to make.\r\n",
                     GET_AUTOCQUEST_MAKENUM(ch));
      }
      break;
    default:
      log("SYSERR: crafting - unsupported SCMD_");
      return 0;
    }

    /* notch skills */
    if (skill != -1)
      increase_skill(ch, skill);
    reset_craft(ch);
    return 0; // done with the event
  }
  log("SYSERR: crafting, crafting_event end");
  return 0;
}

/* the 'harvest' command */
ACMD(do_harvest)
{
  struct obj_data *obj = NULL, *node = NULL;
  int roll = 0, material = -1, minskill = 0;
  int skillnum = 0;
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  char buf[MEDIUM_STRING] = {'\0'};
  int sub_command = SCMD_CRAFT_UNDF;

  if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
  {
    send_to_char(ch, "You must drop something before you can harvest anything else.\r\n");
    return;
  }
  if (IS_CARRYING_W(ch) >= CAN_CARRY_W(ch))
  {
    send_to_char(ch, "You must lighten your load before you can harvest anything else.\r\n");
    return;
  }

  if (FIGHTING(ch))
  {
    send_to_char(ch, "You are too busy fighting!\r\n");
    return;
  }

  one_argument(argument, arg, sizeof(arg));

  if (!*arg)
  {
    send_to_char(ch, "You need to specify what you want to harvest.\r\n");
    return;
  }

  if (!(node = get_obj_in_list_vis(ch, arg, NULL, world[IN_ROOM(ch)].contents)))
  {
    send_to_char(ch, "That doesn't seem to be present in this room.\r\n");
    return;
  }

  if (GET_OBJ_VNUM(node) != HARVESTING_NODE)
  {
    send_to_char(ch, "That is not a harvesting node.\r\n");
    return;
  }

  material = GET_OBJ_MATERIAL(node);

  if (IS_WOOD(material))
  {
    skillnum = SKILL_FORESTING;
    sub_command = SCMD_FOREST;
  }
  else if (IS_LEATHER(material))
  {
    skillnum = SKILL_HUNTING;
    sub_command = SCMD_HUNT;
  }
  else if (IS_CLOTH(material))
  {
    skillnum = SKILL_KNITTING;
    sub_command = SCMD_KNIT;
  }
  else
  {
    skillnum = SKILL_MINING;
    sub_command = SCMD_MINE;
  }

  switch (material)
  {

  case MATERIAL_STEEL:
    roll = dice(1, 100);
    if (roll <= 40)
      obj = read_object(BRONZE_MATERIAL, VIRTUAL); // bronze
    else if (roll <= 75)
      obj = read_object(IRON_MATERIAL, VIRTUAL); // iron
    else if (roll <= 96)
      obj = read_object(STEEL_MATERIAL, VIRTUAL); // steel
    else if (roll <= 98)
      obj = read_object(ONYX_MATERIAL, VIRTUAL); // onyx
    else
      obj = read_object(OBSIDIAN_MATERIAL, VIRTUAL); // obsidian
    minskill = 1;
    break;

  case MATERIAL_COLD_IRON:
    roll = dice(1, 100);
    if (roll <= 48)
      obj = read_object(COLD_IRON_MATERIAL, VIRTUAL); // cold iron
    else if (roll <= 52)
      obj = read_object(ONYX_MATERIAL, VIRTUAL); // onyx
    else
      obj = read_object(IRON_MATERIAL, VIRTUAL); // iron
    minskill = 35;
    break;

  case MATERIAL_MITHRIL:
    roll = dice(1, 100);
    if (roll <= 48)
      obj = read_object(MITHRIL_MATERIAL, VIRTUAL); // mithril
    else if (roll <= 96)
      obj = read_object(MITHRIL_MATERIAL, VIRTUAL); // mithril
    else if (roll <= 98)
      obj = read_object(RUBY_MATERIAL, VIRTUAL); // ruby
    else
      obj = read_object(SAPPHIRE_MATERIAL, VIRTUAL); // sapphire
    minskill = 48;
    break;

  case MATERIAL_ADAMANTINE:
    roll = dice(1, 100);
    if (roll <= 4)
      obj = read_object(ADAMANTINE_MATERIAL, VIRTUAL); // adamantine
    else if (roll <= 96)
      obj = read_object(PLATINUM_MATERIAL, VIRTUAL); // platinum
    else
    {
      if (dice(1, 2) % 2 == 0)
        obj = read_object(DIAMOND_MATERIAL, VIRTUAL); // diamond
      else
        obj = read_object(EMERALD_MATERIAL, VIRTUAL); // emerald
    }
    minskill = 61;
    break;

  case MATERIAL_SILVER:
    roll = dice(1, 10);
    if (roll <= (8))
    {
      roll = dice(1, 100);
      if (roll <= 48)
        obj = read_object(COPPER_MATERIAL, VIRTUAL); // copper
      else if (roll <= 96)
        obj = read_object(ALCHEMAL_SILVER_MATERIAL, VIRTUAL); // alchemal silver
      else if (roll <= 98)
        obj = read_object(ONYX_MATERIAL, VIRTUAL); // onyx
      else
        obj = read_object(OBSIDIAN_MATERIAL, VIRTUAL); // obsidian
    }
    else
    {
      roll = dice(1, 100);
      if (roll <= 48)
        obj = read_object(SILVER_MATERIAL, VIRTUAL); // silver
      else if (roll <= 52)
        obj = read_object(ONYX_MATERIAL, VIRTUAL); // onyx
      else
        obj = read_object(SILVER_MATERIAL, VIRTUAL); // silver
    }
    minskill = 1;
    break;

  case MATERIAL_GOLD:
    roll = dice(1, 10);
    if (roll <= (8))
    {
      roll = dice(1, 100);
      if (roll <= 48)
        obj = read_object(GOLD_MATERIAL, VIRTUAL); // gold
      else if (roll <= 96)
        obj = read_object(GOLD_MATERIAL, VIRTUAL); // gold
      else if (roll <= 98)
        obj = read_object(RUBY_MATERIAL, VIRTUAL); // ruby
      else
        obj = read_object(SAPPHIRE_MATERIAL, VIRTUAL); // sapphire
    }
    else
    {
      roll = dice(1, 100);
      if (roll <= 4)
        obj = read_object(PLATINUM_MATERIAL, VIRTUAL); // platinum
      else if (roll <= 96)
        obj = read_object(PLATINUM_MATERIAL, VIRTUAL); // platinum
      else
      {
        if (dice(1, 2) % 2 == 0)
          obj = read_object(DIAMOND_MATERIAL, VIRTUAL); // diamond
        else
          obj = read_object(EMERALD_MATERIAL, VIRTUAL); // emerald
      }
    }
    minskill = 30;
    break;

  case MATERIAL_WOOD:
    roll = dice(1, 100);
    if (roll <= (80))
    {
      if (dice(1, 100) <= 96)
        obj = read_object(ALDERWOOD_MATERIAL, VIRTUAL); // alderwood
      else
        obj = read_object(FOS_BIRD_MATERIAL, VIRTUAL); // fossilized bird egg
    }
    else if (roll <= (94))
    {
      if (dice(1, 100) <= 96)
        obj = read_object(YEW_MATERIAL, VIRTUAL); // yew
      else
        obj = read_object(FOS_LIZARD_MATERIAL, VIRTUAL); // fossilized giant lizard egg
    }
    else
    {
      if (dice(1, 100) <= 96)
        obj = read_object(OAK_MATERIAL, VIRTUAL); // oak
      else
        obj = read_object(FOS_WYVERN_MATERIAL, VIRTUAL); // fossilized wyvern egg
    }
    minskill = 1;
    break;

  case MATERIAL_DARKWOOD:
    if (dice(1, 100) <= 96)
      obj = read_object(DARKWOOD_MATERIAL, VIRTUAL); // darkwood
    else
      obj = read_object(FOS_DRAGON_MATERIAL, VIRTUAL); // fossilized dragon egg
    minskill = 38;
    break;

  case MATERIAL_LEATHER:
    roll = dice(1, 100);
    if (roll <= (82))
    {
      if (dice(1, 100) <= 96)
      {
        obj = read_object(LEATHER_LQ_MATERIAL, VIRTUAL); // low quality hide
      }
      else
        obj = read_object(FOS_BIRD_MATERIAL, VIRTUAL); // fossilized bird egg
    }
    else if (roll <= (94))
    {
      if (dice(1, 10) <= 96)
      {
        obj = read_object(LEATHER_MQ_MATERIAL, VIRTUAL); // medium quality hide
      }
      else
        obj = read_object(FOS_LIZARD_MATERIAL, VIRTUAL); // fossilized giant lizard egg
    }
    else
    {
      if (dice(1, 100) <= 96)
      {
        obj = read_object(LEATHER_HQ_MATERIAL, VIRTUAL); // high quality hide
      }
      else
        obj = read_object(FOS_WYVERN_MATERIAL, VIRTUAL); // fossilized wyvern egg
    }
    minskill = 1;
    break;

  case MATERIAL_DRAGONHIDE:
    if (dice(1, 100) <= 70)
      obj = read_object(LEATHER_HQ_MATERIAL, VIRTUAL); // high quality leather
    else
      obj = read_object(DRAGONHIDE_MATERIAL, VIRTUAL); // dragon hide
    minskill = 58;
    break;

  case MATERIAL_HEMP:
    if (dice(1, 100) <= 96)
      obj = read_object(HEMP_MATERIAL, VIRTUAL); // hemp
    else
      obj = read_object(FOS_BIRD_MATERIAL, VIRTUAL); // fossilized bird egg
    minskill = 1;
    break;

  case MATERIAL_COTTON:
    if (dice(1, 100) <= 96)
    {
      obj = read_object(COTTON_MATERIAL, VIRTUAL); // cotton
    }
    else
      obj = read_object(FOS_LIZARD_MATERIAL, VIRTUAL); // fossilized giant lizard egg
    minskill = 5;
    break;

  case MATERIAL_WOOL:
    if (dice(1, 100) <= 96)
    {
      obj = read_object(WOOL_MATERIAL, VIRTUAL); // wool
    }
    else
      obj = read_object(FOS_LIZARD_MATERIAL, VIRTUAL); // fossilized giant lizard egg
    minskill = 10;
    break;

  case MATERIAL_VELVET:
    if (dice(1, 100) <= 96)
    {
      obj = read_object(VELVET_MATERIAL, VIRTUAL); // velvet
    }
    else
      obj = read_object(FOS_WYVERN_MATERIAL, VIRTUAL); // fossilized wyvern egg
    minskill = 25;
    break;

  case MATERIAL_SATIN:
    if (dice(1, 100) <= 96)
    {
      obj = read_object(SATIN_MATERIAL, VIRTUAL); // satin
    }
    else
      obj = read_object(FOS_WYVERN_MATERIAL, VIRTUAL); // fossilized wyvern egg
    minskill = 31;
    break;

  case MATERIAL_SILK:
    if (dice(1, 100) <= 96)
    {
      if (dice(1, 100) <= 25)
        obj = read_object(VELVET_MATERIAL, VIRTUAL); // velvet
      else if (dice(1, 100) <= 25)
        obj = read_object(SATIN_MATERIAL, VIRTUAL); // satin
      else
        obj = read_object(SILK_MATERIAL, VIRTUAL); // silk
    }
    else
      obj = read_object(FOS_DRAGON_MATERIAL, VIRTUAL); // fossilized dragon egg
    minskill = 38;
    break;

  default:
    send_to_char(ch, "That is not a valid node type, please report this to a staff member [1].\r\n");
    return;
  }

  if (!obj)
  {
    send_to_char(ch, "That is not a valid node type, please report this to a staff member [2].\r\n");
    return;
  }

  if (GET_SKILL(ch, skillnum) < minskill)
  {
    send_to_char(ch, "You need a minimum %s skill of %d, while yours is only %d.\r\n",
                 spell_info[skillnum].name, minskill, GET_SKILL(ch, skillnum));
    return;
  }

  GET_CRAFTING_TYPE(ch) = sub_command;
  GET_CRAFTING_TICKS(ch) = 5;
  GET_CRAFTING_OBJ(ch) = obj;

  // Tell the character they started.
  snprintf(buf, sizeof(buf), "You begin to %s.", CMD_NAME);
  act(buf, FALSE, ch, 0, NULL, TO_CHAR);

  // Tell the room the character started.
  snprintf(buf, sizeof(buf), "$n begins to %s.", CMD_NAME);
  act(buf, FALSE, ch, 0, NULL, TO_ROOM);

  if (node)
    GET_OBJ_VAL(node, 0)
  --;

  if (node && GET_OBJ_VAL(node, 0) <= 0)
  {
    switch (sub_command)
    {
    case SCMD_MINE:
      mining_nodes--;
      break;
    case SCMD_KNIT:
      farming_nodes--;
      break;
    case SCMD_HUNT:
      hunting_nodes--;
      break;
    default:
      foresting_nodes--;
      break;
    }

    // Tell the room the character used up the node
    act("$p has been depleted.", FALSE, 0, node, 0, TO_ROOM);
    obj_from_room(node);
    extract_obj(node);
  }

  obj_to_char(obj, ch);
  NEW_EVENT(eCRAFTING, ch, NULL, 1 * PASSES_PER_SEC);

  if (!IS_NPC(ch))
    increase_skill(ch, skillnum);

  return;
}

int get_mysql_supply_orders_available(struct char_data *ch)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  char buf[MAX_STRING_LENGTH];
  int avail = 0;
  
  mysql_ping(conn);

  snprintf(buf, sizeof(buf), "SELECT supply_orders_available FROM player_supply_orders WHERE player_name='%s'", GET_NAME(ch));

  if (mysql_query(conn, buf))
  {
    log("SYSERR: Unable to SELECT from player_supply_orders: %s", mysql_error(conn));
    return -1;
  }

  if (!(result = mysql_store_result(conn)))
  {
    log("SYSERR: Unable to SELECT from player_supply_orders: %s", mysql_error(conn));
    return -1;
  }

  if (!(row = mysql_fetch_row(result)))
    return 10;

  avail = atoi(row[0]);

  mysql_free_result(result);

  return avail;
}

void put_mysql_supply_orders_available(struct char_data *ch, int avail)
{
  char buf[MAX_STRING_LENGTH];
  
  mysql_ping(conn);

  snprintf(buf, sizeof(buf), "DELETE FROM supply_orders_available WHERE player_name='%s'", GET_NAME(ch));

  mysql_query(conn, buf);

  snprintf(buf, sizeof(buf), "INSERT INTO supply_orders_available (idnum, player_name, supply_orders_available) VALUES(NULL, '%s', '%d')",
          GET_NAME(ch), avail);

  if (mysql_query(conn, buf))
  {
    log("SYSERR: Unable to INSERT INTO player_supply_orders: %s", mysql_error(conn));
  }
}

ACMD(do_need_craft_kit)
{
  send_to_char(ch, "You must be in a room with a crafting station or have a crafting kit in your inventory to perform this action.\r\n");
}