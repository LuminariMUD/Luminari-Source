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
#include "periodic_owners.h"
#include "point_update_periodic.h"

#define DG_RANDOM_OWNER_TYPES 3
#define DG_TIME_OWNER_TYPES 3

static struct script_data *dg_random_heads[DG_RANDOM_OWNER_TYPES];
static size_t dg_random_counts[DG_RANDOM_OWNER_TYPES];
static struct script_data *dg_random_iteration_next;
static bool dg_random_iteration_active;
static struct script_data *dg_time_heads[DG_TIME_OWNER_TYPES];
static size_t dg_time_counts[DG_TIME_OWNER_TYPES];
static uint64_t dg_time_visited_counts[DG_TIME_OWNER_TYPES];
static uint64_t dg_time_executed_counts[DG_TIME_OWNER_TYPES];
static struct script_data *dg_time_iteration_next;
static bool dg_time_iteration_active;

static long dg_time_trigger_mask(int owner_type)
{
  switch (owner_type)
  {
  case MOB_TRIGGER:
    return MTRIG_TIME;
  case OBJ_TRIGGER:
    return OTRIG_TIME;
  case WLD_TRIGGER:
    return WTRIG_TIME;
  default:
    return 0L;
  }
}

static void dg_time_registry_remove(struct script_data *script)
{
  int owner_type;

  if (script == NULL || !script->time_registered)
    return;
  owner_type = script->owner_type;
  if (dg_time_iteration_active && dg_time_iteration_next == script)
    dg_time_iteration_next = script->time_next;
  if (script->time_prev != NULL)
    script->time_prev->time_next = script->time_next;
  else if (owner_type >= MOB_TRIGGER && owner_type <= WLD_TRIGGER)
    dg_time_heads[owner_type] = script->time_next;
  if (script->time_next != NULL)
    script->time_next->time_prev = script->time_prev;
  script->time_next = NULL;
  script->time_prev = NULL;
  script->time_registered = false;
  if (owner_type >= MOB_TRIGGER && owner_type <= WLD_TRIGGER && dg_time_counts[owner_type] > 0)
    dg_time_counts[owner_type]--;
}

void dg_time_registry_sync(struct script_data *script)
{
  int owner_type;
  bool eligible;

  if (script == NULL)
    return;
  owner_type = script->owner_type;
  eligible = script->owner != NULL && owner_type >= MOB_TRIGGER && owner_type <= WLD_TRIGGER &&
             IS_SET(SCRIPT_TYPES(script), dg_time_trigger_mask(owner_type));
  if (!eligible)
  {
    dg_time_registry_remove(script);
    return;
  }
  if (script->time_registered)
    return;
  script->time_prev = NULL;
  script->time_next = dg_time_heads[owner_type];
  if (script->time_next != NULL)
    script->time_next->time_prev = script;
  dg_time_heads[owner_type] = script;
  script->time_registered = true;
  dg_time_counts[owner_type]++;
}

static void dg_random_registry_remove(struct script_data *script)
{
  int owner_type;

  periodic_dg_random_forget(script);
  if (script == NULL || !script->random_registered)
    return;
  owner_type = script->owner_type;
  if (dg_random_iteration_active && dg_random_iteration_next == script)
    dg_random_iteration_next = script->random_next;
  if (script->random_prev != NULL)
    script->random_prev->random_next = script->random_next;
  else if (owner_type >= MOB_TRIGGER && owner_type <= WLD_TRIGGER)
    dg_random_heads[owner_type] = script->random_next;
  if (script->random_next != NULL)
    script->random_next->random_prev = script->random_prev;
  script->random_next = NULL;
  script->random_prev = NULL;
  script->random_registered = false;
  if (owner_type >= MOB_TRIGGER && owner_type <= WLD_TRIGGER && dg_random_counts[owner_type] > 0)
    dg_random_counts[owner_type]--;
}

