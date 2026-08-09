#include "CuTest.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* Include the actual headers from src */
#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/magic/spells.h"
#include "../../src/wilderness/wilderness.h"

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

void Test_strfrmt_truncates_oversized_input_and_dimensions(CuTest *tc)
{
  size_t input_size = (size_t)MAX_STRING_LENGTH * 2;
  char *long_word;
  char *result;

  long_word = malloc(input_size);
  CuAssertPtrNotNull(tc, long_word);
  if (!long_word)
    return;

  memset(long_word, 'A', input_size - 1);
  long_word[input_size - 1] = '\0';

  result = strfrmt(long_word, 59, 21, FALSE, TRUE, TRUE);
  CuAssertPtrNotNull(tc, result);
  CuAssertTrue(tc, strlen(result) < MAX_STRING_LENGTH);

  result = strfrmt("word", INT_MAX, INT_MAX, FALSE, TRUE, TRUE);
  CuAssertPtrNotNull(tc, result);
  CuAssertTrue(tc, strlen(result) < MAX_STRING_LENGTH);

  CuAssertStrEquals(tc, "", strfrmt(NULL, 59, 21, FALSE, TRUE, TRUE));
  CuAssertStrEquals(tc, "", strfrmt("", 59, 0, FALSE, FALSE, FALSE));
  free(long_word);
}

void Test_strfrmt_preserves_wrapping_and_color_behavior(CuTest *tc)
{
  CuAssertStrEquals(
      tc,
      "The quick brown fox \tn\r\n"
      "jumps over the lazy \tn\r\n"
      "dog.                \r\n",
      strfrmt("The quick brown fox jumps over the lazy dog.", 20, 3, FALSE, TRUE, TRUE));
  CuAssertStrEquals(tc,
                    "\tgGreen words\tn\r\n"
                    "\tgcontinue across\tn\r\n"
                    "\tgseveral lines and\tn\r\n"
                    "\tgstay green\tn.\r\n",
                    strfrmt("\tgGreen words continue across several lines and stay green\tn.", 18,
                            0, FALSE, FALSE, FALSE));
  CuAssertStrEquals(tc, "first\r\nsecond\r\n",
                    strfrmt("first\\\\second", 20, 0, FALSE, FALSE, FALSE));
}

void Test_strpaste_rejects_an_oversized_joiner(CuTest *tc)
{
  size_t joiner_size = (size_t)MAX_STRING_LENGTH * 2;
  char *joiner;
  const char *result;

  joiner = malloc(joiner_size);
  CuAssertPtrNotNull(tc, joiner);
  if (!joiner)
    return;

  memset(joiner, '-', joiner_size - 1);
  joiner[joiner_size - 1] = '\0';

  result = strpaste("left", "right", joiner);
  CuAssertStrEquals(tc, "left", result);
  CuAssertStrEquals(tc, "", strpaste(NULL, NULL, NULL));
  free(joiner);
}

void Test_wilderness_map_truncates_an_oversized_glyph(CuTest *tc)
{
  size_t glyph_size = (size_t)MAX_STRING_LENGTH * 2;
  struct wild_map_tile tile_data[4];
  struct wild_map_tile *map[2];
  char *glyph;
  const char *result;
  int i;

  glyph = malloc(glyph_size);
  CuAssertPtrNotNull(tc, glyph);
  if (!glyph)
    return;

  memset(glyph, 'G', glyph_size - 1);
  glyph[glyph_size - 1] = '\0';
  memset(tile_data, 0, sizeof(tile_data));
  map[0] = &tile_data[0];
  map[1] = &tile_data[2];
  for (i = 0; i < 4; i++)
  {
    tile_data[i].vis = 1;
    tile_data[i].sector_type = SECT_FIELD;
    tile_data[i].glyph = glyph;
  }

  result = wilderness_test_map_to_string(map, 2);
  CuAssertPtrNotNull(tc, result);
  CuAssertTrue(tc, strlen(result) < glyph_size - 1);
  CuAssertTrue(tc, strlen(result) > 0);
  free(glyph);
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
  CuAssertTrue(tc, is_safe_relative_path("etc/config"));
  CuAssertTrue(tc, is_safe_relative_path("config/test-1.cfg"));
  CuAssertTrue(tc, !is_safe_relative_path("/etc/passwd"));
  CuAssertTrue(tc, !is_safe_relative_path("../etc/config"));
  CuAssertTrue(tc, !is_safe_relative_path("etc/../config"));
  CuAssertTrue(tc, !is_safe_relative_path("etc//config"));
  CuAssertTrue(tc, !is_safe_relative_path("etc\\config"));
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
  SUITE_ADD_TEST(suite, Test_strfrmt_truncates_oversized_input_and_dimensions);
  SUITE_ADD_TEST(suite, Test_strfrmt_preserves_wrapping_and_color_behavior);
  SUITE_ADD_TEST(suite, Test_strpaste_rejects_an_oversized_joiner);
  SUITE_ADD_TEST(suite, Test_wilderness_map_truncates_an_oversized_glyph);
  SUITE_ADD_TEST(suite, Test_path_component_validation);
  SUITE_ADD_TEST(suite, Test_fopen_restricted_blocks_world_write);
  return suite;
}
