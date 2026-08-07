/**
 * @file spec_assign_table.h
 * Phase 02 owner-typed declarative legacy assignment tables.
 *
 * A compatibility assignment binds a registered procedure to a hard-coded
 * prototype VNUM. Expressing those bindings as data instead of repeated macro
 * calls lets boot validate them: every row must name a registered definition
 * that supports the row's owner type and permits legacy-assignment binding.
 *
 * The row types are deliberately separate rather than one compact `int vnum`.
 * VNUM namespaces overlap - object constant `SCIMITAR` and an unrelated room
 * number are both 3226 - so a shared row type would let a room constant enter a
 * mobile table without any diagnostic. Typed rows make that a compile error.
 *
 * These tables are a conversion target, not a replacement. A hard-coded
 * assignment converts only when its procedure is registered and its VNUM has a
 * traced symbolic constant; see `docs/testing/SPECIAL_PROCEDURE_PHASE_02_VALIDATION.md`
 * for the current unconverted inventory and why it is blocked.
 */

#ifndef LUMINARI_SPEC_ASSIGN_TABLE_H
#define LUMINARI_SPEC_ASSIGN_TABLE_H

#include <stdbool.h>
#include <stddef.h>

#include "spec/spec_registry.h"

struct spec_mob_assignment
{
  mob_vnum vnum;
  /** Canonical name or explicit alias of a registered definition. */
  const char *definition_name;
};

struct spec_obj_assignment
{
  obj_vnum vnum;
  const char *definition_name;
};

struct spec_room_assignment
{
  room_vnum vnum;
  const char *definition_name;
};

/**
 * Resolve one row's definition for one owner type.
 *
 * Returns NULL and writes a bounded diagnostic when the name is missing,
 * unregistered, incompatible with the owner type, or not permitted as a legacy
 * assignment. `error` may be NULL.
 */
const struct spec_definition *spec_assign_table_resolve(const char *definition_name,
                                                        spec_owner_mask owner, char *error,
                                                        size_t error_size);

/**
 * Validate a whole table and write the first failing row's diagnostic.
 *
 * A table row naming an unregistered or incompatible definition is a programmer
 * error, not content, so boot validation reports it before any binding is
 * applied.
 */
bool spec_assign_table_validate_mobiles(const struct spec_mob_assignment *rows, size_t count,
                                        char *error, size_t error_size);
bool spec_assign_table_validate_objects(const struct spec_obj_assignment *rows, size_t count,
                                        char *error, size_t error_size);
bool spec_assign_table_validate_rooms(const struct spec_room_assignment *rows, size_t count,
                                      char *error, size_t error_size);

#endif /* LUMINARI_SPEC_ASSIGN_TABLE_H */
