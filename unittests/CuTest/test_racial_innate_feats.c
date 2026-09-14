/* Tests for the Duris racial innates converted to feats.  See
 * docs/systems/GAME_MECHANICS_SYSTEMS.md */

#include "CuTest.h"

#include "conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/actionqueues.h"
#include "../../src/act.h"
#include "../../src/character/abilities.h"
#include "../../src/character/feats.h"
#include "../../src/character/race.h"
#include "../../src/character/skill_lists.h"
#include "../../src/combat/assign_wpn_armor.h"
#include "../../src/character/class.h"
#include "../../src/combat/fight.h"
#include "../../src/comm.h"
#include "../../src/db.h"
#include "../../src/dgscript/dg_event.h"
#include "../../src/domain_event_runtime.h"
#include "../../src/handler.h"
#include "../../src/interpreter.h"
#include "../../src/lists.h"
#include "../../src/magic/spells.h"
#include "../../src/mud_event.h"
#include "../../src/movement/movement_tracks.h"
#include "../../src/mudlim.h"
#include "../../src/net/protocol.h"
#include "../../src/obj/shop.h"
#include "../../src/pet_vnums.h"
#include "../../src/vessels/vessels.h"
#include "../../src/wilderness/resource_system.h"
#include "../../src/wilderness/kdtree.h"
#include "../../src/wilderness/wilderness.h"

#include <string.h>
#include <time.h>

struct innate_fixture
{
  struct room_data rooms[4]; /* call_magic() only checks rooms strictly below top_of_world */
  struct char_data ch;
  struct char_data other;
  struct player_special_data ch_specials;
  struct player_special_data other_specials;
  struct descriptor_data ch_descriptor;
  struct descriptor_data other_descriptor;
  struct room_data *saved_world;
  struct char_data *saved_character_list;
  room_rnum saved_top_of_world;
  struct weather_data saved_weather;
  struct group_data group;
  struct index_data mobile_index[1]; /* hit() resolves a mob prototype by vnum */
  struct index_data *saved_mob_index;
  mob_rnum saved_top_of_mobt;
  bool mob_index_swapped;
  struct zone_data zone; /* movement needs a zone row for level checks and flags */
  struct room_direction_data north;
  struct zone_data *saved_zone_table;
  zone_rnum saved_top_of_zone_table;
  bool zone_swapped;
};

static void setup_innate_char(struct char_data *ch, struct player_special_data *specials,
                              struct descriptor_data *descriptor, const char *name)
{
  clear_char(ch);
  GET_ATTACK_QUEUE(ch) = create_attack_queue();
  ch->player_specials = specials;
  ch->player.name = (char *)name;
  ch->desc = descriptor;
  IN_ROOM(ch) = 0;
  GET_LEVEL(ch) = 10;
  GET_REAL_SIZE(ch) = SIZE_MEDIUM;
  ch->points.size = SIZE_MEDIUM;
  GET_POS(ch) = POS_STANDING;
  GET_HIT(ch) = 100;
  GET_MAX_HIT(ch) = 100;
  GET_REAL_MAX_HIT(ch) = 100; /* affect_total() rebuilds the maximum from here */

  memset(descriptor, 0, sizeof(*descriptor));
  descriptor->character = ch;
  descriptor->output = descriptor->small_outbuf;
  descriptor->bufspace = SMALL_BUFSIZE - 1;
  descriptor->pProtocol = ProtocolCreate();
  STATE(descriptor) = CON_PLAYING;
}

static void begin_innate_fixture(struct innate_fixture *fixture)
{
  if (feat_list[FEAT_SUN_VULNERABILITY].name == NULL ||
      !strcmp(feat_list[FEAT_SUN_VULNERABILITY].name, "Unused Feat"))
    assign_feats();

  event_init();
  memset(fixture, 0, sizeof(*fixture));
  fixture->saved_world = world;
  fixture->saved_top_of_world = top_of_world;
  fixture->saved_character_list = character_list;
  fixture->saved_weather = weather_info;

  setup_innate_char(&fixture->ch, &fixture->ch_specials, &fixture->ch_descriptor, "innate one");
  setup_innate_char(&fixture->other, &fixture->other_specials, &fixture->other_descriptor,
                    "innate two");

  fixture->ch.next_in_room = &fixture->other;
  fixture->ch.next = &fixture->other;

  fixture->rooms[0].number = 169910;
  fixture->rooms[0].people = &fixture->ch;
  fixture->rooms[1].number = 169911;
  fixture->rooms[2].number = 169912;
  world = fixture->rooms;
  top_of_world = 2;
  character_list = &fixture->ch;
}

static void end_innate_char(struct char_data *ch, struct descriptor_data *descriptor)
{
  while (ch->affected != NULL)
    affect_remove_no_total(ch, ch->affected);
  clear_char_event_list(ch);
  free_attack_queue(GET_ATTACK_QUEUE(ch));
  GET_ATTACK_QUEUE(ch) = NULL;
  ch->desc = NULL;
  if (descriptor->pProtocol != NULL)
    ProtocolDestroy(descriptor->pProtocol);
  if (descriptor->large_outbuf != NULL)
  {
    free(descriptor->large_outbuf->text);
    free(descriptor->large_outbuf);
    descriptor->large_outbuf = NULL;
  }
}

static void end_innate_fixture(struct innate_fixture *fixture)
{
  end_innate_char(&fixture->ch, &fixture->ch_descriptor);
  end_innate_char(&fixture->other, &fixture->other_descriptor);
  event_free_all();
  (void)event_test_select_backend(EVENT_BACKEND_UNINITIALIZED);
  if (fixture->group.members != NULL)
    free_list(fixture->group.members);
  if (fixture->mob_index_swapped)
  {
    mob_index = fixture->saved_mob_index;
    top_of_mobt = fixture->saved_top_of_mobt;
  }
  if (fixture->zone_swapped)
  {
    zone_table = fixture->saved_zone_table;
    top_of_zone_table = fixture->saved_top_of_zone_table;
  }
  world = fixture->saved_world;
  top_of_world = fixture->saved_top_of_world;
  character_list = fixture->saved_character_list;
  weather_info = fixture->saved_weather;
}

/* open an exit from room 0 north into room 1 under a single fixture zone so
 * the movement engine can carry a character across it */
static void innate_fixture_open_north(struct innate_fixture *fixture)
{
  fixture->saved_zone_table = zone_table;
  fixture->saved_top_of_zone_table = top_of_zone_table;
  fixture->zone.min_level = -1;
  fixture->zone.max_level = LVL_IMPL;
  zone_table = &fixture->zone;
  top_of_zone_table = 0;
  fixture->zone_swapped = TRUE;
  movement_trail_registry_shutdown();

  fixture->north.key = NOTHING;
  fixture->north.to_room = 1;
  fixture->rooms[0].dir_option[NORTH] = &fixture->north;
  fixture->rooms[0].name = (char *)"Charge origin";
  fixture->rooms[0].description = (char *)"A test room.\r\n";
  fixture->rooms[1].name = (char *)"Charge destination";
  fixture->rooms[1].description = (char *)"Another test room.\r\n";
}

/* put both fixture characters in one group with a real member list */
static void group_innate_fixture(struct innate_fixture *fixture)
{
  fixture->group.leader = &fixture->ch;
  fixture->group.members = create_list();
  add_to_list(&fixture->ch, fixture->group.members);
  add_to_list(&fixture->other, fixture->group.members);
  fixture->ch.group = &fixture->group;
  fixture->other.group = &fixture->group;
}

/* clear daylight over a field so IN_SUNLIGHT() is true in room 0 */
static void innate_fixture_sunlit_field(struct innate_fixture *fixture)
{
  fixture->rooms[0].sector_type = SECT_FIELD;
  weather_info.sunlight = SUN_LIGHT;
  weather_info.sky = SKY_CLOUDLESS;
}

/* Every converted innate is registered as an in-game, unlearnable innate ability. */
void TestDurisInnateFeatsAreRegisteredAsInnates(CuTest *tc)
{
  struct innate_fixture fixture;
  int feat;

  begin_innate_fixture(&fixture);

  for (feat = FEAT_SUN_VULNERABILITY; feat < FEAT_LAST_FEAT; feat++)
  {
    if (feat == FEAT_FAST_CASTING || feat == FEAT_SLOW_CASTING)
      continue; /* the stacking pair is checked below */
    CuAssertPtrNotNull(tc, feat_list[feat].name);
    CuAssertTrue(tc, strcmp(feat_list[feat].name, "Unused Feat") != 0);
    CuAssertTrue(tc, feat_list[feat].in_game);
    CuAssertTrue(tc, !feat_list[feat].can_learn);
    /* extra arms is the one rank-per-arm innate; everything else is a single grant */
    CuAssertIntEquals(tc, feat == FEAT_EXTRA_ARMS, feat_list[feat].can_stack != 0);
    CuAssertIntEquals(tc, FEAT_TYPE_INNATE_ABILITY, feat_list[feat].feat_type);
  }
  /* the racial casting-speed pair follows the same rules but stacks per rank */
  for (feat = FEAT_FAST_CASTING; feat <= FEAT_SLOW_CASTING; feat++)
  {
    CuAssertPtrNotNull(tc, feat_list[feat].name);
    CuAssertTrue(tc, feat_list[feat].in_game);
    CuAssertTrue(tc, !feat_list[feat].can_learn);
    CuAssertTrue(tc, feat_list[feat].can_stack);
    CuAssertIntEquals(tc, FEAT_TYPE_INNATE_ABILITY, feat_list[feat].feat_type);
  }
  CuAssertIntEquals(tc, FEAT_FOUR_ARMS + 1, FEAT_LAST_FEAT);

  /* the repurposed haste feat follows the same rules */
  CuAssertTrue(tc, feat_list[FEAT_HASTE].in_game);
  CuAssertTrue(tc, !feat_list[FEAT_HASTE].can_learn);
  CuAssertIntEquals(tc, FEAT_TYPE_INNATE_ABILITY, feat_list[FEAT_HASTE].feat_type);

  end_innate_fixture(&fixture);
}

struct racial_sla_expectation
{
  int subcmd;
  int feat;
  int event;
  int uses;
};

static const struct racial_sla_expectation racial_sla_expectations[] = {
    {SCMD_RSLA_FARSEE, FEAT_SLA_FARSEE, eSLA_FARSEE, 3},
    {SCMD_RSLA_STONESKIN, FEAT_SLA_STONESKIN, eSLA_STONESKIN, 1},
    {SCMD_RSLA_LIGHTNING_BOLT, FEAT_SLA_LIGHTNING_BOLT, eSLA_LIGHTNING_BOLT, 3},
    {SCMD_RSLA_FIRE_SHIELD, FEAT_SLA_FIRE_SHIELD, eSLA_FIRE_SHIELD, 1},
    {SCMD_RSLA_FIRE_STORM, FEAT_SLA_FIRE_STORM, eSLA_FIRE_STORM, 1},
    {SCMD_RSLA_SHADOW_JUMP, FEAT_SLA_SHADOW_JUMP, eSLA_SHADOW_JUMP, 1},
    {SCMD_RSLA_PLANE_SHIFT, FEAT_SLA_PLANE_SHIFT, eSLA_PLANE_SHIFT, 1},
    {SCMD_RSLA_PSIONIC_BLAST, FEAT_SLA_PSIONIC_BLAST, eSLA_PSIONIC_BLAST, 3},
    {SCMD_RSLA_SCARE, FEAT_SLA_SCARE, eSLA_SCARE, 3},
    {SCMD_RSLA_HASTE, FEAT_HASTE, eSLA_HASTE, 1},
    {SCMD_RSLA_FIREBALL, FEAT_SLA_FIREBALL, eSLA_FIREBALL, 3},
    {SCMD_RSLA_MASS_DISPEL, FEAT_SLA_MASS_DISPEL, eSLA_MASS_DISPEL, 1},
    {SCMD_RSLA_FROST_BREATH, FEAT_SLA_FROST_BREATH, eSLA_FROST_BREATH, 3},
    {SCMD_RSLA_WEB, FEAT_SLA_WEB, eSLA_WEB, 3},
    {SCMD_RSLA_SUMMON_WARG, FEAT_SUMMON_WARG, eSUMMON_WARG, 1},
    {SCMD_RSLA_SUMMON_HORDE, FEAT_SUMMON_HORDE, eSUMMON_HORDE, 1},
};