void dg_random_registry_sync(struct script_data *script)
{
  int owner_type;
  bool eligible;

  if (script == NULL)
    return;
  owner_type = script->owner_type;
  eligible = script->owner != NULL && owner_type >= MOB_TRIGGER && owner_type <= WLD_TRIGGER &&
             IS_SET(SCRIPT_TYPES(script), MTRIG_RANDOM);
  if (!eligible)
  {
    dg_random_registry_remove(script);
    return;
  }
  if (script->random_registered)
  {
    periodic_dg_random_sync(script);
    return;
  }
  script->random_prev = NULL;
  script->random_next = dg_random_heads[owner_type];
  if (script->random_next != NULL)
    script->random_next->random_prev = script;
  dg_random_heads[owner_type] = script;
  script->random_registered = true;
  dg_random_counts[owner_type]++;
  periodic_dg_random_sync(script);
}

void dg_script_bind_owner(struct script_data *script, void *owner, int owner_type)
{
  if (script == NULL || owner == NULL || owner_type < MOB_TRIGGER || owner_type > WLD_TRIGGER)
    return;
  dg_random_registry_remove(script);
  dg_time_registry_remove(script);
  script->owner = owner;
  script->owner_type = (byte)owner_type;
  script->owner_vnum = owner_type == WLD_TRIGGER ? ((struct room_data *)owner)->number : NOWHERE;
  dg_random_registry_sync(script);
  dg_time_registry_sync(script);
  if (owner_type == OBJ_TRIGGER)
    point_update_object_sync(owner);
}

void *dg_time_registry_resolve_owner(struct script_data *script)
{
  room_rnum room;

  if (script == NULL)
    return NULL;
  switch (script->owner_type)
  {
  case MOB_TRIGGER:
    if (script->owner != NULL && SCRIPT((struct char_data *)script->owner) == script)
      return script->owner;
    break;
  case OBJ_TRIGGER:
    if (script->owner != NULL && SCRIPT((struct obj_data *)script->owner) == script)
      return script->owner;
    break;
  case WLD_TRIGGER:
    room = real_room(script->owner_vnum);
    if (room != NOWHERE && SCRIPT(&world[room]) == script)
      return &world[room];
    break;
  }
  dg_time_registry_remove(script);
  return NULL;
}

void *dg_time_registry_iteration_next(void)
{
  struct script_data *script;
  void *owner;

  if (!dg_time_iteration_active)
    return NULL;
  while ((script = dg_time_iteration_next) != NULL)
  {
    dg_time_iteration_next = script->time_next;
    owner = dg_time_registry_resolve_owner(script);
    if (owner != NULL)
      return owner;
  }
  return NULL;
}

void *dg_time_registry_iteration_begin(int owner_type)
{
  if (owner_type < MOB_TRIGGER || owner_type > WLD_TRIGGER)
    return NULL;
  if (dg_time_iteration_active)
  {
    log("SYSERR: Nested DG time-owner registry iteration rejected.");
    return NULL;
  }
  dg_time_iteration_active = true;
  dg_time_iteration_next = dg_time_heads[owner_type];
  return dg_time_registry_iteration_next();
}

void dg_time_registry_iteration_end(void)
{
  dg_time_iteration_next = NULL;
  dg_time_iteration_active = false;
}

size_t dg_time_registry_count(int owner_type)
{
  if (owner_type < MOB_TRIGGER || owner_type > WLD_TRIGGER)
    return 0;
  return dg_time_counts[owner_type];
}

uint64_t dg_time_registry_visited(int owner_type)
{
  if (owner_type < MOB_TRIGGER || owner_type > WLD_TRIGGER)
    return 0U;
  return dg_time_visited_counts[owner_type];
}

uint64_t dg_time_registry_executed(int owner_type)
{
  if (owner_type < MOB_TRIGGER || owner_type > WLD_TRIGGER)
    return 0U;
  return dg_time_executed_counts[owner_type];
}

