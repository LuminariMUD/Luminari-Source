/*
 * Crafting System
 * 
 * From d20MUD
 * Ported and re-written by Zusuk
 * 
 * utils.h has most of the header info
 */

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
#include "utils.h"
#include "comm.h"
#include "spells.h"
#include "interpreter.h"
#include "constants.h"
#include "handler.h"
#include "db.h"
#include "spells.h"


// External Functions
void scaleup_dam(int *num, int *size);
void scaledown_dam(int *num, int *size);
void get_random_essence(struct char_data *ch, int level);
void get_random_crystal(struct char_data *ch, int level);
int art_level_exp(int level);
/**********************/

//DEFINES//
/* number of mats needed to complete a supply order */
#define SUPPLYORDER_MATS     3
/* craft_pattern_vnums() determines pattern vnums, but you */
/* can modify the list of patterns here (i.e. molds) */
#define PATTERN_UPPER     30299
#define PATTERN_LOWER     30200
/* regardless of object weight, minimum needed mats to create an object */
#define MIN_MATS          5
/* regardless of object weight, minimum needed mats to create an object
   for those with 'elf-crafting' feat */
#define MIN_ELF_MATS      2
/* Amount of material needed: [mold weight divided by weight_factor] */
#define WEIGHT_FACTOR     50
/* Max level of a crystal in determining bonus */
#define CRYSTAL_CAP       (LVL_IMMORT-1)
/* Crystal bonus division factor, ex. level 30 = +6 bonus (factor of 5) */
#define BONUS_FACTOR      5
/* Maximum crit rolls you can get on crafting */
#define MAX_CRAFT_CRIT    3
#define AUTOCQUEST_VNUM     30084  /* set your autoquest object here */
#define AUTOCQUEST_MAKENUM      5  /* how many objects needed to craft */
// end DEFINES //

/* crafting local utility functions*/

/* simple function to reset craft data */
void reset_craft(struct char_data *ch) {
  /* initialize values */
  GET_CRAFTING_TYPE(ch) = -1; // SCMD_ of craft
  GET_CRAFTING_TICKS(ch)= -1;
  GET_CRAFTING_OBJ(ch) = NULL;
  GET_CRAFTING_REPEAT(ch) = 0;
}

/* simple function to reset auto craft data */
void reset_acraft(struct char_data *ch) {
  /* initialize values */
  GET_AUTOCQUEST_VNUM(ch) = 0;
  GET_AUTOCQUEST_MAKENUM(ch) = 0;
  GET_AUTOCQUEST_QP(ch) = 0;
  GET_AUTOCQUEST_EXP(ch) = 0;
  GET_AUTOCQUEST_GOLD(ch) = 0;
  GET_AUTOCQUEST_MATERIAL(ch) = 0;
  free(GET_AUTOCQUEST_DESC(ch)); // I have no idea if this is actually needed
  GET_AUTOCQUEST_DESC(ch) = strdup("nothing");  
}


void cquest_report(struct char_data *ch) {
  if (GET_AUTOCQUEST_VNUM(ch)) {
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
                     "%d credits will be deposited into your bank account.\r\n"
                     "You will receive %d experience points.\r\n", 
                 GET_AUTOCQUEST_QP(ch), GET_AUTOCQUEST_GOLD(ch), 
                 GET_AUTOCQUEST_EXP(ch));
  } else
    send_to_char(ch, "Type 'supplyorder new' for a new supply order, "
                     "'supplyorder complete' to finish your supply "
                     "order and\r\n"
                     "receive your reward or 'supplyorder quit' to quit your "
                     "supply order.\r\n");
}


/* this function determines the factor of bonus for crystal_value/level */
int crystal_bonus(struct obj_data *crystal, int mod)
{
  int bonus = mod + (GET_OBJ_LEVEL(crystal) / BONUS_FACTOR);
    
  switch (GET_OBJ_VAL(crystal, 0)) {
                 
  case APPLY_CHAR_WEIGHT:
  case APPLY_CHAR_HEIGHT:
    bonus *= 10;
    break;
  
  case APPLY_HIT:
    bonus *= 5;
    break;
  
  case APPLY_DAMROLL:
    bonus += 2;
    break;
    
  default: // default - unmodified
    break;
  }

  return bonus;
}


/************************/
/* start primary engine */
/************************/


// combine crystals to make them stronger
int augment(struct obj_data *station, struct char_data *ch)
{
  struct obj_data *obj = NULL, *crystal_one = NULL, *crystal_two = NULL;
  int num_objs = 0, cost = 0, bonus_one = 0, bonus_two = 0;
  int skill_type = SKILL_CHEMISTRY; // change this to change the skill used
  char buf[MAX_INPUT_LENGTH];
  
  // Cycle through contents and categorize
  for (obj = station->contains; obj != NULL; obj = obj->next_content) {
    num_objs++;
    if (obj && num_objs > 2) {
      send_to_char(ch, "Make sure only two items are in the station.\r\n");
      return 1;
    }
    if (obj && GET_OBJ_TYPE(obj) == ITEM_CRYSTAL && !crystal_one) {
      crystal_one = obj;
      bonus_one = GET_OBJ_LEVEL(crystal_one);
    } else if (obj && GET_OBJ_TYPE(obj) == ITEM_CRYSTAL && !crystal_two) {
      crystal_two = obj;
      bonus_two = GET_OBJ_LEVEL(crystal_two);
      break; // found everything we need
    }
  }
      
  if (!crystal_one || !crystal_two) {
    send_to_char(ch, "You need two crystals to augment.\r\n");
    return 1;
  }
  if (apply_types[GET_OBJ_VAL(crystal_one, 0)] !=
      apply_types[GET_OBJ_VAL(crystal_two, 0)]) {
    send_to_char(ch, "The crystal 'apply type' needs to be the same to"
                     " augment.\r\n");
    return 1;
  }
  // if the crystals aren't at least 2nd level, you are going to create junk
  if (GET_OBJ_LEVEL(crystal_one) < 2 || GET_OBJ_LEVEL(crystal_two) < 2 ) {
    send_to_char(ch, "These crystals are too weak to augment together.\r\n");
    return 1;
  }
  // new level of crystal, with cap
  bonus_one = (bonus_one + bonus_two) * 3 / 4;
  if (bonus_one >= (LVL_IMMORT - 1))
    bonus_one = LVL_IMMORT - 2;
  // high enough skill?
  if (bonus_one > GET_SKILL(ch, skill_type)) {
    send_to_char(ch, "The crystal level is %d but your %s skill is "
                     "only %d.\r\n",
                 bonus_one, spell_info[skill_type].name,
                 GET_SKILL(ch, skill_type));
    return 1;
  }
  // expense for augmenting
  cost = GET_OBJ_LEVEL(crystal_one) * GET_OBJ_LEVEL(crystal_two) * 100 / 3;
  if (GET_GOLD(ch) < cost) {
    send_to_char(ch, "You need %d coins on hand for supplies to augment this "
                     "crystal.\r\n", cost);
    return 1;
  }
  
  // crystal_one is converted to the new crystal
  GET_OBJ_LEVEL(crystal_one) = bonus_one;
  GET_OBJ_COST(crystal_one) = GET_OBJ_COST(crystal_one) + 
                              GET_OBJ_COST(crystal_two);
  // exp bonus for crafting ticks
  GET_CRAFTING_BONUS(ch) = 10 + MIN(60, GET_OBJ_LEVEL(crystal_one));
  // new name
  sprintf(buf, "@wa crystal of@y %s@n max level@y %d@n",
          apply_types[GET_OBJ_VAL(crystal_one, 0)], GET_OBJ_LEVEL(crystal_one));
  crystal_one->name = strdup(buf);
  crystal_one->short_description = strdup(buf);
  sprintf(buf, "@wA crystal of@y %s@n max level@y %d@n lies here.",
          apply_types[GET_OBJ_VAL(crystal_one, 0)], GET_OBJ_LEVEL(crystal_one));
  crystal_one->description = strdup(buf);
   
  send_to_char(ch, "It cost you %d coins in supplies to augment this crytsal.\r\n",
               cost);
  GET_GOLD(ch) -= cost;
    
  GET_CRAFTING_TYPE(ch) = SCMD_CRAFT;
  GET_CRAFTING_TICKS(ch) = 5;  // add code here to modify speed of crafting
  GET_CRAFTING_OBJ(ch) = crystal_one;
  send_to_char(ch, "You begin to augment %s.\r\n", 
               crystal_one->short_description);
  act("$n begins to augment $p.", FALSE, ch, crystal_one, 0, TO_ROOM);
  
  // get rid of the items in the station
  obj_from_obj(crystal_one);
  extract_obj(crystal_two);
   
  /* zusuk - temporary */
  obj_to_char(crystal_one, ch);

  return 1;
}
  
