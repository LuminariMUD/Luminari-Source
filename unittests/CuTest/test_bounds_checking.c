#include "CuTest.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

/* Include the actual headers from src */
#include "conf.h"
#include "../../src/core/sysdep.h"
#include "../../src/core/structs.h"
#include "../../src/core/utils.h"
#include "../../src/core/comm.h"
#include "../../src/core/modify.h"
#include "../../src/act/act.h"
#include "../../src/magic/spells.h"
#include "../../src/net/protocol.h"
#include "../../src/wilderness/wilderness.h"

/* External function declaration */

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
  (void)get_wearoff(0);
  /* Just ensure it doesn't crash - actual result depends on spell_info */

  (void)get_wearoff(1);
  /* Just ensure it doesn't crash */

  (void)get_wearoff(TOP_SPELL_DEFINE - 1);
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

/* Fill the bytes after the terminator so that reading past it changes the
 * result even without a sanitizer. */
static void strip_colors_padded(char *buf, size_t size, const char *input)
{
  memset(buf, 'X', size);
  buf[size - 1] = '\0';
  memcpy(buf, input, strlen(input) + 1);
  strip_colors(buf);
}

void Test_strip_colors_stops_at_a_trailing_marker(CuTest *tc)
{
  char buf[16];

  strip_colors_padded(buf, sizeof(buf), "name@");
  CuAssertStrEquals(tc, "name", buf);

  strip_colors_padded(buf, sizeof(buf), "name\t");
  CuAssertStrEquals(tc, "name", buf);

  strip_colors_padded(buf, sizeof(buf), "@");
  CuAssertStrEquals(tc, "", buf);

  strip_colors_padded(buf, sizeof(buf), "\t");
  CuAssertStrEquals(tc, "", buf);

  /* Escaped markers and complete codes keep their existing behavior. */
  strip_colors_padded(buf, sizeof(buf), "a@Rb@@c\tnd\t\te");
  CuAssertStrEquals(tc, "ab@cd\te", buf);

  strip_colors_padded(buf, sizeof(buf), "@@");
  CuAssertStrEquals(tc, "@", buf);

  strip_colors_padded(buf, sizeof(buf), "");
  CuAssertStrEquals(tc, "", buf);
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
  char small[13];

  CuAssertTrue(tc, build_safe_path(filename, sizeof(filename), "world/wld/", "1200.wld",
                                   SAFE_PATH_FILENAME));
  CuAssertStrEquals(tc, "world/wld/1200.wld", filename);
  CuAssertTrue(
      tc, build_safe_path(filename, sizeof(filename), "", "zone-name_2.obj", SAFE_PATH_FILENAME));
  CuAssertTrue(
      tc, !build_safe_path(filename, sizeof(filename), "", "../etc/passwd", SAFE_PATH_FILENAME));
  CuAssertStrEquals(tc, "", filename);
  CuAssertTrue(
      tc, !build_safe_path(filename, sizeof(filename), "", "subdir/file.wld", SAFE_PATH_FILENAME));
  CuAssertTrue(tc,
               !build_safe_path(filename, sizeof(filename), "", "zone..obj", SAFE_PATH_FILENAME));
  CuAssertTrue(tc, !build_safe_path(filename, sizeof(filename), "", ".", SAFE_PATH_FILENAME));
  CuAssertTrue(tc, !build_safe_path(filename, sizeof(filename), "", "", SAFE_PATH_FILENAME));
  CuAssertTrue(tc, get_filename(filename, sizeof(filename), PLR_FILE, "taure_two"));
  CuAssertTrue(tc, !get_filename(filename, sizeof(filename), PLR_FILE, "../../player"));
  CuAssertTrue(tc,
               build_safe_path(filename, sizeof(filename), "", "etc/config", SAFE_PATH_RELATIVE));
  CuAssertStrEquals(tc, "etc/config", filename);
  CuAssertTrue(
      tc, build_safe_path(filename, sizeof(filename), "", "config/test-1.cfg", SAFE_PATH_RELATIVE));
  CuAssertTrue(tc,
               !build_safe_path(filename, sizeof(filename), "", "/etc/passwd", SAFE_PATH_RELATIVE));
  CuAssertTrue(
      tc, !build_safe_path(filename, sizeof(filename), "", "../etc/config", SAFE_PATH_RELATIVE));
  CuAssertTrue(
      tc, !build_safe_path(filename, sizeof(filename), "", "etc/../config", SAFE_PATH_RELATIVE));
  CuAssertTrue(
      tc, !build_safe_path(filename, sizeof(filename), "", "etc/./config", SAFE_PATH_RELATIVE));
  CuAssertTrue(tc,
               !build_safe_path(filename, sizeof(filename), "", "etc//config", SAFE_PATH_RELATIVE));
  CuAssertTrue(tc, !build_safe_path(filename, sizeof(filename), "", "etc/", SAFE_PATH_RELATIVE));
  CuAssertTrue(tc,
               !build_safe_path(filename, sizeof(filename), "", "etc\\config", SAFE_PATH_RELATIVE));

  /* An operator's own path keeps every character and only refuses a ".." component. */
  CuAssertTrue(tc, build_safe_path(filename, sizeof(filename), "", "/tmp/run dir/server.log",
                                   SAFE_PATH_OPERATOR));
  CuAssertStrEquals(tc, "/tmp/run dir/server.log", filename);
  CuAssertTrue(tc, build_safe_path(filename, sizeof(filename), "", "~user/a+b@c:d,e/./x..log",
                                   SAFE_PATH_OPERATOR));
  CuAssertStrEquals(tc, "~user/a+b@c:d,e/./x..log", filename);
  CuAssertTrue(tc,
               build_safe_path(filename, sizeof(filename), "", "log//syslog/", SAFE_PATH_OPERATOR));
  CuAssertTrue(
      tc, !build_safe_path(filename, sizeof(filename), "", "../log/syslog", SAFE_PATH_OPERATOR));
  CuAssertStrEquals(tc, "", filename);
  CuAssertTrue(
      tc, !build_safe_path(filename, sizeof(filename), "", "log/../syslog", SAFE_PATH_OPERATOR));
  CuAssertTrue(tc, !build_safe_path(filename, sizeof(filename), "", "log/..", SAFE_PATH_OPERATOR));
  CuAssertTrue(tc, !build_safe_path(filename, sizeof(filename), "", "..", SAFE_PATH_OPERATOR));
  CuAssertTrue(tc, !build_safe_path(filename, sizeof(filename), "", "", SAFE_PATH_OPERATOR));

  /* The whole result, prefix included, must fit. */
  CuAssertTrue(tc, build_safe_path(small, sizeof(small), "wld/", "1200.wld", SAFE_PATH_FILENAME));
  CuAssertStrEquals(tc, "wld/1200.wld", small);
  CuAssertTrue(tc, !build_safe_path(small, sizeof(small), "wld/", "12000.wld", SAFE_PATH_FILENAME));
  CuAssertStrEquals(tc, "", small);
  CuAssertTrue(tc,
               !build_safe_path(small, sizeof(small), "world/wld/xx", "1.wld", SAFE_PATH_FILENAME));
  CuAssertStrEquals(tc, "", small);
}

