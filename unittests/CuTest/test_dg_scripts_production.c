#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/dg_scripts.h"

#include <string.h>

void Test_dg_production_text_matching_helpers(CuTest *tc)
{
  char phrase_input[] = "\"two words\" remainder";
  char phrase[MAX_INPUT_LENGTH];
  char *remainder;

  CuAssertTrue(tc, is_substring("needle", "a needle in text"));
  CuAssertTrue(tc, !is_substring("missing", "a needle in text"));
  CuAssertTrue(tc, word_check("alpha beta gamma", "delta beta"));
  CuAssertTrue(tc, !word_check("alpha beta", "gamma delta"));

  remainder = one_phrase(phrase_input, phrase);
  CuAssertStrEquals(tc, "two words", phrase);
  CuAssertStrEquals(tc, " remainder", remainder);
}

void Test_dg_production_variable_lifecycle(CuTest *tc)
{
  struct trig_var_data *variables;

  variables = NULL;
  add_var(&variables, "coverage_name", "first", 17);
  CuAssertPtrNotNull(tc, variables);
  CuAssertStrEquals(tc, "coverage_name", variables->name);
  CuAssertStrEquals(tc, "first", variables->value);
  CuAssertIntEquals(tc, 17, (int)variables->context);

  add_var(&variables, "coverage_name", "updated", 17);
  CuAssertStrEquals(tc, "updated", variables->value);
  CuAssertTrue(tc, remove_var(&variables, "coverage_name"));
  CuAssertPtrEquals(tc, NULL, variables);

  free_varlist(variables);
}

void Test_dg_production_flag_name_matching(CuTest *tc)
{
  int flags[4];
  const char *names[] = {"zero", "one", "two", "three", "\n"};

  memset(flags, 0, sizeof(flags));
  SET_BIT_AR(flags, 2);

  CuAssertTrue(tc, check_flags_by_name_ar(flags, 4, "two", names));
  CuAssertTrue(tc, !check_flags_by_name_ar(flags, 4, "one", names));
}
