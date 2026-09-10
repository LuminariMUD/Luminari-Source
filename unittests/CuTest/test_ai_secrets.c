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

/* Rewind a capture stream and read its whole content as a NUL-terminated string. */
static void read_whole_file(FILE *fp, char *out, size_t out_size)
{
  size_t got;

  fflush(fp);
  rewind(fp);
  got = fread(out, 1, out_size - 1, fp);
  out[got] = '\0';
}

/* A stored key comes back verbatim; a cleared store yields nothing. */
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

/* Setting an empty or NULL key is the same as clearing it. */
void TestAiApiKeyEmptyAndNullClear(CuTest *tc)
{
  CuAssertTrue(tc, ai_api_key_set(TEST_SECRET));
  CuAssertTrue(tc, ai_api_key_set(""));
  CuAssertTrue(tc, !ai_api_key_is_set());

  CuAssertTrue(tc, ai_api_key_set(TEST_SECRET));
  CuAssertTrue(tc, ai_api_key_set(NULL));
  CuAssertTrue(tc, !ai_api_key_is_set());
}

/* The longest key that fits is kept; one byte more is rejected, not truncated. */
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

/* A copy into a buffer that cannot hold the key yields an empty buffer. */
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

/* Rejecting an over-long key logs a reason but never the key material.
 * The log stream is restored before the first assertion. */
void TestAiApiKeyNeverReachesLog(CuTest *tc)
{
  FILE *saved_log = logfile;
  FILE *capture = tmpfile();
  char too_long[AI_API_KEY_MAX_LEN + 8];
  char captured[4096];
  bool stored = FALSE;
  bool rejected = FALSE;

  captured[0] = '\0';
  if (capture)
  {
    logfile = capture;
    stored = ai_api_key_set(TEST_SECRET);
    memset(too_long, 'z', sizeof(too_long) - 1);
    memcpy(too_long, TEST_SECRET, strlen(TEST_SECRET));
    too_long[sizeof(too_long) - 1] = '\0';
    rejected = !ai_api_key_set(too_long);
    ai_api_key_clear();
    read_whole_file(capture, captured, sizeof(captured));
    logfile = saved_log;
    fclose(capture);
  }

  CuAssertPtrNotNull(tc, capture);
  CuAssertTrue(tc, stored);
  CuAssertTrue(tc, rejected);
  CuAssertTrue(tc, strstr(captured, "rejected") != NULL);
  CuAssertTrue(tc, strstr(captured, TEST_SECRET) == NULL);
  CuAssertTrue(tc, strstr(captured, "SECRET") == NULL);
}

/* Write a minimal .env in the current directory for load_ai_config(). */
static void write_scratch_env(const char *body)
{
  FILE *fp = fopen(".env", "w");

  if (!fp)
    return;
  fputs(body, fp);
  fclose(fp);
}

/* Exercises load_ai_config() with a sentinel key and a cleartext endpoint.
 * Every process-wide change (cwd, ai_state.config, logfile, environment) is
 * made, the observations are captured, and the state is restored BEFORE the
 * first CuAssert, because CuAssert longjmps past any cleanup that follows. */
void TestAiLoadConfigPrefersProcessEnvironmentAndRedacts(CuTest *tc)
{
  struct ai_config *saved_config = ai_state.config;
  bool saved_configured = ai_state.openai_configured;
  FILE *saved_log = logfile;
  FILE *capture = tmpfile();
  char cwd[PATH_MAX];
  char scratch[] = "/tmp/luminari-ai-secrets-XXXXXX";
  char captured[8192];
  char first_copy[AI_API_KEY_MAX_LEN];
  char endpoint[256];
  struct ai_config *config = NULL;
  bool setup_ok = FALSE;
  bool first_configured = FALSE;
  bool first_copied = FALSE;
  bool second_configured = TRUE;
  bool second_set = TRUE;

  captured[0] = '\0';
  first_copy[0] = '\0';
  endpoint[0] = '\0';

  if (capture && getcwd(cwd, sizeof(cwd)) && mkdtemp(scratch) && chdir(scratch) == 0)
  {
    /* An empty directory keeps a developer lib/.env out of the picture; the
     * only .env here carries a cleartext endpoint that must be refused. */
    write_scratch_env("OPENAI_API_ENDPOINT=http://proxy.example.test/v1/chat/completions\n");
    CREATE(config, struct ai_config, 1);
    ai_state.config = config;
    logfile = capture;
    setup_ok = setenv("OPENAI_API_KEY", TEST_SECRET, 1) == 0;

    if (setup_ok)
    {
      load_ai_config();
      first_configured = ai_state.openai_configured;
      first_copied = ai_api_key_copy(first_copy, sizeof(first_copy));
      strlcpy(endpoint, ai_state.config->openai_endpoint, sizeof(endpoint));

      /* Removing the key and reloading must wipe the resident copy. */
      setup_ok = unsetenv("OPENAI_API_KEY") == 0;
      load_ai_config();
      second_configured = ai_state.openai_configured;
      second_set = ai_api_key_is_set();
    }

    read_whole_file(capture, captured, sizeof(captured));

    /* Restore everything before asserting. */
    logfile = saved_log;
    ai_state.config = saved_config;
    ai_state.openai_configured = saved_configured;
    free(config);
    unsetenv("OPENAI_API_KEY");
    ai_api_key_clear();
    unlink(".env");
    if (chdir(cwd) == 0)
      rmdir(scratch);
  }
  if (capture)
    fclose(capture);

  CuAssertTrue(tc, setup_ok);
  CuAssertTrue(tc, first_configured);
  CuAssertTrue(tc, first_copied);
  CuAssertStrEquals(tc, TEST_SECRET, first_copy);
  CuAssertStrEquals(tc, DEFAULT_OPENAI_API_ENDPOINT, endpoint);
  CuAssertTrue(tc, !second_configured);
  CuAssertTrue(tc, !second_set);
  CuAssertTrue(tc, strstr(captured, "loaded from environment") != NULL);
  CuAssertTrue(tc, strstr(captured, "must use https://") != NULL);
  CuAssertTrue(tc, strstr(captured, TEST_SECRET) == NULL);
}

/* The https check is case-insensitive and rejects every other scheme. */
void TestAiEndpointRequiresHttps(CuTest *tc)
{
  CuAssertTrue(tc, ai_endpoint_is_https("https://api.openai.com/v1/chat/completions"));
  CuAssertTrue(tc, ai_endpoint_is_https("HTTPS://proxy.example.test/v1"));
  CuAssertTrue(tc, !ai_endpoint_is_https("http://localhost:8080/v1"));
  CuAssertTrue(tc, !ai_endpoint_is_https("ftp://example.test"));
  CuAssertTrue(tc, !ai_endpoint_is_https("api.openai.com"));
  CuAssertTrue(tc, !ai_endpoint_is_https(""));
  CuAssertTrue(tc, !ai_endpoint_is_https(NULL));
}

/* Sanitization escapes JSON metacharacters and never overruns the caller buffer. */
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
