#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/comm.h"
#include "../../src/db.h"
#include "../../src/dgscript/dg_event.h"
#include "../../src/event_debug.h"
#include "../../src/event_runtime.h"
#include "../../src/mud_event.h"
#include "../../src/mudlim.h"
#include "../../src/perfmon.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int luminari_main(int argc, char **argv);

static struct game_event_result
test_profiled_event_callback(const struct game_event_context *context)
{
  (void)context;
  return game_event_result_complete();
}

static int test_event_cancel_cleanup_calls;
static int test_inflight_owner_cancel_payload_reads;
static int test_mud_event_recurrence_runs;

static void test_event_cancel_cleanup(void *payload)
{
  test_event_cancel_cleanup_calls++;
  free(payload);
}

static MUD_EVENT_CALLBACK(test_inflight_owner_cancel_callback)
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

static MUD_EVENT_CALLBACK(test_mud_event_recurrence_callback)
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
  struct event_runtime_handle *owner;
  int *cleanup_calls;
};

static void event_cleanup_test_destructor(void *payload)
{
  struct event_cleanup_test_data *data;

  data = payload;
  *data->owner = EVENT_RUNTIME_HANDLE_NONE;
  (*data->cleanup_calls)++;
  free(data);
}

static game_event_type_id_t register_cleanup_test_type(CuTest *tc, game_event_cleanup cleanup)
{
  struct game_event_type_config config = {0};
  game_event_type_id_t type;

  config.name = "test_profiled_event_callback";
  config.handler = test_profiled_event_callback;
  config.cleanup = cleanup;
  config.lateness_policy = GAME_EVENT_LATENESS_RUN_ONCE;
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, event_runtime_register_type(&config, &type));
  return type;
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
  struct event_runtime_handle test_event;
  game_event_type_id_t type;
  char report[16384];
  unsigned long saved_pulse;

  saved_pulse = pulse;
  event_free_all();
  event_init();
  PERF_reset();

  type = register_cleanup_test_type(tc, NULL);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    event_runtime_schedule_after(type, 1U, NULL, &test_event));
  pulse++;
  event_test_advance();

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
  struct event_runtime_handle test_event;
  game_event_type_id_t type;
  int *event_obj;

  event_free_all();
  event_init();
  event_obj = malloc(sizeof(*event_obj));
  CuAssertPtrNotNull(tc, event_obj);
  *event_obj = 1;
  type = register_cleanup_test_type(tc, test_event_cancel_cleanup);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    event_runtime_schedule_after(type, 10U, event_obj, &test_event));
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
  first_event = (struct mud_event_data *)descriptor.events->pFirstItem->pContent;
  CuAssertTrue(tc, mud_event_is_live(first_event));
  mud_event_cancel(first_event);
  CuAssertPtrEquals(tc, NULL, descriptor.events);

  memset(&descriptor, 0, sizeof(descriptor));
  descriptor.events = create_list();
  attach_mud_event(new_mud_event(ePROTOCOLS, &descriptor, NULL), 100);
  CuAssertTrue(tc, descriptor.event_owner_generation != first_descriptor_generation);
  clear_descriptor_event_list(&descriptor);
  event_free_all();
}

void Test_mud_event_owners_are_generation_aware_on_native_runtime(CuTest *tc)
{
  verify_mud_event_owner_generation(tc, EVENT_BACKEND_GAME_SCHEDULER);
}

static void verify_inflight_owner_cancel_payload_lifetime(CuTest *tc,
                                                          enum event_backend_kind backend)
{
  struct mud_event_data *mud_event;
  struct obj_data object;
  mud_event_callback_func saved_callback;
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
  CuAssertTrue(tc, mud_event_is_live(mud_event));

  pulse++;
  event_test_advance();
  mud_event_index[eARMOR_SPECAB_BLINDING].func = saved_callback;
  CuAssertPtrEquals(tc, NULL, object.events);
  CuAssertIntEquals(tc, 0, event_queue_depth());
  CuAssertIntEquals(tc, 1, mud_event_test_cleanup_count());
  event_free_all();
  pulse = saved_pulse;
}