/* Each SLA row has its daily count, cooldown event, and a lookup entry. */
void TestRacialSlaRowsHaveDailyUsesAndEvents(CuTest *tc)
{
  struct innate_fixture fixture;
  size_t i;

  begin_innate_fixture(&fixture);

  CuAssertIntEquals(tc, NUM_RACIAL_SLAS,
                    (int)(sizeof(racial_sla_expectations) / sizeof(racial_sla_expectations[0])));
  CuAssertTrue(tc, racial_sla_lookup(-1) == NULL);
  CuAssertTrue(tc, racial_sla_lookup(NUM_RACIAL_SLAS) == NULL);

  for (i = 0; i < sizeof(racial_sla_expectations) / sizeof(racial_sla_expectations[0]); i++)
  {
    const struct racial_sla_expectation *row = &racial_sla_expectations[i];

    CuAssertTrue(tc, racial_sla_lookup(row->subcmd) != NULL);
    CuAssertIntEquals(tc, row->event, feat_list[row->feat].event);
    CuAssertIntEquals(tc, row->uses, get_daily_uses(&fixture.ch, row->feat));
    CuAssertIntEquals(tc, row->uses, daily_uses_remaining(&fixture.ch, row->feat));
  }

  end_innate_fixture(&fixture);
}

/* The verbs refuse without the feat and do not touch the cooldown. */
void TestRacialSlaVerbsRefuseWithoutTheFeat(CuTest *tc)
{
  struct innate_fixture fixture;
  size_t i;

  begin_innate_fixture(&fixture);

  for (i = 0; i < sizeof(racial_sla_expectations) / sizeof(racial_sla_expectations[0]); i++)
  {
    const struct racial_sla_expectation *row = &racial_sla_expectations[i];

    do_racial_sla(&fixture.ch, "", 0, row->subcmd);
    CuAssertTrue(tc, char_has_mud_event(&fixture.ch, row->event) == NULL);
    CuAssertIntEquals(tc, row->uses, daily_uses_remaining(&fixture.ch, row->feat));
  }

  end_innate_fixture(&fixture);
}

/* A use is only spent once the ability actually fires: preconditions such as
 * needing an opponent or a plane name return before the cooldown starts, and
 * one real use starts the daily cooldown event. */
void TestRacialSlaUseStartsTheDailyCooldown(CuTest *tc)
{
  struct innate_fixture fixture;
  struct mud_event_data *event = NULL;

  begin_innate_fixture(&fixture);

  SET_FEAT(&fixture.ch, FEAT_SLA_LIGHTNING_BOLT, 1);
  do_racial_sla(&fixture.ch, "", 0, SCMD_RSLA_LIGHTNING_BOLT); /* not fighting */
  CuAssertTrue(tc, char_has_mud_event(&fixture.ch, eSLA_LIGHTNING_BOLT) == NULL);

  SET_FEAT(&fixture.ch, FEAT_SLA_PLANE_SHIFT, 1);
  do_racial_sla(&fixture.ch, "", 0, SCMD_RSLA_PLANE_SHIFT); /* no plane named */
  CuAssertTrue(tc, char_has_mud_event(&fixture.ch, eSLA_PLANE_SHIFT) == NULL);

  SET_FEAT(&fixture.ch, FEAT_SLA_WEB, 1);
  do_racial_sla(&fixture.ch, "", 0, SCMD_RSLA_WEB); /* no opponent */
  CuAssertTrue(tc, char_has_mud_event(&fixture.ch, eSLA_WEB) == NULL);

  SET_FEAT(&fixture.ch, FEAT_SLA_SHADOW_JUMP, 1);
  do_racial_sla(&fixture.ch, "innate", 0, SCMD_RSLA_SHADOW_JUMP); /* resolves to self */
  CuAssertTrue(tc, char_has_mud_event(&fixture.ch, eSLA_SHADOW_JUMP) == NULL);

  SET_FEAT(&fixture.ch, FEAT_SUMMON_WARG, 1);
  fixture.rooms[0].sector_type = SECT_INSIDE;
  do_racial_sla(&fixture.ch, "", 0, SCMD_RSLA_SUMMON_WARG); /* indoors */
  CuAssertTrue(tc, char_has_mud_event(&fixture.ch, eSUMMON_WARG) == NULL);

  SET_FEAT(&fixture.ch, FEAT_HASTE, 1);
  SET_BIT_AR(AFF_FLAGS(&fixture.ch), AFF_HASTE);
  do_racial_sla(&fixture.ch, "", 0, SCMD_RSLA_HASTE); /* already hasted */
  CuAssertTrue(tc, char_has_mud_event(&fixture.ch, eSLA_HASTE) == NULL);
  REMOVE_BIT_AR(AFF_FLAGS(&fixture.ch), AFF_HASTE);

  SET_FEAT(&fixture.ch, FEAT_SLA_FARSEE, 1);
  CuAssertIntEquals(tc, 1, start_daily_use_cooldown(&fixture.ch, FEAT_SLA_FARSEE));
  CuAssertIntEquals(tc, 2, daily_uses_remaining(&fixture.ch, FEAT_SLA_FARSEE));
  event = char_has_mud_event(&fixture.ch, eSLA_FARSEE);
  CuAssertPtrNotNull(tc, event);
  CuAssertStrEquals(tc, "uses:1", event->sVariables);

  end_innate_fixture(&fixture);
}

/* ---- Phase 1: bucket B feats that replaced race checks ---- */

static bool race_assigns_feat(int race, int feat)
{
  struct race_feat_assign *assign;

  for (assign = race_list[race].featassign_list; assign != NULL; assign = assign->next)
    if (assign->feat_num == feat)
      return TRUE;
  return FALSE;
}

/* The races that used to pass the race checks still hold the feats that
 * replaced them, so their behaviour is unchanged. */
void TestBucketBRacesStillHoldTheirWiredFeats(CuTest *tc)
{
  static const int pairs[][2] = {
      {RACE_HALF_TROLL, FEAT_WEAKNESS_TO_FIRE},
      {RACE_HALF_TROLL, FEAT_BODYSLAM},
      {RACE_TRELUX, FEAT_VULNERABLE_TO_COLD},
      {RACE_TRELUX, FEAT_LEAP},
      {RACE_LICH, FEAT_LICH_SPELL_RESIST},
      {RACE_WEMIC, FEAT_LEONINE_FRAME},
      {RACE_DWARF, FEAT_STABILITY},
      {RACE_DUERGAR, FEAT_STABILITY},
      {RACE_CRYSTAL_DWARF, FEAT_STABILITY},
      {RACE_GOLD_DWARF, FEAT_STABILITY},
      {RACE_DWARF, FEAT_COMBAT_TRAINING_VS_GIANTS},
      {RACE_GNOME, FEAT_COMBAT_TRAINING_VS_GIANTS},
      {RACE_DUERGAR, FEAT_COMBAT_TRAINING_VS_GIANTS},
      {RACE_CRYSTAL_DWARF, FEAT_COMBAT_TRAINING_VS_GIANTS},
      {RACE_HALFLING, FEAT_COMBAT_TRAINING_VS_GIANTS},
  };
  size_t i;

  if (race_list[RACE_WEMIC].type == NULL)
    assign_races();
  if (feat_list[FEAT_SUN_VULNERABILITY].name == NULL ||
      !strcmp(feat_list[FEAT_SUN_VULNERABILITY].name, "Unused Feat"))
    assign_feats();

  for (i = 0; i < sizeof(pairs) / sizeof(pairs[0]); i++)
    CuAssert(tc, feat_list[pairs[i][1]].name, race_assigns_feat(pairs[i][0], pairs[i][1]));

  /* the reworded feats no longer name a race */
  CuAssertTrue(tc, strstr(feat_list[FEAT_KENDER_FEARLESSNESS].name, "ender") == NULL);
  CuAssertTrue(tc, strstr(feat_list[FEAT_KENDER_FEARLESSNESS].description, "Kender") == NULL);
  CuAssertTrue(tc,
               strstr(feat_list[FEAT_COMBAT_TRAINING_VS_GIANTS].short_description, "+4") != NULL);
}

/* Fire and cold vulnerability follow the feat, not the race. */
void TestFireAndColdVulnerabilityFollowTheFeat(CuTest *tc)
{
  struct innate_fixture fixture;
  int base_fire, base_cold;

  begin_innate_fixture(&fixture);

  base_fire = compute_damtype_reduction(&fixture.ch, DAM_FIRE, NULL, TYPE_UNDEFINED);
  base_cold = compute_damtype_reduction(&fixture.ch, DAM_COLD, NULL, TYPE_UNDEFINED);

  SET_FEAT(&fixture.ch, FEAT_WEAKNESS_TO_FIRE, 1);
  CuAssertIntEquals(tc, base_fire - 50,
                    compute_damtype_reduction(&fixture.ch, DAM_FIRE, NULL, TYPE_UNDEFINED));
  CuAssertIntEquals(tc, base_cold,
                    compute_damtype_reduction(&fixture.ch, DAM_COLD, NULL, TYPE_UNDEFINED));

  SET_FEAT(&fixture.ch, FEAT_VULNERABLE_TO_COLD, 1);
  CuAssertIntEquals(tc, base_cold - 20,
                    compute_damtype_reduction(&fixture.ch, DAM_COLD, NULL, TYPE_UNDEFINED));

  end_innate_fixture(&fixture);
}

/* Lich spell resistance (15 + level) follows the feat. */
void TestLichSpellResistanceFollowsTheFeat(CuTest *tc)
{
  struct innate_fixture fixture;

  begin_innate_fixture(&fixture);

  CuAssertTrue(tc, compute_spell_res(NULL, &fixture.ch, 0) < 15 + GET_LEVEL(&fixture.ch));
  SET_FEAT(&fixture.ch, FEAT_LICH_SPELL_RESIST, 1);
  CuAssertIntEquals(tc, 15 + GET_LEVEL(&fixture.ch), compute_spell_res(NULL, &fixture.ch, 0));

  end_innate_fixture(&fixture);
}

/* Leonine frame refuses the leg and foot slots and nothing else. */
void TestLeonineFrameBlocksLegAndFootSlots(CuTest *tc)
{
  struct innate_fixture fixture;

  begin_innate_fixture(&fixture);
  if (race_list[RACE_WEMIC].type == NULL)
    assign_races();

  CuAssertTrue(tc, character_wear_slot_restriction(&fixture.ch, WEAR_LEGS) == NULL);
  CuAssertTrue(tc, character_wear_slot_restriction(&fixture.ch, WEAR_FEET) == NULL);

  SET_FEAT(&fixture.ch, FEAT_LEONINE_FRAME, 1);
  CuAssertPtrNotNull(tc, character_wear_slot_restriction(&fixture.ch, WEAR_LEGS));
  CuAssertPtrNotNull(tc, character_wear_slot_restriction(&fixture.ch, WEAR_FEET));
  CuAssertTrue(tc, character_wear_slot_restriction(&fixture.ch, WEAR_HANDS) == NULL);
  CuAssertTrue(tc, !character_can_use_wear_slot(&fixture.ch, WEAR_LEGS));

  end_innate_fixture(&fixture);
}

/* The bodyslam skill is available with the feat and not without it. */
void TestBodyslamAvailabilityFollowsTheFeat(CuTest *tc)
{
  struct innate_fixture fixture;

  begin_innate_fixture(&fixture);

  CuAssertTrue(tc, !meet_skill_reqs(&fixture.ch, SKILL_BODYSLAM));
  SET_FEAT(&fixture.ch, FEAT_BODYSLAM, 1);
  CuAssertTrue(tc, meet_skill_reqs(&fixture.ch, SKILL_BODYSLAM));

  end_innate_fixture(&fixture);
}

