#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"

#include "../../src/comm.h"
#include "../../src/db.h"
#include "../../src/interpreter.h"
#include "../../src/mob/mob_act.h"
#include "../../src/olc/oasis.h"
#include "../../src/vessels/vessels_moving_rooms.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SPEC_PULSE_ROOM_COUNT 2
#define SPEC_PULSE_MOBILE_COUNT 2
#define SPEC_PULSE_OBJECT_COUNT 6
#define SPEC_PULSE_OWNER_COUNT 9
#define SPEC_PULSE_MAX_CALLS 16
#define SPEC_PULSE_SOURCE_LIMIT (1024L * 1024L)

void proc_update(void);

struct spec_pulse_call
{
  struct char_data *actor;
  void *owner;
  int command;
  bool argument_is_null;
  char argument[MAX_INPUT_LENGTH];
};

struct spec_pulse_recorder
{
  struct spec_pulse_call calls[SPEC_PULSE_MAX_CALLS];
  int returns[SPEC_PULSE_MAX_CALLS];
  int return_count;
  int call_count;
};

struct spec_pulse_fixture
{
  struct room_data *saved_world;
  room_rnum saved_top_of_world;
  struct index_data *saved_mob_index;
  mob_rnum saved_top_of_mobt;
  struct index_data *saved_obj_index;
  obj_rnum saved_top_of_objt;
  struct char_data *saved_character_list;
  struct obj_data *saved_object_list;
  struct moving_room_data *saved_moving_rooms;
  struct command_info *saved_complete_cmd_info;
  int saved_no_specials;

  struct room_data rooms[SPEC_PULSE_ROOM_COUNT];
  struct index_data mob_indexes[SPEC_PULSE_MOBILE_COUNT];
  struct index_data obj_indexes[SPEC_PULSE_OBJECT_COUNT];
  struct char_data actor;
  struct char_data mobiles[SPEC_PULSE_MOBILE_COUNT];
  struct obj_data objects[SPEC_PULSE_OBJECT_COUNT];
  struct moving_room_data moving_room;
  struct command_info commands[2];
  struct spec_pulse_recorder recorder;

  int command_handler_calls;
  struct char_data *command_handler_actor;
  int command_handler_command;
  int command_handler_subcommand;
  bool command_handler_argument_is_null;
  char command_handler_argument[MAX_INPUT_LENGTH];
};

static struct spec_pulse_fixture *active_spec_pulse_fixture;

static SPECIAL_DECL(spec_pulse_record_callback)
{
  struct spec_pulse_call *call;
  int call_index;

  if (active_spec_pulse_fixture == NULL)
    return 0;

  call_index = active_spec_pulse_fixture->recorder.call_count;
  if (call_index < SPEC_PULSE_MAX_CALLS)
  {
    call = &active_spec_pulse_fixture->recorder.calls[call_index];
    call->actor = ch;
    call->owner = me;
    call->command = cmd;
    call->argument_is_null = argument == NULL;
    if (argument != NULL)
      snprintf(call->argument, sizeof(call->argument), "%s", argument);
  }
  active_spec_pulse_fixture->recorder.call_count++;

  if (call_index >= 0 && call_index < active_spec_pulse_fixture->recorder.return_count &&
      call_index < SPEC_PULSE_MAX_CALLS)
    return active_spec_pulse_fixture->recorder.returns[call_index];

  return 0;
}

static void spec_pulse_command_handler(struct char_data *ch, const char *argument, int cmd,
                                       int subcmd)
{
  if (active_spec_pulse_fixture == NULL)
    return;

  active_spec_pulse_fixture->command_handler_calls++;
  active_spec_pulse_fixture->command_handler_actor = ch;
  active_spec_pulse_fixture->command_handler_command = cmd;
  active_spec_pulse_fixture->command_handler_subcommand = subcmd;
  active_spec_pulse_fixture->command_handler_argument_is_null = argument == NULL;
  if (argument != NULL)
    snprintf(active_spec_pulse_fixture->command_handler_argument,
             sizeof(active_spec_pulse_fixture->command_handler_argument), "%s", argument);
}

static bool spec_pulse_fixture_begin(struct spec_pulse_fixture *fixture)
{
  int index;

  if (fixture == NULL || active_spec_pulse_fixture != NULL)
    return false;

  memset(fixture, 0, sizeof(*fixture));
  fixture->saved_world = world;
  fixture->saved_top_of_world = top_of_world;
  fixture->saved_mob_index = mob_index;
  fixture->saved_top_of_mobt = top_of_mobt;
  fixture->saved_obj_index = obj_index;
  fixture->saved_top_of_objt = top_of_objt;
  fixture->saved_character_list = character_list;
  fixture->saved_object_list = object_list;
  fixture->saved_moving_rooms = movingRoomList;
  fixture->saved_complete_cmd_info = complete_cmd_info;
  fixture->saved_no_specials = no_specials;

  mobile_activity_reset();

  fixture->rooms[0].number = 100;
  fixture->rooms[1].number = 200;
  for (index = 0; index < SPEC_PULSE_MOBILE_COUNT; index++)
    fixture->mob_indexes[index].vnum = 1000 + index;
  for (index = 0; index < SPEC_PULSE_OBJECT_COUNT; index++)
    fixture->obj_indexes[index].vnum = 2000 + index;

  world = fixture->rooms;
  top_of_world = SPEC_PULSE_ROOM_COUNT - 1;
  mob_index = fixture->mob_indexes;
  top_of_mobt = SPEC_PULSE_MOBILE_COUNT - 1;
  obj_index = fixture->obj_indexes;
  top_of_objt = SPEC_PULSE_OBJECT_COUNT - 1;
  character_list = NULL;
  object_list = NULL;
  movingRoomList = NULL;
  no_specials = 0;
  active_spec_pulse_fixture = fixture;
  return true;
}

static void spec_pulse_fixture_end(struct spec_pulse_fixture *fixture)
{
  int index;

  if (fixture == NULL || active_spec_pulse_fixture != fixture)
    return;

  for (index = 0; index < SPEC_PULSE_OBJECT_COUNT; index++)
    autoproc_registry_remove(&fixture->objects[index]);
  mobile_activity_reset();
  active_spec_pulse_fixture = NULL;
  world = fixture->saved_world;
  top_of_world = fixture->saved_top_of_world;
  mob_index = fixture->saved_mob_index;
  top_of_mobt = fixture->saved_top_of_mobt;
  obj_index = fixture->saved_obj_index;
  top_of_objt = fixture->saved_top_of_objt;
  character_list = fixture->saved_character_list;
  object_list = fixture->saved_object_list;
  movingRoomList = fixture->saved_moving_rooms;
  complete_cmd_info = fixture->saved_complete_cmd_info;
  no_specials = fixture->saved_no_specials;
}

