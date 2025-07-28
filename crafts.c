/*/ \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \
\
/  Luminari Crafts System
/  Created By: Created by Vatiken, (Joseph Arnusch)
\              installed by Ornir
/  Header file: crafts.h
\  Created: June 21st, 2012
/
\ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ /*/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "act.h"
#include "handler.h"
#include "interpreter.h"
#include "screen.h"
#include "constants.h"
#include "oasis.h"
#include "genolc.h"
#include "spells.h"
#include "mud_event.h"
#include "crafts.h"
#include "item.h"

/* Statics */
static void craftedit_disp_menu(struct descriptor_data *d);
static void save_crafts_to_disk(void);
static void remove_components(struct char_data *ch, struct craft_data *craft, bool success);
int num_crafts = 0;

struct list_data *global_craft_list = NULL;

struct craft_data *create_craft(void)
{
  struct craft_data *new_craft;

  CREATE(new_craft, struct craft_data, 1);

  new_craft->craft_name = NULL;
  new_craft->craft_flags = 0;
  new_craft->craft_object_vnum = NOTHING;
  new_craft->craft_timer = 0;
  new_craft->craft_id = 0;
  new_craft->craft_skill = -1;
  new_craft->craft_skill_level = 0;

  new_craft->craft_msg_room = NULL;
  new_craft->craft_msg_self = NULL;

  new_craft->requirements = create_list();
  return (new_craft);
}

struct requirement_data *create_requirement(void)
{
  struct requirement_data *new_requirement;

  CREATE(new_requirement, struct requirement_data, 1);

  new_requirement->req_vnum = NOTHING;
  new_requirement->req_amount = 0;
  new_requirement->req_flags = 0;

  return (new_requirement);
}

void free_craft(struct craft_data *craft)
{
  struct requirement_data *r;
  struct iterator_data Iterator;

  if (craft->craft_name)
    free(craft->craft_name);

  if (craft->craft_msg_self)
    free(craft->craft_msg_self);

  if (craft->craft_msg_room)
    free(craft->craft_msg_room);

  if (craft->requirements->iSize)
  {
    struct requirement_data *next_r = NULL;
    
    /* Fix double-free: Get first item from list */
    r = (struct requirement_data *)merge_iterator(&Iterator, craft->requirements);
    
    while (r)
    {
      /* Get next item BEFORE removing current item from list */
      next_r = (struct requirement_data *)next_in_list(&Iterator);
      
      /* Now safe to remove and free current item */
      remove_from_list(r, craft->requirements);
      free(r);
      
      /* Move to next item */
      r = next_r;
    }
    remove_iterator(&Iterator);
    free_list(craft->requirements);
  }
  free(craft);
}

void load_crafts(void)
{
  FILE *fp;
  char *line;
  char tag[6];
  struct craft_data *craft = NULL;
  struct requirement_data *requirement;
  bool in_craft = FALSE;
  bool done = FALSE;

  if ((fp = fopen(CRAFT_FILE, "r")) == NULL)
  {
    log("No Craft file found!");
    return;
  }
  else
  {
    while ((line = fread_line(fp)) != NULL && line[0] != '\0' && !done)
    {
      if (!in_craft && !strcmp(line, "NEW"))
      {
        craft = create_craft();
        in_craft = TRUE;
      }
      else if (line[0] == '$')
      {
        done = TRUE;
        break;
      }
      else if (in_craft)
      {
        tag_argument(line, tag);
        switch (*tag)
        {
        case 'I':
          if (!strcmp(tag, "Id  "))
            craft->craft_id = atoi(line);
          break;
        case 'E':
          if (!strcmp(tag, "End "))
          {
            in_craft = FALSE;
            add_to_list(craft, global_craft_list);
          }
          break;
        case 'F':
          if (!strcmp(tag, "Flag"))
            craft->craft_flags = atoi(line);
          break;
        case 'M':
          if (!strcmp(tag, "Mroo"))
            craft->craft_msg_room = strdup(line);
          else if (!strcmp(tag, "Mslf"))
            craft->craft_msg_self = strdup(line);
          break;
        case 'N':
          if (!strcmp(tag, "Name"))
            craft->craft_name = strdup(line);
          break;
        case 'R':
          if (!strcmp(tag, "Req "))
          {
            requirement = create_requirement();
            if (sscanf(line, "%d %d %d\n", (int *)&requirement->req_vnum, &requirement->req_amount, &requirement->req_flags) != 3)
              log("SYSERR: Format error in Requirement");
            else
              add_to_list(requirement, craft->requirements);
          }
          break;
        case 'S':
          if (!strcmp(tag, "Skil"))
          {
            if (sscanf(line, "%d %d\n", &craft->craft_skill, &craft->craft_skill_level) != 2)
              log("SYSERR: Format error in Skill Level");
          }
          break;
        case 'T':
          if (!strcmp(tag, "Time"))
            craft->craft_timer = atoi(line);
          break;
        case 'V':
          if (!strcmp(tag, "Vnum"))
            craft->craft_object_vnum = atoi(line);
          break;
        default:
          log("SYSERR: Craft File: Unexpected '%s' in file.", tag);
          break;
        }
      }
      else
        log("SYSERR: Error in the crafts file.");
    }
  }
  fclose(fp);
}

