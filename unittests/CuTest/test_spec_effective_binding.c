#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"

#include "../../src/db.h"
#include "../../src/interpreter.h"
#include "../../src/character/guild_services.h"
#include "../../src/comms/mail.h"
#include "../../src/obj/shop.h"
#include "../../src/obj/vendor.h"
#include "../../src/quest/quest.h"
#include "../../src/spec/spec_assign.h"
#include "../../src/spec/spec_effective_binding.h"
#include "../../src/spec/spec_registry.h"
#include "../../src/vessels/vessels_moving_rooms.h"
#include "test_spec_fixtures.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SPEC_EFFECTIVE_TEST_ERROR_SIZE 512
#define SPEC_EFFECTIVE_SOURCE_LIMIT (1024L * 1024L)

static bool spec_effective_test_contribute(
    struct spec_effective_binding **binding, spec_owner_mask owner, unsigned int vnum,
    spec_binding_source_mask source, const char *requested_name, const char *handler_name,
    spec_legacy_handler handler, bool wrapper, spec_legacy_handler secondary_handler,
    const char *secondary_name, const char *location, char *error, size_t error_size)
{
  struct spec_effective_contribution_input contribution;

  contribution.source = source;
  contribution.requested_name = requested_name;
  contribution.handler_name = handler_name;
  contribution.source_location = location;
  contribution.handler = handler;
  contribution.wrapper = wrapper;
  contribution.secondary_handler = secondary_handler;
  contribution.secondary_name = secondary_name;
  return spec_effective_binding_contribute(binding, owner, vnum, &contribution, error, error_size);
}

void TestSpecEffectiveBindingOutcomesAndDiagnostics(CuTest *tc)
{
  const struct spec_effective_contribution *first;
  const struct spec_effective_contribution *second;
  const struct spec_effective_contribution *third;
  const struct spec_effective_contribution *contribution;
  struct spec_effective_binding *binding;
  char diagnostic[MAX_STRING_LENGTH];
  char error[SPEC_EFFECTIVE_TEST_ERROR_SIZE];
  bool success;

  binding = NULL;
  success = spec_effective_test_contribute(
      &binding, SPEC_OWNER_MOBILE, 1201, SPEC_BINDING_SOURCE_WORLD, "Guildmaster", "Guild", guild,
      false, NULL, NULL, "mobile SpecProc field", error, sizeof(error));
  success = success && spec_effective_test_contribute(
                           &binding, SPEC_OWNER_MOBILE, 1201, SPEC_BINDING_SOURCE_LEGACY_ASSIGNMENT,
                           "postmaster", "Postmaster", postmaster, false, NULL, NULL,
                           "src/spec/spec_assign_mobiles.c:283", error, sizeof(error));
  success = success && spec_effective_test_contribute(
                           &binding, SPEC_OWNER_MOBILE, 1201, SPEC_BINDING_SOURCE_SHOP,
                           "shop_keeper", "shop_keeper", shop_keeper, true, postmaster,
                           "Postmaster", "shop #500", error, sizeof(error));
  success = success && spec_effective_test_contribute(
                           &binding, SPEC_OWNER_MOBILE, 1201, SPEC_BINDING_SOURCE_QUEST,
                           "questmaster", "Questmaster", questmaster, true, shop_keeper,
                           "shop_keeper", "quest #600", error, sizeof(error));

  if (!success || binding == NULL || binding->contribution_count != 4)
  {
    CuFail(tc, error[0] != '\0' ? error : "effective contribution setup failed");
    spec_effective_binding_free(&binding);
    return;
  }
  first = spec_effective_binding_get(binding, 0);
  second = spec_effective_binding_get(binding, 1);
  third = spec_effective_binding_get(binding, 2);
  contribution = spec_effective_binding_get(binding, 3);
  if (first == NULL || second == NULL || third == NULL || contribution == NULL)
  {
    CuFail(tc, "effective contribution chain is incomplete");
    spec_effective_binding_free(&binding);
    return;
  }

  CuAssertIntEquals(tc, 4, (int)binding->contribution_count);
  CuAssertIntEquals(tc, 3, (int)binding->collision_count);
  CuAssertTrue(tc, binding->effective_handler == questmaster);
  CuAssertIntEquals(tc, SPEC_EFFECTIVE_SELECTED, first->outcome);
  CuAssertIntEquals(tc, SPEC_EFFECTIVE_OVERRIDDEN, second->outcome);
  CuAssertIntEquals(tc, SPEC_EFFECTIVE_WRAPPED, third->outcome);
  CuAssertIntEquals(tc, SPEC_EFFECTIVE_WRAPPED, contribution->outcome);
  CuAssertTrue(tc, contribution->secondary_handler == shop_keeper);
  CuAssertStrEquals(tc, "shop_keeper", contribution->secondary_name);

  CuAssertTrue(tc, spec_effective_binding_format_contribution(binding, 2, false, diagnostic,
                                                              sizeof(diagnostic)));
  CuAssertTrue(tc,
               strstr(diagnostic, "mode=normal owner=mobile vnum=1201 step=3 source=shop") != NULL);
  CuAssertTrue(tc, strstr(diagnostic, "outcome=wrapped") != NULL);
  CuAssertTrue(tc, strstr(diagnostic, "secondary='Postmaster'") != NULL);
  CuAssertTrue(tc,
               spec_effective_binding_format_final(binding, false, diagnostic, sizeof(diagnostic)));
  CuAssertTrue(tc, strstr(diagnostic, "authored='Guildmaster'") != NULL);
  CuAssertTrue(tc, strstr(diagnostic, "chosen_source=quest chosen='Questmaster'") != NULL);
  CuAssertTrue(tc,
               spec_effective_binding_format_final(binding, true, diagnostic, sizeof(diagnostic)));
  CuAssertTrue(tc, strstr(diagnostic, "mode=no_specials") != NULL);

  spec_effective_binding_free(&binding);
  CuAssertTrue(tc, binding == NULL);
}

