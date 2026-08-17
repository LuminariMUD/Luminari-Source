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

  if (!get_line(trig_f, line) || (k = sscanf(line, "%d %255s %d", &attach_type, flags, t)) < 2)
  {
    log("SYSERR: Trigger #%d has an invalid numeric header.", nr);
    exit(1);
  }
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
  trig_rnum rnum;
  int vnum, count;
  char_data *mob;
  room_data *room;
  struct trig_proto_list *trg_proto, *new_trg;

  get_line(fp, line);
  count = sscanf(line, "%7s %d", junk, &vnum);

  if (count != 2)
  {
    mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: Error assigning trigger! - Line was\n  %s", line);
    return;
  }

  rnum = real_trigger(vnum);
  if (rnum == NOTHING)
  {
    switch (type)
    {
    case MOB_TRIGGER:
      mudlog(BRF, LVL_BUILDER, TRUE,
             "TRIGGER ERROR: Mobile '%s' (vnum #%d) has trigger #%d attached, but that trigger "
             "doesn't exist!",
             GET_NAME((char_data *)proto), proto_vnum, vnum);
      mudlog(BRF, LVL_BUILDER, TRUE,
             "TRIGGER FIX: Either create trigger #%d with 'trigedit %d', OR remove it from 'medit "
             "%d' (check 'attach')",
             vnum, vnum, proto_vnum);
      mudlog(
          BRF, LVL_BUILDER, TRUE,
          "TRIGGER NOTE: Use 'tlist' to see existing triggers, 'vnum trigger <keyword>' to search");
      break;
    case WLD_TRIGGER:
      mudlog(BRF, LVL_BUILDER, TRUE,
             "TRIGGER ERROR: Room #%d has trigger #%d attached, but that trigger doesn't exist!",
             proto_vnum, vnum);
      mudlog(BRF, LVL_BUILDER, TRUE,
             "TRIGGER FIX: Either create trigger #%d with 'trigedit %d', OR remove it from 'redit "
             "%d' (check 'scripts')",
             vnum, vnum, proto_vnum);
      mudlog(
          BRF, LVL_BUILDER, TRUE,
          "TRIGGER NOTE: Use 'tlist' to see existing triggers, 'vnum trigger <keyword>' to search");
      break;
    default:
      mudlog(BRF, LVL_BUILDER, TRUE,
             "TRIGGER ERROR: Trigger #%d doesn't exist (unknown attachment type)", vnum);
      mudlog(BRF, LVL_BUILDER, TRUE,
             "TRIGGER FIX: Create trigger #%d with 'trigedit %d' or find where it's attached", vnum,
             vnum);
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
      dg_script_bind_owner(SCRIPT(room), room, WLD_TRIGGER);
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
    mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: Trigger vnum #%d assigned to non-mob/obj/room", vnum);
  }
}

void dg_obj_trigger(char *line, struct obj_data *obj, int obj_vnum)
{
  char junk[8];
  trig_rnum rnum;
  int vnum, count;
  struct trig_proto_list *trg_proto, *new_trg;

  count = sscanf(line, "%7s %d", junk, &vnum);

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
           "TRIGGER ERROR: Object '%s' (vnum #%d) has trigger #%d attached, but that trigger "
           "doesn't exist!",
           obj->short_description ? obj->short_description : "UNNAMED", obj_vnum, vnum);
    mudlog(BRF, LVL_BUILDER, TRUE,
           "TRIGGER FIX: Either create trigger #%d with 'trigedit %d', OR remove it from 'oedit "
           "%d' (check 'scripts')",
           vnum, vnum, obj_vnum);
    mudlog(
        BRF, LVL_BUILDER, TRUE,
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

static void assign_trigger_list(struct trig_proto_list *trg_proto, struct script_data **script,
                                const char *owner_type, int owner_vnum, const char *editor,
                                const char *script_menu)
{
  trig_rnum rnum;

  while (trg_proto)
  {
    rnum = real_trigger(trg_proto->vnum);
    if (rnum == NOTHING)
    {
      mudlog(BRF, LVL_BUILDER, TRUE, "TRIGGER ERROR: %s #%d has non-existent trigger #%d assigned!",
             owner_type, owner_vnum, trg_proto->vnum);
      mudlog(BRF, LVL_BUILDER, TRUE,
             "TRIGGER FIX: Create trigger with 'trigedit %d' OR remove from '%s %d'",
             trg_proto->vnum, editor, owner_vnum);
      mudlog(BRF, LVL_BUILDER, TRUE,
             "TRIGGER HELP: Use 'tlist' to see triggers, '%s' in %s to manage", script_menu,
             editor);
    }
    else
    {
      if (!*script)
        CREATE(*script, struct script_data, 1);
      add_trigger(*script, read_trigger(rnum), -1);
    }

    trg_proto = trg_proto->next;
  }
}

void assign_mob_triggers(struct char_data *mob)
{
  if (!mob)
    return;

  assign_trigger_list(mob->proto_script, &mob->script, "Mobile", mob_index[mob->nr].vnum, "medit",
                      "attach");
  if (SCRIPT(mob) != NULL)
    dg_script_bind_owner(SCRIPT(mob), mob, MOB_TRIGGER);
}

void assign_obj_triggers(struct obj_data *obj)
{
  if (!obj)
    return;

  assign_trigger_list(obj->proto_script, &obj->script, "Object", obj_index[obj->item_number].vnum,
                      "oedit", "scripts");
  if (SCRIPT(obj) != NULL)
    dg_script_bind_owner(SCRIPT(obj), obj, OBJ_TRIGGER);
}

void assign_room_triggers(struct room_data *room)
{
  if (!room)
    return;

  assign_trigger_list(room->proto_script, &room->script, "Room", room->number, "redit", "scripts");
  if (SCRIPT(room) != NULL)
    dg_script_bind_owner(SCRIPT(room), room, WLD_TRIGGER);
}
