#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/db.h"
#include "../../src/handler.h"
#include "../../src/modify.h"
#include "../../src/net/protocol.h"
#include "../../src/magic/domains_schools.h"
#include "../../src/magic/spells.h"

#include <string.h>

void Test_spells_production_classification_helpers(CuTest *tc)
{
  CuAssertTrue(tc, is_wall_spell(SPELL_WALL_OF_FIRE));
  CuAssertTrue(tc, is_wall_spell(SPELL_WALL_OF_FORCE));
  CuAssertTrue(tc, !is_wall_spell(SPELL_MAGIC_MISSILE));
  CuAssertTrue(tc, isEpicSpell(SPELL_HELLBALL));
  CuAssertTrue(tc, !isEpicSpell(SPELL_CURE_LIGHT));
}

void Test_spells_production_name_and_level_lookup(CuTest *tc)
{
  const char *saved_name;
  int saved_levels[NUM_CLASSES];
  int i;

  saved_name = spell_info[SPELL_MAGIC_MISSILE].name;
  for (i = 0; i < NUM_CLASSES; i++)
  {
    saved_levels[i] = spell_info[SPELL_MAGIC_MISSILE].min_level[i];
    spell_info[SPELL_MAGIC_MISSILE].min_level[i] = 20;
  }
  spell_info[SPELL_MAGIC_MISSILE].name = "Coverage Missile";
  spell_info[SPELL_MAGIC_MISSILE].min_level[CLASS_WIZARD] = 3;

  CuAssertStrEquals(tc, "Coverage Missile", spell_name(SPELL_MAGIC_MISSILE));
  CuAssertIntEquals(tc, 3, lowest_spell_level(SPELL_MAGIC_MISSILE));
  CuAssertIntEquals(tc, SPELL_MAGIC_MISSILE, find_skill_num("coverage missile"));
  CuAssertIntEquals(tc, SPELL_MAGIC_MISSILE, find_skill_num("cov mis"));
  CuAssertIntEquals(tc, -1, find_skill_num("not a real coverage spell"));

  spell_info[SPELL_MAGIC_MISSILE].name = saved_name;
  for (i = 0; i < NUM_CLASSES; i++)
    spell_info[SPELL_MAGIC_MISSILE].min_level[i] = saved_levels[i];
}

void Test_cure_critical_spell_names_are_canonical(CuTest *tc)
{
  if (spell_info[SPELL_ARMOR].name == NULL || spell_info[SPELL_ARMOR].name == unused_spellname)
    mag_assign_spells();

  CuAssertStrEquals(tc, "cure critical", spell_name(SPELL_CURE_CRITIC));
  CuAssertStrEquals(tc, "mass cure critical", spell_name(SPELL_MASS_CURE_CRIT));
  CuAssertIntEquals(tc, SPELL_CURE_CRITIC, find_skill_num("cure critic"));
  CuAssertIntEquals(tc, SPELL_MASS_CURE_CRIT, find_skill_num("mass cure critic"));
}

void Test_spells_production_cantrip_bounds(CuTest *tc)
{
  bool saved_cantrip;

  saved_cantrip = spell_info[SPELL_DETECT_MAGIC].is_cantrip;
  spell_info[SPELL_DETECT_MAGIC].is_cantrip = true;

  CuAssertTrue(tc, spell_is_cantrip(SPELL_DETECT_MAGIC));
  CuAssertTrue(tc, !spell_is_cantrip(SPELL_RESERVED_DBC));
  CuAssertTrue(tc, !spell_is_cantrip(NUM_SPELLS));

  spell_info[SPELL_DETECT_MAGIC].is_cantrip = saved_cantrip;
}

