#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"

#include "../../src/obj/treasure.h"
#include "../../src/character/guild_services.h"
#include "../../src/comms/mail.h"
#include "../../src/obj/vendor.h"
#include "../../src/spec/spec_binding.h"
#include "../../src/vessels/vessels_legacy.h"
#include "test_spec_fixtures.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define SPEC_ROUND_TRIP_ERROR_SIZE 512
#define SPEC_ROUND_TRIP_CHILD_TIMEOUT 30

enum spec_round_trip_action
{
  SPEC_ROUND_TRIP_ALIAS = 0,
  SPEC_ROUND_TRIP_UNKNOWN,
  SPEC_ROUND_TRIP_INCOMPATIBLE,
  SPEC_ROUND_TRIP_SELECT,
  SPEC_ROUND_TRIP_CLEAR,
  SPEC_ROUND_TRIP_LEGACY_FALLBACK
};

struct spec_round_trip_child_result
{
  int success;
  char error[SPEC_ROUND_TRIP_ERROR_SIZE];
};

static const spec_owner_mask spec_round_trip_owners[SPEC_TEST_OWNER_COUNT] = {
    SPEC_OWNER_MOBILE,
    SPEC_OWNER_OBJECT,
    SPEC_OWNER_ROOM,
};

static const unsigned int spec_round_trip_vnums[SPEC_TEST_OWNER_COUNT] = {
    1201U,
    1402U,
    1403U,
};

static const spec_legacy_handler spec_round_trip_overrides[SPEC_TEST_OWNER_COUNT] = {
    postmaster,
    greyhawk_ship_object,
    greyhawk_ship_commands,
};

static const char *const spec_round_trip_selections[SPEC_TEST_OWNER_COUNT] = {
    "14",
    "5",
    "6",
};

static const char *const spec_round_trip_selected_names[SPEC_TEST_OWNER_COUNT] = {
    "Postmaster",
    "Greyhawk Ship",
    "Greyhawk Ship Commands",
};

static void spec_round_trip_set_error(char *error, size_t error_size, const char *format, ...)
{
  va_list arguments;

  if (error == NULL || error_size == 0)
    return;

  va_start(arguments, format);
  /* NOLINTNEXTLINE(clang-analyzer-valist.Uninitialized) -- va_start initializes arguments. */
  vsnprintf(error, error_size, format, arguments);
  va_end(arguments);
}

static bool spec_round_trip_write_all(int descriptor, const void *buffer, size_t length)
{
  const char *cursor;
  ssize_t written;

  cursor = buffer;
  while (length > 0)
  {
    written = write(descriptor, cursor, length);
    if (written < 0 && errno == EINTR)
      continue;
    if (written <= 0)
      return false;
    cursor += written;
    length -= (size_t)written;
  }

  return true;
}

static bool spec_round_trip_finish_fixture(struct spec_test_fixture *fixture, bool success,
                                           char *error, size_t error_size)
{
  char cleanup_error[SPEC_ROUND_TRIP_ERROR_SIZE];

  cleanup_error[0] = '\0';
  if (!spec_test_fixture_destroy(fixture, cleanup_error, sizeof(cleanup_error)))
  {
    if (success || error == NULL || error[0] == '\0')
      spec_round_trip_set_error(error, error_size, "%s", cleanup_error);
    return false;
  }

  return success;
}

static bool spec_round_trip_prepare_unrelated_save(struct spec_test_fixture *fixture, char *error,
                                                   size_t error_size)
{
  int owner;

  for (owner = SPEC_TEST_OWNER_MOBILE; owner < SPEC_TEST_OWNER_COUNT; owner++)
  {
    if (!spec_test_fixture_set_loaded_handler(fixture, owner, spec_round_trip_overrides[owner]) ||
        !spec_test_fixture_setup_existing_olc(fixture, owner, error, error_size) ||
        !spec_test_fixture_save_current_olc(fixture, owner))
    {
      if (error == NULL || error[0] == '\0')
        spec_round_trip_set_error(error, error_size,
                                  "owner %d unrelated OLC save preparation failed", owner);
      return false;
    }
  }

  return true;
}

