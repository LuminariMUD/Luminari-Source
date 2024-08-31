/**************************************************************************
 *  File: genobj.c                                     Part of LuminariMUD *
 *  Usage: Generic OLC Library - Objects.                                  *
 *                                                                         *
 *  Copyright 1996 by Harvey Gilpin, 1997-2001 by George Greer.            *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "shop.h"
#include "constants.h"
#include "genolc.h"
#include "genobj.h"
#include "genzon.h"
#include "dg_olc.h"
#include "handler.h"
#include "interpreter.h"
#include "boards.h" /* for board_info */
#include "craft.h"
#include "spells.h"

/* local functions */
static int update_all_objects(struct obj_data *obj);
static void copy_object_strings(struct obj_data *to, struct obj_data *from);

obj_rnum add_object(struct obj_data *newobj, obj_vnum ovnum)
{
  int found = NOTHING;
  zone_rnum rznum = real_zone_by_thing(ovnum);

  /* Write object to internal tables. */
  if ((newobj->item_number = real_object(ovnum)) != NOTHING)
  {
    copy_object(&obj_proto[newobj->item_number], newobj);
    update_all_objects(&obj_proto[newobj->item_number]);
    add_to_save_list(zone_table[rznum].number, SL_OBJ);
    return newobj->item_number;
  }

  found = insert_object(newobj, ovnum);
  adjust_objects(found);
  add_to_save_list(zone_table[rznum].number, SL_OBJ);
  return found;
}

/* Fix all existing objects to have these values. We need to run through each
 * and every object currently in the game to see which ones are pointing to
 * this prototype. If object is pointing to this prototype, then we need to
 * replace it with the new one. */
static int update_all_objects(struct obj_data *refobj)
{
  struct obj_data *obj, swap;
  int count = 0;

  for (obj = object_list; obj; obj = obj->next)
  {
    if (obj->item_number != refobj->item_number)
      continue;

    count++;

    /* Update the existing object but save a copy for private information. */
    swap = *obj;
    *obj = *refobj;

    /* Copy game-time dependent variables over. */
    GET_ID(obj) = swap.id;
    IN_ROOM(obj) = swap.in_room;
    obj->carried_by = swap.carried_by;
    obj->worn_by = swap.worn_by;
    obj->worn_on = swap.worn_on;
    obj->in_obj = swap.in_obj;
    obj->contains = swap.contains;
    obj->next_content = swap.next_content;
    obj->next = swap.next;
    obj->sitting_here = swap.sitting_here;
  }

  return count;
}

/* Adjust the internal values of other objects as if something was inserted at
 * the given array index. Might also be useful to make 'holes' in the array
 * for some reason. */
obj_rnum adjust_objects(obj_rnum refpt)
{
  int shop, i, zone, cmd_no;
  struct obj_data *obj;

#if CIRCLE_UNSIGNED_INDEX
  if (refpt == NOTHING || refpt > top_of_objt)
#else
  if (refpt < 0 || refpt > top_of_objt)
#endif
    return NOTHING;

  /* Renumber live objects. */
  for (obj = object_list; obj; obj = obj->next)
    GET_OBJ_RNUM(obj) += (GET_OBJ_RNUM(obj) != NOTHING && GET_OBJ_RNUM(obj) >= refpt);

  /* Renumber zone table. */
  for (zone = 0; zone <= top_of_zone_table; zone++)
  {
    for (cmd_no = 0; ZCMD(zone, cmd_no).command != 'S'; cmd_no++)
    {
      switch (ZCMD(zone, cmd_no).command)
      {
      case 'P':
        ZCMD(zone, cmd_no).arg3 += (ZCMD(zone, cmd_no).arg3 >= refpt);
        /* No break here - drop into next case. */
      case 'O':
      case 'G':
      case 'E':
        ZCMD(zone, cmd_no).arg1 += (ZCMD(zone, cmd_no).arg1 >= refpt);
        break;
      case 'R':
        ZCMD(zone, cmd_no).arg2 += (ZCMD(zone, cmd_no).arg2 >= refpt);
        break;
      }
    }
  }

  /* Renumber notice boards. */
  for (i = 0; i < NUM_OF_BOARDS; i++)
    BOARD_RNUM(i) += (BOARD_RNUM(i) >= refpt);

  /* Renumber shop produce. */
  for (shop = 0; shop <= top_shop; shop++)
    for (i = 0; SHOP_PRODUCT(shop, i) != NOTHING; i++)
      SHOP_PRODUCT(shop, i) += (SHOP_PRODUCT(shop, i) >= refpt);

  return refpt;
}

