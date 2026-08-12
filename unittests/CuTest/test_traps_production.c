#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/db.h"
#include "../../src/handler.h"
#include "../../src/combat/traps.h"

#include <string.h>
#include <time.h>

static void initialize_light_step_staff(struct char_data *ch,
                                        struct player_special_data *player_specials)
{
  memset(ch, 0, sizeof(*ch));
  memset(player_specials, 0, sizeof(*player_specials));

  ch->player_specials = player_specials;
  GET_LEVEL(ch) = LVL_IMMORT;
  ch->aff_abils.dex = 40;
  IN_ROOM(ch) = 0;
  SET_BIT_AR(PRF_FLAGS(ch), PRF_HOLYLIGHT);
}

void Test_traps_light_step_does_not_roll_without_a_trap(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data player_specials;
  struct room_data room;
  struct room_data *saved_world;
  room_rnum saved_top_of_world;
  int expected_roll;
  int actual_roll;
  bool triggered;

  memset(&room, 0, sizeof(room));
  initialize_light_step_staff(&ch, &player_specials);

  saved_world = world;
  saved_top_of_world = top_of_world;
  world = &room;
  top_of_world = 0;

  circle_srandom(12345);
  expected_roll = dice(1, 100);
  circle_srandom(12345);

  triggered = check_trap_trigger(&ch, TRAP_TRIGGER_LEAVE_ROOM, 0, NULL, 0);
  actual_roll = dice(1, 100);

  world = saved_world;
  top_of_world = saved_top_of_world;
  circle_srandom((unsigned long)time(NULL));

  CuAssertTrue(tc, !triggered);
  CuAssertIntEquals(tc, expected_roll, actual_roll);
}

void Test_traps_light_step_rolls_for_an_active_leave_trap(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data player_specials;
  struct room_data room;
  struct trap_data trap;
  struct room_data *saved_world;
  room_rnum saved_top_of_world;
  int avoidance_roll;
  int expected_next_roll;
  int actual_next_roll;
  bool triggered;
  bool trap_was_triggered;

  memset(&room, 0, sizeof(room));
  memset(&trap, 0, sizeof(trap));
  initialize_light_step_staff(&ch, &player_specials);

  trap.trigger_type = TRAP_TRIGGER_LEAVE_ROOM;
  room.traps = &trap;

  saved_world = world;
  saved_top_of_world = top_of_world;
  world = &room;
  top_of_world = 0;

  circle_srandom(12345);
  avoidance_roll = dice(1, 100);
  expected_next_roll = dice(1, 100);
  circle_srandom(12345);

  triggered = check_trap_trigger(&ch, TRAP_TRIGGER_LEAVE_ROOM, 0, NULL, 0);
  actual_next_roll = dice(1, 100);
  trap_was_triggered = IS_SET(trap.flags, TRAP_FLAG_TRIGGERED);

  world = saved_world;
  top_of_world = saved_top_of_world;
  circle_srandom((unsigned long)time(NULL));

  CuAssertTrue(tc, avoidance_roll <= 75);
  CuAssertTrue(tc, !triggered);
  CuAssertTrue(tc, !trap_was_triggered);
  CuAssertIntEquals(tc, expected_next_roll, actual_next_roll);
}

static void initialize_rol_object_trap(struct obj_data *obj, int effect, int damage_type,
                                       int charges)
{
  clear_object(obj);
  obj->name = "test trapped object";
  obj->short_description = "a test trapped object";
  SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_TRAPPED);
  GET_OBJ_VAL(obj, ROL_OBJECT_TRAP_VALUE_EFFECT) = effect;
  GET_OBJ_VAL(obj, ROL_OBJECT_TRAP_VALUE_DAMAGE) = damage_type;
  GET_OBJ_VAL(obj, ROL_OBJECT_TRAP_VALUE_CHARGES) = charges;
  GET_OBJ_VAL(obj, ROL_OBJECT_TRAP_VALUE_LEVEL) = 4;
  GET_OBJ_VAL(obj, ROL_OBJECT_TRAP_VALUE_DICE_COUNT) = 1;
  GET_OBJ_VAL(obj, ROL_OBJECT_TRAP_VALUE_DICE_SIZE) = 1;
}

void Test_traps_rol_object_payload_and_event_matching(CuTest *tc)
{
  struct obj_data obj;

  initialize_rol_object_trap(&obj,
                             ROL_OBJECT_TRAP_EFFECT_MOVE | ROL_OBJECT_TRAP_EFFECT_NORTH |
                                 ROL_OBJECT_TRAP_EFFECT_OBJECT,
                             1, 2);

  CuAssertTrue(tc, rol_object_trap_values_are_valid(&obj));
  CuAssertTrue(tc, is_rol_object_trap(&obj));
  CuAssertTrue(tc, rol_object_trap_matches_event(&obj, ROL_OBJECT_TRAP_EVENT_MOVE, NORTH));
  CuAssertTrue(tc, !rol_object_trap_matches_event(&obj, ROL_OBJECT_TRAP_EVENT_MOVE, SOUTH));
  CuAssertTrue(tc, rol_object_trap_matches_event(&obj, ROL_OBJECT_TRAP_EVENT_OBJECT, 0));
  CuAssertTrue(tc, !rol_object_trap_matches_event(&obj, ROL_OBJECT_TRAP_EVENT_OPEN, 0));

  GET_OBJ_VAL(&obj, ROL_OBJECT_TRAP_VALUE_DICE_SIZE) = 0;
  CuAssertTrue(tc, !rol_object_trap_values_are_valid(&obj));
}