void TestSpecEffectiveBindingValidationCopyAndEscaping(CuTest *tc)
{
  struct spec_effective_binding *binding;
  struct spec_effective_binding *copy;
  const char *original_name;
  char diagnostic[MAX_STRING_LENGTH];
  char error[SPEC_EFFECTIVE_TEST_ERROR_SIZE];
  char long_name[READ_SIZE];
  char too_long[READ_SIZE + 1];

  binding = NULL;
  copy = NULL;
  CuAssertTrue(tc, !spec_effective_test_contribute(
                       &binding, SPEC_OWNER_OBJECT, 1402, SPEC_BINDING_SOURCE_WORLD, "bad\nname",
                       "Bank", bank, false, NULL, NULL, "object Z field", error, sizeof(error)));
  CuAssertTrue(tc, binding == NULL);
  CuAssertTrue(tc, strstr(error, "single-line") != NULL);

  CuAssertTrue(tc, !spec_effective_test_contribute(&binding, SPEC_OWNER_MOBILE, 1402,
                                                   SPEC_BINDING_SOURCE_SHOP, "shop_keeper",
                                                   "shop_keeper", shop_keeper, false, NULL, NULL,
                                                   "shop #1", error, sizeof(error)));
  CuAssertTrue(tc, binding == NULL);
  CuAssertTrue(tc, strstr(error, "wrapper and source") != NULL);
  CuAssertTrue(tc, !spec_effective_test_contribute(
                       &binding, SPEC_OWNER_MOBILE, 1402, SPEC_BINDING_SOURCE_SHOP, "shop_keeper",
                       NULL, NULL, true, NULL, NULL, "shop #1", error, sizeof(error)));
  CuAssertTrue(tc, binding == NULL);
  CuAssertTrue(tc, strstr(error, "wrapper handler") != NULL);

  memset(long_name, 'a', sizeof(long_name) - 1U);
  long_name[sizeof(long_name) - 1U] = '\0';
  CuAssertTrue(tc, spec_effective_test_contribute(
                       &binding, SPEC_OWNER_OBJECT, 1402, SPEC_BINDING_SOURCE_WORLD, long_name,
                       "Bank", bank, false, NULL, NULL, "object Z field", error, sizeof(error)));
  spec_effective_binding_free(&binding);

  memset(too_long, 'b', sizeof(too_long) - 1U);
  too_long[sizeof(too_long) - 1U] = '\0';
  CuAssertTrue(tc, !spec_effective_test_contribute(
                       &binding, SPEC_OWNER_OBJECT, 1402, SPEC_BINDING_SOURCE_WORLD, too_long,
                       "Bank", bank, false, NULL, NULL, "object Z field", error, sizeof(error)));
  CuAssertTrue(tc, binding == NULL);

  CuAssertTrue(tc, spec_effective_test_contribute(
                       &binding, SPEC_OWNER_OBJECT, 1402, SPEC_BINDING_SOURCE_WORLD,
                       "Missing Object Procedure", NULL, NULL, false, NULL, NULL, "object Z field",
                       error, sizeof(error)));
  if (binding == NULL)
  {
    CuFail(tc, "unresolved contribution was not retained");
    return;
  }
  CuAssertIntEquals(tc, SPEC_EFFECTIVE_UNRESOLVED, binding->first->outcome);
  CuAssertTrue(tc, spec_effective_test_contribute(
                       &binding, SPEC_OWNER_OBJECT, 1402, SPEC_BINDING_SOURCE_LEGACY_ASSIGNMENT,
                       "bank", "Bank", bank, false, NULL, NULL, "src/spec/spec_assign_objects.c:1",
                       error, sizeof(error)));
  CuAssertIntEquals(tc, SPEC_EFFECTIVE_SELECTED, binding->last->outcome);
  spec_effective_binding_free(&binding);

  CuAssertTrue(tc, spec_effective_test_contribute(&binding, SPEC_OWNER_OBJECT, 1402,
                                                  SPEC_BINDING_SOURCE_WORLD, "O'Brien\\Control",
                                                  "Bank", bank, false, NULL, NULL, "object Z field",
                                                  error, sizeof(error)));
  CuAssertTrue(tc, spec_effective_binding_copy(&copy, binding, error, sizeof(error)));
  if (binding == NULL || copy == NULL)
  {
    CuFail(tc, "effective binding copy setup failed");
    spec_effective_binding_free(&binding);
    spec_effective_binding_free(&copy);
    return;
  }
  CuAssertTrue(tc, copy != binding && copy->first != binding->first);
  original_name = binding->first->requested_name;
  CuAssertTrue(tc, copy->first->requested_name != original_name);
  CuAssertStrEquals(tc, original_name, copy->first->requested_name);
  spec_effective_binding_free(&binding);

  CuAssertTrue(tc,
               spec_effective_binding_format_final(copy, false, diagnostic, sizeof(diagnostic)));
  CuAssertTrue(tc, strstr(diagnostic, "authored='O\\'Brien\\\\Control'") != NULL);
  CuAssertTrue(tc, copy->effective_handler == bank);
  CuAssertTrue(tc, !spec_effective_binding_format_final(copy, false, diagnostic, 8));
  CuAssertTrue(tc, !spec_effective_binding_format_contribution(copy, 0, false, diagnostic, 8));
  CuAssertTrue(tc, spec_effective_test_contribute(
                       &copy, SPEC_OWNER_OBJECT, 1402, SPEC_BINDING_SOURCE_WORLD, "Bank", "Bank",
                       bank, false, NULL, NULL, "second object Z field", error, sizeof(error)));
  CuAssertTrue(tc,
               spec_effective_binding_format_final(copy, false, diagnostic, sizeof(diagnostic)));
  CuAssertTrue(tc, strstr(diagnostic, "authored='Bank'") != NULL);
  spec_effective_binding_free(&copy);
}

