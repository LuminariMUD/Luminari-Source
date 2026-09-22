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
#include "core/sysdep.h"
#include "core/structs.h"
#include "database/mysql.h"
#include "core/utils.h"
#include "core/comm.h"
#include "magic/spells.h"
#include "core/interpreter.h"
#include "core/constants.h"
#include "core/handler.h"
#include "core/db.h"
#include "craft.h"
#include "magic/spells.h"
#include "events/mud_event.h"
#include "core/modify.h" // for parse_at()
#include "obj/treasure.h"
#include "core/mudlim.h"
#include "character/rewards.h"
#include "character/abilities.h"
#include "obj/item.h"
#include "quest/quest.h"
#include "combat/assign_wpn_armor.h"
#include "olc/genolc.h"
#include "olc/genobj.h"
#include "wilderness/resource_system.h"
#include "wilderness/harvest.h"
#include "crafting_new.h"
#include "events/activity_manager.h"
#include "events/actions.h"
#include "events/domain_event_world.h"


/* global variables */
int mining_nodes = 0;
int farming_nodes = 0;
int hunting_nodes = 0;
int foresting_nodes = 0;

static int award_legacy_crafting_experience(struct char_data *ch, int exp);

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
static bool scale_damage(struct char_data *ch, struct obj_data *weapon, int new_size)
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
    if (weapon_damage_a[counter][0] == num_of_dice && weapon_damage_a[counter][1] == size_of_dice)
    {
      /* valid shift in chart?  calculate our new location on chart */
      if (counter + size_shift >= NUM_SIZES || counter + size_shift <= SIZE_RESERVED)
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
    if (weapon_damage_b[counter][0] == num_of_dice && weapon_damage_b[counter][1] == size_of_dice)
    {
      /* valid shift in chart?  calculate our new location on chart */
      if (counter + size_shift >= NUM_SIZES || counter + size_shift <= SIZE_RESERVED)
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
    if (weapon_damage_c[counter][0] == num_of_dice && weapon_damage_c[counter][1] == size_of_dice)
    {
      /* valid shift in chart?  calculate our new location on chart */
      if (counter + size_shift >= NUM_SIZES || counter + size_shift <= SIZE_FINE)
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
static int award_legacy_crafting_experience(struct char_data *ch, int exp)
{
  int gained = award_experience(ch, exp, AWARD_EXP_MODE_CRAFT);

  if (gained > 0)
    send_to_char(ch, "You gained %d exp for crafting...\r\n", gained);

  return gained;
}

#ifdef LUMINARI_CUTEST
int test_award_legacy_crafting_experience(struct char_data *ch, int exp)
{
  return award_legacy_crafting_experience(ch, exp);
}

#endif

/* simple function to reset craft data */
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
static char *node_keywords(int material)
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
  default:
    return strdup("node harvesting");
  }
}

/* this function returns an appropriate short-desc based on material */
static char *node_sdesc(int material)
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
  default:
    return strdup("a harvesting node");
  }
}

/* this function returns an appropriate desc based on material */
static char *node_desc(int material)
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
  default:
    return strdup("A harvesting node is here.  Please inform an imm, this is an error.");
  }
}

/* a function to try and make an intelligent(?) decision
   about what material a harvesting node should be */
static int random_node_material(int allowed)
{
  int rand = 0;

  if (mining_nodes >= (allowed * 2) && foresting_nodes >= allowed && farming_nodes >= allowed &&
      hunting_nodes >= allowed)
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
  room_rnum cnt = 0;
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
    if (dice(1, 33) == 1)
    {
      obj = read_object(HARVESTING_NODE, VIRTUAL);
      if (!obj)
        continue;

      /* Duplicate strings from prototype to avoid double-free */
      if (obj->name)
        obj->name = strdup(obj->name);
      if (obj->short_description)
        obj->short_description = strdup(obj->short_description);
      if (obj->description)
        obj->description = strdup(obj->description);

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
        break;
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
      free_object_string(obj, obj->name);
      obj->name = node_keywords(GET_OBJ_MATERIAL(obj));
      free_object_string(obj, obj->short_description);
      obj->short_description = node_sdesc(GET_OBJ_MATERIAL(obj));
      free_object_string(obj, obj->description);
      obj->description = node_desc(GET_OBJ_MATERIAL(obj));
      obj_to_room(obj, cnt);
    }
  }
}

/*************************/
/* start primary engines */
/*************************/

static struct obj_data *get_single_bone_armor_object(struct obj_data *kit, int *num_objs)
{
  struct obj_data *obj;
  struct obj_data *selected_obj = NULL;

  if (num_objs == NULL)
    return NULL;

  *num_objs = 0;
  if (kit == NULL)
    return NULL;

  for (obj = kit->contains; obj != NULL; obj = obj->next_content)
  {
    if (*num_objs == 0)
      selected_obj = obj;
    (*num_objs)++;
  }

  return *num_objs == 1 ? selected_obj : NULL;
}

static void update_bone_armor_descriptions(struct obj_data *obj, char *argument)
{
  char buf[MAX_STRING_LENGTH];

  if (obj == NULL || argument == NULL)
    return;

  parse_at(argument);

  free_object_string(obj, obj->name);
  obj->name = strdup(argument);
  strip_colors(obj->name);

  free_object_string(obj, obj->short_description);
  obj->short_description = strdup(argument);

  snprintf(buf, sizeof(buf), "%s lies here.", CAP(argument));
  free_object_string(obj, obj->description);
  obj->description = strdup(buf);
}

#ifdef LUMINARI_CUTEST
struct obj_data *test_get_single_bone_armor_object(struct obj_data *kit, int *num_objs)
{
  return get_single_bone_armor_object(kit, num_objs);
}

void test_update_bone_armor_descriptions(struct obj_data *obj, char *argument)
{
  update_bone_armor_descriptions(obj, argument);
}
#endif

/*************************/
/* Kit operations        */
/*************************/

/* Every crafting-kit operation (and the standalone reforge) runs on the activity manager with
 * one lifecycle (consolidation Decision 11): admission plans the work without touching any
 * object, balance, or gold; the activity targets the kit (or the item, for the standalone
 * reforge); completion re-plans against the current state and then applies the mutation,
 * consumption, payment, quest hook, and experience exactly once. Cancelling spends nothing. */

struct kit_operation
{
  int type;       /* SCMD_CRAFT, SCMD_RESIZE, ... */
  int skill;      /* craft ability, or -1 for a skill-less utility */
  int quest;      /* AQ_CRAFT_* hook fired once on completion, or -1 */
  int seconds;    /* work duration */
  int cost;       /* gold paid at completion */
  int legacy_exp; /* the character experience the old tick loop paid, paid once */
  char text[MAX_INPUT_LENGTH];
  int new_size;
  int reforge_index;
  int obj_level;
  int chance_of_crit;
  int essence_level;
  int material;    /* create: the balance material used */
  int mats_needed; /* create: units debited */
};

static struct obj_data *kit_single_item(struct obj_data *kit, int *count)
{
  struct obj_data *obj;

  *count = 0;
  for (obj = kit->contains; obj != NULL; obj = obj->next_content)
    (*count)++;
  return *count == 1 ? kit->contains : NULL;
}

/* The duration of a kit operation whose old timer ran base_ticks of six seconds. */
static int kit_seconds(struct char_data *ch, int skill, int base_ticks, int cost)
{
  if (cost == 0 && base_ticks > 1)
    return 6; /* the old code ran one tick for free work */
  return craft_legacy_kit_seconds(ch, skill, base_ticks);
}

/* The character experience the old tick loop paid per tick, for the whole run. */
static int kit_legacy_exp(int per_tick, int seconds)
{
  return per_tick * MAX(1, seconds / 6);
}

/* Restring cost table (Thazull wanted very cheap at low level for RP fun). */
static int restring_cost(struct obj_data *obj)
{
  int level = GET_OBJ_LEVEL(obj);

  if (level <= 6)
    return 10;
  if (level <= 12)
    return 20 + level;
  if (level <= 16)
    return 40 + level + GET_OBJ_COST(obj) / 6;
  if (level <= 20)
    return 150 + level + GET_OBJ_COST(obj) / 5;
  if (level <= 25)
    return 500 + level + GET_OBJ_COST(obj) / 4;
  return 2000 + level + GET_OBJ_COST(obj) / 2;
}

/* ---- restring ---- */

static bool plan_restring(struct char_data *ch, struct obj_data *kit, const char *argument,
                          struct kit_operation *op, bool verbose)
{
  int count;
  struct obj_data *obj = kit_single_item(kit, &count);

  if (obj == NULL)
  {
    if (verbose)
      send_to_char(ch, "Only one item should be inside the kit.\r\n");
    return false;
  }
  if ((GET_OBJ_TYPE(obj) == ITEM_CONTAINER || GET_OBJ_TYPE(obj) == ITEM_AMMO_POUCH) &&
      obj->contains)
  {
    if (verbose)
      send_to_char(ch, "You cannot restring bags that have items in them.\r\n");
    return false;
  }
  if (GET_OBJ_TYPE(obj) == ITEM_SPELLBOOK)
  {
    if (verbose)
      send_to_char(ch, "You cannot restring spellbooks.\r\n");
    return false;
  }
  if (GET_OBJ_MATERIAL(obj) && !strstr(argument, material_name[GET_OBJ_MATERIAL(obj)]))
  {
    if (verbose)
      send_to_char(ch,
                   "You must include the material name, '%s', in the object "
                   "description somewhere.\r\n",
                   material_name[GET_OBJ_MATERIAL(obj)]);
    return false;
  }
  op->type = SCMD_RESTRING;
  op->skill = -1;
  op->quest = AQ_CRAFT_RESTRING;
  op->cost = restring_cost(obj);
  op->seconds = kit_seconds(ch, -1, 5, op->cost);
  op->legacy_exp = kit_legacy_exp(GET_OBJ_LEVEL(obj) * GET_LEVEL(ch) + GET_LEVEL(ch), op->seconds);
  snprintf(op->text, sizeof(op->text), "%s", argument);
  return true;
}

static void apply_restring(struct char_data *ch, struct obj_data *obj, struct kit_operation *op)
{
  char buf[MAX_INPUT_LENGTH];
  char *text = op->text;

  (void)ch;
  parse_at(text);
  free_object_string(obj, obj->name);
  obj->name = strdup(text);
  strip_colors(obj->name);
  free_object_string(obj, obj->short_description);
  obj->short_description = strdup(text);
  snprintf(buf, sizeof(buf), "%s lies here.", CAP(text));
  free_object_string(obj, obj->description);
  obj->description = strdup(buf);
  if (obj->ex_description)
  {
    struct extra_descr_data *new_descr;

    /* A live object shares its prototype's extra descriptions until changed. */
    if (obj_proto == NULL || !VALID_OBJ_RNUM(obj) ||
        obj->ex_description != obj_proto[GET_OBJ_RNUM(obj)].ex_description)
      free_ex_descriptions(obj->ex_description);
    CREATE(new_descr, struct extra_descr_data, 1);
    new_descr->keyword = strdup(text);
    new_descr->description = strdup("You don't notice any extra details.\n");
    obj->ex_description = new_descr;
  }
}

