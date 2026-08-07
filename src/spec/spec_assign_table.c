/**
 * @file spec_assign_table.c
 * Phase 02 owner-typed declarative legacy assignment tables.
 */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "spec/spec_assign_table.h"

#include <stdio.h>

static void spec_assign_table_set_error(char *error, size_t error_size, const char *format, ...)
{
  va_list arguments;

  if (error == NULL || error_size == 0)
    return;

  va_start(arguments, format);
  /* NOLINTNEXTLINE(clang-analyzer-valist.Uninitialized) -- va_start initializes arguments. */
  vsnprintf(error, error_size, format, arguments);
  va_end(arguments);
}

const struct spec_definition *spec_assign_table_resolve(const char *definition_name,
                                                        spec_owner_mask owner, char *error,
                                                        size_t error_size)
{
  const struct spec_definition *definition;
  const char *owner_label;

  if (error != NULL && error_size > 0)
    error[0] = '\0';

  owner_label = spec_owner_name(owner);
  if (owner_label == NULL)
  {
    spec_assign_table_set_error(error, error_size, "invalid owner mask %u", (unsigned int)owner);
    return NULL;
  }

  if (definition_name == NULL || *definition_name == '\0')
  {
    spec_assign_table_set_error(error, error_size, "%s row has an empty definition name",
                                owner_label);
    return NULL;
  }

  definition = spec_registry_find_by_name(definition_name);
  if (definition == NULL)
  {
    spec_assign_table_set_error(error, error_size, "%s row '%s' names no registered definition",
                                owner_label, definition_name);
    return NULL;
  }

  if (!spec_definition_supports_owner(definition, owner))
  {
    spec_assign_table_set_error(error, error_size,
                                "%s row '%s' resolves to a definition that "
                                "does not support that owner type",
                                owner_label, definition_name);
    return NULL;
  }

  if (!spec_definition_allows_binding(definition, SPEC_BINDING_SOURCE_LEGACY_ASSIGNMENT))
  {
    spec_assign_table_set_error(error, error_size,
                                "%s row '%s' resolves to a definition that does not permit legacy "
                                "assignment binding",
                                owner_label, definition_name);
    return NULL;
  }

  return definition;
}

/**
 * Shared row-loop diagnostic wrapper.
 *
 * Each typed validator reports the failing index and VNUM so a programmer can
 * find the row without counting entries.
 */
static bool spec_assign_table_validate_row(const char *definition_name, spec_owner_mask owner,
                                           size_t index, unsigned int vnum, char *error,
                                           size_t error_size)
{
  char reason[256];

  if (spec_assign_table_resolve(definition_name, owner, reason, sizeof(reason)) != NULL)
    return TRUE;

  spec_assign_table_set_error(error, error_size, "row %zu (vnum %u): %s", index, vnum, reason);
  return FALSE;
}

bool spec_assign_table_validate_mobiles(const struct spec_mob_assignment *rows, size_t count,
                                        char *error, size_t error_size)
{
  size_t index;

  if (error != NULL && error_size > 0)
    error[0] = '\0';

  if (rows == NULL && count > 0)
  {
    spec_assign_table_set_error(error, error_size, "mobile table is null with %zu rows", count);
    return FALSE;
  }

  for (index = 0; index < count; index++)
    if (!spec_assign_table_validate_row(rows[index].definition_name, SPEC_OWNER_MOBILE, index,
                                        (unsigned int)rows[index].vnum, error, error_size))
      return FALSE;

  return TRUE;
}

bool spec_assign_table_validate_objects(const struct spec_obj_assignment *rows, size_t count,
                                        char *error, size_t error_size)
{
  size_t index;

  if (error != NULL && error_size > 0)
    error[0] = '\0';

  if (rows == NULL && count > 0)
  {
    spec_assign_table_set_error(error, error_size, "object table is null with %zu rows", count);
    return FALSE;
  }

  for (index = 0; index < count; index++)
    if (!spec_assign_table_validate_row(rows[index].definition_name, SPEC_OWNER_OBJECT, index,
                                        (unsigned int)rows[index].vnum, error, error_size))
      return FALSE;

  return TRUE;
}

bool spec_assign_table_validate_rooms(const struct spec_room_assignment *rows, size_t count,
                                      char *error, size_t error_size)
{
  size_t index;

  if (error != NULL && error_size > 0)
    error[0] = '\0';

  if (rows == NULL && count > 0)
  {
    spec_assign_table_set_error(error, error_size, "room table is null with %zu rows", count);
    return FALSE;
  }

  for (index = 0; index < count; index++)
    if (!spec_assign_table_validate_row(rows[index].definition_name, SPEC_OWNER_ROOM, index,
                                        (unsigned int)rows[index].vnum, error, error_size))
      return FALSE;

  return TRUE;
}