static bool spec_effective_loader_scenario(const char *sandbox, char *error, size_t error_size)
{
  const struct spec_effective_contribution *mobile_contribution;
  const struct spec_effective_contribution *object_contribution;
  const struct spec_effective_binding *mobile;
  const struct spec_effective_binding *object;
  const struct spec_effective_binding *room;
  struct spec_test_fixture *fixture;
  char cleanup_error[SPEC_EFFECTIVE_TEST_ERROR_SIZE];
  bool cleanup_success;
  bool success;

  fixture = spec_test_fixture_create_at(sandbox, error, error_size);
  if (fixture == NULL)
    return false;

  success = false;
  if (!spec_test_fixture_load_binding_names(fixture, "Guildmaster", "Missing Object Procedure",
                                            "Bazaar", error, error_size))
    goto cleanup;
  mobile = spec_test_fixture_loaded_effective_binding(fixture, SPEC_TEST_OWNER_MOBILE);
  object = spec_test_fixture_loaded_effective_binding(fixture, SPEC_TEST_OWNER_OBJECT);
  room = spec_test_fixture_loaded_effective_binding(fixture, SPEC_TEST_OWNER_ROOM);
  mobile_contribution = spec_effective_binding_get(mobile, 0);
  object_contribution = spec_effective_binding_get(object, 0);
  if (mobile == NULL || object == NULL || room == NULL || mobile->contribution_count != 1 ||
      object->contribution_count != 1 || room->contribution_count != 1 ||
      mobile_contribution == NULL || object_contribution == NULL ||
      mobile_contribution->requested_name == NULL || mobile_contribution->handler_name == NULL ||
      mobile_contribution->source != SPEC_BINDING_SOURCE_WORLD ||
      strcmp(mobile_contribution->requested_name, "Guildmaster") != 0 ||
      strcmp(mobile_contribution->handler_name, "Guild") != 0 ||
      mobile->effective_handler != guild ||
      object_contribution->outcome != SPEC_EFFECTIVE_UNRESOLVED ||
      object->effective_handler != NULL || object->effective_contribution != NULL ||
      room->effective_handler != find_spec_func_by_name("Bazaar"))
  {
    snprintf(error, error_size, "production loaders recorded unexpected effective provenance");
    goto cleanup;
  }
  success = true;

cleanup:
  cleanup_success = spec_test_fixture_destroy(fixture, cleanup_error, sizeof(cleanup_error));
  if (!cleanup_success)
  {
    if (success || error[0] == '\0')
      snprintf(error, error_size, "%s", cleanup_error);
    success = false;
  }
  return success;
}