/* ---- redesc ---- */

static bool plan_redesc(struct char_data *ch, struct obj_data *kit, const char *argument,
                        struct kit_operation *op, bool verbose)
{
  int count;
  struct obj_data *obj = kit_single_item(kit, &count);

  if (obj == NULL)
  {
    if (verbose)
      send_to_char(ch, "Only one item should be inside the kit.\r\n");
    return false;
  }
  if ((GET_OBJ_TYPE(obj) == ITEM_CONTAINER || GET_OBJ_TYPE(obj) == ITEM_AMMO_POUCH) &&
      obj->contains)
  {
    if (verbose)
      send_to_char(ch, "You cannot redesc bags that have items in them.\r\n");
    return false;
  }
  if (GET_OBJ_TYPE(obj) == ITEM_SPELLBOOK)
  {
    if (verbose)
      send_to_char(ch, "You cannot redesc spellbooks.\r\n");
    return false;
  }
  op->type = SCMD_REDESC;
  op->skill = -1;
  op->quest = -1;
  op->cost = restring_cost(obj);
  op->seconds = kit_seconds(ch, -1, 5, op->cost);
  op->legacy_exp = kit_legacy_exp(GET_OBJ_LEVEL(obj) * GET_LEVEL(ch) + GET_LEVEL(ch), op->seconds);
  snprintf(op->text, sizeof(op->text), "%s", argument);
  return true;
}

static void apply_redesc(struct char_data *ch, struct obj_data *obj, struct kit_operation *op)
{
  char buf[MAX_INPUT_LENGTH];
  struct extra_descr_data *new_descr;

  (void)ch;
  parse_at(op->text);
  if (obj->ex_description)
  {
    if (obj_proto == NULL || !VALID_OBJ_RNUM(obj) ||
        obj->ex_description != obj_proto[GET_OBJ_RNUM(obj)].ex_description)
      free_ex_descriptions(obj->ex_description);
  }
  CREATE(new_descr, struct extra_descr_data, 1);
  new_descr->keyword = strdup(obj->name);
  snprintf(buf, sizeof(buf), "%s\n", strfrmt(op->text, 80, 1, FALSE, FALSE, FALSE));
  new_descr->description = strdup(buf);
  obj->ex_description = new_descr;
}

/* ---- resize ---- */

static int parse_size_name(const char *argument)
{
  static const char *names[] = {"fine",  "diminutive", "tiny",       "small",   "medium",
                                "large", "huge",       "gargantuan", "colossal"};
  static const int size_values[] = {SIZE_FINE,  SIZE_DIMINUTIVE, SIZE_TINY,
                                    SIZE_SMALL, SIZE_MEDIUM,     SIZE_LARGE,
                                    SIZE_HUGE,  SIZE_GARGANTUAN, SIZE_COLOSSAL};
  int i;

  if (!argument || !*argument)
    return SIZE_UNDEFINED;
  for (i = 0; i < 9; i++)
    if (is_abbrev(argument, names[i]))
      return size_values[i];
  return SIZE_UNDEFINED;
}

static bool plan_resize(struct char_data *ch, struct obj_data *kit, const char *argument,
                        struct kit_operation *op, bool verbose)
{
  int count;
  struct obj_data *obj = kit_single_item(kit, &count);

  if (obj == NULL)
  {
    if (verbose)
      send_to_char(ch, "Only one item should be inside the kit.\r\n");
    return false;
  }
  op->new_size = parse_size_name(argument);
  if (op->new_size == SIZE_UNDEFINED)
  {
    if (verbose)
      send_to_char(ch, "That is not a valid size: (fine|diminutive|tiny|small|"
                       "medium|large|huge|gargantuan|colossal)\r\n");
    return false;
  }
  if (op->new_size == GET_OBJ_SIZE(obj))
  {
    if (verbose)
      send_to_char(ch, "The object is already the size you desire.\r\n");
    return false;
  }
  op->type = SCMD_RESIZE;
  op->skill = -1;
  op->quest = AQ_CRAFT_RESIZE;
  /* Resizing to your own size is free: no gold, one tick. */
  op->cost = op->new_size == GET_SIZE(ch) ? 0 : GET_OBJ_COST(obj) / 2;
  op->seconds = kit_seconds(ch, -1, 5, op->cost);
  op->legacy_exp = 0;
  return true;
}

/* Resize applies the weapon damage chart first; an invalid shift leaves the item alone. */
static bool apply_resize(struct char_data *ch, struct obj_data *obj, struct kit_operation *op)
{
  int num_dice, size_dice;

  if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
  {
    num_dice = GET_OBJ_VAL(obj, 1);
    size_dice = GET_OBJ_VAL(obj, 2);
    if (!scale_damage(ch, obj, op->new_size))
    {
      send_to_char(ch, "You failed to resize this weapon!\r\n");
      return false;
    }
    send_to_char(ch, "Weapon change:  %dd%d to %dd%d\r\n", num_dice, size_dice, GET_OBJ_VAL(obj, 1),
                 GET_OBJ_VAL(obj, 2));
  }
  GET_OBJ_WEIGHT(obj) += (op->new_size - GET_OBJ_SIZE(obj)) * GET_OBJ_WEIGHT(obj);
  GET_OBJ_SIZE(obj) = op->new_size;
  if (GET_OBJ_WEIGHT(obj) <= 0)
    GET_OBJ_WEIGHT(obj) = 1;
  return true;
}

/* ---- bonearmor ---- */

static bool plan_bonearmor(struct char_data *ch, struct obj_data *kit, const char *argument,
                           struct kit_operation *op, bool verbose)
{
  int count;
  struct obj_data *obj;

  if (!HAS_REAL_FEAT(ch, FEAT_BONE_ARMOR))
  {
    if (verbose)
      send_to_char(ch, "You must have the bone armor feat to convert armor into bone.\r\n");
    return false;
  }
  obj = get_single_bone_armor_object(kit, &count);
  if (count == 0)
  {
    if (verbose)
      send_to_char(ch, "You must place one armor item in the kit.\r\n");
    return false;
  }
  if (count > 1 || obj == NULL)
  {
    if (verbose)
      send_to_char(ch, "Only one item should be inside the kit.\r\n");
    return false;
  }
  if (GET_OBJ_TYPE(obj) != ITEM_ARMOR)
  {
    if (verbose)
      send_to_char(ch, "You can only convert armor and shields to bone.\r\n");
    return false;
  }
  if (GET_OBJ_MATERIAL(obj) == MATERIAL_BONE)
  {
    if (verbose)
      send_to_char(ch, "The object is already made of bone.\r\n");
    return false;
  }
  if (!strstr(argument, material_name[MATERIAL_BONE]))
  {
    if (verbose)
      send_to_char(ch,
                   "You must include the material name, '%s', in the object description "
                   "somewhere.\r\n",
                   material_name[MATERIAL_BONE]);
    return false;
  }
  op->type = SCMD_BONEARMOR;
  op->skill = ABILITY_CRAFT_ARMORSMITHING;
  op->quest = AQ_CRAFT_RESIZE; /* the old completion fired the resize hook */
  op->cost = GET_OBJ_COST(obj) / 3;
  op->seconds = kit_seconds(ch, op->skill, 5, op->cost);
  op->legacy_exp = kit_legacy_exp(GET_OBJ_LEVEL(obj) * GET_LEVEL(ch) + GET_LEVEL(ch), op->seconds);
  snprintf(op->text, sizeof(op->text), "%s", argument);
  return true;
}

static void apply_bonearmor(struct char_data *ch, struct obj_data *obj, struct kit_operation *op)
{
  (void)ch;
  update_bone_armor_descriptions(obj, op->text);
  GET_OBJ_MATERIAL(obj) = MATERIAL_BONE;
  if (GET_OBJ_WEIGHT(obj) <= 0)
    GET_OBJ_WEIGHT(obj) = 1;
}

/* ---- reforge (shared by the kit and the reforge command) ---- */

/* Validate a reforge and choose the target type without touching the item. The station the
 * item's material needs must be in the room; the item must be a weapon, armor, or shield. */
bool reforge_plan(struct char_data *ch, struct obj_data *obj, const char *target, int *index_out,
                  int *cost_out, bool verbose)
{
  int i = 0, skill;

  if (obj == NULL || (GET_OBJ_TYPE(obj) != ITEM_ARMOR && GET_OBJ_TYPE(obj) != ITEM_WEAPON))
  {
    if (verbose)
      send_to_char(ch, "You can only reforge armor, shields and weapons.\r\n");
    return false;
  }
  while (target && *target == ' ')
    target++;
  if (!target || !*target)
  {
    if (verbose)
      send_to_char(ch, "Please specify the type of weapon, armor or shield you'd like to reforge "
                       "this item into. Type weaponlist or armorlistfull to see options.\r\n");
    return false;
  }
  skill = material_type_to_crafting_skill(GET_OBJ_MATERIAL(obj));
  if (!has_crafting_station_in_room(ch, skill))
  {
    if (verbose)
      send_to_char(ch, "You need %s to reforge this item.\r\n", get_crafting_station_name(skill));
    return false;
  }
  if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
  {
    for (i = 1; i < NUM_WEAPON_TYPES; i++)
      if (is_abbrev(target, weapon_list[i].name))
        break;
    if (i >= NUM_WEAPON_TYPES)
    {
      if (verbose)
        send_to_char(ch, "That is not a valid weapon type. Type weaponlist for options.\r\n");
      return false;
    }
    if (i == GET_OBJ_VAL(obj, 0))
    {
      if (verbose)
        send_to_char(ch, "The item is already %s %s.\r\n", AN(weapon_list[i].name),
                     weapon_list[i].name);
      return false;
    }
  }
  else if (IS_SHIELD(GET_OBJ_VAL(obj, 1)))
  {
    for (i = 1; i < NUM_SPEC_ARMOR_TYPES; i++)
      if (IS_SHIELD(i) && is_abbrev(target, armor_list[i].name))
        break;
    if (i >= NUM_SPEC_ARMOR_TYPES)
    {
      if (verbose)
        send_to_char(ch, "That is not a valid shield type. Type armorlistfull for options.\r\n");
      return false;
    }
    if (i == GET_OBJ_VAL(obj, 1))
    {
      if (verbose)
        send_to_char(ch, "The item is already %s %s.\r\n", AN(armor_list[i].name),
                     armor_list[i].name);
      return false;
    }
  }
  else
  {
    for (i = 1; i < NUM_SPEC_ARMOR_TYPES; i++)
    {
      if (IS_SHIELD(i))
        continue;
      if (CAN_WEAR(obj, ITEM_WEAR_HEAD) && armor_list[i].wear != ITEM_WEAR_HEAD)
        continue;
      if (CAN_WEAR(obj, ITEM_WEAR_BODY) && armor_list[i].wear != ITEM_WEAR_BODY)
        continue;
      if (CAN_WEAR(obj, ITEM_WEAR_ARMS) && armor_list[i].wear != ITEM_WEAR_ARMS)
        continue;
      if (CAN_WEAR(obj, ITEM_WEAR_LEGS) && armor_list[i].wear != ITEM_WEAR_LEGS)
        continue;
      if (is_abbrev(target, armor_list[i].name))
        break;
    }
    if (i >= NUM_SPEC_ARMOR_TYPES)
    {
      if (verbose)
        send_to_char(ch, "That is not a valid armor type for this slot. Type armorlistfull for "
                         "options. Wear types must match: a body item becomes another body "
                         "item.\r\n");
      return false;
    }
    if (i == GET_OBJ_VAL(obj, 1))
    {
      if (verbose)
        send_to_char(ch, "The item is already %s %s.\r\n", AN(armor_list[i].name),
                     armor_list[i].name);
      return false;
    }
  }
  if (index_out)
    *index_out = i;
  if (cost_out)
    *cost_out = GET_OBJ_COST(obj) / 2;
  return true;
}

