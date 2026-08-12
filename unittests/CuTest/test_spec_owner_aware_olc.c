#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"

#include "../../src/obj/vendor.h"
#include "../../src/olc/spec_menu.h"
#include "../../src/spec/spec_mobile_archetypes.h"
#include "../../src/spec/spec_mobiles.h"
#include "test_spec_fixtures.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

#define SPEC_OWNER_OLC_ERROR_SIZE 512

struct spec_owner_expected_view
{
  spec_owner_mask owner;
  enum spec_test_owner fixture_owner;
  const char *const *names;
  size_t count;
};

static const char *const spec_mobile_names[] = {
    "Bank",
    "Bounty Missions",
    "Bulk Identify",
    "Buy Armor",
    "Buy Weapons",
    "Cryogenicist",
    "Guild Guard",
    "Guild",
    "Hunts Master",
    "Identify Mob",
    "Janitor",
    "New Supply Orders",
    "Player Shop",
    "Postmaster",
    "Practice Dummy",
    "Questmaster",
    "Receptionist",
    "Temple Healer",
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
    "RoL Corpse Devourer",
    "RoL Poison Bite",
    "RoL Thief",
    "RoL Shadow Giant",
};

static const char *const spec_object_names[] = {
    "Bank",
    "Crafting Kit",
    "Pet Object",
    "Vampire Cloak",
    "Greyhawk Ship",
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
    "RoL Magic Pool",
};

static const char *const spec_room_names[] = {
    "Bazaar",         "Crafting Quest",         "Dump",           "Pet Shop",
    "Wizard Library", "Greyhawk Ship Commands", "RoL Guild Room", "RoL Auto Distributor",
};

static const struct spec_owner_expected_view spec_expected_views[] = {
    {SPEC_OWNER_MOBILE, SPEC_TEST_OWNER_MOBILE, spec_mobile_names,
     sizeof(spec_mobile_names) / sizeof(spec_mobile_names[0])},
    {SPEC_OWNER_OBJECT, SPEC_TEST_OWNER_OBJECT, spec_object_names,
     sizeof(spec_object_names) / sizeof(spec_object_names[0])},
    {SPEC_OWNER_ROOM, SPEC_TEST_OWNER_ROOM, spec_room_names,
     sizeof(spec_room_names) / sizeof(spec_room_names[0])},
};

static void spec_owner_set_error(char *error, size_t error_size, const char *message)
{
  if (error == NULL || error_size == 0)
    return;

  snprintf(error, error_size, "%s", message != NULL ? message : "owner-aware OLC test failed");
}

static bool spec_owner_finish_fixture(struct spec_test_fixture *fixture, bool success, char *error,
                                      size_t error_size)
{
  char cleanup_error[SPEC_OWNER_OLC_ERROR_SIZE];

  cleanup_error[0] = '\0';
  if (!spec_test_fixture_destroy(fixture, cleanup_error, sizeof(cleanup_error)))
  {
    if (success || error == NULL || error[0] == '\0')
      spec_owner_set_error(error, error_size, cleanup_error);
    return false;
  }

  return success;
}

static bool spec_owner_output_contains_all(const char *output, const char *const *fragments,
                                           size_t fragment_count, char *error, size_t error_size)
{
  size_t fragment_index;

  if (output == NULL)
  {
    spec_owner_set_error(error, error_size, "owner-aware menu produced null output");
    return false;
  }

  for (fragment_index = 0; fragment_index < fragment_count; fragment_index++)
  {
    if (strstr(output, fragments[fragment_index]) == NULL)
    {
      snprintf(error, error_size, "owner-aware menu omitted '%s'", fragments[fragment_index]);
      return false;
    }
  }

  return true;
}

static bool spec_owner_output_contains_none(const char *output, const char *const *fragments,
                                            size_t fragment_count, char *error, size_t error_size)
{
  size_t fragment_index;

  if (output == NULL)
  {
    spec_owner_set_error(error, error_size, "owner-aware menu produced null output");
    return false;
  }

  for (fragment_index = 0; fragment_index < fragment_count; fragment_index++)
  {
    if (strstr(output, fragments[fragment_index]) != NULL)
    {
      snprintf(error, error_size, "owner-aware menu exposed incompatible '%s'",
               fragments[fragment_index]);
      return false;
    }
  }

  return true;
}