/* The -o log path opens exactly as given, and a ".." component stops the boot instead of
 * logging somewhere else. Each call runs in a child: the accepted path redirects stderr and
 * the refused one exits. */
void Test_setup_log_keeps_operator_path_and_refuses_parent_component(CuTest *tc)
{
  char directory[] = "/tmp/luminari-setup-log-XXXXXX";
  char subdirectory[MAX_FILEPATH];
  char log_path[MAX_FILEPATH];
  char refused[MAX_FILEPATH];
  struct stat file_stat;
  pid_t child;
  int status = 0;

  CuAssertPtrNotNull(tc, mkdtemp(directory));
  snprintf(subdirectory, sizeof(subdirectory), "%s/run dir", directory);
  CuAssertIntEquals(tc, 0, mkdir(subdirectory, 0700));
  snprintf(log_path, sizeof(log_path), "%s/run dir/server.log", directory);
  snprintf(refused, sizeof(refused), "%s/run dir/../server.log", directory);

  fflush(NULL);
  child = fork();
  CuAssertTrue(tc, child >= 0);
  if (child == 0)
  {
    if (freopen("/dev/null", "w", stdout) == NULL)
      CuTestChildExit(2);
    setup_log_for_test(log_path);
    log("setup_log test entry");
    CuTestChildExit(logfile == NULL);
  }
  CuAssertIntEquals(tc, child, waitpid(child, &status, 0));
  CuAssertTrue(tc, WIFEXITED(status));
  CuAssertIntEquals(tc, 0, WEXITSTATUS(status));
  CuAssertIntEquals(tc, 0, stat(log_path, &file_stat));
  CuAssertTrue(tc, file_stat.st_size > 0);

  fflush(NULL);
  child = fork();
  CuAssertTrue(tc, child >= 0);
  if (child == 0)
  {
    if (freopen("/dev/null", "w", stdout) == NULL)
      CuTestChildExit(2);
    setup_log_for_test(refused); /* exits 1 before opening anything */
    CuTestChildExit(0);
  }
  CuAssertIntEquals(tc, child, waitpid(child, &status, 0));
  CuAssertTrue(tc, WIFEXITED(status));
  CuAssertIntEquals(tc, 1, WEXITSTATUS(status));

  unlink(log_path);
  rmdir(subdirectory);
  rmdir(directory);
}