/* The one reforge mutation: type, restored cost, enhancement, and material, then the names.
 * An item with a restring identifier keeps its custom strings with the type word replaced;
 * otherwise it becomes "a reforged <type> (+N)". */
void reforge_apply(struct char_data *ch, struct obj_data *obj, int index)
{
  int orig_cost = GET_OBJ_COST(obj), enhancement = GET_OBJ_VAL(obj, 4);
  int material = GET_OBJ_MATERIAL(obj);
  const char *type_name;
  char buf[MAX_STRING_LENGTH], bonus[30];

  (void)ch;
  if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
    set_weapon_object(obj, index);
  else
  {
    GET_OBJ_VAL(obj, 1) = index;
    set_armor_object(obj, index);
  }
  GET_OBJ_COST(obj) = orig_cost;
  GET_OBJ_VAL(obj, 4) = enhancement;
  if (IS_HARD_METAL(GET_OBJ_MATERIAL(obj)) && IS_HARD_METAL(material))
    GET_OBJ_MATERIAL(obj) = material;
  else if (IS_LEATHER(GET_OBJ_MATERIAL(obj)) && IS_LEATHER(material))
    GET_OBJ_MATERIAL(obj) = material;
  else if (IS_CLOTH(GET_OBJ_MATERIAL(obj)) && IS_CLOTH(material))
    GET_OBJ_MATERIAL(obj) = material;
  else if (IS_WOOD(GET_OBJ_MATERIAL(obj)) && IS_WOOD(material))
    GET_OBJ_MATERIAL(obj) = material;
  type_name = GET_OBJ_TYPE(obj) == ITEM_WEAPON ? weapon_list[GET_OBJ_VAL(obj, 0)].name
                                               : armor_list[GET_OBJ_VAL(obj, 1)].name;
  if (obj->restring_identifier && *obj->restring_identifier)
  {
    char *updated;

    updated =
        obj->name ? replace_substring_ci(obj->name, obj->restring_identifier, type_name) : NULL;
    if (updated)
    {
      free_object_string(obj, obj->name);
      obj->name = updated;
    }
    updated = obj->short_description ? replace_substring_ci(obj->short_description,
                                                            obj->restring_identifier, type_name)
                                     : NULL;
    if (updated)
    {
      free_object_string(obj, obj->short_description);
      obj->short_description = updated;
    }
    updated = obj->description
                  ? replace_substring_ci(obj->description, obj->restring_identifier, type_name)
                  : NULL;
    if (updated)
    {
      free_object_string(obj, obj->description);
      obj->description = updated;
    }
    free(obj->restring_identifier);
    obj->restring_identifier = strdup(type_name);
  }
  else
  {
    if (GET_OBJ_VAL(obj, 4) > 0)
      snprintf(bonus, sizeof(bonus), "(+%d)", GET_OBJ_VAL(obj, 4));
    else
      snprintf(bonus, sizeof(bonus), "(no enchantment bonus)");
    snprintf(buf, sizeof(buf), "a reforged %s %s", type_name, bonus);
    free_object_string(obj, obj->name);
    obj->name = strdup(buf);
    strip_colors(obj->name);
    free_object_string(obj, obj->short_description);
    obj->short_description = strdup(buf);
    snprintf(buf, sizeof(buf), "A reforged %s %s lies here.", type_name, bonus);
    free_object_string(obj, obj->description);
    obj->description = strdup(buf);
  }
  if (GET_OBJ_WEIGHT(obj) <= 0)
    GET_OBJ_WEIGHT(obj) = 1;
}

int reforge_skill(struct obj_data *obj)
{
  return GET_OBJ_TYPE(obj) == ITEM_WEAPON ? ABILITY_CRAFT_WEAPONSMITHING
                                          : ABILITY_CRAFT_ARMORSMITHING;
}

static bool plan_reforge(struct char_data *ch, struct obj_data *kit, const char *argument,
                         struct kit_operation *op, bool verbose)
{
  int count;
  struct obj_data *obj = kit_single_item(kit, &count);

  if (obj == NULL)
  {
    if (verbose)
      send_to_char(ch, "Only one item should be inside the kit.\r\n");
    return false;
  }
  if (!reforge_plan(ch, obj, argument, &op->reforge_index, &op->cost, verbose))
    return false;
  op->type = SCMD_REFORGE;
  op->skill = reforge_skill(obj);
  op->quest = AQ_CRAFT_RESIZE;
  op->seconds = kit_seconds(ch, op->skill, 10, op->cost);
  op->legacy_exp = kit_legacy_exp(GET_OBJ_LEVEL(obj) * GET_LEVEL(ch) + GET_LEVEL(ch), op->seconds);
  snprintf(op->text, sizeof(op->text), "%s", argument);
  return true;
}

/* ---- augment ---- */

static bool plan_augment(struct char_data *ch, struct obj_data *kit, const char *argument,
                         struct kit_operation *op, bool verbose)
{
  struct obj_data *obj, *one = NULL, *two = NULL;
  int count = 0, level_diff;

  (void)argument;
  for (obj = kit->contains; obj != NULL; obj = obj->next_content)
  {
    count++;
    if (GET_OBJ_TYPE(obj) == ITEM_ESSENCE && !one)
      one = obj;
    else if (GET_OBJ_TYPE(obj) == ITEM_ESSENCE && !two)
      two = obj;
  }
  if (count > 2)
  {
    if (verbose)
      send_to_char(ch, "Make sure only two items are in the kit.\r\n");
    return false;
  }
  if (!one || !two)
  {
    if (verbose)
      send_to_char(ch, "You need two essences to augment.\r\n");
    return false;
  }
  if (GET_OBJ_LEVEL(one) >= (LVL_IMMORT - 1) || GET_OBJ_LEVEL(two) >= (LVL_IMMORT - 1))
  {
    if (verbose)
      send_to_char(ch, "You can not further augment that essence!\r\n");
    return false;
  }
  level_diff = abs(GET_OBJ_LEVEL(one) - GET_OBJ_LEVEL(two));
  if (level_diff > 4)
  {
    if (verbose)
      send_to_char(ch, "The essence have to be closer in power (level) to each other!\r\n");
    return false;
  }
  op->essence_level = MAX(GET_OBJ_LEVEL(one), GET_OBJ_LEVEL(two));
  op->skill = ABILITY_CRAFT_ALCHEMY;
  if (op->essence_level > (craft_legacy_skill_equivalent(ch, op->skill) / 3))
  {
    if (verbose)
      send_to_char(ch,
                   "The essence level is %d but your %s skill is only capable of creating level "
                   "%d crystals.\r\n",
                   op->essence_level, ability_names[op->skill],
                   craft_legacy_skill_equivalent(ch, op->skill) / 3);
    return false;
  }
  op->type = SCMD_AUGMENT;
  op->quest = AQ_CRAFT_AUGMENT;
  op->cost = op->essence_level * 500 / 3;
  op->seconds = kit_seconds(ch, op->skill, 10, op->cost);
  op->legacy_exp = kit_legacy_exp(op->essence_level * GET_LEVEL(ch) + GET_LEVEL(ch), op->seconds);
  return true;
}

/* The augment roll happens at resolution: the second essence is consumed either way. */
static void apply_augment(struct char_data *ch, struct obj_data *kit, struct kit_operation *op)
{
  struct obj_data *obj, *one = NULL, *two = NULL;
  int level_diff, success_chance, roll;

  for (obj = kit->contains; obj != NULL; obj = obj->next_content)
  {
    if (GET_OBJ_TYPE(obj) == ITEM_ESSENCE && !one)
      one = obj;
    else if (GET_OBJ_TYPE(obj) == ITEM_ESSENCE && !two)
      two = obj;
  }
  if (!one || !two)
  {
    send_to_char(ch, "The kit no longer holds two essences to augment.\r\n");
    return;
  }
  level_diff = abs(GET_OBJ_LEVEL(one) - GET_OBJ_LEVEL(two));
  roll = dice(1, 100);
  success_chance = 100 - (level_diff * 10) - op->essence_level;
  if (roll >= 95)
    success_chance = 150;
  roll += craft_legacy_skill_equivalent(ch, op->skill) / 3;
  if (roll > success_chance)
    send_to_char(ch, "There seems to be a flaw in your augmentation...\r\n");
  else
    GET_OBJ_LEVEL(one) = op->essence_level + 1;
  obj_from_obj(one);
  extract_obj(two);
  obj_to_char(one, ch);
  act("You augment $p.", false, ch, one, 0, TO_CHAR);
  act("$n augments $p.", false, ch, one, 0, TO_ROOM);
}

/* ---- disenchant ---- */

static bool plan_disenchant(struct char_data *ch, struct obj_data *kit, const char *argument,
                            struct kit_operation *op, bool verbose)
{
  int count;
  struct obj_data *obj = kit_single_item(kit, &count);

  (void)argument;
  if (obj == NULL)
  {
    if (verbose)
      send_to_char(ch, count > 1 ? "Only one item should be inside the kit.\r\n"
                                 : "You do not seem to have a magical item in the kit.\r\n");
    return false;
  }
  if (!IS_SET_AR(GET_OBJ_EXTRA(obj), ITEM_MAGIC))
  {
    if (verbose)
      send_to_char(ch, "Only magical items can be disenchanted.\r\n");
    return false;
  }
  if (GET_OBJ_LEVEL(obj) < 5)
  {
    if (verbose)
      send_to_char(ch, "You need a more powerful object to have any chance of extracting magic "
                       "essence.\r\n");
    return false;
  }
  op->type = SCMD_DISENCHANT;
  op->skill = ABILITY_CRAFT_ALCHEMY;
  op->quest = AQ_CRAFT_DISENCHANT;
  op->cost = 0;
  op->obj_level = GET_OBJ_LEVEL(obj);
  op->seconds = MAX(12, craft_legacy_kit_seconds(ch, op->skill, 11));
  op->legacy_exp = kit_legacy_exp(10 * GET_LEVEL(ch) + GET_LEVEL(ch), op->seconds);
  return true;
}