/* write_crafts() */
static void save_crafts_to_disk(void)
{
  FILE *fp;
  struct craft_data *c;
  struct requirement_data *r;
  struct iterator_data Iterator;

  if ((fp = fopen(CRAFT_FILE, "w")) == NULL)
  {
    log("Cannot open craft file for writing!");
    return;
  }

  for (c = (struct craft_data *)merge_iterator(&Iterator, global_craft_list);
       c;
       c = next_in_list(&Iterator))
  {
    fprintf(fp, "NEW\n");
    fprintf(fp, "Name: %s\n", CRAFT_NAME(c));
    fprintf(fp, "Id  : %d\n", CRAFT_ID(c));
    fprintf(fp, "Flag: %d\n", CRAFT_FLAGS(c));
    fprintf(fp, "Vnum: %d\n", CRAFT_OBJVNUM(c));
    fprintf(fp, "Time: %d\n", CRAFT_TIMER(c));
    fprintf(fp, "Skil: %d %d\n", CRAFT_SKILL(c), CRAFT_SKILL_LEVEL(c));

    fprintf(fp, "Mslf: %s\n", CRAFT_MSG_SELF(c));
    fprintf(fp, "Mroo: %s\n", CRAFT_MSG_ROOM(c));

    while ((r = (struct requirement_data *)simple_list(c->requirements)) != NULL)
      fprintf(fp, "Req : %d %d %d\n",
              r->req_vnum, r->req_amount, r->req_flags);
    fprintf(fp, "End :\n");
  }

  remove_iterator(&Iterator);

  fprintf(fp, "$\n");
  fclose(fp);
}

/* Craft Handlers */
void sort_craft_list(void)
{
  struct list_data *sorted;
  struct craft_data *craft, *rem_craft;

  sorted = create_list();

  while (global_craft_list->iSize)
  {
    simple_list(NULL);
    rem_craft = NULL;
    while ((craft = (struct craft_data *)simple_list(global_craft_list)) != NULL)
    {
      if (rem_craft == NULL)
        rem_craft = craft;
      else if (CRAFT_ID(craft) <= CRAFT_ID(rem_craft))
        rem_craft = craft;
    }
    if (rem_craft != NULL)
    {
      add_to_list(rem_craft, sorted);
      remove_from_list(rem_craft, global_craft_list);
    }
    else
      break;
  }

  free_list(global_craft_list);
  global_craft_list = sorted;
}

struct craft_data *get_craft_from_arg(char *arg)
{
  struct iterator_data iterator;
  struct craft_data *craft = NULL;

  if (!global_craft_list->iSize)
    return NULL;

  for (craft = (struct craft_data *)merge_iterator(&iterator, global_craft_list);
       craft != NULL;
       craft = (struct craft_data *)next_in_list(&iterator))
  {
    if (is_abbrev(arg, CRAFT_NAME(craft)))
      break;
  }

  remove_iterator(&iterator);
  return (craft);
}

struct craft_data *get_craft_from_id(int id)
{
  struct iterator_data iterator;
  struct craft_data *craft = NULL;

  if (!global_craft_list->iSize)
    return NULL;

  for (craft = (struct craft_data *)merge_iterator(&iterator, global_craft_list);
       craft != NULL;
       craft = (struct craft_data *)next_in_list(&iterator))
  {
    if (CRAFT_ID(craft) == id)
      break;
  }

  remove_iterator(&iterator);
  return (craft);
}

struct requirement_data *find_requirement_in_craft(struct craft_data *craft, obj_vnum vnum)
{
  struct requirement_data *r = NULL;
  struct iterator_data Iterator;
  bool found = FALSE;

  if (craft->requirements->iSize == 0)
    return NULL;

  for (r = (struct requirement_data *)merge_iterator(&Iterator, craft->requirements);
       r;
       r = next_in_list(&Iterator))
    if (r->req_vnum == vnum)
    {
      found = TRUE;
      break;
    }

  remove_iterator(&Iterator);

  if (found)
    return r;

  return NULL;
}

struct obj_data *get_object_from_requirement(struct char_data *ch, struct requirement_data *req)
{
  struct obj_data *obj;

  if (IS_SET(req->req_flags, REQ_FLAG_NO_REMOVE))
    return (NULL);

  if (IS_SET(req->req_flags, REQ_FLAG_IN_ROOM))
  {
    for (obj = world[IN_ROOM(ch)].contents; obj; obj = obj->next_content)
    {
      if (GET_OBJ_VNUM(obj) == req->req_vnum)
        return (obj);
    }
  }
  else
  {
    for (obj = ch->carrying; obj; obj = obj->next_content)
    {
      if (GET_OBJ_VNUM(obj) == req->req_vnum)
        return (obj);
    }
  }

  return (NULL);
}

bool find_requirement(struct char_data *ch, struct requirement_data *req)
{
  bool in_room = IS_SET(req->req_flags, REQ_FLAG_IN_ROOM);
  obj_vnum vnum = req->req_vnum;
  int amount = req->req_amount;
  struct obj_data *obj;

  if (in_room)
  {
    for (obj = world[IN_ROOM(ch)].contents; obj; obj = obj->next_content)
    {
      if (GET_OBJ_VNUM(obj) == vnum)
        if (--amount <= 0)
          return (TRUE);
    }
  }
  else
  {
    for (obj = ch->carrying; obj; obj = obj->next_content)
    {
      if (GET_OBJ_VNUM(obj) == vnum)
        if (--amount <= 0)
          return (TRUE);
    }
  }

  return (FALSE);
}

