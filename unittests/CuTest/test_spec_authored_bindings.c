#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"

#include "../../src/character/guild_services.h"
#include "../../src/comms/mail.h"
#include "../../src/spec/spec_binding.h"
#include "../../src/vessels/vessels_legacy.h"
#include "test_spec_fixtures.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define SPEC_BINDING_TEST_ERROR_SIZE 512

static const spec_owner_mask spec_binding_test_owners[SPEC_TEST_OWNER_COUNT] = {
    SPEC_OWNER_MOBILE,
    SPEC_OWNER_OBJECT,
    SPEC_OWNER_ROOM,
};

static const unsigned int spec_binding_test_vnums[SPEC_TEST_OWNER_COUNT] = {
    1201U,
    1402U,
    1403U,
};

static void spec_binding_test_set_error(char *error, size_t error_size, const char *format, ...)
{
  va_list arguments;

  if (error == NULL || error_size == 0)
    return;

  va_start(arguments, format);
  /* NOLINTNEXTLINE(clang-analyzer-valist.Uninitialized) -- va_start initializes arguments. */
  vsnprintf(error, error_size, format, arguments);
  va_end(arguments);
}

static bool spec_binding_test_finish_fixture(struct spec_test_fixture *fixture, bool success,
                                             char *error, size_t error_size)
{
  char cleanup_error[SPEC_BINDING_TEST_ERROR_SIZE];

  cleanup_error[0] = '\0';
  if (!spec_test_fixture_destroy(fixture, cleanup_error, sizeof(cleanup_error)))
  {
    if (success || error == NULL || error[0] == '\0')
      spec_binding_test_set_error(error, error_size, "%s", cleanup_error);
    return false;
  }

  return success;
}

static bool spec_binding_test_record_matches(const struct spec_binding *binding,
                                             enum spec_test_owner owner, const char *requested_name,
                                             const char *canonical_name,
                                             enum spec_binding_resolution resolution,
                                             const char *location_fragment, char *error,
                                             size_t error_size)
{
  if (binding == NULL)
  {
    spec_binding_test_set_error(error, error_size, "owner %d has no authored binding", owner);
    return false;
  }
  if (binding->owner != spec_binding_test_owners[owner] ||
      binding->prototype_vnum != spec_binding_test_vnums[owner] ||
      binding->source != SPEC_BINDING_SOURCE_WORLD || binding->resolution != resolution ||
      binding->requested_name == NULL || strcmp(binding->requested_name, requested_name) != 0 ||
      binding->source_location == NULL ||
      strstr(binding->source_location, location_fragment) == NULL)
  {
    spec_binding_test_set_error(error, error_size,
                                "owner %d authored binding metadata did not match", owner);
    return false;
  }
  if (canonical_name == NULL)
  {
    if (binding->definition != NULL)
    {
      spec_binding_test_set_error(error, error_size,
                                  "owner %d unexpectedly resolved an unknown name", owner);
      return false;
    }
  }
  else if (binding->definition == NULL ||
           strcmp(binding->definition->canonical_name, canonical_name) != 0)
  {
    spec_binding_test_set_error(error, error_size,
                                "owner %d resolved the wrong canonical definition", owner);
    return false;
  }

  return true;
}

static bool spec_binding_test_diagnostic_matches(const struct spec_binding *binding,
                                                 const char *resolution_fragment, char *error,
                                                 size_t error_size)
{
  char diagnostic[SPEC_BINDING_TEST_ERROR_SIZE];
  const char *owner_name;

  owner_name = binding != NULL ? spec_owner_name(binding->owner) : NULL;
  if (!spec_binding_format_diagnostic(binding, diagnostic, sizeof(diagnostic)) ||
      owner_name == NULL || strstr(diagnostic, "world binding at") == NULL ||
      strstr(diagnostic, owner_name) == NULL ||
      strstr(diagnostic, binding->requested_name) == NULL ||
      strstr(diagnostic, resolution_fragment) == NULL)
  {
    spec_binding_test_set_error(error, error_size,
                                "authored binding diagnostic omitted required context");
    return false;
  }

  return true;
}