/* The item's level is captured before extraction; the check and the essence roll resolve here. */
static void apply_disenchant(struct char_data *ch, struct obj_data *obj, struct kit_operation *op)
{
  struct obj_data *essence;
  int chem_check = craft_legacy_skill_equivalent(ch, op->skill) + d20(ch);
  int essence_level = dice(1, MAX(1, op->obj_level / 2));

  obj_from_obj(obj);
  extract_obj(obj);
  if (chem_check <= (op->obj_level * 3 + 10))
  {
    send_to_char(ch, "You are having difficulty extracting the magical essence...\r\n");
  }
  else if (!(essence = read_object(MAGICAL_ESSENCE, VIRTUAL)))
  {
    log("SYSERR: disenchant failed to load essence object %d", MAGICAL_ESSENCE);
    send_to_char(ch, "Report to staff please: disenchant failed to load essence object.\r\n");
  }
  else
  {
    GET_OBJ_LEVEL(essence) = essence_level;
    obj_to_char(essence, ch);
    act("You extract $p.", false, ch, essence, 0, TO_CHAR);
  }
  act("You complete the disenchantment process.", false, ch, 0, 0, TO_CHAR);
  act("$n finishes the disenchanting process.", false, ch, 0, 0, TO_ROOM);
}

/* ---- create (mold) ---- */

/* The canonical material group a mold works in, from its object material. */
static int mold_group(struct obj_data *mold)
{
  int group = craft_group_by_material(obj_material_to_craft_material(GET_OBJ_MATERIAL(mold)));

  if (group != CRAFT_GROUP_NONE)
    return group;
  if (IS_CLOTH(GET_OBJ_MATERIAL(mold)))
    return CRAFT_GROUP_CLOTH;
  if (IS_LEATHER(GET_OBJ_MATERIAL(mold)))
    return CRAFT_GROUP_HIDES;
  if (IS_WOOD(GET_OBJ_MATERIAL(mold)))
    return CRAFT_GROUP_WOOD;
  if (IS_HARD_METAL(GET_OBJ_MATERIAL(mold)))
    return CRAFT_GROUP_HARD_METALS;
  if (IS_PRECIOUS_METAL(GET_OBJ_MATERIAL(mold)))
    return CRAFT_GROUP_SOFT_METALS;
  return CRAFT_GROUP_NONE;
}

/* Whether the character may work a mold of this group with this balance material. Bone-armor
 * masters may substitute bone or dragonbone for the normal material. */
static bool mold_accepts_material(struct char_data *ch, struct obj_data *mold, int group,
                                  int material)
{
  if (material <= CRAFT_MAT_NONE || material >= NUM_CRAFT_MATS)
    return false;
  if (HAS_FEAT(ch, FEAT_BONE_ARMOR) &&
      (material == CRAFT_MAT_BONE || material == CRAFT_MAT_DRAGONBONE))
    return !(GET_OBJ_TYPE(mold) == ITEM_ARMOR && material == CRAFT_MAT_DRAGONBONE);
  if (craft_group_by_material(material) != group)
    return false;
  if (GET_OBJ_TYPE(mold) == ITEM_WEAPON && material == CRAFT_MAT_DRAGONSCALE)
    return false;
  return true;
}

/* Case-insensitive substring test. */
static bool contains_ci(const char *haystack, const char *needle)
{
  size_t length = strlen(needle);

  if (length == 0)
    return false;
  for (; *haystack; haystack++)
    if (!strncasecmp(haystack, needle, length))
      return true;
  return false;
}

/* Find the material the description names: the longest acceptable material name it contains
 * (so "high grade hide" beats "hide" and "cold iron" beats "iron"). */
static int mold_material_from_text(struct char_data *ch, struct obj_data *mold, int group,
                                   const char *text)
{
  int material, best = CRAFT_MAT_NONE;
  size_t best_length = 0;

  if (!text || !*text)
    return CRAFT_MAT_NONE;
  for (material = 1; material < NUM_CRAFT_MATS; material++)
  {
    if (!mold_accepts_material(ch, mold, group, material))
      continue;
    if (contains_ci(text, crafting_materials[material]) &&
        strlen(crafting_materials[material]) > best_length)
    {
      best = material;
      best_length = strlen(crafting_materials[material]);
    }
  }
  return best;
}

/* The ability a mold's wear flags use. */
static int mold_skill(struct obj_data *mold)
{
  if (CAN_WEAR(mold, ITEM_WEAR_FINGER) || CAN_WEAR(mold, ITEM_WEAR_ANKLE) ||
      CAN_WEAR(mold, ITEM_WEAR_NECK) || CAN_WEAR(mold, ITEM_WEAR_HOLD))
    return ABILITY_CRAFT_JEWELCRAFTING;
  if (CAN_WEAR(mold, ITEM_WEAR_BODY) || CAN_WEAR(mold, ITEM_WEAR_ARMS) ||
      CAN_WEAR(mold, ITEM_WEAR_LEGS) || CAN_WEAR(mold, ITEM_WEAR_HEAD) ||
      CAN_WEAR(mold, ITEM_WEAR_FEET) || CAN_WEAR(mold, ITEM_WEAR_HANDS) ||
      CAN_WEAR(mold, ITEM_WEAR_WRIST) || CAN_WEAR(mold, ITEM_WEAR_WAIST))
  {
    if (IS_HARD_METAL(GET_OBJ_MATERIAL(mold)))
      return ABILITY_CRAFT_ARMORSMITHING;
    if (IS_LEATHER(GET_OBJ_MATERIAL(mold)))
      return ABILITY_CRAFT_LEATHERWORKING;
    return ABILITY_CRAFT_TAILORING;
  }
  if (CAN_WEAR(mold, ITEM_WEAR_ABOUT))
    return ABILITY_CRAFT_TAILORING;
  if (CAN_WEAR(mold, ITEM_WEAR_WIELD) || CAN_WEAR(mold, ITEM_WEAR_SHIELD))
    return ABILITY_CRAFT_WEAPONSMITHING;
  return ABILITY_CRAFT_WEAPONSMITHING;
}

/* Sort the kit: one mold, at most one crystal and one essence; material bundles are refused
 * with the deposit step, because the mold draws its materials from the shared balances. */
static bool mold_kit_contents(struct char_data *ch, struct obj_data *kit, struct obj_data **mold,
                              struct obj_data **crystal, struct obj_data **essence, bool verbose)
{
  struct obj_data *obj;

  *mold = *crystal = *essence = NULL;
  for (obj = kit->contains; obj != NULL; obj = obj->next_content)
  {
    if (OBJ_FLAGGED(obj, ITEM_MOLD))
    {
      if (*mold)
      {
        if (verbose)
          send_to_char(ch, "You have more than one mold inside the kit, please only put one "
                           "inside.\r\n");
        return false;
      }
      *mold = obj;
    }
    else if (GET_OBJ_TYPE(obj) == ITEM_CRYSTAL)
    {
      if (*crystal)
      {
        if (verbose)
          send_to_char(ch, "You have more than one crystal inside the kit, please only put one "
                           "inside.\r\n");
        return false;
      }
      *crystal = obj;
    }
    else if (GET_OBJ_TYPE(obj) == ITEM_ESSENCE)
    {
      if (*essence)
      {
        if (verbose)
          send_to_char(ch, "You have more than one essence inside the kit, please only put one "
                           "inside.\r\n");
        return false;
      }
      *essence = obj;
    }
    else if (GET_OBJ_TYPE(obj) == ITEM_MATERIAL)
    {
      if (verbose)
        send_to_char(ch,
                     "Molds draw their materials from your crafting materials now. Take %s out "
                     "of the kit and deposit it with 'craftmaterials store', then name the "
                     "material in the item description.\r\n",
                     obj->short_description);
      return false;
    }
    else
    {
      if (verbose)
        send_to_char(ch, "There is an unnecessary item in the kit, please remove it.\r\n");
      return false;
    }
  }
  if (!*mold)
  {
    if (verbose)
      send_to_char(ch, "The creation process requires a mold to continue.\r\n");
    return false;
  }
  return true;
}

#define CREATE_STRING_LIMIT 80

static bool plan_create(struct char_data *ch, struct obj_data *kit, const char *argument,
                        struct kit_operation *op, bool check_only, bool verbose)
{
  struct obj_data *mold, *crystal, *essence;
  int group, material, best_material = CRAFT_MAT_NONE, i;
  size_t l = 0;

  if (!check_only)
  {
    if (!argument || !*argument)
    {
      if (verbose)
        send_to_char(ch, "Please provide an item description containing the material name.\r\n");
      return false;
    }
    if (strchr(argument, '\''))
    {
      if (verbose)
        send_to_char(ch, "The usage of the character: ' is not allowed in create currently (it "
                         "conflicts with color codes).\r\n");
      return false;
    }
    l = strlen(argument);
    if (l > CREATE_STRING_LIMIT)
    {
      if (verbose)
        send_to_char(ch,
                     "The length (%d) of the name you gave your object is over the limit "
                     "(%d).\r\n",
                     (int)l, CREATE_STRING_LIMIT);
      return false;
    }
  }
  if (!mold_kit_contents(ch, kit, &mold, &crystal, &essence, verbose))
    return false;
  group = mold_group(mold);
  if (group == CRAFT_GROUP_NONE)
  {
    if (verbose)
      send_to_char(ch, "This mold's material cannot be worked from your crafting materials.\r\n");
    return false;
  }
  op->obj_level = crystal ? GET_OBJ_LEVEL(crystal) : GET_OBJ_LEVEL(mold);
  op->mats_needed = MAX(MIN_MATS, GET_OBJ_WEIGHT(mold) / WEIGHT_FACTOR);
  if (HAS_FEAT(ch, FEAT_ELVEN_CRAFTING))
    op->mats_needed = MAX(MIN_ELF_MATS, op->mats_needed / 2);