// convert one material into another
// requires multiples of exactly 10 of same mat to do the converstion
int convert(struct obj_data *station, struct char_data *ch)
{
  int cost = 500;  /* flat cost */
  int num_mats = 0, material = -1, obj_vnum = 0;  
  struct obj_data *new_mat = NULL, *obj = NULL;
   
  /* Cycle through contents and categorize */
  /* Cycle through contents and categorize */
  for (obj = station->contains; obj != NULL; obj = obj->next_content) {
    if (obj) {
      if (GET_OBJ_TYPE(obj) != ITEM_MATERIAL) {
        send_to_char(ch, "Only materials should be inside the station in"
                         " order to convert.\r\n");
        return 1;
      } else if (GET_OBJ_TYPE(obj) == ITEM_MATERIAL) {
        if (GET_OBJ_VAL(obj, 0) >= 2) {
          send_to_char(ch, "%s is a bundled item, which must first be "
                           "unbundled before you can use it to craft.\r\n",
                       obj->short_description);
          return 1;
        }
        if (material == -1) /* first item */
          material = GET_OBJ_MATERIAL(obj);
        else if (GET_OBJ_MATERIAL(obj) != material) {
          send_to_char(ch, "You have mixed materials inside the station, "
                           "put only the exact same materials for "
                           "conversion.\r\n");
          return 1;
        }
        num_mats++; /* we found matching material */
        obj_vnum = GET_OBJ_VNUM(obj);
      }
    }
  } 
  
  if (num_mats) {
    if (num_mats % 10) {
      send_to_char(ch, "You must convert materials in multiple "
                       "of 10 units exactly.\r\n");
      return 1;
    }
  } else {
    send_to_char(ch, "There is no material in the station.\r\n");
    return 1;
  }
  
  if (convert_material_vnum(obj_vnum))
    new_mat = read_object(convert_material_vnum(obj_vnum), VIRTUAL);
  else {
    send_to_char(ch, "You do not have a valid material in the crafting "
                     "station.\r\n");
    return 1;
  }
  
  if (GET_GOLD(ch) < cost) {
    send_to_char(ch, "You need %d credits on hand for supplies to covert these "
                     "materials.\r\n", cost);
    return 1;
  }
  send_to_char(ch, "It cost you %d credits in supplies to convert this "
                   "item.\r\n", cost);
  GET_GOLD(ch) -= cost;
        
  send_to_char(ch, "You begin to convert your materials into %s.\r\n",
               new_mat->short_description);
  act("$n begins a conversion of materials into $p.", FALSE, ch,
      new_mat, 0, TO_ROOM);
                       
  GET_CRAFTING_BONUS(ch) = 10 + MIN(60, GET_OBJ_LEVEL(new_mat));
  GET_CRAFTING_TYPE(ch) = SCMD_CRAFT;
  GET_CRAFTING_TICKS(ch) = 6;  // adding time-takes here
  GET_CRAFTING_TICKS(ch) -= MAX(10, GET_SKILL(ch, SKILL_FAST_CRAFTER));
  GET_CRAFTING_OBJ(ch) = new_mat;
  GET_CRAFTING_REPEAT(ch) = MAX(0, (num_mats / 10) + 1);
                           
  obj_vnum = GET_OBJ_VNUM(station);
  obj_from_room(station);
  extract_obj(station);
  station = read_object(obj_vnum, VIRTUAL);
  obj_to_room(station, IN_ROOM(ch));

  /* zusuk - temporary */
  obj_to_char(new_mat, ch);
  
  return 1;
} 
  
/* rename an object */  
int restring(char *argument, struct obj_data *station, struct char_data *ch) {
  int num_objs = 0, cost;
  struct obj_data *obj = NULL;
  char buf[MAX_INPUT_LENGTH];
  
  /* Cycle through contents */
  /* restring requires just one item be inside the station */
  for (obj = station->contains; obj != NULL; obj = obj->next_content) {
    num_objs++;
    break;
  }
  
  if (num_objs > 1) {
    send_to_char(ch, "Only one item should be inside the station.\r\n");
    return 1;
  }
  
  if (obj->ex_description) {
    send_to_char(ch, "You cannot restring items with extra descriptions.\r\n"); 
    return 1;
    send_to_char(ch, "You cannot restring items with extra descriptions.\r\n"); 
    return 1;
  }
   
  if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER) {
    if (obj->contains) {
      send_to_char(ch, "You cannot restring bags that have items in them.\r\n");
      return 1;
    }
  }
  
  if (!strstr(argument, material_name[GET_OBJ_MATERIAL(obj)])) {
    send_to_char(ch, "You must include the material name, '%s', in the object "
                     "description somewhere.\r\n",
                 material_name[GET_OBJ_MATERIAL(obj)]);
    return 1;
  }
  
  cost = GET_OBJ_COST(obj) + GET_OBJ_LEVEL(obj);
  if (GET_GOLD(ch) < cost) {
    send_to_char(ch, "You need %d credits on hand for supplies to restring"
                     " this item.\r\n", cost);
    return 1;
  }
  
  /* success!! */
  obj->name = strdup(argument);
  obj->short_description = strdup(argument);
  sprintf(buf, "%s lies here.", CAP(argument));
  obj->description = strdup(buf);
  GET_CRAFTING_TYPE(ch) = SCMD_CRAFT;
  GET_CRAFTING_TICKS(ch) = 5; // here you'd add tick calculator
  GET_CRAFTING_OBJ(ch) = obj;
  
  send_to_char(ch, "It cost you %d credits in supplies to create this item.\r\n",
               cost);
  GET_GOLD(ch) -= cost;
  send_to_char(ch, "You put the item into the crafting station and wait for it "
                    "to transform into %s.\r\n", obj->short_description);
   
  obj_from_obj(obj);

  /* zusuk - temporary */
  obj_to_char(obj, ch);
  
  return 1;
}


