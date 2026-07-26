#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/db.h"
#include "../../src/dg_scripts.h"

#include <string.h>

void Test_world_loading_production_real_room_lookup(CuTest *tc)
{
  struct room_data fixture[3];
  struct room_data *saved_world;
  room_rnum saved_top_of_world;

  memset(fixture, 0, sizeof(fixture));
  fixture[0].number = 100;
  fixture[1].number = 200;
  fixture[2].number = 300;

  saved_world = world;
  saved_top_of_world = top_of_world;
  world = fixture;
  top_of_world = 2;

  CuAssertIntEquals(tc, 0, real_room(100));
  CuAssertIntEquals(tc, 1, real_room(200));
  CuAssertIntEquals(tc, 2, real_room(300));
  CuAssertIntEquals(tc, NOWHERE, real_room(250));

  world = saved_world;
  top_of_world = saved_top_of_world;
}

void Test_world_loading_production_real_mobile_and_object_lookup(CuTest *tc)
{
  struct index_data mob_fixture[2];
  struct index_data obj_fixture[2];
  struct index_data *saved_mob_index;
  struct index_data *saved_obj_index;
  mob_rnum saved_top_of_mobt;
  obj_rnum saved_top_of_objt;

  memset(mob_fixture, 0, sizeof(mob_fixture));
  memset(obj_fixture, 0, sizeof(obj_fixture));
  mob_fixture[0].vnum = 101;
  mob_fixture[1].vnum = 303;
  obj_fixture[0].vnum = 202;
  obj_fixture[1].vnum = 404;

  saved_mob_index = mob_index;
  saved_obj_index = obj_index;
  saved_top_of_mobt = top_of_mobt;
  saved_top_of_objt = top_of_objt;
  mob_index = mob_fixture;
  obj_index = obj_fixture;
  top_of_mobt = 1;
  top_of_objt = 1;

  CuAssertIntEquals(tc, 0, real_mobile(101));
  CuAssertIntEquals(tc, 1, real_mobile(303));
  CuAssertIntEquals(tc, NOBODY, real_mobile(999));
  CuAssertIntEquals(tc, 0, real_object(202));
  CuAssertIntEquals(tc, 1, real_object(404));
  CuAssertIntEquals(tc, NOTHING, real_object(999));

  mob_index = saved_mob_index;
  obj_index = saved_obj_index;
  top_of_mobt = saved_top_of_mobt;
  top_of_objt = saved_top_of_objt;
}

void Test_world_loading_production_real_trigger_lookup(CuTest *tc)
{
  struct index_data trigger_data[2];
  struct index_data *trigger_fixture[2];
  struct index_data **saved_trig_index;
  trig_rnum saved_top_of_trigt;

  memset(trigger_data, 0, sizeof(trigger_data));
  trigger_data[0].vnum = 501;
  trigger_data[1].vnum = 777;
  trigger_fixture[0] = &trigger_data[0];
  trigger_fixture[1] = &trigger_data[1];

  saved_trig_index = trig_index;
  saved_top_of_trigt = top_of_trigt;
  trig_index = trigger_fixture;
  top_of_trigt = 2;

  CuAssertIntEquals(tc, 0, real_trigger(501));
  CuAssertIntEquals(tc, 1, real_trigger(777));
  CuAssertIntEquals(tc, NOTHING, real_trigger(999));

  trig_index = saved_trig_index;
  top_of_trigt = saved_top_of_trigt;
}