/* Combat training vs giants gives +4 armor class only against larger attackers. */
void TestGiantTrainingGrantsArmorClassAgainstLargerAttackers(CuTest *tc)
{
  struct innate_fixture fixture;
  int base_large, base_same;

  begin_innate_fixture(&fixture);

  fixture.other.points.size = SIZE_LARGE;
  base_large = compute_armor_class(&fixture.other, &fixture.ch, FALSE, MODE_ARMOR_CLASS_NORMAL);
  fixture.other.points.size = SIZE_MEDIUM;
  base_same = compute_armor_class(&fixture.other, &fixture.ch, FALSE, MODE_ARMOR_CLASS_NORMAL);

  SET_FEAT(&fixture.ch, FEAT_COMBAT_TRAINING_VS_GIANTS, 1);
  CuAssertIntEquals(
      tc, base_same,
      compute_armor_class(&fixture.other, &fixture.ch, FALSE, MODE_ARMOR_CLASS_NORMAL));
  fixture.other.points.size = SIZE_LARGE;
  CuAssertIntEquals(
      tc, base_large + 4,
      compute_armor_class(&fixture.other, &fixture.ch, FALSE, MODE_ARMOR_CLASS_NORMAL));

  end_innate_fixture(&fixture);
}

/* Fear immunity is granted by the reworded fearlessness feat. */
void TestFearlessnessFeatGrantsFearImmunity(CuTest *tc)
{
  struct innate_fixture fixture;

  begin_innate_fixture(&fixture);

  CuAssertTrue(tc, !is_immune_fear(&fixture.other, &fixture.ch, FALSE));
  SET_FEAT(&fixture.ch, FEAT_KENDER_FEARLESSNESS, 1);
  CuAssertTrue(tc, is_immune_fear(&fixture.other, &fixture.ch, FALSE));

  end_innate_fixture(&fixture);
}

/* ---- Phase 2: passive defence ---- */

/* Sun vulnerability stops regeneration only in open sunlight. */
void TestSunVulnerabilityStopsRegenerationInOpenSunlight(CuTest *tc)
{
  struct innate_fixture fixture;
  struct obj_data cloak;

  memset(&cloak, 0, sizeof(cloak));
  cloak.item_number = NOTHING; /* no prototype: an ordinary, unregistered cloak */
  begin_innate_fixture(&fixture);
  GET_CLASS(&fixture.ch) = CLASS_WARRIOR;
  GET_COND(&fixture.ch, HUNGER) = 24;
  GET_COND(&fixture.ch, THIRST) = 24;

  innate_fixture_sunlit_field(&fixture);
  CuAssertTrue(tc, !suffers_sun_vulnerability(&fixture.ch));
  CuAssertTrue(tc, hit_gain(&fixture.ch) > 0);
  CuAssertTrue(tc, move_gain(&fixture.ch) > 0);

  SET_FEAT(&fixture.ch, FEAT_SUN_VULNERABILITY, 1);
  CuAssertTrue(tc, suffers_sun_vulnerability(&fixture.ch));
  CuAssertIntEquals(tc, 0, hit_gain(&fixture.ch));
  CuAssertIntEquals(tc, 0, move_gain(&fixture.ch));

  fixture.rooms[0].sector_type = SECT_FOREST; /* sheltered */
  CuAssertTrue(tc, !suffers_sun_vulnerability(&fixture.ch));
  fixture.rooms[0].sector_type = SECT_INSIDE;
  CuAssertTrue(tc, !suffers_sun_vulnerability(&fixture.ch));
  CuAssertTrue(tc, hit_gain(&fixture.ch) > 0);

  /* any cloak worn about the body shelters, unless wind or a grapple strips it */
  fixture.rooms[0].sector_type = SECT_FIELD;
  CuAssertTrue(tc, suffers_sun_vulnerability(&fixture.ch));
  GET_EQ(&fixture.ch, WEAR_ABOUT) = &cloak;
  CuAssertTrue(tc, !suffers_sun_vulnerability(&fixture.ch));
  SET_BIT_AR(AFF_FLAGS(&fixture.ch), AFF_WIND_WALL);
  CuAssertTrue(tc, suffers_sun_vulnerability(&fixture.ch));
  REMOVE_BIT_AR(AFF_FLAGS(&fixture.ch), AFF_WIND_WALL);
  SET_BIT_AR(AFF_FLAGS(&fixture.ch), AFF_GRAPPLED);
  CuAssertTrue(tc, suffers_sun_vulnerability(&fixture.ch));
  REMOVE_BIT_AR(AFF_FLAGS(&fixture.ch), AFF_GRAPPLED);
  GET_EQ(&fixture.ch, WEAR_ABOUT) = NULL;

  end_innate_fixture(&fixture);
}

/* Dayblind blinds in sunlight, not indoors, and never with an eyeless body. */
void TestDayblindFollowsSunlightAndEyeless(CuTest *tc)
{
  struct innate_fixture fixture;

  begin_innate_fixture(&fixture);

  innate_fixture_sunlit_field(&fixture);
  CuAssertTrue(tc, !is_dayblinded(&fixture.ch));
  CuAssertTrue(tc, !char_is_blinded(&fixture.ch));

  SET_FEAT(&fixture.ch, FEAT_DAYBLIND, 1);
  CuAssertTrue(tc, is_dayblinded(&fixture.ch));
  CuAssertTrue(tc, char_is_blinded(&fixture.ch));

  fixture.rooms[0].sector_type = SECT_INSIDE;
  CuAssertTrue(tc, !is_dayblinded(&fixture.ch));

  fixture.rooms[0].sector_type = SECT_FIELD;
  SET_FEAT(&fixture.ch, FEAT_EYELESS, 1);
  CuAssertTrue(tc, !is_dayblinded(&fixture.ch));

  end_innate_fixture(&fixture);
}

/* Eyeless refuses blindness and sees through the blind flag without darkvision. */
void TestEyelessIsImmuneToBlindness(CuTest *tc)
{
  struct innate_fixture fixture;

  begin_innate_fixture(&fixture);

  CuAssertTrue(tc, can_blind(&fixture.ch));
  SET_BIT_AR(AFF_FLAGS(&fixture.ch), AFF_BLIND);
  CuAssertTrue(tc, char_is_blinded(&fixture.ch));

  SET_FEAT(&fixture.ch, FEAT_EYELESS, 1);
  CuAssertTrue(tc, !can_blind(&fixture.ch));
  CuAssertTrue(tc, !char_is_blinded(&fixture.ch));
  CuAssertTrue(tc, !has_blindsense(&fixture.ch));

  end_innate_fixture(&fixture);
}

/* The percentage reductions and vulnerabilities apply by damage type. */
void TestDurisDamageTypeReductions(CuTest *tc)
{
  struct innate_fixture fixture;
  int base_fire, base_force, base_slash, base_holy;

  begin_innate_fixture(&fixture);
  base_fire = compute_damtype_reduction(&fixture.ch, DAM_FIRE, NULL, SPELL_FIREBALL);
  base_force = compute_damtype_reduction(&fixture.ch, DAM_FORCE, NULL, TYPE_HIT);
  base_slash = compute_damtype_reduction(&fixture.ch, DAM_SLASHING, NULL, TYPE_HIT);
  base_holy = compute_damtype_reduction(&fixture.ch, DAM_HOLY, NULL, TYPE_HIT);

  /* magic vulnerability: spells only */
  SET_FEAT(&fixture.ch, FEAT_MAGIC_VULNERABILITY, 1);
  CuAssertIntEquals(tc, base_fire - 10,
                    compute_damtype_reduction(&fixture.ch, DAM_FIRE, NULL, SPELL_FIREBALL));
  CuAssertIntEquals(tc, base_slash,
                    compute_damtype_reduction(&fixture.ch, DAM_SLASHING, NULL, TYPE_HIT));
  SET_FEAT(&fixture.ch, FEAT_MAGIC_VULNERABILITY, 0);

  /* magical reduction: force and energy, not fire */
  SET_FEAT(&fixture.ch, FEAT_MAGICAL_REDUCTION, 1);
  CuAssertIntEquals(tc, base_force + 20,
                    compute_damtype_reduction(&fixture.ch, DAM_FORCE, NULL, TYPE_HIT));
  CuAssertIntEquals(tc, base_fire,
                    compute_damtype_reduction(&fixture.ch, DAM_FIRE, NULL, SPELL_FIREBALL));
  SET_FEAT(&fixture.ch, FEAT_MAGICAL_REDUCTION, 0);

  /* thick hide: physical only */
  SET_FEAT(&fixture.ch, FEAT_THICK_HIDE, 1);
  CuAssertIntEquals(tc, base_slash + 15,
                    compute_damtype_reduction(&fixture.ch, DAM_SLASHING, NULL, TYPE_HIT));
  CuAssertIntEquals(tc, base_fire,
                    compute_damtype_reduction(&fixture.ch, DAM_FIRE, NULL, SPELL_FIREBALL));
  SET_FEAT(&fixture.ch, FEAT_THICK_HIDE, 0);

  /* sacrilegious power: holy, stepping at 20, 25, 30 */
  SET_FEAT(&fixture.ch, FEAT_SACRILEGIOUS_POWER, 1);
  GET_LEVEL(&fixture.ch) = 19;
  CuAssertIntEquals(tc, 0, racial_sacrilegious_power_reduction(&fixture.ch));
  GET_LEVEL(&fixture.ch) = 20;
  CuAssertIntEquals(tc, 25, racial_sacrilegious_power_reduction(&fixture.ch));
  CuAssertIntEquals(tc, base_holy + 25,
                    compute_damtype_reduction(&fixture.ch, DAM_HOLY, NULL, TYPE_HIT));
  GET_LEVEL(&fixture.ch) = 25;
  CuAssertIntEquals(tc, 50, racial_sacrilegious_power_reduction(&fixture.ch));
  GET_LEVEL(&fixture.ch) = 30;
  CuAssertIntEquals(tc, 75, racial_sacrilegious_power_reduction(&fixture.ch));

  end_innate_fixture(&fixture);
}

/* Spell absorb and quick thinking chances follow the feat, level and save type. */
void TestSpellAbsorbAndQuickThinkingChances(CuTest *tc)
{
  struct innate_fixture fixture;

  begin_innate_fixture(&fixture);

  CuAssertIntEquals(tc, 0, racial_spell_absorb_chance(&fixture.ch));
  SET_FEAT(&fixture.ch, FEAT_SPELL_ABSORB, 1);
  CuAssertIntEquals(tc, 5, racial_spell_absorb_chance(&fixture.ch));
  GET_LEVEL(&fixture.ch) = 30;
  CuAssertIntEquals(tc, 15, racial_spell_absorb_chance(&fixture.ch));

  CuAssertIntEquals(tc, 0, racial_quick_thinking_chance(&fixture.ch, SAVING_WILL));
  SET_FEAT(&fixture.ch, FEAT_QUICK_THINKING, 1);
  CuAssertIntEquals(tc, 15, racial_quick_thinking_chance(&fixture.ch, SAVING_WILL));
  CuAssertIntEquals(tc, 0, racial_quick_thinking_chance(&fixture.ch, SAVING_FORT));

  end_innate_fixture(&fixture);
}

/* Groundfighting removes the prone and sitting penalties. */
void TestGroundfightingRemovesPositionPenalties(CuTest *tc)
{
  struct innate_fixture fixture;
  int prone_ac, sitting_ac, sitting_attack;

  begin_innate_fixture(&fixture);

  GET_POS(&fixture.ch) = POS_RECLINING;
  prone_ac = compute_armor_class(NULL, &fixture.ch, FALSE, MODE_ARMOR_CLASS_NORMAL);
  GET_POS(&fixture.ch) = POS_SITTING;
  sitting_ac = compute_armor_class(NULL, &fixture.ch, FALSE, MODE_ARMOR_CLASS_NORMAL);
  sitting_attack = compute_attack_bonus(&fixture.ch, &fixture.other, ATTACK_TYPE_PRIMARY);

  SET_FEAT(&fixture.ch, FEAT_GROUNDFIGHTING, 1);
  CuAssertIntEquals(tc, sitting_ac + 2,
                    compute_armor_class(NULL, &fixture.ch, FALSE, MODE_ARMOR_CLASS_NORMAL));
  CuAssertIntEquals(tc, sitting_attack + 2,
                    compute_attack_bonus(&fixture.ch, &fixture.other, ATTACK_TYPE_PRIMARY));
  GET_POS(&fixture.ch) = POS_RECLINING;
  CuAssertIntEquals(tc, prone_ac + 3,
                    compute_armor_class(NULL, &fixture.ch, FALSE, MODE_ARMOR_CLASS_NORMAL));

  end_innate_fixture(&fixture);
}