void TestSpecEffectiveBindingProductionLoaders(CuTest *tc)
{
  char error[SPEC_EFFECTIVE_TEST_ERROR_SIZE];

  error[0] = '\0';
  CuAssertTrue(tc, spec_test_run_isolated(spec_effective_loader_scenario, error, sizeof(error)));
  CuAssertStrEquals(tc, "", error);
}

struct spec_effective_precedence_fixture
{
  struct char_data *saved_mob_proto;
  struct index_data *saved_mob_index;
  struct index_data *saved_obj_index;
  struct shop_data *saved_shop_index;
  struct aq_data *saved_aquest_table;
  struct command_info *saved_complete_cmd_info;
  struct zone_data *saved_zone_table;
  mob_rnum saved_top_of_mobt;
  obj_rnum saved_top_of_objt;
  qst_rnum saved_total_quests;
  zone_rnum saved_top_of_zone_table;
  int saved_top_shop;
  int saved_mini_mud;

  struct char_data mob_prototypes[2];
  struct index_data mob_indexes[2];
  struct index_data obj_indexes[1];
  struct shop_data shops[1];
  struct aq_data quests[1];
  struct command_info commands[6];
  struct zone_data zones[1];
  room_vnum shop_rooms[1];
};

static void spec_effective_precedence_begin(struct spec_effective_precedence_fixture *fixture)
{
  static const char *const command_names[] = {"say", "tell", "emote", "slap", "shake", "\n"};
  int index;

  memset(fixture, 0, sizeof(*fixture));
  fixture->saved_mob_proto = mob_proto;
  fixture->saved_mob_index = mob_index;
  fixture->saved_top_of_mobt = top_of_mobt;
  fixture->saved_obj_index = obj_index;
  fixture->saved_top_of_objt = top_of_objt;
  fixture->saved_shop_index = shop_index;
  fixture->saved_top_shop = top_shop;
  fixture->saved_aquest_table = aquest_table;
  fixture->saved_total_quests = total_quests;
  fixture->saved_complete_cmd_info = complete_cmd_info;
  fixture->saved_zone_table = zone_table;
  fixture->saved_top_of_zone_table = top_of_zone_table;
  fixture->saved_mini_mud = mini_mud;

  fixture->mob_indexes[0].vnum = 1;
  fixture->mob_indexes[1].vnum = 1201;
  fixture->obj_indexes[0].vnum = 1;
  fixture->shop_rooms[0] = NOWHERE;
  fixture->shops[0].vnum = 500;
  fixture->shops[0].keeper = 1;
  fixture->shops[0].in_room = fixture->shop_rooms;
  fixture->quests[0].vnum = 600;
  fixture->quests[0].name = "Effective precedence quest";
  fixture->quests[0].qm = 1201;
  fixture->zones[0].number = 150;
  fixture->zones[0].bot = 900000;
  fixture->zones[0].top = 900099;
  for (index = 0; index < 6; index++)
  {
    fixture->commands[index].command = command_names[index];
    fixture->commands[index].sort_as = command_names[index];
  }

  mob_proto = fixture->mob_prototypes;
  mob_index = fixture->mob_indexes;
  top_of_mobt = 1;
  obj_index = fixture->obj_indexes;
  top_of_objt = 0;
  shop_index = fixture->shops;
  top_shop = 0;
  aquest_table = fixture->quests;
  total_quests = 1;
  complete_cmd_info = fixture->commands;
  zone_table = fixture->zones;
  top_of_zone_table = 0;
  mini_mud = 1;
}