void Test_mud_event_inflight_owner_cancel_defers_payload_cleanup_on_native_runtime(CuTest *tc)
{
  test_inflight_owner_cancel_payload_reads = 0;
  verify_inflight_owner_cancel_payload_lifetime(tc, EVENT_BACKEND_GAME_SCHEDULER);
  CuAssertIntEquals(tc, 1, test_inflight_owner_cancel_payload_reads);
}

static void verify_mud_event_terminal_recurrence(CuTest *tc, enum event_backend_kind backend)
{
  struct mud_event_data *mud_event;
  struct obj_data object;
  struct event_runtime_handle runtime_handle;
  mud_event_callback_func saved_callback;
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
  runtime_handle = mud_event->runtime_handle;

  pulse++;
  event_test_advance();
  CuAssertIntEquals(tc, 1, test_mud_event_recurrence_runs);
  CuAssertIntEquals(tc, 0, mud_event_test_cleanup_count());
  CuAssertPtrNotNull(tc, object.events);
  CuAssertTrue(tc, mud_event_is_live(mud_event));
  CuAssertIntEquals(tc, 2, (int)mud_event_remaining(mud_event));

  pulse += 2U;
  event_test_advance();
  mud_event_index[eARMOR_SPECAB_BLINDING].func = saved_callback;
  CuAssertIntEquals(tc, 2, test_mud_event_recurrence_runs);
  CuAssertIntEquals(tc, 1, mud_event_test_cleanup_count());
  CuAssertPtrEquals(tc, NULL, object.events);
  CuAssertTrue(tc, !event_runtime_handle_is_live(runtime_handle));
  event_free_all();
  pulse = saved_pulse;
}

void Test_mud_event_terminal_recurrence_cleans_up_once_on_native_runtime(CuTest *tc)
{
  verify_mud_event_terminal_recurrence(tc, EVENT_BACKEND_GAME_SCHEDULER);
}

void Test_mud_event_native_types_are_entity_filterable(CuTest *tc)
{
  struct event_debug_filter filter;
  struct event_debug_snapshot snapshot;
  struct game_scheduler_stats before;
  struct game_scheduler_stats after;
  struct char_data character;
  size_t returned_count;

  event_free_all();
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER));
  event_init();
  memset(&before, 0, sizeof(before));
  event_runtime_get_stats(&before);

  CuAssertTrue(tc, mud_event_runtime_init());
  memset(&after, 0, sizeof(after));
  event_runtime_get_stats(&after);
  CuAssertIntEquals(tc, eMUD_EVENT_COUNT - ePROTOCOLS,
                    (int)(after.registered_type_count - before.registered_type_count));

  memset(&character, 0, sizeof(character));
  attach_mud_event(new_mud_event(eLAYONHANDS, &character, NULL), 25);
  CuAssertPtrNotNull(tc, character.events);

  memset(&filter, 0, sizeof(filter));
  filter.owner_set = true;
  filter.owner.kind = GAME_EVENT_OWNER_CHARACTER;
  filter.owner.runtime_id = (uint64_t)(uintptr_t)&character;
  filter.type_contains = "mud.";
  CuAssertIntEquals(tc, 1, (int)event_debug_inspect(&filter, &snapshot, 1U, &returned_count));
  CuAssertIntEquals(tc, 1, (int)returned_count);
  CuAssertStrEquals(tc, "mud.004.lay_on_hands", snapshot.type_name);
  CuAssertTrue(tc, snapshot.remaining_pulses == 25U);

  clear_char_event_list(&character);
  event_free_all();
}

