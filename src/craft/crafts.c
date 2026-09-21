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
#include "core/sysdep.h"
#include "core/structs.h"
#include "core/utils.h"
#include "core/db.h"
#include "core/comm.h"
#include "act/act.h"
#include "core/handler.h"
#include "core/interpreter.h"
#include "core/screen.h"
#include "core/constants.h"
#include "olc/oasis.h"
#include "olc/genolc.h"
#include "magic/spells.h"
#include "events/mud_event.h"
#include "crafts.h"
#include "obj/item.h"
#include "crafting_new.h"
#include "events/activity_manager.h"
#include "events/domain_event_world.h"

#include <errno.h>
#include <limits.h>
#include <stdint.h>


/* Statics */
static void craftedit_disp_menu(struct descriptor_data *d);
static void save_crafts_to_disk(void);
static int missing_craft_requirements(struct char_data *ch, struct craft_data *craft);
static void remove_components(struct char_data *ch, struct craft_data *craft, bool success);
static bool character_meets_craft_skill(struct char_data *ch, struct craft_data *craft);
int num_crafts = 0;

struct list_data *global_craft_list = NULL;

/* Catalog records name a craft or harvest ability, or -1 for no skill. */
bool craft_skill_id_is_valid(int skill)
{
  return skill == -1 || (skill >= START_CRAFT_ABILITIES && skill <= END_HARVEST_ABILITIES);
}

/* Set a record's skill from a legacy "Skil" line (old 471 to 485 or 2071 to 2085 ids with a
 * 1 to 99 level). An id with no ability keeps its raw value and marks the record unsupported. */
static void craft_set_legacy_skill(struct craft_data *craft, int legacy_skill, int level)
{
  int ability;

  craft->craft_skill_legacy = 0;
  if (legacy_skill == -1)
  {
    craft->craft_skill = -1;
    craft->craft_skill_level = 0;
    return;
  }
  ability = craft_legacy_ability_for_skill(legacy_skill, NULL);
  if (ability < 0)
  {
    log("SYSERR: Craft %d (%s): legacy skill %d has no ability mapping; the recipe cannot run "
        "until craftedit assigns one.",
        CRAFT_ID(craft), CRAFT_NAME(craft), legacy_skill);
    craft->craft_skill = CRAFT_SKILL_UNSUPPORTED;
    craft->craft_skill_legacy = legacy_skill;
    craft->craft_skill_level = level;
    return;
  }
  craft->craft_skill = ability;
  craft->craft_skill_level =
      MAX(0, level + CRAFT_LEGACY_SKILL_PER_RANK - 1) / CRAFT_LEGACY_SKILL_PER_RANK;
}

static bool character_meets_craft_skill(struct char_data *ch, struct craft_data *craft)
{
  if (ch == NULL || craft == NULL || !craft_skill_id_is_valid(CRAFT_SKILL(craft)))
    return FALSE;

  if (CRAFT_SKILL(craft) == -1)
    return TRUE;

  return get_craft_skill_value(ch, CRAFT_SKILL(craft)) >= CRAFT_SKILL_LEVEL(craft);
}

static const char *craft_skill_name(struct craft_data *craft)
{
  int skill;

  if (craft == NULL)
    return "Invalid Skill";

  skill = CRAFT_SKILL(craft);
  if (skill == -1)
    return "No Skill";
  if (skill == CRAFT_SKILL_UNSUPPORTED)
    return "Unsupported Legacy Skill";
  if (!craft_skill_id_is_valid(skill))
    return "Invalid Skill";

  return ability_names[skill];
}

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
  new_craft->craft_skill_legacy = 0;

  new_craft->craft_msg_room = NULL;
  new_craft->craft_msg_self = NULL;

  new_craft->requirements = create_list();
  return (new_craft);
}

static struct requirement_data *create_requirement(void)
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

  if (craft == NULL)
    return;

  if (craft->craft_name)
    free(craft->craft_name);

  if (craft->craft_msg_self)
    free(craft->craft_msg_self);

  if (craft->craft_msg_room)
    free(craft->craft_msg_room);

  if (craft->requirements != NULL && craft->requirements->iSize)
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
  }
  if (craft->requirements != NULL)
    free_list(craft->requirements);
  free(craft);
}