void Test_sharpened_edge_duration_is_ten_minutes_per_level(CuTest *tc)
{
  struct char_data ch;
  struct affected_type *affect;
  int duration_seconds;

  clear_char(&ch);
  SET_BIT_AR(MOB_FLAGS(&ch), MOB_ISNPC);
  ch.player_specials = &dummy_mob;
  ch.player.short_descr = "sharpened edge test character";
  GET_LEVEL(&ch) = 5;

  mag_affects(GET_LEVEL(&ch), &ch, &ch, NULL, PSIONIC_SHARPENED_EDGE, SAVING_WILL, CAST_INNATE, 0);

  affect = ch.affected;
  CuAssertPtrNotNull(tc, affect);
  if (affect != NULL)
  {
    duration_seconds = affect->duration * PULSE_VIOLENCE / PASSES_PER_SEC;
    CuAssertIntEquals(tc, GET_LEVEL(&ch) * 10 * SECS_PER_REAL_MIN, duration_seconds);
  }

  while (ch.affected != NULL)
    affect_remove_no_total(&ch, ch.affected);
}

void Test_group_heal_restores_health_and_cures_blindness(CuTest *tc)
{
  struct char_data ch;
  struct room_data room;
  struct room_data *saved_world;
  room_rnum saved_top_of_world;
  int starting_hit_points;

  clear_char(&ch);
  memset(&room, 0, sizeof(room));
  saved_world = world;
  saved_top_of_world = top_of_world;

  world = &room;
  top_of_world = 0;
  room.people = &ch;
  SET_BIT_AR(MOB_FLAGS(&ch), MOB_ISNPC);
  ch.player_specials = &dummy_mob;
  ch.player.short_descr = "group heal test character";
  IN_ROOM(&ch) = 0;
  GET_LEVEL(&ch) = 20;
  GET_REAL_MAX_HIT(&ch) = 100;
  GET_MAX_HIT(&ch) = 100;
  GET_HIT(&ch) = 10;
  SET_BIT_AR(AFF_FLAGS(&ch), AFF_BLIND);
  starting_hit_points = GET_HIT(&ch);

  mag_groups(GET_LEVEL(&ch), &ch, NULL, SPELL_GROUP_HEAL, SAVING_WILL, CAST_SPELL);

  world = saved_world;
  top_of_world = saved_top_of_world;

  CuAssertTrue(tc, GET_HIT(&ch) > starting_hit_points);
  CuAssertTrue(tc, !AFF_FLAGGED(&ch, AFF_BLIND));
}

void Test_high_circle_swarm_summons_scale_with_caster_level(CuTest *tc)
{
  CuAssertIntEquals(tc, 17, summon_spell_mob_level(SPELL_ELEMENTAL_SWARM, 17));
  CuAssertIntEquals(tc, 20, summon_spell_mob_level(SPELL_ELEMENTAL_SWARM, 30));
  CuAssertIntEquals(tc, 17, summon_spell_mob_level(SPELL_SHAMBLER, 17));
  CuAssertIntEquals(tc, 20, summon_spell_mob_level(SPELL_SHAMBLER, 30));
  CuAssertIntEquals(tc, 0, summon_spell_mob_level(SPELL_HEAL, 30));
}

void Test_domain_command_labels_granted_spell_circles(CuTest *tc)
{
  struct char_data ch;
  struct descriptor_data descriptor;
  struct player_special_data player_specials;
  char expected[128];
  bool found_circle;

  if (spell_info[SPELL_ARMOR].name == NULL || spell_info[SPELL_ARMOR].name == unused_spellname)
    mag_assign_spells();
  assign_domains();

  clear_char(&ch);
  memset(&descriptor, 0, sizeof(descriptor));
  memset(&player_specials, 0, sizeof(player_specials));
  ch.player_specials = &player_specials;
  ch.player.name = "domain command test character";
  GET_PAGE_LENGTH(&ch) = PAGE_LENGTH;
  ch.desc = &descriptor;
  descriptor.character = &ch;
  descriptor.output = descriptor.small_outbuf;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  descriptor.pProtocol = ProtocolCreate();

  if (descriptor.pProtocol == NULL)
  {
    ch.desc = NULL;
    CuFail(tc, "could not initialize protocol output for the domain command test");
    return;
  }

  do_domain(&ch, "", 0, 0);
  snprintf(expected, sizeof(expected), "1: %s|",
           spell_info[domain_list[DOMAIN_AIR].domain_spells[0]].name);
  found_circle = descriptor.showstr_head != NULL && strstr(descriptor.showstr_head, expected);

  show_string(&descriptor, "q");
  ch.desc = NULL;
  ProtocolDestroy(descriptor.pProtocol);

  CuAssertTrue(tc, found_circle);
}

