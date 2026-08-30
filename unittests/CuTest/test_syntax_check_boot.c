#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/comm.h"
#include "../../src/dgscript/dg_event.h"
#include "../../src/mud_event.h"
#include "../../src/perfmon.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int luminari_main(int argc, char **argv);

static EVENTFUNC(test_profiled_event_callback)
{
  return 0;
}

static int test_event_cancel_cleanup_calls;
static int test_inflight_owner_cancel_payload_reads;

static void test_event_cancel_cleanup(struct event *event)
{
  test_event_cancel_cleanup_calls++;
  free(event->event_obj);
}

static EVENTFUNC(test_inflight_owner_cancel_callback)
{
  struct mud_event_data *mud_event;
  struct obj_data *object;

  mud_event = (struct mud_event_data *)event_obj;
  object = (struct obj_data *)mud_event->pStruct;
  clear_obj_event_list(object);
  if (mud_event->sVariables != NULL && strcmp(mud_event->sVariables, "payload-live") == 0)
    test_inflight_owner_cancel_payload_reads++;
  return 10;
}

#define SYNTAX_CHECK_OUTPUT_SIZE (1024 * 1024)
#define SYNTAX_CHECK_DEFAULT_TIMEOUT_SECONDS 60U
#define SYNTAX_CHECK_MAX_TIMEOUT_SECONDS 600UL

struct event_cleanup_test_data
{
  struct event **owner;
  int *cleanup_calls;
};

static EVENTFUNC(event_cleanup_test_callback)
{
  return 0;
}

static void event_cleanup_test_destructor(struct event *event)
{
  struct event_cleanup_test_data *data;

  data = (struct event_cleanup_test_data *)event->event_obj;
  *data->owner = NULL;
  (*data->cleanup_calls)++;
  free(data);
}

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

void Test_event_process_reports_registered_callback_identity(CuTest *tc)
{
  struct event *test_event;
  char report[16384];
  unsigned long saved_pulse;

  saved_pulse = pulse;
  event_free_all();
  event_init();
  PERF_reset();

  test_event = event_create(test_profiled_event_callback, NULL, 1);
  CuAssertPtrNotNull(tc, test_event);
  pulse++;
  event_process();

  PERF_prof_repr_csv(report, sizeof(report));
  CuAssertPtrNotNull(tc, strstr(report, "# event_process_calls=1"));
  CuAssertPtrNotNull(tc, strstr(report, "# event_queue_depth_initial=1"));
  CuAssertPtrNotNull(tc, strstr(report, "# event_queue_depth_latest=0"));
  CuAssertPtrNotNull(tc, strstr(report, "test_profiled_event_callback,1,"));

  event_free_all();
  pulse = saved_pulse;
}