/* Two integers from a record line; anything else, including trailing text, is a format error. */
static bool craft_parse_int_pair(const char *line, int *first, int *second)
{
  char *end = NULL;
  long a, b;

  errno = 0;
  a = strtol(line, &end, 10);
  if (end == line || errno != 0 || a < INT_MIN || a > INT_MAX)
    return false;
  line = end;
  b = strtol(line, &end, 10);
  if (end == line || errno != 0 || b < INT_MIN || b > INT_MAX)
    return false;
  while (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')
    end++;
  if (*end != '\0')
    return false;
  *first = (int)a;
  *second = (int)b;
  return true;
}

static void load_crafts_from(FILE *fp)
{
  char *line;
  char tag[6];
  struct craft_data *craft = NULL;
  struct requirement_data *requirement;
  bool in_craft = FALSE;
  bool done = FALSE;

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
            if (!craft_skill_id_is_valid(CRAFT_SKILL(craft)) &&
                CRAFT_SKILL(craft) != CRAFT_SKILL_UNSUPPORTED)
            {
              log("SYSERR: Rejecting craft %d (%s) with invalid skill id %d.", CRAFT_ID(craft),
                  CRAFT_NAME(craft), CRAFT_SKILL(craft));
              free_craft(craft);
            }
            else
              add_to_list(craft, global_craft_list);
            craft = NULL;
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
            if (sscanf(line, "%d %d %d\n", (int *)&requirement->req_vnum, &requirement->req_amount,
                       &requirement->req_flags) != 3)
            {
              log("SYSERR: Format error in Requirement");
              free(requirement); /* Free the requirement if parsing failed */
            }
            else
              add_to_list(requirement, craft->requirements);
          }
          break;
        case 'A':
          if (!strcmp(tag, "Abil"))
          {
            int ability = -1, rank = 0;

            if (!craft_parse_int_pair(line, &ability, &rank))
              log("SYSERR: Format error in craft %d ability record", CRAFT_ID(craft));
            else if (!craft_skill_id_is_valid(ability))
            {
              log("SYSERR: Craft %d (%s): ability %d is not a craft or harvest ability; the "
                  "recipe cannot run until craftedit assigns one.",
                  CRAFT_ID(craft), CRAFT_NAME(craft), ability);
              craft->craft_skill = CRAFT_SKILL_UNSUPPORTED;
              craft->craft_skill_legacy = ability;
              craft->craft_skill_level = rank;
            }
            else
            {
              craft->craft_skill = ability;
              craft->craft_skill_level = ability == -1 ? 0 : MAX(0, rank);
              craft->craft_skill_legacy = 0;
            }
          }
          break;
        case 'S':
          if (!strcmp(tag, "Skil"))
          {
            int legacy_skill = -1, level = 0;

            if (!craft_parse_int_pair(line, &legacy_skill, &level))
              log("SYSERR: Format error in Skill Level");
            else
              craft_set_legacy_skill(craft, legacy_skill, level);
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

  /* Clean up any incomplete craft that wasn't added to the list */
  if (in_craft && craft)
  {
    log("SYSERR: Craft file ended with incomplete craft definition!");
    free_craft(craft);
  }
}

void load_crafts(void)
{
  FILE *fp;

  if ((fp = fopen(CRAFT_FILE, "r")) == NULL)
  {
    log("No Craft file found!");
    return;
  }
  load_crafts_from(fp);
  fclose(fp);
}

/* write_crafts() */
static void save_crafts_to(FILE *fp)
{
  struct craft_data *c;
  struct requirement_data *r;
  struct iterator_data Iterator;

  for (c = (struct craft_data *)merge_iterator(&Iterator, global_craft_list); c;
       c = next_in_list(&Iterator))
  {
    fprintf(fp, "NEW\n");
    fprintf(fp, "Name: %s\n", CRAFT_NAME(c));
    fprintf(fp, "Id  : %d\n", CRAFT_ID(c));
    fprintf(fp, "Flag: %d\n", CRAFT_FLAGS(c));
    fprintf(fp, "Vnum: %d\n", (int)CRAFT_OBJVNUM(c));
    fprintf(fp, "Time: %d\n", CRAFT_TIMER(c));
    /* An unsupported legacy record writes its raw skill back so nothing is lost. */
    if (CRAFT_SKILL(c) == CRAFT_SKILL_UNSUPPORTED)
      fprintf(fp, "Skil: %d %d\n", CRAFT_SKILL_LEGACY(c), CRAFT_SKILL_LEVEL(c));
    else
      fprintf(fp, "Abil: %d %d\n", CRAFT_SKILL(c), CRAFT_SKILL_LEVEL(c));

    fprintf(fp, "Mslf: %s\n", CRAFT_MSG_SELF(c));
    fprintf(fp, "Mroo: %s\n", CRAFT_MSG_ROOM(c));

    /* Beginner's Note: Reset simple_list iterator before use to prevent
     * cross-contamination from previous iterations. Without this reset,
     * if simple_list was used elsewhere and not completed, it would
     * continue from where it left off instead of starting fresh. */
    simple_list(NULL);

    while ((r = (struct requirement_data *)simple_list(c->requirements)) != NULL)
      fprintf(fp, "Req : %d %d %d\n", (int)r->req_vnum, r->req_amount, r->req_flags);
    fprintf(fp, "End :\n");
  }

  remove_iterator(&Iterator);

  fprintf(fp, "$\n");
}

static void save_crafts_to_disk(void)
{
  FILE *fp;

  if ((fp = fopen_restricted(CRAFT_FILE, "w")) == NULL)
  {
    log("Cannot open craft file for writing!");
    return;
  }
  save_crafts_to(fp);
  fclose(fp);
}

#ifdef LUMINARI_CUTEST
/* Production-linked tests drive the catalog through these seams. */
void test_load_crafts_from(FILE *fp)
{
  if (global_craft_list == NULL)
    global_craft_list = create_list();
  load_crafts_from(fp);
}

void test_save_crafts_to(FILE *fp)
{
  save_crafts_to(fp);
}

void test_clear_crafts(void)
{
  struct craft_data *craft;

  if (global_craft_list == NULL)
    return;
  simple_list(NULL);
  while ((craft = (struct craft_data *)simple_list(global_craft_list)) != NULL)
  {
    remove_from_list(craft, global_craft_list);
    free_craft(craft);
    simple_list(NULL);
  }
}

int test_missing_craft_requirements(struct char_data *ch, struct craft_data *craft)
{
  return missing_craft_requirements(ch, craft);
}

void test_remove_components(struct char_data *ch, struct craft_data *craft, bool success)
{
  remove_components(ch, craft, success);
}

bool test_character_meets_craft_skill(struct char_data *ch, struct craft_data *craft)
{
  return character_meets_craft_skill(ch, craft);
}
#endif

/* Craft Handlers */
static void sort_craft_list(void)
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

  if (global_craft_list == NULL || !global_craft_list->iSize)
    return NULL;

  for (craft = (struct craft_data *)merge_iterator(&iterator, global_craft_list); craft != NULL;
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

  if (global_craft_list == NULL || !global_craft_list->iSize)
    return NULL;

  for (craft = (struct craft_data *)merge_iterator(&iterator, global_craft_list); craft != NULL;
       craft = (struct craft_data *)next_in_list(&iterator))
  {
    if (CRAFT_ID(craft) == id)
      break;
  }

  remove_iterator(&iterator);
  return (craft);
}

static struct requirement_data *find_requirement_in_craft(struct craft_data *craft, obj_vnum vnum)
{
  struct requirement_data *r = NULL;
  struct iterator_data Iterator;
  bool found = FALSE;

  if (craft->requirements->iSize == 0)
    return NULL;

  for (r = (struct requirement_data *)merge_iterator(&Iterator, craft->requirements); r;
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

static struct obj_data *get_object_from_requirement(struct char_data *ch,
                                                    struct requirement_data *req)
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

/* A consumable inventory requirement for a storable material prototype is met from the shared
 * crafting balance instead of carried objects. In-room and no-remove requirements keep their
 * object semantics; gems, fossil eggs, blueprints, and unique components stay objects. */
static int requirement_balance_material(struct requirement_data *req)
{
  obj_rnum rnum;

  if (req == NULL || IS_SET(req->req_flags, REQ_FLAG_IN_ROOM) ||
      IS_SET(req->req_flags, REQ_FLAG_NO_REMOVE))
    return CRAFT_MAT_NONE;
  if ((rnum = real_object(req->req_vnum)) == NOTHING ||
      GET_OBJ_TYPE(&obj_proto[rnum]) != ITEM_MATERIAL)
    return CRAFT_MAT_NONE;
  return craft_material_from_object(&obj_proto[rnum]);
}

/* Sum the balance units a craft needs per material, over every balance-backed requirement. */
static void craft_balance_needs(struct craft_data *craft, int needs[NUM_CRAFT_MATS],
                                bool skip_saved_on_fail)
{
  struct iterator_data iterator;
  struct requirement_data *req;
  int material;

  memset(needs, 0, sizeof(int) * NUM_CRAFT_MATS);
  for (req = (struct requirement_data *)merge_iterator(&iterator, craft->requirements); req;
       req = next_in_list(&iterator))
  {
    if (skip_saved_on_fail && IS_SET(req->req_flags, REQ_SAVE_ON_FAIL))
      continue;
    material = requirement_balance_material(req);
    if (material != CRAFT_MAT_NONE && req->req_amount > 0)
      needs[material] += req->req_amount;
  }
  remove_iterator(&iterator);
}

static bool find_requirement(struct char_data *ch, struct requirement_data *req)
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

static int missing_craft_requirements(struct char_data *ch, struct craft_data *craft)
{
  int missing = 0, material;
  int needs[NUM_CRAFT_MATS];
  struct iterator_data iterator;
  struct requirement_data *requirement;
  obj_rnum rnum;

  if (!craft->requirements->iSize)
    return (-1);

  for (requirement = (struct requirement_data *)merge_iterator(&iterator, craft->requirements);
       requirement != NULL; requirement = (struct requirement_data *)next_in_list(&iterator))
  {
    if ((rnum = real_object(requirement->req_vnum)) == NOTHING)
      continue;
    if (requirement_balance_material(requirement) != CRAFT_MAT_NONE)
      continue; /* checked in aggregate below */
    if (find_requirement(ch, requirement) == FALSE)
      missing++;
  }
  remove_iterator(&iterator);

  craft_balance_needs(craft, needs, FALSE);
  for (material = 1; material < NUM_CRAFT_MATS; material++)
    if (needs[material] > 0 && (IS_NPC(ch) || GET_CRAFT_MAT(ch, material) < needs[material]))
      missing++;

  return (missing);
}

static void remove_components(struct char_data *ch, struct craft_data *craft, bool success)
{
  struct iterator_data iterator;
  struct requirement_data *req;
  struct obj_data *obj;
  int count, material;
  int needs[NUM_CRAFT_MATS];

  /* Balance-backed requirements are debited in aggregate; save-on-fail applies to them too. */
  craft_balance_needs(craft, needs, !success);
  for (material = 1; material < NUM_CRAFT_MATS; material++)
  {
    if (needs[material] <= 0 || IS_NPC(ch))
      continue;
    if (GET_CRAFT_MAT(ch, material) < needs[material])
      log("SYSERR: Craft %d (%s) debits %d %s from %s who holds %d.", CRAFT_ID(craft),
          CRAFT_NAME(craft), needs[material], crafting_materials[material], GET_NAME(ch),
          GET_CRAFT_MAT(ch, material));
    GET_CRAFT_MAT(ch, material) = MAX(0, GET_CRAFT_MAT(ch, material) - needs[material]);
  }

  for (req = (struct requirement_data *)merge_iterator(&iterator, craft->requirements); req;
       req = next_in_list(&iterator))
  {
    if (IS_SET(req->req_flags, REQ_FLAG_NO_REMOVE))
      continue;
    if (!success && IS_SET(req->req_flags, REQ_SAVE_ON_FAIL))
      continue;
    if (requirement_balance_material(req) != CRAFT_MAT_NONE)
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

  if (global_craft_list != NULL && global_craft_list->iSize > 0)
  {
    send_to_char(ch, "\t1Crafts:\r\n"
                     "\t2ID  ) Name                       VNUM   Item Name\tn\r\n");

    /* Beginner's Note: Reset simple_list iterator before use to prevent
     * cross-contamination from previous iterations. Without this reset,
     * if simple_list was used elsewhere and not completed, it would
     * continue from where it left off instead of starting fresh. */
    simple_list(NULL);

    while ((craft = (struct craft_data *)simple_list(global_craft_list)) != NULL)
    {
      send_to_char(ch, "\t2%-4d)\t3 %-22s -> \t1[\t2%-4" PRI_IDX "\t1] [\t2%s\t1]\tn\r\n",
                   CRAFT_ID(craft), CRAFT_NAME(craft), CRAFT_OBJVNUM(craft),
                   (vnum = real_object(CRAFT_OBJVNUM(craft))) != NOWHERE
                       ? obj_proto[vnum].short_description
                       : "MISSING OBJECT");
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

  if (global_craft_list != NULL && global_craft_list->iSize > 0)
  {
    /* Beginner's Note: Reset simple_list iterator before use to prevent
     * cross-contamination from previous iterations. Without this reset,
     * if simple_list was used elsewhere and not completed, it would
     * continue from where it left off instead of starting fresh. */
    simple_list(NULL);

    while ((craft = (struct craft_data *)simple_list(global_craft_list)) != NULL)
    {
      if (IS_SET(CRAFT_FLAGS(craft), CRAFT_RECIPE))
        continue;
      if (!character_meets_craft_skill(ch, craft))
        continue;
      missing = missing_craft_requirements(ch, craft);
      send_to_char(ch, " %d) %s%s%s\r\n", ++count, missing ? CCRED(ch, C_NRM) : CCGRN(ch, C_NRM),
                   CRAFT_NAME(craft), CCNRM(ch, C_NRM));
    }
  }

  if (!count)
    send_to_char(ch, "   You do not currently know of any crafts.\r\n");
  else
    send_to_char(ch, "\r\n%sAll Requirement Met    %sMissing Requirements%s\r\n", CCGRN(ch, C_NRM),
                 CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
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
      send_to_group(NULL, GROUP(ch), "Makes: %s!\r\n",
                    rnum == NOTHING ? "Nothing" : obj_proto[rnum].short_description);
    else
      send_to_char(ch, "Makes: %s!\r\n",
                   rnum == NOTHING ? "Nothing" : obj_proto[rnum].short_description);

    if (mode == ITEM_STAT_MODE_G_LORE)
      send_to_group(NULL, GROUP(ch), "Time: %d\r\n", CRAFT_TIMER(craft));
    else
      send_to_char(ch, "Time: %d\r\n", CRAFT_TIMER(craft));

    /* Beginner's Note: Reset simple_list iterator before use to prevent
     * cross-contamination from previous iterations. Without this reset,
     * if simple_list was used elsewhere and not completed, it would
     * continue from where it left off instead of starting fresh. */
    simple_list(NULL);

    while ((req = (struct requirement_data *)simple_list(craft->requirements)) != NULL)
    {
      if ((rnum = real_object(req->req_vnum)) == NOWHERE)
      {
        if (mode == ITEM_STAT_MODE_G_LORE)
          send_to_group(NULL, GROUP(ch), "Req: NO OBJECT! ");
        else
          send_to_char(ch, "Req: NO OBJECT! ");
      }
      else if (requirement_balance_material(req) != CRAFT_MAT_NONE)
      {
        if (mode == ITEM_STAT_MODE_G_LORE)
          send_to_group(NULL, GROUP(ch), "Req: %-14s (%-2d) %s ",
                        crafting_materials[requirement_balance_material(req)], req->req_amount,
                        "From Crafting Materials");
        else
          send_to_char(ch, "Req: %-14s (%-2d) %s ",
                       crafting_materials[requirement_balance_material(req)], req->req_amount,
                       "From Crafting Materials");
      }
      else
      {
        if (mode == ITEM_STAT_MODE_G_LORE)
          send_to_group(NULL, GROUP(ch), "Req: %-14s (%-2d) %s ", obj_proto[rnum].short_description,
                        req->req_amount,
                        IS_SET(req->req_flags, REQ_FLAG_IN_ROOM) ? "In Room" : "In Inventory");
        else
          send_to_char(ch, "Req: %-14s (%-2d) %s ", obj_proto[rnum].short_description,
                       req->req_amount,
                       IS_SET(req->req_flags, REQ_FLAG_IN_ROOM) ? "In Room" : "In Inventory");
      }

      if (mode == ITEM_STAT_MODE_G_LORE)
        send_to_group(NULL, GROUP(ch), "%s\r\n",
                      IS_SET(req->req_flags, REQ_FLAG_NO_REMOVE) ? "No Remove" : "Remove");
      else
        send_to_char(ch, "%s\r\n",
                     IS_SET(req->req_flags, REQ_FLAG_NO_REMOVE) ? "No Remove" : "Remove");
    }
  }
  else
  {
    if (mode == ITEM_STAT_MODE_G_LORE)
      send_to_group(NULL, GROUP(ch), "Item: %-14s   Print Id: %d\r\n", CRAFT_NAME(craft),
                    CRAFT_ID(craft));
    else
      send_to_char(ch, "Item: %-14s   Print Id: %d\r\n", CRAFT_NAME(craft), CRAFT_ID(craft));

    rnum = real_object(CRAFT_OBJVNUM(craft));

    if (mode == ITEM_STAT_MODE_G_LORE)
      send_to_group(NULL, GROUP(ch),
                    "These prints display in detail the how-to of creating %s.\r\n",
                    rnum == NOTHING ? "Nothing" : obj_proto[rnum].short_description);
    else
      send_to_char(ch, "These prints display in detail the how-to of creating %s.\r\n",
                   rnum == NOTHING ? "Nothing" : obj_proto[rnum].short_description);

    if (mode == ITEM_STAT_MODE_G_LORE)
      send_to_group(NULL, GROUP(ch),
                    "Judging by the difficulty, you estimate that will take about %d seconds.\r\n",
                    CRAFT_TIMER(craft));
    else
      send_to_char(ch,
                   "Judging by the difficulty, you estimate that will take about %d seconds.\r\n",
                   CRAFT_TIMER(craft));

    if (mode == ITEM_STAT_MODE_G_LORE)
      send_to_group(NULL, GROUP(ch),
                    "Gazing at the requirements list, you envision what you need:\r\n");
    else
      send_to_char(ch, "Gazing at the requirements list, you envision what you need:\r\n");

    /* Beginner's Note: Reset simple_list iterator before use to prevent
     * cross-contamination from previous iterations. Without this reset,
     * if simple_list was used elsewhere and not completed, it would
     * continue from where it left off instead of starting fresh. */
    simple_list(NULL);

    while ((req = (struct requirement_data *)simple_list(craft->requirements)) != NULL)
    {
      if ((rnum = real_object(req->req_vnum)) == NOWHERE)
      {
        if (mode == ITEM_STAT_MODE_G_LORE)
          send_to_group(NULL, GROUP(ch), "Req: NO OBJECT! ");
        else
          send_to_char(ch, "Req: NO OBJECT! ");
      }
      else if (requirement_balance_material(req) != CRAFT_MAT_NONE)
      {
        if (mode == ITEM_STAT_MODE_G_LORE)
          send_to_group(NULL, GROUP(ch), "Req: %-14s (%-2d) %s ",
                        crafting_materials[requirement_balance_material(req)], req->req_amount,
                        "From Crafting Materials");
        else
          send_to_char(ch, "Req: %-14s (%-2d) %s ",
                       crafting_materials[requirement_balance_material(req)], req->req_amount,
                       "From Crafting Materials");
      }
      else
      {
        if (mode == ITEM_STAT_MODE_G_LORE)
          send_to_group(NULL, GROUP(ch), "  %d, %s %s.\r\n", req->req_amount,
                        obj_proto[rnum].short_description,
                        IS_SET(req->req_flags, REQ_FLAG_IN_ROOM) ? "in the room"
                                                                 : "in your possession");
        else
          send_to_char(ch, "  %d, %s %s.\r\n", req->req_amount, obj_proto[rnum].short_description,
                       IS_SET(req->req_flags, REQ_FLAG_IN_ROOM) ? "in the room"
                                                                : "in your possession");
      }
    }
  }
}

/* One catalog attempt on the activity manager: the timer runs as a timed step, a "struggle"
 * result retries with the same delay, success or failure ends the work. Nothing is consumed
 * before the attempt resolves; a cancellation spends nothing. */
static long catalog_craft_step(struct char_data *ch, void *target, void *context)
{
  struct craft_data *craft = get_craft_from_id((int)(intptr_t)context);
  struct obj_data *obj;
  int missing, skill, rand;

  (void)target;
  if (craft == NULL)
  {
    mudlog(CMP, LVL_STAFF, TRUE, "SYSERR: catalog craft step without a craft.");
    return 0;
  }
  if (FIGHTING(ch))
  {
    send_to_char(ch, "You abort your attempt to craft.\r\n");
    return 0;
  }
  if (GET_POS(ch) != POS_STANDING)
  {
    send_to_char(ch, "You must be standing to craft.\r\n");
    return 0;
  }
  if ((missing = missing_craft_requirements(ch, craft)) > 0)
  {
    send_to_char(ch, "You are still missing %d components.\r\n", missing);
    return 0;
  }
  if (!craft_skill_id_is_valid(CRAFT_SKILL(craft)) || !character_meets_craft_skill(ch, craft))
  {
    send_to_char(ch, "You are no longer able to craft that.\r\n");
    return 0;
  }
  if (real_object(CRAFT_OBJVNUM(craft)) == NOTHING)
  {
    send_to_char(ch, "That craft has no usable result; please report it.\r\n");
    mudlog(CMP, LVL_STAFF, TRUE, "SYSERR: Craft %d (%s) has missing output prototype %d.",
           CRAFT_ID(craft), CRAFT_NAME(craft), (int)CRAFT_OBJVNUM(craft));
    return 0;
  }

  /* The roll keeps its legacy 1 to 99 scale: the rank and the threshold both read through the
   * legacy equivalent, so converting a record does not change its odds. */
  if (CRAFT_SKILL(craft) == -1)
  {
    skill = 1;
    rand = 0;
  }
  else
  {
    skill = craft_legacy_skill_equivalent(ch, CRAFT_SKILL(craft));
    rand = rand_number(0, (CRAFT_SKILL_LEVEL(craft) * CRAFT_LEGACY_SKILL_PER_RANK * 2));
    rand = MIN(151, rand);
  }

  if (skill > rand)
  {
    if ((obj = read_object(CRAFT_OBJVNUM(craft), VIRTUAL)) == NULL)
    {
      send_to_char(ch, "You seem to have an issue with your crafting.\r\n");
      mudlog(CMP, LVL_STAFF, TRUE, "SYSERR: catalog craft could not create its object.");
      return 0;
    }
    remove_components(ch, craft, TRUE);
    obj_to_char(obj, ch);
    if (!CRAFT_MSG_SELF(craft))
      send_to_char(ch, "You have created %s.\r\n", obj->short_description);
    else
      act(CRAFT_MSG_SELF(craft), TRUE, ch, obj, 0, TO_CHAR);
    if (CRAFT_MSG_ROOM(craft))
      act(CRAFT_MSG_ROOM(craft), TRUE, ch, obj, 0, TO_NOTVICT);
    /* One craft experience award per success on the recipe's ability; none for no-skill
     * recipes, retries, or failures. */
    if (CRAFT_SKILL(craft) != -1)
      gain_craft_exp(ch, craft_operation_exp(GET_OBJ_LEVEL(obj)), CRAFT_SKILL(craft), TRUE);
    return 0;
  }
  if (skill > (rand / 2))
  {
    act("You struggle in your attempt to craft, but you continue on.", TRUE, ch, 0, 0, TO_CHAR);
    act("$n struggles in $s attempt to craft.", TRUE, ch, 0, 0, TO_NOTVICT);
    return CRAFT_TIMER(craft) * PASSES_PER_SEC;
  }
  remove_components(ch, craft, FALSE);
  act("You mess up your attempt to craft.", TRUE, ch, 0, 0, TO_CHAR);
  act("$n messes up $s attempt to craft.", TRUE, ch, 0, 0, TO_NOTVICT);
  return 0;
}

static bool catalog_craft_recheck(struct char_data *ch, void *target, void *context)
{
  (void)context;
  return ch && ch->desc && STATE(ch->desc) == CON_PLAYING && !FIGHTING(ch) &&
         target == &world[IN_ROOM(ch)];
}

static bool start_catalog_craft(struct char_data *ch, struct craft_data *craft)
{
  struct primary_activity_definition definition = {0};

  definition.type = PRIMARY_ACTIVITY_CRAFT;
  definition.display_name = "crafting from the catalog";
  definition.capabilities = PRIMARY_ACTIVITY_CAP_HANDS | PRIMARY_ACTIVITY_CAP_ATTENTION;
  definition.traits = PRIMARY_ACTIVITY_TRAIT_STATIONARY | PRIMARY_ACTIVITY_TRAIT_HANDS_OCCUPIED;
  definition.progress_model = PRIMARY_ACTIVITY_PROGRESS_PROGRESSIVE;
  definition.progress_owner = PRIMARY_ACTIVITY_PROGRESS_CHARACTER;
  definition.total_steps = 1U;
  definition.step_interval = MAX(1, CRAFT_TIMER(craft)) * PASSES_PER_SEC;
  definition.wall_clock = true;
  definition.movement_response = PRIMARY_ACTIVITY_RESPONSE_CANCEL;
  definition.damage_response = PRIMARY_ACTIVITY_RESPONSE_CANCEL;
  definition.combat_response = PRIMARY_ACTIVITY_RESPONSE_CANCEL;
  definition.target_loss_response = PRIMARY_ACTIVITY_RESPONSE_CANCEL;
  definition.command_response = PRIMARY_ACTIVITY_RESPONSE_REJECT;
  definition.timed_step = catalog_craft_step;
  definition.recheck = catalog_craft_recheck;
  definition.context = (void *)(intptr_t)CRAFT_ID(craft);
  return primary_activity_start(ch, domain_event_room_handle(IN_ROOM(ch)), &definition);
}

/* One crafting surface in every configuration: the project editor. */
ACMDU(do_craft)
{
  do_newcraft(ch, argument, cmd, SCMD_NEWCRAFT_CREATE);
}

ACMDU(do_craft_with_kits)
{
  struct craft_data *craft;
  struct obj_data *obj;
  struct primary_activity_snapshot snapshot;
  int missing;

  if (IS_NPC(ch))
    return;

  if (!*argument)
  {
    list_available_crafts(ch);
    return;
  }

  if (primary_activity_snapshot(ch, &snapshot))
  {
    send_to_char(ch, "You are already busy with another task.\r\n");
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
  if (!craft_skill_id_is_valid(CRAFT_SKILL(craft)))
  {
    mudlog(CMP, LVL_STAFF, TRUE, "SYSERR: Craft %d (%s) has invalid skill id %d.", CRAFT_ID(craft),
           CRAFT_NAME(craft), CRAFT_SKILL(craft));
    send_to_char(ch, "That craft has an invalid skill requirement.\r\n");
    return;
  }

  if (!character_meets_craft_skill(ch, craft))
  {
    send_to_char(ch, "You aren't skilled enough in %s to craft that, you need at least %d.\r\n",
                 craft_skill_name(craft), CRAFT_SKILL_LEVEL(craft));
    return;
  }

  if (IN_ROOM(ch) == NOWHERE || !start_catalog_craft(ch, craft))
  {
    send_to_char(ch, "You cannot begin crafting right now.\r\n");
    return;
  }
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
         from_req; from_req = next_in_list(&Iterator))
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

static void craftedit_setup_existing(struct descriptor_data *d, int craft_id)
{
  struct craft_data *craft;

  craft = get_craft_from_id(craft_id);

  /* Allocate craft in memory. */
  OLC_CRAFT(d) = create_craft();
  copy_craft(OLC_CRAFT(d), craft);

  /* Attach new craft to player's descriptor. */
  OLC_VAL(d) = 0;
}

static void craftedit_save_internal(struct craft_data *craft)
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
    if (STATE(d) == CON_CRAFTEDIT && d->olc && (int)OLC_NUM(d) == idnum)
    {
      send_to_char(ch, "Someone is currently editing that craft.\r\n");
      return;
    }

  /* Point d to the builder's descriptor (for easier typing later). */
  d = ch->desc;

  /* Give the descriptor an OLC structure. */
  if (d->olc)
  {
    mudlog(BRF, LVL_IMMORT, TRUE, "SYSERR: do_oasis: Player already had olc structure.");
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
  mudlog(CMP, LVL_IMMORT, TRUE, "OLC: %s starts editing the craft file", GET_NAME(ch));
}

/* Display craft skill menu. */
static void craftedit_disp_skill_menu(struct descriptor_data *d)
{
  int counter, columns = 0;

  get_char_colors(d->character);
  clear_screen(d);

  for (counter = START_CRAFT_ABILITIES; counter <= END_HARVEST_ABILITIES; counter++)
  {
    if (crafting_skill_type(counter) == CRAFT_SKILL_TYPE_NONE)
      continue;
    write_to_output(d, "\t2%3d\t3) \t1%-20.20s\tn %s", counter, ability_names[counter],
                    !(++columns % 3) ? "\r\n" : "");
  }
  write_to_output(d, "\r\n%sEnter craft ability choice (-1 for none) : ", nrm);
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
    /* Beginner's Note: Reset simple_list iterator before use to prevent
     * cross-contamination from previous iterations. Without this reset,
     * if simple_list was used elsewhere and not completed, it would
     * continue from where it left off instead of starting fresh. */
    simple_list(NULL);

    while ((r = (struct requirement_data *)simple_list(c->requirements)) != NULL)
    {
      sprintbit(r->req_flags, requirement_flags, buf, sizeof(buf));
      write_to_output(d, "  \t2[\t3%-5" PRI_IDX "\t2]\t1)\t3 %d, %s \t2[\t3%s\t2]\tn\r\n",
                      r->req_vnum, r->req_amount,
                      ((vnum = real_object(r->req_vnum)) != NOTHING)
                          ? obj_proto[vnum].short_description
                          : "None",
                      buf);
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

  write_to_output(d,
                  "\t1-- Craftedit Menu\t1 : \t2[\t3%d\t2]\tn\r\n"
                  "\t21\t3) Craft Name     : \t1%s\tn\r\n"
                  "\t22\t3) Craft Timer    : \t1%d seconds\tn\r\n"
                  "\t23\t3) Craft Item     : \t1\"\tn%s\t1\"\tn\r\n"
                  "\t24\t3) Craft Self Msg : \t1\"\tn%s\t1\"\tn\r\n"
                  "\t25\t3) Craft Room Msg : \t1\"\tn%s\t1\"\tn\r\n",
                  CRAFT_ID(c), CRAFT_NAME(c), CRAFT_TIMER(c),
                  ((vnum = real_object(CRAFT_OBJVNUM(c))) != NOTHING)
                      ? obj_proto[vnum].short_description
                      : "None",
                  CRAFT_MSG_SELF(c), CRAFT_MSG_ROOM(c));

  write_to_output(d, "\t2S\t3) Craft Ability  : \t1%s \t2(rank \t3%d\t2)\tn\r\n",
                  craft_skill_name(c), CRAFT_SKILL_LEVEL(c));

  sprintbit(CRAFT_FLAGS(c), craft_flags, buf, sizeof(buf));
  write_to_output(d, "\t2F\t3) Flags          : \t1%s\tn\r\n", buf);

  write_to_output(d, "\t2R\t3) Requirements   :\tn\r\n");

  if (c->requirements->iSize)
  {
    /* Beginner's Note: Reset simple_list iterator before use to prevent
     * cross-contamination from previous iterations. Without this reset,
     * if simple_list was used elsewhere and not completed, it would
     * continue from where it left off instead of starting fresh. */
    simple_list(NULL);

    while ((r = (struct requirement_data *)simple_list(c->requirements)) != NULL)
    {
      sprintbit(r->req_flags, requirement_flags, buf, sizeof(buf));
      write_to_output(
          d, "   \t2[\t3%-4" PRI_IDX "\t2] \t1%-2d, \t1\"\tn%s\t1\"\tn \t2[\t3%s\t2]\tn\r\n",
          r->req_vnum, r->req_amount,
          ((vnum = real_object(r->req_vnum)) != NOTHING) ? obj_proto[vnum].short_description
                                                         : "None",
          buf);
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

    if (!is_number(arg) || !craft_skill_id_is_valid((var = atoi(arg))) ||
        (var != -1 && crafting_skill_type(var) == CRAFT_SKILL_TYPE_NONE))
    {
      write_to_output(d, "Please select -1 or a listed craft ability: ");
      return;
    }

    OLC_CRAFT(d)->craft_skill = var;
    OLC_CRAFT(d)->craft_skill_legacy = 0;
    if (var == -1)
    {
      OLC_CRAFT(d)->craft_skill_level = 0;
      craftedit_disp_menu(d);
      return;
    }

    write_to_output(d, "At what rank?: ");
    OLC_MODE(d) = CRAFTEDIT_SKILL_LEVEL;
    return;
  case CRAFTEDIT_SKILL_LEVEL:
    if (!*arg)
    {
      write_to_output(d, "Please select a rank: ");
      return;
    }

    OLC_CRAFT(d)->craft_skill_level = LIMIT(atoi(arg), 0, UCHAR_MAX);
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