/* A quadruped body cannot be knocked down by an attacker of its size and cannot mount. */
void TestQuadrupedBodyResistsKnockdownAndRefusesMounts(CuTest *tc)
{
  struct innate_fixture fixture;

  begin_innate_fixture(&fixture);

  SET_FEAT(&fixture.ch, FEAT_QUADRUPED_BODY, 1);
  CuAssertTrue(tc, !perform_knockdown(&fixture.other, &fixture.ch, SKILL_BASH, FALSE, FALSE));

  do_mount(&fixture.ch, "innate", 0, 0);
  CuAssertTrue(tc, RIDING(&fixture.ch) == NULL);

  end_innate_fixture(&fixture);
}

/* Water breathing sets the permanent water-breath flag on the per-round pass. */
void TestWaterBreathingSetsThePermanentFlag(CuTest *tc)
{
  struct innate_fixture fixture;

  begin_innate_fixture(&fixture);

  update_damage_and_effects_over_time_one(&fixture.ch);
  CuAssertTrue(tc, !AFF_FLAGGED(&fixture.ch, AFF_WATER_BREATH));

  SET_FEAT(&fixture.ch, FEAT_WATER_BREATHING, 1);
  update_damage_and_effects_over_time_one(&fixture.ch);
  CuAssertTrue(tc, AFF_FLAGGED(&fixture.ch, AFF_WATER_BREATH));

  end_innate_fixture(&fixture);
}

/* Undead fealty and calming exempt the character from aggression by level. */
void TestUndeadFealtyAndCalmingAggressionRules(CuTest *tc)
{
  struct innate_fixture fixture;
  struct char_data *mob;

  begin_innate_fixture(&fixture);
  mob = &fixture.other;
  SET_BIT_AR(MOB_FLAGS(mob), MOB_ISNPC);
  GET_REAL_RACE(mob) = RACE_TYPE_UNDEAD;
  GET_LEVEL(&fixture.ch) = 20;

  GET_LEVEL(mob) = 10;
  CuAssertTrue(tc, !undead_fealty_protects(mob, &fixture.ch));
  SET_FEAT(&fixture.ch, FEAT_UNDEAD_FEALTY, 1);
  CuAssertTrue(tc, undead_fealty_protects(mob, &fixture.ch));
  GET_LEVEL(mob) = 11;
  CuAssertTrue(tc, !undead_fealty_protects(mob, &fixture.ch));
  GET_LEVEL(mob) = 10;
  GET_REAL_RACE(mob) = RACE_TYPE_ANIMAL;
  CuAssertTrue(tc, !undead_fealty_protects(mob, &fixture.ch));

  GET_LEVEL(mob) = 25;
  CuAssertTrue(tc, !calming_applies(mob, &fixture.ch));
  SET_FEAT(&fixture.ch, FEAT_CALMING, 1);
  CuAssertTrue(tc, calming_applies(mob, &fixture.ch));
  GET_LEVEL(mob) = 26;
  CuAssertTrue(tc, !calming_applies(mob, &fixture.ch));
  GET_LEVEL(mob) = 15;
  CuAssertTrue(tc, calming_applies(mob, &fixture.ch));

  end_innate_fixture(&fixture);
}

/* ---- Phase 2: passive offence ---- */

static void make_test_weapon(struct obj_data *obj, int weapon_type)
{
  memset(obj, 0, sizeof(*obj));
  GET_OBJ_TYPE(obj) = ITEM_WEAPON;
  GET_OBJ_VAL(obj, 0) = weapon_type;
}

/* Weapon-family mastery scales with level and only for a matching weapon. */
void TestWeaponMasteryScalesWithLevelAndWeapon(CuTest *tc)
{
  struct innate_fixture fixture;
  struct obj_data sword, axe;
  int base_damage;

  begin_innate_fixture(&fixture);
  make_test_weapon(&sword, WEAPON_TYPE_LONG_SWORD);
  make_test_weapon(&axe, WEAPON_TYPE_BATTLE_AXE);

  base_damage = compute_damage_bonus(&fixture.ch, &fixture.other, &sword, TYPE_HIT, 0,
                                     MODE_NORMAL_HIT, ATTACK_TYPE_PRIMARY);

  SET_FEAT(&fixture.ch, FEAT_LONGSWORD_MASTERY, 1);
  GET_LEVEL(&fixture.ch) = 7;
  CuAssertIntEquals(tc, 0, racial_weapon_mastery_bonus(&fixture.ch, &sword));
  GET_LEVEL(&fixture.ch) = 8;
  CuAssertIntEquals(tc, 1, racial_weapon_mastery_bonus(&fixture.ch, &sword));
  GET_LEVEL(&fixture.ch) = 24;
  CuAssertIntEquals(tc, 3, racial_weapon_mastery_bonus(&fixture.ch, &sword));
  GET_LEVEL(&fixture.ch) = 30;
  CuAssertIntEquals(tc, 3, racial_weapon_mastery_bonus(&fixture.ch, &sword));
  CuAssertIntEquals(tc, 0, racial_weapon_mastery_bonus(&fixture.ch, &axe));
  CuAssertIntEquals(tc, 0, racial_weapon_mastery_bonus(&fixture.ch, NULL));

  GET_LEVEL(&fixture.ch) = 10;
  CuAssertIntEquals(tc, base_damage + 1,
                    compute_damage_bonus(&fixture.ch, &fixture.other, &sword, TYPE_HIT, 0,
                                         MODE_NORMAL_HIT, ATTACK_TYPE_PRIMARY));

  SET_FEAT(&fixture.ch, FEAT_AXE_MASTERY, 1);
  CuAssertIntEquals(tc, 1, racial_weapon_mastery_bonus(&fixture.ch, &axe));

  end_innate_fixture(&fixture);
}

/* Hatred adds damage against evil opponents only. */
void TestHatredAddsDamageAgainstEvil(CuTest *tc)
{
  struct innate_fixture fixture;
  int base_damage;

  begin_innate_fixture(&fixture);
  GET_ALIGNMENT(&fixture.other) = 0;
  base_damage = compute_damage_bonus(&fixture.ch, &fixture.other, NULL, TYPE_HIT, 0,
                                     MODE_NORMAL_HIT, ATTACK_TYPE_UNARMED);

  SET_FEAT(&fixture.ch, FEAT_HATRED, 1);
  CuAssertIntEquals(tc, base_damage,
                    compute_damage_bonus(&fixture.ch, &fixture.other, NULL, TYPE_HIT, 0,
                                         MODE_NORMAL_HIT, ATTACK_TYPE_UNARMED));
  GET_ALIGNMENT(&fixture.other) = -1000;
  CuAssertIntEquals(tc, base_damage + 2,
                    compute_damage_bonus(&fixture.ch, &fixture.other, NULL, TYPE_HIT, 0,
                                         MODE_NORMAL_HIT, ATTACK_TYPE_UNARMED));

  end_innate_fixture(&fixture);
}

/* Battle frenzy only triggers on melee hits against humanoids. */
void TestBattleFrenzyGate(CuTest *tc)
{
  struct innate_fixture fixture;

  begin_innate_fixture(&fixture);

  CuAssertTrue(tc, !battle_frenzy_applies(&fixture.ch, &fixture.other, ATTACK_TYPE_PRIMARY));
  SET_FEAT(&fixture.ch, FEAT_BATTLE_FRENZY, 1);
  CuAssertTrue(tc, battle_frenzy_applies(&fixture.ch, &fixture.other, ATTACK_TYPE_PRIMARY));
  CuAssertTrue(tc, !battle_frenzy_applies(&fixture.ch, &fixture.other, ATTACK_TYPE_RANGED));

  SET_BIT_AR(MOB_FLAGS(&fixture.other), MOB_ISNPC);
  GET_REAL_RACE(&fixture.other) = RACE_TYPE_ANIMAL;
  CuAssertTrue(tc, !battle_frenzy_applies(&fixture.ch, &fixture.other, ATTACK_TYPE_PRIMARY));

  end_innate_fixture(&fixture);
}

/* Extra arms adds one melee attack per rank and never touches the ranged count. */
void TestExtraArmsAddMeleeAttacksPerRankOnly(CuTest *tc)
{
  struct innate_fixture fixture;
  struct obj_data bow;
  int melee_attacks, ranged_attacks;

  begin_innate_fixture(&fixture);
  if (!IS_SET(weapon_list[WEAPON_TYPE_LONG_BOW].weaponFlags, WEAPON_FLAG_RANGED))
    load_weapons();
  make_test_weapon(&bow, WEAPON_TYPE_LONG_BOW);

#define RETURN_NUM_ATTACKS 1
#define PHASE_0 0
  melee_attacks = perform_attacks(&fixture.ch, RETURN_NUM_ATTACKS, PHASE_0);
  GET_EQ(&fixture.ch, WEAR_WIELD_1) = &bow;
  ranged_attacks = perform_attacks(&fixture.ch, RETURN_NUM_ATTACKS, PHASE_0);
  GET_EQ(&fixture.ch, WEAR_WIELD_1) = NULL;

  SET_FEAT(&fixture.ch, FEAT_EXTRA_ARMS, 1);
  CuAssertIntEquals(tc, melee_attacks + 1,
                    perform_attacks(&fixture.ch, RETURN_NUM_ATTACKS, PHASE_0));

  /* the Thri-Kreen shape: two extra arms, two extra swings */
  SET_FEAT(&fixture.ch, FEAT_EXTRA_ARMS, 2);
  CuAssertIntEquals(tc, melee_attacks + 2,
                    perform_attacks(&fixture.ch, RETURN_NUM_ATTACKS, PHASE_0));

  GET_EQ(&fixture.ch, WEAR_WIELD_1) = &bow;
  CuAssertIntEquals(tc, ranged_attacks, perform_attacks(&fixture.ch, RETURN_NUM_ATTACKS, PHASE_0));
  GET_EQ(&fixture.ch, WEAR_WIELD_1) = NULL;
#undef PHASE_0
#undef RETURN_NUM_ATTACKS

  end_innate_fixture(&fixture);
}

/* Warcaller's fury counts grouped members in the room; rrakkma counts feat holders. */
void TestWarcallersFuryAndRrakkmaCountTheGroup(CuTest *tc)
{
  struct innate_fixture fixture;
  int base_damage, base_ac;

  begin_innate_fixture(&fixture);
  base_damage = compute_damage_bonus(&fixture.ch, &fixture.other, NULL, TYPE_HIT, 0,
                                     MODE_NORMAL_HIT, ATTACK_TYPE_UNARMED);
  base_ac = compute_armor_class(NULL, &fixture.ch, FALSE, MODE_ARMOR_CLASS_NORMAL);

  SET_FEAT(&fixture.ch, FEAT_WARCALLERS_FURY, 1);
  SET_FEAT(&fixture.ch, FEAT_RRAKKMA, 1);
  CuAssertIntEquals(tc, 0, racial_warcallers_fury_bonus(&fixture.ch)); /* ungrouped */
  CuAssertIntEquals(tc, 0, racial_rrakkma_allies(&fixture.ch));

  group_innate_fixture(&fixture);
  CuAssertIntEquals(tc, 2, racial_warcallers_fury_bonus(&fixture.ch));
  CuAssertIntEquals(tc, base_damage + 2,
                    compute_damage_bonus(&fixture.ch, &fixture.other, NULL, TYPE_HIT, 0,
                                         MODE_NORMAL_HIT, ATTACK_TYPE_UNARMED));
  CuAssertIntEquals(tc, 0, racial_rrakkma_allies(&fixture.ch)); /* ally lacks the feat */
  CuAssertIntEquals(tc, base_ac,
                    compute_armor_class(NULL, &fixture.ch, FALSE, MODE_ARMOR_CLASS_NORMAL));

  SET_FEAT(&fixture.other, FEAT_RRAKKMA, 1);
  CuAssertIntEquals(tc, 1, racial_rrakkma_allies(&fixture.ch));
  CuAssertIntEquals(tc, base_ac + 1,
                    compute_armor_class(NULL, &fixture.ch, FALSE, MODE_ARMOR_CLASS_NORMAL));

  IN_ROOM(&fixture.other) = 1; /* elsewhere: neither counts */
  CuAssertIntEquals(tc, 1, racial_warcallers_fury_bonus(&fixture.ch));
  CuAssertIntEquals(tc, 0, racial_rrakkma_allies(&fixture.ch));

  end_innate_fixture(&fixture);
}