int missing_craft_requirements(struct char_data *ch, struct craft_data *craft)
{
  int missing = 0;
  struct iterator_data iterator;
  struct requirement_data *requirement;
  obj_rnum rnum;

  if (!craft->requirements->iSize)
    return (-1);

  for (requirement = (struct requirement_data *)merge_iterator(&iterator, craft->requirements);
       requirement != NULL;
       requirement = (struct requirement_data *)next_in_list(&iterator))
  {
    if ((rnum = real_object(requirement->req_vnum)) == NOTHING)
      continue;
    if (find_requirement(ch, requirement) == FALSE)
      missing++;
  }

  remove_iterator(&iterator);
  return (missing);
}

static void remove_components(struct char_data *ch, struct craft_data *craft, bool success)
{
  struct iterator_data iterator;
  struct requirement_data *req;
  struct obj_data *obj;
  int count;

  for (req = (struct requirement_data *)merge_iterator(&iterator, craft->requirements);
       req;
       req = next_in_list(&iterator))
  {
    if (IS_SET(req->req_flags, REQ_FLAG_NO_REMOVE))
      continue;
    if (!success && IS_SET(req->req_flags, REQ_SAVE_ON_FAIL))
      continue;
    count = req->req_amount;
    while (count)
    {
      if ((obj = get_object_from_requirement(ch, req)) == NULL)
        break;
      if (obj->in_room != NOWHERE)
      {
        obj_from_room(obj);
        extract_obj(obj);
      }
      else if (obj->carried_by != NULL)
      {
        obj_from_char(obj);
        extract_obj(obj);
      }
      count--;
    }
  }

  remove_iterator(&iterator);
}

void list_all_crafts(struct char_data *ch)
{
  struct craft_data *craft;
  obj_vnum vnum;

  if (global_craft_list->iSize > 0)
  {
    send_to_char(ch, "\t1Crafts:\r\n"
                     "\t2ID  ) Name                       VNUM   Item Name\tn\r\n");

    while ((craft = (struct craft_data *)simple_list(global_craft_list)) != NULL)
    {
      send_to_char(ch, "\t2%-4d)\t3 %-22s -> \t1[\t2%-4d\t1] [\t2%s\t1]\tn\r\n", CRAFT_ID(craft), CRAFT_NAME(craft), CRAFT_OBJVNUM(craft),
                   (vnum = real_object(CRAFT_OBJVNUM(craft))) != NOWHERE ? obj_proto[vnum].short_description : "MISSING OBJECT");
    }
  }
  else
    send_to_char(ch, "There are no crafts available... use 'craftedit #' to create one.\r\n");
}

