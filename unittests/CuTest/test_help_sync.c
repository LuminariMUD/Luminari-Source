#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/help.h"

void Test_help_sync_reload_token_validation(CuTest *tc)
{
  const char *valid = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
  const char *uppercase = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdeF";

  CuAssertTrue(tc, help_sync_reload_token_valid(valid));
  CuAssertTrue(tc, !help_sync_reload_token_valid(NULL));
  CuAssertTrue(tc, !help_sync_reload_token_valid("short"));
  CuAssertTrue(tc, !help_sync_reload_token_valid(uppercase));
}

void Test_help_sync_barrier_file_reports_owner(CuTest *tc)
{
  char path[] = "/tmp/luminari-help-sync-test-XXXXXX";
  char owner[80] = {'\0'};
  const char *plan_id = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
  int descriptor;
  ssize_t written;

  descriptor = mkstemp(path);
  CuAssertTrue(tc, descriptor >= 0);
  if (descriptor < 0)
    return;
  written = write(descriptor, plan_id, strlen(plan_id));
  CuAssertIntEquals(tc, (int)strlen(plan_id), (int)written);
  written = write(descriptor, "\n", 1);
  CuAssertIntEquals(tc, 1, (int)written);
  close(descriptor);

  CuAssertTrue(tc, help_sync_barrier_active_at(path, owner, sizeof(owner)));
  CuAssertStrEquals(tc, plan_id, owner);
  unlink(path);
  CuAssertTrue(tc, !help_sync_barrier_active_at(path, owner, sizeof(owner)));
}