size_t dg_time_registry_validate(int owner_type)
{
  struct char_data *ch;
  struct obj_data *obj;
  struct script_data *script;
  room_rnum room;
  size_t expected = 0U;
  size_t actual = 0U;
  size_t invalid = 0U;

  if (owner_type < MOB_TRIGGER || owner_type > WLD_TRIGGER)
    return 0U;
  if (owner_type == MOB_TRIGGER)
    for (ch = character_list; ch != NULL; ch = ch->next)
      if (SCRIPT(ch) != NULL && IS_SET(SCRIPT_TYPES(SCRIPT(ch)), MTRIG_TIME))
        expected++;
  if (owner_type == OBJ_TRIGGER)
    for (obj = object_list; obj != NULL; obj = obj->next)
      if (SCRIPT(obj) != NULL && IS_SET(SCRIPT_TYPES(SCRIPT(obj)), OTRIG_TIME))
        expected++;
  if (owner_type == WLD_TRIGGER && world != NULL)
    for (room = 0; room <= top_of_world; room++)
      if (SCRIPT(&world[room]) != NULL && IS_SET(SCRIPT_TYPES(SCRIPT(&world[room])), WTRIG_TIME))
        expected++;
  for (script = dg_time_heads[owner_type]; script != NULL; script = script->time_next)
  {
    actual++;
    if (!script->time_registered || script->owner_type != owner_type || script->owner == NULL ||
        !IS_SET(SCRIPT_TYPES(script), dg_time_trigger_mask(owner_type)))
    {
      invalid++;
      continue;
    }
    if (owner_type == MOB_TRIGGER && SCRIPT((struct char_data *)script->owner) != script)
      invalid++;
    else if (owner_type == OBJ_TRIGGER && SCRIPT((struct obj_data *)script->owner) != script)
      invalid++;
    else if (owner_type == WLD_TRIGGER)
    {
      room = real_room(script->owner_vnum);
      if (room == NOWHERE || SCRIPT(&world[room]) != script)
        invalid++;
    }
  }
  if (expected == actual && actual == dg_time_counts[owner_type] && invalid == 0U)
    return 0U;
  return invalid + (expected > actual ? expected - actual : actual - expected) +
         (actual > dg_time_counts[owner_type] ? actual - dg_time_counts[owner_type]
                                              : dg_time_counts[owner_type] - actual);
}

void dg_time_registry_note_dispatch(int owner_type, bool executed)
{
  if (owner_type < MOB_TRIGGER || owner_type > WLD_TRIGGER)
    return;
  dg_time_visited_counts[owner_type]++;
  if (executed)
    dg_time_executed_counts[owner_type]++;
}

void *dg_random_registry_resolve_owner(struct script_data *script)
{
  room_rnum room;

  if (script == NULL)
    return NULL;
  switch (script->owner_type)
  {
  case MOB_TRIGGER:
    if (script->owner != NULL && SCRIPT((struct char_data *)script->owner) == script)
      return script->owner;
    break;
  case OBJ_TRIGGER:
    if (script->owner != NULL && SCRIPT((struct obj_data *)script->owner) == script)
      return script->owner;
    break;
  case WLD_TRIGGER:
    room = real_room(script->owner_vnum);
    if (room != NOWHERE && SCRIPT(&world[room]) == script)
      return &world[room];
    break;
  }
  dg_random_registry_remove(script);
  return NULL;
}

void *dg_random_registry_iteration_next(void)
{
  struct script_data *script;
  void *owner;

  if (!dg_random_iteration_active)
    return NULL;
  while ((script = dg_random_iteration_next) != NULL)
  {
    dg_random_iteration_next = script->random_next;
    owner = dg_random_registry_resolve_owner(script);
    if (owner != NULL)
      return owner;
  }
  return NULL;
}

void *dg_random_registry_iteration_begin(int owner_type)
{
  if (owner_type < MOB_TRIGGER || owner_type > WLD_TRIGGER)
    return NULL;
  if (dg_random_iteration_active)
  {
    log("SYSERR: Nested DG random-owner registry iteration rejected.");
    return NULL;
  }
  dg_random_iteration_active = true;
  dg_random_iteration_next = dg_random_heads[owner_type];
  return dg_random_registry_iteration_next();
}

void dg_random_registry_iteration_end(void)
{
  dg_random_iteration_next = NULL;
  dg_random_iteration_active = false;
}

size_t dg_random_registry_count(int owner_type)
{
  if (owner_type < MOB_TRIGGER || owner_type > WLD_TRIGGER)
    return 0;
  return dg_random_counts[owner_type];
}