/* supplyorder - crafting quest command */   
int supplyorder(struct obj_data *station, struct char_data *ch) {
  int material, obj_vnum, num_mats = 0, material_level = 1;
  struct obj_data *obj = NULL;
  
  if (!GET_AUTOCQUEST_MATERIAL(ch)) {
    send_to_char(ch, "You do not have a supply order active right now.\r\n");   
    return 1;  
  }  
  if (!GET_AUTOCQUEST_MAKENUM(ch)) {
    send_to_char(ch, "You have completed your supply order, go turn it in.\r\n");
    return 1;
  }
                     
  material = GET_AUTOCQUEST_MATERIAL(ch);
    
  /* Cycle through contents and categorize */
  for (obj = station->contains; obj != NULL; obj = obj->next_content) {
    if (obj) {
      if (GET_OBJ_TYPE(obj) != ITEM_MATERIAL) {
        send_to_char(ch, "Only materials should be inside the station in"  
                         " order to complete a supplyorder.\r\n");
        return 1;
      } else if (GET_OBJ_TYPE(obj) == ITEM_MATERIAL) {
        if (GET_OBJ_VAL(obj, 0) >= 2) {
          send_to_char(ch, "%s is a bundled item, which must first be "
                           "unbundled before you can use it to craft.\r\n",
                       obj->short_description);
          return 1;
        }
        if (GET_OBJ_MATERIAL(obj) != material) {
          send_to_char(ch, "You need %s to complete this supplyorder.\r\n",
                       material_name[GET_AUTOCQUEST_MATERIAL(ch)]);
          return 1;
        }
        material_level = GET_OBJ_LEVEL(obj); /* material level affects bonus gold */
        obj_vnum = GET_OBJ_VNUM(obj);
        num_mats++; /* we found matching material */
        if (num_mats > SUPPLYORDER_MATS) {
          send_to_char(ch, "You have too much materials in the station, put "   
                           "exactly %d for the supplyorder.\r\n", SUPPLYORDER_MATS);
          return 1;
        }
      } else { /* must be an essence */
        send_to_char(ch, "Essence items will not work for supplyorders!\r\n");
        return 1;
      }
    }
  }

  if (num_mats < SUPPLYORDER_MATS) {
    send_to_char(ch, "You have %d material units in the station, you will need "
                     "%d more units to complete the supplyorder.\r\n",
                 num_mats, SUPPLYORDER_MATS - num_mats);
    return 1;
  }
    
  obj = read_object(30084, VIRTUAL);
  obj->name = strdup(GET_AUTOCQUEST_DESC(ch));
  obj->description = strdup(GET_AUTOCQUEST_DESC(ch));
  obj->short_description = strdup(GET_AUTOCQUEST_DESC(ch));
   
  GET_AUTOCQUEST_GOLD(ch) += GET_LEVEL(ch) * GET_LEVEL(ch);
  
  /*     this is where we determine level of object
  if (GET_ARTISAN_TYPE(ch) == ARTISAN_TYPE_TINKERING)
    GET_OBJ_LEVEL(obj) = get_skill_value(ch, SKILL_TINKERING);
  else if (GET_ARTISAN_TYPE(ch) == ARTISAN_TYPE_WEAPONTECH)
    GET_OBJ_LEVEL(obj) = get_skill_value(ch, SKILL_WEAPONTECH);
  else if (GET_ARTISAN_TYPE(ch) == ARTISAN_TYPE_ARMORTECH)
    GET_OBJ_LEVEL(obj) = get_skill_value(ch, SKILL_ARMORTECH);
  else
*/
    GET_OBJ_LEVEL(obj) = 1;
          
  GET_CRAFTING_TYPE(ch) = SCMD_CRAFT;
  GET_CRAFTING_TICKS(ch) = 5;
  GET_CRAFTING_OBJ(ch) = obj;
  send_to_char(ch, "You begin a supply order for %s.\r\n",
    obj->short_description);
  act("$n begins a supply order for $p.", FALSE, ch, obj, 0, TO_ROOM);
          
  obj_vnum = GET_OBJ_VNUM(station);
  obj_from_room(station);
  extract_obj(station);
  station = read_object(obj_vnum, VIRTUAL);
  obj_to_room(station, IN_ROOM(ch));

  /* zusuk - temporary */
  obj_to_char(obj, ch);

  return 1;
}
         