void Test_global_event_cleanup_invokes_custom_destructor(CuTest *tc)
{
  struct event_cleanup_test_data *data;
  struct event_runtime_handle owner;
  game_event_type_id_t type;
  int cleanup_calls;

  owner = EVENT_RUNTIME_HANDLE_NONE;
  cleanup_calls = 0;
  event_free_all();
  event_init();

  data = malloc(sizeof(*data));
  CuAssertPtrNotNull(tc, data);
  data->owner = &owner;
  data->cleanup_calls = &cleanup_calls;
  type = register_cleanup_test_type(tc, event_cleanup_test_destructor);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, event_runtime_schedule_after(type, 100U, data, &owner));

  event_free_all();

  CuAssertTrue(tc, event_runtime_handle_is_none(owner));
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
      CuAssertIntEquals(tc, MUD_EVENT_OFFLINE_ELAPSE, policy->offline_policy);
      CuAssertIntEquals(tc, 2, (int)policy->schema_version);
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
                                                  struct player_special_data *specials, long idnum)
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
  struct event_runtime_handle source_handle;
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
  source_handle = source_event->runtime_handle;
  source_generation = source_event->owner.generation;
  CuAssertTrue(tc, !event_runtime_handle_is_none(source_handle));
  CuAssertTrue(tc, event_runtime_handle_is_live(source_handle));
  CuAssertTrue(tc, mud_event_make_durable_record(&source, source_event, 1000, &record));
  CuAssertIntEquals(tc, eLAYONHANDS, record.event_type);
  CuAssertIntEquals(tc, 2, (int)record.schema_version);
  CuAssertIntEquals(tc, 2, record.payload_value);

  clear_char_event_list(&source);
  CuAssertTrue(tc, !event_runtime_handle_is_live(source_handle));
  CuAssertIntEquals(tc, 1, mud_event_test_cleanup_count());
  CuAssertIntEquals(tc, MUD_EVENT_RESTORE_OK,
                    mud_event_restore_character_record(&restored, &record, 1000));
  restored_event = char_has_mud_event(&restored, eLAYONHANDS);
  CuAssertPtrNotNull(tc, restored_event);
  CuAssertIntEquals(tc, 77, (int)mud_event_remaining(restored_event));
  CuAssertStrEquals(tc, "uses:2", restored_event->sVariables);
  CuAssertTrue(tc, !event_runtime_handles_equal(restored_event->runtime_handle, source_handle));
  CuAssertTrue(tc, restored_event->owner.generation != source_generation);
  CuAssertTrue(tc, restored_event->owner.runtime_id == (uint64_t)(uintptr_t)&restored);
  CuAssertIntEquals(tc, MUD_EVENT_RESTORE_DUPLICATE,
                    mud_event_restore_character_record(&restored, &record, 1000));

  clear_char_event_list(&restored);
  clear_char_event_list(&restored);
  CuAssertIntEquals(tc, 2, mud_event_test_cleanup_count());
  event_free_all();
  pulse = saved_pulse;
}

void Test_mud_event_save_preserves_overdue_charge_debt(CuTest *tc)
{
  struct mud_event_durable_record record;
  struct player_special_data specials;
  struct char_data ch;
  struct mud_event_data *event;
  unsigned long saved_pulse = pulse;

  event_free_all();
  pulse = 100U;
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER));
  event_init();
  initialize_persistence_test_character(&ch, &specials, 4244L);
  attach_mud_event(new_mud_event(eLAYONHANDS, &ch, "uses:3"), 5L);
  event = char_has_mud_event(&ch, eLAYONHANDS);
  pulse = 110U;
  CuAssertIntEquals(tc, 0, (int)mud_event_remaining(event));
  CuAssertTrue(tc, mud_event_make_durable_record(&ch, event, 1000, &record));
  CuAssertIntEquals(tc, 3, record.payload_value);
  CuAssertTrue(tc, record.remaining_ticks == 1);
  clear_char_event_list(&ch);
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
  record.schema_version = 2U;
  record.owner_id = 77;
  record.remaining_ticks = 50;
  record.saved_at_epoch = 1000;
  record.payload_value = 1;

  record.schema_version = 3U;
  CuAssertIntEquals(tc, MUD_EVENT_RESTORE_SCHEMA_MISMATCH,
                    mud_event_restore_character_record(&ch, &record, 1100));
  record.schema_version = 2U;
  record.recovery_interval_ticks = -1;
  CuAssertIntEquals(tc, MUD_EVENT_RESTORE_INVALID_FORMAT,
                    mud_event_restore_character_record(&ch, &record, 1100));
  record.recovery_interval_ticks = 864001;
  CuAssertIntEquals(tc, MUD_EVENT_RESTORE_INVALID_FORMAT,
                    mud_event_restore_character_record(&ch, &record, 1100));
  record.recovery_interval_ticks = 0;
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
  record.event_type = eTREATINJURY;
  record.schema_version = 2U;
  record.recovery_interval_ticks = 10;
  CuAssertIntEquals(tc, MUD_EVENT_RESTORE_INVALID_FORMAT,
                    mud_event_restore_character_record(&ch, &record, 1100));
}