static bool spec_owner_render_scenario(const char *sandbox, char *error, size_t error_size)
{
  static const char *const mobile_required[] = {
      "Mobile Prototypes",
      "Janitor [World]",
      "Picks up low-value trash",
      "mobile activity",
      "flags MOB_SPEC",
      "Practice Dummy",
      "mobile combat turn",
      "placement combat",
      "RoL Corpse Devourer [RoL Conversion]",
      "RoL Poison Bite [RoL Conversion]",
      "RoL Thief [RoL Conversion]",
  };
  static const char *const mobile_forbidden[] = {
      "Crafting Kit",
      "Pet Shop [Commerce]",
      "Greyhawk Ship Commands",
      "Guildmaster",
  };
  static const char *const object_required[] = {
      "Object Prototypes",       "Crafting Kit [Crafting]", "placement carried",
      "Pet Object [Companions]", "flags ITEM_AUTOPROC",     "Vampire Cloak [Equipment]",
      "placement equipped",
  };
  static const char *const object_forbidden[] = {
      "Janitor",
      "Pet Shop [Commerce]",
      "Wizard Library",
      "Guildmaster",
  };
  static const char *const room_required[] = {
      "Room Prototypes",
      "Bazaar [Commerce]",
      "Crafting Quest [Crafting]",
      "Wizard Library [Magic]",
      "Greyhawk Ship Commands [Vessels]",
      "RoL Guild Room [RoL Conversion]",
      "prerequisites: none",
  };
  static const char *const room_forbidden[] = {
      "MOB_SPEC",
      "ITEM_AUTOPROC",
      "Vampire Cloak",
      "Guildmaster",
  };
  struct spec_test_fixture *fixture;
  const struct spec_owner_expected_view *view;
  const char *const *required;
  const char *const *forbidden;
  const char *output;
  size_t forbidden_count;
  size_t owner_index;
  size_t required_count;

  fixture = spec_test_fixture_create_at(sandbox, error, error_size);
  if (fixture == NULL)
    return false;

  if (!spec_test_fixture_load_named_bindings(fixture, error, error_size))
    return spec_owner_finish_fixture(fixture, false, error, error_size);

  for (owner_index = 0; owner_index < sizeof(spec_expected_views) / sizeof(spec_expected_views[0]);
       owner_index++)
  {
    view = &spec_expected_views[owner_index];
    switch (view->owner)
    {
    case SPEC_OWNER_MOBILE:
      required = mobile_required;
      required_count = sizeof(mobile_required) / sizeof(mobile_required[0]);
      forbidden = mobile_forbidden;
      forbidden_count = sizeof(mobile_forbidden) / sizeof(mobile_forbidden[0]);
      break;
    case SPEC_OWNER_OBJECT:
      required = object_required;
      required_count = sizeof(object_required) / sizeof(object_required[0]);
      forbidden = object_forbidden;
      forbidden_count = sizeof(object_forbidden) / sizeof(object_forbidden[0]);
      break;
    case SPEC_OWNER_ROOM:
      required = room_required;
      required_count = sizeof(room_required) / sizeof(room_required[0]);
      forbidden = room_forbidden;
      forbidden_count = sizeof(room_forbidden) / sizeof(room_forbidden[0]);
      break;
    default:
      spec_owner_set_error(error, error_size, "unexpected owner-aware menu test view");
      return spec_owner_finish_fixture(fixture, false, error, error_size);
    }

    if (!spec_test_fixture_reset_olc(fixture, view->fixture_owner, bank) ||
        !spec_test_fixture_open_olc_menu(fixture, view->fixture_owner))
    {
      spec_owner_set_error(error, error_size, "unable to open a production owner-aware menu");
      return spec_owner_finish_fixture(fixture, false, error, error_size);
    }
    output = spec_test_fixture_olc_output(fixture);
    if (!spec_owner_output_contains_all(output, required, required_count, error, error_size) ||
        !spec_owner_output_contains_none(output, forbidden, forbidden_count, error, error_size) ||
        strstr(output, "Enter selection (0 to clear, Q to quit):") == NULL)
    {
      if (error[0] == '\0')
        spec_owner_set_error(error, error_size, "owner-aware menu output was incomplete");
      return spec_owner_finish_fixture(fixture, false, error, error_size);
    }
  }

  return spec_owner_finish_fixture(fixture, true, error, error_size);
}