int resize(char *argument, struct obj_data *station, struct char_data *ch) {
  int num_objs = 0, newsize, cost, i;
  struct obj_data *obj = NULL;
       
  /* Cycle through contents */
  /* resize requires just one item be inside the station */
  for (obj = station->contains; obj != NULL; obj = obj->next_content) {
    num_objs++;
    break;
  }
                 
  if (num_objs > 1) {
    send_to_char(ch, "Only one item should be inside the station.\r\n");
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
  else {
    send_to_char(ch, "That is not a valid size: (fine|diminutive|tiny|small|"
                               "medium|large|huge|gargantuan|colossal)\r\n");
    return 1;
  }
          
  if (newsize == obj->size) {
    send_to_char(ch, "The object is already the size you desire.\r\n");
    return 1;
  }
  
  /* "cost" of resizing */
  cost = GET_OBJ_COST(obj) / 2;
  if (GET_GOLD(ch) < cost) {
    send_to_char(ch, "You need %d coins on hand for supplies to make this "
                                     "item.\r\n", cost);
    return 1;
  }
  send_to_char(ch, "It cost you %d coins to resize this item.\r\n",
                       cost); 
  GET_GOLD(ch) -= cost;
  
  /* weapon damage adjustment */
  if (GET_OBJ_TYPE(obj) == ITEM_WEAPON) {
    int ndice, diesize, sz, lvl;
                 
    ndice = GET_OBJ_VAL(obj, 1);
    diesize = GET_OBJ_VAL(obj, 2);
    
    sz = newsize - obj->size;
    if (!sz) /* should never pass this test */
      send_to_char(ch, "ERROR:  Report to Staff Error: Weapon Dam Adjustment\r\n");
    else if (sz > 0)
      for (lvl = 0; lvl < sz; lvl++)
        scaleup_dam(&ndice, &diesize);
    else {
      sz *= -1;
      for (lvl = 0; lvl < sz; lvl++)
        scaledown_dam(&ndice, &diesize);
    }
    
    send_to_char(ch, "Old weapond dice: %dd%d, New weapons dice: %dd%d.\r\n",
                 GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2),
                 ndice, diesize);
    GET_OBJ_VAL(obj, 1) = ndice;
    GET_OBJ_VAL(obj, 2) = diesize;
  }
  
  send_to_char(ch, "You resize %s from %s to %s.\r\n",
               obj->short_description, size_names[obj->size],
               size_names[newsize]);
  act("$n resizes $p.", FALSE, ch, obj, 0, TO_ROOM);
  obj_from_obj(obj);
   
  /* resize object after taking out of station, otherwise issues */
  /* weight adjustment of object */
  obj->size = newsize;
  for (i = 0; i < newsize - obj->size; i++)
    GET_OBJ_WEIGHT(obj) = GET_OBJ_WEIGHT(obj) * 3 / 2;
  for (i = 0; i < obj->size - newsize; i++)
    GET_OBJ_WEIGHT(obj) = GET_OBJ_WEIGHT(obj) * 2 / 3;
  
  obj_to_char(obj, ch);
    
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
int create(char *argument, struct obj_data *station, struct char_data *ch, int mode) {
  struct obj_data *obj = NULL, *mold = NULL, *crystal = NULL,
                  *material = NULL, *essence = NULL;
  int num_mats = 0, obj_level = 1, skill = -1, crystal_value = -1,
      mats_needed = 12345, found = 0, i = 0, bonus = 0;

  /* sort through our station and check if we got everything we need */
  for (obj = station->contains; obj != NULL; obj = obj->next_content) {
    if (obj) {
      /* find a mold? */
      if (OBJ_FLAGGED(obj, ITEM_MOLD)) {
        if (!mold) {
          mold = obj;
          found++;
        } else {
          send_to_char(ch, "You have more than one mold inside the station, "
                               "please only put one inside.\r\n");
          return 1;
        }
      }
    
      if (found) /* we didn't have a mold and found one, interate main loop */
        continue;
  
      /* find a crystal? */
      if (GET_OBJ_TYPE(obj) == ITEM_CRYSTAL) {
        if (!crystal) {
          crystal = obj;
        } else {
          send_to_char(ch, "You have more than one crystal inside the station, "
                           "please only put one inside.\r\n");
          return 1;
        }
   
      /* find a material? */
      } else if (GET_OBJ_TYPE(obj) == ITEM_MATERIAL) {
        if (GET_OBJ_VAL(obj, 0) >= 2) {
          send_to_char(ch, "%s is a bundled item, which must first be unbundled "
                           "before you can use it to craft.\r\n",
                       obj->short_description);
          return 1;
        }
        if (!material) {
          material = obj;
          num_mats++;
        } else if (GET_OBJ_MATERIAL(obj) != GET_OBJ_MATERIAL(material)) {
          send_to_char(ch, "You have mixed materials in the station, please "
                           "make sure to use only the required materials.\r\n");
          return 1;
        } else { /* this should be good */
          num_mats++;   
        }
        
      /* find an essence? */
      } else if (GET_OBJ_TYPE(obj) == ITEM_ESSENCE) {
        if (!essence) {
          essence = obj;
        } else {
          send_to_char(ch, "You have more than one essence inside the station, "
                           "please only put one inside.\r\n");
          return 1;   
        }
              
      } else { /* didn't find anything we need */
        send_to_char(ch, "There is an unnecessary item in the station, please "
                         "remove it.\r\n");
        return 1;
      }  
    }  
  } /* end our sorting loop */
      
  /** check we have all the ingredients we need **/
  if (!mold) {
    send_to_char(ch, "The creation process requires a mold to continue.\r\n");
    return 1;
  }
  obj_level = GET_OBJ_LEVEL(mold);
  if (!material) {
    send_to_char(ch, "You need to put materials into the station.\r\n");
    return 1;
  }
         
  /* right material? */
  if (GET_SKILL(ch, SKILL_BONE_ARMOR) &&
             GET_OBJ_MATERIAL(material) == MATERIAL_BONE) {
    send_to_char(ch, "You use your mastery in bone-crafting to substitutue "
                     "bone for the normal material needed...\r\n");
  } else if (IS_CLOTH(GET_OBJ_MATERIAL(mold)) &&
             !IS_CLOTH(GET_OBJ_MATERIAL(material))) {
    send_to_char(ch, "You need cloth for this mold pattern.\r\n");
    return 1;
  } else if (IS_LEATHER(GET_OBJ_MATERIAL(mold)) &&
             !IS_LEATHER(GET_OBJ_MATERIAL(material))) {
    send_to_char(ch, "You need leather for this mold pattern.\r\n");
    return 1;
  } else if (IS_WOOD(GET_OBJ_MATERIAL(mold)) &&
             !IS_WOOD(GET_OBJ_MATERIAL(material))) {
    send_to_char(ch, "You need wood for this mold pattern.\r\n");
    return 1;
  } else if (IS_HARD_METAL(GET_OBJ_MATERIAL(mold)) &&
             !IS_HARD_METAL(GET_OBJ_MATERIAL(material))) {
    send_to_char(ch, "You need hard metal for this mold pattern.\r\n");
    return 1;
  } else if (IS_PRECIOUS_METAL(GET_OBJ_MATERIAL(mold)) &&
             !IS_PRECIOUS_METAL(GET_OBJ_MATERIAL(material))) {
    send_to_char(ch, "You need precious metal for this mold pattern.\r\n");
    return 1;   
  } else {
    send_to_char(ch, "This material is not currently replicatable.\r\n");
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
  if (GET_SKILL(ch, SKILL_ELVEN_CRAFTING))
    mats_needed = MAX(MIN_ELF_MATS, mats_needed / 2);
  if (num_mats < mats_needed) {
    send_to_char(ch, "You do not have enough materials to make that item.  "
                     "You need %d more units of the same type.\r\n",
                 mats_needed - num_mats);
    return 1; 
  } else if (num_mats > mats_needed) {
    send_to_char(ch, "You put too much material in the station, please "
                     "take out %d units.\r\n", num_mats - mats_needed);
    return 1;
  }
    
  /** check for other disqualifiers */
  /* valid name */
  if (mode == 1 && !strstr(argument, 
      material_name[GET_OBJ_MATERIAL(material)])) {
    send_to_char(ch, "You must include the material name, '%s', in the object "
                     "description somewhere.\r\n", material_name[GET_OBJ_MATERIAL(material)]);
             
    return 1;
  }
  
  /*** valid crystal usage ***/
  if (crystal) {
    crystal_value = GET_OBJ_VAL(crystal, 0);
      
    if (crystal_value == APPLY_HITROLL &&
        !CAN_WEAR(mold, ITEM_WEAR_HANDS)) {
      send_to_char(ch, "You can only imbue gauntlets or gloves with a "
                       "hitroll enhancement.\r\n");
      return 1;
    }
             
    if ((crystal_value == APPLY_HITROLL ||
        crystal_value == APPLY_DAMROLL) &&
        !CAN_WEAR(mold, ITEM_WEAR_WIELD)) {
      send_to_char(ch, "You cannot imbue a non-weapon with weapon bonuses.\r\n");
      return 1;
    }
      
    /* for skill restriction and object level */
    obj_level += GET_OBJ_LEVEL(crystal);
  
    /* determine crystal bonus, etc */
    int mod = 0;
    for (i = 0; i <= MAX_CRAFT_CRIT; i++) {
      if (dice(1,100) < 1)
        mod++;
      if (mod)
        send_to_char(ch, "@l@WYou have received a critical success on your "
                       "craft! (+%d)@n\r\n", mod); 
    }
    if (GET_SKILL(ch, SKILL_MASTERWORK_CRAFTING)) {
      send_to_char(ch, "Your masterwork-crafting skill increases the quality of "
                       "the item.\r\n");   
      mod++;
    }
    if (GET_SKILL(ch, SKILL_DWARVEN_CRAFTING)) {
      send_to_char(ch, "Your dwarven-crafting skill increases the quality of "
                       "the item.\r\n");
      mod++;
    }
    if (GET_SKILL(ch, SKILL_DRACONIC_CRAFTING)) {
      send_to_char(ch, "Your draconic-crafting skill increases the quality of "  
                       "the item.\r\n");
      mod++;
    }
    bonus = crystal_bonus(crystal, mod);   
  }
  /*** end valid crystal usage ***/
                         
  /* which skill is used for this crafting session? */
  /* for now, skill is determined by TYPE flag */
  /* for now, only dealing with 4 crafting skills: armortech, weapontech,
      tinkering and robotics */
  switch (GET_OBJ_TYPE(mold)) {
        
  /* tinkering */
  case ITEM_TRASH:
  case ITEM_SCROLL:
  case ITEM_WAND:
  case ITEM_MATERIAL:
  case ITEM_CRYSTAL:
    skill = SKILL_JEWELRY_MAKING;
    break;
      
  /* armortech */
  case ITEM_ARMOR:
  case ITEM_WORN:
  case ITEM_OTHER:
  case ITEM_CONTAINER:
  case ITEM_NOTE:
  case ITEM_DRINKCON:
  case ITEM_KEY:
    skill = SKILL_ARMOR_SMITHING;
    break;
                       
  /* robotics */
  case ITEM_LIGHT:
  case ITEM_POTION:
  case ITEM_FOUNTAIN:
  case ITEM_BOAT:
    skill = SKILL_LEATHER_WORKING;
    break;
  
  /* weapontech */
  case ITEM_STAFF:
  case ITEM_WEAPON:
  case ITEM_TREASURE:
  case ITEM_FOOD:
  case ITEM_MONEY:   
  case ITEM_PEN: 
    skill = SKILL_WEAPON_SMITHING;
    break;
  
  default: /* you can add here other categories */
    skill = SKILL_LEATHER_WORKING;
    break;
  }
      
  /* skill restriction */
  if (GET_SKILL(ch, skill) < obj_level) {
    send_to_char(ch, "Your skill in %s is too low to create that item.\r\n",
                 spell_info[skill].name);
    return 1;
  }
  
  int cost = obj_level * obj_level * 100 / 3;
  
  /** passed all the tests, time to check or create the item **/
  if (mode == 2) { /* checkcraft */
    send_to_char(ch, "This crafting session will create the following "
                     "item:\r\n\r\n");
    call_magic(ch, ch, mold, SPELL_IDENTIFY, LVL_IMMORT, CAST_SPELL);
    if (crystal) {     
      send_to_char(ch, "You will be enhancing this value: %s.\r\n",
                   apply_types[crystal_value]);
      send_to_char(ch, "The total bonus will be: %d.\r\n",  bonus);
    }
    send_to_char(ch, "The item will be level: %d.\r\n", obj_level);
    send_to_char(ch, "It will make use of your %s skill, which has a value "
                     "of %d.\r\n",
                 spell_info[skill].name, GET_SKILL(ch, skill));
    send_to_char(ch, "This crafting session will take 60 seconds.\r\n");
    send_to_char(ch, "You need %d credits on hand to make this item.\r\n", cost);
    return 1;
  } else if (GET_GOLD(ch) < cost) {
    send_to_char(ch, "You need %d coins on hand for supplies to make"
                     "this item.\r\n", cost);
    return 1;
  } else { /* create */
    REMOVE_BIT_AR(GET_OBJ_EXTRA(mold), ITEM_MOLD);
    if (essence)  
      SET_BIT_AR(GET_OBJ_EXTRA(mold), ITEM_MAGIC);
    GET_OBJ_LEVEL(mold) = obj_level;
    GET_OBJ_MATERIAL(mold) = GET_OBJ_MATERIAL(material);
    if (crystal) {
      mold->affected[0].location = crystal_value;
      mold->affected[0].modifier = bonus;
    }
    GET_OBJ_COST(mold) = 100 + GET_OBJ_LEVEL(mold) * 50 *
                         MAX(1, GET_OBJ_LEVEL(mold) - 1) +
                         GET_OBJ_COST(mold);
    GET_CRAFTING_BONUS(ch) = 10 + MIN(60, GET_OBJ_LEVEL(mold));
    
    send_to_char(ch, "It cost you %d credits in supplies to create this item.\r\n",
               cost);
    GET_GOLD(ch) -= cost;
  
    /* restringing aspect */
    char buf[MAX_INPUT_LENGTH];
    mold->name = strdup(argument);
    mold->short_description = strdup(argument);
    sprintf(buf, "%s lies here.", CAP(argument));
    mold->description = strdup(buf);
  
    send_to_char(ch, "You begin to craft %s.\r\n", mold->short_description);
    act("$n begins to craft $p.", FALSE, ch, mold, 0, TO_ROOM);

    /*
    if (dice(1, 100) < ((GET_GUILD(ch) != GUILD_ARTISANS) ? 0 :
        ((GET_GUILD_RANK(ch) + 2) / 4 * 5))) {
      send_to_char(ch, "One of %s was refunded to your due to your "
                       "artisan guild bonus.\r\n", material->short_description);
      obj_from_obj(material);
      obj_to_char(material, ch);
    }
    */
    
    GET_CRAFTING_OBJ(ch) = mold;
    obj_from_obj(mold); /* extracting this causes issues, solution? */
    GET_CRAFTING_TYPE(ch) = SCMD_CRAFT;
    GET_CRAFTING_TICKS(ch) = 5;
    int station_obj_vnum = GET_OBJ_VNUM(station);
    obj_from_room(station);
    extract_obj(station);
    station = read_object(station_obj_vnum, VIRTUAL);
    obj_to_room(station, IN_ROOM(ch));
  
    /* zusuk - temporary */
    obj_to_char(obj, ch);

  } 
    
  return 1;
}
                         
                         
SPECIAL(crafting_kit)
{   
    
  if (!CMD_IS("resize") && !CMD_IS("create") && !CMD_IS("checkcraft") &&
      !CMD_IS("restring") && !CMD_IS("augment") && !CMD_IS("convert") &&
      !CMD_IS("supplyorder"))
    return 0;
    
  if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch)) {
    send_to_char(ch, "You cannot craft anything until you've made some "
                     "room in your inventory.\r\n");
    return 1;
  }
  
  if (GET_CRAFTING_OBJ(ch)) {
    send_to_char(ch, "You are already doing something.  Please wait until "
                     "your current task ends.\r\n");
    return 1;
  }
      
  extern int circle_copyover;
  if (circle_copyover) {
    send_to_char(ch, "A hot reboot is scheduled, thus you cannot begin any "
                     "crafting actions.\r\n");
    return 1;
  }
    
  struct obj_data *station = (struct obj_data *) me;
    
  skip_spaces(&argument);
    
  /* Some of the commands require argument */
  if (!*argument && !CMD_IS("checkcraft") && !CMD_IS("augment") &&
      !CMD_IS("supplyorder") && !CMD_IS("convert")) {
    if (CMD_IS("create") || CMD_IS("restring"))
      send_to_char(ch, "Please provide an item description containing the material and item "
                       "name in the string.\r\n");
    else if (CMD_IS("resize"))
      send_to_char(ch, "What would you like the new size to be? (fine|diminutive|tiny|small|"
                       "medium|large|huge|gargantuan|colossal)\r\n");
    return 1;
  }

  if (!station->contains) {
    if (CMD_IS("augment"))
      send_to_char(ch, "You must place at least two crystals of the same type into the "
                       "station in order to augment.\r\n");
    else if (CMD_IS("supplyorder")) {
      if (GET_AUTOCQUEST_MATERIAL(ch))
        send_to_char(ch, "You must place %d units of %s or a similar type of material "
                         "(all the same type) into the station to continue.\r\n",
                     SUPPLYORDER_MATS,
                     material_name[GET_AUTOCQUEST_MATERIAL(ch)]);
      else
        send_to_char(ch, "You do not have a supply order active right now.\r\n");
    } else if (CMD_IS("create"))
      send_to_char(ch, "You must place an item to use as the mold pattern, a crystal and your "
                       "crafting resource materials\r\nin the station and then type 'create "
                       "<optional item description>'\r\n");
    else if (CMD_IS("restring"))
      send_to_char(ch, "You must place the item to restring and in the crafting station.\r\n");
    else if (CMD_IS("resize"))
      send_to_char(ch, "You must place the original item plus enough material in the station "
                       "to resize it.\r\n");
    else if (CMD_IS("checkcraft"))
      send_to_char(ch, "You must place an item to use as the mold pattern, a crystal and "
                       "your crafting resource materials\r\nin the station and then type "
                       "'checkcraft'\r\n");
    else if (CMD_IS("convert"))
      send_to_char(ch, "You must place exact multiples of 10, of a material to being "
                       "the conversion process.\r\n");
    else
      send_to_char(ch, "Unrecognized crafting-station command!\r\n");
    return 1;
  }
    
  if (CMD_IS("resize"))
    return resize(argument, station, ch);
  else if (CMD_IS("restring"))
    return restring(argument, station, ch);
  else if (CMD_IS("augment"))
    return augment(station, ch);
  else if (CMD_IS("convert"))
    return convert(station, ch);
  else if (CMD_IS("supplyorder"))
    return supplyorder(station, ch);
  else if (CMD_IS("create"))  
    return create(argument, station, ch, 1);
  else if (CMD_IS("checkcraft"))
    return create(NULL, station, ch, 2);
  else {
    send_to_char(ch, "Invalid command.\r\n");
    return 0;
  }
  return 0;
}