/* ---- Phase 3: terrain and utility ---- */

/* Each terrain stealth feat pays +6 only in its own sector set; forest sight +4 in forests. */
void TestTerrainStealthAndForestSightFollowTheSector(CuTest *tc)
{
  struct innate_fixture fixture;
  int base_stealth, base_perception;

  begin_innate_fixture(&fixture);
  fixture.rooms[0].sector_type = SECT_FIELD;
  base_stealth = compute_ability(&fixture.ch, ABILITY_STEALTH);
  base_perception = compute_ability(&fixture.ch, ABILITY_PERCEPTION);

  SET_FEAT(&fixture.ch, FEAT_OUTDOOR_STEALTH, 1);
  CuAssertIntEquals(tc, 6, racial_terrain_ability_bonus(&fixture.ch, ABILITY_STEALTH));
  CuAssertIntEquals(tc, base_stealth + 6, compute_ability(&fixture.ch, ABILITY_STEALTH));
  fixture.rooms[0].sector_type = SECT_UD_WILD;
  CuAssertIntEquals(tc, 0, racial_terrain_ability_bonus(&fixture.ch, ABILITY_STEALTH));
  fixture.rooms[0].sector_type = SECT_INSIDE;
  CuAssertIntEquals(tc, 0, racial_terrain_ability_bonus(&fixture.ch, ABILITY_STEALTH));
  SET_FEAT(&fixture.ch, FEAT_OUTDOOR_STEALTH, 0);

  SET_FEAT(&fixture.ch, FEAT_SWAMP_STEALTH, 1);
  fixture.rooms[0].sector_type = SECT_FIELD;
  CuAssertIntEquals(tc, 0, racial_terrain_ability_bonus(&fixture.ch, ABILITY_STEALTH));
  fixture.rooms[0].sector_type = SECT_MARSHLAND;
  CuAssertIntEquals(tc, 6, racial_terrain_ability_bonus(&fixture.ch, ABILITY_STEALTH));
  SET_FEAT(&fixture.ch, FEAT_SWAMP_STEALTH, 0);

  SET_FEAT(&fixture.ch, FEAT_UNDERDARK_STEALTH, 1);
  CuAssertIntEquals(tc, 0, racial_terrain_ability_bonus(&fixture.ch, ABILITY_STEALTH));
  fixture.rooms[0].sector_type = SECT_UD_NOGROUND;
  CuAssertIntEquals(tc, 6, racial_terrain_ability_bonus(&fixture.ch, ABILITY_STEALTH));
  /* the feats do not stack in a shared sector */
  SET_FEAT(&fixture.ch, FEAT_OUTDOOR_STEALTH, 1);
  fixture.rooms[0].sector_type = SECT_MARSHLAND;
  SET_FEAT(&fixture.ch, FEAT_SWAMP_STEALTH, 1);
  CuAssertIntEquals(tc, 6, racial_terrain_ability_bonus(&fixture.ch, ABILITY_STEALTH));

  fixture.rooms[0].sector_type = SECT_FOREST;
  CuAssertIntEquals(tc, 0, racial_terrain_ability_bonus(&fixture.ch, ABILITY_PERCEPTION));
  SET_FEAT(&fixture.ch, FEAT_FOREST_SIGHT, 1);
  CuAssertIntEquals(tc, 4, racial_terrain_ability_bonus(&fixture.ch, ABILITY_PERCEPTION));
  CuAssertIntEquals(tc, base_perception + 4, compute_ability(&fixture.ch, ABILITY_PERCEPTION));
  fixture.rooms[0].sector_type = SECT_FIELD;
  CuAssertIntEquals(tc, 0, racial_terrain_ability_bonus(&fixture.ch, ABILITY_PERCEPTION));

  end_innate_fixture(&fixture);
}

/* Miner raises the harvest skill for minerals, stone and crystal only. */
void TestMinerRaisesMineralHarvestSkill(CuTest *tc)
{
  struct innate_fixture fixture;
  int base_minerals, base_herbs;

  begin_innate_fixture(&fixture);
  base_minerals = get_harvest_skill_level(&fixture.ch, RESOURCE_MINERALS);
  base_herbs = get_harvest_skill_level(&fixture.ch, RESOURCE_HERBS);

  SET_FEAT(&fixture.ch, FEAT_MINER, 1);
  CuAssertIntEquals(tc, base_minerals + 4, get_harvest_skill_level(&fixture.ch, RESOURCE_MINERALS));
  CuAssertIntEquals(tc, base_minerals + 4, get_harvest_skill_level(&fixture.ch, RESOURCE_STONE));
  CuAssertIntEquals(tc, base_minerals + 4, get_harvest_skill_level(&fixture.ch, RESOURCE_CRYSTAL));
  CuAssertIntEquals(tc, base_herbs, get_harvest_skill_level(&fixture.ch, RESOURCE_HERBS));

  end_innate_fixture(&fixture);
}

/* Barter is worth ten points of charisma in the shop haggle, seadog one tile at the helm. */
void TestBarterAndSeadogBonuses(CuTest *tc)
{
  struct innate_fixture fixture;
  int base_score;

  begin_innate_fixture(&fixture);
  base_score = shop_haggle_score(&fixture.ch);
  SET_FEAT(&fixture.ch, FEAT_BARTER, 1);
  CuAssertIntEquals(tc, base_score + 10, shop_haggle_score(&fixture.ch));

  CuAssertIntEquals(tc, 0, vessel_pilot_speed_bonus(NULL));
  CuAssertIntEquals(tc, 0, vessel_pilot_speed_bonus(&fixture.ch));
  SET_FEAT(&fixture.ch, FEAT_SEADOG, 1);
  CuAssertIntEquals(tc, 1, vessel_pilot_speed_bonus(&fixture.ch));

  end_innate_fixture(&fixture);
}

/* ---- Phase 4: bespoke commands ---- */

/* Flurry applies a four-round haste affect once per day and never stacks with haste. */
void TestRacialFlurryAppliesAShortHasteAffect(CuTest *tc)
{
  struct innate_fixture fixture;
  struct affected_type *af;

  begin_innate_fixture(&fixture);

  do_racial_flurry(&fixture.ch, "", 0, 0); /* no feat */
  CuAssertTrue(tc, !affected_by_spell(&fixture.ch, AFFECT_RACIAL_FLURRY));

  SET_FEAT(&fixture.ch, FEAT_RACIAL_FLURRY, 1);
  do_racial_flurry(&fixture.ch, "", 0, 0);
  CuAssertTrue(tc, affected_by_spell(&fixture.ch, AFFECT_RACIAL_FLURRY));
  CuAssertTrue(tc, AFF_FLAGGED(&fixture.ch, AFF_HASTE));
  CuAssertIntEquals(tc, 0, daily_uses_remaining(&fixture.ch, FEAT_RACIAL_FLURRY));
  for (af = fixture.ch.affected; af != NULL; af = af->next)
    if (af->spell == AFFECT_RACIAL_FLURRY)
      CuAssertIntEquals(tc, 4, af->duration);

  /* already hasted by something else: refused, no use spent */
  while (fixture.ch.affected != NULL)
    affect_remove_no_total(&fixture.ch, fixture.ch.affected);
  clear_char_event_list(&fixture.ch);
  SET_BIT_AR(AFF_FLAGS(&fixture.ch), AFF_HASTE);
  do_racial_flurry(&fixture.ch, "", 0, 0);
  CuAssertTrue(tc, !affected_by_spell(&fixture.ch, AFFECT_RACIAL_FLURRY));
  CuAssertIntEquals(tc, 1, daily_uses_remaining(&fixture.ch, FEAT_RACIAL_FLURRY));

  end_innate_fixture(&fixture);
}

/* Doorbash refuses without the feat and on pickproof doors; the forced open clears both sides. */
void TestDoorbashOpensBothSidesOfTheDoor(CuTest *tc)
{
  struct innate_fixture fixture;
  struct room_direction_data east, west;

  begin_innate_fixture(&fixture);
  memset(&east, 0, sizeof(east));
  memset(&west, 0, sizeof(west));
  east.to_room = 1;
  east.exit_info = EX_ISDOOR | EX_CLOSED | EX_LOCKED;
  west.to_room = 0;
  west.exit_info = EX_ISDOOR | EX_CLOSED | EX_LOCKED;
  fixture.rooms[0].dir_option[EAST] = &east;
  fixture.rooms[1].dir_option[WEST] = &west;

  do_doorbash(&fixture.ch, "east", 0, 0); /* no feat */
  CuAssertTrue(tc, IS_SET(east.exit_info, EX_CLOSED));

  SET_FEAT(&fixture.ch, FEAT_DOORBASH, 1);
  east.exit_info |= EX_PICKPROOF;
  do_doorbash(&fixture.ch, "east", 0, 0); /* pickproof */
  CuAssertTrue(tc, IS_SET(east.exit_info, EX_CLOSED));
  east.exit_info &= ~EX_PICKPROOF;

  doorbash_open_exit(&fixture.ch, EAST);
  CuAssertTrue(tc, !IS_SET(east.exit_info, EX_CLOSED));
  CuAssertTrue(tc, !IS_SET(east.exit_info, EX_LOCKED));
  CuAssertTrue(tc, !IS_SET(west.exit_info, EX_CLOSED));
  CuAssertTrue(tc, !IS_SET(west.exit_info, EX_LOCKED));

  fixture.rooms[0].dir_option[EAST] = NULL;
  fixture.rooms[1].dir_option[WEST] = NULL;
  end_innate_fixture(&fixture);
}

/* Stampede needs the feat, an open room, and an opponent before it starts its cooldown. */
void TestStampedeRefusals(CuTest *tc)
{
  struct innate_fixture fixture;

  begin_innate_fixture(&fixture);

  do_stampede(&fixture.ch, "", 0, 0); /* no feat */
  CuAssertTrue(tc, char_has_mud_event(&fixture.ch, eSTAMPEDE) == NULL);

  SET_FEAT(&fixture.ch, FEAT_STAMPEDE, 1);
  do_stampede(&fixture.ch, "", 0, 0); /* not fighting */
  CuAssertTrue(tc, char_has_mud_event(&fixture.ch, eSTAMPEDE) == NULL);

  SET_BIT_AR(ROOM_FLAGS(0), ROOM_SINGLEFILE);
  FIGHTING(&fixture.ch) = &fixture.other;
  do_stampede(&fixture.ch, "", 0, 0); /* single file */
  CuAssertTrue(tc, char_has_mud_event(&fixture.ch, eSTAMPEDE) == NULL);
  FIGHTING(&fixture.ch) = NULL;
  REMOVE_BIT_AR(ROOM_FLAGS(0), ROOM_SINGLEFILE);

  end_innate_fixture(&fixture);
}

/* The cooldown and the action are only spent with someone to run over: an
 * opponent in another room or in the air is not one, ch's own opponent who has
 * not turned to fight back yet is. */