static bool spec_binding_canonical_loader_scenario(const char *sandbox, char *error,
                                                   size_t error_size)
{
  static const char *const requested_names[SPEC_TEST_OWNER_COUNT] = {
      "Postmaster",
      "Greyhawk Ship",
      "Greyhawk Ship Commands",
  };
  static const char *const locations[SPEC_TEST_OWNER_COUNT] = {
      "mobile SpecProc field",
      "object Z field",
      "special-procedure test fixture room Z field",
  };
  struct spec_test_fixture *fixture;
  int owner;

  fixture = spec_test_fixture_create_at(sandbox, error, error_size);
  if (fixture == NULL)
    return false;
  if (!spec_test_fixture_load_named_bindings(fixture, error, error_size))
    return spec_binding_test_finish_fixture(fixture, false, error, error_size);

  for (owner = SPEC_TEST_OWNER_MOBILE; owner < SPEC_TEST_OWNER_COUNT; owner++)
  {
    if (!spec_binding_test_record_matches(
            spec_test_fixture_loaded_binding(fixture, owner), owner, requested_names[owner],
            requested_names[owner], SPEC_BINDING_RESOLVED, locations[owner], error, error_size))
      return spec_binding_test_finish_fixture(fixture, false, error, error_size);
  }
  if (spec_test_fixture_loaded_handler(fixture, SPEC_TEST_OWNER_MOBILE) != postmaster ||
      spec_test_fixture_loaded_handler(fixture, SPEC_TEST_OWNER_OBJECT) != greyhawk_ship_object ||
      spec_test_fixture_loaded_handler(fixture, SPEC_TEST_OWNER_ROOM) != greyhawk_ship_commands)
  {
    spec_binding_test_set_error(error, error_size,
                                "canonical loader records produced unexpected callbacks");
    return spec_binding_test_finish_fixture(fixture, false, error, error_size);
  }

  return spec_binding_test_finish_fixture(fixture, true, error, error_size);
}

static bool spec_binding_alias_loader_scenario(const char *sandbox, char *error, size_t error_size)
{
  struct spec_test_fixture *fixture;
  const struct spec_binding *binding;

  fixture = spec_test_fixture_create_at(sandbox, error, error_size);
  if (fixture == NULL)
    return false;
  if (!spec_test_fixture_load_binding_names(fixture, "Guildmaster", "Bank", "Bazaar", error,
                                            error_size))
    return spec_binding_test_finish_fixture(fixture, false, error, error_size);

  binding = spec_test_fixture_loaded_binding(fixture, SPEC_TEST_OWNER_MOBILE);
  if (!spec_binding_test_record_matches(binding, SPEC_TEST_OWNER_MOBILE, "Guildmaster", "Guild",
                                        SPEC_BINDING_RESOLVED, "mobile SpecProc field", error,
                                        error_size) ||
      binding->requested_name == binding->definition->canonical_name ||
      spec_binding_callback(binding) != guild ||
      !spec_binding_test_record_matches(
          spec_test_fixture_loaded_binding(fixture, SPEC_TEST_OWNER_OBJECT), SPEC_TEST_OWNER_OBJECT,
          "Bank", "Bank", SPEC_BINDING_RESOLVED, "object Z field", error, error_size) ||
      !spec_binding_test_record_matches(
          spec_test_fixture_loaded_binding(fixture, SPEC_TEST_OWNER_ROOM), SPEC_TEST_OWNER_ROOM,
          "Bazaar", "Bazaar", SPEC_BINDING_RESOLVED, "special-procedure test fixture room Z field",
          error, error_size))
    return spec_binding_test_finish_fixture(fixture, false, error, error_size);

  return spec_binding_test_finish_fixture(fixture, true, error, error_size);
}

static bool spec_binding_unknown_loader_scenario(const char *sandbox, char *error,
                                                 size_t error_size)
{
  static const char *const names[SPEC_TEST_OWNER_COUNT] = {
      "Missing Mobile Procedure",
      "Missing Object Procedure",
      "Missing Room Procedure",
  };
  static const char *const locations[SPEC_TEST_OWNER_COUNT] = {
      "mobile SpecProc field",
      "object Z field",
      "special-procedure test fixture room Z field",
  };
  struct spec_test_fixture *fixture;
  const struct spec_binding *binding;
  int owner;

  fixture = spec_test_fixture_create_at(sandbox, error, error_size);
  if (fixture == NULL)
    return false;
  if (!spec_test_fixture_load_binding_names(fixture, names[SPEC_TEST_OWNER_MOBILE],
                                            names[SPEC_TEST_OWNER_OBJECT],
                                            names[SPEC_TEST_OWNER_ROOM], error, error_size))
    return spec_binding_test_finish_fixture(fixture, false, error, error_size);

  for (owner = SPEC_TEST_OWNER_MOBILE; owner < SPEC_TEST_OWNER_COUNT; owner++)
  {
    binding = spec_test_fixture_loaded_binding(fixture, owner);
    if (!spec_binding_test_record_matches(binding, owner, names[owner], NULL,
                                          SPEC_BINDING_UNKNOWN_NAME, locations[owner], error,
                                          error_size) ||
        spec_test_fixture_loaded_handler(fixture, owner) != NULL ||
        spec_binding_callback(binding) != NULL ||
        !spec_binding_test_diagnostic_matches(binding, "unknown special procedure", error,
                                              error_size))
      return spec_binding_test_finish_fixture(fixture, false, error, error_size);
  }

  return spec_binding_test_finish_fixture(fixture, true, error, error_size);
}