static void spec_effective_precedence_end(struct spec_effective_precedence_fixture *fixture)
{
  spec_effective_binding_free(&fixture->mob_indexes[0].effective_binding);
  spec_effective_binding_free(&fixture->mob_indexes[1].effective_binding);
  spec_effective_binding_free(&fixture->obj_indexes[0].effective_binding);
  mob_proto = fixture->saved_mob_proto;
  mob_index = fixture->saved_mob_index;
  top_of_mobt = fixture->saved_top_of_mobt;
  obj_index = fixture->saved_obj_index;
  top_of_objt = fixture->saved_top_of_objt;
  shop_index = fixture->saved_shop_index;
  top_shop = fixture->saved_top_shop;
  aquest_table = fixture->saved_aquest_table;
  total_quests = fixture->saved_total_quests;
  complete_cmd_info = fixture->saved_complete_cmd_info;
  zone_table = fixture->saved_zone_table;
  top_of_zone_table = fixture->saved_top_of_zone_table;
  mini_mud = fixture->saved_mini_mud;
}

void TestSpecEffectiveBindingProductionPrecedenceAndSecondaries(CuTest *tc)
{
  const struct spec_effective_contribution *legacy_contribution;
  const struct spec_effective_contribution *shop_contribution;
  const struct spec_effective_contribution *quest_contribution;
  struct spec_effective_precedence_fixture fixture;
  struct spec_effective_binding *binding;
  char error[SPEC_EFFECTIVE_TEST_ERROR_SIZE];
  bool matches;

  spec_effective_precedence_begin(&fixture);
  binding = NULL;
  matches = spec_effective_test_contribute(
      &binding, SPEC_OWNER_MOBILE, 1201, SPEC_BINDING_SOURCE_WORLD, "Guildmaster", "Guild", guild,
      false, NULL, NULL, "mobile SpecProc field", error, sizeof(error));
  fixture.mob_indexes[1].effective_binding = binding;
  fixture.mob_indexes[1].func = guild;

  assign_mobiles();
  assign_the_shopkeepers();
  assign_the_quests();

  binding = fixture.mob_indexes[1].effective_binding;
  legacy_contribution = spec_effective_binding_get(binding, 1);
  shop_contribution = spec_effective_binding_get(binding, 2);
  quest_contribution = spec_effective_binding_get(binding, 3);
  matches =
      matches && binding != NULL && binding->contribution_count == 4 &&
      binding->collision_count == 3 && fixture.mob_indexes[1].func == questmaster &&
      fixture.shops[0].func == postmaster && fixture.quests[0].func == shop_keeper &&
      legacy_contribution != NULL && shop_contribution != NULL && quest_contribution != NULL &&
      legacy_contribution->requested_name != NULL && legacy_contribution->source_location != NULL &&
      legacy_contribution->source == SPEC_BINDING_SOURCE_LEGACY_ASSIGNMENT &&
      strcmp(legacy_contribution->requested_name, "postmaster") == 0 &&
      strncmp(legacy_contribution->source_location,
              "src/spec/spec_assign_mobiles.c:", strlen("src/spec/spec_assign_mobiles.c:")) == 0 &&
      shop_contribution->source == SPEC_BINDING_SOURCE_SHOP &&
      shop_contribution->secondary_handler == postmaster &&
      shop_contribution->secondary_name != NULL &&
      strcmp(shop_contribution->secondary_name, "Postmaster") == 0 &&
      quest_contribution->source == SPEC_BINDING_SOURCE_QUEST &&
      quest_contribution->secondary_handler == shop_keeper &&
      quest_contribution->secondary_name != NULL &&
      strcmp(quest_contribution->secondary_name, "shop_keeper") == 0;

  spec_effective_precedence_end(&fixture);
  CuAssert(tc, error, matches);
}

