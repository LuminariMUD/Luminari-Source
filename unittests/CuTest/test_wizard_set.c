/* Contract tests for the staff "set" command family in src/act/act.wizard.set.c.
 *
 * The family splits parsing from execution: find_set_field() maps a typed
 * field name onto an index into set_fields[], and perform_set() acts on that
 * index. Everything here pins the parsing side and the table invariants that
 * make the mapping safe. Execution of individual fields is covered where the
 * behaviour lives; test_gameplay_e2e.c drives the class-level fields through
 * perform_set() end to end. */

#include "CuTest.h"

#include "conf.h"
#include "../../src/core/sysdep.h"
#include "../../src/core/structs.h"
#include "../../src/core/utils.h"
#include "../../src/core/interpreter.h"
#include "../../src/core/mudlim.h"
#include "../../src/act/act.h"

#include <string.h>

/* perform_set() compares the resolved mode against this literal to route the
 * "name" field through the rename machinery instead of writing the field. */
#define EXPECTED_SET_NAME_FIELD 34

void Test_wizard_set_resolves_exact_field_names(CuTest *tc)
{
  int count;

  count = set_field_count_for_test();
  CuAssertTrue(tc, count > 0);

  /* A field that does not resolve is a staff command that silently stops
   * working, so anchor a few spread across the table. */
  CuAssertTrue(tc, find_set_field_for_test("ac") >= 0);
  CuAssertTrue(tc, find_set_field_for_test("align") >= 0);
  CuAssertTrue(tc, find_set_field_for_test("name") >= 0);

  /* Resolution must stay inside the table; anything else indexes past the
   * sentinel in perform_set(). */
  CuAssertTrue(tc, find_set_field_for_test("ac") < count);
  CuAssertTrue(tc, find_set_field_for_test("name") < count);
}

void Test_wizard_set_rejects_unknown_and_empty_fields(CuTest *tc)
{
  /* find_set_field() walks to the "\n" sentinel. If the terminator were ever
   * dropped these would run off the end of the table instead of returning
   * -1, so they double as a guard on the sentinel itself. */
  CuAssertIntEquals(tc, -1, find_set_field_for_test("definitelynotafield"));
  CuAssertIntEquals(tc, -1, find_set_field_for_test(""));
  CuAssertIntEquals(tc, -1, find_set_field_for_test(NULL));
}

void Test_wizard_set_name_field_index_matches_the_hardcoded_constant(CuTest *tc)
{
  /* perform_set() special-cases the "name" field by comparing the resolved
   * mode against a literal index. Inserting a row above "name" would leave
   * the literal pointing at an unrelated field, so the rename path would
   * start firing on the wrong one. This is the regression guard for that. */
  CuAssertIntEquals(tc, EXPECTED_SET_NAME_FIELD, find_set_field_for_test("name"));
}

void Test_wizard_set_gates_every_field_behind_staff_level(CuTest *tc)
{
  int count;
  int mode;
  int level;

  count = set_field_count_for_test();
  CuAssertTrue(tc, count > 0);

  for (mode = 0; mode < count; mode++)
  {
    level = set_field_min_level_for_test(mode);
    /* Every settable field mutates live character state. A row that dropped
     * below builder level would expose that to ordinary players. */
    CuAssertTrue(tc, level >= LVL_BUILDER);
    CuAssertTrue(tc, level <= LVL_IMPL);
  }

  /* Out-of-range lookups are reported, not indexed. */
  CuAssertIntEquals(tc, -1, set_field_min_level_for_test(-1));
  CuAssertIntEquals(tc, -1, set_field_min_level_for_test(count));
}