void Test_event_free_all_runs_specialized_cancel_cleanup(CuTest *tc)
{
  struct event *test_event;
  int *event_obj;

  event_free_all();
  event_init();
  event_obj = malloc(sizeof(*event_obj));
  CuAssertPtrNotNull(tc, event_obj);
  *event_obj = 1;
  test_event = event_create_with_cleanup(test_profiled_event_callback, event_obj, 10,
                                         test_event_cancel_cleanup);
  CuAssertPtrNotNull(tc, test_event);
  test_event_cancel_cleanup_calls = 0;

  event_free_all();

  CuAssertIntEquals(tc, 1, test_event_cancel_cleanup_calls);
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

static void verify_mud_event_owner_generation(CuTest *tc, enum event_backend_kind backend)
{
  struct game_event_owner first_owner;
  struct descriptor_data descriptor;
  struct obj_data object;
  uint64_t first_descriptor_generation;

  event_free_all();
  CuAssertIntEquals(tc, 1, event_test_select_backend(backend));
  event_init();

  memset(&object, 0, sizeof(object));
  attach_mud_event(new_mud_event(eARMOR_SPECAB_BLINDING, &object, NULL), 100);
  attach_mud_event(new_mud_event(eITEM_SPECAB_HORN_OF_SUMMONING, &object, NULL), 100);
  CuAssertPtrNotNull(tc, object.events);
  CuAssertIntEquals(tc, 2, object.events->iSize);
  CuAssertTrue(tc, object.event_owner_generation != 0);
  first_owner = ((struct event *)object.events->pFirstItem->pContent)->owner;
  CuAssertIntEquals(tc, GAME_EVENT_OWNER_OBJECT, first_owner.kind);
  CuAssertTrue(tc, first_owner.runtime_id == (uint64_t)(uintptr_t)&object);
  CuAssertTrue(tc, first_owner.generation == object.event_owner_generation);
  CuAssertTrue(tc, game_event_owner_equal(
                       first_owner,
                       ((struct event *)object.events->pLastItem->pContent)->owner));
  clear_obj_event_list(&object);
  CuAssertPtrEquals(tc, NULL, object.events);
  CuAssertIntEquals(tc, 0, event_queue_depth());

  memset(&object, 0, sizeof(object));
  attach_mud_event(new_mud_event(eARMOR_SPECAB_BLINDING, &object, NULL), 100);
  CuAssertPtrNotNull(tc, object.events);
  CuAssertTrue(tc, object.event_owner_generation != first_owner.generation);
  clear_obj_event_list(&object);

  memset(&descriptor, 0, sizeof(descriptor));
  descriptor.events = create_list();
  attach_mud_event(new_mud_event(ePROTOCOLS, &descriptor, NULL), 100);
  CuAssertPtrNotNull(tc, descriptor.events);
  CuAssertIntEquals(tc, 1, descriptor.events->iSize);
  first_descriptor_generation = descriptor.event_owner_generation;
  clear_descriptor_event_list(&descriptor);
  CuAssertPtrEquals(tc, NULL, descriptor.events);

  memset(&descriptor, 0, sizeof(descriptor));
  descriptor.events = create_list();
  attach_mud_event(new_mud_event(ePROTOCOLS, &descriptor, NULL), 100);
  CuAssertTrue(tc, descriptor.event_owner_generation != first_descriptor_generation);
  clear_descriptor_event_list(&descriptor);
  event_free_all();
}

void Test_mud_event_owners_are_generation_aware_on_both_backends(CuTest *tc)
{
  verify_mud_event_owner_generation(tc, EVENT_BACKEND_LEGACY_QUEUE);
  verify_mud_event_owner_generation(tc, EVENT_BACKEND_GAME_SCHEDULER);
}

static void verify_inflight_owner_cancel_payload_lifetime(CuTest *tc,
                                                          enum event_backend_kind backend)
{
  struct game_event_owner owner;
  struct mud_event_data *mud_event;
  struct obj_data object;
  struct event *event;
  unsigned long saved_pulse;

  saved_pulse = pulse;
  event_free_all();
  CuAssertIntEquals(tc, 1, event_test_select_backend(backend));
  event_init();
  memset(&object, 0, sizeof(object));
  object.event_owner_generation = backend + 100U;
  object.events = create_list();
  owner.kind = GAME_EVENT_OWNER_OBJECT;
  owner.runtime_id = (uint64_t)(uintptr_t)&object;
  owner.generation = object.event_owner_generation;
  mud_event = new_mud_event(eARMOR_SPECAB_BLINDING, &object, "payload-live");
  mud_event->owner = owner;
  event = event_create_owned_named(test_inflight_owner_cancel_callback, mud_event, 1,
                                   "in-flight owner cancellation", owner);
  CuAssertPtrNotNull(tc, event);
  event->isMudEvent = TRUE;
  mud_event->pEvent = event;
  add_to_list(event, object.events);

  pulse++;
  event_process();
  CuAssertPtrEquals(tc, NULL, object.events);
  CuAssertIntEquals(tc, 0, event_queue_depth());
  event_free_all();
  pulse = saved_pulse;
}

void Test_mud_event_inflight_owner_cancel_defers_payload_cleanup_on_both_backends(CuTest *tc)
{
  test_inflight_owner_cancel_payload_reads = 0;
  verify_inflight_owner_cancel_payload_lifetime(tc, EVENT_BACKEND_LEGACY_QUEUE);
  verify_inflight_owner_cancel_payload_lifetime(tc, EVENT_BACKEND_GAME_SCHEDULER);
  CuAssertIntEquals(tc, 2, test_inflight_owner_cancel_payload_reads);
}

void Test_global_event_cleanup_invokes_custom_destructor(CuTest *tc)
{
  struct event_cleanup_test_data *data;
  struct event *owner;
  int cleanup_calls;

  owner = NULL;
  cleanup_calls = 0;
  event_free_all();
  event_init();

  data = malloc(sizeof(*data));
  CuAssertPtrNotNull(tc, data);
  data->owner = &owner;
  data->cleanup_calls = &cleanup_calls;
  owner = event_create_with_cleanup(event_cleanup_test_callback, data, 100,
                                    event_cleanup_test_destructor);
  CuAssertPtrNotNull(tc, owner);

  event_free_all();

  CuAssertPtrEquals(tc, NULL, owner);
  CuAssertIntEquals(tc, 1, cleanup_calls);
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

    argv[0] = (char *)"luminari";
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
  size_t i;

  CuAssertIntEquals(tc, eMUD_EVENT_COUNT, (int)mud_event_index_count);
  CuAssertStrEquals(tc, "Dragon Attack Cooldown",
                    mud_event_index[eDRAGON_ATTACK_COOLDOWN].event_name);
  CuAssertTrue(tc, mud_event_index[eDRAGON_ATTACK_COOLDOWN].func == event_countdown);

  for (i = 1; i < mud_event_index_count; i++)
  {
    if (mud_event_index[i].func != event_daily_use_cooldown)
      continue;

    CuAssert(tc, mud_event_index[i].event_name,
             mud_event_index[i].feat_num != FEAT_UNDEFINED || mud_event_index[i].daily_uses > 0);
  }
}