static bool spec_round_trip_prepare_selection(struct spec_test_fixture *fixture, bool clear,
                                              char *error, size_t error_size)
{
  const struct spec_binding *binding;
  int owner;

  for (owner = SPEC_TEST_OWNER_MOBILE; owner < SPEC_TEST_OWNER_COUNT; owner++)
  {
    if (!spec_test_fixture_setup_existing_olc(fixture, owner, error, error_size) ||
        !spec_test_fixture_parse_olc(fixture, owner,
                                     clear ? "0" : spec_round_trip_selections[owner]) ||
        !spec_test_fixture_save_current_olc(fixture, owner))
    {
      if (error == NULL || error[0] == '\0')
        spec_round_trip_set_error(error, error_size, "owner %d explicit OLC action failed", owner);
      return false;
    }

    binding = spec_test_fixture_loaded_binding(fixture, owner);
    if (clear)
    {
      if (binding != NULL || spec_test_fixture_loaded_handler(fixture, owner) != NULL)
      {
        spec_round_trip_set_error(error, error_size, "owner %d explicit clear left prototype state",
                                  owner);
        return false;
      }
    }
    else if (binding == NULL || binding->requested_name == NULL ||
             strcmp(binding->requested_name, spec_round_trip_selected_names[owner]) != 0 ||
             spec_test_fixture_loaded_handler(fixture, owner) != spec_round_trip_overrides[owner])
    {
      spec_round_trip_set_error(error, error_size, "owner %d explicit selection was not canonical",
                                owner);
      return false;
    }
  }

  return true;
}

static bool spec_round_trip_writer(struct spec_test_fixture *fixture,
                                   enum spec_round_trip_action action, char *error,
                                   size_t error_size)
{
  static const char *const alias_names[SPEC_TEST_OWNER_COUNT] = {
      "Guildmaster",
      "Bank",
      "Bazaar",
  };
  static const char *const unknown_names[SPEC_TEST_OWNER_COUNT] = {
      "Missing Mobile Procedure",
      "Missing Object Procedure",
      "Missing Room Procedure",
  };
  static const char *const incompatible_names[SPEC_TEST_OWNER_COUNT] = {
      "Bazaar",
      "Postmaster",
      "Crafting Kit",
  };
  const char *const *names;
  int owner;

  switch (action)
  {
  case SPEC_ROUND_TRIP_ALIAS:
    names = alias_names;
    break;
  case SPEC_ROUND_TRIP_UNKNOWN:
  case SPEC_ROUND_TRIP_SELECT:
    names = unknown_names;
    break;
  case SPEC_ROUND_TRIP_INCOMPATIBLE:
    names = incompatible_names;
    break;
  case SPEC_ROUND_TRIP_CLEAR:
  case SPEC_ROUND_TRIP_LEGACY_FALLBACK:
    names = spec_round_trip_selected_names;
    break;
  default:
    spec_round_trip_set_error(error, error_size, "invalid round-trip writer action");
    return false;
  }

  if (!spec_test_fixture_load_binding_names(fixture, names[SPEC_TEST_OWNER_MOBILE],
                                            names[SPEC_TEST_OWNER_OBJECT],
                                            names[SPEC_TEST_OWNER_ROOM], error, error_size))
    return false;

  switch (action)
  {
  case SPEC_ROUND_TRIP_ALIAS:
  case SPEC_ROUND_TRIP_UNKNOWN:
  case SPEC_ROUND_TRIP_INCOMPATIBLE:
    if (!spec_round_trip_prepare_unrelated_save(fixture, error, error_size))
      return false;
    break;
  case SPEC_ROUND_TRIP_SELECT:
    if (!spec_round_trip_prepare_selection(fixture, false, error, error_size))
      return false;
    break;
  case SPEC_ROUND_TRIP_CLEAR:
    if (!spec_round_trip_prepare_selection(fixture, true, error, error_size))
      return false;
    break;
  case SPEC_ROUND_TRIP_LEGACY_FALLBACK:
    for (owner = SPEC_TEST_OWNER_MOBILE; owner < SPEC_TEST_OWNER_COUNT; owner++)
      if (!spec_test_fixture_discard_loaded_binding(fixture, owner))
      {
        spec_round_trip_set_error(error, error_size, "owner %d legacy fallback preparation failed",
                                  owner);
        return false;
      }
    break;
  }

  return spec_test_fixture_save_named_bindings(fixture, error, error_size);
}