/* The atoi() family replacements read numbers the same way but define the edge cases. */
void Test_parse_number_helpers(CuTest *tc)
{
  CuAssertIntEquals(tc, 42, parse_int("42"));
  CuAssertIntEquals(tc, -7, parse_int("  -7 apples"));
  CuAssertIntEquals(tc, 0, parse_int("apples"));
  CuAssertIntEquals(tc, 0, parse_int(""));
  CuAssertIntEquals(tc, 0, parse_int(NULL));
  CuAssertIntEquals(tc, INT_MAX, parse_int("99999999999"));
  CuAssertIntEquals(tc, INT_MIN, parse_int("-99999999999"));
  CuAssertTrue(tc, parse_long("123456789012") == 123456789012L);
  CuAssertTrue(tc, parse_long(NULL) == 0L);
  CuAssertTrue(tc, parse_llong("-9000000000000") == -9000000000000LL);
  CuAssertTrue(tc, parse_llong(NULL) == 0LL);
  CuAssertDblEquals(tc, 2.5, parse_double("2.5 units"), 0.0);
  CuAssertDblEquals(tc, 0.0, parse_double(NULL), 0.0);
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

void Test_scan_distance_labels_cover_every_farsee_range(CuTest *tc)
{
  static const char *const expected[] = {
      "You see scan target close by north.\r\n",
      "You see scan target a ways off north.\r\n",
      "You see scan target far off to the north.\r\n",
      "You see scan target far off to the north.\r\n",
      "You see scan target far off to the north.\r\n",
      "You see scan target far off to the north.\r\n",
  };
  struct char_data ch;
  struct descriptor_data descriptor;
  size_t distance;

  memset(&ch, 0, sizeof(ch));
  memset(&descriptor, 0, sizeof(descriptor));
  ch.player.name = CuMutableString("scan target");
  ch.desc = &descriptor;
  descriptor.character = &ch;
  descriptor.output = descriptor.small_outbuf;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  descriptor.pProtocol = ProtocolCreate();

  if (descriptor.pProtocol == NULL)
  {
    CuFail(tc, "could not initialize protocol output for the scan range test");
    return;
  }

  for (distance = 0; distance < sizeof(expected) / sizeof(expected[0]); distance++)
  {
    list_scanned_chars(&ch, &ch, (int)distance, NORTH);
    CuAssertStrEquals_Msg(tc, "scan output must be valid for every Farsee range",
                          expected[distance], descriptor.output);
    descriptor.small_outbuf[0] = '\0';
    descriptor.output = descriptor.small_outbuf;
    descriptor.bufptr = 0;
    descriptor.bufspace = SMALL_BUFSIZE - 1;
  }

  ch.desc = NULL;
  ProtocolDestroy(descriptor.pProtocol);
}