void TestStampedeSpendsOnlyWithSomeoneToTrample(CuTest *tc)
{
  struct innate_fixture fixture;

  begin_innate_fixture(&fixture);
  SET_FEAT(&fixture.ch, FEAT_STAMPEDE, 1);
  FIGHTING(&fixture.ch) = &fixture.other;

  /* opponent elsewhere */
  fixture.ch.next_in_room = NULL;
  IN_ROOM(&fixture.other) = 1;
  fixture.rooms[1].people = &fixture.other;
  do_stampede(&fixture.ch, "", 0, 0);
  CuAssertTrue(tc, char_has_mud_event(&fixture.ch, eSTAMPEDE) == NULL);

  /* opponent here but flying */
  fixture.rooms[1].people = NULL;
  IN_ROOM(&fixture.other) = 0;
  fixture.ch.next_in_room = &fixture.other;
  SET_BIT_AR(AFF_FLAGS(&fixture.other), AFF_FLYING);
  do_stampede(&fixture.ch, "", 0, 0);
  CuAssertTrue(tc, char_has_mud_event(&fixture.ch, eSTAMPEDE) == NULL);
  REMOVE_BIT_AR(AFF_FLAGS(&fixture.other), AFF_FLYING);

  /* ch's own opponent on the ground, not yet fighting back */
  do_stampede(&fixture.ch, "", 0, 0);
  CuAssertPtrNotNull(tc, char_has_mud_event(&fixture.ch, eSTAMPEDE));

  FIGHTING(&fixture.ch) = NULL;
  FIGHTING(&fixture.other) = NULL;
  end_innate_fixture(&fixture);
}

/* A spell-like ability that fizzles keeps its daily use. */
void TestRacialSlaFizzleKeepsTheDailyUse(CuTest *tc)
{
  struct innate_fixture fixture;

  begin_innate_fixture(&fixture);
  SET_FEAT(&fixture.ch, FEAT_SLA_FIRE_STORM, 1);
  fixture.rooms[0].people = &fixture.other;
  fixture.ch.next_in_room = NULL;
  IN_ROOM(&fixture.ch) = 1;
  fixture.rooms[1].people = &fixture.ch;
  SET_BIT_AR(ROOM_FLAGS(1), ROOM_NOMAGIC);

  do_racial_sla(&fixture.ch, "", 0, SCMD_RSLA_FIRE_STORM);
  CuAssertTrue(tc, char_has_mud_event(&fixture.ch, eSLA_FIRE_STORM) == NULL);
  CuAssertIntEquals(tc, 1, daily_uses_remaining(&fixture.ch, FEAT_SLA_FIRE_STORM));

  REMOVE_BIT_AR(ROOM_FLAGS(1), ROOM_NOMAGIC);
  end_innate_fixture(&fixture);
}

/* Mass dispel skips groupmates, never starts a fight, and spends its use only
 * once an affect actually came off a hostile. */
void TestMassDispelSkipsAlliesAndNeverStartsAFight(CuTest *tc)
{
  struct innate_fixture fixture;
  struct affected_type af;
  struct char_data *caster = &fixture.ch;

  begin_innate_fixture(&fixture);
  fixture.rooms[0].light = 1;
  SET_FEAT(&fixture.ch, FEAT_SLA_MASS_DISPEL, 1);
  new_affect(&af);
  af.spell = SPELL_HASTE;
  af.duration = 5;
  affect_to_char(&fixture.other, &af);

  /* a groupmate is left alone and no use is spent */
  group_innate_fixture(&fixture);
  do_racial_sla(&fixture.ch, "", 0, SCMD_RSLA_MASS_DISPEL);
  CuAssertTrue(tc, affected_by_spell(&fixture.other, SPELL_HASTE));
  CuAssertTrue(tc, FIGHTING(&fixture.ch) == NULL);
  CuAssertTrue(tc, FIGHTING(&fixture.other) == NULL);
  CuAssertIntEquals(tc, 1, daily_uses_remaining(&fixture.ch, FEAT_SLA_MASS_DISPEL));
  fixture.ch.group = NULL;
  fixture.other.group = NULL;

  /* a hostile mob is dispelled without anyone entering combat, and the use
   * goes with the strip.  Caster level 30 against a level 1 mob makes the
   * d20 dispel check (30 + 1 vs at most 1 + 20) certain. */
  SET_BIT_AR(MOB_FLAGS(&fixture.other), MOB_ISNPC);
  fixture.other.player.short_descr = (char *)"innate two";
  GET_LEVEL(&fixture.other) = 1;
  CLASS_LEVEL(caster, CLASS_WIZARD) = 30;
  do_racial_sla(&fixture.ch, "", 0, SCMD_RSLA_MASS_DISPEL);
  CuAssertTrue(tc, FIGHTING(&fixture.ch) == NULL);
  CuAssertTrue(tc, FIGHTING(&fixture.other) == NULL);
  CuAssertTrue(tc, !affected_by_spell(&fixture.other, SPELL_HASTE));
  CuAssertIntEquals(tc, 0, daily_uses_remaining(&fixture.ch, FEAT_SLA_MASS_DISPEL));
  CLASS_LEVEL(caster, CLASS_WIZARD) = 0;
  REMOVE_BIT_AR(MOB_FLAGS(&fixture.other), MOB_ISNPC);

  end_innate_fixture(&fixture);
}

/* Slow strips the racial flurry like any haste, and onslaught refuses while slowed. */
void TestSlowStripsTheRacialFlurry(CuTest *tc)
{
  struct innate_fixture fixture;

  begin_innate_fixture(&fixture);
  SET_FEAT(&fixture.ch, FEAT_RACIAL_FLURRY, 1);

  do_racial_flurry(&fixture.ch, "", 0, 0);
  CuAssertTrue(tc, affected_by_spell(&fixture.ch, AFFECT_RACIAL_FLURRY));
  mag_affects(10, &fixture.other, &fixture.ch, NULL, SPELL_SLOW, SAVING_WILL, CAST_INNATE, 0);
  CuAssertTrue(tc, !affected_by_spell(&fixture.ch, AFFECT_RACIAL_FLURRY));
  CuAssertTrue(tc, !AFF_FLAGGED(&fixture.ch, AFF_HASTE));

  clear_char_event_list(&fixture.ch);
  SET_BIT_AR(AFF_FLAGS(&fixture.ch), AFF_SLOW);
  do_racial_flurry(&fixture.ch, "", 0, 0);
  CuAssertTrue(tc, !affected_by_spell(&fixture.ch, AFFECT_RACIAL_FLURRY));
  CuAssertIntEquals(tc, 1, daily_uses_remaining(&fixture.ch, FEAT_RACIAL_FLURRY));
  REMOVE_BIT_AR(AFF_FLAGS(&fixture.ch), AFF_SLOW);

  end_innate_fixture(&fixture);
}

/* Shadow jump needs both rooms in shadow, refuses no-teleport and powerful
 * targets, and otherwise moves the caster. */
void TestShadowJumpNeedsShadowInBothRooms(CuTest *tc)
{
  struct innate_fixture fixture;
  struct zone_data zone;
  struct zone_data *saved_zone_table = zone_table;
  zone_rnum saved_top_of_zone_table = top_of_zone_table;

  begin_innate_fixture(&fixture);
  memset(&zone, 0, sizeof(zone));
  zone_table = &zone;
  top_of_zone_table = 0;
  fixture.ch.player.title = (char *)"";
  fixture.other.player.title = (char *)"";
  fixture.ch.next_in_room = NULL;
  IN_ROOM(&fixture.other) = 1;
  fixture.rooms[1].people = &fixture.other;

  /* a lit target room refuses */
  SET_BIT_AR(ROOM_FLAGS(1), ROOM_MAGICLIGHT);
  spell_shadow_jump(10, &fixture.ch, &fixture.other, NULL, CAST_INNATE);
  CuAssertIntEquals(tc, 0, IN_ROOM(&fixture.ch));
  REMOVE_BIT_AR(ROOM_FLAGS(1), ROOM_MAGICLIGHT);

  /* teleport's unique-mob guards apply */
  SET_BIT_AR(MOB_FLAGS(&fixture.other), MOB_ISNPC);
  fixture.other.player.short_descr = (char *)"innate two";
  SET_BIT_AR(MOB_FLAGS(&fixture.other), MOB_NOTELEPORT);
  spell_shadow_jump(10, &fixture.ch, &fixture.other, NULL, CAST_INNATE);
  CuAssertIntEquals(tc, 0, IN_ROOM(&fixture.ch));
  REMOVE_BIT_AR(MOB_FLAGS(&fixture.other), MOB_NOTELEPORT);
  GET_LEVEL(&fixture.other) = LVL_IMMORT;
  spell_shadow_jump(10, &fixture.ch, &fixture.other, NULL, CAST_INNATE);
  CuAssertIntEquals(tc, 0, IN_ROOM(&fixture.ch));
  GET_LEVEL(&fixture.other) = 10;
  REMOVE_BIT_AR(MOB_FLAGS(&fixture.other), MOB_ISNPC);

  /* two dark indoor rooms: the jump lands */
  spell_shadow_jump(10, &fixture.ch, &fixture.other, NULL, CAST_INNATE);
  CuAssertIntEquals(tc, 1, IN_ROOM(&fixture.ch));

  zone_table = saved_zone_table;
  top_of_zone_table = saved_top_of_zone_table;
  end_innate_fixture(&fixture);
}

/* The racial summons are summon mobs with their own follower categories: one
 * warg at a time, and a horde of two to four orcs admitted as a whole batch
 * against a limit of four, independent of the general and summon slots. */
void TestRacialSummonFollowerLimits(CuTest *tc)
{
  struct innate_fixture fixture;
  struct char_data prototypes[2], orcs[4], warg;
  struct index_data indexes[2];
  struct follow_type links[5];
  struct char_data *saved_proto = mob_proto;
  struct index_data *saved_index = mob_index;
  mob_rnum saved_top = top_of_mobt;
  int i;

  begin_innate_fixture(&fixture);
  GET_CHA(&fixture.ch) = 10;

  for (i = 0; i < 2; i++)
  {
    clear_char(&prototypes[i]);
    SET_BIT_AR(MOB_FLAGS(&prototypes[i]), MOB_ISNPC);
    GET_MOB_RNUM(&prototypes[i]) = i;
  }
  indexes[0].vnum = PET_RACIAL_WARG; /* sorted: 19502 before 19503 */
  indexes[1].vnum = PET_RACIAL_ORC_WARRIOR;
  mob_proto = prototypes;
  mob_index = indexes;
  top_of_mobt = 1;

  memset(links, 0, sizeof(links));
  for (i = 0; i < 4; i++)
  {
    clear_char(&orcs[i]);
    SET_BIT_AR(MOB_FLAGS(&orcs[i]), MOB_ISNPC);
    SET_BIT_AR(AFF_FLAGS(&orcs[i]), AFF_CHARM);
    GET_MOB_RNUM(&orcs[i]) = 1;
    orcs[i].master = &fixture.ch;
    links[i].follower = &orcs[i];
  }
  clear_char(&warg);
  SET_BIT_AR(MOB_FLAGS(&warg), MOB_ISNPC);
  SET_BIT_AR(AFF_FLAGS(&warg), AFF_CHARM);
  GET_MOB_RNUM(&warg) = 0;
  warg.master = &fixture.ch;
  links[4].follower = &warg;

  CuAssertTrue(tc, isSummonMob(PET_RACIAL_WARG));
  CuAssertTrue(tc, isSummonMob(PET_RACIAL_ORC_WARRIOR));

  /* nothing following: any legal horde, never five, never none */
  for (i = 2; i <= 4; i++)
    CuAssertTrue(tc, can_add_summoned_followers(&fixture.ch, PET_RACIAL_ORC_WARRIOR,
                                                ABILITY_SUMMON_HORDE, i));
  CuAssertTrue(tc, !can_add_summoned_followers(&fixture.ch, PET_RACIAL_ORC_WARRIOR,
                                               ABILITY_SUMMON_HORDE, 5));
  CuAssertTrue(tc, !can_add_summoned_followers(&fixture.ch, PET_RACIAL_ORC_WARRIOR,
                                               ABILITY_SUMMON_HORDE, 0));
  CuAssertTrue(tc,
               can_add_summoned_followers(&fixture.ch, PET_RACIAL_WARG, ABILITY_SUMMON_WARG, 1));

  /* one orc already here: three more fit, four do not */
  fixture.ch.followers = &links[0];
  CuAssertTrue(
      tc, can_add_summoned_followers(&fixture.ch, PET_RACIAL_ORC_WARRIOR, ABILITY_SUMMON_HORDE, 3));
  CuAssertTrue(tc, !can_add_summoned_followers(&fixture.ch, PET_RACIAL_ORC_WARRIOR,
                                               ABILITY_SUMMON_HORDE, 4));

  /* a full horde blocks another horde but not the warg */
  for (i = 0; i < 3; i++)
    links[i].next = &links[i + 1];
  CuAssertTrue(tc, !can_add_summoned_followers(&fixture.ch, PET_RACIAL_ORC_WARRIOR,
                                               ABILITY_SUMMON_HORDE, 2));
  CuAssertTrue(tc,
               can_add_summoned_followers(&fixture.ch, PET_RACIAL_WARG, ABILITY_SUMMON_WARG, 1));

  /* a warg already following refuses a second one, and leaves the horde alone */
  fixture.ch.followers = &links[4];
  CuAssertTrue(tc,
               !can_add_summoned_followers(&fixture.ch, PET_RACIAL_WARG, ABILITY_SUMMON_WARG, 1));
  CuAssertTrue(
      tc, can_add_summoned_followers(&fixture.ch, PET_RACIAL_ORC_WARRIOR, ABILITY_SUMMON_HORDE, 4));

  fixture.ch.followers = NULL;
  mob_proto = saved_proto;
  mob_index = saved_index;
  top_of_mobt = saved_top;
  end_innate_fixture(&fixture);
}

