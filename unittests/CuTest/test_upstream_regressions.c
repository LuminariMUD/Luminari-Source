#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/dgscript/dg_olc.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

void Test_upstream_random_generator_sequence(CuTest *tc)
{
  unsigned long first, second;

  circle_srandom(1);
  CuAssertTrue(tc, circle_random() == 16807UL);
  CuAssertTrue(tc, circle_random() == 282475249UL);
  CuAssertTrue(tc, circle_random() == 1622650073UL);

  circle_srandom(42);
  first = circle_random();
  second = circle_random();
  circle_srandom(42);
  CuAssertTrue(tc, circle_random() == first);
  CuAssertTrue(tc, circle_random() == second);

  circle_srandom((unsigned long)time(NULL));
}

void Test_clan_armor_uses_builder_value_two(CuTest *tc)
{
  struct obj_data obj = {0};

  GET_OBJ_VAL(&obj, 1) = 42;
  GET_OBJ_VAL(&obj, 2) = 7;

  CuAssertIntEquals(tc, 42, GET_OBJ_CLAN(&obj));
}

void Test_upstream_random_range_and_dice(CuTest *tc)
{
  int i, value;

  circle_srandom(12345);
  for (i = 0; i < 200; i++)
  {
    value = rand_number(1, 10);
    CuAssertTrue(tc, value >= 1 && value <= 10);
  }
  CuAssertIntEquals(tc, 7, rand_number(7, 7));

  value = rand_number(10, 1);
  CuAssertTrue(tc, value >= 1 && value <= 10);

  CuAssertIntEquals(tc, 0, dice(0, 6));
  CuAssertIntEquals(tc, 0, dice(3, 0));
  CuAssertIntEquals(tc, 1, dice(1, 1));
  for (i = 0; i < 200; i++)
  {
    value = dice(2, 6);
    CuAssertTrue(tc, value >= 2 && value <= 12);
  }

  circle_srandom((unsigned long)time(NULL));
}

void Test_upstream_string_comparison_and_pruning(CuTest *tc)
{
  char crlf[] = "hello\r\n";
  char lf[] = "hello\n";
  char repeated[] = "hi\r\n\r\n";

  prune_crlf(crlf);
  prune_crlf(lf);
  prune_crlf(repeated);
  CuAssertStrEquals(tc, "hello", crlf);
  CuAssertStrEquals(tc, "hello", lf);
  CuAssertStrEquals(tc, "hi", repeated);

  CuAssertIntEquals(tc, 0, str_cmp("Hello", "hello"));
  CuAssertTrue(tc, str_cmp("a", "b") < 0);
  CuAssertTrue(tc, str_cmp("b", "a") > 0);
  CuAssertIntEquals(tc, 0, strn_cmp("hello!", "hellox", 5));
  CuAssertTrue(tc, strn_cmp("abc", "xyz", 3) != 0);
}

void Test_upstream_type_and_bit_formatting(CuTest *tc)
{
  const char *names[] = {"FLAG_A", "FLAG_B", "\n"};
  char result[256];

  sprintbit(0, names, result, sizeof(result));
  CuAssertStrEquals(tc, "None ", result);
  sprintbit(1, names, result, sizeof(result));
  CuAssertStrEquals(tc, "FLAG_A ", result);
  sprintbit(3, names, result, sizeof(result));
  CuAssertStrEquals(tc, "FLAG_A FLAG_B ", result);
  sprintbit(4, names, result, sizeof(result));
  CuAssertStrEquals(tc, "UNDEFINED ", result);

  sprinttype(0, names, result, sizeof(result));
  CuAssertStrEquals(tc, "FLAG_A", result);
  sprinttype(1, names, result, sizeof(result));
  CuAssertStrEquals(tc, "FLAG_B", result);
  sprinttype(5, names, result, sizeof(result));
  CuAssertStrEquals(tc, "UNDEFINED", result);
}

