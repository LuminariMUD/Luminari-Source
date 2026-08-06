#ifndef TEST_SPEC_FIXTURES_H
#define TEST_SPEC_FIXTURES_H

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"

enum spec_test_owner
{
  SPEC_TEST_OWNER_MOBILE = 0,
  SPEC_TEST_OWNER_OBJECT,
  SPEC_TEST_OWNER_ROOM,
  SPEC_TEST_OWNER_COUNT
};

struct spec_test_fixture;

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
SPECIAL_DECL(*spec_test_fixture_olc_handler(const struct spec_test_fixture *fixture,
                                            enum spec_test_owner owner));
int spec_test_fixture_olc_changed(const struct spec_test_fixture *fixture);
const char *spec_test_fixture_olc_output(const struct spec_test_fixture *fixture);

#endif /* TEST_SPEC_FIXTURES_H */
