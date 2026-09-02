#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/comm.h"
#include "../../src/dgscript/dg_event.h"
#include "../../src/dgscript/dg_scripts.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DG_SOURCE_LIMIT (2L * 1024L * 1024L)

static bool dg_read_source(const char *relative_path, char **text)
{
  const char *root;
  FILE *file;
  char path[PATH_MAX];
  char *buffer;
  long source_length;
  size_t bytes_read;
  bool success;

  *text = NULL;
  root = getenv("LUMINARI_TEST_ROOT");
  if (root == NULL || *root == '\0')
    root = ".";
  if (snprintf(path, sizeof(path), "%s/%s", root, relative_path) >= (int)sizeof(path))
    return false;

  file = fopen(path, "rb");
  if (file == NULL)
    return false;
  success = fseek(file, 0, SEEK_END) == 0;
  source_length = success ? ftell(file) : -1;
  if (source_length < 0 || source_length > DG_SOURCE_LIMIT || fseek(file, 0, SEEK_SET) != 0)
    success = false;

  buffer = NULL;
  if (success)
  {
    buffer = malloc((size_t)source_length + 1);
    success = buffer != NULL;
  }
  if (success)
  {
    bytes_read = fread(buffer, 1, (size_t)source_length, file);
    success = bytes_read == (size_t)source_length && ferror(file) == 0;
    buffer[bytes_read] = '\0';
  }
  if (fclose(file) != 0)
    success = false;

  if (!success)
  {
    free(buffer);
    return false;
  }

  *text = buffer;
  return true;
}

void Test_dg_production_text_matching_helpers(CuTest *tc)
{
  char phrase_input[] = "\"two words\" remainder";
  char phrase[MAX_INPUT_LENGTH];
  char *remainder;

  CuAssertTrue(tc, is_substring("needle", "a needle in text"));
  CuAssertTrue(tc, !is_substring("missing", "a needle in text"));
  CuAssertTrue(tc, word_check("alpha beta gamma", "delta beta"));
  CuAssertTrue(tc, !word_check("alpha beta", "gamma delta"));

  remainder = one_phrase(phrase_input, phrase);
  CuAssertStrEquals(tc, "two words", phrase);
  CuAssertStrEquals(tc, " remainder", remainder);
}

void Test_dg_production_variable_lifecycle(CuTest *tc)
{
  struct trig_var_data *variables;

  variables = NULL;
  add_var(&variables, "coverage_name", "first", 17);
  CuAssertPtrNotNull(tc, variables);
  CuAssertStrEquals(tc, "coverage_name", variables->name);
  CuAssertStrEquals(tc, "first", variables->value);
  CuAssertIntEquals(tc, 17, (int)variables->context);

  add_var(&variables, "coverage_name", "updated", 17);
  CuAssertStrEquals(tc, "updated", variables->value);
  CuAssertTrue(tc, remove_var(&variables, "coverage_name"));
  CuAssertPtrEquals(tc, NULL, variables);

  free_varlist(variables);
}

void Test_dg_production_typed_script_field_lifecycle(CuTest *tc)
{
  struct script_data *script = NULL;
  struct trig_proto_list source_first = {0};
  struct trig_proto_list source_second = {0};
  struct trig_proto_list *copy = NULL;

  CREATE(script, struct script_data, 1);
  add_var(&script->global_vars, "typed_owner", "safe", 0);
  extract_script(&script);
  CuAssertPtrEquals(tc, NULL, script);

  source_first.vnum = 101;
  source_first.next = &source_second;
  source_second.vnum = 202;
  copy_proto_script(&source_first, &copy);

  CuAssertPtrNotNull(tc, copy);
  CuAssertIntEquals(tc, source_first.vnum, copy->vnum);
  CuAssertPtrNotNull(tc, copy->next);
  CuAssertIntEquals(tc, source_second.vnum, copy->next->vnum);
  CuAssertPtrEquals(tc, NULL, copy->next->next);

  free_proto_script(&copy);
  CuAssertPtrEquals(tc, NULL, copy);
}