static void verify_mud_event_restore_rollback_backend(CuTest *tc, enum event_backend_kind backend)
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
  record.schema_version = 2U;
  record.owner_id = 8080;
  record.remaining_ticks = 25;
  record.saved_at_epoch = 5000;
  record.payload_value = -1;

  CuAssertIntEquals(tc, MUD_EVENT_RESTORE_OK,
                    mud_event_restore_character_record(&ch, &record, 5000));
  restored_event = char_has_mud_event(&ch, eTREATINJURY);
  CuAssertPtrNotNull(tc, restored_event);
  CuAssertIntEquals(tc, backend, event_backend_current());
  CuAssertTrue(tc, mud_event_is_live(restored_event));
  clear_char_event_list(&ch);
  event_free_all();
}

void Test_mud_event_durable_restore_supports_both_timed_backends(CuTest *tc)
{
  verify_mud_event_restore_rollback_backend(tc, EVENT_BACKEND_GAME_SCHEDULER);
}

void Test_mud_event_durable_restore_elapses_offline_and_migrates_schema_one(CuTest *tc)
{
  struct mud_event_durable_record record;
  struct player_special_data specials;
  struct mud_event_data *restored_event;
  struct char_data ch;

  event_free_all();
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER));
  event_init();
  initialize_persistence_test_character(&ch, &specials, 9001L);
  memset(&record, 0, sizeof(record));
  record.event_type = eTREATINJURY;
  record.schema_version = 1U;
  record.owner_id = 9001;
  record.remaining_ticks = 100;
  record.saved_at_epoch = 1000;
  record.payload_value = -1;

  CuAssertIntEquals(tc, MUD_EVENT_RESTORE_OK,
                    mud_event_restore_character_record(&ch, &record, 1005));
  restored_event = char_has_mud_event(&ch, eTREATINJURY);
  CuAssertPtrNotNull(tc, restored_event);
  CuAssertIntEquals(tc, 50, (int)mud_event_remaining(restored_event));
  clear_char_event_list(&ch);

  SPELLBATTLE(&ch) = 8;
  record.event_type = eSPELLBATTLE;
  record.schema_version = 2U;
  record.remaining_ticks = 50;
  CuAssertIntEquals(tc, MUD_EVENT_RESTORE_EXPIRED,
                    mud_event_restore_character_record(&ch, &record, 1005));
  CuAssertIntEquals(tc, 0, SPELLBATTLE(&ch));
  CuAssertPtrEquals(tc, NULL, char_has_mud_event(&ch, eSPELLBATTLE));
  event_free_all();
}

