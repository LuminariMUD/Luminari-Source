/**
 * @file spec/spec_rol_drow.c
 * Source-profiled drow-equipment decay for the Realms of Luminari conversion.
 */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"

#include "act.h"
#include "comm.h"
#include "db.h"
#include "dgscript/dg_scripts.h"
#include "handler.h"
#include "mud_event.h"
#include "spec/spec_context.h"
#include "spec/spec_dispatch.h"
#include "spec/spec_rol_drow.h"

#define ROL_DROW_SOURCE_PASSES_PER_SEC 4
#define ROL_DROW_SOURCE_JITTER_MIN -4
#define ROL_DROW_SOURCE_JITTER_MAX 4

static const int rol_drow_equipment_vnums[] = {
    2092080, 2092081, 2092082, 2092096, 2093081, 2093082, 2093083, 2093084,
    2093085, 2093087, 2093150, 2093151, 2093152, 2093153, 2093154, 2093155,
};

size_t rol_drow_equipment_profile_count(void)
{
  return sizeof(rol_drow_equipment_vnums) / sizeof(rol_drow_equipment_vnums[0]);
}

bool rol_drow_equipment_profile(int object_vnum)
{
  size_t index;

  for (index = 0; index < rol_drow_equipment_profile_count(); index++)
    if (rol_drow_equipment_vnums[index] == object_vnum)
      return true;
  return false;
}

bool rol_drow_decayable_sector(int sector_type)
{
  switch (sector_type)
  {
  case SECT_UD_WILD:
  case SECT_UD_CITY:
  case SECT_UD_INSIDE:
  case SECT_UD_WATER:
  case SECT_UD_NOSWIM:
  case SECT_UD_NOGROUND:
  case SECT_CAVE:
    return false;
  default:
    return true;
  }
}

int rol_drow_decay_modulus(bool inside_object, int hour, bool sunlight)
{
  int decay = 2;

  if (inside_object)
    decay--;

  /* Preserve the source predicates exactly. Each OR is true for every hour. */
  if (hour > 4 || hour < 22)
    decay++;
  if (hour > 5 || hour < 21)
    decay += decay;
  if (!inside_object && sunlight)
    decay += decay;

  return MAX(1, 12 - decay);
}

long rol_drow_decay_delay_pulses(int jitter_pulses)
{
  long jitter;

  jitter_pulses = MAX(ROL_DROW_SOURCE_JITTER_MIN, MIN(ROL_DROW_SOURCE_JITTER_MAX, jitter_pulses));
  jitter = (long)jitter_pulses * PASSES_PER_SEC / ROL_DROW_SOURCE_PASSES_PER_SEC;
  return (SECS_PER_MUD_HOUR * PASSES_PER_SEC) + jitter;
}

bool rol_drow_reduce_object_value(struct obj_data *obj, int decay_modulus)
{
  int index;
  int reduction;

  if (obj == NULL)
    return true;
  decay_modulus = MAX(1, decay_modulus);

  SET_OBJ_FLAG(obj, ITEM_NOSELL);
  reduction = GET_OBJ_COST(obj) / decay_modulus;
  if (reduction != 0)
    GET_OBJ_COST(obj) -= reduction;
  else if (GET_OBJ_COST(obj) > 0)
    GET_OBJ_COST(obj)--;

  reduction = GET_OBJ_WEIGHT(obj) / decay_modulus;
  if (reduction != 0)
    GET_OBJ_WEIGHT(obj) -= reduction;
  else if (GET_OBJ_WEIGHT(obj) > 1)
    GET_OBJ_WEIGHT(obj)--;

  switch (GET_OBJ_TYPE(obj))
  {
  case ITEM_WEAPON:
    if (GET_OBJ_VAL(obj, 1) > 1)
    {
      GET_OBJ_VAL(obj, 2) *= GET_OBJ_VAL(obj, 1);
      GET_OBJ_VAL(obj, 1) = 1;
    }
    reduction = GET_OBJ_VAL(obj, 2) / decay_modulus;
    if (reduction != 0)
      GET_OBJ_VAL(obj, 2) -= reduction;
    else if (GET_OBJ_VAL(obj, 2) > 0)
      GET_OBJ_VAL(obj, 2)--;
    else
      return true;
    break;
  case ITEM_ARMOR:
    reduction = GET_OBJ_VAL(obj, 0) / decay_modulus;
    if (reduction != 0)
      GET_OBJ_VAL(obj, 0) -= reduction;
    else if (GET_OBJ_VAL(obj, 0) > 0)
      GET_OBJ_VAL(obj, 0)--;
    else
      return true;
    break;
  default:
    break;
  }

  /* The source contract decays only its first two object affects. */
  for (index = 0; index < 2; index++)
  {
    reduction = obj->affected[index].modifier / decay_modulus;
    if (reduction != 0)
      obj->affected[index].modifier -= reduction;
    else if (obj->affected[index].modifier != 0)
      obj->affected[index].modifier--;
  }
  return false;
}