static bool spec_binding_incompatible_loader_scenario(const char *sandbox, char *error,
                                                      size_t error_size)
{
  static const char *const names[SPEC_TEST_OWNER_COUNT] = {
      "Bazaar",
      "Postmaster",
      "Crafting Kit",
  };
  static const char *const canonical_names[SPEC_TEST_OWNER_COUNT] = {
      "Bazaar",
      "Postmaster",
      "Crafting Kit",
  };
  static const char *const locations[SPEC_TEST_OWNER_COUNT] = {
      "mobile SpecProc field",
      "object Z field",
      "special-procedure test fixture room Z field",
  };
  struct spec_test_fixture *fixture;
  const struct spec_binding *binding;
  int owner;

  fixture = spec_test_fixture_create_at(sandbox, error, error_size);
  if (fixture == NULL)
    return false;
  if (!spec_test_fixture_load_binding_names(fixture, names[SPEC_TEST_OWNER_MOBILE],
                                            names[SPEC_TEST_OWNER_OBJECT],
                                            names[SPEC_TEST_OWNER_ROOM], error, error_size))
    return spec_binding_test_finish_fixture(fixture, false, error, error_size);

  for (owner = SPEC_TEST_OWNER_MOBILE; owner < SPEC_TEST_OWNER_COUNT; owner++)
  {
    binding = spec_test_fixture_loaded_binding(fixture, owner);
    if (!spec_binding_test_record_matches(binding, owner, names[owner], canonical_names[owner],
                                          SPEC_BINDING_INCOMPATIBLE_OWNER, locations[owner], error,
                                          error_size) ||
        spec_test_fixture_loaded_handler(fixture, owner) != NULL ||
        spec_binding_callback(binding) != NULL ||
        !spec_binding_test_diagnostic_matches(binding, "incompatible owner", error, error_size))
      return spec_binding_test_finish_fixture(fixture, false, error, error_size);
  }

  return spec_binding_test_finish_fixture(fixture, true, error, error_size);
}