void Test_dg_production_flag_name_matching(CuTest *tc)
{
  int flags[4];
  const char *names[] = {"zero", "one", "two", "three", "\n"};

  memset(flags, 0, sizeof(flags));
  SET_BIT_AR(flags, 2);

  CuAssertTrue(tc, check_flags_by_name_ar(flags, 4, "two", names));
  CuAssertTrue(tc, !check_flags_by_name_ar(flags, 4, "one", names));
}

void Test_dg_wait_resume_does_not_scan_global_owner_lists(CuTest *tc)
{
  char *source;
  char *callback;
  char *callback_end;
  char saved_end;
  bool source_loaded;
  bool callback_bounded;

  source = NULL;
  source_loaded = dg_read_source("src/dgscript/dg_scripts.c", &source);
  callback_bounded = false;
  if (source_loaded)
  {
    callback = strstr(source, "static void resume_trig_wait");
    callback_end = callback != NULL
                       ? strstr(callback, "static struct game_event_result trig_wait_event")
                       : NULL;
    if (callback != NULL && callback_end != NULL)
    {
      saved_end = *callback_end;
      *callback_end = '\0';
      callback_bounded = strstr(callback, "script_driver(&restart_args);") != NULL &&
                         strstr(callback, "character_list") == NULL &&
                         strstr(callback, "object_list") == NULL &&
                         strstr(callback, "top_of_world") == NULL;
      *callback_end = saved_end;
    }
  }
  free(source);

  CuAssertTrue(tc, source_loaded);
  CuAssertTrue(tc, callback_bounded);
}

static void verify_stale_wait_replacement(CuTest *tc, enum event_backend_kind backend)
{
  struct trig_data *trigger;
  struct event_runtime_handle stale_runtime_handle;
  struct event_runtime_handle active_runtime_handle;
  event_handle_t stale_rollback_handle;
  event_handle_t active_rollback_handle;

  event_free_all();
  CuAssertIntEquals(tc, 1, event_test_select_backend(backend));
  event_init();
  dg_wait_reset_telemetry_for_test();
  CREATE(trigger, struct trig_data, 1);
  trigger->name = strdup("stale wait replacement");
  CuAssertPtrNotNull(tc, trigger->name);

  CuAssertTrue(tc, dg_wait_schedule_for_test(trigger, 1L));
  stale_runtime_handle = GET_TRIG_WAIT_HANDLE(trigger);
  stale_rollback_handle = trigger->wait_rollback_handle;
  CuAssertTrue(tc, dg_wait_schedule_for_test(trigger, 10L));
  active_runtime_handle = GET_TRIG_WAIT_HANDLE(trigger);
  active_rollback_handle = trigger->wait_rollback_handle;
  if (backend == EVENT_BACKEND_GAME_SCHEDULER)
  {
    CuAssertTrue(tc, !event_runtime_handle_is_none(stale_runtime_handle));
    CuAssertTrue(tc, !event_runtime_handle_is_none(active_runtime_handle));
    CuAssertTrue(tc, !event_runtime_handles_equal(stale_runtime_handle, active_runtime_handle));
    CuAssertTrue(tc, !event_runtime_handle_is_live(stale_runtime_handle));
  }
  else
  {
    CuAssertTrue(tc, stale_rollback_handle != EVENT_HANDLE_NONE);
    CuAssertTrue(tc, active_rollback_handle != EVENT_HANDLE_NONE);
    CuAssertTrue(tc, stale_rollback_handle != active_rollback_handle);
    CuAssertTrue(tc, !event_handle_is_live(stale_rollback_handle));
  }

  pulse++;
  event_process();
  CuAssertIntEquals(tc, 0, (int)dg_wait_resume_count_for_test());
  if (backend == EVENT_BACKEND_GAME_SCHEDULER)
  {
    CuAssertTrue(tc, !event_runtime_handle_is_live(stale_runtime_handle));
    CuAssertTrue(tc, event_runtime_handle_is_live(active_runtime_handle));
    CuAssertTrue(tc,
                 event_runtime_handles_equal(GET_TRIG_WAIT_HANDLE(trigger), active_runtime_handle));
  }
  else
  {
    CuAssertTrue(tc, !event_handle_is_live(stale_rollback_handle));
    CuAssertTrue(tc, event_handle_is_live(active_rollback_handle));
    CuAssertTrue(tc, trigger->wait_rollback_handle == active_rollback_handle);
  }
  CuAssertPtrNotNull(tc, GET_TRIG_WAIT_DATA(trigger));

  dg_trigger_wait_cancel(trigger);
  CuAssertTrue(tc, event_runtime_handle_is_none(GET_TRIG_WAIT_HANDLE(trigger)));
  CuAssertTrue(tc, trigger->wait_rollback_handle == EVENT_HANDLE_NONE);
  CuAssertPtrEquals(tc, NULL, GET_TRIG_WAIT_DATA(trigger));
  free_trigger(trigger);
  event_free_all();
}