/* Function handle the insertion of an object within the prototype framework.
 * Note that this does not adjust internal values of other objects, use
 * add_object() for that. */
obj_rnum insert_object(struct obj_data *obj, obj_vnum ovnum)
{
  obj_rnum i;

  top_of_objt++;
  RECREATE(obj_index, struct index_data, top_of_objt + 1);
  RECREATE(obj_proto, struct obj_data, top_of_objt + 1);

  /* Start counting through both tables. */
  for (i = top_of_objt; i > 0; i--)
  {
    /* Check if current virtual is bigger than our virtual number. */
    if (ovnum > obj_index[i - 1].vnum)
      return index_object(obj, ovnum, i);

    /* Copy over the object that should be here. */
    obj_index[i] = obj_index[i - 1];
    obj_proto[i] = obj_proto[i - 1];
    obj_proto[i].item_number = i;
  }

  /* Not found, place at 0. */
  return index_object(obj, ovnum, 0);
}

obj_rnum index_object(struct obj_data *obj, obj_vnum ovnum, obj_rnum ornum)
{
#if CIRCLE_UNSIGNED_INDEX
  if (obj == NULL || ornum == NOTHING || ornum > top_of_objt)
#else
  if (obj == NULL || ovnum < 0 || ornum < 0 || ornum > top_of_objt)
#endif
    return NOWHERE;

  obj->item_number = ornum;
  obj_index[ornum].vnum = ovnum;
  obj_index[ornum].number = 0;
  obj_index[ornum].func = NULL;

  copy_object_preserve(&obj_proto[ornum], obj);
  obj_proto[ornum].in_room = NOWHERE;

  return ornum;
}

