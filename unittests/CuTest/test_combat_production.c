#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/act.h"
#include "../../src/combat/encounters.h"
#include "../../src/combat/fight.h"
#include "../../src/magic/spells.h"

#include <stdlib.h>
#include <string.h>

void Test_combat_production_condensed_stats_initialize_and_reset(CuTest *tc)
{
  struct char_data ch;

  memset(&ch, 0, sizeof(ch));
  init_condensed_combat_data(&ch);
  CuAssertPtrNotNull(tc, CNDNSD(&ch));

  CNDNSD(&ch)->damage_inflicted = 42;
  CNDNSD(&ch)->num_times_hit_by_others = 3;
  init_condensed_combat_data(&ch);
  CuAssertIntEquals(tc, 0, CNDNSD(&ch)->damage_inflicted);
  CuAssertIntEquals(tc, 0, CNDNSD(&ch)->num_times_hit_by_others);

  free(CNDNSD(&ch));
  CNDNSD(&ch) = NULL;
}

void Test_combat_production_npc_barehand_damage_dice(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data player_specials;
  int dice_count;
  int dice_size;

  memset(&ch, 0, sizeof(ch));
  memset(&player_specials, 0, sizeof(player_specials));
  ch.player_specials = &player_specials;
  SET_BIT_AR(MOB_FLAGS(&ch), MOB_ISNPC);
  ch.mob_specials.damnodice = 3;
  ch.mob_specials.damsizedice = 7;
  dice_count = 0;
  dice_size = 0;

  compute_barehand_dam_dice(&ch, &dice_count, &dice_size);
  CuAssertIntEquals(tc, 3, dice_count);
  CuAssertIntEquals(tc, 7, dice_size);
}

void Test_combat_production_damage_type_validation(CuTest *tc)
{
  CuAssertTrue(tc, ok_damage_handling(TYPE_HIT));
  CuAssertTrue(tc, ok_damage_handling(SPELL_MAGIC_MISSILE));
  CuAssertTrue(tc, !ok_damage_handling(TYPE_SUFFERING));
  CuAssertTrue(tc, !ok_damage_handling(SKILL_BASH));
}

void Test_random_encounters_respect_peaceful_rooms(CuTest *tc)
{
  struct room_data room;

  memset(&room, 0, sizeof(room));

  CuAssertTrue(tc, random_encounter_allowed_in_room(&room));
  SET_BIT_AR(room.room_flags, ROOM_PEACEFUL);
  CuAssertTrue(tc, !random_encounter_allowed_in_room(&room));
  CuAssertTrue(tc, !random_encounter_allowed_in_room(NULL));
}

void Test_lich_touch_self_heal_ignores_single_file_reach(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data player_specials;
  struct room_data room;
  struct room_data *saved_world;
  room_rnum saved_top_of_world;
  bool succeeded;

  memset(&ch, 0, sizeof(ch));
  memset(&player_specials, 0, sizeof(player_specials));
  memset(&room, 0, sizeof(room));
  saved_world = world;
  saved_top_of_world = top_of_world;

  world = &room;
  top_of_world = 0;
  SET_BIT_AR(ROOM_FLAGS(0), ROOM_SINGLEFILE);
  room.people = &ch;
  ch.player_specials = &player_specials;
  ch.player.name = "lich touch test character";
  IN_ROOM(&ch) = 0;
  GET_REAL_RACE(&ch) = RACE_LICH;
  GET_LEVEL(&ch) = 30;
  GET_REAL_INT(&ch) = 12;
  GET_REAL_MAX_HIT(&ch) = 100;
  GET_MAX_HIT(&ch) = 100;
  GET_HIT(&ch) = 10;

  succeeded = perform_lichtouch(&ch, &ch);

  world = saved_world;
  top_of_world = saved_top_of_world;

  CuAssertTrue(tc, succeeded);
  CuAssertTrue(tc, GET_HIT(&ch) > 10);
}