static bool spec_effective_conflict_loader_scenario(const char *sandbox, char *error,
                                                    size_t error_size)
{
  struct spec_test_fixture *fixture;
  char cleanup_error[SPEC_EFFECTIVE_TEST_ERROR_SIZE];
  bool success;

  fixture = spec_test_fixture_create_at(sandbox, error, error_size);
  if (fixture == NULL)
    return false;

  success = spec_test_fixture_expect_room_load_rejection(fixture, true) &&
            spec_test_fixture_expect_room_load_rejection(fixture, false);
  if (!success)
    snprintf(error, error_size, "production room loader accepted an M plus Z conflict");
  if (!spec_test_fixture_destroy(fixture, cleanup_error, sizeof(cleanup_error)))
  {
    if (success || error[0] == '\0')
      snprintf(error, error_size, "%s", cleanup_error);
    success = false;
  }
  return success;
}

void TestSpecEffectiveBindingRejectsBothRoomLoadOrders(CuTest *tc)
{
  char error[SPEC_EFFECTIVE_TEST_ERROR_SIZE];

  error[0] = '\0';
  CuAssertTrue(
      tc, spec_test_run_isolated(spec_effective_conflict_loader_scenario, error, sizeof(error)));
  CuAssertStrEquals(tc, "", error);
}

static bool spec_effective_olc_conflict_scenario(const char *sandbox, char *error,
                                                 size_t error_size)
{
  struct spec_test_fixture *fixture;
  char cleanup_error[SPEC_EFFECTIVE_TEST_ERROR_SIZE];
  bool success;

  fixture = spec_test_fixture_create_at(sandbox, error, error_size);
  if (fixture == NULL)
    return false;

  success = false;
  if (!spec_test_fixture_load_named_bindings(fixture, error, error_size) ||
      !spec_test_fixture_discard_loaded_binding(fixture, SPEC_TEST_OWNER_ROOM) ||
      !spec_test_fixture_set_loaded_room_mover(fixture) ||
      !spec_test_fixture_setup_existing_olc(fixture, SPEC_TEST_OWNER_ROOM, error, error_size) ||
      !spec_test_fixture_open_olc_menu(fixture, SPEC_TEST_OWNER_ROOM) ||
      strstr(spec_test_fixture_olc_output(fixture), "Moving rooms own the room callback slot") ==
          NULL ||
      !spec_test_fixture_force_room_olc_binding(fixture, "Bazaar", error, error_size) ||
      !spec_test_fixture_save_current_olc(fixture, SPEC_TEST_OWNER_ROOM) ||
      strstr(spec_test_fixture_olc_output(fixture), "Save rejected") == NULL ||
      spec_test_fixture_loaded_binding(fixture, SPEC_TEST_OWNER_ROOM) != NULL ||
      spec_test_fixture_loaded_handler(fixture, SPEC_TEST_OWNER_ROOM) != moving_rooms ||
      !spec_test_fixture_set_loaded_handler(fixture, SPEC_TEST_OWNER_ROOM,
                                            find_spec_func_by_name("Bazaar")) ||
      spec_test_fixture_save_room(fixture, error, error_size) != FALSE)
  {
    if (error[0] == '\0')
      snprintf(error, error_size, "OLC or room writer accepted moving-room SpecProc ownership");
    goto cleanup;
  }
  success = true;

cleanup:
  if (!spec_test_fixture_destroy(fixture, cleanup_error, sizeof(cleanup_error)))
  {
    if (success || error[0] == '\0')
      snprintf(error, error_size, "%s", cleanup_error);
    success = false;
  }
  return success;
}