void Test_mud_event_durable_restore_catches_up_staggered_daily_uses(CuTest *tc)
{
  struct mud_event_durable_record record;
  struct player_special_data specials;
  struct mud_event_data *restored_event;
  struct char_data ch;

  event_free_all();
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER));
  event_init();
  initialize_persistence_test_character(&ch, &specials, 9002L);
  memset(&record, 0, sizeof(record));
  record.event_type = eSLA_INVIS;
  record.schema_version = 2U;
  record.owner_id = 9002;
  record.remaining_ticks = 1000;
  record.saved_at_epoch = 2000;
  record.payload_value = 3;

  CuAssertIntEquals(tc, MUD_EVENT_RESTORE_OK,
                    mud_event_restore_character_record(&ch, &record, 2125));
  restored_event = char_has_mud_event(&ch, eSLA_INVIS);
  CuAssertPtrNotNull(tc, restored_event);
  CuAssertIntEquals(tc, 5750, (int)mud_event_remaining(restored_event));
  CuAssertStrEquals(tc, "uses:2", restored_event->sVariables);
  clear_char_event_list(&ch);

  CuAssertIntEquals(tc, MUD_EVENT_RESTORE_EXPIRED,
                    mud_event_restore_character_record(&ch, &record, 3300));
  CuAssertPtrEquals(tc, NULL, char_has_mud_event(&ch, eSLA_INVIS));
  event_free_all();
}

void Test_durable_charge_recovery_preserves_equipped_save_cadence(CuTest *tc)
{
  struct player_special_data specials;
  struct char_data ch;
  struct mud_event_durable_record record;
  struct mud_event_data *event;
  int remaining_uses = -1;
  bool retained_cadence = false;

  event_free_all();
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER));
  event_init();
  initialize_persistence_test_character(&ch, &specials, 9201L);
  GET_REAL_CHA(&ch) = 10;
  GET_CHA(&ch) = 20; /* Effective Charisma while the saved equipment is worn. */
  attach_mud_event(new_mud_event(eCHANNELENERGY, &ch, "uses:3"), PASSES_PER_SEC);
  event = char_has_mud_event(&ch, eCHANNELENERGY);
  CuAssertPtrNotNull(tc, event);
  CuAssertTrue(tc, mud_event_make_durable_record(&ch, event, 1000, &record));
  clear_char_event_list(&ch);

  /* Player-file loading precedes equipment restoration. Two charges must
   * recover using the saved eight-use cadence, not the naked three-use one. */
  GET_CHA(&ch) = GET_REAL_CHA(&ch);
  IN_ROOM(&ch) = NOWHERE;
  mud_event_restore_character_record(&ch, &record, 1002 + SECS_PER_MUD_DAY / 8);
  event = char_has_mud_event(&ch, eCHANNELENERGY);
  if (event != NULL && event->sVariables != NULL)
  {
    sscanf(event->sVariables, "uses:%d", &remaining_uses);
    retained_cadence = mud_event_make_durable_record(&ch, event, 2000, &record) &&
                       record.recovery_interval_ticks == (SECS_PER_MUD_DAY / 8) * PASSES_PER_SEC;
  }
  clear_char_event_list(&ch);
  event_free_all();
  CuAssertIntEquals(tc, 1, remaining_uses);
  CuAssertTrue(tc, retained_cadence);
}

