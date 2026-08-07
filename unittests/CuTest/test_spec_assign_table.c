/**
 * @file test_spec_assign_table.c
 * Phase 02 declarative legacy assignment table validation.
 *
 * These tests cover the row contract: a row must name a registered definition
 * that supports the row's owner type and permits legacy-assignment binding.
 * They also pin the production rows that were converted, so a registry change
 * that would silently unbind them fails here instead of at boot.
 */

#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"

#include "../../src/spec/spec_assign_table.h"
#include "../../src/spec/spec_binding.h"
#include "../../src/spec/spec_registry.h"

#include <string.h>

void Test_spec_assign_table_resolves_converted_production_rows(CuTest *tc)
{
  const struct spec_definition *crafting_kit;
  const struct spec_definition *vampire_cloak;
  char error[256];

  /* The two hard-coded assignments converted in Phase 02. Both are object rows
   * whose VNUM has a traced symbolic constant. */
  crafting_kit = spec_assign_table_resolve("Crafting Kit", SPEC_OWNER_OBJECT, error, sizeof(error));
  CuAssertPtrNotNull(tc, (void *)crafting_kit);
  CuAssertStrEquals(tc, "Crafting Kit", crafting_kit->canonical_name);
  CuAssertPtrNotNull(tc, (void *)spec_definition_callback(crafting_kit));
  CuAssertStrEquals(tc, "", error);

  vampire_cloak =
      spec_assign_table_resolve("Vampire Cloak", SPEC_OWNER_OBJECT, error, sizeof(error));
  CuAssertPtrNotNull(tc, (void *)vampire_cloak);
  CuAssertStrEquals(tc, "Vampire Cloak", vampire_cloak->canonical_name);
  CuAssertPtrNotNull(tc, (void *)spec_definition_callback(vampire_cloak));
  CuAssertPtrNotNull(tc, (void *)vampire_cloak->typed_handler);
  CuAssertStrEquals(tc, "", error);
}

void Test_spec_assign_table_resolve_accepts_aliases(CuTest *tc)
{
  const struct spec_definition *by_canonical;
  const struct spec_definition *by_alias;

  by_canonical = spec_assign_table_resolve("Guild", SPEC_OWNER_MOBILE, NULL, 0);
  by_alias = spec_assign_table_resolve("Guildmaster", SPEC_OWNER_MOBILE, NULL, 0);

  CuAssertPtrNotNull(tc, (void *)by_canonical);
  CuAssertPtrEquals(tc, (void *)by_canonical, (void *)by_alias);
  /* An alias never becomes canonical through the table. */
  CuAssertStrEquals(tc, "Guild", by_alias->canonical_name);
}

void Test_spec_assign_table_rejects_unknown_and_empty_names(CuTest *tc)
{
  char error[256];

  CuAssertPtrEquals(tc, NULL,
                    (void *)spec_assign_table_resolve("No Such Procedure", SPEC_OWNER_MOBILE, error,
                                                      sizeof(error)));
  CuAssertTrue(tc, strstr(error, "no registered definition") != NULL);
  CuAssertTrue(tc, strstr(error, "No Such Procedure") != NULL);

  CuAssertPtrEquals(
      tc, NULL, (void *)spec_assign_table_resolve(NULL, SPEC_OWNER_OBJECT, error, sizeof(error)));
  CuAssertTrue(tc, strstr(error, "empty definition name") != NULL);

  CuAssertPtrEquals(tc, NULL,
                    (void *)spec_assign_table_resolve("", SPEC_OWNER_ROOM, error, sizeof(error)));
  CuAssertTrue(tc, strstr(error, "empty definition name") != NULL);
}

