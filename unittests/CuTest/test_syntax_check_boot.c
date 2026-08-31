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
static int test_mud_event_recurrence_runs;

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

static EVENTFUNC(test_mud_event_recurrence_callback)
{
  struct mud_event_data *mud_event;

  mud_event = (struct mud_event_data *)event_obj;
  if (mud_event != NULL && mud_event->sVariables != NULL &&
      strcmp(mud_event->sVariables, "recurrence-live") == 0)
    test_mud_event_recurrence_runs++;
  return test_mud_event_recurrence_runs % 2 == 1 ? 2 : 0;
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
  struct mud_event_data *first_event;
  struct mud_event_data *last_event;
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
  first_event = (struct mud_event_data *)object.events->pFirstItem->pContent;
  last_event = (struct mud_event_data *)object.events->pLastItem->pContent;
  first_owner = first_event->owner;
  CuAssertIntEquals(tc, GAME_EVENT_OWNER_OBJECT, first_owner.kind);
  CuAssertTrue(tc, first_owner.runtime_id == (uint64_t)(uintptr_t)&object);
  CuAssertTrue(tc, first_owner.generation == object.event_owner_generation);
  CuAssertTrue(tc, game_event_owner_equal(first_owner, last_event->owner));
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
  struct mud_event_data *mud_event;
  struct obj_data object;
  EVENTFUNC(*saved_callback);
  unsigned long saved_pulse;

  saved_pulse = pulse;
  event_free_all();
  CuAssertIntEquals(tc, 1, event_test_select_backend(backend));
  event_init();
  memset(&object, 0, sizeof(object));
  mud_event_test_reset_cleanup_count();
  saved_callback = mud_event_index[eARMOR_SPECAB_BLINDING].func;
  mud_event_index[eARMOR_SPECAB_BLINDING].func = test_inflight_owner_cancel_callback;
  mud_event = new_mud_event(eARMOR_SPECAB_BLINDING, &object, "payload-live");
  attach_mud_event(mud_event, 1);
  CuAssertPtrNotNull(tc, object.events);
  CuAssertTrue(tc, mud_event->event_handle != EVENT_HANDLE_NONE);

  pulse++;
  event_process();
  mud_event_index[eARMOR_SPECAB_BLINDING].func = saved_callback;
  CuAssertPtrEquals(tc, NULL, object.events);
  CuAssertIntEquals(tc, 0, event_queue_depth());
  CuAssertIntEquals(tc, 1, mud_event_test_cleanup_count());
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

static void verify_mud_event_terminal_recurrence(CuTest *tc,
                                                 enum event_backend_kind backend)
{
  struct mud_event_data *mud_event;
  struct obj_data object;
  event_handle_t handle;
  EVENTFUNC(*saved_callback);
  unsigned long saved_pulse;

  saved_pulse = pulse;
  event_free_all();
  CuAssertIntEquals(tc, 1, event_test_select_backend(backend));
  event_init();
  memset(&object, 0, sizeof(object));
  mud_event_test_reset_cleanup_count();
  test_mud_event_recurrence_runs = 0;
  saved_callback = mud_event_index[eARMOR_SPECAB_BLINDING].func;
  mud_event_index[eARMOR_SPECAB_BLINDING].func = test_mud_event_recurrence_callback;
  mud_event = new_mud_event(eARMOR_SPECAB_BLINDING, &object, "recurrence-live");
  attach_mud_event(mud_event, 1);
  handle = mud_event->event_handle;

  pulse++;
  event_process();
  CuAssertIntEquals(tc, 1, test_mud_event_recurrence_runs);
  CuAssertIntEquals(tc, 0, mud_event_test_cleanup_count());
  CuAssertPtrNotNull(tc, object.events);
  CuAssertTrue(tc, event_handle_is_live(handle));
  CuAssertIntEquals(tc, 2, (int)event_handle_time(handle));

  pulse += 2U;
  event_process();
  mud_event_index[eARMOR_SPECAB_BLINDING].func = saved_callback;
  CuAssertIntEquals(tc, 2, test_mud_event_recurrence_runs);
  CuAssertIntEquals(tc, 1, mud_event_test_cleanup_count());
  CuAssertPtrEquals(tc, NULL, object.events);
  CuAssertTrue(tc, !event_handle_is_live(handle));
  event_free_all();
  pulse = saved_pulse;
}

void Test_mud_event_terminal_recurrence_cleans_up_once_on_both_backends(CuTest *tc)
{
  verify_mud_event_terminal_recurrence(tc, EVENT_BACKEND_LEGACY_QUEUE);
  verify_mud_event_terminal_recurrence(tc, EVENT_BACKEND_GAME_SCHEDULER);
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
  CuAssertPtrNotNull(tc, strstr(output, "Combat round scheduling:"));
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

void Test_mud_event_persistence_policy_classifies_entire_registry(CuTest *tc)
{
  const struct mud_event_persistence_policy *policy;
  size_t persisted;
  size_t reconstructable;
  size_t transient;
  size_t i;

  persisted = 0;
  reconstructable = 0;
  transient = 0;
  for (i = 0; i < mud_event_index_count; i++)
  {
    policy = mud_event_persistence_policy((event_id)i);
    CuAssertPtrNotNull(tc, policy);
    switch (policy->storage_class)
    {
    case MUD_EVENT_TRANSIENT:
      transient++;
      CuAssertIntEquals(tc, MUD_EVENT_OFFLINE_DISCARD, policy->offline_policy);
      CuAssertIntEquals(tc, 0, (int)policy->schema_version);
      break;
    case MUD_EVENT_RECONSTRUCTABLE:
      reconstructable++;
      CuAssertIntEquals(tc, MUD_EVENT_OFFLINE_RECONSTRUCT, policy->offline_policy);
      break;
    case MUD_EVENT_PERSISTED:
      persisted++;
      CuAssertIntEquals(tc, MUD_EVENT_OFFLINE_PAUSE, policy->offline_policy);
      CuAssertIntEquals(tc, 1, (int)policy->schema_version);
      CuAssertIntEquals(tc, EVENT_CHAR, mud_event_index[i].iEvent_Type);
      break;
    case MUD_EVENT_COPYOVER_PRESERVED:
    default:
      CuFail(tc, "Unexpected or invalid MUD event persistence class");
      return;
    }
  }

  CuAssertIntEquals(tc, 93, (int)persisted);
  CuAssertIntEquals(tc, 1, (int)reconstructable);
  CuAssertTrue(tc, transient + persisted + reconstructable == mud_event_index_count);
  CuAssertIntEquals(tc, MUD_EVENT_RECONSTRUCTABLE,
                    mud_event_persistence_policy(eENCOUNTER_REG_RESET)->storage_class);
  CuAssertIntEquals(tc, MUD_EVENT_TRANSIENT,
                    mud_event_persistence_policy(ePROTOCOLS)->storage_class);
  CuAssertIntEquals(tc, MUD_EVENT_TRANSIENT,
                    mud_event_persistence_policy(eCOMBAT_ROUND)->storage_class);
}

static void initialize_persistence_test_character(struct char_data *ch,
                                                  struct player_special_data *specials,
                                                  long idnum)
{
  memset(ch, 0, sizeof(*ch));
  memset(specials, 0, sizeof(*specials));
  ch->player_specials = specials;
  GET_IDNUM(ch) = idnum;
}

void Test_mud_event_durable_restore_rehydrates_fresh_runtime_identity(CuTest *tc)
{
  struct mud_event_durable_record record;
  struct player_special_data source_specials;
  struct player_special_data restored_specials;
  struct mud_event_data *source_event;
  struct mud_event_data *restored_event;
  struct char_data source;
  struct char_data restored;
  event_handle_t source_handle;
  uint64_t source_generation;
  unsigned long saved_pulse;

  saved_pulse = pulse;
  event_free_all();
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER));
  event_init();
  initialize_persistence_test_character(&source, &source_specials, 4242L);
  initialize_persistence_test_character(&restored, &restored_specials, 4242L);
  mud_event_test_reset_cleanup_count();

  attach_mud_event(new_mud_event(eLAYONHANDS, &source, "uses:2"), 77);
  source_event = char_has_mud_event(&source, eLAYONHANDS);
  CuAssertPtrNotNull(tc, source_event);
  source_handle = source_event->event_handle;
  source_generation = source_event->owner.generation;
  CuAssertTrue(tc, source_handle != EVENT_HANDLE_NONE);
  CuAssertTrue(tc, event_handle_is_live(source_handle));
  CuAssertTrue(tc, mud_event_make_durable_record(&source, source_event, 1000, &record));
  CuAssertIntEquals(tc, eLAYONHANDS, record.event_type);
  CuAssertIntEquals(tc, 2, record.payload_value);

  clear_char_event_list(&source);
  CuAssertTrue(tc, !event_handle_is_live(source_handle));
  CuAssertIntEquals(tc, 1, mud_event_test_cleanup_count());
  CuAssertIntEquals(tc, MUD_EVENT_RESTORE_OK,
                    mud_event_restore_character_record(&restored, &record, 87400));
  restored_event = char_has_mud_event(&restored, eLAYONHANDS);
  CuAssertPtrNotNull(tc, restored_event);
  CuAssertIntEquals(tc, 77, (int)event_handle_time(restored_event->event_handle));
  CuAssertStrEquals(tc, "uses:2", restored_event->sVariables);
  CuAssertTrue(tc, restored_event->event_handle != source_handle);
  CuAssertTrue(tc, restored_event->owner.generation != source_generation);
  CuAssertTrue(tc, restored_event->owner.runtime_id == (uint64_t)(uintptr_t)&restored);
  CuAssertIntEquals(tc, MUD_EVENT_RESTORE_DUPLICATE,
                    mud_event_restore_character_record(&restored, &record, 87400));

  clear_char_event_list(&restored);
  clear_char_event_list(&restored);
  CuAssertIntEquals(tc, 2, mud_event_test_cleanup_count());
  event_free_all();
  pulse = saved_pulse;
}