int save_objects(zone_rnum zone_num)
{
  char filename[128], buf[MAX_STRING_LENGTH] = {'\0'}, buf2[MAX_STRING_LENGTH] = {'\0'};
  char ebuf1[MAX_STRING_LENGTH] = {'\0'}, ebuf2[MAX_STRING_LENGTH] = {'\0'},
       ebuf3[MAX_STRING_LENGTH] = {'\0'}, ebuf4[MAX_STRING_LENGTH] = {'\0'};
  char wbuf1[MAX_STRING_LENGTH] = {'\0'}, wbuf2[MAX_STRING_LENGTH] = {'\0'},
       wbuf3[MAX_STRING_LENGTH] = {'\0'}, wbuf4[MAX_STRING_LENGTH] = {'\0'};
  char pbuf1[MAX_STRING_LENGTH] = {'\0'}, pbuf2[MAX_STRING_LENGTH] = {'\0'},
       pbuf3[MAX_STRING_LENGTH] = {'\0'}, pbuf4[MAX_STRING_LENGTH] = {'\0'};
  int counter, counter2, realcounter;
  FILE *fp;
  struct obj_data *obj;
  struct extra_descr_data *ex_desc;
  struct obj_special_ability *specab;

#if CIRCLE_UNSIGNED_INDEX
  if (zone_num == NOWHERE || zone_num > top_of_zone_table)
  {
#else
  if (zone_num < 0 || zone_num > top_of_zone_table)
  {
#endif
    log("SYSERR: GenOLC: save_objects: Invalid real zone number %d. (0-%d)",
        zone_num, top_of_zone_table);
    return FALSE;
  }

  snprintf(filename, sizeof(filename), "%s/%d.new", OBJ_PREFIX,
           zone_table[zone_num].number);
  if (!(fp = fopen(filename, "w+")))
  {
    mudlog(BRF, LVL_IMMORT, TRUE, "SYSERR: OLC: Cannot open objects file %s!",
           filename);
    return FALSE;
  }
  /* Start running through all objects in this zone. */
  for (counter = genolc_zone_bottom(zone_num);
       counter <= zone_table[zone_num].top; counter++)
  {
    if ((realcounter = real_object(counter)) != NOTHING)
    {
      if ((obj = &obj_proto[realcounter])->action_description)
      {
        strncpy(buf, obj->action_description, sizeof(buf) - 1);
        strip_cr(buf);
      }
      else
        *buf = '\0';

      snprintf(buf2, sizeof(buf2),
               "#%d\n"
               "%s~\n"
               "%s~\n"
               "%s~\n"
               "%s~\n",

               GET_OBJ_VNUM(obj),
               (obj->name && *obj->name) ? obj->name : "undefined",
               (obj->short_description && *obj->short_description) ? obj->short_description : "undefined",
               (obj->description && *obj->description) ? obj->description : "undefined",
               buf);

      fprintf(fp, convert_from_tabs(buf2), 0);

      sprintascii(ebuf1, GET_OBJ_EXTRA(obj)[0]);
      sprintascii(ebuf2, GET_OBJ_EXTRA(obj)[1]);
      sprintascii(ebuf3, GET_OBJ_EXTRA(obj)[2]);
      sprintascii(ebuf4, GET_OBJ_EXTRA(obj)[3]);
      sprintascii(wbuf1, GET_OBJ_WEAR(obj)[0]);
      sprintascii(wbuf2, GET_OBJ_WEAR(obj)[1]);
      sprintascii(wbuf3, GET_OBJ_WEAR(obj)[2]);
      sprintascii(wbuf4, GET_OBJ_WEAR(obj)[3]);
      sprintascii(pbuf1, GET_OBJ_PERM(obj)[0]);
      sprintascii(pbuf2, GET_OBJ_PERM(obj)[1]);
      sprintascii(pbuf3, GET_OBJ_PERM(obj)[2]);
      sprintascii(pbuf4, GET_OBJ_PERM(obj)[3]);

      fprintf(fp, "%d %s %s %s %s %s %s %s %s %s %s %s %s\n"
                  "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n"
                  "%d %d %d %d %d\n",

              GET_OBJ_TYPE(obj),
              ebuf1, ebuf2, ebuf3, ebuf4,
              wbuf1, wbuf2, wbuf3, wbuf4,
              pbuf1, pbuf2, pbuf3, pbuf4,
              GET_OBJ_VAL(obj, 0), GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2), GET_OBJ_VAL(obj, 3),
              GET_OBJ_VAL(obj, 4), GET_OBJ_VAL(obj, 5), GET_OBJ_VAL(obj, 6), GET_OBJ_VAL(obj, 7),
              GET_OBJ_VAL(obj, 8), GET_OBJ_VAL(obj, 9), GET_OBJ_VAL(obj, 10), GET_OBJ_VAL(obj, 11),
              GET_OBJ_VAL(obj, 12), GET_OBJ_VAL(obj, 13), GET_OBJ_VAL(obj, 14), GET_OBJ_VAL(obj, 15),
              GET_OBJ_WEIGHT(obj), GET_OBJ_COST(obj),
              GET_OBJ_RENT(obj), GET_OBJ_LEVEL(obj), GET_OBJ_TIMER(obj));

      /* A:  Do we have affects? */
      for (counter2 = 0; counter2 < MAX_OBJ_AFFECT; counter2++)
        if (obj->affected[counter2].modifier)
          fprintf(fp,
                  "A\n"
                  "%d %d %d %d\n",
                  obj->affected[counter2].location,
                  obj->affected[counter2].modifier,
                  obj->affected[counter2].bonus_type,
                  obj->affected[counter2].specific);

      /* B:  Do we have spells? */
      if (obj->sbinfo)
      { /*. Yep, save them too . */
        for (counter2 = 0; counter2 < SPELLBOOK_SIZE; counter2++)
        {
          if (obj->sbinfo[counter2].spellname == 0)
          {
            break;
          }
          fprintf(fp, "B\n"
                      "%d %d\n",
                  obj->sbinfo[counter2].spellname,
                  obj->sbinfo[counter2].pages);
          continue;
        }
      }
      /* C:  Do we have Special Abilities? */
      if (obj->special_abilities)
      { /* Yes, save them too. */
        for (specab = obj->special_abilities; specab != NULL; specab = specab->next)
        {
          fprintf(fp, "C\n"
                      "%d %d %d %d %d %d %d %s\n",
                  specab->ability, specab->level, specab->activation_method,
                  specab->value[0], specab->value[1], specab->value[2], specab->value[3],
                  (specab->command_word && *specab->command_word) ? specab->command_word : "");
        }
      }
      /* E:  Do we have extra descriptions? */
      if (obj->ex_description)
      { /* Yes, save them too. */
        for (ex_desc = obj->ex_description; ex_desc; ex_desc = ex_desc->next)
        {
          /* Sanity check to prevent nasty protection faults. */
          if (!ex_desc->keyword || !ex_desc->description || !*ex_desc->keyword || !*ex_desc->description)
          {
            mudlog(BRF, LVL_IMMORT, TRUE, "SYSERR: OLC: oedit_save_to_disk: "
                                          "Corrupt ex_desc!");
            continue;
          }
          strncpy(buf, ex_desc->description, sizeof(buf) - 1);
          strip_cr(buf);
          fprintf(fp, "E\n"
                      "%s~\n"
                      "%s~\n",
                  ex_desc->keyword, buf);
        }
      }

      // G:  object proficiency
      fprintf(fp, "G\n"
                  "%d\n",
              GET_OBJ_PROF(obj));

      // H:  object material
      fprintf(fp, "H\n"
                  "%d\n",
              GET_OBJ_MATERIAL(obj));

      // I:  object size
      fprintf(fp, "I\n"
                  "%d\n",
              GET_OBJ_SIZE(obj));

      // J:  vnum of mob that can receive item
      fprintf(fp, "J\n"
                  "%d\n",
              obj->mob_recepient);

      // k: spells that can be activated
      if (obj->activate_spell[ACT_SPELL_LEVEL] > 0 && obj->activate_spell[ACT_SPELL_SPELLNUM] > 0)
      {
        fprintf(fp, "K\n"
                  "%d %d %d %d %d\n",
              obj->activate_spell[ACT_SPELL_LEVEL], obj->activate_spell[ACT_SPELL_SPELLNUM], 
              obj->activate_spell[ACT_SPELL_CURRENT_USES], obj->activate_spell[ACT_SPELL_MAX_USES], 
              obj->activate_spell[ACT_SPELL_COOLDOWN]);
      }

      // S:  weapon spells
      for (counter2 = 0; counter2 < MAX_WEAPON_SPELLS; counter2++)
        if (obj->wpn_spells[counter2].spellnum)
          fprintf(fp, "S\n"
                      "%d %d %d %d\n",
                  obj->wpn_spells[counter2].spellnum,
                  obj->wpn_spells[counter2].level,
                  obj->wpn_spells[counter2].percent,
                  obj->wpn_spells[counter2].inCombat);

      /* T:  Do we have script(s) attached? */
      script_save_to_disk(fp, obj, OBJ_TRIGGER);
    }
  }

  /* Write the final line, close the file. */
  fprintf(fp, "$~\n");
  fclose(fp);
  snprintf(buf, sizeof(buf), "%s/%d.obj", OBJ_PREFIX,
           zone_table[zone_num].number);
  remove(buf);
  rename(filename, buf);

  if (in_save_list(zone_table[zone_num].number, SL_OBJ))
    remove_from_save_list(zone_table[zone_num].number, SL_OBJ);
  return TRUE;
}

