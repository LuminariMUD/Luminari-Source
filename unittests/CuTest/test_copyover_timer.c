#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/comm.h"

#include <string.h>

void Test_copyover_executes_the_installed_release(CuTest *tc)
{
  char line[1024];
  char source_path[4096];
  const char *test_root;
  FILE *source_file;
  bool found_exact_exec = FALSE;
  bool found_installed_resolution = FALSE;
  bool found_proc_resolution = FALSE;

  test_root = getenv("LUMINARI_TEST_ROOT");
  if (test_root == NULL || *test_root == '\0')
    test_root = ".";
  snprintf(source_path, sizeof(source_path), "%s/src/act.wizard.c", test_root);
  source_file = fopen(source_path, "r");
  if (source_file == NULL)
  {
    CuFail(tc, "Unable to inspect the production copyover implementation");
    return;
  }

  while (fgets(line, sizeof(line), source_file) != NULL)
  {
    if (strstr(line, "realpath(\"../\" EXE_FILE, copyover_executable)") != NULL)
      found_installed_resolution = TRUE;
    if (strstr(line, "readlink(\"/proc/self/exe\"") != NULL)
      found_proc_resolution = TRUE;
    if (strstr(line, "execl(copyover_executable") != NULL)
      found_exact_exec = TRUE;
  }
  fclose(source_file);

  CuAssertTrue(tc, found_installed_resolution);
  CuAssertTrue(tc, found_exact_exec);
  CuAssertTrue(tc, !found_proc_resolution);
}

void Test_copyover_checkpoint_timer_is_suspended_and_restored(CuTest *tc)
{
#ifdef CIRCLE_UNIX
  struct itimerval original_timer;
  struct itimerval test_timer;
  struct itimerval suspended_timer;
  struct itimerval resumed_timer;
  int get_original_result;
  int arm_result;
  int suspend_result;
  int inspect_suspended_result;
  int resume_result;
  int inspect_resumed_result;
  int cleanup_result;

  memset(&test_timer, 0, sizeof(test_timer));
  memset(&suspended_timer, 0, sizeof(suspended_timer));
  memset(&resumed_timer, 0, sizeof(resumed_timer));

  get_original_result = getitimer(ITIMER_VIRTUAL, &original_timer);
  if (get_original_result != 0)
  {
    CuFail(tc, "Unable to read the original virtual timer");
    return;
  }

  test_timer.it_interval.tv_sec = 120;
  test_timer.it_value.tv_sec = 120;
  arm_result = setitimer(ITIMER_VIRTUAL, &test_timer, NULL);
  suspend_result = arm_result == 0 ? suspend_checkpoint_timer() : FALSE;
  inspect_suspended_result = suspend_result ? getitimer(ITIMER_VIRTUAL, &suspended_timer) : -1;
  resume_result = suspend_result ? resume_checkpoint_timer() : FALSE;
  inspect_resumed_result = resume_result ? getitimer(ITIMER_VIRTUAL, &resumed_timer) : -1;
  cleanup_result = setitimer(ITIMER_VIRTUAL, &original_timer, NULL);

  CuAssertIntEquals(tc, 0, arm_result);
  CuAssertTrue(tc, suspend_result);
  CuAssertIntEquals(tc, 0, inspect_suspended_result);
  CuAssertIntEquals(tc, 0, suspended_timer.it_interval.tv_sec);
  CuAssertIntEquals(tc, 0, suspended_timer.it_interval.tv_usec);
  CuAssertIntEquals(tc, 0, suspended_timer.it_value.tv_sec);
  CuAssertIntEquals(tc, 0, suspended_timer.it_value.tv_usec);
  CuAssertTrue(tc, resume_result);
  CuAssertIntEquals(tc, 0, inspect_resumed_result);
  CuAssertIntEquals(tc, 120, resumed_timer.it_interval.tv_sec);
  CuAssertIntEquals(tc, 0, resumed_timer.it_interval.tv_usec);
  CuAssertTrue(tc, timerisset(&resumed_timer.it_value));
  CuAssertIntEquals(tc, 0, cleanup_result);
#else
  CuAssertTrue(tc, TRUE);
#endif
}