static struct char_data *rol_drow_object_owner(struct obj_data *obj)
{
  if (obj == NULL)
    return NULL;
  if (obj->worn_by != NULL)
    return obj->worn_by;
  if (obj->carried_by != NULL)
    return obj->carried_by;
  return rol_drow_object_owner(obj->in_obj);
}

static void rol_drow_schedule_decay(struct obj_data *obj, bool jitter)
{
  int jitter_pulses = 0;

  if (obj == NULL || obj_has_mud_event(obj, eROL_DROW_DECAY) != NULL)
    return;
  if (jitter)
    jitter_pulses = rand_number(ROL_DROW_SOURCE_JITTER_MIN, ROL_DROW_SOURCE_JITTER_MAX);
  attach_mud_event(new_mud_event(eROL_DROW_DECAY, obj, NULL),
                   rol_drow_decay_delay_pulses(jitter_pulses));
}

static bool rol_drow_decay_once(struct obj_data *obj)
{
  struct char_data *owner;
  room_rnum room;
  bool inside_object;
  bool sunlight;
  int modulus;

  if (obj == NULL || (room = obj_room(obj)) == NOWHERE || !VALID_ROOM_RNUM(room))
    return false;
  if (!rol_drow_decayable_sector(SECT(room)))
    return false;

  owner = rol_drow_object_owner(obj);
  inside_object = obj->in_obj != NULL;
  sunlight = !inside_object && is_room_in_sunlight(room);
  modulus = rol_drow_decay_modulus(inside_object, time_info.hours, sunlight);
  if (rol_drow_reduce_object_value(obj, modulus))
  {
    if (!inside_object && owner != NULL)
    {
      act("$p has completely decayed in the surface air!", TRUE, owner, obj, NULL, TO_CHAR);
      act("$p completely decays in the surface air!", TRUE, owner, obj, NULL, TO_ROOM);
    }
    return true;
  }

  if (!inside_object && owner != NULL)
  {
    act("Your $p crumbles a little in the surface air!", TRUE, owner, obj, NULL, TO_CHAR);
    act("$n's $p crumbles slightly in the surface air.", TRUE, owner, obj, NULL, TO_ROOM);
  }
  return false;
}

EVENTFUNC(event_rol_drow_decay)
{
  struct mud_event_data *event = event_obj;
  struct obj_data *obj;
  room_rnum room;

  if (event == NULL || (obj = event->pStruct) == NULL ||
      !rol_drow_equipment_profile(GET_OBJ_VNUM(obj)) || (room = obj_room(obj)) == NOWHERE ||
      !VALID_ROOM_RNUM(room) || !rol_drow_decayable_sector(SECT(room)))
    return 0;

  if (rol_drow_decay_once(obj))
  {
    /* Detach the running event before extraction so object cleanup cannot cancel it twice. */
    mud_event_detach_owner(event);
    extract_obj(obj);
    return 0;
  }
  return rol_drow_decay_delay_pulses(
      rand_number(ROL_DROW_SOURCE_JITTER_MIN, ROL_DROW_SOURCE_JITTER_MAX));
}

int rol_drow_equipment(struct char_data *ch, void *me, int cmd, const char *argument)
{
  UNUSED(ch);
  UNUSED(me);
  UNUSED(cmd);
  UNUSED(argument);

  return FALSE;
}

int rol_drow_equipment_typed(struct spec_event_context *context)
{
  struct obj_data *obj;
  room_rnum room;

  if (context == NULL || context->owner_type != SPEC_OWNER_OBJECT || context->owner == NULL)
    return FALSE;
  obj = context->owner;
  if (!rol_drow_equipment_profile(GET_OBJ_VNUM(obj)))
    return FALSE;

  if (context->event == SPEC_EVENT_OBJECT_AUTOMATIC)
  {
    if (!obj->rol_drow_decay_initialized)
    {
      obj->rol_drow_decay_initialized = true;
      rol_drow_schedule_decay(obj, false);
    }
    return FALSE;
  }
  if (context->event != SPEC_EVENT_COMMAND || (room = obj_room(obj)) == NOWHERE ||
      !VALID_ROOM_RNUM(room) || !rol_drow_decayable_sector(SECT(room)))
    return FALSE;

  rol_drow_schedule_decay(obj, false);
  return FALSE;
}