static bool spec_round_trip_run_writer(struct spec_test_fixture *fixture,
                                       enum spec_round_trip_action action, char *error,
                                       size_t error_size)
{
  struct spec_round_trip_child_result result;
  int result_pipe[2];
  int child_status;
  pid_t child_pid;
  pid_t waited_pid;
  size_t received;
  ssize_t read_result;

  memset(&result, 0, sizeof(result));
  if (pipe(result_pipe) != 0)
  {
    spec_round_trip_set_error(error, error_size, "unable to create round-trip writer pipe");
    return false;
  }

  child_pid = fork();
  if (child_pid < 0)
  {
    close(result_pipe[0]);
    close(result_pipe[1]);
    spec_round_trip_set_error(error, error_size, "unable to fork round-trip writer");
    return false;
  }
  if (child_pid == 0)
  {
    close(result_pipe[0]);
    alarm(SPEC_ROUND_TRIP_CHILD_TIMEOUT);
    result.success = spec_round_trip_writer(fixture, action, result.error, sizeof(result.error));
    if (!result.success && result.error[0] == '\0')
      spec_round_trip_set_error(result.error, sizeof(result.error), "round-trip writer failed");
    if (!spec_round_trip_write_all(result_pipe[1], &result, sizeof(result)))
      _exit(2);
    close(result_pipe[1]);
    _exit(result.success ? 0 : 1);
  }

  close(result_pipe[1]);
  received = 0;
  while (received < sizeof(result))
  {
    read_result = read(result_pipe[0], (char *)&result + received, sizeof(result) - received);
    if (read_result < 0 && errno == EINTR)
      continue;
    if (read_result <= 0)
      break;
    received += (size_t)read_result;
  }
  close(result_pipe[0]);

  do
  {
    waited_pid = waitpid(child_pid, &child_status, 0);
  } while (waited_pid < 0 && errno == EINTR);

  if (waited_pid != child_pid || received != sizeof(result) || !WIFEXITED(child_status) ||
      WEXITSTATUS(child_status) != 0 || !result.success)
  {
    spec_round_trip_set_error(error, error_size,
                              result.error[0] != '\0' ? result.error
                                                      : "round-trip writer exited unexpectedly");
    return false;
  }

  return true;
}

static bool spec_round_trip_saved_name_matches(const struct spec_test_fixture *fixture,
                                               enum spec_test_owner owner, const char *name,
                                               char *error, size_t error_size)
{
  const char *text;
  char expected[256];
  int length;

  text = spec_test_fixture_saved_text(fixture, owner);
  if (text == NULL || name == NULL)
  {
    spec_round_trip_set_error(error, error_size, "owner %d has no captured output", owner);
    return false;
  }

  if (owner == SPEC_TEST_OWNER_MOBILE)
    length = snprintf(expected, sizeof(expected), "\nSpecProc: %s\n", name);
  else
    length = snprintf(expected, sizeof(expected), "\nZ\n%s\n", name);
  if (length < 0 || (size_t)length >= sizeof(expected) || strstr(text, expected) == NULL)
  {
    spec_round_trip_set_error(error, error_size,
                              "owner %d output did not retain authored name '%s'", owner, name);
    return false;
  }

  return true;
}