void Test_spec_owner_olc_filtered_inventories_are_exact(CuTest *tc)
{
  const struct spec_definition *definition;
  const struct spec_owner_expected_view *view;
  size_t owner_index;
  size_t selection_index;

  for (owner_index = 0; owner_index < sizeof(spec_expected_views) / sizeof(spec_expected_views[0]);
       owner_index++)
  {
    view = &spec_expected_views[owner_index];
    CuAssertIntEquals(tc, (int)view->count, (int)spec_olc_menu_count(view->owner));
    CuAssertTrue(tc, spec_olc_menu_get(view->owner, INT_MIN) == NULL);
    CuAssertTrue(tc, spec_olc_menu_get(view->owner, -1) == NULL);
    CuAssertTrue(tc, spec_olc_menu_get(view->owner, (int)view->count) == NULL);
    CuAssertTrue(tc, spec_olc_menu_get(view->owner, INT_MAX) == NULL);

    for (selection_index = 0; selection_index < view->count; selection_index++)
    {
      definition = spec_olc_menu_get(view->owner, (int)selection_index);
      CuAssertPtrNotNull(tc, definition);
      if (definition == NULL)
        return;
      CuAssertStrEquals(tc, view->names[selection_index], definition->canonical_name);
      CuAssertStrEquals(tc, definition->canonical_name, definition->display_name);
      CuAssertIntEquals(tc, SPEC_BUILDER_VISIBLE, definition->builder_visibility);
      CuAssertTrue(tc, spec_definition_supports_owner(definition, view->owner));
      CuAssertTrue(tc, spec_definition_allows_binding(definition, SPEC_BINDING_SOURCE_WORLD));
      CuAssertPtrNotNull(tc, (void *)spec_definition_callback(definition));
      CuAssertPtrNotNull(tc, definition->category);
      CuAssertPtrNotNull(tc, definition->description);
    }
  }

  CuAssertIntEquals(tc, 0, (int)spec_olc_menu_count(SPEC_OWNER_NONE));
  CuAssertIntEquals(tc, 0, (int)spec_olc_menu_count(SPEC_OWNER_ALL));
  CuAssertTrue(tc, spec_olc_menu_get(SPEC_OWNER_NONE, 0) == NULL);
  CuAssertTrue(tc, spec_olc_menu_get(SPEC_OWNER_ALL, 0) == NULL);
}

