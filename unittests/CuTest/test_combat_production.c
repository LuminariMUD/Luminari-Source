#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/act.h"
#include "../../src/magic/spells.h"
#include "../../src/character/class.h"
#include "../../src/combat/encounters.h"
#include "../../src/combat/fight.h"
#include "../../src/mudlim.h"
#include "../../src/net/protocol.h"

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

void Test_psionic_death_effects_bypass_the_generic_damage_cap(CuTest *tc)
{
  struct char_data ch;

  memset(&ch, 0, sizeof(ch));

  CuAssertIntEquals(tc, DAMAGE_CAP, test_cap_combat_damage(&ch, 5000, TYPE_HIT));
  CuAssertIntEquals(tc, 5000, test_cap_combat_damage(&ch, 5000, PSIONIC_DEADLY_FEAR));
  CuAssertIntEquals(tc, 5000, test_cap_combat_damage(&ch, 5000, PSIONIC_PSYCHIC_CRUSH));
  CuAssertIntEquals(tc, 5000, test_cap_combat_damage(&ch, 5000, PSIONIC_RECALL_DEATH));
}

void Test_capped_kill_experience_does_not_report_zero_award(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data player_specials;
  struct descriptor_data descriptor;
  int normal_gain;
  int capped_gain;
  bool normal_award_reported;
  bool cap_reported;
  bool zero_award_reported;
  int saved_max_exp_gain;
  int saved_experience_multiplier;

  memset(&ch, 0, sizeof(ch));
  memset(&player_specials, 0, sizeof(player_specials));
  memset(&descriptor, 0, sizeof(descriptor));
  descriptor.output = descriptor.small_outbuf;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  descriptor.character = &ch;
  descriptor.pProtocol = ProtocolCreate();
  ch.desc = &descriptor;
  ch.player_specials = &player_specials;
  ch.player.name = "kill experience test character";
  IN_ROOM(&ch) = NOWHERE;
  GET_CLASS(&ch) = CLASS_WARRIOR;
  GET_LEVEL(&ch) = 12;
  ch.player_specials->saved.stage_info.current_stage = 1;

  if (descriptor.pProtocol == NULL)
  {
    ch.desc = NULL;
    CuFail(tc, "could not initialize the kill experience fixture");
    return;
  }

  saved_max_exp_gain = CONFIG_MAX_EXP_GAIN;
  saved_experience_multiplier = CONFIG_EXPERIENCE_MULTIPLIER;
  CONFIG_MAX_EXP_GAIN = 100000;
  CONFIG_EXPERIENCE_MULTIPLIER = 100;
  GET_EXP(&ch) = level_exp(&ch, GET_LEVEL(&ch));

  normal_gain = test_award_kill_experience(&ch, 100, GAIN_EXP_MODE_SOLO);
  normal_award_reported = strstr(descriptor.output, "You receive 100 experience points.") != NULL;

  descriptor.small_outbuf[0] = '\0';
  descriptor.output = descriptor.small_outbuf;
  descriptor.bufptr = 0;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  GET_EXP(&ch) = level_exp(&ch, GET_LEVEL(&ch) + 2) + 1;

  capped_gain = test_award_kill_experience(&ch, 100, GAIN_EXP_MODE_SOLO);
  cap_reported = strstr(descriptor.output, "Your experience has been capped.") != NULL;
  zero_award_reported = strstr(descriptor.output, "You receive 0 experience points.") != NULL;

  ch.desc = NULL;
  ProtocolDestroy(descriptor.pProtocol);
  CONFIG_MAX_EXP_GAIN = saved_max_exp_gain;
  CONFIG_EXPERIENCE_MULTIPLIER = saved_experience_multiplier;

  CuAssertIntEquals(tc, 100, normal_gain);
  CuAssertTrue(tc, normal_award_reported);
  CuAssertIntEquals(tc, 0, capped_gain);
  CuAssertTrue(tc, cap_reported);
  CuAssertTrue(tc, !zero_award_reported);
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