void Test_dg_stale_wait_replacement_cannot_resume_trigger(CuTest *tc)
{
  unsigned long saved_pulse = pulse;

  pulse = 4000U;
  verify_stale_wait_replacement(tc, EVENT_BACKEND_LEGACY_QUEUE);
  pulse = 5000U;
  verify_stale_wait_replacement(tc, EVENT_BACKEND_GAME_SCHEDULER);
  pulse = saved_pulse;
}

static void verify_replaced_wait_allows_trigger_destruction(CuTest *tc,
                                                            enum event_backend_kind backend)
{
  struct trig_data *trigger;
  struct event_runtime_handle stale_runtime_handle;
  event_handle_t stale_rollback_handle;

  event_free_all();
  CuAssertIntEquals(tc, 1, event_test_select_backend(backend));
  event_init();
  dg_wait_reset_telemetry_for_test();
  CREATE(trigger, struct trig_data, 1);
  trigger->name = strdup("destroyed stale wait replacement");
  CuAssertPtrNotNull(tc, trigger->name);

  CuAssertTrue(tc, dg_wait_schedule_for_test(trigger, 1L));
  stale_runtime_handle = GET_TRIG_WAIT_HANDLE(trigger);
  stale_rollback_handle = trigger->wait_rollback_handle;
  CuAssertTrue(tc, dg_wait_schedule_for_test(trigger, 10L));
  if (backend == EVENT_BACKEND_GAME_SCHEDULER)
    CuAssertTrue(tc, !event_runtime_handle_is_live(stale_runtime_handle));
  else
    CuAssertTrue(tc, !event_handle_is_live(stale_rollback_handle));

  free_trigger(trigger);
  CuAssertIntEquals(tc, 0, event_queue_depth());
  pulse += 10U;
  event_process();
  CuAssertIntEquals(tc, 0, (int)dg_wait_resume_count_for_test());
  event_free_all();
}

void Test_dg_replaced_wait_allows_trigger_destruction_before_dispatch(CuTest *tc)
{
  unsigned long saved_pulse = pulse;

  pulse = 5200U;
  verify_replaced_wait_allows_trigger_destruction(tc, EVENT_BACKEND_LEGACY_QUEUE);
  pulse = 5300U;
  verify_replaced_wait_allows_trigger_destruction(tc, EVENT_BACKEND_GAME_SCHEDULER);
  pulse = saved_pulse;
}

