/**************************************************************************
 *  File: spec/spec_assign.c                          Part of LuminariMUD *
 *  Usage: Shared owner-typed special-procedure assignment machinery.      *
 *                                                                         *
 *  All rights reserved. See license for complete information.             *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"

#include "spec_assign_internal.h"
#include "spec_effective_binding.h"
#include "spec_registry.h"

static void record_legacy_assignment(struct spec_effective_binding **target, spec_owner_mask owner,
                                     unsigned int prototype_vnum, spec_legacy_handler handler,
                                     const char *symbol, const char *source_location)
{
  const struct spec_definition *definition;
  struct spec_effective_contribution_input contribution;
  char error[256];

  definition = spec_registry_find_by_handler(handler);
  contribution.source = SPEC_BINDING_SOURCE_LEGACY_ASSIGNMENT;
  contribution.requested_name = symbol;
  contribution.handler_name = definition != NULL ? definition->canonical_name : symbol;
  contribution.source_location = source_location;
  contribution.handler = handler;
  contribution.wrapper = false;
  contribution.secondary_handler = NULL;
  contribution.secondary_name = NULL;
  if (!spec_effective_binding_contribute(target, owner, prototype_vnum, &contribution, error,
                                         sizeof(error)))
    log("SYSERR: Unable to record legacy special-procedure assignment: %s", error);
}

void spec_assign_mobile(mob_vnum mob, spec_legacy_handler handler, const char *symbol,
                        const char *source_location)
{
  mob_rnum rnum;

  if ((rnum = real_mobile(mob)) != NOBODY)
  {
    mob_index[rnum].func = handler;
    record_legacy_assignment(&mob_index[rnum].effective_binding, SPEC_OWNER_MOBILE,
                             (unsigned int)mob, handler, symbol, source_location);
  }
  else if (!mini_mud)
    log("SYSERR: Attempt to assign spec to non-existant mob #%d", mob);
}

void spec_assign_object(obj_vnum obj, spec_legacy_handler handler, const char *symbol,
                        const char *source_location)
{
  obj_rnum rnum;

  if ((rnum = real_object(obj)) != NOTHING)
  {
    obj_index[rnum].func = handler;
    record_legacy_assignment(&obj_index[rnum].effective_binding, SPEC_OWNER_OBJECT,
                             (unsigned int)obj, handler, symbol, source_location);
  }
  else if (!mini_mud)
    log("SYSERR: Attempt to assign spec to non-existant obj #%d", obj);
}

void spec_assign_room(room_vnum room, spec_legacy_handler handler, const char *symbol,
                      const char *source_location)
{
  room_rnum rnum;

  if ((rnum = real_room(room)) != NOWHERE)
  {
    world[rnum].func = handler;
    record_legacy_assignment(&world[rnum].effective_binding, SPEC_OWNER_ROOM, (unsigned int)room,
                             handler, symbol, source_location);
  }
  else if (!mini_mud)
    log("SYSERR: Attempt to assign spec to non-existant room #%d", room);
}

/* eof */
