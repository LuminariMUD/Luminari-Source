#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/dgscript/dg_scripts.h"

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

void Test_dg_production_empty_expression_operands_are_safe(CuTest *tc)
{
  struct script_data script = {0};
  struct trig_data trigger = {0};
  struct trig_var_data *variable;
  char empty_right[] = "eval empty_right 1 ==";
  char empty_left[] = "eval empty_left == 1";
  bool found_empty_right = false;
  bool found_empty_left = false;

  trigger.name = "empty operand regression";

  process_eval(NULL, &script, &trigger, WLD_TRIGGER, empty_right);
  process_eval(NULL, &script, &trigger, WLD_TRIGGER, empty_left);

  for (variable = trigger.var_list; variable != NULL; variable = variable->next)
  {
    if (!strcmp(variable->name, "empty_right"))
    {
      found_empty_right = true;
      CuAssertStrEquals(tc, "0", variable->value);
    }
    else if (!strcmp(variable->name, "empty_left"))
    {
      found_empty_left = true;
      CuAssertStrEquals(tc, "0", variable->value);
    }
  }

  CuAssertTrue(tc, found_empty_right);
  CuAssertTrue(tc, found_empty_left);
  free_varlist(trigger.var_list);
}