void Test_internal_affects_have_registered_wearoff_messages(CuTest *tc)
{
  static const int internal_affects[] = {
      RACIAL_LICH_TOUCH,
      AFFECT_BARD_FLOURISH,
      AFFECT_BARD_AGILE_DISENGAGE,
      AFFECT_BARD_PERFECT_TEMPO,
      AFFECT_BARD_SHOWSTOPPER,
      AFFECT_BARD_FEINT_AND_FINISH,
      AFFECT_BARD_SUPREME_STYLE,
      AFFECT_BARD_CURTAIN_CALL,
      AFFECT_BARD_CURTAIN_CALL_DISORIENTED,
      AFFECT_BLACKGUARD_SHAKEN,
      AFFECT_BLACKGUARD_FEAR,
      AFFECT_BLACKGUARD_COWER,
      AFFECT_BLACKGUARD_CRUEL_MOMENTUM,
      AFFECT_BLACKGUARD_PROFANE_WEAPON_BOND,
      AFFECT_BLACKGUARD_BLEEDING,
      AFFECT_BLACKGUARD_UNHOLY_BLITZ,
      AFFECT_BLACKGUARD_AVATAR_OF_PROFANITY,
      AFFECT_BLACKGUARD_CATACLYSMIC_SMITE,
      AFFECT_BLACKGUARD_SHADE_STEP,
      AFFECT_BLACKGUARD_REPRISAL,
      AFFECT_DIVINE_RESILIENCE,
      AFFECT_INQUISITOR_AMBUSH_USED,
      AFFECT_INQUISITOR_DEADLY_AIM,
      AFFECT_BERSERKER_INDOMITABLE_WILL,
      AFFECT_BARD_HEIGHTENED_HARMONY,
      AFFECT_BARD_SYMPHONIC_RESONANCE,
      AFFECT_ALCHEMIST_DISCOVERY_EXTRACTION,
      AFFECT_ALCHEMIST_QUINTESSENTIAL_EXTRACTION,
      AFFECT_BERSERKER_CRIPPLING_BLOW,
      AFFECT_BERSERKER_STUNNING_BLOW,
      AFFECT_BARD_FROSTBITE_REFRAIN_I,
      AFFECT_BARD_FROSTBITE_REFRAIN_II,
      AFFECT_BARD_COMMANDING_CADENCE,
      AFFECT_BARD_COMMANDING_CADENCE_IMMUNITY,
      AFFECT_BARD_WINTERS_WAR_MARCH,
      AFFECT_BARD_WINTERS_WAR_MARCH_IMMUNITY,
      AFFECT_INQUISITOR_PERFECT_ADAPTATION,
      AFFECT_INQUISITOR_SUPREMACY,
      AFFECT_CLERIC_BEACON_OF_HOPE,
      AFFECT_WIZARD_IRRESISTIBLE_MAGIC,
      AFFECT_CLERIC_AVATAR_OF_WAR,
      AFFECT_MONK_AVATAR_OF_ELEMENTS,
      AFFECT_RANGER_NATURES_WRATH,
      AFFECT_PSIONICIST_FOCUS_CHANNELING,
      AFFECT_PSIONICIST_OVERWHELM,
      AFFECT_PSIONICIST_LINKED_MENACE,
      AFFECT_PSIONICIST_PSYCHIC_SUNDERING,
      AFFECT_INTIMIDATING_PRESENCE,
      SKILL_BLEEDING_ATTACK,
      SKILL_CRIPPLING_STRIKE,
      SKILL_PRESSURE_POINT_STRIKE,
      SKILL_FLAMES_OF_PHOENIX,
      SKILL_ETERNAL_MOUNTAIN_DEFENSE,
      SKILL_BREATH_OF_WINTER,
      SKILL_HARDY,
      SKILL_WATER_WHIP,
      SKILL_GONG_OF_SUMMIT,
      SKILL_FIST_OF_UNBROKEN_AIR,
      SKILL_SWEEPING_CINDER_STRIKE,
      SKILL_RUSH_OF_GALE_SPIRITS,
      SKILL_CLENCH_OF_NORTH_WIND,
      SKILL_APPLY_NATURES_WRATH_DAMAGE,
      SPELL_ABSOLUTE_GEAS,
      SPELL_HIVE_COMMANDER_MARK,
      SPELL_ARTIFACT_BONUS,
      SPELL_ARTIFACT_PASSIVE,
      SPELL_ARTIFACT_SURGE,
  };
  const char *wearoff;
  char failure[128];
  size_t affect_count;
  size_t i;
  size_t j;

  if (spell_info[SPELL_ARMOR].name == NULL || spell_info[SPELL_ARMOR].name == unused_spellname)
    mag_assign_spells();

  affect_count = sizeof(internal_affects) / sizeof(internal_affects[0]);
  for (i = 0; i < affect_count; i++)
  {
    snprintf(failure, sizeof(failure), "internal affect %d is outside the wear-off table",
             internal_affects[i]);
    CuAssert(tc, failure,
             internal_affects[i] > SPELL_RESERVED_DBC && internal_affects[i] < TOP_SPELL_DEFINE);

    snprintf(failure, sizeof(failure), "internal affect %d is not registered", internal_affects[i]);
    CuAssert(tc, failure, spell_info[internal_affects[i]].name != unused_spellname);

    wearoff = get_wearoff(internal_affects[i]);
    snprintf(failure, sizeof(failure), "internal affect %d has no wear-off message",
             internal_affects[i]);
    CuAssert(tc, failure,
             wearoff != NULL && wearoff[0] != '\0' && strcmp(wearoff, "!UNUSED WEAROFF!") != 0);

    for (j = i + 1; j < affect_count; j++)
    {
      snprintf(failure, sizeof(failure), "internal affects share ID %d", internal_affects[i]);
      CuAssert(tc, failure, internal_affects[i] != internal_affects[j]);
    }
  }
}