/* Free all, unconditionally. */
void free_object_strings(struct obj_data *obj)
{
  if (obj->name)
    free(obj->name);
  if (obj->description)
    free(obj->description);
  if (obj->short_description)
    free(obj->short_description);
  if (obj->action_description)
    free(obj->action_description);
  if (obj->ex_description)
    free_ex_descriptions(obj->ex_description);
}

/* For object instances that are not the prototype. */
void free_object_strings_proto(struct obj_data *obj)
{
  int robj_num = GET_OBJ_RNUM(obj);

  if (obj->name && obj->name != obj_proto[robj_num].name)
    free(obj->name);
  if (obj->description && obj->description != obj_proto[robj_num].description)
    free(obj->description);
  if (obj->short_description && obj->short_description !=
                                    obj_proto[robj_num].short_description)
    free(obj->short_description);
  if (obj->action_description && obj->action_description !=
                                     obj_proto[robj_num].action_description)
    free(obj->action_description);
  if (obj->ex_description)
  {
    struct extra_descr_data *thised, *plist, *next_one; /* O(horrible) */
    int ok_key, ok_desc, ok_item;
    for (thised = obj->ex_description; thised; thised = next_one)
    {
      next_one = thised->next;
      for (ok_item = ok_key = ok_desc = 1,
          plist = obj_proto[robj_num].ex_description;
           plist; plist = plist->next)
      {
        if (plist->keyword == thised->keyword)
          ok_key = 0;
        if (plist->description == thised->description)
          ok_desc = 0;
        if (plist == thised)
          ok_item = 0;
      }
      if (thised->keyword && ok_key)
        free(thised->keyword);
      if (thised->description && ok_desc)
        free(thised->description);
      if (ok_item)
        free(thised);
    }
  }
}