void TestSpecEffectiveBindingRejectsMovingRoomOlcAndWriter(CuTest *tc)
{
  char error[SPEC_EFFECTIVE_TEST_ERROR_SIZE];

  error[0] = '\0';
  CuAssertTrue(tc,
               spec_test_run_isolated(spec_effective_olc_conflict_scenario, error, sizeof(error)));
  CuAssertStrEquals(tc, "", error);
}

static bool spec_effective_read_source(const char *relative_path, char **text)
{
  const char *root;
  FILE *file;
  char path[PATH_MAX];
  char *buffer;
  long length;
  size_t bytes_read;

  *text = NULL;
  root = getenv("LUMINARI_TEST_ROOT");
  if (root == NULL || *root == '\0')
    root = ".";
  if (snprintf(path, sizeof(path), "%s/%s", root, relative_path) >= (int)sizeof(path))
    return false;

  file = fopen(path, "rb");
  if (file == NULL)
    return false;
  if (fseek(file, 0, SEEK_END) != 0)
  {
    fclose(file);
    return false;
  }
  length = ftell(file);
  if (length < 0 || length > SPEC_EFFECTIVE_SOURCE_LIMIT || fseek(file, 0, SEEK_SET) != 0)
  {
    fclose(file);
    return false;
  }
  buffer = malloc((size_t)length + 1U);
  if (buffer == NULL)
  {
    fclose(file);
    return false;
  }
  bytes_read = fread(buffer, 1, (size_t)length, file);
  if (bytes_read != (size_t)length || ferror(file) != 0 || fclose(file) != 0)
  {
    free(buffer);
    return false;
  }
  buffer[bytes_read] = '\0';
  *text = buffer;
  return true;
}

static size_t spec_effective_count_text(const char *source, const char *needle)
{
  const char *cursor;
  size_t count;
  size_t needle_length;

  if (source == NULL || needle == NULL || *needle == '\0')
    return 0;

  count = 0;
  needle_length = strlen(needle);
  for (cursor = source; (cursor = strstr(cursor, needle)) != NULL; cursor += needle_length)
    count++;
  return count;
}