void list_available_crafts(struct char_data *ch)
{
  struct craft_data *craft;
  int missing, count = 0;

  send_to_char(ch, "Crafts:\r\n");

  if (global_craft_list->iSize > 0)
  {
    while ((craft = (struct craft_data *)simple_list(global_craft_list)) != NULL)
    {
      if (IS_SET(CRAFT_FLAGS(craft), CRAFT_RECIPE))
        continue;
      if (GET_SKILL(ch, CRAFT_SKILL(craft)) < CRAFT_SKILL_LEVEL(craft))
        continue;
      missing = missing_craft_requirements(ch, craft);
      send_to_char(ch, " %d) %s%s%s\r\n", ++count, missing ? CCRED(ch, C_NRM) : CCGRN(ch, C_NRM), CRAFT_NAME(craft), CCNRM(ch, C_NRM));
    }
  }

  if (!count)
    send_to_char(ch, "   You do not currently know of any crafts.\r\n");
  else
    send_to_char(ch, "\r\n%sAll Requirement Met    %sMissing Requirements%s\r\n", CCGRN(ch, C_NRM), CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
}

void show_craft(struct char_data *ch, struct craft_data *craft, int mode)
{
  struct requirement_data *req;
  obj_rnum rnum;

  if (craft == NULL)
    return;

  if (!IS_SET(CRAFT_FLAGS(craft), CRAFT_RECIPE))
  {
    if (mode == ITEM_STAT_MODE_G_LORE)
      send_to_group(NULL, GROUP(ch), "Craft: %s, (%d)\r\n", CRAFT_NAME(craft), CRAFT_ID(craft));
    else
      send_to_char(ch, "Craft: %s, (%d)\r\n", CRAFT_NAME(craft), CRAFT_ID(craft));

    rnum = real_object(CRAFT_OBJVNUM(craft));

    if (mode == ITEM_STAT_MODE_G_LORE)
      send_to_group(NULL, GROUP(ch), "Makes: %s!\r\n", rnum == NOTHING ? "Nothing" : obj_proto[rnum].short_description);
    else
      send_to_char(ch, "Makes: %s!\r\n", rnum == NOTHING ? "Nothing" : obj_proto[rnum].short_description);

    if (mode == ITEM_STAT_MODE_G_LORE)
      send_to_group(NULL, GROUP(ch), "Time: %d\r\n", CRAFT_TIMER(craft));
    else
      send_to_char(ch, "Time: %d\r\n", CRAFT_TIMER(craft));

    while ((req = (struct requirement_data *)simple_list(craft->requirements)) != NULL)
    {
      if ((rnum = real_object(req->req_vnum)) == NOWHERE)
      {
        if (mode == ITEM_STAT_MODE_G_LORE)
          send_to_group(NULL, GROUP(ch), "Req: NO OBJECT! ");
        else
          send_to_char(ch, "Req: NO OBJECT! ");
      }
      else
      {
        if (mode == ITEM_STAT_MODE_G_LORE)
          send_to_group(NULL, GROUP(ch), "Req: %-14s (%-2d) %s ", obj_proto[rnum].short_description, req->req_amount, IS_SET(req->req_flags, REQ_FLAG_IN_ROOM) ? "In Room" : "In Inventory");
        else
          send_to_char(ch, "Req: %-14s (%-2d) %s ", obj_proto[rnum].short_description, req->req_amount, IS_SET(req->req_flags, REQ_FLAG_IN_ROOM) ? "In Room" : "In Inventory");
      }

      if (mode == ITEM_STAT_MODE_G_LORE)
        send_to_group(NULL, GROUP(ch), "%s\r\n", IS_SET(req->req_flags, REQ_FLAG_NO_REMOVE) ? "No Remove" : "Remove");
      else
        send_to_char(ch, "%s\r\n", IS_SET(req->req_flags, REQ_FLAG_NO_REMOVE) ? "No Remove" : "Remove");
    }
  }
  else
  {
    if (mode == ITEM_STAT_MODE_G_LORE)
      send_to_group(NULL, GROUP(ch), "Item: %-14s   Print Id: %d\r\n", CRAFT_NAME(craft), CRAFT_ID(craft));
    else
      send_to_char(ch, "Item: %-14s   Print Id: %d\r\n", CRAFT_NAME(craft), CRAFT_ID(craft));

    rnum = real_object(CRAFT_OBJVNUM(craft));

    if (mode == ITEM_STAT_MODE_G_LORE)
      send_to_group(NULL, GROUP(ch), "These prints display in detail the how-to of creating %s.\r\n", rnum == NOTHING ? "Nothing" : obj_proto[rnum].short_description);
    else
      send_to_char(ch, "These prints display in detail the how-to of creating %s.\r\n", rnum == NOTHING ? "Nothing" : obj_proto[rnum].short_description);

    if (mode == ITEM_STAT_MODE_G_LORE)
      send_to_group(NULL, GROUP(ch), "Judging by the difficulty, you estimate that will take about %d seconds.\r\n", CRAFT_TIMER(craft));
    else
      send_to_char(ch, "Judging by the difficulty, you estimate that will take about %d seconds.\r\n", CRAFT_TIMER(craft));

    if (mode == ITEM_STAT_MODE_G_LORE)
      send_to_group(NULL, GROUP(ch), "Gazing at the requirements list, you envision what you need:\r\n");
    else
      send_to_char(ch, "Gazing at the requirements list, you envision what you need:\r\n");

    while ((req = (struct requirement_data *)simple_list(craft->requirements)) != NULL)
    {
      if ((rnum = real_object(req->req_vnum)) == NOWHERE)
      {
        if (mode == ITEM_STAT_MODE_G_LORE)
          send_to_group(NULL, GROUP(ch), "Req: NO OBJECT! ");
        else
          send_to_char(ch, "Req: NO OBJECT! ");
      }
      else
      {
        if (mode == ITEM_STAT_MODE_G_LORE)
          send_to_group(NULL, GROUP(ch), "  %d, %s %s.\r\n", req->req_amount, obj_proto[rnum].short_description, IS_SET(req->req_flags, REQ_FLAG_IN_ROOM) ? "in the room" : "in your possession");
        else
          send_to_char(ch, "  %d, %s %s.\r\n", req->req_amount, obj_proto[rnum].short_description, IS_SET(req->req_flags, REQ_FLAG_IN_ROOM) ? "in the room" : "in your possession");
      }
    }
  }
}

EVENTFUNC(event_craft)
{
  struct mud_event_data *pMudEvent;
  struct craft_data *craft;
  struct char_data *ch = NULL;
  struct obj_data *obj;
  int missing, skill, rand;

  pMudEvent = (struct mud_event_data *)event_obj;
  ch = (struct char_data *)pMudEvent->pStruct;

  if (FIGHTING(ch))
  {
    send_to_char(ch, "You abort your attempt at crafting.\r\n");
    return (0);
  }

  if (GET_POS(ch) != POS_STANDING)
  {
    send_to_char(ch, "You must be standing to craft.\r\n");
    return (0);
  }

  if ((craft = get_craft_from_arg(pMudEvent->sVariables)) == NULL)
  {
    mudlog(CMP, LVL_STAFF, TRUE, "SYSERR: Event Craft called without craft.");
    return (0);
  }

  if ((missing = missing_craft_requirements(ch, craft)) > 0)
  {
    send_to_char(ch, "You are still missing %d components.\r\n", missing);
    return (0);
  }

  skill = GET_SKILL(ch, CRAFT_SKILL(craft));
  rand = rand_number(0, (CRAFT_SKILL_LEVEL(craft) * 2));
  rand = MIN(151, rand);

  if (skill > rand)
  {

    if ((obj = read_object(CRAFT_OBJVNUM(craft), VIRTUAL)) == NULL)
    {
      send_to_char(ch, "You seem to have an issue with your crafting.\r\n");
      mudlog(CMP, LVL_STAFF, TRUE, "SYSERR: Event Craft called without created object.");
      return (0);
    }

    remove_components(ch, craft, TRUE);
    obj_to_char(obj, ch);

    if (!CRAFT_MSG_SELF(craft))
      send_to_char(ch, "You have created %s.\r\n", obj->short_description);
    else
      act(CRAFT_MSG_SELF(craft), TRUE, ch, obj, 0, TO_CHAR);

    if (CRAFT_MSG_ROOM(craft))
      act(CRAFT_MSG_ROOM(craft), TRUE, ch, obj, 0, TO_NOTVICT);
  }
  else if (skill > (rand / 2))
  {
    act("You struggle in your attempt to craft, but you continue on.", TRUE, ch, 0, 0, TO_CHAR);
    act("$n struggles in $s attempt to craft.", TRUE, ch, 0, 0, TO_NOTVICT);
    return (CRAFT_TIMER(craft) * PASSES_PER_SEC);
  }
  else
  {
    remove_components(ch, craft, FALSE);
    act("You mess up your attempt to craft.", TRUE, ch, 0, 0, TO_CHAR);
    act("$n messes up $s attempt to craft.", TRUE, ch, 0, 0, TO_NOTVICT);
  }

  return (0);
}

ACMDU(do_craft)
{
  struct craft_data *craft;
  struct obj_data *obj;
  int missing;

  if (IS_NPC(ch))
    return;

  if (!*argument)
  {
    list_available_crafts(ch);
    return;
  }

  if (char_has_mud_event(ch, eCRAFT))
  {
    send_to_char(ch, "You are already attempting to craft something.\r\n");
    return;
  }

  skip_spaces(&argument);

  if ((obj = get_obj_in_list_vis(ch, argument, NULL, ch->carrying)) == NULL ||
      GET_OBJ_TYPE(obj) != ITEM_BLUEPRINT ||
      (craft = get_craft_from_id(GET_OBJ_VAL(obj, 0))) == NULL)
  {
    if ((craft = get_craft_from_arg(argument)) == NULL || IS_SET(CRAFT_FLAGS(craft), CRAFT_RECIPE))
    {
      send_to_char(ch, "What are you trying to craft?\r\n");
      return;
    }
  }

  if ((missing = missing_craft_requirements(ch, craft)) > 0)
  {
    send_to_char(ch, "You are still missing %d components.\r\n", missing);
    return;
  }

  /* Other Checks */
  if (GET_SKILL(ch, CRAFT_SKILL(craft)) < CRAFT_SKILL_LEVEL(craft))
  {
    send_to_char(ch, "You aren't skilled enough in %s to craft that, you need at least %d.\r\n",
                 spell_info[CRAFT_SKILL(craft)].name, CRAFT_SKILL_LEVEL(craft));
    return;
  }

  /* Activate the Event */
  NEW_EVENT(eCRAFT, ch, CRAFT_NAME(craft), CRAFT_TIMER(craft) * PASSES_PER_SEC);
  act("You begin attempting to craft.", TRUE, ch, 0, 0, TO_CHAR);
  act("$n is attempting to craft.", TRUE, ch, 0, 0, TO_NOTVICT);
}

/***********************************************************************
 CRAFT EDITOR
 **********************************************************************/
static void copy_requirement(struct requirement_data *to, struct requirement_data *from)
{
  *to = *from;
}

static void copy_craft(struct craft_data *to, struct craft_data *from)
{
  struct requirement_data *from_req, *to_req;
  struct list_data *tmp_list;
  struct iterator_data Iterator;

  if (from == NULL)
    return;

  tmp_list = to->requirements;

  *to = *from;
  if (to->requirements != tmp_list)
    to->requirements = tmp_list;

  if (CRAFT_NAME(from))
    to->craft_name = strdup(CRAFT_NAME(from));

  if (CRAFT_MSG_SELF(from))
    to->craft_msg_self = strdup(CRAFT_MSG_SELF(from));

  if (CRAFT_MSG_ROOM(from))
    to->craft_msg_room = strdup(CRAFT_MSG_ROOM(from));

  if (from->requirements->iSize)
  {
    for (from_req = (struct requirement_data *)merge_iterator(&Iterator, from->requirements);
         from_req;
         from_req = next_in_list(&Iterator))
    {
      to_req = create_requirement();
      copy_requirement(to_req, from_req);
      add_to_list(to_req, to->requirements);
    }

    remove_iterator(&Iterator);
  }
}

static void craftedit_setup_new(struct descriptor_data *d)
{
  OLC_CRAFT(d) = create_craft();
  CRAFT_ID(OLC_CRAFT(d)) = OLC_NUM(d);

  OLC_CRAFT(d)->craft_msg_self = strdup("You craft $p.");
  OLC_CRAFT(d)->craft_msg_room = strdup("$n crafts $p.");

  OLC_VAL(d) = 0;
}

void craftedit_setup_existing(struct descriptor_data *d, int craft_id)
{
  struct craft_data *craft;

  craft = get_craft_from_id(craft_id);

  /* Allocate craft in memory. */
  OLC_CRAFT(d) = create_craft();
  copy_craft(OLC_CRAFT(d), craft);

  /* Attach new craft to player's descriptor. */
  OLC_VAL(d) = 0;
}

void craftedit_save_internal(struct craft_data *craft)
{
  struct craft_data *temp_craft;

  if ((temp_craft = get_craft_from_id(CRAFT_ID(craft))) != NULL)
  {
    remove_from_list(temp_craft, global_craft_list);
    free_craft(temp_craft);
  }

  temp_craft = create_craft();
  copy_craft(temp_craft, craft);

  add_to_list(temp_craft, global_craft_list);
  sort_craft_list();
}

ACMD(do_oasis_craftedit)
{
  struct descriptor_data *d;
  int idnum;

  if (!*argument)
  {
    send_to_char(ch, "Which craft would you like to edit?\r\n");
    return;
  }

  if ((idnum = atoi(argument)) <= 0)
  {
    send_to_char(ch, "Please select a craft to edit.\r\n");
    return;
  }

  for (d = descriptor_list; d; d = d->next)
    if (STATE(d) == CON_CRAFTEDIT && d->olc && OLC_NUM(d) == idnum)
    {
      send_to_char(ch, "Someone is currently editing that craft.\r\n");
      return;
    }

  /* Point d to the builder's descriptor (for easier typing later). */
  d = ch->desc;

  /* Give the descriptor an OLC structure. */
  if (d->olc)
  {
    mudlog(BRF, LVL_IMMORT, TRUE,
           "SYSERR: do_oasis: Player already had olc structure.");
    free(d->olc);
  }

  CREATE(d->olc, struct oasis_olc_data, 1);

  OLC_CRAFT_REQ(d) = NULL;
  OLC_NUM(d) = idnum;

  if (get_craft_from_id(idnum))
    craftedit_setup_existing(d, idnum);
  else
    craftedit_setup_new(d);

  craftedit_disp_menu(d);
  STATE(d) = CON_CRAFTEDIT;

  /* Send the OLC message to the players in the same room as the builder. */
  act("$n starts using OLC.", TRUE, d->character, 0, 0, TO_ROOM);
  SET_BIT_AR(PLR_FLAGS(ch), PLR_WRITING);

  /* Log the OLC message. */
  mudlog(CMP, LVL_IMMORT, TRUE, "OLC: %s starts editing the craft file",
         GET_NAME(ch));
}

/* Display craft skill menu. */
static void craftedit_disp_skill_menu(struct descriptor_data *d)
{
  int counter, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);

  for (counter = TOP_CRAFT_SKILL; counter < BOTTOM_CRAFT_SKILL; counter++)
  {
    if (spell_info[counter].min_level[0] == LVL_IMPL + 1) /* UNUSED */
      continue;
    write_to_output(d, "\t2%3d\t3) \t1%-20.20s\tn %s", counter,
                    spell_info[counter].name, !(++columns % 3) ? "\r\n" : "");
  }
  write_to_output(d, "\r\n%sEnter skill choice (-1 for none) : ", nrm);
}

