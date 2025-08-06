/**************************************************************************
 *  File: dg_db_scripts.c                              Part of LuminariMUD *
 *  Usage: Contains routines to handle db functions for scripts and trigs. *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 *                                                                         *
 *  Death's Gate MUD is based on CircleMUD, Copyright (C) 1993, 94.        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 *                                                                         *
 *  $Author: Mark A. Heilpern/egreen/Welcor $                              *
 *  $Date: 2004/10/11 12:07:00$                                            *
 *  $Revision: 1.0.14 $                                                    *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "dg_scripts.h"
#include "utils.h"
#include "db.h"
#include "handler.h"
#include "dg_event.h"
#include "comm.h"
#include "constants.h"
#include "interpreter.h" /* For half_chop */

/* local functions */
static void trig_data_init(trig_data *this_data);

void parse_trigger(FILE *trig_f, int nr)
{
  int t[2], k, attach_type;
  char line[MEDIUM_STRING] = {'\0'}, *cmds, *s, flags[MEDIUM_STRING] = {'\0'},
       errors[MAX_INPUT_LENGTH] = {'\0'};
  struct cmdlist_element *cle;
  struct index_data *t_index;
  struct trig_data *trig;

  CREATE(trig, trig_data, 1);
  CREATE(t_index, index_data, 1);

  t_index->vnum = nr;
  t_index->number = 0;
  t_index->func = NULL;
  t_index->proto = trig;

  snprintf(errors, sizeof(errors), "trig vnum %d", nr);

  trig->nr = top_of_trigt;
  trig->name = fread_string(trig_f, errors);

  get_line(trig_f, line);
  k = sscanf(line, "%d %s %d", &attach_type, flags, t);
  trig->attach_type = (byte)attach_type;
  trig->trigger_type = (long)asciiflag_conv(flags);
  trig->narg = (k == 3) ? t[0] : 0;

  trig->arglist = fread_string(trig_f, errors);

  cmds = s = fread_string(trig_f, errors);

  CREATE(trig->cmdlist, struct cmdlist_element, 1);
  trig->cmdlist->cmd = strdup(strtok(s, "\n\r"));
  cle = trig->cmdlist;

  while ((s = strtok(NULL, "\n\r")))
  {
    CREATE(cle->next, struct cmdlist_element, 1);
    cle = cle->next;
    cle->cmd = strdup(s);
  }

  free(cmds);

  trig_index[top_of_trigt++] = t_index;
}

/* Create a new trigger from a prototype. nr is the real number of the trigger. */
trig_data *read_trigger(int nr)
{
  index_data *t_index;
  trig_data *trig;

  if (nr >= top_of_trigt)
    return NULL;
  if ((t_index = trig_index[nr]) == NULL)
    return NULL;

  CREATE(trig, trig_data, 1);
  trig_data_copy(trig, t_index->proto);

  t_index->number++;

  return trig;
}

static void trig_data_init(trig_data *this_data)
{
  this_data->nr = NOTHING;
  this_data->data_type = 0;
  this_data->name = NULL;
  this_data->trigger_type = 0;
  this_data->cmdlist = NULL;
  this_data->curr_state = NULL;
  this_data->narg = 0;
  this_data->arglist = NULL;
  this_data->depth = 0;
  this_data->wait_event = NULL;
  this_data->purged = FALSE;
  this_data->var_list = NULL;

  this_data->next = NULL;
}

void trig_data_copy(trig_data *this_data, const trig_data *trg)
{
  trig_data_init(this_data);

  this_data->nr = trg->nr;
  this_data->attach_type = trg->attach_type;
  this_data->data_type = trg->data_type;
  if (trg->name)
    this_data->name = strdup(trg->name);
  else
  {
    this_data->name = strdup("unnamed trigger");
    log("Trigger with no name! (%d)", trg->nr);
  }
  this_data->trigger_type = trg->trigger_type;
  this_data->cmdlist = trg->cmdlist;
  this_data->narg = trg->narg;
  if (trg->arglist)
    this_data->arglist = strdup(trg->arglist);
}