  /* The material: named in the description, or for a check the best-stocked acceptable one. */
  if (check_only)
  {
    for (material = 1; material < NUM_CRAFT_MATS; material++)
      if (mold_accepts_material(ch, mold, group, material) &&
          (best_material == CRAFT_MAT_NONE ||
           GET_CRAFT_MAT(ch, material) > GET_CRAFT_MAT(ch, best_material)))
        best_material = material;
    op->material = best_material;
  }
  else
    op->material = mold_material_from_text(ch, mold, group, argument);
  if (op->material == CRAFT_MAT_NONE)
  {
    if (verbose)
    {
      send_to_char(ch, "You must name the material in the object description. This mold takes %s: ",
                   crafting_material_groups[group]);
      for (material = 1, i = 0; material < NUM_CRAFT_MATS; material++)
        if (mold_accepts_material(ch, mold, group, material))
          send_to_char(ch, "%s%s", i++ ? ", " : "", crafting_materials[material]);
      send_to_char(ch, ".\r\n");
    }
    return false;
  }
  if (GET_CRAFT_MAT(ch, op->material) < op->mats_needed)
  {
    if (verbose)
      send_to_char(ch,
                   "You need %d units of %s in your crafting materials to make that item; you "
                   "have %d. Deposit bundles with 'craftmaterials store'.\r\n",
                   op->mats_needed, crafting_materials[op->material],
                   GET_CRAFT_MAT(ch, op->material));
    return false;
  }
  op->chance_of_crit = 0;
  if (essence)
  {
    op->chance_of_crit = GET_OBJ_LEVEL(essence) * 2;
    if (HAS_FEAT(ch, FEAT_MASTERWORK_CRAFTING))
      op->chance_of_crit += 10;
    if (HAS_FEAT(ch, FEAT_DWARVEN_CRAFTING))
      op->chance_of_crit += 10;
    if (HAS_FEAT(ch, FEAT_DRACONIC_CRAFTING))
      op->chance_of_crit += 10;
  }
  op->skill = mold_skill(mold);
  if (craft_legacy_skill_equivalent(ch, op->skill) / 3 < op->obj_level)
  {
    if (verbose)
      send_to_char(ch,
                   "Your skill in %s (rank %d) is too low to create that item, you need rank "
                   "%d.\r\n",
                   ability_names[op->skill], get_craft_skill_value(ch, op->skill),
                   (op->obj_level * 3 + CRAFT_LEGACY_SKILL_PER_RANK - 1) /
                       CRAFT_LEGACY_SKILL_PER_RANK);
    return false;
  }
  op->type = SCMD_CRAFT;
  op->quest = AQ_CRAFT;
  op->cost = op->obj_level * op->obj_level * 100 / 3;
  op->seconds = kit_seconds(ch, op->skill, 11, op->cost);
  op->legacy_exp = kit_legacy_exp(op->obj_level * GET_LEVEL(ch) + GET_LEVEL(ch), op->seconds);
  if (!check_only)
    snprintf(op->text, sizeof(op->text), "%s", argument);
  return true;
}

/* checkcraft: the preview of what create would make. */
static void preview_create(struct char_data *ch, struct obj_data *kit, struct kit_operation *op)
{
  struct obj_data *mold, *crystal, *essence;

  if (!mold_kit_contents(ch, kit, &mold, &crystal, &essence, false))
    return;
  send_to_char(ch, "This crafting session will create the following item:\r\n\r\n");
  do_stat_object(ch, mold, ITEM_STAT_MODE_IDENTIFY_SPELL);
  if (crystal)
  {
    send_to_char(ch, "You will be enhancing it with this crystal:\r\n");
    do_stat_object(ch, crystal, ITEM_STAT_MODE_IDENTIFY_SPELL);
  }
  if (essence)
  {
    send_to_char(ch, "Basic essence chance of critical (masterwork): %d.  ",
                 GET_OBJ_LEVEL(essence) * 2);
    if (HAS_FEAT(ch, FEAT_MASTERWORK_CRAFTING))
      send_to_char(ch, "Masterwork Crafting feat bonus: 10.  ");
    if (HAS_FEAT(ch, FEAT_DWARVEN_CRAFTING))
      send_to_char(ch, "Dwarven Crafting feat bonus: 10.  ");
    if (HAS_FEAT(ch, FEAT_DRACONIC_CRAFTING))
      send_to_char(ch, "Draconic Crafting feat bonus: 10.  ");
    send_to_char(ch, "\r\nYou have a %d percent chance of creating a masterwork item.\r\n",
                 op->chance_of_crit);
  }
  send_to_char(ch, "The item will be level: %d.\r\n", op->obj_level);
  send_to_char(ch, "It will use %d units of %s from your crafting materials (you have %d).\r\n",
               op->mats_needed, crafting_materials[op->material], GET_CRAFT_MAT(ch, op->material));
  send_to_char(ch, "It will make use of your %s skill, which is at rank %d.\r\n",
               ability_names[op->skill], get_craft_skill_value(ch, op->skill));
  send_to_char(ch, "This crafting session will take %d seconds.\r\n", op->seconds);
  send_to_char(ch, "You need %d gold on hand to make this item.\r\n", op->cost);
}

/* The mold becomes the item: flags, level, material, crystal affects and enhancement, the
 * masterwork roll, cost, and the player's strings; the crystal and essence are consumed. */
static void apply_create(struct char_data *ch, struct obj_data *kit, struct kit_operation *op)
{
  struct obj_data *mold, *crystal, *essence;
  char buf[MAX_INPUT_LENGTH];
  char *text = op->text;
  int i, chance_of_crit = op->chance_of_crit;

  if (!mold_kit_contents(ch, kit, &mold, &crystal, &essence, false))
    return;
  REMOVE_BIT_AR(GET_OBJ_EXTRA(mold), ITEM_MOLD);
  if (essence || crystal)
    SET_BIT_AR(GET_OBJ_EXTRA(mold), ITEM_MAGIC);
  GET_OBJ_LEVEL(mold) = op->obj_level;
  GET_OBJ_MATERIAL(mold) = craft_material_to_obj_material(op->material);
  if (crystal)
  {
    for (i = 0; i < MAX_OBJ_AFFECT; i++)
    {
      if (crystal->affected[i].modifier && crystal->affected[i].location)
      {
        mold->affected[i].location = crystal->affected[i].location;
        mold->affected[i].modifier = crystal->affected[i].modifier;
        mold->affected[i].bonus_type = crystal->affected[i].bonus_type
                                           ? crystal->affected[i].bonus_type
                                           : BONUS_TYPE_ENHANCEMENT;
      }
    }
    if (CAN_WEAR(mold, ITEM_WEAR_WIELD) || CAN_WEAR(mold, ITEM_WEAR_SHIELD) ||
        CAN_WEAR(mold, ITEM_WEAR_HEAD) || CAN_WEAR(mold, ITEM_WEAR_BODY) ||
        CAN_WEAR(mold, ITEM_WEAR_LEGS) || CAN_WEAR(mold, ITEM_WEAR_ARMS) ||
        GET_OBJ_TYPE(mold) == ITEM_MISSILE)
      GET_OBJ_VAL(mold, 4) = MIN(CRAFT_MAX_BONUS, ((GET_OBJ_LEVEL(mold) + 5) / 5));
  }
  if (essence)
  {
    if (GET_LEVEL(ch) >= LVL_IMMORT)
    {
      send_to_char(ch, "Staff override on crit chance (real chance: %d)\r\n", chance_of_crit);
      chance_of_crit = 101;
    }
    if (dice(1, 100) <= chance_of_crit)
    {
      mold->affected[3].location = random_apply_value();
      mold->affected[3].modifier = adjust_bonus_value(mold->affected[3].location, 1);
      mold->affected[3].bonus_type = BONUS_TYPE_INHERENT;
      send_to_char(ch, "You feel a sense of inspiration as you finish your craft!\r\n");
    }
  }
  GET_OBJ_COST(mold) =
      100 + GET_OBJ_LEVEL(mold) * 50 * MAX(1, GET_OBJ_LEVEL(mold) - 1) + GET_OBJ_COST(mold);
  parse_at(text);
  free_object_string(mold, mold->short_description);
  mold->short_description = strdup(text);
  snprintf(buf, sizeof(buf), "%s lies here.", CAP(text));
  free_object_string(mold, mold->description);
  mold->description = strdup(buf);
  strip_colors(text);
  free_object_string(mold, mold->name);
  mold->name = strdup(text);
  GET_CRAFT_MAT(ch, op->material) -= op->mats_needed;
  obj_from_obj(mold);
  if (crystal)
    extract_obj(crystal);
  if (essence)
    extract_obj(essence);
  obj_to_char(mold, ch);
  act("You create $p.", false, ch, mold, 0, TO_CHAR);
  act("$n creates $p.", false, ch, mold, 0, TO_ROOM);
}

/* ---- the activity ---- */

static bool plan_kit_operation(struct char_data *ch, struct obj_data *kit, int type,
                               const char *argument, struct kit_operation *op, bool verbose)
{
  memset(op, 0, sizeof(*op));
  op->skill = -1;
  op->quest = -1;
  switch (type)
  {
  case SCMD_RESTRING:
    return plan_restring(ch, kit, argument, op, verbose);
  case SCMD_REDESC:
    return plan_redesc(ch, kit, argument, op, verbose);
  case SCMD_RESIZE:
    return plan_resize(ch, kit, argument, op, verbose);
  case SCMD_BONEARMOR:
    return plan_bonearmor(ch, kit, argument, op, verbose);
  case SCMD_REFORGE:
    return plan_reforge(ch, kit, argument, op, verbose);
  case SCMD_AUGMENT:
    return plan_augment(ch, kit, argument, op, verbose);
  case SCMD_DISENCHANT:
    return plan_disenchant(ch, kit, argument, op, verbose);
  case SCMD_CRAFT:
    return plan_create(ch, kit, argument, op, false, verbose);
  default:
    return false;
  }
}

static bool kit_activity_recheck(struct char_data *ch, void *target, void *context)
{
  struct kit_operation *op = context;
  struct obj_data *kit = target;
  struct kit_operation again;

  if (!ch || !kit || !op || kit->carried_by != ch || !FIGHTING(ch) == false ||
      GET_POS(ch) < POS_STANDING || ch->desc == NULL)
    return false;
  return plan_kit_operation(ch, kit, op->type, op->text, &again, false);
}