/* Display craft requirement flags menu. */
static void craftedit_disp_req_flags(struct descriptor_data *d)
{
  int i, count = 0, columns = 0;
  char flags[MAX_STRING_LENGTH] = {'\0'};

  get_char_colors(d->character);
  clear_screen(d);

  for (i = 0; i < NUM_REQ_FLAGS; i++)
  {
    write_to_output(d, "\t2%2d\t3) \t1%-20.20s\tn  %s", ++count, requirement_flags[i],
                    !(++columns % 2) ? "\r\n" : "");
  }

  sprintbit(OLC_CRAFT_REQ(d)->req_flags, requirement_flags, flags, sizeof(flags));
  write_to_output(d, "\r\nCurrent flags : %s\r\nEnter craft req flags (0 to quit) : ", flags);
}

/* Display craft flags menu. */
static void craftedit_disp_craft_flags(struct descriptor_data *d)
{
  int i, count = 0, columns = 0;
  char flags[MAX_STRING_LENGTH] = {'\0'};

  get_char_colors(d->character);
  clear_screen(d);

  /* Mob flags has special handling to remove illegal flags from the list */
  for (i = 0; i < NUM_CRAFT_FLAGS; i++)
  {
    write_to_output(d, "\t2%2d\t3) \t1%-20.20s\tn  %s", ++count, craft_flags[i],
                    !(++columns % 2) ? "\r\n" : "");
  }

  sprintbit(CRAFT_FLAGS(OLC_CRAFT(d)), craft_flags, flags, sizeof(flags));
  write_to_output(d, "\r\nCurrent flags : %s\r\nEnter craft flags (0 to quit) : ", flags);
}