/* for mobs and rooms: */
void dg_read_trigger(FILE *fp, void *proto, int type, int proto_vnum)
{
  char line[READ_SIZE];
  char junk[8];
  int vnum, rnum, count;
  char_data *mob;
  room_data *room;
  struct trig_proto_list *trg_proto, *new_trg;

  get_line(fp, line);
  count = sscanf(line, "%7s %d", junk, &vnum);

  if (count != 2)
  {
    mudlog(BRF, LVL_BUILDER, TRUE,
           "SYSERR: Error assigning trigger! - Line was\n  %s", line);
    return;
  }

  rnum = real_trigger(vnum);
  if (rnum == NOTHING)
  {
    switch (type)
    {
    case MOB_TRIGGER:
      mudlog(BRF, LVL_BUILDER, TRUE,
             "TRIGGER ERROR: Mobile '%s' (vnum #%d) has trigger #%d attached, but that trigger doesn't exist!",
             GET_NAME((char_data *)proto), proto_vnum, vnum);
      mudlog(BRF, LVL_BUILDER, TRUE,
             "TRIGGER FIX: Either create trigger #%d with 'trigedit %d', OR remove it from 'medit %d' (check 'attach')",
             vnum, vnum, proto_vnum);
      mudlog(BRF, LVL_BUILDER, TRUE,
             "TRIGGER NOTE: Use 'tlist' to see existing triggers, 'vnum trigger <keyword>' to search");
      break;
    case WLD_TRIGGER:
      mudlog(BRF, LVL_BUILDER, TRUE,
             "TRIGGER ERROR: Room #%d has trigger #%d attached, but that trigger doesn't exist!",
             proto_vnum, vnum);
      mudlog(BRF, LVL_BUILDER, TRUE,
             "TRIGGER FIX: Either create trigger #%d with 'trigedit %d', OR remove it from 'redit %d' (check 'scripts')",
             vnum, vnum, proto_vnum);
      mudlog(BRF, LVL_BUILDER, TRUE,
             "TRIGGER NOTE: Use 'tlist' to see existing triggers, 'vnum trigger <keyword>' to search");
      break;
    default:
      mudlog(BRF, LVL_BUILDER, TRUE,
             "TRIGGER ERROR: Trigger #%d doesn't exist (unknown attachment type)", vnum);
      mudlog(BRF, LVL_BUILDER, TRUE,
             "TRIGGER FIX: Create trigger #%d with 'trigedit %d' or find where it's attached",
             vnum, vnum);
      break;
    }
    return;
  }

  switch (type)
  {
  case MOB_TRIGGER:
    CREATE(new_trg, struct trig_proto_list, 1);
    new_trg->vnum = vnum;
    new_trg->next = NULL;

    mob = (char_data *)proto;
    trg_proto = mob->proto_script;
    if (!trg_proto)
    {
      mob->proto_script = trg_proto = new_trg;
    }
    else
    {
      while (trg_proto->next)
        trg_proto = trg_proto->next;
      trg_proto->next = new_trg;
    }
    break;
  case WLD_TRIGGER:
    CREATE(new_trg, struct trig_proto_list, 1);
    new_trg->vnum = vnum;
    new_trg->next = NULL;
    room = (room_data *)proto;
    trg_proto = room->proto_script;
    if (!trg_proto)
    {
      room->proto_script = trg_proto = new_trg;
    }
    else
    {
      while (trg_proto->next)
        trg_proto = trg_proto->next;
      trg_proto->next = new_trg;
    }

    if (rnum != NOTHING)
    {
      if (!(room->script))
        CREATE(room->script, struct script_data, 1);
      add_trigger(SCRIPT(room), read_trigger(rnum), -1);
    }
    else
    {
      mudlog(BRF, LVL_BUILDER, TRUE,
             "TRIGGER ERROR: Room #%d has non-existent trigger #%d assigned during zone reset",
             room->number, vnum);
      mudlog(BRF, LVL_BUILDER, TRUE,
             "TRIGGER FIX: Check 'zedit' for zone containing room #%d and remove/fix T commands",
             room->number);
    }
    break;
  default:
    mudlog(BRF, LVL_BUILDER, TRUE,
           "SYSERR: Trigger vnum #%d assigned to non-mob/obj/room", vnum);
  }
}

void dg_obj_trigger(char *line, struct obj_data *obj, int obj_vnum)
{
  char junk[8];
  int vnum, rnum, count;
  struct trig_proto_list *trg_proto, *new_trg;

  count = sscanf(line, "%s %d", junk, &vnum);

  if (count != 2)
  {
    mudlog(BRF, LVL_BUILDER, TRUE,
           "SYSERR: dg_obj_trigger() : Error assigning trigger! - Line was:\n  %s", line);
    return;
  }

  rnum = real_trigger(vnum);
  if (rnum == NOTHING)
  {
    mudlog(BRF, LVL_BUILDER, TRUE,
           "TRIGGER ERROR: Object '%s' (vnum #%d) has trigger #%d attached, but that trigger doesn't exist!",
           obj->short_description ? obj->short_description : "UNNAMED", obj_vnum, vnum);
    mudlog(BRF, LVL_BUILDER, TRUE,
           "TRIGGER FIX: Either create trigger #%d with 'trigedit %d', OR remove it from 'oedit %d' (check 'scripts')",
           vnum, vnum, obj_vnum);
    mudlog(BRF, LVL_BUILDER, TRUE,
           "TRIGGER NOTE: Use 'tlist' to see existing triggers, 'vnum trigger <keyword>' to search");
    return;
  }

  CREATE(new_trg, struct trig_proto_list, 1);
  new_trg->vnum = vnum;
  new_trg->next = NULL;

  trg_proto = obj->proto_script;
  if (!trg_proto)
  {
    obj->proto_script = trg_proto = new_trg;
  }
  else
  {
    while (trg_proto->next)
      trg_proto = trg_proto->next;
    trg_proto->next = new_trg;
  }
}

