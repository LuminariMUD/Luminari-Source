#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"

#include "../../src/spec_procs.h"
#include "test_spec_fixtures.h"

#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define SPEC_TEST_CHILD_TIMEOUT 30
#define SPEC_TEST_ERROR_SIZE 256

struct spec_binding_inventory
{
  int total[SPEC_TEST_OWNER_COUNT];
  int expected[SPEC_TEST_OWNER_COUNT];
};

struct spec_child_result
{
  int success;
  char error[SPEC_TEST_ERROR_SIZE];
};

typedef bool (*spec_child_scenario)(const char *sandbox, char *error, size_t error_size);

static void spec_test_set_error(char *error, size_t error_size, const char *message)
{
  if (error == NULL || error_size == 0)
    return;

  snprintf(error, error_size, "%s", message != NULL ? message : "unknown test failure");
}

static const char *spec_test_source_root(void)
{
  const char *root;

  root = getenv("LUMINARI_TEST_ROOT");
  return root != NULL && *root != '\0' ? root : ".";
}

static void spec_test_strip_line_end(char *line)
{
  size_t length;

  length = strlen(line);
  while (length > 0 && (line[length - 1] == '\n' || line[length - 1] == '\r'))
    line[--length] = '\0';
}

static bool spec_test_has_suffix(const char *name, const char *suffix)
{
  size_t name_length;
  size_t suffix_length;

  name_length = strlen(name);
  suffix_length = strlen(suffix);
  return name_length >= suffix_length && strcmp(name + name_length - suffix_length, suffix) == 0;
}

static bool spec_test_scan_binding_file(const char *path, enum spec_test_owner owner,
                                        struct spec_binding_inventory *inventory, char *error,
                                        size_t error_size)
{
  const char *expected_name;
  FILE *file;
  char line[MAX_STRING_LENGTH];
  char *value;
  bool expect_name;
  bool read_error;
  bool success;
  int close_result;

  expected_name = owner == SPEC_TEST_OWNER_MOBILE   ? "Postmaster"
                  : owner == SPEC_TEST_OWNER_OBJECT ? "Greyhawk Ship"
                                                    : "Greyhawk Ship Commands";
  file = fopen(path, "r");
  if (file == NULL)
  {
    spec_test_set_error(error, error_size, "unable to open a checked-in world file");
    return false;
  }

  expect_name = false;
  success = true;
  while (fgets(line, sizeof(line), file) != NULL)
  {
    spec_test_strip_line_end(line);
    if (owner == SPEC_TEST_OWNER_MOBILE)
    {
      if (strncmp(line, "SpecProc:", 9) != 0)
        continue;

      value = line + 9;
      while (*value == ' ' || *value == '\t')
        value++;
      inventory->total[owner]++;
      if (strcmp(value, expected_name) == 0)
        inventory->expected[owner]++;
      continue;
    }

    if (expect_name)
    {
      inventory->total[owner]++;
      if (strcmp(line, expected_name) == 0)
        inventory->expected[owner]++;
      expect_name = false;
      continue;
    }

    if (strcmp(line, "Z") == 0)
      expect_name = true;
  }

  read_error = ferror(file) != 0;
  close_result = fclose(file);
  if (read_error || expect_name || close_result != 0)
  {
    spec_test_set_error(error, error_size, "unable to read a complete checked-in world file");
    success = false;
  }

  return success;
}