/* Completion: re-plan against the current kit, gold, and gates, then resolve once. */
static void kit_activity_complete(struct char_data *ch, void *target, void *context)
{
  struct kit_operation *op = context, now;
  struct obj_data *kit = target, *obj;
  int count;
  bool applied = true;

  if (!ch || !kit || !op || kit->carried_by != ch)
    return;
  if (!plan_kit_operation(ch, kit, op->type, op->text, &now, true))
  {
    send_to_char(ch, "Your crafting work comes to nothing; the kit's contents no longer suit "
                     "it.\r\n");
    return;
  }
  if (GET_GOLD(ch) < now.cost)
  {
    send_to_char(ch, "You need %d coins on hand for supplies to finish this work.\r\n", now.cost);
    return;
  }
  if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
  {
    send_to_char(ch, "You must make room in your inventory before the finished item can leave "
                     "the kit.\r\n");
    return;
  }
  obj = kit_single_item(kit, &count);
  switch (now.type)
  {
  case SCMD_RESTRING:
    apply_restring(ch, obj, &now);
    obj_from_obj(obj);
    obj_to_char(obj, ch);
    act("You rename $p.", false, ch, obj, 0, TO_CHAR);
    act("$n renames $p.", false, ch, obj, 0, TO_ROOM);
    break;
  case SCMD_REDESC:
    apply_redesc(ch, obj, &now);
    obj_from_obj(obj);
    obj_to_char(obj, ch);
    act("You redesc $p.", false, ch, obj, 0, TO_CHAR);
    act("$n redescs $p.", false, ch, obj, 0, TO_ROOM);
    break;
  case SCMD_RESIZE:
    applied = apply_resize(ch, obj, &now);
    if (!applied)
      break;
    obj_from_obj(obj);
    obj_to_char(obj, ch);
    act("You resize $p.", false, ch, obj, 0, TO_CHAR);
    act("$n resizes $p.", false, ch, obj, 0, TO_ROOM);
    break;
  case SCMD_BONEARMOR:
    apply_bonearmor(ch, obj, &now);
    obj_from_obj(obj);
    obj_to_char(obj, ch);
    act("You finish converting $p into bone.", false, ch, obj, 0, TO_CHAR);
    act("$n finishes converting $p into bone.", false, ch, obj, 0, TO_ROOM);
    break;
  case SCMD_REFORGE:
    reforge_apply(ch, obj, now.reforge_index);
    obj_from_obj(obj);
    obj_to_char(obj, ch);
    act("You finish reforging $p.", false, ch, obj, 0, TO_CHAR);
    act("$n finishes reforging $p.", false, ch, obj, 0, TO_ROOM);
    break;
  case SCMD_AUGMENT:
    apply_augment(ch, kit, &now);
    break;
  case SCMD_DISENCHANT:
    apply_disenchant(ch, obj, &now);
    break;
  case SCMD_CRAFT:
    apply_create(ch, kit, &now);
    break;
  default:
    return;
  }
  if (!applied)
    return;
  if (now.cost > 0)
  {
    send_to_char(ch, "It cost you %d coins in supplies.\r\n", now.cost);
    award_gold(ch, -now.cost);
  }
  if (now.quest >= 0)
    autoquest_trigger_check(ch, NULL, NULL, 0, now.quest);
  if (now.skill >= 0)
    gain_craft_exp(ch, craft_operation_exp(now.obj_level > 0 ? now.obj_level : 1), now.skill, TRUE);
  if (now.legacy_exp > 0)
    award_legacy_crafting_experience(ch, now.legacy_exp);
  save_char(ch, 0);
  Crash_crashsave(ch);
}

static const char *kit_operation_verb(int type)
{
  switch (type)
  {
  case SCMD_RESTRING:
    return "restring";
  case SCMD_REDESC:
    return "redesc";
  case SCMD_RESIZE:
    return "resize";
  case SCMD_BONEARMOR:
    return "convert to bone";
  case SCMD_REFORGE:
    return "reforge";
  case SCMD_AUGMENT:
    return "augment";
  case SCMD_DISENCHANT:
    return "disenchant";
  default:
    return "craft";
  }
}

/* Start a planned kit operation: nothing is spent or changed until completion. */
static bool start_kit_operation(struct char_data *ch, struct obj_data *kit,
                                struct kit_operation *planned)
{
  struct primary_activity_definition definition = {0};
  struct primary_activity_snapshot snapshot;
  struct kit_operation *op;
  char description[64];

  if (primary_activity_snapshot(ch, &snapshot))
  {
    send_to_char(ch, "You are already doing something. Please wait until your current task "
                     "ends.\r\n");
    return false;
  }
  if (GET_GOLD(ch) < planned->cost)
  {
    send_to_char(ch, "You need %d coins on hand for supplies to %s this item.\r\n", planned->cost,
                 kit_operation_verb(planned->type));
    return false;
  }
  CREATE(op, struct kit_operation, 1);
  *op = *planned;
  snprintf(description, sizeof(description), "using a crafting kit to %s",
           kit_operation_verb(op->type));
  definition.type = PRIMARY_ACTIVITY_CRAFT;
  definition.display_name = description;
  definition.capabilities = PRIMARY_ACTIVITY_CAP_HANDS | PRIMARY_ACTIVITY_CAP_ATTENTION;
  definition.traits = PRIMARY_ACTIVITY_TRAIT_STATIONARY | PRIMARY_ACTIVITY_TRAIT_HANDS_OCCUPIED;
  definition.progress_model = PRIMARY_ACTIVITY_PROGRESS_PROGRESSIVE;
  definition.progress_owner = PRIMARY_ACTIVITY_PROGRESS_CHARACTER;
  definition.total_steps = (uint32_t)MAX(1, op->seconds);
  definition.step_interval = PASSES_PER_SEC;
  definition.wall_clock = true;
  definition.movement_response = PRIMARY_ACTIVITY_RESPONSE_CANCEL;
  definition.damage_response = PRIMARY_ACTIVITY_RESPONSE_CANCEL;
  definition.combat_response = PRIMARY_ACTIVITY_RESPONSE_CANCEL;
  definition.target_loss_response = PRIMARY_ACTIVITY_RESPONSE_CANCEL;
  definition.command_response = PRIMARY_ACTIVITY_RESPONSE_REJECT;
  definition.recheck = kit_activity_recheck;
  definition.complete = kit_activity_complete;
  definition.cleanup_context = free;
  definition.context = op;
  if (!primary_activity_start(ch, domain_event_object_handle(kit), &definition))
  {
    free(op);
    send_to_char(ch, "Your crafting task could not be scheduled. Please try again.\r\n");
    return false;
  }
  send_to_char(ch, "You begin to %s. This will take %d seconds.\r\n",
               kit_operation_verb(planned->type), planned->seconds);
  act("$n starts working with a crafting kit.", FALSE, ch, 0, 0, TO_ROOM);
  return true;
}

/* Plan and start one kit operation from any front end (the kit special or 'craft mold'). */
static bool run_kit_operation(struct char_data *ch, struct obj_data *kit, int type,
                              const char *argument)
{
  struct kit_operation op;

  if (!plan_kit_operation(ch, kit, type, argument, &op, true))
    return false;
  return start_kit_operation(ch, kit, &op);
}

/* The first crafting kit the character carries, for the editor's mold entry point. */
static struct obj_data *carried_crafting_kit(struct char_data *ch)
{
  struct obj_data *obj;

  for (obj = ch->carrying; obj; obj = obj->next_content)
    if (is_crafting_kit(obj))
      return obj;
  return NULL;
}

/* craft mold [check | <description>]: the editor's front end to mold creation. */
void craft_mold_command(struct char_data *ch, const char *argument)
{
  struct obj_data *kit = carried_crafting_kit(ch);
  struct kit_operation op;

  while (argument && *argument == ' ')
    argument++;
  if (kit == NULL)
  {
    send_to_char(ch, "Mold creation needs a crafting kit in your inventory holding the mold "
                     "(and optionally a crystal and an essence).\r\n");
    return;
  }
  if (!argument || !*argument || !str_cmp(argument, "check"))
  {
    if (plan_create(ch, kit, NULL, &op, true, true))
      preview_create(ch, kit, &op);
    return;
  }
  (void)run_kit_operation(ch, kit, SCMD_CRAFT, argument);
}

SPECIAL(crafting_kit)
{
  struct obj_data *kit = (struct obj_data *)me;
  struct kit_operation op;
  int type;

  if (!cmd && argument && !strcmp(argument, "identify"))
  {
    send_to_char(ch, "This is a crafting kit. You can use the following commands:\r\n");
    send_to_char(ch, "  resize      - Resize armor or weapons\r\n");
    send_to_char(ch, "  create      - Create an item from a mold (materials come from your "
                     "crafting materials; 'craft mold' does the same)\r\n");
    send_to_char(ch, "  checkcraft  - Preview what create would make\r\n");
    send_to_char(ch, "  restring    - Change an item's short description\r\n");
    send_to_char(ch, "  redesc      - Change an item's long description\r\n");
    send_to_char(ch, "  augment     - Combine two essences\r\n");
    send_to_char(ch, "  disenchant  - Extract an essence from a magic item\r\n");
    send_to_char(ch, "  bonearmor   - Convert armor to bone\r\n");
    send_to_char(ch, "  reforge     - Reforge weapons, armor, and shields\r\n");
    return TRUE;
  }

  if (CMD_IS("resize"))
    type = SCMD_RESIZE;
  else if (CMD_IS("create"))
    type = SCMD_CRAFT;
  else if (CMD_IS("checkcraft"))
    type = SCMD_CRAFT_UNDF;
  else if (CMD_IS("restring"))
    type = SCMD_RESTRING;
  else if (CMD_IS("redesc"))
    type = SCMD_REDESC;
  else if (CMD_IS("augment"))
    type = SCMD_AUGMENT;
  else if (CMD_IS("disenchant"))
    type = SCMD_DISENCHANT;
  else if (CMD_IS("bonearmor"))
    type = SCMD_BONEARMOR;
  else if (CMD_IS("reforge"))
    type = SCMD_REFORGE;
  else
    return 0;

  if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
  {
    send_to_char(ch, "You cannot craft anything until you've made some room in your "
                     "inventory.\r\n");
    return 1;
  }
  skip_spaces(&argument);
  if (!*argument && (type == SCMD_CRAFT || type == SCMD_RESTRING || type == SCMD_BONEARMOR ||
                     type == SCMD_REDESC || type == SCMD_RESIZE || type == SCMD_REFORGE))
  {
    if (type == SCMD_RESIZE)
      send_to_char(ch, "What would you like the new size to be? (fine|diminutive|tiny|small|"
                       "medium|large|huge|gargantuan|colossal)\r\n");
    else if (type == SCMD_REFORGE)
      send_to_char(ch, "Please specify the type of weapon, armor or shield you'd like to reforge "
                       "this item into. See weaponlist and armorlistfull for options.\r\n");
    else
      send_to_char(ch, "Please provide an item description containing the item name in the "
                       "string.\r\n");
    return 1;
  }
  if (!kit->contains)
  {
    if (type == SCMD_AUGMENT)
      send_to_char(ch, "You must place two essences into the kit in order to augment.\r\n");
    else if (type == SCMD_CRAFT || type == SCMD_CRAFT_UNDF)
      send_to_char(ch, "You must place a mold (and optionally a crystal and an essence) in the "
                       "kit, deposit your materials with 'craftmaterials store', and then type "
                       "'create <item description naming the material>'.\r\n");
    else
      send_to_char(ch, "You must place the item in the kit first.\r\n");
    return 1;
  }
  if (kit->carried_by != ch)
  {
    send_to_char(ch, "You must be holding your kit to perform any crafting tasks.\r\n");
    return 1;
  }
  if (type == SCMD_CRAFT_UNDF)
  {
    if (plan_create(ch, kit, NULL, &op, true, true))
      preview_create(ch, kit, &op);
    return 1;
  }
  (void)run_kit_operation(ch, kit, type, argument);
  return 1;
}

