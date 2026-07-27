#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/db.h"
#include "../../src/traps.h"

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