void Test_traps_rol_object_trigger_consumes_charge_and_applies_sleep(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data player_specials;
  struct obj_data obj;
  struct room_data room;
  struct room_data *saved_world;
  room_rnum saved_top_of_world;
  bool triggered;

  memset(&ch, 0, sizeof(ch));
  memset(&player_specials, 0, sizeof(player_specials));
  memset(&room, 0, sizeof(room));
  ch.player_specials = &player_specials;
  ch.player.name = "trap tester";
  GET_LEVEL(&ch) = 1;
  GET_POS(&ch) = POS_STANDING;
  IN_ROOM(&ch) = 0;
  initialize_rol_object_trap(&obj, ROL_OBJECT_TRAP_EFFECT_OBJECT, 11, 2);

  saved_world = world;
  saved_top_of_world = top_of_world;
  world = &room;
  top_of_world = 0;

  triggered = check_rol_object_trap(&ch, &obj, ROL_OBJECT_TRAP_EVENT_OBJECT, 0);

  CuAssertTrue(tc, triggered);
  CuAssertIntEquals(tc, 1, GET_OBJ_VAL(&obj, ROL_OBJECT_TRAP_VALUE_CHARGES));
  CuAssertTrue(tc, AFF_FLAGGED(&ch, AFF_SLEEP));
  CuAssertIntEquals(tc, POS_SLEEPING, GET_POS(&ch));

  while (ch.affected)
    affect_remove(&ch, ch.affected);
  world = saved_world;
  top_of_world = saved_top_of_world;
}

void Test_traps_rol_object_trigger_bypasses_staff_without_consuming_charge(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data player_specials;
  struct obj_data obj;

  initialize_light_step_staff(&ch, &player_specials);
  initialize_rol_object_trap(&obj, ROL_OBJECT_TRAP_EFFECT_OBJECT, 1, 2);

  CuAssertTrue(tc, !check_rol_object_trap(&ch, &obj, ROL_OBJECT_TRAP_EVENT_OBJECT, 0));
  CuAssertIntEquals(tc, 2, GET_OBJ_VAL(&obj, ROL_OBJECT_TRAP_VALUE_CHARGES));
}

void Test_traps_rol_exit_payload_creation_and_copy(CuTest *tc)
{
  struct trap_data *trap, *copy;

  CuAssertTrue(tc, rol_exit_trap_values_are_valid(NORTH, 1, 10, 1, 50, 1, -40, 100));
  CuAssertTrue(tc, !rol_exit_trap_values_are_valid(NORTH, 1, 10, 50, 1, 1, -40, 100));
  CuAssertTrue(tc, !rol_exit_trap_values_are_valid(DIR_COUNT, 1, 10, 1, 50, 1, -40, 100));

  trap = create_rol_exit_trap(NORTH, 1, 10, 1, 50, 1, -40, 100);
  CuAssertPtrNotNull(tc, trap);
  CuAssertTrue(tc, IS_SET(trap->flags, TRAP_FLAG_ROL_EXIT));
  CuAssertTrue(tc, IS_SET(trap->flags, TRAP_FLAG_REUSABLE));
  CuAssertTrue(tc, IS_SET(trap->flags, TRAP_FLAG_AREA_EFFECT));
  CuAssertIntEquals(tc, 10, trap->rol_source_type);
  CuAssertIntEquals(tc, 1, trap->rol_minimum_damage);
  CuAssertIntEquals(tc, 50, trap->rol_maximum_damage);
  CuAssertIntEquals(tc, -40, trap->rol_hardness);

  copy = copy_trap_list(trap);
  CuAssertPtrNotNull(tc, copy);
  CuAssertTrue(tc, copy != trap);
  CuAssertTrue(tc, copy->trap_name != trap->trap_name);
  CuAssertStrEquals(tc, trap->trap_name, copy->trap_name);
  CuAssertIntEquals(tc, trap->rol_source_type, copy->rol_source_type);
  free_trap_list(copy);
  free_trap_list(trap);
}

void Test_traps_rol_exit_rearm_restores_a_disarmed_trap(CuTest *tc)
{
  struct room_data room;
  struct room_data *saved_world;
  struct trap_data *trap;
  room_rnum saved_top_of_world;

  memset(&room, 0, sizeof(room));
  trap = create_rol_exit_trap(EAST, 1, 5, 10, 20, 0, 0, 100);
  CuAssertPtrNotNull(tc, trap);
  SET_BIT(trap->flags, TRAP_FLAG_DETECTED | TRAP_FLAG_DISARMED | TRAP_FLAG_TRIGGERED);
  room.traps = trap;

  saved_world = world;
  saved_top_of_world = top_of_world;
  world = &room;
  top_of_world = 0;

  CuAssertTrue(tc, rol_exit_trap_rearm(0, EAST));
  CuAssertTrue(tc, !IS_SET(trap->flags, TRAP_FLAG_DETECTED));
  CuAssertTrue(tc, !IS_SET(trap->flags, TRAP_FLAG_DISARMED));
  CuAssertTrue(tc, !IS_SET(trap->flags, TRAP_FLAG_TRIGGERED));

  world = saved_world;
  top_of_world = saved_top_of_world;
  free_trap_list(trap);
}