void Test_spec_owner_olc_selection_parser_is_strict_and_bounded(CuTest *tc)
{
  const struct spec_definition *definition;
  enum spec_olc_selection_result result;

  definition = spec_registry_find_by_name("Bank");
  result = spec_olc_parse_selection(SPEC_OWNER_MOBILE, "0", &definition);
  CuAssertIntEquals(tc, SPEC_OLC_SELECTION_CLEAR, result);
  CuAssertTrue(tc, definition == NULL);

  result = spec_olc_parse_selection(SPEC_OWNER_MOBILE, " 1 ", &definition);
  CuAssertIntEquals(tc, SPEC_OLC_SELECTION_DEFINITION, result);
  CuAssertPtrNotNull(tc, definition);
  if (definition == NULL)
    return;
  CuAssertStrEquals(tc, "Bank", definition->canonical_name);

  result = spec_olc_parse_selection(SPEC_OWNER_MOBILE, "18", &definition);
  CuAssertIntEquals(tc, SPEC_OLC_SELECTION_DEFINITION, result);
  CuAssertPtrNotNull(tc, definition);
  if (definition == NULL)
    return;
  CuAssertStrEquals(tc, "Temple Healer", definition->canonical_name);
  result = spec_olc_parse_selection(SPEC_OWNER_OBJECT, "5", &definition);
  CuAssertIntEquals(tc, SPEC_OLC_SELECTION_DEFINITION, result);
  CuAssertPtrNotNull(tc, definition);
  if (definition == NULL)
    return;
  CuAssertStrEquals(tc, "Greyhawk Ship", definition->canonical_name);
  result = spec_olc_parse_selection(SPEC_OWNER_ROOM, "6", &definition);
  CuAssertIntEquals(tc, SPEC_OLC_SELECTION_DEFINITION, result);
  CuAssertPtrNotNull(tc, definition);
  if (definition == NULL)
    return;
  CuAssertStrEquals(tc, "Greyhawk Ship Commands", definition->canonical_name);

  result = spec_olc_parse_selection(SPEC_OWNER_MOBILE, "36", &definition);
  CuAssertIntEquals(tc, SPEC_OLC_SELECTION_INVALID, result);
  CuAssertTrue(tc, definition == NULL);
  CuAssertIntEquals(tc, SPEC_OLC_SELECTION_INVALID,
                    spec_olc_parse_selection(SPEC_OWNER_MOBILE, "", &definition));
  CuAssertTrue(tc, definition == NULL);
  CuAssertIntEquals(tc, SPEC_OLC_SELECTION_INVALID,
                    spec_olc_parse_selection(SPEC_OWNER_MOBILE, "   ", &definition));
  CuAssertTrue(tc, definition == NULL);
  CuAssertIntEquals(tc, SPEC_OLC_SELECTION_INVALID,
                    spec_olc_parse_selection(SPEC_OWNER_MOBILE, "-1", &definition));
  CuAssertIntEquals(tc, SPEC_OLC_SELECTION_INVALID,
                    spec_olc_parse_selection(SPEC_OWNER_MOBILE, "one", &definition));
  CuAssertIntEquals(tc, SPEC_OLC_SELECTION_INVALID,
                    spec_olc_parse_selection(SPEC_OWNER_MOBILE, "1x", &definition));
  CuAssertIntEquals(tc, SPEC_OLC_SELECTION_INVALID,
                    spec_olc_parse_selection(SPEC_OWNER_MOBILE,
                                             "999999999999999999999999999999999999", &definition));
  CuAssertIntEquals(tc, SPEC_OLC_SELECTION_INVALID,
                    spec_olc_parse_selection(SPEC_OWNER_MOBILE, NULL, &definition));
  CuAssertIntEquals(tc, SPEC_OLC_SELECTION_INVALID,
                    spec_olc_parse_selection(SPEC_OWNER_NONE, "1", &definition));
  CuAssertIntEquals(tc, SPEC_OLC_SELECTION_INVALID,
                    spec_olc_parse_selection(SPEC_OWNER_ALL, "1", &definition));
  CuAssertIntEquals(tc, SPEC_OLC_SELECTION_INVALID,
                    spec_olc_parse_selection(SPEC_OWNER_MOBILE, "1", NULL));
}

void Test_spec_owner_olc_menus_render_metadata_and_prerequisites(CuTest *tc)
{
  char error[SPEC_OWNER_OLC_ERROR_SIZE];

  error[0] = '\0';
  CuAssert(tc, error, spec_test_run_isolated(spec_owner_render_scenario, error, sizeof(error)));
}

void Test_spec_owner_olc_empty_view_is_explicit(CuTest *tc)
{
  struct spec_test_fixture *fixture;
  char error[SPEC_OWNER_OLC_ERROR_SIZE];
  const char *output;
  bool success;

  error[0] = '\0';
  fixture = spec_test_fixture_create(error, sizeof(error));
  CuAssertPtrNotNull(tc, fixture);
  if (fixture == NULL)
    return;

  success = spec_test_fixture_display_olc_menu(fixture, SPEC_OWNER_NONE);
  output = spec_test_fixture_olc_output(fixture);
  success = success && output != NULL && strstr(output, "Unsupported Prototypes") != NULL &&
            strstr(output, "No compatible builder-visible procedures are available.") != NULL &&
            strstr(output, "Enter selection (0 to clear, Q to quit):") != NULL &&
            strstr(output, "Bank [Services]") == NULL;
  if (!success)
    spec_owner_set_error(error, sizeof(error), "empty owner-aware menu was not explicit");
  success = spec_owner_finish_fixture(fixture, success, error, sizeof(error));

  CuAssert(tc, error, success);
}