void Test_skill_numbered_affect_expiration_dispatches_wearoff(CuTest *tc)
{
  struct affected_type af;
  struct char_data ch;
  struct char_data *saved_character_list;
  struct descriptor_data descriptor;
  bool announced;
  bool remained_for_final_tick;
  bool removed;

  if (spell_info[SPELL_ARMOR].name == NULL || spell_info[SPELL_ARMOR].name == unused_spellname)
    mag_assign_spells();

  clear_char(&ch);
  memset(&descriptor, 0, sizeof(descriptor));
  ch.player_specials = &dummy_mob;
  ch.player.short_descr = "wear-off test character";
  SET_BIT_AR(MOB_FLAGS(&ch), MOB_ISNPC);
  ch.desc = &descriptor;
  descriptor.character = &ch;
  descriptor.output = descriptor.small_outbuf;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  descriptor.pProtocol = ProtocolCreate();

  if (descriptor.pProtocol == NULL)
  {
    ch.desc = NULL;
    CuFail(tc, "could not initialize protocol output for the wear-off test");
    return;
  }

  new_affect(&af);
  af.spell = SKILL_BLEEDING_ATTACK;
  af.duration = 1;
  affect_to_char(&ch, &af);

  saved_character_list = character_list;
  ch.next = NULL;
  character_list = &ch;
  affect_update();
  remained_for_final_tick = affected_by_spell(&ch, SKILL_BLEEDING_ATTACK);
  affect_update();
  removed = !affected_by_spell(&ch, SKILL_BLEEDING_ATTACK);
  announced = strstr(descriptor.output, "The bleeding from the attack stops.") != NULL;
  character_list = saved_character_list;

  while (ch.affected != NULL)
    affect_remove_no_total(&ch, ch.affected);
  ch.desc = NULL;
  ProtocolDestroy(descriptor.pProtocol);

  CuAssertTrue(tc, remained_for_final_tick);
  CuAssertTrue(tc, removed);
  CuAssertTrue(tc, announced);
}