void Test_spec_assign_table_rejects_owner_mismatch(CuTest *tc)
{
  char error[256];

  /* "Crafting Kit" is object-only; a mobile or room row naming it is a
   * programmer error, not a content error. */
  CuAssertPtrEquals(
      tc, NULL,
      (void *)spec_assign_table_resolve("Crafting Kit", SPEC_OWNER_MOBILE, error, sizeof(error)));
  CuAssertTrue(tc, strstr(error, "does not support that owner type") != NULL);

  CuAssertPtrEquals(
      tc, NULL,
      (void *)spec_assign_table_resolve("Crafting Kit", SPEC_OWNER_ROOM, error, sizeof(error)));
  CuAssertTrue(tc, strstr(error, "does not support that owner type") != NULL);

  /* "Wizard Library" is room-only. */
  CuAssertPtrEquals(
      tc, NULL,
      (void *)spec_assign_table_resolve("Wizard Library", SPEC_OWNER_OBJECT, error, sizeof(error)));
  CuAssertTrue(tc, strstr(error, "does not support that owner type") != NULL);
}

void Test_spec_assign_table_rejects_invalid_owner_mask(CuTest *tc)
{
  char error[256];

  /* A combined mask names no single owner type and must not resolve. */
  CuAssertPtrEquals(tc, NULL,
                    (void *)spec_assign_table_resolve("Crafting Kit",
                                                      SPEC_OWNER_OBJECT | SPEC_OWNER_MOBILE, error,
                                                      sizeof(error)));
  CuAssertTrue(tc, strstr(error, "invalid owner mask") != NULL);

  CuAssertPtrEquals(
      tc, NULL,
      (void *)spec_assign_table_resolve("Crafting Kit", SPEC_OWNER_NONE, error, sizeof(error)));
  CuAssertTrue(tc, strstr(error, "invalid owner mask") != NULL);
}

void Test_spec_assign_table_rejects_definitions_that_forbid_legacy_assignment(CuTest *tc)
{
  const struct spec_definition *definition;
  char error[256];

  /* "Temple Healer" is world-data binding only. A hard-coded row must not be
   * able to install it behind the builder's back. */
  definition = spec_registry_find_by_name("Temple Healer");
  CuAssertPtrNotNull(tc, (void *)definition);
  CuAssertTrue(tc,
               !spec_definition_allows_binding(definition, SPEC_BINDING_SOURCE_LEGACY_ASSIGNMENT));

  CuAssertPtrEquals(
      tc, NULL,
      (void *)spec_assign_table_resolve("Temple Healer", SPEC_OWNER_MOBILE, error, sizeof(error)));
  CuAssertTrue(tc, strstr(error, "does not permit legacy assignment") != NULL);
}

void Test_spec_assign_table_validators_accept_valid_tables(CuTest *tc)
{
  static const struct spec_mob_assignment mobiles[] = {
      {3095, "Cryogenicist"},
      {110, "Postmaster"},
  };
  static const struct spec_obj_assignment objects[] = {
      {3118, "Crafting Kit"},
      {1234, "Vampire Cloak"},
  };
  static const struct spec_room_assignment rooms[] = {
      {3226, "Wizard Library"},
  };
  char error[256];

  CuAssertTrue(tc, spec_assign_table_validate_mobiles(mobiles, sizeof(mobiles) / sizeof(mobiles[0]),
                                                      error, sizeof(error)));
  CuAssertStrEquals(tc, "", error);
  CuAssertTrue(tc, spec_assign_table_validate_objects(objects, sizeof(objects) / sizeof(objects[0]),
                                                      error, sizeof(error)));
  CuAssertStrEquals(tc, "", error);
  CuAssertTrue(tc, spec_assign_table_validate_rooms(rooms, sizeof(rooms) / sizeof(rooms[0]), error,
                                                    sizeof(error)));
  CuAssertStrEquals(tc, "", error);

  /* An empty table is valid, and a null table with no rows is not an error. */
  CuAssertTrue(tc, spec_assign_table_validate_mobiles(NULL, 0, error, sizeof(error)));
  CuAssertTrue(tc, spec_assign_table_validate_objects(NULL, 0, error, sizeof(error)));
  CuAssertTrue(tc, spec_assign_table_validate_rooms(NULL, 0, error, sizeof(error)));
}