/* ---- racial casting speed and spell power (issue 165) ---- */

/* Runs one real timed cast of a healing spell through cast_spell() and the game scheduler.
 * Returns the casting time the cast started with (0 when it completed at once) and only
 * returns after the target has been healed, which proves the cast resolved. */
static int racial_timed_cast(CuTest *tc, struct innate_fixture *fixture, int fast, int slow)
{
  struct char_data *caster = &fixture->ch;
  struct char_data *target = &fixture->other;
  int started_with = 0;
  unsigned int tick = 0;

  SET_FEAT(caster, FEAT_FAST_CASTING, fast);
  SET_FEAT(caster, FEAT_SLOW_CASTING, slow);
  GET_HIT(target) = 10;
  CuAssertIntEquals(tc, 1, cast_spell(caster, target, NULL, SPELL_CURE_LIGHT, METAMAGIC_NONE));
  started_with = CASTING_TIME(caster);
  if (started_with > 0)
    CuAssertTrue(tc, IS_CASTING(caster));
  else
    CuAssertTrue(tc, !IS_CASTING(caster));
  for (tick = 0; tick < 10U * PASSES_PER_SEC && IS_CASTING(caster); tick++)
  {
    pulse++;
    event_test_advance();
  }
  CuAssertTrue(tc, !IS_CASTING(caster));
  CuAssertTrue(tc, GET_HIT(target) > 10);
  return started_with;
}

static void begin_racial_cast_fixture(CuTest *tc, struct innate_fixture *fixture,
                                      struct spell_info_type *saved_spell, int *saved_mode,
                                      unsigned long *saved_pulse)
{
  begin_innate_fixture(fixture);
  *saved_spell = spell_info[SPELL_CURE_LIGHT];
  *saved_mode = CONFIG_SPELLCASTING_TIME_MODE;
  *saved_pulse = pulse;
  memset(&spell_info[SPELL_CURE_LIGHT], 0, sizeof(spell_info[SPELL_CURE_LIGHT]));
  spell_info[SPELL_CURE_LIGHT].name = "cure light";
  spell_info[SPELL_CURE_LIGHT].min_position = POS_FIGHTING;
  spell_info[SPELL_CURE_LIGHT].targets = TAR_CHAR_ROOM;
  spell_info[SPELL_CURE_LIGHT].routines = MAG_POINTS;
  spell_info[SPELL_CURE_LIGHT].time = 2;
  /* the runtime's periodic services recompute a player's max hit from class levels while the
   * cast runs, so the heal target is a mobile with real points */
  SET_BIT_AR(MOB_FLAGS(&fixture->other), MOB_ISNPC);
  fixture->other.player.short_descr = (char *)"innate two";
  GET_REAL_MAX_HIT(&fixture->ch) = 100;
  GET_REAL_MAX_HIT(&fixture->other) = 100;
  /* a mortal casting an at-will racial cantrip: no spell preparation, real timed cast */
  SET_FEAT(&fixture->ch, FEAT_HIGH_ELF_CANTRIP, 1);
  HIGH_ELF_CANTRIP((&fixture->ch)) = SPELL_CURE_LIGHT;
  fixture->rooms[0].light = 1;
  domain_event_runtime_shutdown();
  event_free_all();
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER));
  event_init();
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_init());
}

static void end_racial_cast_fixture(struct innate_fixture *fixture,
                                    const struct spell_info_type *saved_spell, int saved_mode,
                                    unsigned long saved_pulse)
{
  domain_event_runtime_shutdown();
  if (fixture->ch.events != NULL)
  {
    free_list(fixture->ch.events);
    fixture->ch.events = NULL;
  }
  spell_info[SPELL_CURE_LIGHT] = *saved_spell;
  CONFIG_SPELLCASTING_TIME_MODE = saved_mode;
  pulse = saved_pulse;
  end_innate_fixture(fixture);
}

void Test_racial_casting_feats_shift_a_timed_cast_by_one_tick_per_rank(CuTest *tc)
{
  struct innate_fixture fixture;
  struct spell_info_type saved_spell;
  int saved_mode;
  unsigned long saved_pulse;

  begin_racial_cast_fixture(tc, &fixture, &saved_spell, &saved_mode, &saved_pulse);
  CONFIG_SPELLCASTING_TIME_MODE = 1;

  CuAssertIntEquals(tc, 2, racial_timed_cast(tc, &fixture, 0, 0));
  CuAssertIntEquals(tc, 3, racial_timed_cast(tc, &fixture, 0, 1));
  CuAssertIntEquals(tc, 4, racial_timed_cast(tc, &fixture, 0, 2));
  CuAssertIntEquals(tc, 1, racial_timed_cast(tc, &fixture, 1, 0));
  /* two ranks take a two-tick spell to zero: it completes inside cast_spell() */
  CuAssertIntEquals(tc, 0, racial_timed_cast(tc, &fixture, 2, 0));
  CuAssertIntEquals(tc, 0, racial_timed_cast(tc, &fixture, 3, 0));
  /* the two feats cancel rank for rank */
  CuAssertIntEquals(tc, 2, racial_timed_cast(tc, &fixture, 1, 1));

  end_racial_cast_fixture(&fixture, &saved_spell, saved_mode, saved_pulse);
}

void Test_racial_casting_feats_apply_in_standard_action_mode(CuTest *tc)
{
  struct innate_fixture fixture;
  struct spell_info_type saved_spell;
  int saved_mode;
  unsigned long saved_pulse;

  begin_racial_cast_fixture(tc, &fixture, &saved_spell, &saved_mode, &saved_pulse);
  CONFIG_SPELLCASTING_TIME_MODE = 0;

  /* standard-action mode casts every non-ritual spell in one tick regardless of SINFO.time */
  CuAssertIntEquals(tc, 1, racial_timed_cast(tc, &fixture, 0, 0));
  CuAssertIntEquals(tc, 2, racial_timed_cast(tc, &fixture, 0, 1));
  CuAssertIntEquals(tc, 0, racial_timed_cast(tc, &fixture, 1, 0));

  end_racial_cast_fixture(&fixture, &saved_spell, saved_mode, saved_pulse);
}

/* Spell power decision: no racial spell-power feat.  FEAT_ENHANCED_SPELL_DAMAGE is granted by
 * the race level-feat path without any class prerequisite, stacks per grant, and mag_damage()
 * reads it through HAS_FEAT() for every spell-number damage roll. */
void Test_enhanced_spell_damage_is_race_assignable_and_stacks(CuTest *tc)
{
  struct innate_fixture fixture;
  struct race_feat_assign grants[2];
  struct race_feat_assign *saved_head = NULL;
  int race = 0;

  begin_innate_fixture(&fixture);
  race = GET_RACE(&fixture.ch);
  saved_head = race_list[race].featassign_list;
  memset(grants, 0, sizeof(grants));
  grants[0].feat_num = FEAT_ENHANCED_SPELL_DAMAGE;
  grants[0].level_received = 1;
  grants[0].next = &grants[1];
  grants[1].feat_num = FEAT_ENHANCED_SPELL_DAMAGE;
  grants[1].level_received = 3;
  grants[1].stacks = TRUE;
  grants[1].next = saved_head;
  race_list[race].featassign_list = grants;

  /* no class, no caster level: the race grant does not consult the class prerequisites */
  CuAssertIntEquals(tc, 0, HAS_FEAT(&fixture.ch, FEAT_ENHANCED_SPELL_DAMAGE));
  GET_LEVEL(&fixture.ch) = 1;
  process_race_level_feats(&fixture.ch);
  CuAssertIntEquals(tc, 1, HAS_FEAT(&fixture.ch, FEAT_ENHANCED_SPELL_DAMAGE));
  GET_LEVEL(&fixture.ch) = 2;
  process_race_level_feats(&fixture.ch);
  CuAssertIntEquals(tc, 1, HAS_FEAT(&fixture.ch, FEAT_ENHANCED_SPELL_DAMAGE));
  GET_LEVEL(&fixture.ch) = 3;
  process_race_level_feats(&fixture.ch);
  CuAssertIntEquals(tc, 2, HAS_FEAT(&fixture.ch, FEAT_ENHANCED_SPELL_DAMAGE));

  race_list[race].featassign_list = saved_head;
  end_innate_fixture(&fixture);
}

/* make the second fixture character a monster with a lot of hit points so a
 * player may fight it and never kill it */
static void make_innate_other_a_monster(struct innate_fixture *fixture)
{
  fixture->saved_mob_index = mob_index;
  fixture->saved_top_of_mobt = top_of_mobt;
  fixture->mobile_index[0].vnum = 1;
  mob_index = fixture->mobile_index;
  top_of_mobt = 0;
  fixture->mob_index_swapped = TRUE;

  SET_BIT_AR(MOB_FLAGS(&fixture->other), MOB_ISNPC);
  fixture->other.player.short_descr = (char *)"innate two";
  GET_HIT(&fixture->other) = 5000;
  GET_MAX_HIT(&fixture->other) = 5000;
  GET_REAL_MAX_HIT(&fixture->other) = 5000;
}

/* affect_total() recomputes a player's maximum hit points from level and
 * Constitution, so settle it once and return the settled maximum */
static int settle_innate_player_hit_points(struct char_data *ch)
{
  GET_REAL_CON(ch) = 12;
  affect_total(ch);
  GET_HIT(ch) = GET_MAX_HIT(ch);
  return GET_MAX_HIT(ch);
}

/* throw away everything written to the fixture descriptor so far */
static void reset_innate_output(struct descriptor_data *descriptor)
{
  descriptor->output = descriptor->small_outbuf;
  descriptor->bufptr = 0;
  descriptor->bufspace = SMALL_BUFSIZE - 1;
  descriptor->small_outbuf[0] = '\0';
}

/* Bull charge: 'charge <direction> <target>' crosses the exit and charges the
 * target there; without the feat a direction is just an unknown target, the
 * exit and the target must both exist, an unseen namesake ahead of the target
 * does not hide it, and a peaceful destination refuses the charge. */
