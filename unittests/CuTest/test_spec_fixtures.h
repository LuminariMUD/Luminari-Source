#ifndef TEST_SPEC_FIXTURES_H
#define TEST_SPEC_FIXTURES_H

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/spec/spec_registry.h"

enum spec_test_owner
{
  SPEC_TEST_OWNER_MOBILE = 0,
  SPEC_TEST_OWNER_OBJECT,
  SPEC_TEST_OWNER_ROOM,
  SPEC_TEST_OWNER_COUNT
};

struct spec_test_fixture;

typedef bool (*spec_test_isolated_scenario)(const char *sandbox, char *error, size_t error_size);

bool spec_test_run_isolated(spec_test_isolated_scenario scenario, char *error, size_t error_size);
bool spec_test_run_isolated_with_path(spec_test_isolated_scenario scenario, char *sandbox_result,
                                      size_t sandbox_result_size, char *error, size_t error_size);

struct spec_test_fixture *spec_test_fixture_create(char *error, size_t error_size);
struct spec_test_fixture *spec_test_fixture_create_at(const char *sandbox, char *error,
                                                      size_t error_size);
bool spec_test_fixture_destroy(struct spec_test_fixture *fixture, char *error, size_t error_size);
bool spec_test_cleanup_sandbox(const char *sandbox, char *error, size_t error_size);

bool spec_test_fixture_load_named_bindings(struct spec_test_fixture *fixture, char *error,
                                           size_t error_size);
SPECIAL_DECL(*spec_test_fixture_loaded_handler(const struct spec_test_fixture *fixture,
                                               enum spec_test_owner owner));

bool spec_test_fixture_save_named_bindings(struct spec_test_fixture *fixture, char *error,
                                           size_t error_size);
const char *spec_test_fixture_saved_text(const struct spec_test_fixture *fixture,
                                         enum spec_test_owner owner);

bool spec_test_fixture_reset_olc(struct spec_test_fixture *fixture, enum spec_test_owner owner,
                                 SPECIAL_DECL(*initial_handler));
bool spec_test_fixture_parse_olc(struct spec_test_fixture *fixture, enum spec_test_owner owner,
                                 const char *argument);
bool spec_test_fixture_open_olc_menu(struct spec_test_fixture *fixture, enum spec_test_owner owner);
bool spec_test_fixture_display_olc_menu(struct spec_test_fixture *fixture, spec_owner_mask owner);
SPECIAL_DECL(*spec_test_fixture_olc_handler(const struct spec_test_fixture *fixture,
                                            enum spec_test_owner owner));
int spec_test_fixture_olc_changed(const struct spec_test_fixture *fixture);
const char *spec_test_fixture_olc_output(const struct spec_test_fixture *fixture);
bool spec_test_fixture_activation_enabled(const struct spec_test_fixture *fixture,
                                          enum spec_test_owner owner);

#endif /* TEST_SPEC_FIXTURES_H */
