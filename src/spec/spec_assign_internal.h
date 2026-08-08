/**
 * @file spec_assign_internal.h
 * Internal owner-typed assignment helpers shared by compiled inventories.
 */

#ifndef LUMINARI_SPEC_ASSIGN_INTERNAL_H
#define LUMINARI_SPEC_ASSIGN_INTERNAL_H

#include "spec_registry.h"

void spec_assign_mobile(mob_vnum mob, spec_legacy_handler handler, const char *symbol,
                        const char *source_location);
void spec_assign_object(obj_vnum obj, spec_legacy_handler handler, const char *symbol,
                        const char *source_location);
void spec_assign_room(room_vnum room, spec_legacy_handler handler, const char *symbol,
                      const char *source_location);

#endif /* LUMINARI_SPEC_ASSIGN_INTERNAL_H */
