/* Regression coverage for the running executable's ELF build ID lookup.
 *
 * Copyover replaces the process with execl(), which preserves the launching
 * release's LUMINARI_ELF_BUILD_ID.  The startup identity line therefore has to
 * derive the build ID from the image that is actually running rather than from
 * the inherited environment. */

#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/elf_build_id.h"

#include <stdlib.h>
#include <string.h>

void TestSelfElfBuildIdIsReadable(CuTest *tc)
{
  const char *build_id;

  build_id = get_self_elf_build_id();
  CuAssertPtrNotNull(tc, (void *)build_id);
}

void TestSelfElfBuildIdIsLowercaseHex(CuTest *tc)
{
  const char *build_id;
  size_t length;

  build_id = get_self_elf_build_id();
  if (build_id == NULL)
    return;

  length = strlen(build_id);
  CuAssertTrue(tc, length >= 16);
  CuAssertTrue(tc, length <= 128);
  CuAssertTrue(tc, (length % 2) == 0);
  CuAssertIntEquals(tc, (int)length, (int)strspn(build_id, "0123456789abcdef"));
}

/* The lookup must not depend on the environment, which copyover carries over
 * from the previous release. */
void TestSelfElfBuildIdIgnoresEnvironment(CuTest *tc)
{
  char expected[129];
  const char *build_id;
  const char *saved;
  char *saved_copy = NULL;

  build_id = get_self_elf_build_id();
  if (build_id == NULL)
    return;
  snprintf(expected, sizeof(expected), "%s", build_id);

  saved = getenv("LUMINARI_ELF_BUILD_ID");
  if (saved != NULL)
    saved_copy = strdup(saved);

  setenv("LUMINARI_ELF_BUILD_ID", "deadbeefdeadbeefdeadbeef", 1);
  build_id = get_self_elf_build_id();
  CuAssertPtrNotNull(tc, (void *)build_id);
  CuAssertStrEquals(tc, expected, build_id);

  if (saved_copy != NULL)
  {
    setenv("LUMINARI_ELF_BUILD_ID", saved_copy, 1);
    free(saved_copy);
  }
  else
    unsetenv("LUMINARI_ELF_BUILD_ID");
}
