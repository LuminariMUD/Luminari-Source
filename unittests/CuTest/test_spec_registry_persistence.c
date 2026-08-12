#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"

#include "../../src/character/guild_services.h"
#include "../../src/comms/mail.h"
#include "../../src/obj/vendor.h"
#include "../../src/olc/spec_menu.h"
#include "../../src/spec/spec_registry.h"
#include "../../src/spec/spec_rol_conversion.h"
#include "../../src/spec/spec_rol_lavatubes.h"
#include "../../src/spec/spec_rol_pilot.h"
#include "../../src/spec/spec_rol_totem.h"
#include "../../src/spec/spec_rol_utility_objects.h"
#include "../../src/vessels/vessels_legacy.h"
#include "../../src/vessels/vessels_rol.h"
#include "test_spec_fixtures.h"

#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define SPEC_TEST_ERROR_SIZE 256

struct spec_binding_inventory
{
  int total[SPEC_TEST_OWNER_COUNT];
  int expected[SPEC_TEST_OWNER_COUNT];
};

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

static const char *spec_test_world_root(void)
{
  const char *root;

  root = getenv("LUMINARI_TEST_SPEC_WORLD_ROOT");
  return root != NULL && *root != '\0' ? root : NULL;
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
    spec_test_set_error(error, error_size, "unable to open a world inventory file");
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
    spec_test_set_error(error, error_size, "unable to read a complete world inventory file");
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

  if (spec_test_world_root() != NULL)
  {
    if (snprintf(directory_path, sizeof(directory_path), "%s/%s", spec_test_world_root(),
                 relative) >= (int)sizeof(directory_path))
    {
      spec_test_set_error(error, error_size, "world inventory fixture path is too long");
      return false;
    }
  }
  else if (snprintf(directory_path, sizeof(directory_path), "%s/lib/world/%s",
                    spec_test_source_root(), relative) >= (int)sizeof(directory_path))
  {
    spec_test_set_error(error, error_size, "development world directory path is too long");
    return false;
  }

  directory = opendir(directory_path);
  if (directory == NULL)
  {
    spec_test_set_error(error, error_size, "unable to open a world inventory directory");
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
      spec_test_set_error(error, error_size, "world inventory file path is too long");
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
    spec_test_set_error(error, error_size, "unable to enumerate a world inventory directory");
    success = false;
  }
  if (closedir(directory) != 0 && success)
  {
    spec_test_set_error(error, error_size, "unable to close a world inventory directory");
    success = false;
  }

  return success;
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
  spec_owner_mask owner_mask;
  char high_selection[32];
  bool cleanup_success;
  bool success;

  switch (owner)
  {
  case SPEC_TEST_OWNER_MOBILE:
    owner_name = "medit";
    valid_selection = "14";
    owner_mask = SPEC_OWNER_MOBILE;
    expected_handler = postmaster;
    break;
  case SPEC_TEST_OWNER_OBJECT:
    owner_name = "oedit";
    valid_selection = "5";
    owner_mask = SPEC_OWNER_OBJECT;
    expected_handler = greyhawk_ship_object;
    break;
  case SPEC_TEST_OWNER_ROOM:
    owner_name = "redit";
    valid_selection = "6";
    owner_mask = SPEC_OWNER_ROOM;
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
  if (snprintf(high_selection, sizeof(high_selection), "%zu",
               spec_olc_menu_count(owner_mask) + 1) >= (int)sizeof(high_selection))
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
                                               "Greyhawk Ship Commands",
                                               "breath_attack_fire",
                                               "breath_attack_acid",
                                               "breath_attack_lightning",
                                               "breath_weapon_fire",
                                               "breath_weapon_cold",
                                               "breath_weapon_acid",
                                               "breath_weapon_gas",
                                               "breath_weapon_lightning",
                                               "hulburg_beholder_major",
                                               "hulburg_beholder_minor",
                                               "money_changer",
                                               "plant_attacks_blindness",
                                               "plant_attacks_paralysis",
                                               "cemetary_black_blade",
                                               "cemetary_cloakMeteors",
                                               "cemetary_disruption",
                                               "cemetary_gleaming_blade",
                                               "cemetary_lightsaber",
                                               "cemetary_skeletal_hand",
                                               "flaming_tanthorian",
                                               "longsword_tanthorian",
                                               "murlynds_spoon",
                                               "muspel_bec_de_corbin",
                                               "muspel_crystal_scimitar",
                                               "muspel_dagger_whispers",
                                               "muspel_dragon_lance",
                                               "muspel_duergar_battlehammer",
                                               "muspel_recurve_bow",
                                               "muspel_spider_dagger",
                                               "obj_drain",
                                               "thorn_shield",
                                               "RoL Guild Room",
                                               "RoL Mage Guild Room",
                                               "RoL Thief Guild Room",
                                               "RoL Warrior Guild Room",
                                               "RoL Cleric Guild Room",
                                               "RoL Bard Guild Room",
                                               "RoL Waterdeep Guild Room",
                                               "RoL Corpse Devourer",
                                               "RoL Poison Bite",
                                               "RoL Thief",
                                               "RoL Bloodstone Portal",
                                               "RoL Portal Door",
                                               "RoL Travel Portal",
                                               "RoL Bloodstone Critter",
                                               "RoL Designated Follower",
                                               "RoL Fixed Bodyguard",
                                               "RoL Floating Pool",
                                               "RoL Item Blocker",
                                               "RoL Magic Pool",
                                               "RoL Banana",
                                               "RoL Auto Distributor",
                                               "RoL Command Sentinel",
                                               "RoL Toll Keeper",
                                               "RoL Shadow Giant",
                                               "RoL Guild Guard",
                                               "RoL Major Beholder",
                                               "RoL Monster Combat",
                                               "RoL Lich Energy Drain",
                                               "RoL Lich Rite",
                                               "RoL Undead Drain",
                                               "RoL Trade Bandit",
                                               "RoL Sister Knight",
                                               "RoL Shaman Totem",
                                               "RoL Totem Restorer",
                                               "RoL Ship",
                                               "RoL Ship Control",
                                               "RoL Ship Exit",
                                               "RoL Ship Lookout",
                                               "RoL Ship Navigator",
                                               "RoL Alert Caller",
                                               "RoL Yggdrasil Branch",
                                               "RoL Waterdeep Ambient",
                                               "RoL Waterdeep Peacekeeper",
                                               "RoL Weapon Proc",
                                               "RoL Source Periodic",
                                               "RoL Stateful Periodic",
                                               "RoL Lavatubes Mobile",
                                               "RoL Lavatubes Object",
                                               "RoL Lavatubes Room",
                                               "RoL Tarrasque Encounter",
                                               "RoL Utility Object",
                                               "RoL Utility Room",
                                               "RoL Scheduled Mobile"};
  int expected_count;
  int index;

  expected_count = (int)(sizeof(expected_names) / sizeof(expected_names[0]));
  CuAssertIntEquals(tc, 113, expected_count);
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
  CuAssertStrEquals(tc, "RoL Scheduled Mobile", get_spec_func_name_by_index(count - 1));
  CuAssertTrue(tc, get_spec_func_by_index(count - 1) == rol_scheduled_mobile);
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
  scanned = spec_test_scan_binding_directory("mob", ".mob", SPEC_TEST_OWNER_MOBILE, &inventory,
                                             error, sizeof(error)) &&
            spec_test_scan_binding_directory("obj", ".obj", SPEC_TEST_OWNER_OBJECT, &inventory,
                                             error, sizeof(error)) &&
            spec_test_scan_binding_directory("wld", ".wld", SPEC_TEST_OWNER_ROOM, &inventory, error,
                                             sizeof(error));

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