#undef CREATE_STRING_LIMIT

/* eCRAFTING, eCRAFT, and eBREWING keep their serialized ids; the work they timed now runs on the
 * activity manager. A stray record from an older save ends here without effect. */
MUD_EVENT_CALLBACK(event_retired)
{
  (void)event_obj;
  log("SYSERR: retired crafting event fired; ignoring.");
  return 0;
}

/* Use wilderness harvesting only when the legacy node system cannot handle the request. */
static bool try_wilderness_harvest_fallback(struct char_data *ch, const char *argument, int cmd,
                                            int subcmd)
{
  if (IN_ROOM(ch) == NOWHERE || !ZONE_FLAGGED(world[IN_ROOM(ch)].zone, ZONE_WILDERNESS))
    return false;

  do_wilderness_harvest(ch, argument, cmd, subcmd);
  return true;
}

/* ---- Object nodes on the activity manager (consolidation Decision 3) ---- */

/* Roll a node's authored drop table into a prototype vnum, and report the minimum legacy skill
 * the node needs. The probabilities are the original table's; NOTHING for an unknown node. */
static obj_vnum node_drop_prototype(int material, int *minskill_out)
{
  obj_vnum vnum = NOTHING;
  int roll = 0, min_skill = 0;

  switch (material)
  {
  case MATERIAL_STEEL:
    roll = dice(1, 100);
    if (roll <= 40)
      vnum = BRONZE_MATERIAL;
    else if (roll <= 75)
      vnum = IRON_MATERIAL;
    else if (roll <= 96)
      vnum = STEEL_MATERIAL;
    else if (roll <= 98)
      vnum = ONYX_MATERIAL;
    else
      vnum = OBSIDIAN_MATERIAL;
    min_skill = 1;
    break;

  case MATERIAL_COLD_IRON:
    roll = dice(1, 100);
    if (roll <= 48)
      vnum = COLD_IRON_MATERIAL;
    else if (roll <= 52)
      vnum = ONYX_MATERIAL;
    else
      vnum = IRON_MATERIAL;
    min_skill = 35;
    break;

  case MATERIAL_MITHRIL:
    roll = dice(1, 100);
    if (roll <= 48)
      vnum = MITHRIL_MATERIAL;
    else if (roll <= 96)
      vnum = MITHRIL_MATERIAL;
    else if (roll <= 98)
      vnum = RUBY_MATERIAL;
    else
      vnum = SAPPHIRE_MATERIAL;
    min_skill = 48;
    break;

  case MATERIAL_ADAMANTINE:
    roll = dice(1, 100);
    if (roll <= 4)
      vnum = ADAMANTINE_MATERIAL;
    else if (roll <= 96)
      vnum = PLATINUM_MATERIAL;
    else
    {
      if (dice(1, 2) % 2 == 0)
        vnum = DIAMOND_MATERIAL;
      else
        vnum = EMERALD_MATERIAL;
    }
    min_skill = 61;
    break;

  case MATERIAL_SILVER:
    roll = dice(1, 10);
    if (roll <= (8))
    {
      roll = dice(1, 100);
      if (roll <= 48)
        vnum = COPPER_MATERIAL;
      else if (roll <= 96)
        vnum = ALCHEMAL_SILVER_MATERIAL;
      else if (roll <= 98)
        vnum = ONYX_MATERIAL;
      else
        vnum = OBSIDIAN_MATERIAL;
    }
    else
    {
      roll = dice(1, 100);
      if (roll <= 48)
        vnum = SILVER_MATERIAL;
      else if (roll <= 52)
        vnum = ONYX_MATERIAL;
      else
        vnum = SILVER_MATERIAL;
    }
    min_skill = 1;
    break;

  case MATERIAL_GOLD:
    roll = dice(1, 10);
    if (roll <= (8))
    {
      roll = dice(1, 100);
      if (roll <= 48)
        vnum = GOLD_MATERIAL;
      else if (roll <= 96)
        vnum = GOLD_MATERIAL;
      else if (roll <= 98)
        vnum = RUBY_MATERIAL;
      else
        vnum = SAPPHIRE_MATERIAL;
    }
    else
    {
      roll = dice(1, 100);
      if (roll <= 4)
        vnum = PLATINUM_MATERIAL;
      else if (roll <= 96)
        vnum = PLATINUM_MATERIAL;
      else
      {
        if (dice(1, 2) % 2 == 0)
          vnum = DIAMOND_MATERIAL;
        else
          vnum = EMERALD_MATERIAL;
      }
    }
    min_skill = 30;
    break;

  case MATERIAL_WOOD:
    roll = dice(1, 100);
    if (roll <= (80))
    {
      if (dice(1, 100) <= 96)
        vnum = ALDERWOOD_MATERIAL;
      else
        vnum = FOS_BIRD_MATERIAL;
    }
    else if (roll <= (94))
    {
      if (dice(1, 100) <= 96)
        vnum = YEW_MATERIAL;
      else
        vnum = FOS_LIZARD_MATERIAL;
    }
    else
    {
      if (dice(1, 100) <= 96)
        vnum = OAK_MATERIAL;
      else
        vnum = FOS_WYVERN_MATERIAL;
    }
    min_skill = 1;
    break;

  case MATERIAL_DARKWOOD:
    if (dice(1, 100) <= 96)
      vnum = DARKWOOD_MATERIAL;
    else
      vnum = FOS_DRAGON_MATERIAL;
    min_skill = 38;
    break;

  case MATERIAL_LEATHER:
    roll = dice(1, 100);
    if (roll <= (82))
    {
      if (dice(1, 100) <= 96)
      {
        vnum = LEATHER_LQ_MATERIAL;
      }
      else
        vnum = FOS_BIRD_MATERIAL;
    }
    else if (roll <= (94))
    {
      if (dice(1, 10) <= 96)
      {
        vnum = LEATHER_MQ_MATERIAL;
      }
      else
        vnum = FOS_LIZARD_MATERIAL;
    }
    else
    {
      if (dice(1, 100) <= 96)
      {
        vnum = LEATHER_HQ_MATERIAL;
      }
      else
        vnum = FOS_WYVERN_MATERIAL;
    }
    min_skill = 1;
    break;

  case MATERIAL_DRAGONHIDE:
    if (dice(1, 100) <= 70)
      vnum = LEATHER_HQ_MATERIAL;
    else
      vnum = DRAGONHIDE_MATERIAL;
    min_skill = 58;
    break;

  case MATERIAL_HEMP:
    if (dice(1, 100) <= 96)
      vnum = HEMP_MATERIAL;
    else
      vnum = FOS_BIRD_MATERIAL;
    min_skill = 1;
    break;

  case MATERIAL_COTTON:
    if (dice(1, 100) <= 96)
    {
      vnum = COTTON_MATERIAL;
    }
    else
      vnum = FOS_LIZARD_MATERIAL;
    min_skill = 5;
    break;

  case MATERIAL_WOOL:
    if (dice(1, 100) <= 96)
    {
      vnum = WOOL_MATERIAL;
    }
    else
      vnum = FOS_LIZARD_MATERIAL;
    min_skill = 10;
    break;

  case MATERIAL_VELVET:
    if (dice(1, 100) <= 96)
    {
      vnum = VELVET_MATERIAL;
    }
    else
      vnum = FOS_WYVERN_MATERIAL;
    min_skill = 25;
    break;

  case MATERIAL_SATIN:
    if (dice(1, 100) <= 96)
    {
      vnum = SATIN_MATERIAL;
    }
    else
      vnum = FOS_WYVERN_MATERIAL;
    min_skill = 31;
    break;

  case MATERIAL_SILK:
    if (dice(1, 100) <= 96)
    {
      if (dice(1, 100) <= 25)
        vnum = VELVET_MATERIAL;
      else if (dice(1, 100) <= 25)
        vnum = SATIN_MATERIAL;
      else
        vnum = SILK_MATERIAL;
    }
    else
      vnum = FOS_DRAGON_MATERIAL;
    min_skill = 38;
    break;

  default:
    return NOTHING;
  }


  if (minskill_out)
    *minskill_out = min_skill;
  return vnum;
}

/* The minimum legacy-unit skill a node needs, without rolling a drop; -1 for an unknown node.
 * These mirror the thresholds in node_drop_prototype(). */
static int node_minimum_skill(int material)
{
  switch (material)
  {
  case MATERIAL_STEEL:
  case MATERIAL_SILVER:
  case MATERIAL_WOOD:
  case MATERIAL_LEATHER:
  case MATERIAL_HEMP:
    return 1;
  case MATERIAL_COTTON:
    return 5;
  case MATERIAL_WOOL:
    return 10;
  case MATERIAL_VELVET:
    return 25;
  case MATERIAL_GOLD:
    return 30;
  case MATERIAL_SATIN:
    return 31;
  case MATERIAL_COLD_IRON:
    return 35;
  case MATERIAL_DARKWOOD:
  case MATERIAL_SILK:
    return 38;
  case MATERIAL_MITHRIL:
    return 48;
  case MATERIAL_DRAGONHIDE:
    return 58;
  case MATERIAL_ADAMANTINE:
    return 61;
  }
  return -1;
}

/* The harvest ability and node family for a node's material. */
static int node_harvest_skill(int material, int *sub_command)
{
  if (IS_WOOD(material))
  {
    *sub_command = SCMD_FOREST;
    return ABILITY_HARVEST_FORESTRY;
  }
  if (IS_LEATHER(material))
  {
    *sub_command = SCMD_HUNT;
    return ABILITY_HARVEST_HUNTING;
  }
  if (IS_CLOTH(material))
  {
    *sub_command = SCMD_KNIT;
    return ABILITY_HARVEST_GATHERING;
  }
  *sub_command = SCMD_MINE;
  return ABILITY_HARVEST_MINING;
}

struct node_harvest_context
{
  int material;    /* the node's object material */
  int skill;       /* harvest ability */
  int sub_command; /* node family, for counters and quest hooks */
  room_rnum room;
};

/* The node must still be here, charged, and the harvester still able to work. */
static bool node_harvest_recheck(struct char_data *ch, void *target, void *context)
{
  struct node_harvest_context *harvest = context;
  struct obj_data *node = target;

  return ch && node && harvest && IN_ROOM(ch) == harvest->room && node->in_room == IN_ROOM(ch) &&
         GET_OBJ_VNUM(node) == HARVESTING_NODE && GET_OBJ_VAL(node, 0) > 0 && !FIGHTING(ch) &&
         GET_POS(ch) >= POS_STANDING;
}

/* Spend a charge after a confirmed reward: counters and extraction on depletion. */
static void node_spend_charge(struct obj_data *node, int sub_command)
{
  GET_OBJ_VAL(node, 0)--;
  if (GET_OBJ_VAL(node, 0) > 0)
    return;
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
  act("$p has been depleted.", FALSE, 0, node, 0, TO_ROOM);
  obj_from_room(node);
  extract_obj(node);
}