static bool spec_binding_olc_lifecycle_scenario(const char *sandbox, char *error, size_t error_size)
{
  static const char *const missing_names[SPEC_TEST_OWNER_COUNT] = {
      "Missing Mobile Procedure",
      "Missing Object Procedure",
      "Missing Room Procedure",
  };
  static const char *const selections[SPEC_TEST_OWNER_COUNT] = {
      "14",
      "5",
      "6",
  };
  static const char *const selected_names[SPEC_TEST_OWNER_COUNT] = {
      "Postmaster",
      "Greyhawk Ship",
      "Greyhawk Ship Commands",
  };
  static const spec_legacy_handler selected_handlers[SPEC_TEST_OWNER_COUNT] = {
      postmaster,
      greyhawk_ship_object,
      greyhawk_ship_commands,
  };
  struct spec_test_fixture *fixture;
  const struct spec_binding *olc_binding;
  const struct spec_binding *prototype_binding;
  int owner;

  fixture = spec_test_fixture_create_at(sandbox, error, error_size);
  if (fixture == NULL)
    return false;
  if (!spec_test_fixture_load_binding_names(fixture, missing_names[SPEC_TEST_OWNER_MOBILE],
                                            missing_names[SPEC_TEST_OWNER_OBJECT],
                                            missing_names[SPEC_TEST_OWNER_ROOM], error, error_size))
    return spec_binding_test_finish_fixture(fixture, false, error, error_size);

  for (owner = SPEC_TEST_OWNER_MOBILE; owner < SPEC_TEST_OWNER_COUNT; owner++)
  {
    prototype_binding = spec_test_fixture_loaded_binding(fixture, owner);
    if (!spec_test_fixture_setup_existing_olc(fixture, owner, error, error_size))
      return spec_binding_test_finish_fixture(fixture, false, error, error_size);
    olc_binding = spec_test_fixture_olc_binding(fixture, owner);
    if (olc_binding == NULL || olc_binding == prototype_binding ||
        olc_binding->requested_name == prototype_binding->requested_name ||
        strcmp(olc_binding->requested_name, missing_names[owner]) != 0 ||
        olc_binding->resolution != SPEC_BINDING_UNKNOWN_NAME)
    {
      spec_binding_test_set_error(error, error_size,
                                  "owner %d OLC setup did not deep-copy unresolved state", owner);
      return spec_binding_test_finish_fixture(fixture, false, error, error_size);
    }

    if (!spec_test_fixture_parse_olc(fixture, owner, selections[owner]))
    {
      spec_binding_test_set_error(error, error_size, "owner %d OLC selection failed", owner);
      return spec_binding_test_finish_fixture(fixture, false, error, error_size);
    }
    olc_binding = spec_test_fixture_olc_binding(fixture, owner);
    if (olc_binding == NULL || olc_binding->resolution != SPEC_BINDING_RESOLVED ||
        strcmp(olc_binding->requested_name, selected_names[owner]) != 0 ||
        spec_test_fixture_olc_handler(fixture, owner) != selected_handlers[owner] ||
        !spec_test_fixture_save_current_olc(fixture, owner))
    {
      spec_binding_test_set_error(error, error_size, "owner %d OLC replacement/save failed", owner);
      return spec_binding_test_finish_fixture(fixture, false, error, error_size);
    }

    prototype_binding = spec_test_fixture_loaded_binding(fixture, owner);
    if (prototype_binding == NULL || prototype_binding == olc_binding ||
        prototype_binding->requested_name == olc_binding->requested_name ||
        prototype_binding->prototype_vnum != spec_binding_test_vnums[owner] ||
        strcmp(prototype_binding->requested_name, selected_names[owner]) != 0 ||
        spec_test_fixture_loaded_handler(fixture, owner) != selected_handlers[owner])
    {
      spec_binding_test_set_error(error, error_size,
                                  "owner %d prototype did not own saved OLC state", owner);
      return spec_binding_test_finish_fixture(fixture, false, error, error_size);
    }

    if (!spec_test_fixture_open_olc_menu(fixture, owner) ||
        !spec_test_fixture_parse_olc(fixture, owner, "0") ||
        spec_test_fixture_olc_binding(fixture, owner) != NULL ||
        !spec_test_fixture_save_current_olc(fixture, owner) ||
        spec_test_fixture_loaded_binding(fixture, owner) != NULL ||
        spec_test_fixture_loaded_handler(fixture, owner) != NULL)
    {
      spec_binding_test_set_error(error, error_size, "owner %d OLC clear did not persist", owner);
      return spec_binding_test_finish_fixture(fixture, false, error, error_size);
    }
  }

  return spec_binding_test_finish_fixture(fixture, true, error, error_size);
}

void TestSpecAuthoredBindingModelCopyReplaceAndCleanup(CuTest *tc)
{
  struct spec_binding *binding;
  struct spec_binding *copy;
  struct spec_binding *original;
  char *original_name;
  char diagnostic[256];
  char error[256];

  binding = NULL;
  copy = NULL;
  CuAssertTrue(tc, spec_binding_replace(&binding, SPEC_OWNER_MOBILE, 1201U, "Guildmaster",
                                        SPEC_BINDING_SOURCE_WORLD, "unit mobile field", error,
                                        sizeof(error)));
  CuAssertPtrNotNull(tc, binding);
  if (binding == NULL)
    return;
  CuAssertIntEquals(tc, SPEC_BINDING_RESOLVED, binding->resolution);
  CuAssertStrEquals(tc, "Guildmaster", binding->requested_name);
  CuAssertPtrNotNull(tc, binding->definition);
  if (binding->definition == NULL)
  {
    spec_binding_free(&binding);
    return;
  }
  CuAssertStrEquals(tc, "Guild", binding->definition->canonical_name);
  CuAssertTrue(tc, spec_binding_callback(binding) == guild);
  CuAssertTrue(tc, !spec_binding_format_diagnostic(binding, diagnostic, sizeof(diagnostic)));

  CuAssertTrue(tc, spec_binding_copy(&copy, binding, error, sizeof(error)));
  CuAssertTrue(tc, copy != binding);
  CuAssertTrue(tc, copy->requested_name != binding->requested_name);
  CuAssertTrue(tc, copy->source_location != binding->source_location);
  CuAssertTrue(tc, copy->definition == binding->definition);
  CuAssertTrue(tc, spec_binding_copy(&copy, copy, error, sizeof(error)));

  original = binding;
  original_name = binding->requested_name;
  CuAssertTrue(tc, !spec_binding_replace(&binding, SPEC_OWNER_ALL, 1201U, "Postmaster",
                                         SPEC_BINDING_SOURCE_WORLD, "invalid owner", error,
                                         sizeof(error)));
  CuAssertTrue(tc, binding == original);
  CuAssertTrue(tc, binding->requested_name == original_name);
  CuAssertPtrNotNull(tc, strstr(error, "owner is invalid"));

  CuAssertTrue(tc, spec_binding_copy(&copy, NULL, error, sizeof(error)));
  CuAssertPtrEquals(tc, NULL, copy);
  spec_binding_free(&binding);
  spec_binding_free(&binding);
  CuAssertPtrEquals(tc, NULL, binding);
}