static void craftedit_requirement_menu(struct descriptor_data *d)
{
  struct craft_data *c = OLC_CRAFT(d);
  struct requirement_data *r;
  char buf[MAX_STRING_LENGTH] = {'\0'};
  obj_vnum vnum;

  write_to_output(d, "\t1Craft Requirements:\tn\r\n");

  if (c->requirements->iSize)
  {
    while ((r = (struct requirement_data *)simple_list(c->requirements)) != NULL)
    {
      sprintbit(r->req_flags, requirement_flags, buf, sizeof(buf));
      write_to_output(d, "  \t2[\t3%-5d\t2]\t1)\t3 %d, %s \t2[\t3%s\t2]\tn\r\n",
                      r->req_vnum, r->req_amount,
                      ((vnum = real_object(r->req_vnum)) != NOTHING) ? obj_proto[vnum].short_description : "None", buf);
    }
  }
  else
    write_to_output(d, "  \t1NONE\tn\r\n");
  write_to_output(d, "N) New/Edit Craft\r\nX) Delete Craft\r\nQ) Quit\r\nSelect Option: ");
  OLC_MODE(d) = CRAFTEDIT_REQUIREMENTS;
}

static void craftedit_disp_menu(struct descriptor_data *d)
{
  struct craft_data *c = OLC_CRAFT(d);
  struct requirement_data *r;
  obj_vnum vnum;
  char buf[MAX_STRING_LENGTH] = {'\0'};

  write_to_output(d, "\t1-- Craftedit Menu\t1 : \t2[\t3%d\t2]\tn\r\n"
                     "\t21\t3) Craft Name     : \t1%s\tn\r\n"
                     "\t22\t3) Craft Timer    : \t1%d seconds\tn\r\n"
                     "\t23\t3) Craft Item     : \t1\"\tn%s\t1\"\tn\r\n"
                     "\t24\t3) Craft Self Msg : \t1\"\tn%s\t1\"\tn\r\n"
                     "\t25\t3) Craft Room Msg : \t1\"\tn%s\t1\"\tn\r\n",
                  CRAFT_ID(c),
                  CRAFT_NAME(c),
                  CRAFT_TIMER(c),
                  ((vnum = real_object(CRAFT_OBJVNUM(c))) != NOTHING) ? obj_proto[vnum].short_description : "None",
                  CRAFT_MSG_SELF(c),
                  CRAFT_MSG_ROOM(c));

  write_to_output(d, "\t2S\t3) Craft Skill    : \t1%s \t2(\t3%d\t2)\tn\r\n", CRAFT_SKILL(c) == -1 ? "No Skill" : spell_info[CRAFT_SKILL(c)].name, CRAFT_SKILL_LEVEL(c));

  sprintbit(CRAFT_FLAGS(c), craft_flags, buf, sizeof(buf));
  write_to_output(d, "\t2F\t3) Flags          : \t1%s\tn\r\n", buf);

  write_to_output(d, "\t2R\t3) Requirements   :\tn\r\n");

  if (c->requirements->iSize)
  {
    while ((r = (struct requirement_data *)simple_list(c->requirements)) != NULL)
    {
      sprintbit(r->req_flags, requirement_flags, buf, sizeof(buf));
      write_to_output(d, "   \t2[\t3%-4d\t2] \t1%-2d, \t1\"\tn%s\t1\"\tn \t2[\t3%s\t2]\tn\r\n",
                      r->req_vnum, r->req_amount,
                      ((vnum = real_object(r->req_vnum)) != NOTHING) ? obj_proto[vnum].short_description : "None", buf);
    }
  }
  else
    write_to_output(d, "  \t1None\tn\r\n");

  write_to_output(d, "\t2X\t3) Delete\tn\r\n"
                     "\t2Q\t3) Quit\tn\r\n"
                     "Enter choice: ");
  OLC_MODE(d) = CRAFTEDIT_MENU;
}