void TestSpecAssignmentModulesExposeNarrowBoundaries(CuTest *tc)
{
  static const char *const assignment_sources[] = {
      "src/spec/spec_assign.c",
      "src/spec/spec_assign_mobiles.c",
      "src/spec/spec_assign_objects.c",
      "src/spec/spec_assign_rooms.c",
  };
  static const char removed_assignment_path[] = "src/spec_"
                                                "assign.c";
  static const char removed_umbrella_path[] = "src/spec_"
                                              "procs.h";
  char *shared;
  char *mobiles;
  char *objects;
  char *rooms;
  char *assignment_header;
  char *registry_header;
  char *database;
  char *automake;
  char *cmake;
  char *removed;
  size_t index;
  size_t assignment_tokens;
  bool loaded;
  bool matches;

  shared = NULL;
  mobiles = NULL;
  objects = NULL;
  rooms = NULL;
  assignment_header = NULL;
  registry_header = NULL;
  database = NULL;
  automake = NULL;
  cmake = NULL;
  removed = NULL;

  loaded = spec_effective_read_source(assignment_sources[0], &shared) &&
           spec_effective_read_source(assignment_sources[1], &mobiles) &&
           spec_effective_read_source(assignment_sources[2], &objects) &&
           spec_effective_read_source(assignment_sources[3], &rooms) &&
           spec_effective_read_source("src/spec/spec_assign.h", &assignment_header) &&
           spec_effective_read_source("src/spec/spec_registry.h", &registry_header) &&
           spec_effective_read_source("src/db.c", &database) &&
           spec_effective_read_source("Makefile.am", &automake) &&
           spec_effective_read_source("CMakeLists.txt", &cmake);

  assignment_tokens = spec_effective_count_text(mobiles, "ASSIGNMOB(") +
                      spec_effective_count_text(mobiles, "ASSIGNOBJ(") +
                      spec_effective_count_text(objects, "ASSIGNOBJ(") +
                      spec_effective_count_text(rooms, "ASSIGNROOM(");
  matches = loaded && strstr(shared, "void spec_assign_mobile(") != NULL &&
            strstr(shared, "void spec_assign_object(") != NULL &&
            strstr(shared, "void spec_assign_room(") != NULL &&
            strstr(shared, "void assign_mobiles(") == NULL &&
            strstr(mobiles, "void assign_mobiles(") != NULL &&
            strstr(mobiles, "src/spec/spec_assign_mobiles.c:") != NULL &&
            strstr(objects, "void spec_assign_table_boot_validate(") != NULL &&
            strstr(objects, "void assign_objects(") != NULL &&
            strstr(objects, "src/spec/spec_assign_objects.c:") != NULL &&
            strstr(rooms, "void assign_rooms(") != NULL &&
            strstr(rooms, "src/spec/spec_assign_rooms.c:") != NULL && assignment_tokens == 697 &&
            strstr(assignment_header, "spec_assign_table_boot_validate") != NULL &&
            strstr(assignment_header, "spec_registry") == NULL &&
            strstr(registry_header, "get_spec_func_name(spec_legacy_handler func)") != NULL &&
            strstr(registry_header, "find_spec_func_by_name(const char *name)") != NULL &&
            strstr(database, "#include \"spec/spec_assign.h\"") != NULL &&
            strstr(database, "#include \"spec/spec_registry.h\"") != NULL;

  for (index = 0; loaded && index < sizeof(assignment_sources) / sizeof(assignment_sources[0]);
       index++)
    matches = matches && strstr(automake, assignment_sources[index]) != NULL &&
              strstr(cmake, assignment_sources[index]) != NULL;

  matches = matches && !spec_effective_read_source(removed_assignment_path, &removed) &&
            removed == NULL && !spec_effective_read_source(removed_umbrella_path, &removed) &&
            removed == NULL && strstr(automake, removed_assignment_path) == NULL &&
            strstr(cmake, removed_assignment_path) == NULL;

  free(shared);
  free(mobiles);
  free(objects);
  free(rooms);
  free(assignment_header);
  free(registry_header);
  free(database);
  free(automake);
  free(cmake);
  free(removed);

  CuAssertTrue(tc, loaded);
  CuAssertTrue(tc, matches);
}

void TestSpecEffectiveBindingReportFollowsNoSpecialsAssignmentGate(CuTest *tc)
{
  char *source;
  const char *assignment;
  const char *guard_end_and_report;
  const char *spell_levels;
  bool loaded;
  bool ordered;

  source = NULL;
  loaded = spec_effective_read_source("src/db.c", &source);
  assignment = loaded ? strstr(source, "assign_mobiles();") : NULL;
  guard_end_and_report =
      loaded ? strstr(source, "assign_the_quests();\n  }\n\n  report_effective_spec_bindings();")
             : NULL;
  spell_levels = loaded ? strstr(source, "init_spell_levels();") : NULL;
  ordered = assignment != NULL && guard_end_and_report != NULL && spell_levels != NULL &&
            assignment < guard_end_and_report && guard_end_and_report < spell_levels;
  free(source);

  CuAssertTrue(tc, loaded);
  CuAssertTrue(tc, ordered);
}