void TestBullChargeReachesAnAdjacentRoom(CuTest *tc)
{
  struct innate_fixture fixture;
  struct char_data decoy;
  struct player_special_data decoy_specials;
  struct descriptor_data decoy_descriptor;

  begin_innate_fixture(&fixture);
  innate_fixture_open_north(&fixture);
  fixture.ch.player.title = (char *)"";
  make_innate_other_a_monster(&fixture);
  CuAssertTrue(tc, settle_innate_player_hit_points(&fixture.ch) > 0);
  GET_MOVE(&fixture.ch) = 100;
  GET_MAX_MOVE(&fixture.ch) = 100;

  /* an invisible namesake stands ahead of the target in the destination */
  memset(&decoy_specials, 0, sizeof(decoy_specials));
  setup_innate_char(&decoy, &decoy_specials, &decoy_descriptor, "innate two");
  SET_BIT_AR(AFF_FLAGS(&decoy), AFF_INVISIBLE);
  IN_ROOM(&decoy) = 1;
  decoy.next_in_room = &fixture.other;
  fixture.ch.next_in_room = NULL;
  IN_ROOM(&fixture.other) = 1;
  fixture.rooms[1].people = &decoy;

  /* without the feat a direction is just an unknown target */
  do_charge(&fixture.ch, "north two", 0, 0);
  CuAssertIntEquals(tc, 0, IN_ROOM(&fixture.ch));
  CuAssertTrue(tc, FIGHTING(&fixture.ch) == NULL);

  SET_FEAT(&fixture.ch, FEAT_BULL_CHARGE, 1);

  /* no exit that way */
  do_charge(&fixture.ch, "south two", 0, 0);
  CuAssertIntEquals(tc, 0, IN_ROOM(&fixture.ch));

  /* nobody by that name over there */
  do_charge(&fixture.ch, "north nobody", 0, 0);
  CuAssertIntEquals(tc, 0, IN_ROOM(&fixture.ch));
  CuAssertTrue(tc, FIGHTING(&fixture.ch) == NULL);

  /* a peaceful destination refuses the charge before anyone moves */
  SET_BIT_AR(ROOM_FLAGS(1), ROOM_PEACEFUL);
  reset_innate_output(&fixture.ch_descriptor);
  do_charge(&fixture.ch, "north two", 0, 0);
  CuAssertIntEquals(tc, 0, IN_ROOM(&fixture.ch));
  CuAssertTrue(tc, FIGHTING(&fixture.ch) == NULL);
  CuAssertTrue(tc, FIGHTING(&fixture.other) == NULL);
  CuAssertPtrNotNull(tc, strstr(fixture.ch_descriptor.output, "cannot charge in"));
  REMOVE_BIT_AR(ROOM_FLAGS(1), ROOM_PEACEFUL);

  /* the charge crosses the exit and lands on the visible namesake */
  do_charge(&fixture.ch, "north two", 0, 0);
  CuAssertIntEquals(tc, 1, IN_ROOM(&fixture.ch));
  CuAssertTrue(tc, FIGHTING(&fixture.ch) == &fixture.other);
  CuAssertTrue(tc, FIGHTING(&fixture.other) == &fixture.ch);
  CuAssertTrue(tc, FIGHTING(&decoy) == NULL);

  stop_fighting(&fixture.ch);
  stop_fighting(&fixture.other);
  end_innate_char(&decoy, &decoy_descriptor);
  end_innate_fixture(&fixture);
}

/* In the wilderness an exit points at the sentinel room, so the charge
 * resolves the adjacent tile by coordinates and never searches the sentinel. */
void TestBullChargeResolvesWildernessTilesByCoordinates(CuTest *tc)
{
  struct innate_fixture fixture;
  struct kdtree *saved_kd_wilderness_rooms = kd_wilderness_rooms;
  struct room_direction_data south, east;

  begin_innate_fixture(&fixture);
  innate_fixture_open_north(&fixture);
  SET_BIT_AR(fixture.zone.zone_flags, ZONE_WILDERNESS);
  /* every wilderness exit points at the sentinel */
  south = fixture.north;
  east = fixture.north;
  fixture.rooms[0].dir_option[SOUTH] = &south;
  fixture.rooms[0].dir_option[EAST] = &east;
  fixture.rooms[1].number = WILD_ROOM_VNUM_START; /* the sentinel every exit points at */
  fixture.rooms[2].number = WILD_ROOM_VNUM_START + 1;
  fixture.rooms[2].coords[0] = 0;
  fixture.rooms[2].coords[1] = 1; /* one tile north of the origin at (0, 0) */
  fixture.rooms[3].number = WILD_ROOM_VNUM_START + 2;
  fixture.rooms[3].coords[0] = 0;
  fixture.rooms[3].coords[1] = -1;
  top_of_world = 3;
  kd_wilderness_rooms = NULL;
  initialize_wilderness_lists();
  SET_FEAT(&fixture.ch, FEAT_BULL_CHARGE, 1);

  CuAssertIntEquals(tc, 2, bull_charge_destination(&fixture.ch, NORTH));
  CuAssertIntEquals(tc, 3, bull_charge_destination(&fixture.ch, SOUTH));
  CuAssertIntEquals(tc, NOWHERE, bull_charge_destination(&fixture.ch, EAST));

  /* a body in the sentinel is not to the north */
  fixture.ch.next_in_room = NULL;
  IN_ROOM(&fixture.other) = 1;
  fixture.rooms[1].people = &fixture.other;
  reset_innate_output(&fixture.ch_descriptor);
  do_charge(&fixture.ch, "north two", 0, 0);
  CuAssertIntEquals(tc, 0, IN_ROOM(&fixture.ch));
  CuAssertTrue(tc, FIGHTING(&fixture.ch) == NULL);
  CuAssertPtrNotNull(tc, strstr(fixture.ch_descriptor.output, "nobody like that"));

  kd_free(kd_wilderness_rooms);
  kd_wilderness_rooms = saved_kd_wilderness_rooms;
  end_innate_fixture(&fixture);
}

/* seed the generator so the next d20 is neither a natural 1 nor a natural 20 */
static unsigned long innate_seed_for_ordinary_d20(void)
{
  unsigned long seed;
  int roll;

  for (seed = 1;; seed++)
  {
    circle_srandom(seed);
    roll = rand_number(1, 20);
    if (roll > 1 && roll < 20)
    {
      circle_srandom(seed);
      return seed;
    }
  }
}

/* The charge stun is a one-round stun event gated by a Fortitude save, and a
 * target that cannot be stunned is never stunned. */
void TestBullChargeStunFollowsTheFortitudeSave(CuTest *tc)
{
  struct innate_fixture fixture;
  unsigned long seed;

  begin_innate_fixture(&fixture);
  fixture.ch.player.title = (char *)"";
  fixture.other.player.title = (char *)"";
  seed = innate_seed_for_ordinary_d20();

  /* an unshakable target */
  SET_FEAT(&fixture.other, FEAT_TOUGH_AS_BONE, 1);
  CuAssertTrue(tc, !bull_charge_stun(&fixture.ch, &fixture.other));
  CuAssertTrue(tc, char_has_mud_event(&fixture.other, eSTUNNED) == NULL);
  SET_FEAT(&fixture.other, FEAT_TOUGH_AS_BONE, 0);

  /* a passed save keeps the target on its feet */
  GET_SAVE(&fixture.other, SAVING_FORT) = 1000;
  circle_srandom(seed);
  CuAssertTrue(tc, !bull_charge_stun(&fixture.ch, &fixture.other));
  CuAssertTrue(tc, char_has_mud_event(&fixture.other, eSTUNNED) == NULL);

  /* a failed save stuns for one round */
  GET_SAVE(&fixture.other, SAVING_FORT) = -1000;
  circle_srandom(seed);
  CuAssertTrue(tc, bull_charge_stun(&fixture.ch, &fixture.other));
  CuAssertPtrNotNull(tc, char_has_mud_event(&fixture.other, eSTUNNED));

  circle_srandom((unsigned long)time(NULL));
  end_innate_fixture(&fixture);
}

/* Bloodlust takes hold when a combat round finds the character below half hit
 * points and lets go once they are back at half or more; without the feat the
 * round check does nothing. */
void TestBloodlustEngagesAndReleasesAtHalfHitPoints(CuTest *tc)
{
  struct innate_fixture fixture;
  int max_hit = 0, below_half = 0, at_least_half = 0;

  begin_innate_fixture(&fixture);
  fixture.ch.player.title = (char *)"";
  make_innate_other_a_monster(&fixture);
  FIGHTING(&fixture.ch) = &fixture.other;
  max_hit = settle_innate_player_hit_points(&fixture.ch);
  CuAssertTrue(tc, max_hit >= 4);
  below_half = (max_hit - 1) / 2;
  at_least_half = (max_hit + 1) / 2;

  GET_HIT(&fixture.ch) = below_half;
  bloodlust_round_check(&fixture.ch);
  CuAssertTrue(tc, !affected_by_spell(&fixture.ch, SKILL_BLOODLUST));

  SET_FEAT(&fixture.ch, FEAT_BLOODLUST, 1);
  bloodlust_round_check(&fixture.ch);
  CuAssertTrue(tc, affected_by_spell(&fixture.ch, SKILL_BLOODLUST));

  /* half is not below half */
  GET_HIT(&fixture.ch) = at_least_half;
  bloodlust_round_check(&fixture.ch);
  CuAssertTrue(tc, !affected_by_spell(&fixture.ch, SKILL_BLOODLUST));

  /* the combat round itself runs the check */
  GET_HIT(&fixture.ch) = below_half;
  perform_violence(&fixture.ch, 1);
  CuAssertTrue(tc, affected_by_spell(&fixture.ch, SKILL_BLOODLUST));

  GET_HIT(&fixture.ch) = max_hit;
  perform_violence(&fixture.ch, 1);
  CuAssertTrue(tc, !affected_by_spell(&fixture.ch, SKILL_BLOODLUST));

  stop_fighting(&fixture.ch);
  stop_fighting(&fixture.other);
  end_innate_fixture(&fixture);
}

/* While the bloodlust holds, casting is refused at the spell engine and fleeing
 * is refused at the flee engine. */
void TestBloodlustRefusesCastingAndFleeing(CuTest *tc)
{
  struct innate_fixture fixture;

  begin_innate_fixture(&fixture);
  fixture.ch.player.title = (char *)"";
  make_innate_other_a_monster(&fixture);
  FIGHTING(&fixture.ch) = &fixture.other;
  SET_FEAT(&fixture.ch, FEAT_BLOODLUST, 1);
  GET_HIT(&fixture.ch) = (settle_innate_player_hit_points(&fixture.ch) - 1) / 2;
  bloodlust_round_check(&fixture.ch);
  CuAssertTrue(tc, affected_by_spell(&fixture.ch, SKILL_BLOODLUST));

  reset_innate_output(&fixture.ch_descriptor);
  CuAssertIntEquals(tc, 0, cast_spell(&fixture.ch, &fixture.other, NULL, SPELL_MAGIC_MISSILE, 0));
  CuAssertTrue(tc, !IS_CASTING(&fixture.ch));
  CuAssertPtrNotNull(tc, strstr(fixture.ch_descriptor.output, "no room for the focus to cast"));

  reset_innate_output(&fixture.ch_descriptor);
  perform_flee(&fixture.ch);
  CuAssertIntEquals(tc, 0, IN_ROOM(&fixture.ch));
  CuAssertTrue(tc, FIGHTING(&fixture.ch) == &fixture.other);
  CuAssertPtrNotNull(tc, strstr(fixture.ch_descriptor.output, "will not let you leave"));

  /* a directed flee through an open exit is refused the same way */
  innate_fixture_open_north(&fixture);
  SET_FEAT(&fixture.ch, FEAT_SPRING_ATTACK, 1);
  GET_MOVE(&fixture.ch) = 100;
  GET_MAX_MOVE(&fixture.ch) = 100;
  reset_innate_output(&fixture.ch_descriptor);
  do_flee(&fixture.ch, "north", 0, 0);
  CuAssertIntEquals(tc, 0, IN_ROOM(&fixture.ch));
  CuAssertTrue(tc, FIGHTING(&fixture.ch) == &fixture.other);
  CuAssertPtrNotNull(tc, strstr(fixture.ch_descriptor.output, "will not let you leave"));

  /* disengaging from a foe that is not fighting back is refused too */
  FIGHTING(&fixture.other) = NULL;
  reset_innate_output(&fixture.ch_descriptor);
  do_disengage(&fixture.ch, "", 0, 0);
  CuAssertTrue(tc, FIGHTING(&fixture.ch) == &fixture.other);
  CuAssertPtrNotNull(tc, strstr(fixture.ch_descriptor.output, "will not let you leave"));

  /* and all three open up once the bloodlust lets go */
  GET_HIT(&fixture.ch) = GET_MAX_HIT(&fixture.ch);
  bloodlust_round_check(&fixture.ch);
  CuAssertTrue(tc, !affected_by_spell(&fixture.ch, SKILL_BLOODLUST));
  do_disengage(&fixture.ch, "", 0, 0);
  CuAssertTrue(tc, FIGHTING(&fixture.ch) == NULL);

  end_innate_fixture(&fixture);
}