static void spec_pulse_recorder_reset(struct spec_pulse_fixture *fixture)
{
  memset(&fixture->recorder, 0, sizeof(fixture->recorder));
}

static void spec_pulse_set_mobile_flags(struct char_data *mobile, bool has_special)
{
  memset(MOB_FLAGS(mobile), 0, sizeof(mobile->char_specials.saved.act));
  SET_BIT_AR(MOB_FLAGS(mobile), MOB_ISNPC);
  SET_BIT_AR(MOB_FLAGS(mobile), MOB_SENTINEL);
  if (has_special)
    SET_BIT_AR(MOB_FLAGS(mobile), MOB_SPEC);
}

static void spec_pulse_prepare_command_graph(struct spec_pulse_fixture *fixture,
                                             void **expected_owners)
{
  int index;

  fixture->rooms[0].func = spec_pulse_record_callback;
  IN_ROOM(&fixture->actor) = 0;

  for (index = 0; index < SPEC_PULSE_OBJECT_COUNT; index++)
  {
    GET_OBJ_RNUM(&fixture->objects[index]) = index;
    fixture->obj_indexes[index].func = spec_pulse_record_callback;
  }
  GET_EQ(&fixture->actor, 0) = &fixture->objects[0];
  GET_EQ(&fixture->actor, NUM_WEARS - 1) = &fixture->objects[1];
  fixture->actor.carrying = &fixture->objects[2];
  fixture->objects[2].next_content = &fixture->objects[3];

  for (index = 0; index < SPEC_PULSE_MOBILE_COUNT; index++)
  {
    fixture->mobiles[index].nr = index;
    IN_ROOM(&fixture->mobiles[index]) = 0;
    SET_BIT_AR(MOB_FLAGS(&fixture->mobiles[index]), MOB_ISNPC);
    fixture->mob_indexes[index].func = spec_pulse_record_callback;
  }
  fixture->rooms[0].people = &fixture->mobiles[0];
  fixture->mobiles[0].next_in_room = &fixture->mobiles[1];

  fixture->rooms[0].contents = &fixture->objects[4];
  fixture->objects[4].next_content = &fixture->objects[5];

  expected_owners[0] = &fixture->rooms[0];
  expected_owners[1] = &fixture->objects[0];
  expected_owners[2] = &fixture->objects[1];
  expected_owners[3] = &fixture->objects[2];
  expected_owners[4] = &fixture->objects[3];
  expected_owners[5] = &fixture->mobiles[0];
  expected_owners[6] = &fixture->mobiles[1];
  expected_owners[7] = &fixture->objects[4];
  expected_owners[8] = &fixture->objects[5];
}

static void spec_pulse_prepare_mobile_activity(struct spec_pulse_fixture *fixture,
                                               SPECIAL_DECL(*callback))
{
  struct char_data *mobile;
  time_t now;

  mobile = &fixture->mobiles[0];
  now = time(NULL);
  mobile->nr = 0;
  IN_ROOM(mobile) = 0;
  mobile->player.short_descr = "pulse mobile";
  GET_LEVEL(mobile) = 0;
  GET_HIT(mobile) = 10;
  GET_POS(mobile) = POS_RESTING;
  GET_DEFAULT_POS(mobile) = POS_SITTING;
  GET_MOB_LOADROOM(mobile) = 0;
  mobile->mob_specials.last_slot_regen = now;
  mobile->mob_specials.last_known_slot_regen = now;
  spec_pulse_set_mobile_flags(mobile, true);

  fixture->mob_indexes[0].func = callback;
  fixture->rooms[0].people = mobile;
  character_list = mobile;
}

static void spec_pulse_add_second_mobile(struct spec_pulse_fixture *fixture,
                                         SPECIAL_DECL(*callback))
{
  struct char_data *mobile;
  time_t now;

  mobile = &fixture->mobiles[1];
  now = time(NULL);
  mobile->nr = 1;
  IN_ROOM(mobile) = 0;
  mobile->player.short_descr = "second pulse mobile";
  GET_LEVEL(mobile) = 0;
  GET_HIT(mobile) = 10;
  GET_POS(mobile) = POS_RESTING;
  GET_DEFAULT_POS(mobile) = POS_SITTING;
  GET_MOB_LOADROOM(mobile) = 0;
  mobile->mob_specials.last_slot_regen = now;
  mobile->mob_specials.last_known_slot_regen = now;
  spec_pulse_set_mobile_flags(mobile, true);

  fixture->mob_indexes[1].func = callback;
  fixture->mobiles[0].next = mobile;
  fixture->mobiles[0].next_in_room = mobile;
}

static bool spec_pulse_calls_match(struct spec_pulse_fixture *fixture, void **owners,
                                   int owner_count, struct char_data *actor, int command,
                                   const char *argument)
{
  struct spec_pulse_call *call;
  int index;

  if (fixture->recorder.call_count != owner_count || owner_count > SPEC_PULSE_MAX_CALLS)
    return false;

  for (index = 0; index < owner_count; index++)
  {
    call = &fixture->recorder.calls[index];
    if (call->owner != owners[index] || call->actor != actor || call->command != command ||
        call->argument_is_null || strcmp(call->argument, argument) != 0)
      return false;
  }
  return true;
}

static const char *spec_pulse_source_root(void)
{
  const char *root;

  root = getenv("LUMINARI_TEST_ROOT");
  return root != NULL && *root != '\0' ? root : ".";
}

