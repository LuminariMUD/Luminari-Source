#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/dgscript/dg_event.h"
#include "../../src/mud_event.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int luminari_main(int argc, char **argv);

#define SYNTAX_CHECK_OUTPUT_SIZE (1024 * 1024)
#define SYNTAX_CHECK_DEFAULT_TIMEOUT_SECONDS 60U
#define SYNTAX_CHECK_MAX_TIMEOUT_SECONDS 600UL

static const char *syntax_check_test_root(void)
{
  const char *root;

  root = getenv("LUMINARI_TEST_ROOT");
  return root != NULL && *root != '\0' ? root : ".";
}

static const char *syntax_check_data_dir(void)
{
  const char *data_dir;

  data_dir = getenv("LUMINARI_TEST_DATA_DIR");
  return data_dir != NULL && *data_dir != '\0' ? data_dir : NULL;
}

static const char *syntax_check_config_file(void)
{
  const char *config_file;

  config_file = getenv("LUMINARI_TEST_CONFIG_FILE");
  return config_file != NULL && *config_file != '\0' ? config_file : NULL;
}

static unsigned int syntax_check_timeout_seconds(void)
{
  const char *timeout_text;
  char *end;
  unsigned long timeout;

  timeout_text = getenv("LUMINARI_TEST_SYNTAX_TIMEOUT_SECONDS");
  if (timeout_text == NULL || *timeout_text == '\0')
    return SYNTAX_CHECK_DEFAULT_TIMEOUT_SECONDS;

  end = NULL;
  timeout = strtoul(timeout_text, &end, 10);
  if (end == timeout_text || *end != '\0' || timeout == 0 ||
      timeout > SYNTAX_CHECK_MAX_TIMEOUT_SECONDS)
    return SYNTAX_CHECK_DEFAULT_TIMEOUT_SECONDS;

  return (unsigned int)timeout;
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

void Test_global_event_cleanup_detaches_live_object_owner(CuTest *tc)
{
  struct obj_data obj;

  memset(&obj, 0, sizeof(obj));
  event_free_all();
  event_init();

  attach_mud_event(new_mud_event(eARMOR_SPECAB_BLINDING, &obj, NULL), 100);
  CuAssertPtrNotNull(tc, obj.events);

  event_free_all();

  CuAssertPtrEquals(tc, NULL, obj.events);
}

void Test_syntax_check_encounter_world_boots_and_cleans_up_once(CuTest *tc)
{
  char data_dir[PATH_MAX];
  char output[SYNTAX_CHECK_OUTPUT_SIZE];
  char *argv[10];
  const char *config_file;
  const char *configured_data_dir;
  const char *root;
  ssize_t bytes_read;
  char discard[4096];
  size_t output_length;
  int output_pipe[2];
  int child_status;
  pid_t child_pid;

  if (getenv("LUMINARI_TEST_SKIP_SYNTAX_BOOT") != NULL)
    return;

  configured_data_dir = syntax_check_data_dir();
  if (configured_data_dir != NULL)
    CuAssert(tc, "syntax-check data path is too long",
             snprintf(data_dir, sizeof(data_dir), "%s", configured_data_dir) <
                 (int)sizeof(data_dir));
  else
  {
    root = syntax_check_test_root();
    CuAssert(tc, "syntax-check data path is too long",
             snprintf(data_dir, sizeof(data_dir), "%s/lib", root) < (int)sizeof(data_dir));
  }
  config_file = syntax_check_config_file();
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
    if (config_file != NULL)
    {
      argv[1] = (char *)"-f";
      argv[2] = (char *)config_file;
      argv[3] = (char *)"-c";
      argv[4] = (char *)"-q";
      argv[5] = (char *)"-d";
      argv[6] = data_dir;
      argv[7] = NULL;
      argv[8] = NULL;
      argv[9] = NULL;
    }
    else
    {
      argv[1] = (char *)"-c";
      argv[2] = (char *)"-q";
      argv[3] = (char *)"-d";
      argv[4] = data_dir;
      argv[5] = NULL;
      argv[6] = NULL;
      argv[7] = NULL;
      argv[8] = NULL;
      argv[9] = NULL;
    }

    alarm(syntax_check_timeout_seconds());
    event_test_reset_lifecycle_counts();
    result = luminari_main(config_file != NULL ? 7 : 5, argv);
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
  if (!WIFEXITED(child_status) || WEXITSTATUS(child_status) != EXIT_SUCCESS)
    fprintf(stderr, "Syntax-check child output:\n%s\n", output);
  CuAssert(tc, "syntax-check child did not exit normally", WIFEXITED(child_status));
  CuAssertIntEquals(tc, EXIT_SUCCESS, WEXITSTATUS(child_status));
  CuAssertPtrNotNull(tc, strstr(output, "Creating encounter reset event"));
  CuAssertPtrEquals(tc, NULL, strstr(output, "event_create called before event_init"));
  CuAssertPtrEquals(tc, NULL, strstr(output, "remove_from_list() called with NULL list pointer"));
  CuAssertPtrNotNull(tc, strstr(output, "Done."));
}
void Test_mud_event_registry_matches_enum(CuTest *tc)
{
  CuAssertIntEquals(tc, eMUD_EVENT_COUNT, (int)mud_event_index_count);
  CuAssertStrEquals(tc, "Dragon Attack Cooldown",
                    mud_event_index[eDRAGON_ATTACK_COOLDOWN].event_name);
  CuAssertTrue(tc, mud_event_index[eDRAGON_ATTACK_COOLDOWN].func == event_countdown);
}