static void copy_object_strings(struct obj_data *to, struct obj_data *from)
{
  to->name = from->name ? strdup(from->name) : NULL;
  to->description = from->description ? strdup(from->description) : NULL;
  to->short_description = from->short_description ? strdup(from->short_description) : NULL;
  to->action_description = from->action_description ? strdup(from->action_description) : NULL;

  if (from->ex_description)
    copy_ex_descriptions(&to->ex_description, from->ex_description);
  else
    to->ex_description = NULL;
}

int copy_object(struct obj_data *to, struct obj_data *from)
{
  free_object_strings(to);
  return copy_object_main(to, from, TRUE);
}

int copy_object_preserve(struct obj_data *to, struct obj_data *from)
{
  return copy_object_main(to, from, FALSE);
}

int copy_object_main(struct obj_data *to,
                     struct obj_data *from, int free_object)
{
  *to = *from;
  copy_object_strings(to, from);
  return TRUE;
}

int delete_object(obj_rnum rnum)
{
  obj_rnum i;
  zone_rnum zrnum;
  struct obj_data *obj, *tmp, *next_obj;
  int shop, j, zone, cmd_no;

  if (rnum == NOTHING || rnum > top_of_objt)
    return NOTHING;

  obj = &obj_proto[rnum];

  zrnum = real_zone_by_thing(GET_OBJ_VNUM(obj));

  /* This is something you might want to read about in the logs. */
  log("GenOLC: delete_object: Deleting object #%d (%s).", GET_OBJ_VNUM(obj),
      obj->short_description);

  for (tmp = object_list; tmp; tmp = next_obj)
  {
    next_obj = tmp->next;
    if (tmp->item_number != obj->item_number)
      continue;

    /* extract_obj() will just axe contents. */
    if (tmp->contains)
    {
      struct obj_data *this_content, *next_content;
      for (this_content = tmp->contains; this_content;
           this_content = next_content)
      {
        next_content = this_content->next_content;
        if (IN_ROOM(tmp))
        {
          /* Transfer stuff from object to room. */
          obj_from_obj(this_content);
          obj_to_room(this_content, IN_ROOM(tmp));
        }
        else if (tmp->worn_by || tmp->carried_by)
        {
          /* Transfer stuff from object to person inventory. */
          obj_from_char(this_content);
          obj_to_char(this_content, tmp->carried_by);
        }
        else if (tmp->in_obj)
        {
          /* Transfer stuff from object to containing object. */
          obj_from_obj(this_content);
          obj_to_obj(this_content, tmp->in_obj);
        }
      }
    }
    /* Remove from object_list, etc. - handles weightchanges, and similar. */
    extract_obj(tmp);
  }

  /* Make sure all are removed. */
  assert(obj_index[rnum].number == 0);

  /* Adjust rnums of all other objects. */
  for (tmp = object_list; tmp; tmp = tmp->next)
  {
    GET_OBJ_RNUM(tmp) -= (GET_OBJ_RNUM(tmp) > rnum);
  }

  for (i = rnum; i < top_of_objt; i++)
  {
    obj_index[i] = obj_index[i + 1];
    obj_proto[i] = obj_proto[i + 1];
    obj_proto[i].item_number = i;
  }

  top_of_objt--;
  RECREATE(obj_index, struct index_data, top_of_objt + 1);
  RECREATE(obj_proto, struct obj_data, top_of_objt + 1);

  /* Renumber notice boards. */
  for (j = 0; j < NUM_OF_BOARDS; j++)
    BOARD_RNUM(j) -= (BOARD_RNUM(j) > rnum);

  /* Renumber shop produce. */
  for (shop = 0; shop <= top_shop; shop++)
    for (j = 0; SHOP_PRODUCT(shop, j) != NOTHING; j++)
      SHOP_PRODUCT(shop, j) -= (SHOP_PRODUCT(shop, j) > rnum);

  /* Renumber zone table. */
  for (zone = 0; zone <= top_of_zone_table; zone++)
  {
    for (cmd_no = 0; ZCMD(zone, cmd_no).command != 'S'; cmd_no++)
    {
      switch (ZCMD(zone, cmd_no).command)
      {
      case 'P':
        if (ZCMD(zone, cmd_no).arg3 == rnum)
        {
          delete_zone_command(&zone_table[zone], cmd_no);
        }
        else
          ZCMD(zone, cmd_no).arg3 -= (ZCMD(zone, cmd_no).arg3 > rnum);
        /* No break here - drop into next case. */
      case 'O':
      case 'G':
      case 'E':
        if (ZCMD(zone, cmd_no).arg1 == rnum)
        {
          delete_zone_command(&zone_table[zone], cmd_no);
        }
        else
          ZCMD(zone, cmd_no).arg1 -= (ZCMD(zone, cmd_no).arg1 > rnum);
        break;
      case 'R':
        if (ZCMD(zone, cmd_no).arg2 == rnum)
        {
          delete_zone_command(&zone_table[zone], cmd_no);
        }
        else
          ZCMD(zone, cmd_no).arg2 -= (ZCMD(zone, cmd_no).arg2 > rnum);
        break;
      }
    }
  }

  save_objects(zrnum);

  return rnum;
}