static bool spec_owner_mapping_scenario(const char *sandbox, char *error, size_t error_size)
{
  struct spec_test_fixture *fixture;
  const struct spec_definition *definition;
  const struct spec_owner_expected_view *view;
  char selection[32];
  size_t owner_index;
  size_t selection_index;

  fixture = spec_test_fixture_create_at(sandbox, error, error_size);
  if (fixture == NULL)
    return false;

  if (!spec_test_fixture_load_named_bindings(fixture, error, error_size))
    return spec_owner_finish_fixture(fixture, false, error, error_size);

  for (owner_index = 0; owner_index < sizeof(spec_expected_views) / sizeof(spec_expected_views[0]);
       owner_index++)
  {
    view = &spec_expected_views[owner_index];
    for (selection_index = 0; selection_index < view->count; selection_index++)
    {
      definition = spec_olc_menu_get(view->owner, (int)selection_index);
      if (definition == NULL ||
          snprintf(selection, sizeof(selection), "%zu", selection_index + 1) >=
              (int)sizeof(selection) ||
          !spec_test_fixture_reset_olc(fixture, view->fixture_owner, bank) ||
          !spec_test_fixture_parse_olc(fixture, view->fixture_owner, selection) ||
          spec_test_fixture_olc_handler(fixture, view->fixture_owner) !=
              spec_definition_callback(definition) ||
          spec_test_fixture_olc_changed(fixture) != 1)
      {
        snprintf(error, error_size, "production editor failed filtered selection %zu for %s",
                 selection_index + 1, spec_owner_name(view->owner));
        return spec_owner_finish_fixture(fixture, false, error, error_size);
      }
    }
  }

  return spec_owner_finish_fixture(fixture, true, error, error_size);
}

void Test_spec_owner_olc_all_production_selections_map_exactly(CuTest *tc)
{
  char error[SPEC_OWNER_OLC_ERROR_SIZE];

  error[0] = '\0';
  CuAssert(tc, error, spec_test_run_isolated(spec_owner_mapping_scenario, error, sizeof(error)));
}

static bool spec_owner_control_scenario(const char *sandbox, char *error, size_t error_size)
{
  static const char *const invalid_inputs[] = {
      "-1", "   ", "not-a-number", "1x", "999999999999999999999999999999999999",
  };
  struct spec_test_fixture *fixture;
  const struct spec_owner_expected_view *view;
  char high_selection[32];
  size_t invalid_index;
  size_t owner_index;

  fixture = spec_test_fixture_create_at(sandbox, error, error_size);
  if (fixture == NULL)
    return false;

  if (!spec_test_fixture_load_named_bindings(fixture, error, error_size))
    return spec_owner_finish_fixture(fixture, false, error, error_size);

  for (owner_index = 0; owner_index < sizeof(spec_expected_views) / sizeof(spec_expected_views[0]);
       owner_index++)
  {
    view = &spec_expected_views[owner_index];
    if (snprintf(high_selection, sizeof(high_selection), "%zu", view->count + 1) >=
        (int)sizeof(high_selection))
      return spec_owner_finish_fixture(fixture, false, error, error_size);

    if (!spec_test_fixture_reset_olc(fixture, view->fixture_owner, bank) ||
        !spec_test_fixture_parse_olc(fixture, view->fixture_owner, "") ||
        spec_test_fixture_olc_handler(fixture, view->fixture_owner) != bank ||
        spec_test_fixture_olc_changed(fixture) != 0 ||
        spec_test_fixture_olc_output(fixture) == NULL ||
        *spec_test_fixture_olc_output(fixture) == '\0')
    {
      spec_owner_set_error(error, error_size, "production editor changed state on empty input");
      return spec_owner_finish_fixture(fixture, false, error, error_size);
    }

    for (invalid_index = 0; invalid_index <= sizeof(invalid_inputs) / sizeof(invalid_inputs[0]);
         invalid_index++)
    {
      const char *invalid_input;

      invalid_input = invalid_index == sizeof(invalid_inputs) / sizeof(invalid_inputs[0])
                          ? high_selection
                          : invalid_inputs[invalid_index];
      if (!spec_test_fixture_reset_olc(fixture, view->fixture_owner, bank) ||
          !spec_test_fixture_parse_olc(fixture, view->fixture_owner, invalid_input) ||
          spec_test_fixture_olc_handler(fixture, view->fixture_owner) != bank ||
          spec_test_fixture_olc_changed(fixture) != 0 ||
          strstr(spec_test_fixture_olc_output(fixture), "Invalid selection") == NULL)
      {
        spec_owner_set_error(error, error_size,
                             "production editor accepted an invalid filtered selection");
        return spec_owner_finish_fixture(fixture, false, error, error_size);
      }
    }

    if (!spec_test_fixture_reset_olc(fixture, view->fixture_owner, bank) ||
        !spec_test_fixture_parse_olc(fixture, view->fixture_owner, "Q") ||
        spec_test_fixture_olc_handler(fixture, view->fixture_owner) != bank ||
        spec_test_fixture_olc_changed(fixture) != 0)
    {
      spec_owner_set_error(error, error_size, "production editor changed state while quitting");
      return spec_owner_finish_fixture(fixture, false, error, error_size);
    }

    if (!spec_test_fixture_reset_olc(fixture, view->fixture_owner, bank) ||
        !spec_test_fixture_parse_olc(fixture, view->fixture_owner, "0") ||
        spec_test_fixture_olc_handler(fixture, view->fixture_owner) != NULL ||
        spec_test_fixture_olc_changed(fixture) != 1)
    {
      spec_owner_set_error(error, error_size, "production editor did not explicitly clear");
      return spec_owner_finish_fixture(fixture, false, error, error_size);
    }
  }

  return spec_owner_finish_fixture(fixture, true, error, error_size);
}