static bool spec_test_scan_binding_directory(const char *relative, const char *suffix,
                                             enum spec_test_owner owner,
                                             struct spec_binding_inventory *inventory, char *error,
                                             size_t error_size)
{
  struct dirent *entry;
  DIR *directory;
  char directory_path[PATH_MAX];
  char file_path[PATH_MAX];
  bool success;

  if (snprintf(directory_path, sizeof(directory_path), "%s/%s", spec_test_source_root(),
               relative) >= (int)sizeof(directory_path))
  {
    spec_test_set_error(error, error_size, "checked-in world directory path is too long");
    return false;
  }

  directory = opendir(directory_path);
  if (directory == NULL)
  {
    spec_test_set_error(error, error_size, "unable to open a checked-in world directory");
    return false;
  }

  success = true;
  for (;;)
  {
    errno = 0;
    entry = readdir(directory);
    if (entry == NULL)
      break;
    if (!spec_test_has_suffix(entry->d_name, suffix))
      continue;
    if (snprintf(file_path, sizeof(file_path), "%s/%s", directory_path, entry->d_name) >=
        (int)sizeof(file_path))
    {
      spec_test_set_error(error, error_size, "checked-in world file path is too long");
      success = false;
      break;
    }
    if (!spec_test_scan_binding_file(file_path, owner, inventory, error, error_size))
    {
      success = false;
      break;
    }
  }

  if (success && errno != 0)
  {
    spec_test_set_error(error, error_size, "unable to enumerate a checked-in world directory");
    success = false;
  }
  if (closedir(directory) != 0 && success)
  {
    spec_test_set_error(error, error_size, "unable to close a checked-in world directory");
    success = false;
  }

  return success;
}

static bool spec_test_write_all(int descriptor, const void *buffer, size_t length)
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

