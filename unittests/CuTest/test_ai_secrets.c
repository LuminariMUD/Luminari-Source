/* Regression tests for the OpenAI API key lifecycle in src/ai_security.c.
 * Covers store/copy/clear boundaries, caller-buffer sanitization, runtime
 * injection priority, and proof that the key never reaches the log. */

#include "CuTest.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/ai_service.h"

extern FILE *logfile;

#define TEST_SECRET "sk-test-SECRET-0123456789abcdef"

static void read_whole_file(FILE *fp, char *out, size_t out_size)
{
  size_t got;

  fflush(fp);
  rewind(fp);
  got = fread(out, 1, out_size - 1, fp);
  out[got] = '\0';
}

void TestAiApiKeyRoundTrip(CuTest *tc)
{
  char out[AI_API_KEY_MAX_LEN];

  CuAssertTrue(tc, ai_api_key_set(TEST_SECRET));
  CuAssertTrue(tc, ai_api_key_is_set());
  CuAssertTrue(tc, ai_api_key_copy(out, sizeof(out)));
  CuAssertStrEquals(tc, TEST_SECRET, out);

  ai_api_key_clear();
  CuAssertTrue(tc, !ai_api_key_is_set());
  CuAssertTrue(tc, !ai_api_key_copy(out, sizeof(out)));
  CuAssertStrEquals(tc, "", out);
}

void TestAiApiKeyEmptyAndNullClear(CuTest *tc)
{
  CuAssertTrue(tc, ai_api_key_set(TEST_SECRET));
  CuAssertTrue(tc, ai_api_key_set(""));
  CuAssertTrue(tc, !ai_api_key_is_set());

  CuAssertTrue(tc, ai_api_key_set(TEST_SECRET));
  CuAssertTrue(tc, ai_api_key_set(NULL));
  CuAssertTrue(tc, !ai_api_key_is_set());
}

void TestAiApiKeyLengthBoundary(CuTest *tc)
{
  char longest[AI_API_KEY_MAX_LEN];
  char too_long[AI_API_KEY_MAX_LEN + 1];
  char out[AI_API_KEY_MAX_LEN];

  memset(longest, 'k', sizeof(longest) - 1);
  longest[sizeof(longest) - 1] = '\0';
  CuAssertTrue(tc, ai_api_key_set(longest));
  CuAssertTrue(tc, ai_api_key_copy(out, sizeof(out)));
  CuAssertStrEquals(tc, longest, out);

  memset(too_long, 'k', sizeof(too_long) - 1);
  too_long[sizeof(too_long) - 1] = '\0';
  CuAssertTrue(tc, !ai_api_key_set(too_long));
  /* A rejected key must not leave the previous key resident either. */
  CuAssertTrue(tc, !ai_api_key_is_set());
  CuAssertTrue(tc, !ai_api_key_copy(out, sizeof(out)));
  CuAssertStrEquals(tc, "", out);

  ai_api_key_clear();
}

void TestAiApiKeyCopyRefusesSmallBuffer(CuTest *tc)
{
  char small[8];

  CuAssertTrue(tc, ai_api_key_set(TEST_SECRET));
  memset(small, 'x', sizeof(small));
  CuAssertTrue(tc, !ai_api_key_copy(small, sizeof(small)));
  CuAssertStrEquals(tc, "", small);
  CuAssertTrue(tc, !ai_api_key_copy(NULL, 16));
  CuAssertTrue(tc, !ai_api_key_copy(small, 0));

  ai_api_key_clear();
}