void Test_spec_owner_olc_control_paths_preserve_state(CuTest *tc)
{
  char error[SPEC_OWNER_OLC_ERROR_SIZE];

  error[0] = '\0';
  CuAssert(tc, error, spec_test_run_isolated(spec_owner_control_scenario, error, sizeof(error)));
}

static bool spec_owner_activation_scenario(const char *sandbox, char *error, size_t error_size)
{
  struct spec_test_fixture *fixture;
  bool success;

  fixture = spec_test_fixture_create_at(sandbox, error, error_size);
  if (fixture == NULL)
    return false;

  success = spec_test_fixture_load_named_bindings(fixture, error, error_size) &&
            !spec_test_fixture_activation_enabled(fixture, SPEC_TEST_OWNER_MOBILE) &&
            !spec_test_fixture_activation_enabled(fixture, SPEC_TEST_OWNER_OBJECT);
  if (success)
  {
    success = spec_test_fixture_reset_olc(fixture, SPEC_TEST_OWNER_MOBILE, bank) &&
              spec_test_fixture_parse_olc(fixture, SPEC_TEST_OWNER_MOBILE, "11") &&
              spec_test_fixture_olc_handler(fixture, SPEC_TEST_OWNER_MOBILE) == janitor &&
              !spec_test_fixture_activation_enabled(fixture, SPEC_TEST_OWNER_MOBILE);
  }
  if (success)
  {
    success = spec_test_fixture_reset_olc(fixture, SPEC_TEST_OWNER_MOBILE, bank) &&
              spec_test_fixture_parse_olc(fixture, SPEC_TEST_OWNER_MOBILE, "15") &&
              spec_test_fixture_olc_handler(fixture, SPEC_TEST_OWNER_MOBILE) == practice_dummy &&
              !spec_test_fixture_activation_enabled(fixture, SPEC_TEST_OWNER_MOBILE);
  }
  if (success)
  {
    success = spec_test_fixture_reset_olc(fixture, SPEC_TEST_OWNER_OBJECT, bank) &&
              spec_test_fixture_parse_olc(fixture, SPEC_TEST_OWNER_OBJECT, "3") &&
              spec_test_fixture_olc_handler(fixture, SPEC_TEST_OWNER_OBJECT) == bought_pet &&
              !spec_test_fixture_activation_enabled(fixture, SPEC_TEST_OWNER_OBJECT);
  }
  if (!success && error[0] == '\0')
    spec_owner_set_error(error, error_size, "selection mutated a runtime activation flag");

  return spec_owner_finish_fixture(fixture, success, error, error_size);
}

void Test_spec_owner_olc_selection_does_not_mutate_activation_flags(CuTest *tc)
{
  char error[SPEC_OWNER_OLC_ERROR_SIZE];

  error[0] = '\0';
  CuAssert(tc, error, spec_test_run_isolated(spec_owner_activation_scenario, error, sizeof(error)));
}