void Test_mud_event_durable_restore_rejects_invalid_records(CuTest *tc)
{
  struct mud_event_durable_record record;
  struct player_special_data specials;
  struct char_data ch;

  initialize_persistence_test_character(&ch, &specials, 77L);
  memset(&record, 0, sizeof(record));
  record.event_type = eLAYONHANDS;
  record.schema_version = 1U;
  record.owner_id = 77;
  record.remaining_ticks = 50;
  record.saved_at_epoch = 1000;
  record.payload_value = 1;

  record.schema_version = 2U;
  CuAssertIntEquals(tc, MUD_EVENT_RESTORE_SCHEMA_MISMATCH,
                    mud_event_restore_character_record(&ch, &record, 1100));
  record.schema_version = 1U;
  record.owner_id = 78;
  CuAssertIntEquals(tc, MUD_EVENT_RESTORE_OWNER_MISMATCH,
                    mud_event_restore_character_record(&ch, &record, 1100));
  record.owner_id = 77;
  record.payload_value = 0;
  CuAssertIntEquals(tc, MUD_EVENT_RESTORE_PAYLOAD_MALFORMED,
                    mud_event_restore_character_record(&ch, &record, 1100));
  record.payload_value = 1;
  record.saved_at_epoch = 2000;
  CuAssertIntEquals(tc, MUD_EVENT_RESTORE_INVALID_FORMAT,
                    mud_event_restore_character_record(&ch, &record, 1100));
  record.saved_at_epoch = 1000;
  record.event_type = eCOMBAT_ROUND;
  record.schema_version = 0U;
  record.payload_value = -1;
  CuAssertIntEquals(tc, MUD_EVENT_RESTORE_CLASS_MISMATCH,
                    mud_event_restore_character_record(&ch, &record, 1100));
  record.event_type = eMUD_EVENT_COUNT;
  CuAssertIntEquals(tc, MUD_EVENT_RESTORE_UNKNOWN_TYPE,
                    mud_event_restore_character_record(&ch, &record, 1100));
}