static int node_quest_type(int sub_command)
{
  switch (sub_command)
  {
  case SCMD_MINE:
    return AQ_CRAFT_MINE;
  case SCMD_KNIT:
    return AQ_CRAFT_KNIT;
  case SCMD_HUNT:
    return AQ_CRAFT_HUNT;
  default:
    return AQ_CRAFT_FOREST;
  }
}

/* Completion: recheck the node, roll the drop, credit a balance unit or deliver an object,
 * and only then spend the charge, fire the quest hook, and award experience. */
static void node_harvest_complete(struct char_data *ch, void *target, void *context)
{
  struct node_harvest_context *harvest = context;
  struct obj_data *node = target, *reward = NULL;
  obj_vnum drop;
  obj_rnum drop_rnum;
  int balance = CRAFT_MAT_NONE, grade;

  if (!node_harvest_recheck(ch, target, context))
  {
    if (ch)
      send_to_char(ch, "The node has been depleted.\r\n");
    return;
  }
  drop = node_drop_prototype(harvest->material, NULL);
  drop_rnum = drop == NOTHING ? NOTHING : real_object(drop);
  if (drop_rnum == NOTHING)
  {
    send_to_char(ch, "Nothing useful can be taken from this node; please report it.\r\n");
    log("SYSERR: node harvest: material %d rolled missing prototype %d", harvest->material,
        (int)drop);
    return;
  }
  balance = craft_material_from_object(&obj_proto[drop_rnum]);
  if (balance != CRAFT_MAT_NONE)
  {
    if (!craft_balance_add(ch, balance, 1))
    {
      send_to_char(ch, "Your crafting storage cannot hold any more %s.\r\n",
                   crafting_materials[balance]);
      return;
    }
    send_to_char(ch, "Your efforts in the area yield 1 unit of %s.\r\n",
                 crafting_materials[balance]);
    act("$n's efforts in the area yield some material.", FALSE, ch, 0, 0, TO_ROOM);
    grade = material_grade(balance);
  }
  else
  {
    reward = IS_CARRYING_N(ch) >= CAN_CARRY_N(ch) ? NULL : read_object(drop, VIRTUAL);
    if (!reward)
    {
      send_to_char(ch, "You must drop something before you can take what this node yields.\r\n");
      return;
    }
    if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(reward) > CAN_CARRY_W(ch))
    {
      extract_obj(reward);
      send_to_char(ch, "You must lighten your load before you can take what this node yields.\r\n");
      return;
    }
    obj_to_char(reward, ch);
    act("Your efforts in the area result in: $p.", FALSE, ch, reward, 0, TO_CHAR);
    act("$n's efforts in the area result in: $p.", FALSE, ch, reward, 0, TO_ROOM);
    /* A rare object pays by the node's ordinary material grade. */
    grade = material_grade(obj_material_to_craft_material(harvest->material));
  }
  grade = MAX(1, grade);
  autoquest_trigger_check(ch, NULL, NULL, 0, node_quest_type(harvest->sub_command));
  gain_craft_exp(ch, 20 + 10 * grade, harvest->skill, TRUE);
  node_spend_charge(node, harvest->sub_command);
}

/* the 'harvest' command */
ACMD(do_harvest)
{
  struct obj_data *node = NULL;
  struct primary_activity_definition definition = {0};
  struct node_harvest_context *harvest;
  int material = -1, minskill = 0;
  int skillnum = 0;
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  char buf[MEDIUM_STRING] = {'\0'};
  int sub_command = SCMD_CRAFT_UNDF;

  /* Explicit legacy nodes retain their original path. Category harvests store
   * crafting balances and do not depend on physical inventory capacity. */
  if (IN_ROOM(ch) != NOWHERE && IN_ROOM(ch) <= top_of_world &&
      ZONE_FLAGGED(world[IN_ROOM(ch)].zone, ZONE_WILDERNESS))
  {
    one_argument(argument, arg, sizeof(arg));
    node = *arg ? get_obj_in_list_vis(ch, arg, NULL, world[IN_ROOM(ch)].contents) : NULL;
    if (!node || GET_OBJ_VNUM(node) != HARVESTING_NODE)
    {
      do_wilderness_harvest(ch, argument, cmd, subcmd);
      return;
    }
    node = NULL;
  }

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
    if (try_wilderness_harvest_fallback(ch, argument, cmd, subcmd))
      return;
    send_to_char(ch, "You need to specify what you want to harvest.\r\n");
    return;
  }

  if (!(node = get_obj_in_list_vis(ch, arg, NULL, world[IN_ROOM(ch)].contents)))
  {
    if (try_wilderness_harvest_fallback(ch, argument, cmd, subcmd))
      return;
    send_to_char(ch, "That doesn't seem to be present in this room.\r\n");
    return;
  }

  if (GET_OBJ_VNUM(node) != HARVESTING_NODE)
  {
    if (try_wilderness_harvest_fallback(ch, argument, cmd, subcmd))
      return;
    send_to_char(ch, "That is not a harvesting node.\r\n");
    return;
  }

  material = GET_OBJ_MATERIAL(node);
  skillnum = node_harvest_skill(material, &sub_command);
  minskill = node_minimum_skill(material);
  if (minskill < 0)
  {
    send_to_char(ch,
                 "That is not a valid node type, please report this to a staff member [1].\r\n");
    return;
  }

  /* Node thresholds are written in legacy units; ranks read through the equivalent. */
  if (craft_legacy_skill_equivalent(ch, skillnum) < minskill)
  {
    send_to_char(ch, "You need a minimum %s rank of %d, while yours is only %d.\r\n",
                 ability_names[skillnum],
                 (minskill + CRAFT_LEGACY_SKILL_PER_RANK - 1) / CRAFT_LEGACY_SKILL_PER_RANK,
                 get_craft_skill_value(ch, skillnum));
    return;
  }
  if (GET_OBJ_VAL(node, 0) <= 0)
  {
    send_to_char(ch, "That node has been depleted.\r\n");
    return;
  }
  if (!is_action_available(ch, atSTANDARD, FALSE) || !is_action_available(ch, atMOVE, FALSE))
  {
    send_to_char(ch, "You must recover your full round before harvesting.\r\n");
    return;
  }

  /* Nothing is allocated, spent, or rolled at admission: the reward, the charge, and the
   * experience all wait for completion, so cancelling costs nothing and pays nothing. */
  CREATE(harvest, struct node_harvest_context, 1);
  harvest->material = material;
  harvest->skill = skillnum;
  harvest->sub_command = sub_command;
  harvest->room = IN_ROOM(ch);
  snprintf(buf, sizeof(buf), "harvesting %s",
           node->short_description ? node->short_description : "a node");
  definition.type = PRIMARY_ACTIVITY_HARVEST;
  definition.display_name = buf;
  definition.capabilities = PRIMARY_ACTIVITY_CAP_HANDS | PRIMARY_ACTIVITY_CAP_ATTENTION |
                            PRIMARY_ACTIVITY_CAP_STANDARD | PRIMARY_ACTIVITY_CAP_MOVE;
  definition.traits = PRIMARY_ACTIVITY_TRAIT_STATIONARY | PRIMARY_ACTIVITY_TRAIT_HANDS_OCCUPIED |
                      PRIMARY_ACTIVITY_TRAIT_OBVIOUS;
  definition.progress_model = PRIMARY_ACTIVITY_PROGRESS_PROGRESSIVE;
  definition.progress_owner = PRIMARY_ACTIVITY_PROGRESS_CHARACTER;
  definition.total_steps = NODE_HARVEST_STEPS;
  definition.step_interval = PULSE_VIOLENCE;
  definition.combat_actions_required = ACTION_STANDARD | ACTION_MOVE;
  definition.movement_response = PRIMARY_ACTIVITY_RESPONSE_CANCEL;
  definition.damage_response = PRIMARY_ACTIVITY_RESPONSE_CANCEL;
  definition.combat_response = PRIMARY_ACTIVITY_RESPONSE_CANCEL;
  definition.target_loss_response = PRIMARY_ACTIVITY_RESPONSE_CANCEL;
  definition.command_response = PRIMARY_ACTIVITY_RESPONSE_REJECT;
  definition.cannot_pause = true;
  definition.recheck = node_harvest_recheck;
  definition.complete = node_harvest_complete;
  definition.cleanup_context = free;
  definition.context = harvest;
  if (!primary_activity_start(ch, domain_event_object_handle(node), &definition))
  {
    free(harvest);
    send_to_char(ch, "You are already occupied or cannot begin harvesting right now.\r\n");
    return;
  }

  act("You begin to harvest $p.", FALSE, ch, node, NULL, TO_CHAR);
  act("$n begins to harvest $p.", FALSE, ch, node, NULL, TO_ROOM);
  USE_FULL_ROUND_ACTION(ch);
}

int get_mysql_supply_orders_available(struct char_data *ch)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  char buf[MAX_STRING_LENGTH];
  int avail = 0;

  /* Ensure database connection is active */
  if (!MYSQL_PING_CONN(conn))
  {
    log("SYSERR: %s: Database connection failed", __func__);
    return 0;
  }

  char *escaped_name = mysql_escape_string_alloc(conn, GET_NAME(ch));
  if (!escaped_name)
  {
    log("SYSERR: Failed to escape player name in get_avail_supply_orders");
    return -1;
  }
  snprintf(buf, sizeof(buf),
           "SELECT supply_orders_available FROM player_supply_orders WHERE player_name='%s'",
           escaped_name);
  free(escaped_name);

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

  avail = parse_int(row[0]);

  mysql_free_result(result);

  return avail;
}

void put_mysql_supply_orders_available(struct char_data *ch, int avail)
{
  char buf[MAX_STRING_LENGTH];

  /* Ensure database connection is active */
  if (!MYSQL_PING_CONN(conn))
  {
    log("SYSERR: %s: Database connection failed", __func__);
    return;
  }

  char *escaped_name = mysql_escape_string_alloc(conn, GET_NAME(ch));
  if (!escaped_name)
  {
    log("SYSERR: Failed to escape player name in put_mysql_supply_orders_available");
    return;
  }
  snprintf(buf, sizeof(buf), "DELETE FROM supply_orders_available WHERE player_name='%s'",
           escaped_name);

  mysql_query(conn, buf);

  snprintf(buf, sizeof(buf),
           "INSERT INTO supply_orders_available (idnum, player_name, supply_orders_available) "
           "VALUES(NULL, '%s', '%d')",
           escaped_name, avail);
  free(escaped_name);

  if (mysql_query(conn, buf))
  {
    log("SYSERR: Unable to INSERT INTO player_supply_orders: %s", mysql_error(conn));
  }
}

ACMD(do_need_craft_kit)
{
  send_to_char(ch, "You must be in a room with a crafting station or have a crafting kit in your "
                   "inventory to perform this action.\r\n");
}