void TestAiApiKeyNeverReachesLog(CuTest *tc)
{
  FILE *saved_log = logfile;
  FILE *capture = tmpfile();
  char too_long[AI_API_KEY_MAX_LEN + 8];
  char captured[4096];

  CuAssertPtrNotNull(tc, capture);
  logfile = capture;

  CuAssertTrue(tc, ai_api_key_set(TEST_SECRET));
  memset(too_long, 'z', sizeof(too_long) - 1);
  memcpy(too_long, TEST_SECRET, strlen(TEST_SECRET));
  too_long[sizeof(too_long) - 1] = '\0';
  CuAssertTrue(tc, !ai_api_key_set(too_long));
  ai_api_key_clear();

  read_whole_file(capture, captured, sizeof(captured));
  logfile = saved_log;
  fclose(capture);

  CuAssertTrue(tc, strstr(captured, "rejected") != NULL);
  CuAssertTrue(tc, strstr(captured, TEST_SECRET) == NULL);
  CuAssertTrue(tc, strstr(captured, "SECRET") == NULL);
}

void TestAiLoadConfigPrefersProcessEnvironmentAndRedacts(CuTest *tc)
{
  struct ai_config *saved_config = ai_state.config;
  bool saved_configured = ai_state.openai_configured;
  FILE *saved_log = logfile;
  FILE *capture = tmpfile();
  char cwd[PATH_MAX];
  char scratch[] = "/tmp/luminari-ai-secrets-XXXXXX";
  char captured[8192];
  char out[AI_API_KEY_MAX_LEN];
  struct ai_config *config;

  CuAssertPtrNotNull(tc, capture);
  CuAssertPtrNotNull(tc, getcwd(cwd, sizeof(cwd)));
  /* Run from an empty directory so no developer lib/.env can supply a key. */
  CuAssertPtrNotNull(tc, mkdtemp(scratch));
  CuAssertIntEquals(tc, 0, chdir(scratch));

  CREATE(config, struct ai_config, 1);
  ai_state.config = config;
  logfile = capture;

  CuAssertIntEquals(tc, 0, setenv("OPENAI_API_KEY", TEST_SECRET, 1));
  load_ai_config();
  CuAssertTrue(tc, ai_state.openai_configured);
  CuAssertTrue(tc, ai_api_key_copy(out, sizeof(out)));
  CuAssertStrEquals(tc, TEST_SECRET, out);

  /* Removing the key and reloading must wipe the resident copy. */
  CuAssertIntEquals(tc, 0, unsetenv("OPENAI_API_KEY"));
  load_ai_config();
  CuAssertTrue(tc, !ai_state.openai_configured);
  CuAssertTrue(tc, !ai_api_key_is_set());

  read_whole_file(capture, captured, sizeof(captured));
  CuAssertTrue(tc, strstr(captured, "loaded from environment") != NULL);
  CuAssertTrue(tc, strstr(captured, TEST_SECRET) == NULL);

  logfile = saved_log;
  fclose(capture);
  free(config);
  ai_state.config = saved_config;
  ai_state.openai_configured = saved_configured;
  CuAssertIntEquals(tc, 0, chdir(cwd));
  rmdir(scratch);
}

void TestSanitizeAiInputUsesCallerBuffer(CuTest *tc)
{
  char out[16];

  sanitize_ai_input("say \"hi\"\\\x01\r\n  ", out, sizeof(out));
  CuAssertStrEquals(tc, "say \\\"hi\\\"\\\\", out);

  /* Output is bounded and never splits an escape sequence. */
  sanitize_ai_input("aaaaaaaaaaaaa\"bbbb", out, sizeof(out));
  CuAssertTrue(tc, strlen(out) < sizeof(out));
  CuAssertStrEquals(tc, "aaaaaaaaaaaaa\\\"", out);
  sanitize_ai_input("aaaaaaaaaaaaaa\"bbbb", out, sizeof(out));
  CuAssertStrEquals(tc, "aaaaaaaaaaaaaa", out);

  memset(out, 'x', sizeof(out));
  sanitize_ai_input(NULL, out, sizeof(out));
  CuAssertStrEquals(tc, "", out);

  /* Degenerate buffers must not be written. */
  sanitize_ai_input("text", NULL, 8);
  out[0] = 'q';
  sanitize_ai_input("text", out, 0);
  CuAssertTrue(tc, out[0] == 'q');
}
