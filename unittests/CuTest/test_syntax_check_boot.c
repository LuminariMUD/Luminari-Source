#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/dgscript/dg_event.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int luminari_main(int argc, char **argv);

#define SYNTAX_CHECK_OUTPUT_SIZE (1024 * 1024)

static const char *syntax_check_test_root(void)
{
  const char *root;

  root = getenv("LUMINARI_TEST_ROOT");
  return root != NULL && *root != '\0' ? root : ".";
}

void Test_syntax_check_empty_event_queue_lifecycle(CuTest *tc)
{
  event_free_all();
  event_test_reset_lifecycle_counts();

  event_init();
  event_free_all();

  CuAssertIntEquals(tc, 1, event_test_init_call_count());
  CuAssertIntEquals(tc, 1, event_test_free_all_call_count());
}

void Test_syntax_check_encounter_world_boots_and_cleans_up_once(CuTest *tc)
{
  char data_dir[PATH_MAX];
  char output[SYNTAX_CHECK_OUTPUT_SIZE];
  char *argv[7];
  const char *root;
  ssize_t bytes_read;
  char discard[4096];
  size_t output_length;
  int output_pipe[2];
  int child_status;
  pid_t child_pid;

  root = syntax_check_test_root();
  CuAssert(tc, "syntax-check data path is too long",
           snprintf(data_dir, sizeof(data_dir), "%s/lib", root) < (int)sizeof(data_dir));
  CuAssertIntEquals(tc, 0, pipe(output_pipe));

  child_pid = fork();
  CuAssert(tc, "fork failed", child_pid >= 0);

  if (child_pid == 0)
  {
    int result;

    close(output_pipe[0]);
    if (dup2(output_pipe[1], STDOUT_FILENO) < 0 || dup2(output_pipe[1], STDERR_FILENO) < 0)
      _exit(20);
    close(output_pipe[1]);

    argv[0] = (char *)"circle";
    argv[1] = (char *)"-c";
    argv[2] = (char *)"-q";
    argv[3] = (char *)"-d";
    argv[4] = data_dir;
    argv[5] = NULL;
    argv[6] = NULL;

    alarm(60);
    event_test_reset_lifecycle_counts();
    result = luminari_main(5, argv);
    if (result != EXIT_SUCCESS)
      _exit(21);
    if (event_test_init_call_count() != 1)
      _exit(22);
    if (event_test_free_all_call_count() != 1)
      _exit(23);
    _exit(EXIT_SUCCESS);
  }

  close(output_pipe[1]);
  output_length = 0;
  while ((bytes_read = read(output_pipe[0],
                            output_length < sizeof(output) - 1 ? output + output_length : discard,
                            output_length < sizeof(output) - 1 ? sizeof(output) - output_length - 1
                                                               : sizeof(discard))) > 0)
  {
    if (output_length < sizeof(output) - 1)
      output_length += (size_t)bytes_read;
  }
  output[output_length] = '\0';
  close(output_pipe[0]);

  CuAssertIntEquals(tc, child_pid, waitpid(child_pid, &child_status, 0));
  CuAssert(tc, "syntax-check child did not exit normally", WIFEXITED(child_status));
  CuAssertIntEquals(tc, EXIT_SUCCESS, WEXITSTATUS(child_status));
  CuAssertPtrNotNull(tc, strstr(output, "Creating encounter reset event"));
  CuAssertPtrEquals(tc, NULL, strstr(output, "event_create called before event_init"));
  CuAssertPtrEquals(tc, NULL, strstr(output, "remove_from_list() called with NULL list pointer"));
  CuAssertPtrNotNull(tc, strstr(output, "Done."));
}