void craftedit_parse(struct descriptor_data *d, char *arg)
{
  int var;
  struct requirement_data *r = NULL;
  struct craft_data *craft;

  switch (OLC_MODE(d))
  {
  case CRAFTEDIT_MENU:
    switch (*arg)
    {
    case 'q':
    case 'Q':
      if (OLC_VAL(d))
      {
        write_to_output(d, "Would you like to save?\r\n");
        OLC_MODE(d) = CRAFTEDIT_SAVE;
      }
      else
      {
        write_to_output(d, "Aborting Craftedit!\r\n");
        cleanup_olc(d, CLEANUP_ALL);
      }
      return;
    case 'x':
    case 'X':
      write_to_output(d, "Are you sure? Y/N\r\n");
      OLC_MODE(d) = CRAFTEDIT_DELETE;
      return;
    case '1':
      write_to_output(d, "Enter craft name : ");
      OLC_MODE(d) = CRAFTEDIT_NAME;
      break;
    case '2':
      write_to_output(d, "Enter time to build (in seconds): ");
      OLC_MODE(d) = CRAFTEDIT_TIMER;
      break;
    case '3':
      write_to_output(d, "Enter craft product vnum: ");
      OLC_MODE(d) = CRAFTEDIT_VNUM;
      break;
    case '4':
      write_to_output(d, "Enter craft self message: ");
      OLC_MODE(d) = CRAFTEDIT_MSG_SELF;
      break;
    case '5':
      write_to_output(d, "Enter craft room message: ");
      OLC_MODE(d) = CRAFTEDIT_MSG_ROOM;
      break;
    case 'f':
    case 'F':
      craftedit_disp_craft_flags(d);
      OLC_MODE(d) = CRAFTEDIT_CRAFT_FLAGS;
      break;
    case 'r':
    case 'R':
      craftedit_requirement_menu(d);
      break;
    case 's':
    case 'S':
      craftedit_disp_skill_menu(d);
      OLC_MODE(d) = CRAFTEDIT_SKILL;
      break;
    }
    return;
  case CRAFTEDIT_DELETE:
    if (*arg == 'y' || *arg == 'Y')
    {
      craft = get_craft_from_id(OLC_NUM(d));
      cleanup_olc(d, CLEANUP_ALL);

      if (craft)
      {
        remove_from_list(craft, global_craft_list);
        free_craft(craft);
        save_crafts_to_disk();
        write_to_output(d, "Deleting Craft.\r\n");
      }
      return;
    }
    write_to_output(d, "Delete Aborted.\r\n");
    craftedit_disp_menu(d);
    return;
  case CRAFTEDIT_SAVE:
    if (*arg == 'y' || *arg == 'Y')
    {
      craftedit_save_internal(OLC_CRAFT(d));
      save_crafts_to_disk();
      write_to_output(d, "Saving Craft.\r\n");
      cleanup_olc(d, CLEANUP_ALL);
      return;
    }
    else if (*arg == 'n' || *arg == 'N')
    {
      write_to_output(d, "Quit Craft Editor.\r\n");
      cleanup_olc(d, CLEANUP_ALL);
      return;
    }
    write_to_output(d, "Quitting Aborted.\r\n");
    craftedit_disp_menu(d);
    return;
  case CRAFTEDIT_NAME:
    if (!genolc_checkstring(d, arg))
      break;
    if (OLC_CRAFT(d)->craft_name)
      free(OLC_CRAFT(d)->craft_name);
    OLC_CRAFT(d)->craft_name = str_udup(arg);
    break;
  case CRAFTEDIT_TIMER:
    OLC_CRAFT(d)->craft_timer = LIMIT(atoi(arg), 0, 60);
    break;
  case CRAFTEDIT_VNUM:
    var = atoi(arg);
    if (real_object(var) != NOTHING)
      OLC_CRAFT(d)->craft_object_vnum = var;
    break;
  case CRAFTEDIT_MSG_SELF:
    if (!genolc_checkstring(d, arg))
      break;
    if (OLC_CRAFT(d)->craft_msg_self)
      free(OLC_CRAFT(d)->craft_msg_self);
    delete_doubledollar(arg);
    OLC_CRAFT(d)->craft_msg_self = strdup(arg);
    break;
  case CRAFTEDIT_MSG_ROOM:
    if (!genolc_checkstring(d, arg))
      break;
    if (OLC_CRAFT(d)->craft_msg_room)
      free(OLC_CRAFT(d)->craft_msg_room);
    delete_doubledollar(arg);
    OLC_CRAFT(d)->craft_msg_room = strdup(arg);
    break;
  case CRAFTEDIT_CRAFT_FLAGS:
    if (!*arg)
    {
      write_to_output(d, "Select a flag # or press 0 to exit: ");
      return;
    }

    if ((var = atoi(arg)))
    {
      TOGGLE_BIT(CRAFT_FLAGS(OLC_CRAFT(d)), (1 << (var - 1)));
      craftedit_disp_craft_flags(d);
      return;
    }

    break;
  case CRAFTEDIT_SKILL:
    if (!*arg)
    {
      write_to_output(d, "Please select a skill: ");
      return;
    }

    OLC_CRAFT(d)->craft_skill = LIMIT(atoi(arg), 1, TOP_SKILL_DEFINE);

    write_to_output(d, "At what level?: ");
    OLC_MODE(d) = CRAFTEDIT_SKILL_LEVEL;
    return;
  case CRAFTEDIT_SKILL_LEVEL:
    if (!*arg)
    {
      write_to_output(d, "Please select a skill level: ");
      return;
    }

    OLC_CRAFT(d)->craft_skill_level = LIMIT(atoi(arg), 0, 100);
    break;
  case CRAFTEDIT_REQUIREMENTS:
    switch (*arg)
    {
    case 'n':
    case 'N':
      OLC_CRAFT_REQ(d) = NULL;
      OLC_MODE(d) = CRAFTEDIT_REQ_NEW_VNUM;
      write_to_output(d, "Select Requirement VNUM: ");
      return;
    case 'x':
    case 'X':
      if (OLC_CRAFT(d)->requirements->iSize == 0)
      {
        write_to_output(d, "No requirements to delete.\r\n");
        return;
      }
      OLC_MODE(d) = CRAFTEDIT_REQ_DELETE;
      write_to_output(d, "Select Requirement VNUM to Delete: ");
      return;
    case 'q':
    case 'Q':
      break;
    }
    break;
  case CRAFTEDIT_REQ_DELETE:
    if (!*arg)
    {
      write_to_output(d, "Select Requirement VNUM to Delete: ");
      return;
    }

    if ((r = find_requirement_in_craft(OLC_CRAFT(d), atoi(arg))) != NULL)
    {
      remove_from_list(r, OLC_CRAFT(d)->requirements);
      free(r);
      OLC_VAL(d) = 1;
      craftedit_requirement_menu(d);
      break;
    }

    write_to_output(d, "Unable to find that vnum in the requirements list.\r\n");
    return;
  case CRAFTEDIT_REQ_NEW_VNUM:
    if (!*arg)
    {
      write_to_output(d, "Select Requirement VNUM: ");
      return;
    }

    var = atoi(arg);

    if ((OLC_CRAFT_REQ(d) = find_requirement_in_craft(OLC_CRAFT(d), var)) == NULL)
    {
      if (real_object(var) == NOTHING)
      {
        write_to_output(d, "Select Requirement VNUM: ");
        return;
      }
      else
      {
        OLC_CRAFT_REQ(d) = create_requirement();
        OLC_CRAFT_REQ(d)->req_vnum = var;
      }
    }

    OLC_MODE(d) = CRAFTEDIT_REQ_NEW_AMOUNT;
    write_to_output(d, "How many do you need?: ");
    return;
  case CRAFTEDIT_REQ_NEW_AMOUNT:
    if (!*arg)
    {
      write_to_output(d, "How many do you need?: ");
      return;
    }

    OLC_CRAFT_REQ(d)->req_amount = atoi(arg);

    craftedit_disp_req_flags(d);
    OLC_MODE(d) = CRAFTEDIT_REQ_FLAGS;
    return;
  case CRAFTEDIT_REQ_FLAGS:
    if (!*arg)
    {
      write_to_output(d, "Please select a flag #, or type 0 to return. ");
      return;
    }

    if ((var = atoi(arg)))
    {
      TOGGLE_BIT(OLC_CRAFT_REQ(d)->req_flags, (1 << (var - 1)));
      craftedit_disp_req_flags(d);
      return;
    }

    if (find_requirement_in_craft(OLC_CRAFT(d), OLC_CRAFT_REQ(d)->req_vnum) == NULL)
    {
      add_to_list(OLC_CRAFT_REQ(d), OLC_CRAFT(d)->requirements);
      write_to_output(d, "New Craft Requirement Added To Craft.\r\n");
    }
    else
      write_to_output(d, "Craft Requirement Modified.\r\n");

    OLC_CRAFT_REQ(d) = NULL;
    craftedit_requirement_menu(d);
    return;
  default:
    write_to_output(d, "Oops, care to try that again?\r\n");
    return;
  }
  craftedit_disp_menu(d);
  OLC_VAL(d) = 1;
}