void Test_upstream_text_measurement_helpers(CuTest *tc)
{
  CuAssertIntEquals(tc, 0, levenshtein_distance("hello", "hello"));
  CuAssertIntEquals(tc, 5, levenshtein_distance("", "hello"));
  CuAssertIntEquals(tc, 5, levenshtein_distance("hello", ""));
  CuAssertIntEquals(tc, 1, levenshtein_distance("abc", "abcd"));
  CuAssertIntEquals(tc, 1, levenshtein_distance("abcd", "abc"));
  CuAssertIntEquals(tc, 1, levenshtein_distance("abc", "axc"));

  CuAssertIntEquals(tc, 0, count_color_chars("hello"));
  CuAssertIntEquals(tc, 2, count_color_chars("\tRhello"));
  CuAssertIntEquals(tc, 1, count_color_chars("\t\t"));
  CuAssertIntEquals(tc, 5, count_non_protocol_chars("hello"));
  CuAssertIntEquals(tc, 5, count_non_protocol_chars("\r\nhello"));
  CuAssertIntEquals(tc, 2, count_non_protocol_chars("@[bold]hi"));
}

void Test_upstream_index_helpers(CuTest *tc)
{
  CuAssertIntEquals(tc, 42, (int)atoidx("42"));
  CuAssertIntEquals(tc, 0, (int)atoidx("0"));
  CuAssertIntEquals(tc, (int)NOWHERE, (int)atoidx("-1"));
  CuAssertIntEquals(tc, (int)NOWHERE, (int)atoidx("99999999999999999999999999999999"));
}

void Test_upstream_new_affect_initializes_all_fields(CuTest *tc)
{
  struct affected_type affect;
  int i;

  memset(&affect, 0xA5, sizeof(affect));
  new_affect(&affect);

  CuAssertIntEquals(tc, 0, affect.spell);
  CuAssertIntEquals(tc, 0, affect.duration);
  CuAssertIntEquals(tc, 0, affect.modifier);
  CuAssertIntEquals(tc, APPLY_NONE, affect.location);
  CuAssertIntEquals(tc, BONUS_TYPE_ENHANCEMENT, affect.bonus_type);
  CuAssertIntEquals(tc, 0, affect.specific);
  CuAssertTrue(tc, affect.next == NULL);
  for (i = 0; i < AF_ARRAY_MAX; i++)
  {
    CuAssertIntEquals(tc, 0, affect.bitvector[i]);
    CuAssertIntEquals(tc, 0, affect.bitvector2[i]);
  }
}

void Test_upstream_time_helpers(CuTest *tc)
{
  struct time_info_data *elapsed;
  time_t base = 1000000;

  elapsed = real_time_passed(base + 3 * SECS_PER_REAL_HOUR, base);
  CuAssertIntEquals(tc, 3, elapsed->hours);
  CuAssertIntEquals(tc, 0, elapsed->day);
  elapsed = real_time_passed(base + 2 * SECS_PER_REAL_DAY + SECS_PER_REAL_HOUR, base);
  CuAssertIntEquals(tc, 1, elapsed->hours);
  CuAssertIntEquals(tc, 2, elapsed->day);

  elapsed = mud_time_passed(base + 2 * SECS_PER_MUD_HOUR, base);
  CuAssertIntEquals(tc, 2, elapsed->hours);
  elapsed = mud_time_passed(base + SECS_PER_MUD_DAY, base);
  CuAssertIntEquals(tc, 1, elapsed->day);
  elapsed = mud_time_passed(base + SECS_PER_MUD_MONTH, base);
  CuAssertIntEquals(tc, 1, elapsed->month);
}

void Test_upstream_script_formatter_nested_blocks(CuTest *tc)
{
  struct descriptor_data descriptor = {0};
  char *script;
  const char *expected;

  script = strdup("switch %mode%\r\n"
                  "case one\r\n"
                  "if %enabled%\r\n"
                  "say yes\r\n"
                  "else\r\n"
                  "say no\r\n"
                  "end\r\n"
                  "break\r\n"
                  "default\r\n"
                  "while %waiting%\r\n"
                  "wait 1 sec\r\n"
                  "done\r\n"
                  "end\r\n");
  CuAssertPtrNotNull(tc, script);
  descriptor.str = &script;
  descriptor.max_str = MAX_CMD_LENGTH;

  CuAssertTrue(tc, format_script(&descriptor));
  expected = "switch %mode%\r\n"
             "  case one\r\n"
             "    if %enabled%\r\n"
             "      say yes\r\n"
             "    else\r\n"
             "      say no\r\n"
             "    end\r\n"
             "    break\r\n"
             "  default\r\n"
             "    while %waiting%\r\n"
             "      wait 1 sec\r\n"
             "    done\r\n"
             "end\r\n";
  CuAssertStrEquals(tc, expected, script);
  free(script);
}