size_t dg_random_registry_validate(int owner_type)
{
  struct char_data *ch;
  struct obj_data *obj;
  struct script_data *script;
  room_rnum room;
  size_t expected;
  size_t actual;

  if (owner_type < MOB_TRIGGER || owner_type > WLD_TRIGGER)
    return 0;
  expected = 0;
  if (owner_type == MOB_TRIGGER)
    for (ch = character_list; ch != NULL; ch = ch->next)
      if (SCRIPT(ch) != NULL && IS_SET(SCRIPT_TYPES(SCRIPT(ch)), MTRIG_RANDOM))
        expected++;
  if (owner_type == OBJ_TRIGGER)
    for (obj = object_list; obj != NULL; obj = obj->next)
      if (SCRIPT(obj) != NULL && IS_SET(SCRIPT_TYPES(SCRIPT(obj)), OTRIG_RANDOM))
        expected++;
  if (owner_type == WLD_TRIGGER && world != NULL)
    for (room = 0; room <= top_of_world; room++)
      if (SCRIPT(&world[room]) != NULL && IS_SET(SCRIPT_TYPES(SCRIPT(&world[room])), WTRIG_RANDOM))
        expected++;
  actual = 0;
  for (script = dg_random_heads[owner_type]; script != NULL; script = script->random_next)
    actual++;
  if (expected == actual && actual == dg_random_counts[owner_type])
    return 0;
  return expected > actual ? expected - actual : actual - expected;
}

#ifdef LUMINARI_CUTEST
void dg_random_registry_reset_for_test(void)
{
  struct script_data *script;
  struct script_data *next;
  int owner_type;

  for (owner_type = MOB_TRIGGER; owner_type <= WLD_TRIGGER; owner_type++)
    for (script = dg_random_heads[owner_type]; script != NULL; script = next)
    {
      next = script->random_next;
      periodic_dg_random_forget(script);
      script->random_next = NULL;
      script->random_prev = NULL;
      script->random_registered = false;
    }
  memset(dg_random_heads, 0, sizeof(dg_random_heads));
  memset(dg_random_counts, 0, sizeof(dg_random_counts));
  dg_random_iteration_next = NULL;
  dg_random_iteration_active = false;
}

void dg_time_registry_reset_for_test(void)
{
  struct script_data *script;
  struct script_data *next;
  int owner_type;

  for (owner_type = MOB_TRIGGER; owner_type <= WLD_TRIGGER; owner_type++)
    for (script = dg_time_heads[owner_type]; script != NULL; script = next)
    {
      next = script->time_next;
      script->time_next = NULL;
      script->time_prev = NULL;
      script->time_registered = false;
    }
  memset(dg_time_heads, 0, sizeof(dg_time_heads));
  memset(dg_time_counts, 0, sizeof(dg_time_counts));
  memset(dg_time_visited_counts, 0, sizeof(dg_time_visited_counts));
  memset(dg_time_executed_counts, 0, sizeof(dg_time_executed_counts));
  dg_time_iteration_next = NULL;
  dg_time_iteration_active = false;
}
#endif

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
  struct wait_event_data *wait_event_obj;

  if (trig == NULL)
    return;
  wait_event_obj = GET_TRIG_WAIT_DATA(trig);
  if (wait_event_obj != NULL && dg_trigger_wait_is_dispatching(trig))
  {
    wait_event_obj->destroy_trigger_after_cleanup = true;
    dg_trigger_wait_cancel(trig);
    return;
  }

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
  dg_trigger_wait_cancel(trig);

  free(trig);
}

/* remove a single trigger from a mob/obj/room */
void extract_trigger(struct trig_data *trig)
{
  struct trig_data *temp;

  dg_trigger_wait_cancel(trig);

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
  dg_random_registry_remove(sc);
  dg_time_registry_remove(sc);
  if (sc->owner_type == OBJ_TRIGGER)
    point_update_object_sync(sc->owner);

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
    if (!dg_trigger_wait_is_live(trig) || GET_TRIG_WAIT_DATA(trig) == NULL)
      continue;

    GET_TRIG_WAIT_DATA(trig)->go = to;
  }
}