static bool spec_pulse_read_source(const char *relative_path, char **text)
{
  FILE *file;
  char path[PATH_MAX];
  char *buffer;
  long source_length;
  size_t bytes_read;
  bool success;

  *text = NULL;
  if (snprintf(path, sizeof(path), "%s/%s", spec_pulse_source_root(), relative_path) >=
      (int)sizeof(path))
    return false;

  file = fopen(path, "rb");
  if (file == NULL)
    return false;

  success = fseek(file, 0, SEEK_END) == 0;
  source_length = success ? ftell(file) : -1;
  if (source_length < 0 || source_length > SPEC_PULSE_SOURCE_LIMIT || fseek(file, 0, SEEK_SET) != 0)
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

static char *spec_pulse_find_in_region(char *begin, char *end, const char *marker)
{
  char *match;

  if (begin == NULL || end == NULL || begin >= end)
    return NULL;
  match = strstr(begin, marker);
  return match != NULL && match < end ? match : NULL;
}

static char *spec_pulse_find_block_end(char *begin, char *limit)
{
  char *cursor;
  int depth = 0;

  cursor = spec_pulse_find_in_region(begin, limit, "{");
  if (cursor == NULL)
    return NULL;
  for (; cursor < limit; cursor++)
  {
    if (*cursor == '{')
      depth++;
    else if (*cursor == '}' && --depth == 0)
      return cursor + 1;
  }
  return NULL;
}

void Test_spec_command_traverses_all_owners_in_order(CuTest *tc)
{
  struct spec_pulse_fixture fixture;
  void *owners[SPEC_PULSE_OWNER_COUNT];
  char argument[] = "  exact payload";
  bool setup_ok;
  bool calls_match;
  bool mobile_flags_absent;
  int handled;

  setup_ok = spec_pulse_fixture_begin(&fixture);
  if (!setup_ok)
  {
    CuFail(tc, "unable to initialize command fixture");
    return;
  }

  spec_pulse_prepare_command_graph(&fixture, owners);
  mobile_flags_absent =
      !MOB_FLAGGED(&fixture.mobiles[0], MOB_SPEC) && !MOB_FLAGGED(&fixture.mobiles[1], MOB_SPEC);
  handled = special(&fixture.actor, 37, argument);
  calls_match = spec_pulse_calls_match(&fixture, owners, SPEC_PULSE_OWNER_COUNT, &fixture.actor, 37,
                                       argument);
  spec_pulse_fixture_end(&fixture);

  CuAssertIntEquals(tc, 0, handled);
  CuAssertTrue(tc, calls_match);
  CuAssertTrue(tc, mobile_flags_absent);
}

void Test_spec_command_stops_at_each_first_nonzero_owner(CuTest *tc)
{
  struct spec_pulse_fixture fixture;
  void *owners[SPEC_PULSE_OWNER_COUNT];
  char argument[] = " stop payload";
  bool setup_ok;
  bool all_prefixes_match;
  int stop_index;
  int handled;

  setup_ok = spec_pulse_fixture_begin(&fixture);
  if (!setup_ok)
  {
    CuFail(tc, "unable to initialize command fixture");
    return;
  }

  spec_pulse_prepare_command_graph(&fixture, owners);
  all_prefixes_match = true;
  for (stop_index = 0; stop_index < SPEC_PULSE_OWNER_COUNT; stop_index++)
  {
    spec_pulse_recorder_reset(&fixture);
    fixture.recorder.return_count = stop_index + 1;
    fixture.recorder.returns[stop_index] = 1;
    handled = special(&fixture.actor, 19, argument);
    if (handled != 1 ||
        !spec_pulse_calls_match(&fixture, owners, stop_index + 1, &fixture.actor, 19, argument))
      all_prefixes_match = false;
  }
  spec_pulse_fixture_end(&fixture);

  CuAssertTrue(tc, all_prefixes_match);
}

void Test_spec_command_rejects_nowhere_and_skips_pending_mobile(CuTest *tc)
{
  struct spec_pulse_fixture fixture;
  void *owners[SPEC_PULSE_OWNER_COUNT];
  void *without_pending[SPEC_PULSE_OWNER_COUNT - 1];
  char argument[] = " pending payload";
  bool setup_ok;
  bool pending_skipped;
  bool nowhere_skipped;
  int index;
  int handled;

  setup_ok = spec_pulse_fixture_begin(&fixture);
  if (!setup_ok)
  {
    CuFail(tc, "unable to initialize command fixture");
    return;
  }

  spec_pulse_prepare_command_graph(&fixture, owners);
  SET_BIT_AR(MOB_FLAGS(&fixture.mobiles[0]), MOB_NOTDEADYET);
  for (index = 0; index < 5; index++)
    without_pending[index] = owners[index];
  without_pending[5] = owners[6];
  without_pending[6] = owners[7];
  without_pending[7] = owners[8];

  handled = special(&fixture.actor, 23, argument);
  pending_skipped =
      handled == 0 && spec_pulse_calls_match(&fixture, without_pending, SPEC_PULSE_OWNER_COUNT - 1,
                                             &fixture.actor, 23, argument);

  spec_pulse_recorder_reset(&fixture);
  IN_ROOM(&fixture.actor) = NOWHERE;
  handled = special(&fixture.actor, 23, argument);
  nowhere_skipped = handled == 0 && fixture.recorder.call_count == 0;
  spec_pulse_fixture_end(&fixture);

  CuAssertTrue(tc, pending_skipped);
  CuAssertTrue(tc, nowhere_skipped);
}

void Test_spec_command_no_specials_bypasses_special_dispatch(CuTest *tc)
{
  struct spec_pulse_fixture fixture;
  char normal_command[] = "specprobe payload";
  char suppressed_command[] = "specprobe payload";
  bool setup_ok;
  bool normal_handled;
  bool suppressed_bypassed;

  setup_ok = spec_pulse_fixture_begin(&fixture);
  if (!setup_ok)
  {
    CuFail(tc, "unable to initialize command fixture");
    return;
  }

  SET_BIT_AR(MOB_FLAGS(&fixture.actor), MOB_ISNPC);
  fixture.actor.nr = NOBODY;
  IN_ROOM(&fixture.actor) = 0;
  GET_POS(&fixture.actor) = POS_STANDING;
  GET_LEVEL(&fixture.actor) = 0;
  fixture.actor.player.short_descr = "command actor";
  fixture.rooms[0].func = spec_pulse_record_callback;
  fixture.recorder.return_count = 1;
  fixture.recorder.returns[0] = 1;

  fixture.commands[0].command = "specprobe";
  fixture.commands[0].sort_as = "specprobe";
  fixture.commands[0].minimum_position = POS_DEAD;
  fixture.commands[0].command_pointer = spec_pulse_command_handler;
  fixture.commands[0].minimum_level = 0;
  fixture.commands[0].subcmd = 71;
  fixture.commands[0].ignore_wait = TRUE;
  fixture.commands[0].actions_required = ACTION_NONE;
  fixture.commands[1].command = "\n";
  fixture.commands[1].sort_as = "\n";
  complete_cmd_info = fixture.commands;

  command_interpreter(&fixture.actor, normal_command);
  normal_handled = fixture.recorder.call_count == 1 && fixture.command_handler_calls == 0 &&
                   fixture.recorder.calls[0].actor == &fixture.actor &&
                   fixture.recorder.calls[0].owner == &fixture.rooms[0] &&
                   fixture.recorder.calls[0].command == 0 &&
                   strcmp(fixture.recorder.calls[0].argument, " payload") == 0;

  spec_pulse_recorder_reset(&fixture);
  no_specials = 1;
  command_interpreter(&fixture.actor, suppressed_command);
  suppressed_bypassed =
      fixture.recorder.call_count == 0 && fixture.command_handler_calls == 1 &&
      fixture.command_handler_actor == &fixture.actor && fixture.command_handler_command == 0 &&
      fixture.command_handler_subcommand == 71 && !fixture.command_handler_argument_is_null &&
      strcmp(fixture.command_handler_argument, " payload") == 0;
  spec_pulse_fixture_end(&fixture);

  CuAssertTrue(tc, normal_handled);
  CuAssertTrue(tc, suppressed_bypassed);
}

void Test_spec_mobile_activity_handled_callback_skips_default_ai(CuTest *tc)
{
  struct spec_pulse_fixture fixture;
  struct spec_pulse_call call;
  bool setup_ok;
  bool payload_matches;
  int position;

  setup_ok = spec_pulse_fixture_begin(&fixture);
  if (!setup_ok)
  {
    CuFail(tc, "unable to initialize mobile fixture");
    return;
  }

  spec_pulse_prepare_mobile_activity(&fixture, spec_pulse_record_callback);
  fixture.recorder.return_count = 1;
  fixture.recorder.returns[0] = 1;
  mobile_activity();
  call = fixture.recorder.calls[0];
  position = (unsigned char)GET_POS(&fixture.mobiles[0]);
  payload_matches = fixture.recorder.call_count == 1 && call.actor == &fixture.mobiles[0] &&
                    call.owner == &fixture.mobiles[0] && call.command == 0 &&
                    !call.argument_is_null && call.argument[0] == '\0';
  spec_pulse_fixture_end(&fixture);

  CuAssertTrue(tc, payload_matches);
  CuAssertIntEquals(tc, POS_RESTING, position);
}

void Test_spec_mobile_activity_zero_callback_runs_default_ai(CuTest *tc)
{
  struct spec_pulse_fixture fixture;
  bool setup_ok;
  bool callback_called;
  int position;

  setup_ok = spec_pulse_fixture_begin(&fixture);
  if (!setup_ok)
  {
    CuFail(tc, "unable to initialize mobile fixture");
    return;
  }

  spec_pulse_prepare_mobile_activity(&fixture, spec_pulse_record_callback);
  mobile_activity();
  callback_called = fixture.recorder.call_count == 1;
  position = (unsigned char)GET_POS(&fixture.mobiles[0]);
  spec_pulse_fixture_end(&fixture);

  CuAssertTrue(tc, callback_called);
  CuAssertIntEquals(tc, POS_SITTING, position);
}

void Test_spec_mobile_activity_activation_gates(CuTest *tc)
{
  struct spec_pulse_fixture fixture;
  struct char_data *mobile;
  bool setup_ok;
  bool flag_gate;
  bool suppression_gate;
  bool missing_callback_gate;

  setup_ok = spec_pulse_fixture_begin(&fixture);
  if (!setup_ok)
  {
    CuFail(tc, "unable to initialize mobile fixture");
    return;
  }

  spec_pulse_prepare_mobile_activity(&fixture, spec_pulse_record_callback);
  mobile = &fixture.mobiles[0];

  spec_pulse_set_mobile_flags(mobile, false);
  mobile_activity();
  flag_gate = fixture.recorder.call_count == 0 && GET_POS(mobile) == POS_SITTING;

  spec_pulse_recorder_reset(&fixture);
  spec_pulse_set_mobile_flags(mobile, true);
  GET_POS(mobile) = POS_RESTING;
  no_specials = 1;
  mobile_activity();
  suppression_gate = fixture.recorder.call_count == 0 && MOB_FLAGGED(mobile, MOB_SPEC) &&
                     GET_POS(mobile) == POS_SITTING;

  spec_pulse_recorder_reset(&fixture);
  spec_pulse_set_mobile_flags(mobile, true);
  GET_POS(mobile) = POS_RESTING;
  fixture.mob_indexes[0].func = NULL;
  no_specials = 0;
  mobile_activity();
  missing_callback_gate = fixture.recorder.call_count == 0 && !MOB_FLAGGED(mobile, MOB_SPEC) &&
                          GET_POS(mobile) == POS_SITTING;
  spec_pulse_fixture_end(&fixture);

  CuAssertTrue(tc, flag_gate);
  CuAssertTrue(tc, suppression_gate);
  CuAssertTrue(tc, missing_callback_gate);
}

void Test_spec_mobile_activity_scheduler_visits_each_mobile_once_per_cycle(CuTest *tc)
{
  struct spec_pulse_fixture fixture;
  bool setup_ok;
  bool distributed;
  int activity_pulse;

  setup_ok = spec_pulse_fixture_begin(&fixture);
  if (!setup_ok)
  {
    CuFail(tc, "unable to initialize mobile fixture");
    return;
  }

  spec_pulse_prepare_mobile_activity(&fixture, spec_pulse_record_callback);
  spec_pulse_add_second_mobile(&fixture, spec_pulse_record_callback);
  fixture.recorder.return_count = 2;
  fixture.recorder.returns[0] = 1;
  fixture.recorder.returns[1] = 1;

  mobile_activity_pulse(0);
  distributed = fixture.recorder.call_count == 1;
  for (activity_pulse = 1; activity_pulse < PULSE_MOBILE; activity_pulse++)
    mobile_activity_pulse(activity_pulse);

  CuAssertTrue(tc, distributed);
  CuAssertIntEquals(tc, 2, fixture.recorder.call_count);
  CuAssertPtrEquals(tc, &fixture.mobiles[0], fixture.recorder.calls[0].actor);
  CuAssertPtrEquals(tc, &fixture.mobiles[1], fixture.recorder.calls[1].actor);
  spec_pulse_fixture_end(&fixture);
}

void Test_spec_mobile_activity_scheduler_forgets_removed_cursor(CuTest *tc)
{
  struct spec_pulse_fixture fixture;
  bool setup_ok;
  int activity_pulse;

  setup_ok = spec_pulse_fixture_begin(&fixture);
  if (!setup_ok)
  {
    CuFail(tc, "unable to initialize mobile fixture");
    return;
  }

  spec_pulse_prepare_mobile_activity(&fixture, spec_pulse_record_callback);
  spec_pulse_add_second_mobile(&fixture, spec_pulse_record_callback);
  fixture.recorder.return_count = 2;
  fixture.recorder.returns[0] = 1;
  fixture.recorder.returns[1] = 1;

  mobile_activity_pulse(0);
  mobile_activity_forget_character(&fixture.mobiles[1]);
  fixture.mobiles[0].next = NULL;
  fixture.mobiles[0].next_in_room = NULL;
  for (activity_pulse = 1; activity_pulse < PULSE_MOBILE; activity_pulse++)
    mobile_activity_pulse(activity_pulse);

  CuAssertIntEquals(tc, 1, fixture.recorder.call_count);
  spec_pulse_fixture_end(&fixture);
}

void Test_spec_proc_update_worn_object_uses_wearer_once(CuTest *tc)
{
  struct spec_pulse_fixture fixture;
  struct spec_pulse_call call;
  struct obj_data *object;
  bool setup_ok;
  bool call_matches;

  setup_ok = spec_pulse_fixture_begin(&fixture);
  if (!setup_ok)
  {
    CuFail(tc, "unable to initialize object fixture");
    return;
  }

  object = &fixture.objects[0];
  GET_OBJ_RNUM(object) = 0;
  SET_BIT_AR(GET_OBJ_EXTRA(object), ITEM_AUTOPROC);
  autoproc_registry_sync(object);
  object->worn_by = &fixture.actor;
  fixture.obj_indexes[0].func = spec_pulse_record_callback;
  fixture.recorder.return_count = 1;
  fixture.recorder.returns[0] = 1;
  object_list = object;

  proc_update();
  call = fixture.recorder.calls[0];
  call_matches = fixture.recorder.call_count == 1 && call.actor == &fixture.actor &&
                 call.owner == object && call.command == 0 && !call.argument_is_null &&
                 call.argument[0] == '\0';
  spec_pulse_fixture_end(&fixture);

  CuAssertTrue(tc, call_matches);
}

void Test_spec_proc_update_carried_object_uses_null_then_carrier(CuTest *tc)
{
  struct spec_pulse_fixture fixture;
  struct obj_data *object;
  bool setup_ok;
  bool fallback_matches;
  bool handled_null_stops;
  bool unowned_runs_once;

  setup_ok = spec_pulse_fixture_begin(&fixture);
  if (!setup_ok)
  {
    CuFail(tc, "unable to initialize object fixture");
    return;
  }

  object = &fixture.objects[0];
  GET_OBJ_RNUM(object) = 0;
  SET_BIT_AR(GET_OBJ_EXTRA(object), ITEM_AUTOPROC);
  autoproc_registry_sync(object);
  object->carried_by = &fixture.actor;
  fixture.obj_indexes[0].func = spec_pulse_record_callback;
  object_list = object;

  fixture.recorder.return_count = 2;
  fixture.recorder.returns[0] = 0;
  fixture.recorder.returns[1] = 1;
  proc_update();
  fallback_matches = fixture.recorder.call_count == 2 && fixture.recorder.calls[0].actor == NULL &&
                     fixture.recorder.calls[1].actor == &fixture.actor &&
                     fixture.recorder.calls[0].owner == object &&
                     fixture.recorder.calls[1].owner == object;

  spec_pulse_recorder_reset(&fixture);
  fixture.recorder.return_count = 1;
  fixture.recorder.returns[0] = 1;
  proc_update();
  handled_null_stops = fixture.recorder.call_count == 1 && fixture.recorder.calls[0].actor == NULL;

  spec_pulse_recorder_reset(&fixture);
  object->carried_by = NULL;
  proc_update();
  unowned_runs_once = fixture.recorder.call_count == 1 && fixture.recorder.calls[0].actor == NULL;
  spec_pulse_fixture_end(&fixture);

  CuAssertTrue(tc, fallback_matches);
  CuAssertTrue(tc, handled_null_stops);
  CuAssertTrue(tc, unowned_runs_once);
}

void Test_spec_proc_update_gates_and_ignores_no_specials(CuTest *tc)
{
  struct spec_pulse_fixture fixture;
  struct obj_data *unflagged;
  struct obj_data *inert_weapon;
  struct obj_data *missing_callback;
  bool setup_ok;
  bool gates_match;
  bool suppression_ignored;

  setup_ok = spec_pulse_fixture_begin(&fixture);
  if (!setup_ok)
  {
    CuFail(tc, "unable to initialize object fixture");
    return;
  }

  unflagged = &fixture.objects[0];
  inert_weapon = &fixture.objects[1];
  missing_callback = &fixture.objects[2];
  GET_OBJ_RNUM(unflagged) = 0;
  GET_OBJ_RNUM(inert_weapon) = 1;
  GET_OBJ_RNUM(missing_callback) = 2;
  fixture.obj_indexes[0].func = spec_pulse_record_callback;
  fixture.obj_indexes[1].func = spec_pulse_record_callback;
  fixture.obj_indexes[2].func = NULL;
  SET_BIT_AR(GET_OBJ_EXTRA(inert_weapon), ITEM_AUTOPROC);
  autoproc_registry_sync(inert_weapon);
  GET_OBJ_TYPE(inert_weapon) = ITEM_WEAPON;
  GET_OBJ_VAL(inert_weapon, 0) = 0;
  SET_BIT_AR(GET_OBJ_EXTRA(missing_callback), ITEM_AUTOPROC);
  autoproc_registry_sync(missing_callback);
  unflagged->next = inert_weapon;
  inert_weapon->next = missing_callback;
  object_list = unflagged;

  proc_update();
  gates_match = fixture.recorder.call_count == 0;

  spec_pulse_recorder_reset(&fixture);
  SET_BIT_AR(GET_OBJ_EXTRA(unflagged), ITEM_AUTOPROC);
  autoproc_registry_sync(unflagged);
  unflagged->worn_by = &fixture.actor;
  unflagged->next = NULL;
  no_specials = 1;
  fixture.recorder.return_count = 1;
  fixture.recorder.returns[0] = 1;
  proc_update();
  suppression_ignored = fixture.recorder.call_count == 1 &&
                        fixture.recorder.calls[0].actor == &fixture.actor &&
                        fixture.recorder.calls[0].owner == unflagged;
  spec_pulse_fixture_end(&fixture);

  CuAssertTrue(tc, gates_match);
  CuAssertTrue(tc, suppression_ignored);
}

void Test_spec_moving_rooms_timer_payload_and_return(CuTest *tc)
{
  struct spec_pulse_fixture fixture;
  struct spec_pulse_call first_call;
  struct spec_pulse_call second_call;
  bool setup_ok;
  bool first_tick_waited;
  bool first_expiry_matches;
  bool second_expiry_matches;

  setup_ok = spec_pulse_fixture_begin(&fixture);
  if (!setup_ok)
  {
    CuFail(tc, "unable to initialize moving-room fixture");
    return;
  }

  fixture.rooms[1].func = spec_pulse_record_callback;
  fixture.moving_room.destination = 200;
  fixture.moving_room.remainingZonePulses = 2;
  fixture.moving_room.resetZonePulse = 5;
  movingRoomList = &fixture.moving_room;

  moving_rooms_update();
  first_tick_waited =
      fixture.recorder.call_count == 0 && fixture.moving_room.remainingZonePulses == 1;

  fixture.recorder.return_count = 1;
  fixture.recorder.returns[0] = 0;
  moving_rooms_update();
  first_call = fixture.recorder.calls[0];
  first_expiry_matches = fixture.recorder.call_count == 1 && first_call.actor == NULL &&
                         first_call.owner == &fixture.moving_room && first_call.command == 0 &&
                         first_call.argument_is_null &&
                         fixture.moving_room.remainingZonePulses == 5;

  fixture.moving_room.remainingZonePulses = 1;
  fixture.recorder.return_count = 2;
  fixture.recorder.returns[1] = 1;
  moving_rooms_update();
  second_call = fixture.recorder.calls[1];
  second_expiry_matches = fixture.recorder.call_count == 2 && second_call.actor == NULL &&
                          second_call.owner == &fixture.moving_room && second_call.command == 0 &&
                          second_call.argument_is_null &&
                          fixture.moving_room.remainingZonePulses == 5;
  spec_pulse_fixture_end(&fixture);

  CuAssertTrue(tc, first_tick_waited);
  CuAssertTrue(tc, first_expiry_matches);
  CuAssertTrue(tc, second_expiry_matches);
}

void Test_spec_moving_rooms_ignores_no_specials(CuTest *tc)
{
  struct spec_pulse_fixture fixture;
  bool setup_ok;
  bool suppression_ignored;

  setup_ok = spec_pulse_fixture_begin(&fixture);
  if (!setup_ok)
  {
    CuFail(tc, "unable to initialize moving-room fixture");
    return;
  }

  fixture.rooms[1].func = spec_pulse_record_callback;
  fixture.moving_room.destination = 200;
  fixture.moving_room.remainingZonePulses = 1;
  fixture.moving_room.resetZonePulse = 7;
  movingRoomList = &fixture.moving_room;
  no_specials = 1;

  moving_rooms_update();
  suppression_ignored =
      fixture.recorder.call_count == 1 && fixture.recorder.calls[0].actor == NULL &&
      fixture.recorder.calls[0].owner == &fixture.moving_room &&
      fixture.recorder.calls[0].argument_is_null && fixture.moving_room.remainingZonePulses == 7;
  spec_pulse_fixture_end(&fixture);

  CuAssertTrue(tc, suppression_ignored);
}

void Test_spec_heartbeat_preserves_noncombat_proc_schedule(CuTest *tc)
{
  char *source;
  char *heartbeat;
  char *heartbeat_end;
  char *moving_gate;
  char *moving_end;
  char *moving_call;
  char *one_second_gate;
  char *active_gate;
  char *active_end;
  char *mobile_gate;
  char *mobile_end;
  char *mobile_call;
  char *proc_call;
  char *autoproc_gate;
  char *avernus_call;
  char *dg_gate;
  char *dg_end;
  char *dg_rollback;
  char *event_process_call;
  char *violence_gate;
  char *violence_end;
  char *affected_gate;
  char *affected_end;
  char *affect_call;
  char *d20_call;
  char *character_gate;
  char *character_end;
  char *psp_call;
  char *walk_gate;
  char *walk_end;
  char *walk_call;
  char *bard_gate;
  char *bard_end;
  char *bard_call;
  char *hint_gate;
  char *hint_end;
  char *hint_call;
  bool source_loaded;
  bool moving_schedule_matches;
  bool pulse_order_matches;
  bool character_rollback_matches;

  source = NULL;
  source_loaded = spec_pulse_read_source("src/comm.c", &source);
  moving_schedule_matches = false;
  pulse_order_matches = false;
  character_rollback_matches = false;
  if (source_loaded)
  {
    heartbeat = strstr(source, "void heartbeat(int heart_pulse)");
    heartbeat_end = heartbeat != NULL ? strstr(heartbeat, "static void timediff(") : NULL;

    moving_gate = spec_pulse_find_in_region(
        heartbeat, heartbeat_end, "if (!(heart_pulse % (PASSES_PER_SEC * 10)))");
    moving_end = spec_pulse_find_block_end(moving_gate, heartbeat_end);
    moving_call = spec_pulse_find_in_region(moving_gate, moving_end, "moving_rooms_update();");
    one_second_gate = spec_pulse_find_in_region(
        moving_end, heartbeat_end, "if (!(heart_pulse % PASSES_PER_SEC))");
    moving_schedule_matches = moving_gate != NULL && moving_end != NULL && moving_call != NULL &&
                              one_second_gate != NULL && moving_end < one_second_gate;

    event_process_call = spec_pulse_find_in_region(
        heartbeat, heartbeat_end, "event_process_compatibility_pulse();");
    dg_gate = spec_pulse_find_in_region(event_process_call, heartbeat_end,
                                        "if (!(heart_pulse % PULSE_DG_SCRIPT)");
    dg_end = spec_pulse_find_block_end(dg_gate, heartbeat_end);
    dg_rollback =
        spec_pulse_find_in_region(dg_gate, dg_end, "!periodic_dg_random_enabled()");

    active_gate =
        spec_pulse_find_in_region(heartbeat, heartbeat_end, "if (!active_world_enabled())");
    active_end = spec_pulse_find_block_end(active_gate, heartbeat_end);
    mobile_call = spec_pulse_find_in_region(active_gate, active_end,
                                            "mobile_activity_pulse(heart_pulse);");
    mobile_gate = spec_pulse_find_in_region(active_end, heartbeat_end,
                                            "if (!(heart_pulse % PULSE_MOBILE))");
    mobile_end = spec_pulse_find_block_end(mobile_gate, heartbeat_end);
    autoproc_gate = spec_pulse_find_in_region(mobile_gate, mobile_end,
                                              "if (!periodic_autoproc_enabled())");
    proc_call = spec_pulse_find_in_region(mobile_gate, mobile_end, "proc_update();");
    avernus_call =
        spec_pulse_find_in_region(mobile_gate, mobile_end, "rol_avernus_room_pulse();");
    violence_gate = spec_pulse_find_in_region(
        mobile_end, heartbeat_end, "if (!(heart_pulse % PULSE_VIOLENCE))");
    violence_end = spec_pulse_find_block_end(violence_gate, heartbeat_end);
    affected_gate = spec_pulse_find_in_region(
        violence_gate, violence_end, "if (!affected_owner_events_enabled())");
    affected_end = spec_pulse_find_block_end(affected_gate, violence_end);
    affect_call = spec_pulse_find_in_region(affected_gate, affected_end, "affect_update();");
    d20_call = spec_pulse_find_in_region(affected_end, violence_end, "proc_d20_round();");

    character_gate = spec_pulse_find_in_region(
        heartbeat, heartbeat_end,
        "if (!(heart_pulse % (PASSES_PER_SEC * 5)) && !character_periodic_events_enabled())");
    character_end = spec_pulse_find_block_end(character_gate, heartbeat_end);
    psp_call = spec_pulse_find_in_region(character_gate, character_end, "regen_psp();");
    walk_gate = spec_pulse_find_in_region(
        character_end, heartbeat_end,
        "if (!(heart_pulse % (int)(PASSES_PER_SEC * 0.75)) && "
        "!character_periodic_events_enabled())");
    walk_end = spec_pulse_find_in_region(
        walk_gate, heartbeat_end, "if (!(heart_pulse % (PASSES_PER_SEC * 60)))");
    walk_call =
        spec_pulse_find_in_region(walk_gate, walk_end, "process_walkto_actions();");
    bard_gate = spec_pulse_find_in_region(
        walk_end, heartbeat_end,
        "if (!(pulse % PULSE_VERSE_INTERVAL) && !character_periodic_events_enabled())");
    bard_end = spec_pulse_find_block_end(bard_gate, heartbeat_end);
    bard_call =
        spec_pulse_find_in_region(bard_gate, bard_end, "pulse_bardic_performance();");
    hint_gate = spec_pulse_find_in_region(
        bard_end, heartbeat_end,
        "if (!(pulse % PULSE_HINTS) && !character_periodic_events_enabled())");
    hint_end = spec_pulse_find_block_end(hint_gate, heartbeat_end);
    hint_call = spec_pulse_find_in_region(hint_gate, hint_end, "show_hints();");
    character_rollback_matches =
        character_gate != NULL && character_end != NULL && psp_call != NULL &&
        walk_gate != NULL && walk_end != NULL && walk_call != NULL && bard_gate != NULL &&
        bard_end != NULL && bard_call != NULL && hint_gate != NULL && hint_end != NULL &&
        hint_call != NULL && psp_call < walk_call && walk_call < bard_call &&
        bard_call < hint_call;

    pulse_order_matches = heartbeat != NULL && heartbeat_end != NULL && mobile_call != NULL &&
                          mobile_gate != NULL && mobile_end != NULL && autoproc_gate != NULL &&
                          proc_call != NULL && avernus_call != NULL && violence_gate != NULL &&
                          violence_end != NULL && affected_gate != NULL && affected_end != NULL &&
                          affect_call != NULL && d20_call != NULL &&
                          dg_gate != NULL && dg_end != NULL && dg_rollback != NULL &&
                          mobile_call < proc_call && autoproc_gate < proc_call &&
                          proc_call < avernus_call && avernus_call < violence_gate &&
                          affected_gate < affect_call && affect_call < d20_call;
  }
  free(source);

  CuAssertTrue(tc, source_loaded);
  CuAssertTrue(tc, moving_schedule_matches);
  CuAssertTrue(tc, pulse_order_matches);
  CuAssertTrue(tc, character_rollback_matches);
}

void Test_spec_mixed_owner_pulses_have_independent_rollback_gates(CuTest *tc)
{
  char *comm_source = NULL;
  char *limits_source = NULL;
  char *runtime_source = NULL;
  char *periodic_source = NULL;
  char *handler_source = NULL;
  char *luminari_gate;
  char *luminari_call;
  char *affected_term;
  char *character_term;
  char *round_gate;
  char *round_character_term;
  char *damage_call;
  char *misc_call;
  char *room_legacy_gate;
  char *room_legacy_loop;
  char *character_legacy_gate;
  char *character_legacy_loop;
  bool sources_loaded;
  bool heartbeat_contract;
  bool wrapper_contract;
  bool movement_contract;

  sources_loaded = spec_pulse_read_source("src/comm.c", &comm_source) &&
                   spec_pulse_read_source("src/limits.c", &limits_source) &&
                   spec_pulse_read_source("src/domain_event_runtime.c", &runtime_source) &&
                   spec_pulse_read_source("src/character_periodic.c", &periodic_source) &&
                   spec_pulse_read_source("src/handler.c", &handler_source);
  heartbeat_contract = false;
  wrapper_contract = false;
  movement_contract = false;
  if (sources_loaded)
  {
    luminari_gate = strstr(comm_source, "if (!(pulse % PULSE_LUMINARI)");
    luminari_call = luminari_gate != NULL ? strstr(luminari_gate, "pulse_luminari();") : NULL;
    affected_term =
        luminari_gate != NULL ? strstr(luminari_gate, "!affected_owner_events_enabled()") : NULL;
    character_term =
        luminari_gate != NULL ? strstr(luminari_gate, "!character_periodic_events_enabled()") : NULL;
    round_gate = strstr(comm_source, "if (!(heart_pulse % (6 * PASSES_PER_SEC)) &&");
    round_character_term =
        round_gate != NULL ? strstr(round_gate, "!character_periodic_events_enabled()") : NULL;
    damage_call =
        round_gate != NULL ? strstr(round_gate, "update_damage_and_effects_over_time();") : NULL;
    misc_call = round_gate != NULL ? strstr(round_gate, "update_player_misc();") : NULL;
    heartbeat_contract = luminari_gate != NULL && luminari_call != NULL && affected_term != NULL &&
                         character_term != NULL && affected_term < luminari_call &&
                         character_term < luminari_call && round_gate != NULL &&
                         round_character_term != NULL && damage_call != NULL && misc_call != NULL &&
                         round_character_term < damage_call && damage_call < misc_call;

    room_legacy_gate = strstr(limits_source, "if (!affected_owner_events_enabled())");
    room_legacy_loop =
        room_legacy_gate != NULL ? strstr(room_legacy_gate, "for (raff = raff_list;") : NULL;
    character_legacy_gate = strstr(limits_source, "if (!character_periodic_events_enabled())");
    character_legacy_loop = character_legacy_gate != NULL
                                ? strstr(character_legacy_gate, "for (ch = character_list;")
                                : NULL;
    wrapper_contract = room_legacy_gate != NULL && room_legacy_loop != NULL &&
                       character_legacy_gate != NULL && character_legacy_loop != NULL;

    movement_contract = strstr(runtime_source, "character_periodic_register_handlers(runtime_bus)") !=
                            NULL &&
                        strstr(periodic_source, "DOMAIN_EVENT_CHARACTER_MOVED") != NULL &&
                        strstr(periodic_source, "character_periodic_sync(ch);") != NULL &&
                        strstr(handler_source, "domain_event_runtime_character_moved(") != NULL;
  }

  free(comm_source);
  free(limits_source);
  free(runtime_source);
  free(periodic_source);
  free(handler_source);
  CuAssertTrue(tc, sources_loaded);
  CuAssertTrue(tc, heartbeat_contract);
  CuAssertTrue(tc, wrapper_contract);
  CuAssertTrue(tc, movement_contract);
}

void Test_spec_vessel_owner_pulses_have_lifecycle_and_rollback_gates(CuTest *tc)
{
  char *comm_source = NULL;
  char *periodic_source = NULL;
  char *rol_source = NULL;
  char *handler_source = NULL;
  char *edit_source = NULL;
  char *combat_source = NULL;
  bool sources_loaded;

  sources_loaded = spec_pulse_read_source("src/comm.c", &comm_source) &&
                   spec_pulse_read_source("src/vessels/vessel_periodic.c", &periodic_source) &&
                   spec_pulse_read_source("src/vessels/vessels_rol.c", &rol_source) &&
                   spec_pulse_read_source("src/handler.c", &handler_source) &&
                   spec_pulse_read_source("src/vessels/vessels_edit.c", &edit_source) &&
                   spec_pulse_read_source("src/vessels/vessels_combat.c", &combat_source);
  if (sources_loaded)
  {
    CuAssertPtrNotNull(tc, strstr(comm_source, "!vessel_periodic_events_enabled()"));
    CuAssertPtrNotNull(tc, strstr(periodic_source, "LUMINARI_VESSEL_EVENTS"));
    CuAssertPtrNotNull(tc, strstr(periodic_source, "GAME_EVENT_OWNER_VESSEL"));
    CuAssertPtrNotNull(tc, strstr(periodic_source, "autopilot_tick_one(ship);"));
    CuAssertPtrNotNull(tc, strstr(periodic_source, "schedule_tick_one(ship);"));
    CuAssertPtrNotNull(tc, strstr(rol_source, "rol_ship_owner_event"));
    CuAssertPtrEquals(tc, NULL, strstr(rol_source, "object_list"));
    CuAssertPtrNotNull(tc, strstr(handler_source, "rol_ship_note_object_placed(object);"));
    CuAssertPtrNotNull(tc, strstr(handler_source, "rol_ship_note_object_extracted(obj);"));
    CuAssertPtrNotNull(tc, strstr(edit_source, "vessel_periodic_sync(ship);"));
    CuAssertPtrNotNull(tc, strstr(combat_source, "vessel_periodic_forget(ship);"));
  }

  free(comm_source);
  free(periodic_source);
  free(rol_source);
  free(handler_source);
  free(edit_source);
  free(combat_source);
  CuAssertTrue(tc, sources_loaded);
}

void Test_spec_character_periodic_control_transfers_resync_owners(CuTest *tc)
{
  const char *paths[] = {"src/act.wizard.c", "src/magic/spells.c", "src/character/evolutions.c"};
  const char *transfer_markers[] = {"victim->desc = ch->desc;", "eye->desc = ch->desc;",
                                    "eidolon->desc = ch->desc;"};
  const char *new_owner_syncs[] = {"character_periodic_sync(victim);",
                                   "character_periodic_sync(eye);",
                                   "character_periodic_sync(eidolon);"};
  size_t i;

  for (i = 0; i < sizeof(paths) / sizeof(paths[0]); i++)
  {
    char *source = NULL;
    char *transfer;
    char *old_owner_sync;
    char *new_owner_sync;

    CuAssertTrue(tc, spec_pulse_read_source(paths[i], &source));
    transfer = strstr(source, transfer_markers[i]);
    old_owner_sync = transfer != NULL ? strstr(transfer, "character_periodic_sync(ch);") : NULL;
    new_owner_sync = transfer != NULL ? strstr(transfer, new_owner_syncs[i]) : NULL;
    CuAssertTrue(tc, transfer != NULL && old_owner_sync != NULL && new_owner_sync != NULL);
    free(source);
  }
}

void Test_spec_descriptor_cleanup_forgets_character_periodic_before_free(CuTest *tc)
{
  char *source = NULL;
  char *close_socket;
  char *close_socket_end;
  char *forget_call;
  char *free_call;

  CuAssertTrue(tc, spec_pulse_read_source("src/comm.c", &source));
  close_socket = strstr(source, "void close_socket(struct descriptor_data *d)");
  close_socket_end =
      close_socket != NULL ? strstr(close_socket, "static void check_idle_passwords(void)") : NULL;
  forget_call = spec_pulse_find_in_region(
      close_socket, close_socket_end, "character_periodic_forget(d->character);");
  free_call = spec_pulse_find_in_region(close_socket, close_socket_end, "free_char(d->character);");

  CuAssertTrue(tc, close_socket != NULL && close_socket_end != NULL && forget_call != NULL &&
                       free_call != NULL && forget_call < free_call);
  free(source);
}