static bool spec_round_trip_saved_binding_absent(const struct spec_test_fixture *fixture,
                                                 enum spec_test_owner owner, char *error,
                                                 size_t error_size)
{
  const char *marker;
  const char *text;

  text = spec_test_fixture_saved_text(fixture, owner);
  marker = owner == SPEC_TEST_OWNER_MOBILE ? "\nSpecProc:" : "\nZ\n";
  if (text == NULL || strstr(text, marker) != NULL)
  {
    spec_round_trip_set_error(error, error_size, "owner %d explicit clear still emitted a binding",
                              owner);
    return false;
  }

  return true;
}

static bool spec_round_trip_binding_matches(const struct spec_test_fixture *fixture,
                                            enum spec_test_owner owner, const char *requested_name,
                                            const char *canonical_name,
                                            enum spec_binding_resolution resolution, char *error,
                                            size_t error_size)
{
  const struct spec_binding *binding;

  binding = spec_test_fixture_loaded_binding(fixture, owner);
  if (binding == NULL || binding->owner != spec_round_trip_owners[owner] ||
      binding->prototype_vnum != spec_round_trip_vnums[owner] ||
      binding->source != SPEC_BINDING_SOURCE_WORLD || binding->resolution != resolution ||
      binding->requested_name == NULL || strcmp(binding->requested_name, requested_name) != 0)
  {
    spec_round_trip_set_error(error, error_size,
                              "owner %d reloaded authored metadata did not match", owner);
    return false;
  }
  if (canonical_name == NULL)
  {
    if (binding->definition != NULL)
    {
      spec_round_trip_set_error(error, error_size, "owner %d unexpectedly resolved after reload",
                                owner);
      return false;
    }
  }
  else if (binding->definition == NULL ||
           strcmp(binding->definition->canonical_name, canonical_name) != 0)
  {
    spec_round_trip_set_error(error, error_size, "owner %d reloaded the wrong canonical definition",
                              owner);
    return false;
  }

  return true;
}

static bool
spec_round_trip_scenario(const char *sandbox, enum spec_round_trip_action action,
                         const char *const requested_names[SPEC_TEST_OWNER_COUNT],
                         const char *const canonical_names[SPEC_TEST_OWNER_COUNT],
                         enum spec_binding_resolution resolution,
                         const spec_legacy_handler expected_handlers[SPEC_TEST_OWNER_COUNT],
                         bool expect_absent, char *error, size_t error_size)
{
  struct spec_test_fixture *fixture;
  int owner;

  fixture = spec_test_fixture_create_at(sandbox, error, error_size);
  if (fixture == NULL)
    return false;
  if (!spec_round_trip_run_writer(fixture, action, error, error_size) ||
      !spec_test_fixture_load_saved_bindings(fixture, error, error_size))
    return spec_round_trip_finish_fixture(fixture, false, error, error_size);

  for (owner = SPEC_TEST_OWNER_MOBILE; owner < SPEC_TEST_OWNER_COUNT; owner++)
  {
    if (expect_absent)
    {
      if (!spec_round_trip_saved_binding_absent(fixture, owner, error, error_size) ||
          spec_test_fixture_loaded_binding(fixture, owner) != NULL ||
          spec_test_fixture_loaded_handler(fixture, owner) != NULL)
      {
        if (error == NULL || error[0] == '\0')
          spec_round_trip_set_error(error, error_size, "owner %d clear did not survive reload",
                                    owner);
        return spec_round_trip_finish_fixture(fixture, false, error, error_size);
      }
      continue;
    }

    if (!spec_round_trip_saved_name_matches(fixture, owner, requested_names[owner], error,
                                            error_size) ||
        !spec_round_trip_binding_matches(fixture, owner, requested_names[owner],
                                         canonical_names[owner], resolution, error, error_size) ||
        spec_test_fixture_loaded_handler(fixture, owner) != expected_handlers[owner])
    {
      if (error == NULL || error[0] == '\0')
        spec_round_trip_set_error(error, error_size, "owner %d reloaded an unexpected callback",
                                  owner);
      return spec_round_trip_finish_fixture(fixture, false, error, error_size);
    }
  }

  return spec_round_trip_finish_fixture(fixture, true, error, error_size);
}