SPECIAL(crafting_quest) {

  if (!CMD_IS("supplyorder")) {
    return 0;
  }

  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  two_arguments(argument, arg, arg2);

  if (!*arg)
    cquest_report(ch);
  else if (!strcmp(arg, "new")) {

    if (GET_AUTOCQUEST_VNUM(ch) && GET_AUTOCQUEST_MAKENUM(ch) <= 0) {
      send_to_char(ch, "You can't take a new supply order until you've handed in "
                       "the one you've completed.\r\n");
      return 1;
    }
    
    char desc[MAX_INPUT_LENGTH];
    int roll = 0;

    /* initialize values */
    reset_acraft(ch);
    GET_AUTOCQUEST_VNUM(ch) = AUTOCQUEST_VNUM;

    switch (dice(1,5)) {
      case 1:
        sprintf(desc, "a shield"); 
        GET_AUTOCQUEST_MATERIAL(ch) = MATERIAL_STEEL;
        break;
      case 2:
        sprintf(desc, "a sword"); 
        GET_AUTOCQUEST_MATERIAL(ch) = MATERIAL_STEEL;
        break;
      case 3:
        if ((roll = dice(1, 7)) == 1) {
          sprintf(desc, "a necklace"); 
          GET_AUTOCQUEST_MATERIAL(ch) = MATERIAL_COPPER;
        } else if (roll == 2) {
          sprintf(desc, "a bracer"); 
          GET_AUTOCQUEST_MATERIAL(ch) = MATERIAL_STEEL;
        } else if (roll == 3) {
          sprintf(desc, "a stim injection");  
          GET_AUTOCQUEST_MATERIAL(ch) = MATERIAL_STEEL;
        } else if (roll == 4) {
          sprintf(desc, "a cape");  
          GET_AUTOCQUEST_MATERIAL(ch) = MATERIAL_HEMP;
        } else if (roll == 5) {
          sprintf(desc, "a belt");  
          GET_AUTOCQUEST_MATERIAL(ch) = MATERIAL_LEATHER;
        } else if (roll == 6) {
          sprintf(desc, "a pair of gloves");  
          GET_AUTOCQUEST_MATERIAL(ch) = MATERIAL_HEMP;
        } else {
          sprintf(desc, "a pair of boots");  
          GET_AUTOCQUEST_MATERIAL(ch) = MATERIAL_LEATHER;
        }
        break;
      case 4:
        if ((roll = dice(1, 2)) == 1) {
          sprintf(desc, "a hover droid"); 
          GET_AUTOCQUEST_MATERIAL(ch) = MATERIAL_STEEL;
        } else {
          sprintf(desc, "an implant");  
          GET_AUTOCQUEST_MATERIAL(ch) = MATERIAL_STEEL;
        }
        break;
      default:
          sprintf(desc, "some war supplies"); 
          GET_AUTOCQUEST_MATERIAL(ch) = MATERIAL_STEEL;
        break;
    }

    GET_AUTOCQUEST_DESC(ch) = strdup(desc);
    GET_AUTOCQUEST_MAKENUM(ch) = AUTOCQUEST_MAKENUM;
    GET_AUTOCQUEST_QP(ch) = 1;
    GET_AUTOCQUEST_EXP(ch) = GET_LEVEL(ch) * GET_LEVEL(ch);
    GET_AUTOCQUEST_GOLD(ch) = GET_LEVEL(ch) * 100;

    send_to_char(ch, "You have been commissioned for a supply order to make %s.  "
                     "We expect you to make %d before you can collect your reward."
                     "  Good luck.\r\nOnce completed you will receive the "
                     "following:\r\n"
                     "You will receive %d reputation points.\r\n"
                     "%d credits will be deposited into your bank account.\r\n"
                     "You will receive %d artisan experience points.\r\n", 
                 desc, GET_AUTOCQUEST_MAKENUM(ch), GET_AUTOCQUEST_QP(ch),
                 GET_AUTOCQUEST_GOLD(ch), GET_AUTOCQUEST_EXP(ch));

  } else if (!strcmp(arg, "complete")) {

    if (GET_AUTOCQUEST_VNUM(ch) && GET_AUTOCQUEST_MAKENUM(ch) <= 0) {
      send_to_char(ch, "You have completed your supply order contract for %s.\r\n"
                       "You receive %d reputation points.\r\n"
                       "%d credits have been deposited into your bank account.\r\n"
                       "You receive %d experience points.\r\n", 
                       GET_AUTOCQUEST_DESC(ch), GET_AUTOCQUEST_QP(ch), 
                       GET_AUTOCQUEST_GOLD(ch), GET_AUTOCQUEST_EXP(ch));
      GET_QUESTPOINTS(ch) += GET_AUTOCQUEST_QP(ch);
      GET_GOLD(ch) += GET_AUTOCQUEST_GOLD(ch);

      reset_acraft(ch);

    } else
      cquest_report(ch);
      
  } else if (!strcmp(arg, "quit")) {

    send_to_char(ch, "You abandon your supply order to make %d %s.\r\n", 
                 GET_AUTOCQUEST_MAKENUM(ch), GET_AUTOCQUEST_DESC(ch));

    reset_acraft(ch);

  } else
    cquest_report(ch);

  return 1;
}


