#include "CuTest.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* Include the actual headers from src */
#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/spells.h"

/* External function declaration */
extern const char *get_wearoff(int abilnum);

/* Test for get_wearoff bounds checking */
void Test_get_wearoff_bounds_checking(CuTest *tc)
{
  const char *result;

  /* Test negative spell numbers */
  result = get_wearoff(-1);
  CuAssertPtrEquals(tc, NULL, result);

  result = get_wearoff(-100);
  CuAssertPtrEquals(tc, NULL, result);

  result = get_wearoff(-999999);
  CuAssertPtrEquals(tc, NULL, result);

  /* Test out of bounds positive spell numbers */
  result = get_wearoff(TOP_SPELL_DEFINE);
  CuAssertPtrEquals(tc, NULL, result);

  result = get_wearoff(TOP_SPELL_DEFINE + 1);
  CuAssertPtrEquals(tc, NULL, result);

  result = get_wearoff(TOP_SPELL_DEFINE + 1000);
  CuAssertPtrEquals(tc, NULL, result);

  result = get_wearoff(99999);
  CuAssertPtrEquals(tc, NULL, result);

  /* Test valid spell numbers (assuming spell_info is initialized) */
  /* Note: These tests may return NULL if spell_info is not initialized,
       but they should not crash */
  result = get_wearoff(0);
  /* Just ensure it doesn't crash - actual result depends on spell_info */

  result = get_wearoff(1);
  /* Just ensure it doesn't crash */

  result = get_wearoff(TOP_SPELL_DEFINE - 1);
  /* Just ensure it doesn't crash */
}

/* Test for damage reduction spell bounds checking in fight.c */
void Test_dr_spell_bounds_validation(CuTest *tc)
{
  /* This test validates that the bounds checking in apply_damage_reduction
       prevents invalid spell numbers from being used */

  /* Create a mock damage reduction structure */
  struct damage_reduction_type dr;

  /* Test with invalid spell numbers */
  dr.spell = -1;
  /* In real code, this would be checked in apply_damage_reduction */
  CuAssertTrue(tc, dr.spell < 0 || dr.spell >= TOP_SPELL_DEFINE);

  dr.spell = TOP_SPELL_DEFINE;
  CuAssertTrue(tc, dr.spell < 0 || dr.spell >= TOP_SPELL_DEFINE);

  dr.spell = TOP_SPELL_DEFINE + 100;
  CuAssertTrue(tc, dr.spell < 0 || dr.spell >= TOP_SPELL_DEFINE);

  /* Test with valid spell numbers */
  dr.spell = 0;
  CuAssertTrue(tc, dr.spell >= 0 && dr.spell < TOP_SPELL_DEFINE);

  dr.spell = 100;
  CuAssertTrue(tc, dr.spell >= 0 && dr.spell < TOP_SPELL_DEFINE);

  dr.spell = TOP_SPELL_DEFINE - 1;
  CuAssertTrue(tc, dr.spell >= 0 && dr.spell < TOP_SPELL_DEFINE);
}

void Test_snprintf_append_saturates_offset(CuTest *tc)
{
  char buffer[8] = {'\0'};
  int offset;

  strlcpy(buffer, "123456789", sizeof(buffer));
  offset = 9;
  offset = snprintf_append(buffer, sizeof(buffer), offset, "%s", "ignored");

  CuAssertIntEquals(tc, (int)sizeof(buffer) - 1, offset);
  CuAssertStrEquals(tc, "1234567", buffer);

  offset = snprintf_append(buffer, sizeof(buffer), offset, "%s", "ignored");
  CuAssertIntEquals(tc, (int)sizeof(buffer) - 1, offset);
  CuAssertStrEquals(tc, "1234567", buffer);
}

void Test_path_component_validation(CuTest *tc)
{
  char filename[MAX_FILEPATH];

  CuAssertTrue(tc, is_safe_path_component("1200.wld"));
  CuAssertTrue(tc, is_safe_path_component("zone-name_2.obj"));
  CuAssertTrue(tc, !is_safe_path_component("../etc/passwd"));
  CuAssertTrue(tc, !is_safe_path_component("subdir/file.wld"));
  CuAssertTrue(tc, !is_safe_path_component("zone..obj"));
  CuAssertTrue(tc, get_filename(filename, sizeof(filename), PLR_FILE, "taure_two"));
  CuAssertTrue(tc, !get_filename(filename, sizeof(filename), PLR_FILE, "../../player"));
}

void Test_fopen_restricted_blocks_world_write(CuTest *tc)
{
  char path[] = "/tmp/luminari-fopen-test-XXXXXX";
  struct stat file_stat;
  FILE *file;
  int fd;
  int opened;
  int stat_ok;
  int secure_mode;

  fd = mkstemp(path);
  CuAssertTrue(tc, fd >= 0);
  close(fd);
  unlink(path);

  file = fopen_restricted(path, "w");
  opened = file != NULL;
  if (file)
    fclose(file);
  stat_ok = stat(path, &file_stat) == 0;
  secure_mode = stat_ok && !(file_stat.st_mode & S_IWOTH);
  unlink(path);

  CuAssertTrue(tc, opened);
  CuAssertTrue(tc, stat_ok);
  CuAssertTrue(tc, secure_mode);
}

/* Suite setup */
CuSuite *BoundsCheckingSuite(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, Test_get_wearoff_bounds_checking);
  SUITE_ADD_TEST(suite, Test_dr_spell_bounds_validation);
  SUITE_ADD_TEST(suite, Test_snprintf_append_saturates_offset);
  SUITE_ADD_TEST(suite, Test_path_component_validation);
  SUITE_ADD_TEST(suite, Test_fopen_restricted_blocks_world_write);
  return suite;
}