void Test_player_offline_cooldowns_restore_counters_and_staggered_uses(CuTest *tc)
{
  struct player_special_data specials;
  struct char_data ch;
  struct char_data *player;

  initialize_persistence_test_character(&ch, &specials, 9003L);
  player = &ch;
  player->player_specials->saved.mission_cooldown = 10;
  GET_FORAGE_COOLDOWN(player) = 2;
  CALL_EIDOLON_COOLDOWN(player) = 3;
  GET_FIGHT_TO_THE_DEATH_COOLDOWN(player) = 10;
  GET_BONUS_DOMAIN_SLOTS_USED(player) = 2;
  GET_BONUS_DOMAIN_REGEN_TIMER(player) = 4;
  ch.player_specials->saved.moon_bonus_spells_used = 2;
  ch.player_specials->saved.moon_bonus_regen_timer = 2;
  EFREETI_MAGIC_USES(player) = 1;
  EFREETI_MAGIC_TIMER(player) = 2;

  reconcile_player_offline_cooldowns(player, 1000, 1018);
  CuAssertIntEquals(tc, 7, player->player_specials->saved.mission_cooldown);
  CuAssertIntEquals(tc, 0, GET_FORAGE_COOLDOWN(player));
  CuAssertIntEquals(tc, 0, CALL_EIDOLON_COOLDOWN(player));
  CuAssertIntEquals(tc, 0, GET_FIGHT_TO_THE_DEATH_COOLDOWN(player));
  CuAssertIntEquals(tc, 1, GET_BONUS_DOMAIN_SLOTS_USED(player));
  CuAssertIntEquals(tc, 2, GET_BONUS_DOMAIN_REGEN_TIMER(player));
  CuAssertIntEquals(tc, 1, ch.player_specials->saved.moon_bonus_spells_used);
  CuAssertIntEquals(tc, 2999, ch.player_specials->saved.moon_bonus_regen_timer);
  CuAssertIntEquals(tc, 0, EFREETI_MAGIC_TIMER(player));
  CuAssertIntEquals(tc, EFREETI_MAGIC_USES_PER_DAY, EFREETI_MAGIC_USES(player));

  reconcile_player_offline_cooldowns(player, 1018, 1012);
  CuAssertIntEquals(tc, 7, player->player_specials->saved.mission_cooldown);
  reconcile_player_offline_cooldowns(player, 1018, 1018 + 18000);
  CuAssertIntEquals(tc, 0, player->player_specials->saved.mission_cooldown);
  CuAssertIntEquals(tc, 0, GET_BONUS_DOMAIN_SLOTS_USED(player));
  CuAssertIntEquals(tc, 0, ch.player_specials->saved.moon_bonus_spells_used);
  CuAssertIntEquals(tc, 0, ch.player_specials->saved.moon_bonus_regen_timer);
}

void Test_legacy_event_loader_normalizes_payloads_by_policy(CuTest *tc)
{
  struct player_special_data specials;
  struct mud_event_data *event;
  struct char_data ch;
  FILE *fixture;

  fixture = tmpfile();
  CuAssertPtrNotNull(tc, fixture);
  if (fixture == NULL)
    return;

  event_free_all();
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER));
  event_init();
  initialize_persistence_test_character(&ch, &specials, 9191L);
  fprintf(fixture, "%d 25 9\n%d 30 2\n-1\n", eTREATINJURY, eLAYONHANDS);
  rewind(fixture);

  load_legacy_events_for_test(fixture, &ch);
  event = char_has_mud_event(&ch, eTREATINJURY);
  CuAssertPtrNotNull(tc, event);
  if (event != NULL)
    CuAssertPtrEquals(tc, NULL, event->sVariables);
  event = char_has_mud_event(&ch, eLAYONHANDS);
  CuAssertPtrNotNull(tc, event);
  if (event != NULL)
    CuAssertStrEquals(tc, "uses:2", event->sVariables);

  clear_char_event_list(&ch);
  event_free_all();
  fclose(fixture);
}

void Test_durable_event_section_skip_reports_terminator_state(CuTest *tc)
{
  char line[MAX_INPUT_LENGTH + 1];
  FILE *fixture;

  fixture = tmpfile();
  CuAssertPtrNotNull(tc, fixture);
  if (fixture == NULL)
    return;
  fputs("discarded record\n-1\nNext tag\n", fixture);
  rewind(fixture);
  CuAssertTrue(tc, skip_durable_event_section_for_test(fixture));
  CuAssertTrue(tc, get_line(fixture, line));
  CuAssertStrEquals(tc, "Next tag", line);
  fclose(fixture);

  fixture = tmpfile();
  CuAssertPtrNotNull(tc, fixture);
  if (fixture == NULL)
    return;
  fputs("discarded record\n", fixture);
  rewind(fixture);
  CuAssertTrue(tc, !skip_durable_event_section_for_test(fixture));
  fclose(fixture);
}