/*
void crafting_update(void)
{
  struct char_data *ch, *next_char;
  char buf[100];
  char buf2[100];
  int exp = 0, bonus = 0;
  int skill = -1;
  int buff;
  int roll = 0;
  struct obj_data *obj2 = NULL;

  // cycle through all the players
  for (ch = character_list; ch; ch = next_char) {
    next_char = ch->next;

    if (IS_NPC(ch) || !ch->desc)
      continue;

    if (GET_CRAFTING_OBJ(ch) == NULL || // something is off, so ensure reset
        GET_CRAFTING_TYPE(ch) == -1)
      continue;
    
    if (GET_CRAFTING_TICKS(ch) == 0) {  // the item is done

      switch(GET_CRAFTING_TYPE(ch)) {
        case SCMD_RESIZE:
          sprintf(buf, "You resize $p.  Success!!!");
          act(buf, false, ch, GET_CRAFTING_OBJ(ch), 0, TO_CHAR);
          sprintf(buf, "$n resizes $p.");
          act(buf, false, ch, GET_CRAFTING_OBJ(ch), 0, TO_ROOM);
          break;
        case SCMD_DIVIDE:
          sprintf(buf, "You create $p (x%d).  Success!!!",
                  GET_CRAFTING_REPEAT(ch));
          act(buf, false, ch, GET_CRAFTING_OBJ(ch), 0, TO_CHAR);
          sprintf(buf, "$n creates $p (x%d).", GET_CRAFTING_REPEAT(ch));
          act(buf, false, ch, GET_CRAFTING_OBJ(ch), 0, TO_ROOM);
          int i = 0;
          for (i = 1; i < GET_CRAFTING_REPEAT(ch); i++) {
            obj2 = read_object(GET_OBJ_VNUM(GET_CRAFTING_OBJ(ch)), VIRTUAL);
            obj_to_char(obj2, ch);
          }
          break;
        case SCMD_MINE:
          skill = SKILL_MINING;
          sprintf(buf, "You mine $p.  Success!!!");
          act(buf, false, ch, GET_CRAFTING_OBJ(ch), 0, TO_CHAR);
          sprintf(buf, "$n mines $p.");
          act(buf, false, ch, GET_CRAFTING_OBJ(ch), 0, TO_ROOM);
          break;
        case SCMD_HUNT:
          skill = SKILL_FORESTING;
          sprintf(buf, "You find $p from your hunting.  Success!!!");
          act(buf, false, ch, GET_CRAFTING_OBJ(ch), 0, TO_CHAR);
          sprintf(buf, "$n finds $p from $s hunting.");
          act(buf, false, ch, GET_CRAFTING_OBJ(ch), 0, TO_ROOM);
          break;
        case SCMD_FOREST:
          skill = SKILL_FORESTING;
          sprintf(buf, "You forest $p.  Success!!!");
          act(buf, false, ch, GET_CRAFTING_OBJ(ch), 0, TO_CHAR);
          sprintf(buf, "$n forests $p.");
          act(buf, false, ch, GET_CRAFTING_OBJ(ch), 0, TO_ROOM);
          break;
        case SCMD_FARM:
          skill = SKILL_FARMING;
          sprintf(buf, "You farm $p.  Success!!!");
          act(buf, false, ch, GET_CRAFTING_OBJ(ch), 0, TO_CHAR);
          sprintf(buf, "$n farms $p.");
          act(buf, false, ch, GET_CRAFTING_OBJ(ch), 0, TO_ROOM);
          break;
        case SCMD_DISENCHANT:
          // disenchant here
          break;
        case SCMD_SYNTHESIZE:
          // synthesizing here
          break;
        default:
          // crafting
          if (GET_OBJ_VNUM(GET_CRAFTING_OBJ(ch)) != GET_AUTOCQUEST_VNUM(ch)) {
            if (ch->craft_times > 0) {
              sprintf(buf2, " (x%d)", ch->craft_times + 1);
              for (i = 0; i < MAX(0, ch->craft_times); i++) {
                obj2 = read_object(ch->craft_vnum, VIRTUAL);
                obj_to_char(obj2, ch);
              }
              ch->craft_vnum = 0;
              ch->craft_times = 0;              
            } else
              sprintf(buf2, "@n");
            sprintf(buf, "You create $p%s.  Success!!!", buf2);
            act(buf, false, ch, GET_CRAFTING_OBJ(ch), 0, TO_CHAR);
            sprintf(buf, "$n creates $p%s.", buf2);
            act(buf, false, ch, GET_CRAFTING_OBJ(ch), 0, TO_ROOM);
            if (GET_GOLD(ch) < (GET_OBJ_COST(GET_CRAFTING_OBJ(ch)) / 4)) {
              GET_BANK_GOLD(ch) -= GET_OBJ_COST(GET_CRAFTING_OBJ(ch)) / 4;
            } else {
              GET_GOLD(ch) -= GET_OBJ_COST(GET_CRAFTING_OBJ(ch)) / 4;
            }

            // artisan/xp gain
            exp = 30 + (GET_OBJ_LEVEL(GET_CRAFTING_OBJ(ch)) * MAX(1,
                    ch->player_specials->crafting_exp_mult)) *
                    (HAS_FEAT(ch, FEAT_ELVEN_CRAFTING) ? 2 : 1) * 2 *
                    (GET_OBJ_MATERIAL(GET_CRAFTING_OBJ(ch)) == MATERIAL_MITHRIL ?
                    2 : 1);
            buff = 0;  // init buff for each player
            if (affected_by_spell(ch, SPELL_LEARNING))
              buff = 10;
            if (affected_by_spell(ch, SPELL_IMPROVED_LEARNING))
              buff = 20;
            if (affected_by_spell(ch, SPELL_GREATER_LEARNING))
              buff = 33;
            if (affected_by_spell(ch, SPELL_EPIC_LEARNING))
              buff = 50;
            bonus = exp * (GET_RP_ART_EXP_BONUS(ch) + (buff * 100));
            bonus /= 10000;
            //gain_exp(ch, MAX(1, level_exp( GET_CLASS_LEVEL(ch), GET_REAL_RACE(ch)) / (500 + (GET_LEVEL(ch) * 20))));
            gain_artisan_exp(ch, exp);
          } else if (GET_OBJ_VNUM(GET_CRAFTING_OBJ(ch)) == 
                     GET_AUTOCQUEST_VNUM(ch)){  //auto quest crafting
            GET_AUTOCQUEST_MAKENUM(ch)--;
            if (GET_AUTOCQUEST_MAKENUM(ch) == 0) {
              send_to_char(ch, "You have completed your supply order! Go turn"
                               " it in for more exp, quest points and "
                               "gold!.\r\n");
            } else {
              send_to_char(ch, "You have completed another item in your supply "
                               "order and have %d more to make.\r\n",
                           GET_AUTOCQUEST_MAKENUM(ch));
            }
            exp = 30 + (GET_OBJ_LEVEL(GET_CRAFTING_OBJ(ch)) * MAX(1, 
                  ch->player_specials->crafting_exp_mult)) *
                  (HAS_FEAT(ch, FEAT_ELVEN_CRAFTING) ? 2 : 1) * 
                  (GET_OBJ_MATERIAL(GET_CRAFTING_OBJ(ch)) == MATERIAL_MITHRIL
                  ? 2 : 1);
            bonus = exp * (GET_RP_ART_EXP_BONUS(ch) + (buff * 100));
            bonus /= 10000;
            gain_artisan_exp(ch, exp);
          }
          break;
      }

      // give crafted object to char
      if (GET_OBJ_VNUM(GET_CRAFTING_OBJ(ch)) != GET_AUTOCQUEST_VNUM(ch))
        obj_to_char(GET_CRAFTING_OBJ(ch), ch);

      if (skill != -1) { // artisan gain, bonus goodies
        exp = 30 + (get_skill_value(ch, skill) * MAX(1, 
              ch->player_specials->crafting_exp_mult)) *
              (HAS_FEAT(ch, FEAT_ELVEN_CRAFTING) ? 2 : 1) * 10 *
              (GET_OBJ_MATERIAL(GET_CRAFTING_OBJ(ch)) == 
              MATERIAL_MITHRIL ? 2 : 1);
        gain_artisan_exp(ch, exp);

        roll = dice(1, 100);
        if (roll <= 3) {
          if (dice(1, 20) <= 15)
            get_random_essence(ch, GET_CLASS_RANKS(ch, CLASS_ARTISAN));
          else
            get_random_crystal(ch, GET_CLASS_RANKS(ch, CLASS_ARTISAN));
        }
      }
      reset_craft(ch);
    } else {
    // the crafting tick is still going!
      if (GET_ADMLEVEL(ch) > 0)
        GET_CRAFTING_TICKS(ch) = 1;
      switch (GET_CRAFTING_TYPE(ch)) {
        case SCMD_DISENCHANT:
          break;
        case SCMD_SYNTHESIZE:
          break;
        case SCMD_DIVIDE:
          break;
        case SCMD_RESIZE:
          break;
        case SCMD_MINE:
        case SCMD_FARM:
        case SCMD_FOREST:
        case SCMD_HUNT:
        default:
          skill = 1;
          break;
      }
      if (!PRF_FLAGGED(ch, PRF_CRAFTING_BRIEF)) {
        if (GET_OBJ_VNUM(GET_CRAFTING_OBJ(ch)) != GET_AUTOCQUEST_VNUM(ch))
          send_to_char(ch, "You continue to %s %s.\r\n",
                       AssemblyTypes[GET_CRAFTING_TYPE(ch)],
                       GET_CRAFTING_OBJ(ch)->short_description);
        else
          send_to_char(ch, "You continue your supply order.\r\n");
      }
      if (skill == 1) {
        exp = 30 + (GET_OBJ_LEVEL(GET_CRAFTING_OBJ(ch)) * MAX(1, 
              ch->player_specials->crafting_exp_mult)) *
              (HAS_FEAT(ch, FEAT_ELVEN_CRAFTING) ? 2 : 1) *
              (GET_OBJ_MATERIAL(GET_CRAFTING_OBJ(ch)) == MATERIAL_MITHRIL ? 2 
              : 1);
        bonus = exp * (GET_RP_ART_EXP_BONUS(ch) + (buff * 100));
        bonus /= 10000;
        gain_artisan_exp(ch, exp);
      }
      send_to_char(ch, "You have approximately %d minutes and %d seconds "
                       "left to go.\r\n", GET_CRAFTING_TICKS(ch) / 6,
                       (GET_CRAFTING_TICKS(ch) % 6) * 10);
      GET_CRAFTING_TICKS(ch) -= 1;
    }
  }
}
*/



#undef PATTERN_UPPER
#undef PATTERN_LOWER
#undef MIN_MATS
#undef MIN_ELF_MATS
#undef WEIGHT_FACTOR
#undef CRYSTAL_CAP
#undef BONUS_FACTOR
#undef MAX_CRAFT_CRIT
#undef SUPPLYORDER_MATS
#undef AUTOCQUEST_VNUM
#undef AUTOCQUEST_MAKENUM