void Test_spec_assign_table_validators_report_the_failing_row(CuTest *tc)
{
  static const struct spec_obj_assignment objects[] = {
      {3118, "Crafting Kit"},
      {4242, "Wizard Library"}, /* room-only definition in an object table */
      {5150, "Vampire Cloak"},
  };
  static const struct spec_mob_assignment mobiles[] = {
      {7777, "Not A Procedure"},
  };
  char error[256];

  CuAssertTrue(tc, !spec_assign_table_validate_objects(
                       objects, sizeof(objects) / sizeof(objects[0]), error, sizeof(error)));
  /* The diagnostic must locate the row without the reader counting entries. */
  CuAssertTrue(tc, strstr(error, "row 1") != NULL);
  CuAssertTrue(tc, strstr(error, "4242") != NULL);
  CuAssertTrue(tc, strstr(error, "does not support that owner type") != NULL);

  CuAssertTrue(tc, !spec_assign_table_validate_mobiles(
                       mobiles, sizeof(mobiles) / sizeof(mobiles[0]), error, sizeof(error)));
  CuAssertTrue(tc, strstr(error, "row 0") != NULL);
  CuAssertTrue(tc, strstr(error, "7777") != NULL);
  CuAssertTrue(tc, strstr(error, "no registered definition") != NULL);
}

void Test_spec_assign_table_validators_reject_null_tables_with_rows(CuTest *tc)
{
  char error[256];

  CuAssertTrue(tc, !spec_assign_table_validate_mobiles(NULL, 3, error, sizeof(error)));
  CuAssertTrue(tc, strstr(error, "mobile table is null") != NULL);
  CuAssertTrue(tc, !spec_assign_table_validate_objects(NULL, 1, error, sizeof(error)));
  CuAssertTrue(tc, strstr(error, "object table is null") != NULL);
  CuAssertTrue(tc, !spec_assign_table_validate_rooms(NULL, 2, error, sizeof(error)));
  CuAssertTrue(tc, strstr(error, "room table is null") != NULL);
}

void Test_spec_assign_table_tolerates_absent_error_buffer(CuTest *tc)
{
  static const struct spec_obj_assignment objects[] = {
      {4242, "Wizard Library"},
  };

  /* Callers that only need the verdict must not be forced to supply a buffer. */
  CuAssertPtrEquals(tc, NULL,
                    (void *)spec_assign_table_resolve("Nope", SPEC_OWNER_MOBILE, NULL, 0));
  CuAssertPtrNotNull(tc,
                     (void *)spec_assign_table_resolve("Crafting Kit", SPEC_OWNER_OBJECT, NULL, 0));
  CuAssertTrue(tc, !spec_assign_table_validate_objects(
                       objects, sizeof(objects) / sizeof(objects[0]), NULL, 0));
}

void Test_spec_binding_source_name_reports_stable_labels(CuTest *tc)
{
  CuAssertStrEquals(tc, "world", spec_binding_source_name(SPEC_BINDING_SOURCE_WORLD));
  CuAssertStrEquals(tc, "legacy assignment",
                    spec_binding_source_name(SPEC_BINDING_SOURCE_LEGACY_ASSIGNMENT));
  CuAssertStrEquals(tc, "parser hook", spec_binding_source_name(SPEC_BINDING_SOURCE_PARSER_HOOK));
  CuAssertStrEquals(tc, "shop", spec_binding_source_name(SPEC_BINDING_SOURCE_SHOP));
  CuAssertStrEquals(tc, "quest", spec_binding_source_name(SPEC_BINDING_SOURCE_QUEST));
  CuAssertPtrEquals(tc, NULL, (void *)spec_binding_source_name(SPEC_BINDING_SOURCE_NONE));
  CuAssertPtrEquals(
      tc, NULL,
      (void *)spec_binding_source_name(SPEC_BINDING_SOURCE_WORLD | SPEC_BINDING_SOURCE_SHOP));
}
