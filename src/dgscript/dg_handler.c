/**************************************************************************
*  File: dg_handler.c                                 Part of LuminariMUD *
*  Usage: Contains functions to handle memory for scripts.                *
*                                                                         *
*  All rights reserved.  See license for complete information.            *
*                                                                         *
*  Death's Gate MUD is based on CircleMUD, Copyright (C) 1993, 94.        *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author: Mark A. Heilpern/egreen/Welcor $                              *
*  $Date: 2004/10/11 12:07:00$                                            *
*  $Revision: 1.0.14 $                                                    *
***************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "dg_scripts.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "magic/spells.h"
#include "dg_event.h"
#include "constants.h"

/* frees memory associated with var */
void free_var_el(struct trig_var_data *var)
{
  if (var->name)
    free(var->name);
  if (var->value)
    free(var->value);
  free(var);
}

/* release memory allocated for a variable list */
void free_varlist(struct trig_var_data *vd)
{
  struct trig_var_data *i, *j;

  for (i = vd; i;)
  {
    j = i;
    i = i->next;
    free_var_el(j);
  }
}

/* Free context-specific global variables from a script */
void free_context_vars(struct script_data *sc, long context)
{
  struct trig_var_data *vd, *vd_prev = NULL, *vd_next;

  if (!sc || !sc->global_vars || context == 0)
    return;

  for (vd = sc->global_vars; vd; vd = vd_next)
  {
    vd_next = vd->next;

    /* Remove variables that match this context */
    if (vd->context == context)
    {
      if (vd_prev)
        vd_prev->next = vd->next;
      else
        sc->global_vars = vd->next;

      free_var_el(vd);
    }
    else
    {
      vd_prev = vd;
    }
  }
}

/* Remove var name from var_list. Returns 1 if found, else 0. */
int remove_var(struct trig_var_data **var_list, char *name)
{
  struct trig_var_data *i, *j;

  for (j = NULL, i = *var_list; i && str_cmp(name, i->name); j = i, i = i->next)
    ;

  if (i)
  {
    if (j)
    {
      j->next = i->next;
      free_var_el(i);
    }
    else
    {
      *var_list = i->next;
      free_var_el(i);
    }

    return 1;
  }

  return 0;
}

/* Return memory used by a trigger. The command list is free'd when changed and
 * when shutting down. Note: The cmdlist is shared between all instances of a
 * trigger and is only freed when the prototype is destroyed in destroy_db(). */
void free_trigger(struct trig_data *trig)
{
  free(trig->name);
  trig->name = NULL;

  if (trig->arglist)
  {
    free(trig->arglist);
    trig->arglist = NULL;
  }
  if (trig->var_list)
  {
    free_varlist(trig->var_list);
    trig->var_list = NULL;
  }
  if (GET_TRIG_WAIT(trig))
    event_cancel(GET_TRIG_WAIT(trig));

  free(trig);
}

/* remove a single trigger from a mob/obj/room */
void extract_trigger(struct trig_data *trig)
{
  struct trig_data *temp;

  if (GET_TRIG_WAIT(trig))
  {
    event_cancel(GET_TRIG_WAIT(trig));
    GET_TRIG_WAIT(trig) = NULL;
  }

  trig_index[trig->nr]->number--;

  /* walk the trigger list and remove this one */
  REMOVE_FROM_LIST(trig, trigger_list, next_in_world);

  free_trigger(trig);
}

/* Remove and free all triggers referenced by a script field. */
void extract_script(struct script_data **script)
{
  struct script_data *sc;
  struct trig_data *trig = NULL, *next_trig = NULL;

  if (!script || !*script)
    return;

  sc = *script;
  *script = NULL;

  /* zusuk disabled this debug 10/15/2017 */
#if 0 /* debugging */
  {
    struct char_data *i = character_list;
    struct obj_data *j = object_list;
    room_rnum k = 0;
    if (sc) {
      for ( ; i ; i = i->next)
        assert(sc != SCRIPT(i));

      for ( ; j ; j = j->next)
        assert(sc != SCRIPT(j));

      for (k = 0; k < top_of_world; k++)
        assert(sc != SCRIPT(&world[k]));
    }
  }
#endif

  for (trig = TRIGGERS(sc); trig; trig = next_trig)
  {
    next_trig = trig->next;
    extract_trigger(trig);
  }
  TRIGGERS(sc) = NULL;

  /* Thanks to James Long for tracking down this memory leak */
  free_varlist(sc->global_vars);

  free(sc);
}

/* erase the script memory of a mob */
void extract_script_mem(struct script_memory *sc)
{
  struct script_memory *next;
  while (sc)
  {
    next = sc->next;
    if (sc->cmd)
      free(sc->cmd);
    free(sc);
    sc = next;
  }
}

void free_proto_script(struct trig_proto_list **proto_script)
{
  struct trig_proto_list *proto, *fproto = NULL;

  if (!proto_script)
    return;

  proto = *proto_script;
  *proto_script = NULL;

/* zusuk disabled this debug 10/15/2017 */
#if 0 /* debugging */
  {
    struct char_data *i = character_list;
    struct obj_data *j = object_list;
    room_rnum k;
    if (proto) {
      for ( ; i ; i = i->next)
        assert(proto != i->proto_script);

      for ( ; j ; j = j->next)
        assert(proto != j->proto_script);

      for (k = 0; k < top_of_world; k++)
        assert(proto != world[k].proto_script);
    }
  }
#endif

  while (proto)
  {
    fproto = proto;
    proto = proto->next;
    free(fproto);
  }
}

void copy_proto_script(const struct trig_proto_list *source, struct trig_proto_list **destination)
{
  const struct trig_proto_list *tp_src = source;
  struct trig_proto_list *tp_dst = NULL;

  if (tp_src && destination)
  {
    CREATE(tp_dst, struct trig_proto_list, 1);
    *destination = tp_dst;

    while (tp_src)
    {
      tp_dst->vnum = tp_src->vnum;
      tp_src = tp_src->next;
      if (tp_src)
        CREATE(tp_dst->next, struct trig_proto_list, 1);
      tp_dst = tp_dst->next;
    }
  }
}

void delete_variables(const char *charname)
{
  char filename[MAX_FILEPATH];

  if (!get_filename(filename, sizeof(filename), SCRIPT_VARS_FILE, charname))
    return;

  if (remove(filename) < 0 && errno != ENOENT)
    log("SYSERR: deleting variable file %s: %s", filename, strerror(errno));
}

void update_wait_events(struct room_data *to, struct room_data *from)
{
  struct trig_data *trig;

  if (!SCRIPT(from))
    return;

  for (trig = TRIGGERS(SCRIPT(from)); trig; trig = trig->next)
  {
    if (!GET_TRIG_WAIT(trig))
      continue;

    ((struct wait_event_data *)GET_TRIG_WAIT(trig)->event_obj)->go = to;
  }
}