static void verify_failed_replacement_preserves_wait(CuTest *tc, enum event_backend_kind backend)
{
  struct trig_data *trigger;
  struct wait_event_data *wait_data;
  struct event_runtime_handle runtime_handle;
  event_handle_t rollback_handle;

  event_free_all();
  CuAssertIntEquals(tc, 1, event_test_select_backend(backend));
  event_init();
  dg_wait_reset_telemetry_for_test();
  CREATE(trigger, struct trig_data, 1);
  trigger->name = strdup("failed wait replacement");
  CuAssertPtrNotNull(tc, trigger->name);

  CuAssertTrue(tc, dg_wait_schedule_for_test(trigger, 10L));
  wait_data = GET_TRIG_WAIT_DATA(trigger);
  runtime_handle = GET_TRIG_WAIT_HANDLE(trigger);
  rollback_handle = trigger->wait_rollback_handle;
  dg_wait_fail_next_schedule_for_test();
  CuAssertTrue(tc, !dg_wait_schedule_for_test(trigger, 20L));

  CuAssertPtrEquals(tc, wait_data, GET_TRIG_WAIT_DATA(trigger));
  if (backend == EVENT_BACKEND_GAME_SCHEDULER)
  {
    CuAssertTrue(tc, event_runtime_handles_equal(runtime_handle, GET_TRIG_WAIT_HANDLE(trigger)));
    CuAssertTrue(tc, event_runtime_handle_is_live(runtime_handle));
  }
  else
  {
    CuAssertTrue(tc, rollback_handle == trigger->wait_rollback_handle);
    CuAssertTrue(tc, event_handle_is_live(rollback_handle));
  }

  dg_trigger_wait_cancel(trigger);
  free_trigger(trigger);
  event_free_all();
}

void Test_dg_failed_wait_replacement_preserves_existing_wait(CuTest *tc)
{
  verify_failed_replacement_preserves_wait(tc, EVENT_BACKEND_LEGACY_QUEUE);
  verify_failed_replacement_preserves_wait(tc, EVENT_BACKEND_GAME_SCHEDULER);
}

static void verify_inflight_trigger_free(CuTest *tc, enum event_backend_kind backend)
{
  struct trig_data *trigger;
  struct event_runtime_handle runtime_handle;
  event_handle_t rollback_handle;

  event_free_all();
  CuAssertIntEquals(tc, 1, event_test_select_backend(backend));
  event_init();
  dg_wait_reset_telemetry_for_test();
  CREATE(trigger, struct trig_data, 1);
  trigger->name = strdup("in-flight wait destruction");
  CuAssertPtrNotNull(tc, trigger->name);

  CuAssertTrue(tc, dg_wait_schedule_inflight_free_for_test(trigger, 1L));
  runtime_handle = GET_TRIG_WAIT_HANDLE(trigger);
  rollback_handle = trigger->wait_rollback_handle;
  pulse++;
  event_process();

  if (backend == EVENT_BACKEND_GAME_SCHEDULER)
    CuAssertTrue(tc, !event_runtime_handle_is_live(runtime_handle));
  else
    CuAssertTrue(tc, !event_handle_is_live(rollback_handle));
  CuAssertIntEquals(tc, 1, (int)dg_wait_deferred_free_count_for_test());
  CuAssertIntEquals(tc, 0, event_queue_depth());
  event_free_all();
}

void Test_dg_inflight_wait_defers_trigger_free_until_cleanup(CuTest *tc)
{
  unsigned long saved_pulse = pulse;

  pulse = 6000U;
  verify_inflight_trigger_free(tc, EVENT_BACKEND_LEGACY_QUEUE);
  pulse = 7000U;
  verify_inflight_trigger_free(tc, EVENT_BACKEND_GAME_SCHEDULER);
  pulse = saved_pulse;
}

void Test_dg_production_empty_expression_operands_are_safe(CuTest *tc)
{
  struct script_data script = {0};
  struct trig_data trigger = {0};
  struct trig_var_data *variable;
  char empty_right[] = "eval empty_right 1 ==";
  char empty_left[] = "eval empty_left == 1";
  bool found_empty_right = false;
  bool found_empty_left = false;

  trigger.name = "empty operand regression";

  process_eval(NULL, &script, &trigger, WLD_TRIGGER, empty_right);
  process_eval(NULL, &script, &trigger, WLD_TRIGGER, empty_left);

  for (variable = trigger.var_list; variable != NULL; variable = variable->next)
  {
    if (!strcmp(variable->name, "empty_right"))
    {
      found_empty_right = true;
      CuAssertStrEquals(tc, "0", variable->value);
    }
    else if (!strcmp(variable->name, "empty_left"))
    {
      found_empty_left = true;
      CuAssertStrEquals(tc, "0", variable->value);
    }
  }

  CuAssertTrue(tc, found_empty_right);
  CuAssertTrue(tc, found_empty_left);
  free_varlist(trigger.var_list);
}