void assign_triggers(void *i, int type)
{
  struct char_data *mob = NULL;
  struct obj_data *obj = NULL;
  struct room_data *room = NULL;
  int rnum;
  struct trig_proto_list *trg_proto;

  switch (type)
  {
  case MOB_TRIGGER:
    mob = (char_data *)i;
    trg_proto = mob->proto_script;
    while (trg_proto)
    {
      rnum = real_trigger(trg_proto->vnum);
      if (rnum == NOTHING)
      {
        mudlog(BRF, LVL_BUILDER, TRUE,
               "TRIGGER ERROR: Mobile #%d has non-existent trigger #%d assigned!",
               mob_index[mob->nr].vnum, trg_proto->vnum);
        mudlog(BRF, LVL_BUILDER, TRUE,
               "TRIGGER FIX: Create trigger with 'trigedit %d' OR remove from 'medit %d'",
               trg_proto->vnum, mob_index[mob->nr].vnum);
        mudlog(BRF, LVL_BUILDER, TRUE,
               "TRIGGER HELP: Use 'tlist' to see triggers, 'attach' in medit to manage");
      }
      else
      {
        if (!SCRIPT(mob))
          CREATE(SCRIPT(mob), struct script_data, 1);
        add_trigger(SCRIPT(mob), read_trigger(rnum), -1);
      }
      trg_proto = trg_proto->next;
    }
    break;
  case OBJ_TRIGGER:
    obj = (obj_data *)i;
    trg_proto = obj->proto_script;
    while (trg_proto)
    {
      rnum = real_trigger(trg_proto->vnum);
      if (rnum == NOTHING)
      {
        mudlog(BRF, LVL_BUILDER, TRUE,
               "TRIGGER ERROR: Object #%d has non-existent trigger #%d assigned!",
               obj_index[obj->item_number].vnum, trg_proto->vnum);
        mudlog(BRF, LVL_BUILDER, TRUE,
               "TRIGGER FIX: Create trigger with 'trigedit %d' OR remove from 'oedit %d'",
               trg_proto->vnum, obj_index[obj->item_number].vnum);
        mudlog(BRF, LVL_BUILDER, TRUE,
               "TRIGGER HELP: Use 'tlist' to see triggers, 'scripts' in oedit to manage");
      }
      else
      {
        if (!SCRIPT(obj))
          CREATE(SCRIPT(obj), struct script_data, 1);
        add_trigger(SCRIPT(obj), read_trigger(rnum), -1);
      }
      trg_proto = trg_proto->next;
    }
    break;
  case WLD_TRIGGER:
    room = (struct room_data *)i;
    trg_proto = room->proto_script;
    while (trg_proto)
    {
      rnum = real_trigger(trg_proto->vnum);
      if (rnum == NOTHING)
      {
        mudlog(BRF, LVL_BUILDER, TRUE,
               "TRIGGER ERROR: Room #%d has non-existent trigger #%d assigned!",
               room->number, trg_proto->vnum);
        mudlog(BRF, LVL_BUILDER, TRUE,
               "TRIGGER FIX: Create trigger with 'trigedit %d' OR remove from 'redit %d'",
               trg_proto->vnum, room->number);
        mudlog(BRF, LVL_BUILDER, TRUE,
               "TRIGGER HELP: Use 'tlist' to see triggers, 'scripts' in redit to manage");
      }
      else
      {
        if (!SCRIPT(room))
          CREATE(SCRIPT(room), struct script_data, 1);
        add_trigger(SCRIPT(room), read_trigger(rnum), -1);
      }
      trg_proto = trg_proto->next;
    }
    break;
  default:
    mudlog(BRF, LVL_BUILDER, TRUE,
           "TRIGGER ERROR: Unknown type passed to assign_triggers() function!");
    mudlog(BRF, LVL_BUILDER, TRUE,
           "TRIGGER FIX: This is a code bug - report to developers");
    break;
  }
}