void TestSpecAuthoredBindingSourceCompatibilityAndDiagnostics(CuTest *tc)
{
  struct spec_binding *binding;
  char diagnostic[256];
  char tiny[8];
  char error[256];

  binding = NULL;
  CuAssertTrue(tc, spec_binding_replace(&binding, SPEC_OWNER_MOBILE, 1201U, "Postmaster",
                                        SPEC_BINDING_SOURCE_SHOP, "shop fixture", error,
                                        sizeof(error)));
  CuAssertPtrNotNull(tc, binding);
  if (binding == NULL)
    return;
  CuAssertIntEquals(tc, SPEC_BINDING_INCOMPATIBLE_SOURCE, binding->resolution);
  CuAssertPtrNotNull(tc, binding->definition);
  CuAssertPtrEquals(tc, NULL, spec_binding_callback(binding));
  CuAssertStrEquals(tc, "shop", spec_binding_source_name(binding->source));
  CuAssertStrEquals(tc, "incompatible source", spec_binding_resolution_name(binding->resolution));
  CuAssertTrue(tc, spec_binding_format_diagnostic(binding, diagnostic, sizeof(diagnostic)));
  CuAssertPtrNotNull(tc, strstr(diagnostic, "shop binding at shop fixture"));
  CuAssertPtrNotNull(tc, strstr(diagnostic, "mobile prototype #1201"));
  CuAssertPtrNotNull(tc, strstr(diagnostic, "requested 'Postmaster'"));
  CuAssertPtrNotNull(tc, strstr(diagnostic, "incompatible source"));

  memset(tiny, 'X', sizeof(tiny));
  CuAssertTrue(tc, spec_binding_format_diagnostic(binding, tiny, sizeof(tiny)));
  CuAssertIntEquals(tc, '\0', tiny[sizeof(tiny) - 1]);
  spec_binding_free(&binding);
}

void TestSpecAuthoredBindingCanonicalLoaders(CuTest *tc)
{
  char error[SPEC_BINDING_TEST_ERROR_SIZE];

  error[0] = '\0';
  CuAssertTrue(
      tc, spec_test_run_isolated(spec_binding_canonical_loader_scenario, error, sizeof(error)));
  CuAssertStrEquals(tc, "", error);
}

void TestSpecAuthoredBindingAliasRetention(CuTest *tc)
{
  char error[SPEC_BINDING_TEST_ERROR_SIZE];

  error[0] = '\0';
  CuAssertTrue(tc,
               spec_test_run_isolated(spec_binding_alias_loader_scenario, error, sizeof(error)));
  CuAssertStrEquals(tc, "", error);
}

void TestSpecAuthoredBindingUnknownLoaders(CuTest *tc)
{
  char error[SPEC_BINDING_TEST_ERROR_SIZE];

  error[0] = '\0';
  CuAssertTrue(tc,
               spec_test_run_isolated(spec_binding_unknown_loader_scenario, error, sizeof(error)));
  CuAssertStrEquals(tc, "", error);
}

void TestSpecAuthoredBindingIncompatibleOwnerLoaders(CuTest *tc)
{
  char error[SPEC_BINDING_TEST_ERROR_SIZE];

  error[0] = '\0';
  CuAssertTrue(
      tc, spec_test_run_isolated(spec_binding_incompatible_loader_scenario, error, sizeof(error)));
  CuAssertStrEquals(tc, "", error);
}

void TestSpecAuthoredBindingOlcLifecycle(CuTest *tc)
{
  char error[SPEC_BINDING_TEST_ERROR_SIZE];
  bool success;

  error[0] = '\0';
  success = spec_test_run_isolated(spec_binding_olc_lifecycle_scenario, error, sizeof(error));
  CuAssert(tc, error, success);
  CuAssertStrEquals(tc, "", error);
}