static bool spec_round_trip_alias_scenario(const char *sandbox, char *error, size_t error_size)
{
  static const char *const requested_names[SPEC_TEST_OWNER_COUNT] = {
      "Guildmaster",
      "Bank",
      "Bazaar",
  };
  static const char *const canonical_names[SPEC_TEST_OWNER_COUNT] = {
      "Guild",
      "Bank",
      "Bazaar",
  };
  static const spec_legacy_handler handlers[SPEC_TEST_OWNER_COUNT] = {
      guild,
      bank,
      bazaar,
  };

  return spec_round_trip_scenario(sandbox, SPEC_ROUND_TRIP_ALIAS, requested_names, canonical_names,
                                  SPEC_BINDING_RESOLVED, handlers, false, error, error_size);
}

static bool spec_round_trip_unknown_scenario(const char *sandbox, char *error, size_t error_size)
{
  static const char *const names[SPEC_TEST_OWNER_COUNT] = {
      "Missing Mobile Procedure",
      "Missing Object Procedure",
      "Missing Room Procedure",
  };
  static const char *const canonical_names[SPEC_TEST_OWNER_COUNT] = {NULL, NULL, NULL};
  static const spec_legacy_handler handlers[SPEC_TEST_OWNER_COUNT] = {NULL, NULL, NULL};

  return spec_round_trip_scenario(sandbox, SPEC_ROUND_TRIP_UNKNOWN, names, canonical_names,
                                  SPEC_BINDING_UNKNOWN_NAME, handlers, false, error, error_size);
}

static bool spec_round_trip_incompatible_scenario(const char *sandbox, char *error,
                                                  size_t error_size)
{
  static const char *const names[SPEC_TEST_OWNER_COUNT] = {
      "Bazaar",
      "Postmaster",
      "Crafting Kit",
  };
  static const spec_legacy_handler handlers[SPEC_TEST_OWNER_COUNT] = {NULL, NULL, NULL};

  return spec_round_trip_scenario(sandbox, SPEC_ROUND_TRIP_INCOMPATIBLE, names, names,
                                  SPEC_BINDING_INCOMPATIBLE_OWNER, handlers, false, error,
                                  error_size);
}

static bool spec_round_trip_selection_scenario(const char *sandbox, char *error, size_t error_size)
{
  return spec_round_trip_scenario(sandbox, SPEC_ROUND_TRIP_SELECT, spec_round_trip_selected_names,
                                  spec_round_trip_selected_names, SPEC_BINDING_RESOLVED,
                                  spec_round_trip_overrides, false, error, error_size);
}

static bool spec_round_trip_clear_scenario(const char *sandbox, char *error, size_t error_size)
{
  static const char *const unused_names[SPEC_TEST_OWNER_COUNT] = {NULL, NULL, NULL};
  static const spec_legacy_handler unused_handlers[SPEC_TEST_OWNER_COUNT] = {NULL, NULL, NULL};

  return spec_round_trip_scenario(sandbox, SPEC_ROUND_TRIP_CLEAR, unused_names, unused_names,
                                  SPEC_BINDING_UNKNOWN_NAME, unused_handlers, true, error,
                                  error_size);
}

static bool spec_round_trip_legacy_fallback_scenario(const char *sandbox, char *error,
                                                     size_t error_size)
{
  return spec_round_trip_scenario(sandbox, SPEC_ROUND_TRIP_LEGACY_FALLBACK,
                                  spec_round_trip_selected_names, spec_round_trip_selected_names,
                                  SPEC_BINDING_RESOLVED, spec_round_trip_overrides, false, error,
                                  error_size);
}