static bool spec_test_run_isolated_with_path(spec_child_scenario scenario, char *sandbox_result,
                                             size_t sandbox_result_size, char *error,
                                             size_t error_size)
{
  struct spec_child_result result;
  char cleanup_error[SPEC_TEST_ERROR_SIZE];
  char sandbox[PATH_MAX];
  int result_pipe[2];
  int child_status;
  pid_t child_pid;
  pid_t waited_pid;
  size_t received;
  ssize_t read_result;

  memset(&result, 0, sizeof(result));
  if (scenario == NULL)
  {
    spec_test_set_error(error, error_size, "cannot run a null isolated test scenario");
    return false;
  }
  if (snprintf(sandbox, sizeof(sandbox), "/tmp/luminari-spec-registry-run-XXXXXX") >=
          (int)sizeof(sandbox) ||
      mkdtemp(sandbox) == NULL)
  {
    spec_test_set_error(error, error_size, "unable to create isolated test sandbox");
    return false;
  }
  if (sandbox_result != NULL &&
      snprintf(sandbox_result, sandbox_result_size, "%s", sandbox) >= (int)sandbox_result_size)
  {
    spec_test_cleanup_sandbox(sandbox, cleanup_error, sizeof(cleanup_error));
    spec_test_set_error(error, error_size, "isolated test sandbox result buffer is too small");
    return false;
  }
  if (pipe(result_pipe) != 0)
  {
    if (!spec_test_cleanup_sandbox(sandbox, cleanup_error, sizeof(cleanup_error)))
    {
      spec_test_set_error(error, error_size, cleanup_error);
      return false;
    }
    spec_test_set_error(error, error_size, "unable to create isolated test result pipe");
    return false;
  }

  child_pid = fork();
  if (child_pid < 0)
  {
    close(result_pipe[0]);
    close(result_pipe[1]);
    if (!spec_test_cleanup_sandbox(sandbox, cleanup_error, sizeof(cleanup_error)))
    {
      spec_test_set_error(error, error_size, cleanup_error);
      return false;
    }
    spec_test_set_error(error, error_size, "unable to fork isolated parser test");
    return false;
  }

  if (child_pid == 0)
  {
    close(result_pipe[0]);
    alarm(SPEC_TEST_CHILD_TIMEOUT);
    result.success = scenario(sandbox, result.error, sizeof(result.error));
    if (!result.success && result.error[0] == '\0')
      spec_test_set_error(result.error, sizeof(result.error), "isolated scenario failed");
    if (!spec_test_write_all(result_pipe[1], &result, sizeof(result)))
      _exit(2);
    close(result_pipe[1]);
    _exit(result.success ? EXIT_SUCCESS : EXIT_FAILURE);
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

  if (!spec_test_cleanup_sandbox(sandbox, cleanup_error, sizeof(cleanup_error)))
  {
    spec_test_set_error(error, error_size, cleanup_error);
    return false;
  }
  if (waited_pid != child_pid || received != sizeof(result))
  {
    spec_test_set_error(error, error_size, "isolated parser test exited before reporting a result");
    return false;
  }
  if (!WIFEXITED(child_status))
  {
    spec_test_set_error(error, error_size, "isolated parser test did not exit normally");
    return false;
  }
  if (WEXITSTATUS(child_status) != EXIT_SUCCESS || !result.success)
  {
    spec_test_set_error(error, error_size,
                        result.error[0] != '\0' ? result.error : "isolated parser test failed");
    return false;
  }

  return true;
}

static bool spec_test_run_isolated(spec_child_scenario scenario, char *error, size_t error_size)
{
  return spec_test_run_isolated_with_path(scenario, NULL, 0, error, error_size);
}

static bool spec_test_persistence_scenario(const char *sandbox, char *error, size_t error_size)
{
  struct spec_test_fixture *fixture;
  char cleanup_error[SPEC_TEST_ERROR_SIZE];
  char repeated_error[SPEC_TEST_ERROR_SIZE];
  bool cleanup_success;
  bool success;

  fixture = spec_test_fixture_create_at(sandbox, error, error_size);
  if (fixture == NULL)
    return false;

  success = false;
  repeated_error[0] = '\0';
  if (!spec_test_fixture_load_named_bindings(fixture, error, error_size))
    goto cleanup;
  if (spec_test_fixture_loaded_handler(fixture, SPEC_TEST_OWNER_MOBILE) != postmaster ||
      spec_test_fixture_loaded_handler(fixture, SPEC_TEST_OWNER_OBJECT) != greyhawk_ship_object ||
      spec_test_fixture_loaded_handler(fixture, SPEC_TEST_OWNER_ROOM) != greyhawk_ship_commands)
  {
    spec_test_set_error(error, error_size, "production parsers resolved an unexpected handler");
    goto cleanup;
  }
  if (spec_test_fixture_load_named_bindings(fixture, repeated_error, sizeof(repeated_error)) ||
      strcmp(repeated_error, "named binding fixture cannot be loaded twice") != 0)
  {
    spec_test_set_error(error, error_size, "fixture did not reject a repeated parser lifecycle");
    goto cleanup;
  }
  if (!spec_test_fixture_save_named_bindings(fixture, error, error_size))
    goto cleanup;
  if (strstr(spec_test_fixture_saved_text(fixture, SPEC_TEST_OWNER_MOBILE),
             "\nSpecProc: Postmaster\n") == NULL ||
      strstr(spec_test_fixture_saved_text(fixture, SPEC_TEST_OWNER_OBJECT),
             "\nZ\nGreyhawk Ship\n") == NULL ||
      strstr(spec_test_fixture_saved_text(fixture, SPEC_TEST_OWNER_ROOM),
             "\nZ\nGreyhawk Ship Commands\n") == NULL)
  {
    spec_test_set_error(error, error_size, "production writers changed a canonical binding field");
    goto cleanup;
  }

  success = true;

cleanup:
  cleanup_success = spec_test_fixture_destroy(fixture, cleanup_error, sizeof(cleanup_error));
  if (!cleanup_success)
  {
    if (success || error[0] == '\0')
      spec_test_set_error(error, error_size, cleanup_error);
    success = false;
  }
  return success;
}

static bool spec_test_olc_scenario(enum spec_test_owner owner, const char *sandbox, char *error,
                                   size_t error_size)
{
  struct spec_test_fixture *fixture;
  SPECIAL_DECL(*expected_handler);
  char cleanup_error[SPEC_TEST_ERROR_SIZE];
  const char *owner_name;
  const char *valid_selection;
  char high_selection[32];
  bool cleanup_success;
  bool success;

  switch (owner)
  {
  case SPEC_TEST_OWNER_MOBILE:
    owner_name = "medit";
    valid_selection = "21";
    expected_handler = postmaster;
    break;
  case SPEC_TEST_OWNER_OBJECT:
    owner_name = "oedit";
    valid_selection = "28";
    expected_handler = greyhawk_ship_object;
    break;
  case SPEC_TEST_OWNER_ROOM:
    owner_name = "redit";
    valid_selection = "29";
    expected_handler = greyhawk_ship_commands;
    break;
  case SPEC_TEST_OWNER_COUNT:
    spec_test_set_error(error, error_size, "invalid OLC owner scenario");
    return false;
  }

  fixture = spec_test_fixture_create_at(sandbox, error, error_size);
  if (fixture == NULL)
    return false;

  success = false;
  if (!spec_test_fixture_load_named_bindings(fixture, error, error_size))
    goto cleanup;
  if (snprintf(high_selection, sizeof(high_selection), "%d", get_spec_func_count() + 1) >=
      (int)sizeof(high_selection))
  {
    spec_test_set_error(error, error_size, "unable to format the invalid OLC selection");
    goto cleanup;
  }

  if (!spec_test_fixture_reset_olc(fixture, owner, bank) ||
      !spec_test_fixture_parse_olc(fixture, owner, valid_selection) ||
      spec_test_fixture_olc_handler(fixture, owner) != expected_handler ||
      spec_test_fixture_olc_changed(fixture) != 1)
  {
    snprintf(error, error_size, "%s did not apply its valid registry selection", owner_name);
    goto cleanup;
  }
  if (!spec_test_fixture_reset_olc(fixture, owner, bank) ||
      !spec_test_fixture_parse_olc(fixture, owner, "-1") ||
      spec_test_fixture_olc_handler(fixture, owner) != bank ||
      spec_test_fixture_olc_changed(fixture) != 0 ||
      strstr(spec_test_fixture_olc_output(fixture), "Invalid selection") == NULL)
  {
    snprintf(error, error_size, "%s did not reject its lower invalid bound", owner_name);
    goto cleanup;
  }
  if (!spec_test_fixture_reset_olc(fixture, owner, bank) ||
      !spec_test_fixture_parse_olc(fixture, owner, high_selection) ||
      spec_test_fixture_olc_handler(fixture, owner) != bank ||
      spec_test_fixture_olc_changed(fixture) != 0 ||
      strstr(spec_test_fixture_olc_output(fixture), "Invalid selection") == NULL)
  {
    snprintf(error, error_size, "%s did not reject its upper invalid bound", owner_name);
    goto cleanup;
  }
  if (!spec_test_fixture_reset_olc(fixture, owner, bank) ||
      !spec_test_fixture_parse_olc(fixture, owner, "Q") ||
      spec_test_fixture_olc_handler(fixture, owner) != bank ||
      spec_test_fixture_olc_changed(fixture) != 0)
  {
    snprintf(error, error_size, "%s changed its binding while quitting selection", owner_name);
    goto cleanup;
  }
  if (!spec_test_fixture_reset_olc(fixture, owner, bank) ||
      !spec_test_fixture_parse_olc(fixture, owner, "0") ||
      spec_test_fixture_olc_handler(fixture, owner) != NULL ||
      spec_test_fixture_olc_changed(fixture) != 1)
  {
    snprintf(error, error_size, "%s did not explicitly clear its binding", owner_name);
    goto cleanup;
  }

  success = true;

cleanup:
  cleanup_success = spec_test_fixture_destroy(fixture, cleanup_error, sizeof(cleanup_error));
  if (!cleanup_success)
  {
    if (success || error[0] == '\0')
      spec_test_set_error(error, error_size, cleanup_error);
    success = false;
  }
  return success;
}

static bool spec_test_medit_scenario(const char *sandbox, char *error, size_t error_size)
{
  return spec_test_olc_scenario(SPEC_TEST_OWNER_MOBILE, sandbox, error, error_size);
}

static bool spec_test_oedit_scenario(const char *sandbox, char *error, size_t error_size)
{
  return spec_test_olc_scenario(SPEC_TEST_OWNER_OBJECT, sandbox, error, error_size);
}

static bool spec_test_redit_scenario(const char *sandbox, char *error, size_t error_size)
{
  return spec_test_olc_scenario(SPEC_TEST_OWNER_ROOM, sandbox, error, error_size);
}

static bool spec_test_aborting_scenario(const char *sandbox, char *error, size_t error_size)
{
  struct spec_test_fixture *fixture;

  fixture = spec_test_fixture_create_at(sandbox, error, error_size);
  if (fixture == NULL)
    return false;

  _exit(3);
}

void Test_spec_registry_current_name_inventory(CuTest *tc)
{
  static const char *const expected_names[] = {"Bank",
                                               "Bazaar",
                                               "Bounty Missions",
                                               "Bulk Identify",
                                               "Buy Armor",
                                               "Buy Weapons",
                                               "Crafting Kit",
                                               "Crafting Quest",
                                               "Cryogenicist",
                                               "Dump",
                                               "Guild Guard",
                                               "Guild",
                                               "Guildmaster",
                                               "Hunts Master",
                                               "Identify Mob",
                                               "Janitor",
                                               "New Supply Orders",
                                               "Pet Object",
                                               "Pet Shop",
                                               "Player Shop",
                                               "Postmaster",
                                               "Practice Dummy",
                                               "Questmaster",
                                               "Receptionist",
                                               "Temple Healer",
                                               "Vampire Cloak",
                                               "Wizard Library",
                                               "Greyhawk Ship",
                                               "Greyhawk Ship Commands"};
  int expected_count;
  int index;

  expected_count = (int)(sizeof(expected_names) / sizeof(expected_names[0]));
  CuAssertIntEquals(tc, 29, expected_count);
  CuAssertIntEquals(tc, expected_count, get_spec_func_count());

  for (index = 0; index < expected_count; index++)
    CuAssertStrEquals(tc, expected_names[index], get_spec_func_name_by_index(index));
}

void Test_spec_registry_legacy_lookup_inputs(CuTest *tc)
{
  CuAssertTrue(tc, find_spec_func_by_name("pOsTmAsTeR") == postmaster);
  CuAssertTrue(tc, find_spec_func_by_name("gReYhAwK sHiP") == greyhawk_ship_object);
  CuAssertTrue(tc, find_spec_func_by_name(NULL) == NULL);
  CuAssertTrue(tc, find_spec_func_by_name("") == NULL);
  CuAssertTrue(tc, find_spec_func_by_name("Not A Registered Procedure") == NULL);
}

void Test_spec_registry_guild_alias_and_reverse_lookup(CuTest *tc)
{
  SPECIAL_DECL(*canonical_handler);
  SPECIAL_DECL(*alias_handler);

  canonical_handler = find_spec_func_by_name("Guild");
  alias_handler = find_spec_func_by_name("Guildmaster");

  CuAssertTrue(tc, canonical_handler == guild);
  CuAssertTrue(tc, alias_handler == guild);
  CuAssertTrue(tc, canonical_handler == alias_handler);
  CuAssertStrEquals(tc, "Guild", get_spec_func_name(canonical_handler));
}

void Test_spec_registry_legacy_accessor_boundaries(CuTest *tc)
{
  int count;

  count = get_spec_func_count();

  CuAssertTrue(tc, get_spec_func_name_by_index(-1) == NULL);
  CuAssertTrue(tc, get_spec_func_by_index(-1) == NULL);
  CuAssertTrue(tc, get_spec_func_name_by_index(count) == NULL);
  CuAssertTrue(tc, get_spec_func_by_index(count) == NULL);
  CuAssertStrEquals(tc, "Greyhawk Ship Commands", get_spec_func_name_by_index(count - 1));
  CuAssertTrue(tc, get_spec_func_by_index(count - 1) == greyhawk_ship_commands);
  CuAssertTrue(tc, get_spec_func_name(NULL) == NULL);
}

void Test_spec_world_binding_loaders_resolve_known_names(CuTest *tc)
{
  char error[SPEC_TEST_ERROR_SIZE];

  error[0] = '\0';
  CuAssert(tc, error, spec_test_run_isolated(spec_test_persistence_scenario, error, sizeof(error)));
}

void Test_spec_isolated_runner_cleans_abnormal_child_sandbox(CuTest *tc)
{
  char error[SPEC_TEST_ERROR_SIZE];
  char sandbox[PATH_MAX];
  bool completed;

  error[0] = '\0';
  sandbox[0] = '\0';
  completed = spec_test_run_isolated_with_path(spec_test_aborting_scenario, sandbox,
                                               sizeof(sandbox), error, sizeof(error));

  CuAssertTrue(tc, !completed);
  CuAssertPtrNotNull(tc, strstr(error, "before reporting a result"));
  CuAssertTrue(tc, sandbox[0] != '\0');
  errno = 0;
  CuAssertTrue(tc, access(sandbox, F_OK) != 0 && errno == ENOENT);
}

void Test_spec_world_binding_source_inventory(CuTest *tc)
{
  struct spec_binding_inventory inventory;
  char error[SPEC_TEST_ERROR_SIZE];
  bool scanned;

  memset(&inventory, 0, sizeof(inventory));
  error[0] = '\0';
  scanned = spec_test_scan_binding_directory("lib/world/mob", ".mob", SPEC_TEST_OWNER_MOBILE,
                                             &inventory, error, sizeof(error)) &&
            spec_test_scan_binding_directory("lib/world/obj", ".obj", SPEC_TEST_OWNER_OBJECT,
                                             &inventory, error, sizeof(error)) &&
            spec_test_scan_binding_directory("lib/world/wld", ".wld", SPEC_TEST_OWNER_ROOM,
                                             &inventory, error, sizeof(error));

  CuAssert(tc, error, scanned);
  CuAssertIntEquals(tc, 1, inventory.total[SPEC_TEST_OWNER_MOBILE]);
  CuAssertIntEquals(tc, 2, inventory.total[SPEC_TEST_OWNER_OBJECT]);
  CuAssertIntEquals(tc, 2, inventory.total[SPEC_TEST_OWNER_ROOM]);
  CuAssertIntEquals(tc, inventory.total[SPEC_TEST_OWNER_MOBILE],
                    inventory.expected[SPEC_TEST_OWNER_MOBILE]);
  CuAssertIntEquals(tc, inventory.total[SPEC_TEST_OWNER_OBJECT],
                    inventory.expected[SPEC_TEST_OWNER_OBJECT]);
  CuAssertIntEquals(tc, inventory.total[SPEC_TEST_OWNER_ROOM],
                    inventory.expected[SPEC_TEST_OWNER_ROOM]);
}

void Test_spec_medit_current_selection_behavior(CuTest *tc)
{
  char error[SPEC_TEST_ERROR_SIZE];

  error[0] = '\0';
  CuAssert(tc, error, spec_test_run_isolated(spec_test_medit_scenario, error, sizeof(error)));
}

void Test_spec_oedit_current_selection_behavior(CuTest *tc)
{
  char error[SPEC_TEST_ERROR_SIZE];

  error[0] = '\0';
  CuAssert(tc, error, spec_test_run_isolated(spec_test_oedit_scenario, error, sizeof(error)));
}

void Test_spec_redit_current_selection_behavior(CuTest *tc)
{
  char error[SPEC_TEST_ERROR_SIZE];

  error[0] = '\0';
  CuAssert(tc, error, spec_test_run_isolated(spec_test_redit_scenario, error, sizeof(error)));
}