static void verify_mud_event_restore_rollback_backend(CuTest *tc,
                                                     enum event_backend_kind backend)
{
  struct mud_event_durable_record record;
  struct player_special_data specials;
  struct mud_event_data *restored_event;
  struct char_data ch;

  event_free_all();
  CuAssertIntEquals(tc, 1, event_test_select_backend(backend));
  event_init();
  initialize_persistence_test_character(&ch, &specials, 8080L);
  memset(&record, 0, sizeof(record));
  record.event_type = eTREATINJURY;
  record.schema_version = 1U;
  record.owner_id = 8080;
  record.remaining_ticks = 25;
  record.saved_at_epoch = 5000;
  record.payload_value = -1;

  CuAssertIntEquals(tc, MUD_EVENT_RESTORE_OK,
                    mud_event_restore_character_record(&ch, &record, 5000));
  restored_event = char_has_mud_event(&ch, eTREATINJURY);
  CuAssertPtrNotNull(tc, restored_event);
  CuAssertIntEquals(tc, backend, event_backend_current());
  CuAssertTrue(tc, event_handle_is_live(restored_event->event_handle));
  clear_char_event_list(&ch);
  event_free_all();
}

void Test_mud_event_durable_restore_supports_both_timed_backends(CuTest *tc)
{
  verify_mud_event_restore_rollback_backend(tc, EVENT_BACKEND_GAME_SCHEDULER);
  verify_mud_event_restore_rollback_backend(tc, EVENT_BACKEND_LEGACY_QUEUE);
}