void TestSpecBindingPersistenceNameContract(CuTest *tc)
{
  struct spec_binding *binding;
  char error[SPEC_ROUND_TRIP_ERROR_SIZE];

  binding = NULL;
  CuAssertPtrEquals(tc, NULL, spec_binding_persisted_name(NULL));
  CuAssertTrue(tc, spec_binding_replace(&binding, SPEC_OWNER_MOBILE, 1201U, "Guildmaster",
                                        SPEC_BINDING_SOURCE_WORLD, "unit persistence field", error,
                                        sizeof(error)));
  CuAssertStrEquals(tc, "Guildmaster", spec_binding_persisted_name(binding));
  spec_binding_free(&binding);

  CuAssertTrue(tc, spec_binding_replace(&binding, SPEC_OWNER_MOBILE, 1201U,
                                        "Missing Mobile Procedure", SPEC_BINDING_SOURCE_WORLD,
                                        "unit persistence field", error, sizeof(error)));
  CuAssertStrEquals(tc, "Missing Mobile Procedure", spec_binding_persisted_name(binding));
  spec_binding_free(&binding);

  CuAssertTrue(tc, spec_binding_replace(&binding, SPEC_OWNER_MOBILE, 1201U, "Invalid\nName",
                                        SPEC_BINDING_SOURCE_WORLD, "unit persistence field", error,
                                        sizeof(error)));
  CuAssertPtrEquals(tc, NULL, spec_binding_persisted_name(binding));
  spec_binding_free(&binding);

  CuAssertTrue(tc, spec_binding_replace(&binding, SPEC_OWNER_MOBILE, 1201U, "Postmaster",
                                        SPEC_BINDING_SOURCE_SHOP, "unit shop binding", error,
                                        sizeof(error)));
  CuAssertPtrEquals(tc, NULL, spec_binding_persisted_name(binding));
  spec_binding_free(&binding);
}

void TestSpecBindingAliasAndOverrideRoundTrip(CuTest *tc)
{
  char error[SPEC_ROUND_TRIP_ERROR_SIZE];

  error[0] = '\0';
  CuAssert(tc, error, spec_test_run_isolated(spec_round_trip_alias_scenario, error, sizeof(error)));
}

void TestSpecBindingUnknownAndOverrideRoundTrip(CuTest *tc)
{
  char error[SPEC_ROUND_TRIP_ERROR_SIZE];

  error[0] = '\0';
  CuAssert(tc, error,
           spec_test_run_isolated(spec_round_trip_unknown_scenario, error, sizeof(error)));
}

void TestSpecBindingIncompatibleAndOverrideRoundTrip(CuTest *tc)
{
  char error[SPEC_ROUND_TRIP_ERROR_SIZE];

  error[0] = '\0';
  CuAssert(tc, error,
           spec_test_run_isolated(spec_round_trip_incompatible_scenario, error, sizeof(error)));
}

void TestSpecBindingExplicitSelectionRoundTrip(CuTest *tc)
{
  char error[SPEC_ROUND_TRIP_ERROR_SIZE];

  error[0] = '\0';
  CuAssert(tc, error,
           spec_test_run_isolated(spec_round_trip_selection_scenario, error, sizeof(error)));
}

void TestSpecBindingExplicitClearRoundTrip(CuTest *tc)
{
  char error[SPEC_ROUND_TRIP_ERROR_SIZE];

  error[0] = '\0';
  CuAssert(tc, error, spec_test_run_isolated(spec_round_trip_clear_scenario, error, sizeof(error)));
}

void TestSpecBindingLegacyCallbackFallbackRoundTrip(CuTest *tc)
{
  char error[SPEC_ROUND_TRIP_ERROR_SIZE];

  error[0] = '\0';
  CuAssert(tc, error,
           spec_test_run_isolated(spec_round_trip_legacy_fallback_scenario, error, sizeof(error)));
}