/* oset handling, this location should be temporary */
bool oset_alias(struct obj_data *obj, const char *argument)
{
  static int max_len = 64;
  int i = GET_OBJ_RNUM(obj);

  skip_spaces_c(&argument);

  if (strlen(argument) > max_len)
    return FALSE;

  if (i != NOWHERE && obj->name && obj->name != obj_proto[i].name)
    free(obj->name);

  obj->name = strdup(argument);

  return TRUE;
}

bool oset_apply(struct obj_data *obj, const char *argument)
{
  int i = 0, apply = -1, location = -1, mod = 0, empty = -1, value;
  char arg[MAX_INPUT_LENGTH] = {'\0'};

  argument = one_argument(argument, arg, sizeof(arg));

  skip_spaces_c(&argument);

  if ((value = atoi(argument)) == 0)
    return FALSE;

  while (*apply_types[i] != '\n')
  {
    if (is_abbrev(apply_types[i], arg))
    {
      apply = i;
      break;
    }
    i++;
  }

  if (apply == -1)
    return FALSE;

  for (i = 0; i < MAX_OBJ_AFFECT; i++)
  {
    if (obj->affected[i].location == apply)
    {
      location = i;
      mod = obj->affected[i].modifier;
      break;
    }
    else if (obj->affected[i].location == APPLY_NONE && empty == -1)
    {
      empty = i;
    }
  }

  /* No slot already using APPLY_XXX, so use an empty one... if available */
  if (location == -1)
    location = empty;

  /* There is no slot already using our APPLY_XXX type, and no empty slots either */
  if (location == -1)
    return FALSE;

  obj->affected[location].modifier = mod + value;
  obj->affected[location].bonus_type = BONUS_TYPE_UNDEFINED;

  /* Our modifier is set at 0, so lets just clear the apply location so that it may
   * be reused at a later point */
  if (obj->affected[location].modifier != 0)
    obj->affected[location].location = apply;
  else
    obj->affected[location].location = APPLY_NONE;

  return TRUE;
}

bool oset_short_description(struct obj_data *obj, const char *argument)
{
  static int max_len = 64;
  int i = GET_OBJ_RNUM(obj);

  skip_spaces_c(&argument);

  if (strlen(argument) > max_len)
    return FALSE;

  if (i != NOWHERE && obj->short_description && obj->short_description != obj_proto[i].short_description)
    free(obj->short_description);

  obj->short_description = strdup(argument);

  return TRUE;
}

bool oset_long_description(struct obj_data *obj, const char *argument)
{
  static int max_len = 128;
  int i = GET_OBJ_RNUM(obj);

  skip_spaces_c(&argument);

  if (strlen(argument) > max_len)
    return FALSE;

  if (i != NOWHERE && obj->description && obj->description != obj_proto[i].description)
    free(obj->description);

  obj->description = strdup(argument);

  return TRUE;
}
