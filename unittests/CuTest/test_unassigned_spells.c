/* Production-linked tests for intentionally unassigned spells. */

#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/character/evolutions.h"
#include "../../src/combat/fight.h"
#include "../../src/handler.h"
#include "../../src/magic/domains_schools.h"
#include "../../src/magic/spells.h"

#include <string.h>

static const int foundational_spells[] = {
    SPELL_FARSEE,
    SPELL_REJUVENATE_MAJOR,
    SPELL_REJUVENATE_MINOR,
    SPELL_AGE,
    SPELL_COMMAND_UNDEAD,
    SPELL_SLOW_POISON,
    SPELL_COMPREHEND_LANGUAGES,
    SPELL_FUMBLE,
    SPELL_STUMBLE,
    SPELL_ENERVATE,
    SPELL_PROT_UNDEAD,
    SPELL_PROT_FROM_UNDEAD,
    SPELL_COMMAND_HORDE,
    SPELL_ANCESTRAL_SHIELD,
    SPELL_PROTECTION_FROM_ANIMALS,
    SPELL_PASS_WITHOUT_TRACE,
    SPELL_GREATER_REALM_OF_PROTECTION,
    SPELL_FEIGN_DEATH,
    SPELL_TRANQUILITY,
    SPELL_AGILITY,
    SPELL_NATURES_BLESSING,
    SPELL_SONG_OF_TRAVEL,
};

struct spell_registration_expectation
{
  int spellnum;
  int routines;
};

static const struct spell_registration_expectation offensive_spells[] = {
    {SPELL_SANDBLAST, MAG_DAMAGE | MAG_AFFECTS},
    {SPELL_FELL_FROST, MAG_DAMAGE | MAG_AFFECTS},
    {SPELL_NERVE_DANCE, MAG_DAMAGE},
    {SPELL_SPECTRAL_HAND, MAG_DAMAGE},
    {SPELL_RAIN_OF_BLOOD, MAG_AREAS},
    {SPELL_ROT, MAG_AREAS},
    {SPELL_ICE_TOMB, MAG_DAMAGE | MAG_AFFECTS},
    {SPELL_CONSTRICTION, MAG_DAMAGE | MAG_AFFECTS},
    {SPELL_SANDSTORM, MAG_AREAS | MAG_AFFECTS},
    {SPELL_BLACKLIGHT_BURST, MAG_AREAS | MAG_AFFECTS},
    {SPELL_MINUTE_METEORS, MAG_LOOPS},
    {SPELL_THUNDER_LANCE, MAG_DAMAGE},
    {SPELL_SHADOW_BOLT, MAG_LOOPS},
    {SPELL_SHADOW_BURST, MAG_AREAS},
    {SPELL_SHADOW_MAGIC, MAG_DAMAGE},
    {SPELL_PHANTASMAL_BLADES, MAG_AREAS},
    {SPELL_NEEDLE_SWARM, MAG_DAMAGE},
    {SPELL_SNAPPING_TEETH, MAG_DAMAGE},
    {SPELL_BELTYNS_BURNING_BLOOD, MAG_DAMAGE | MAG_AFFECTS},
    {SPELL_BLACKMANTLE, MAG_AFFECTS},
    {SPELL_EARTHBLOOD, MAG_DAMAGE | MAG_AFFECTS},
    {SPELL_SOUL_TEMPEST, MAG_AREAS},
    {SPELL_DUST_DEVIL, MAG_DAMAGE | MAG_AFFECTS},
    {SPELL_SUFFOCATE, MAG_DAMAGE | MAG_AFFECTS},
    {SPELL_BLACKTHORNS, MAG_DAMAGE},
    {SPELL_SHADECHILL, MAG_DAMAGE},
    {SPELL_AIR_BLAST, MAG_DAMAGE},
    {SPELL_SHADOW_FLUX, MAG_AFFECTS},
    {SPELL_POLTERGEIST, MAG_MANUAL},
};

static const struct spell_registration_expectation utility_spells[] = {
    {SPELL_MINOR_CREATE, MAG_MANUAL},
    {SPELL_VENTRILOQUATE, MAG_MANUAL},
    {SPELL_PRESERVE, MAG_ALTER_OBJS},
    {SPELL_WRAITHFORM, MAG_MANUAL},
    {SPELL_CREATE_SPRING, MAG_MANUAL},
    {SPELL_MOONWELL, MAG_MANUAL},
    {SPELL_EMBALM, MAG_ALTER_OBJS},
    {SPELL_AIRY_WATER, MAG_ROOM},
    {SPELL_BLINK, MAG_MANUAL},
    {SPELL_UNSEEN_SERVANT, MAG_AFFECTS},
    {SPELL_MISLEAD, MAG_AFFECTS},
    {SPELL_SEQUESTER, MAG_AFFECTS},
    {SPELL_DIMENSION_SHIFT, MAG_MANUAL},
    {SPELL_SOUL_BIND, MAG_AFFECTS},
    {SPELL_DEATH_PACT, MAG_GROUPS},
    {SPELL_SPIRIT_WALK, MAG_MANUAL},
    {SPELL_ROCK_TO_MUD, MAG_MANUAL},
    {SPELL_MUD_TO_ROCK, MAG_MANUAL},
    {SPELL_PHANTOM_HEAL, MAG_MANUAL},
    {SPELL_CURSE_OBJ, MAG_ALTER_OBJS},
    {SPELL_CORPSE_GLAMOR, MAG_ALTER_OBJS},
    {SPELL_SUN_SHADOW, MAG_ROOM},
    {SPELL_EARTH_FOG, MAG_ROOM},
    {SPELL_FIRE_FOG, MAG_ROOM},
};

static const struct spell_registration_expectation remaining_support_spells[] = {
    {SPELL_HEAL_UNDEAD, MAG_MANUAL},
    {SPELL_DARK_WRATH, MAG_MANUAL},
    {SPELL_UNHOLY_AURA, MAG_MANUAL},
    {SPELL_CAMOUFLAGE, MAG_MANUAL},
};

static const struct spell_registration_expectation damage_control_spells[] = {
    {SPELL_CYCLONE, MAG_AREAS},
    {SPELL_LICH_TOUCH, MAG_DAMAGE | MAG_AFFECTS},
    {SPELL_LAVA_BURST, MAG_AREAS},
    {SPELL_ICE_LAYER, MAG_MANUAL},
};

static void add_test_affect(struct char_data *ch, int spellnum, int location, int modifier)
{
  struct affected_type af;

  new_affect(&af);
  af.spell = spellnum;
  af.duration = 10;
  af.location = location;
  af.modifier = modifier;
  affect_to_char(ch, &af);
}

static void remove_test_affects(struct char_data *ch)
{
  while (ch->affected != NULL)
    affect_remove_no_total(ch, ch->affected);
}

static struct affected_type *find_test_affect(struct char_data *ch, int spellnum, int location)
{
  struct affected_type *af;

  for (af = ch->affected; af != NULL; af = af->next)
    if (af->spell == spellnum && af->location == location)
      return af;

  return NULL;
}

void TestFoundationalSpellsAreRegisteredWithoutClassAssignments(CuTest *tc)
{
  size_t index;
  int class_num;
  int domain_num;
  int spellnum;

  mag_assign_spells();

  for (index = 0; index < sizeof(foundational_spells) / sizeof(foundational_spells[0]); index++)
  {
    spellnum = foundational_spells[index];
    CuAssertTrue(tc, spell_info[spellnum].name != NULL);
    CuAssertTrue(tc, spell_info[spellnum].name != unused_spellname);
    CuAssertTrue(tc, IS_SET(spell_info[spellnum].routines, MAG_MANUAL));

    for (class_num = 0; class_num < NUM_CLASSES; class_num++)
      CuAssertIntEquals(tc, LVL_IMMORT, spell_info[spellnum].min_level[class_num]);
    for (domain_num = 0; domain_num < NUM_DOMAINS; domain_num++)
      CuAssertIntEquals(tc, LVL_IMMORT, spell_info[spellnum].domain[domain_num]);
  }
}

void TestOffensiveSpellsUseNativeRoutinesWithoutClassAssignments(CuTest *tc)
{
  size_t index;
  int class_num;
  int domain_num;
  int spellnum;

  mag_assign_spells();

  for (index = 0; index < sizeof(offensive_spells) / sizeof(offensive_spells[0]); index++)
  {
    spellnum = offensive_spells[index].spellnum;
    CuAssertTrue(tc, spell_info[spellnum].name != NULL);
    CuAssertTrue(tc, spell_info[spellnum].name != unused_spellname);
    CuAssertIntEquals(tc, offensive_spells[index].routines, spell_info[spellnum].routines);

    for (class_num = 0; class_num < NUM_CLASSES; class_num++)
      CuAssertIntEquals(tc, LVL_IMMORT, spell_info[spellnum].min_level[class_num]);
    for (domain_num = 0; domain_num < NUM_DOMAINS; domain_num++)
      CuAssertIntEquals(tc, LVL_IMMORT, spell_info[spellnum].domain[domain_num]);
  }
}

void TestUtilitySpellsUseNativeRoutinesWithoutClassAssignments(CuTest *tc)
{
  size_t index;
  int class_num;
  int domain_num;
  int spellnum;

  mag_assign_spells();

  for (index = 0; index < sizeof(utility_spells) / sizeof(utility_spells[0]); index++)
  {
    spellnum = utility_spells[index].spellnum;
    CuAssertTrue(tc, spell_info[spellnum].name != NULL);
    CuAssertTrue(tc, spell_info[spellnum].name != unused_spellname);
    CuAssertIntEquals(tc, utility_spells[index].routines, spell_info[spellnum].routines);

    for (class_num = 0; class_num < NUM_CLASSES; class_num++)
      CuAssertIntEquals(tc, LVL_IMMORT, spell_info[spellnum].min_level[class_num]);
    for (domain_num = 0; domain_num < NUM_DOMAINS; domain_num++)
      CuAssertIntEquals(tc, LVL_IMMORT, spell_info[spellnum].domain[domain_num]);
  }
}

void TestRemainingSupportSpellsAreNativeAndUnassigned(CuTest *tc)
{
  size_t index;
  int class_num;
  int domain_num;
  int spellnum;

  mag_assign_spells();

  for (index = 0; index < sizeof(remaining_support_spells) / sizeof(remaining_support_spells[0]);
       index++)
  {
    spellnum = remaining_support_spells[index].spellnum;
    CuAssertTrue(tc, spell_info[spellnum].name != NULL);
    CuAssertTrue(tc, spell_info[spellnum].name != unused_spellname);
    CuAssertIntEquals(tc, remaining_support_spells[index].routines, spell_info[spellnum].routines);

    for (class_num = 0; class_num < NUM_CLASSES; class_num++)
      CuAssertIntEquals(tc, LVL_IMMORT, spell_info[spellnum].min_level[class_num]);
    for (domain_num = 0; domain_num < NUM_DOMAINS; domain_num++)
      CuAssertIntEquals(tc, LVL_IMMORT, spell_info[spellnum].domain[domain_num]);
  }
}

void TestDamageControlSpellsAreNativeAndUnassigned(CuTest *tc)
{
  size_t index;
  int class_num;
  int domain_num;
  int spellnum;

  mag_assign_spells();

  for (index = 0; index < sizeof(damage_control_spells) / sizeof(damage_control_spells[0]); index++)
  {
    spellnum = damage_control_spells[index].spellnum;
    CuAssertTrue(tc, spell_info[spellnum].name != NULL);
    CuAssertTrue(tc, spell_info[spellnum].name != unused_spellname);
    CuAssertIntEquals(tc, damage_control_spells[index].routines, spell_info[spellnum].routines);

    for (class_num = 0; class_num < NUM_CLASSES; class_num++)
      CuAssertIntEquals(tc, LVL_IMMORT, spell_info[spellnum].min_level[class_num]);
    for (domain_num = 0; domain_num < NUM_DOMAINS; domain_num++)
      CuAssertIntEquals(tc, LVL_IMMORT, spell_info[spellnum].domain[domain_num]);
  }
}

void TestCycloneUsesSourceWindThresholdOnlyForPlayerCasters(CuTest *tc)
{
  CuAssertIntEquals(tc, 50, test_cyclone_damage_percent(true, 25));
  CuAssertIntEquals(tc, 100, test_cyclone_damage_percent(true, 26));
  CuAssertIntEquals(tc, 100, test_cyclone_damage_percent(false, 0));
}

void TestLichTouchPreservesElementalAndShieldDamageInteractions(CuTest *tc)
{
  struct char_data victim;

  clear_char(&victim);
  SET_BIT_AR(MOB_FLAGS(&victim), MOB_ISNPC);
  GET_REAL_RACE(&victim) = RACE_TYPE_ELEMENTAL;
  GET_SUBRACE(&victim, 0) = SUBRACE_FIRE;

  CuAssertIntEquals(tc, 125, test_adjust_lich_touch_damage(&victim, 100));
  SET_BIT_AR(AFF_FLAGS(&victim), AFF_CSHIELD);
  CuAssertIntEquals(tc, 62, test_adjust_lich_touch_damage(&victim, 100));
  SET_BIT_AR(AFF_FLAGS(&victim), AFF_FSHIELD);
  CuAssertIntEquals(tc, 112, test_adjust_lich_touch_damage(&victim, 100));
}

void TestIceLayerRecognizesPreservedSourceImmunities(CuTest *tc)
{
  struct char_data victim;

  clear_char(&victim);
  SET_BIT_AR(MOB_FLAGS(&victim), MOB_ISNPC);
  GET_REAL_RACE(&victim) = RACE_TYPE_HUMANOID;
  CuAssertTrue(tc, !test_ice_layer_target_is_immune(&victim));

  SET_BIT_AR(MOB_FLAGS(&victim), MOB_ROL_BEHOLDER);
  CuAssertTrue(tc, test_ice_layer_target_is_immune(&victim));
  REMOVE_BIT_AR(MOB_FLAGS(&victim), MOB_ROL_BEHOLDER);
  SET_BIT_AR(MOB_FLAGS(&victim), MOB_ROL_DEMON);
  CuAssertTrue(tc, test_ice_layer_target_is_immune(&victim));
  REMOVE_BIT_AR(MOB_FLAGS(&victim), MOB_ROL_DEMON);
  GET_REAL_RACE(&victim) = RACE_TYPE_DRAGON;
  CuAssertTrue(tc, test_ice_layer_target_is_immune(&victim));
}

void TestLavaBurstIgnitesOnlySuccessfullyDamagedSurvivors(CuTest *tc)
{
  CuAssertTrue(tc, test_lava_burst_should_ignite(0, 100, 75, false));
  CuAssertTrue(tc, !test_lava_burst_should_ignite(-1, 100, 0, false));
  CuAssertTrue(tc, !test_lava_burst_should_ignite(0, 100, 100, false));
  CuAssertTrue(tc, !test_lava_burst_should_ignite(0, 100, 75, true));
}

void TestHealUndeadPreservesLichAndBlackmantleRules(CuTest *tc)
{
  struct char_data caster;
  struct char_data target;
  struct player_special_data caster_specials;
  struct player_special_data target_specials;

  clear_char(&caster);
  clear_char(&target);
  memset(&caster_specials, 0, sizeof(caster_specials));
  memset(&target_specials, 0, sizeof(target_specials));
  caster.player_specials = &caster_specials;
  target.player_specials = &target_specials;
  GET_REAL_RACE(&caster) = RACE_LICH;
  GET_REAL_RACE(&target) = RACE_LICH;
  GET_MAX_HIT(&target) = 500;
  GET_HIT(&target) = 100;

  spell_heal_undead(30, &caster, &target, NULL, CAST_SPELL);
  CuAssertIntEquals(tc, 200, GET_HIT(&target));

  SET_BIT_AR(AFF_FLAGS(&target), AFF_BLACKMANTLE);
  spell_heal_undead(30, &caster, &target, NULL, CAST_SPELL);
  CuAssertIntEquals(tc, 200, GET_HIT(&target));
}

void TestDarkWrathAppliesSourceBonusesToAllSpellSaves(CuTest *tc)
{
  struct affected_type *af;
  struct char_data ch;
  struct player_special_data specials;

  clear_char(&ch);
  memset(&specials, 0, sizeof(specials));
  ch.player_specials = &specials;

  spell_dark_wrath(45, &ch, &ch, NULL, CAST_SPELL);

  af = find_test_affect(&ch, SPELL_DARK_WRATH, APPLY_DAMROLL);
  CuAssertPtrNotNull(tc, af);
  CuAssertIntEquals(tc, 1, af->modifier);
  CuAssertTrue(tc, af->duration >= 5 && af->duration <= 8);
  af = find_test_affect(&ch, SPELL_DARK_WRATH, APPLY_SAVING_FORT);
  CuAssertPtrNotNull(tc, af);
  CuAssertIntEquals(tc, 3, af->modifier);
  af = find_test_affect(&ch, SPELL_DARK_WRATH, APPLY_SAVING_REFL);
  CuAssertPtrNotNull(tc, af);
  CuAssertIntEquals(tc, 3, af->modifier);
  af = find_test_affect(&ch, SPELL_DARK_WRATH, APPLY_SAVING_WILL);
  CuAssertPtrNotNull(tc, af);
  CuAssertIntEquals(tc, 3, af->modifier);

  remove_test_affects(&ch);
}

void TestUnholyAuraKeepsItsOwnFireShieldAffect(CuTest *tc)
{
  struct affected_type *af;
  struct char_data ch;
  struct player_special_data specials;

  clear_char(&ch);
  memset(&specials, 0, sizeof(specials));
  ch.player_specials = &specials;

  spell_unholy_aura(30, &ch, &ch, NULL, CAST_SPELL);
  af = find_test_affect(&ch, SPELL_UNHOLY_AURA, APPLY_NONE);
  CuAssertPtrNotNull(tc, af);
  CuAssertIntEquals(tc, 3, af->duration);
  CuAssertTrue(tc, AFF_FLAGGED(&ch, AFF_FSHIELD));

  remove_test_affects(&ch);
}

void TestCamouflageHasDistinctStateAndBreaksCleanly(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data specials;

  clear_char(&ch);
  memset(&specials, 0, sizeof(specials));
  ch.player_specials = &specials;

  spell_camouflage(30, &ch, &ch, NULL, CAST_SPELL);
  CuAssertTrue(tc, affected_by_spell(&ch, SPELL_CAMOUFLAGE));
  CuAssertTrue(tc, AFF_FLAGGED(&ch, AFF_HIDE));

  remove_spell_camouflage(&ch);
  CuAssertTrue(tc, !affected_by_spell(&ch, SPELL_CAMOUFLAGE));
  CuAssertTrue(tc, !AFF_FLAGGED(&ch, AFF_HIDE));
}

void TestUnseenServantAddsOnlyItsStoredCarryCapacity(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data specials;
  int base_limit;

  clear_char(&ch);
  memset(&specials, 0, sizeof(specials));
  ch.player_specials = &specials;
  GET_REAL_STR(&ch) = ch.aff_abils.str = 10;
  base_limit = can_carry_weight_limit(&ch);

  add_test_affect(&ch, SPELL_UNSEEN_SERVANT, APPLY_SPECIAL, 75);
  CuAssertIntEquals(tc, base_limit + 75, can_carry_weight_limit(&ch));

  remove_test_affects(&ch);
}

void TestDeathPactKeepsACharacterStandingAboveItsLimit(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data specials;

  clear_char(&ch);
  memset(&specials, 0, sizeof(specials));
  ch.player_specials = &specials;
  GET_HIT(&ch) = -50;
  GET_POS(&ch) = POS_STANDING;

  add_test_affect(&ch, SPELL_DEATH_PACT, APPLY_SPECIAL, 0);
  update_pos(&ch);
  CuAssertIntEquals(tc, POS_STANDING, GET_POS(&ch));

  remove_test_affects(&ch);
  update_pos(&ch);
  CuAssertIntEquals(tc, POS_DEAD, GET_POS(&ch));
}

void TestComprehendLanguagesDoesNotGrantSpeech(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data specials;

  clear_char(&ch);
  memset(&specials, 0, sizeof(specials));
  ch.player_specials = &specials;
  GET_LEVEL(&ch) = 10;
  GET_REAL_RACE(&ch) = RACE_HUMAN;

  CuAssertTrue(tc, !can_speak_language(&ch, LANG_DRACONIC));
  CuAssertTrue(tc, !can_understand_language(&ch, LANG_DRACONIC));

  add_test_affect(&ch, SPELL_COMPREHEND_LANGUAGES, APPLY_NONE, 0);
  CuAssertTrue(tc, !can_speak_language(&ch, LANG_DRACONIC));
  CuAssertTrue(tc, can_understand_language(&ch, LANG_DRACONIC));

  remove_test_affects(&ch);
}

void TestMinorRejuvenationChangesDisplayedAgeTemporarily(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data specials;
  int original_age;

  clear_char(&ch);
  memset(&specials, 0, sizeof(specials));
  ch.player_specials = &specials;
  ch.player.time.birth = time(NULL) - (time_t)20 * SECS_PER_MUD_YEAR;
  original_age = GET_AGE(&ch);

  add_test_affect(&ch, SPELL_REJUVENATE_MINOR, APPLY_AGE, -5);
  CuAssertIntEquals(tc, original_age - 5, GET_AGE(&ch));

  remove_test_affects(&ch);
  CuAssertIntEquals(tc, original_age, GET_AGE(&ch));
}

void TestAreaSpellWardsUseTheStrongestQuarterReduction(CuTest *tc)
{
  struct char_data victim;
  struct player_special_data victim_specials;

  clear_char(&victim);
  memset(&victim_specials, 0, sizeof(victim_specials));
  victim.player_specials = &victim_specials;
  CuAssertIntEquals(tc, 100, adjust_area_damage_for_spell_wards(&victim, 100));

  add_test_affect(&victim, SPELL_ANCESTRAL_SHIELD, APPLY_NONE, 0);
  CuAssertIntEquals(tc, 75, adjust_area_damage_for_spell_wards(&victim, 100));

  add_test_affect(&victim, SPELL_NATURES_BLESSING, APPLY_HITROLL, 2);
  CuAssertIntEquals(tc, 75, adjust_area_damage_for_spell_wards(&victim, 100));

  remove_test_affects(&victim);
}

void TestCreatureWardsReduceOnlyMatchingAttackers(CuTest *tc)
{
  struct char_data animal;
  struct char_data undead;
  struct char_data humanoid;
  struct char_data victim;
  struct player_special_data animal_specials;
  struct player_special_data humanoid_specials;
  struct player_special_data undead_specials;
  struct player_special_data victim_specials;

  clear_char(&animal);
  clear_char(&undead);
  clear_char(&humanoid);
  clear_char(&victim);
  memset(&animal_specials, 0, sizeof(animal_specials));
  memset(&undead_specials, 0, sizeof(undead_specials));
  memset(&humanoid_specials, 0, sizeof(humanoid_specials));
  memset(&victim_specials, 0, sizeof(victim_specials));
  animal.player_specials = &animal_specials;
  undead.player_specials = &undead_specials;
  humanoid.player_specials = &humanoid_specials;
  victim.player_specials = &victim_specials;
  SET_BIT_AR(MOB_FLAGS(&animal), MOB_ISNPC);
  SET_BIT_AR(MOB_FLAGS(&undead), MOB_ISNPC);
  SET_BIT_AR(MOB_FLAGS(&humanoid), MOB_ISNPC);
  GET_REAL_RACE(&animal) = RACE_TYPE_ANIMAL;
  GET_REAL_RACE(&undead) = RACE_TYPE_UNDEAD;
  GET_REAL_RACE(&humanoid) = RACE_TYPE_HUMANOID;

  add_test_affect(&victim, SPELL_PROTECTION_FROM_ANIMALS, APPLY_NONE, 0);
  CuAssertIntEquals(tc, 75, adjust_damage_for_creature_wards(&animal, &victim, 100));
  CuAssertIntEquals(tc, 100, adjust_damage_for_creature_wards(&undead, &victim, 100));

  add_test_affect(&victim, SPELL_PROT_FROM_UNDEAD, APPLY_AC_NEW, 2);
  CuAssertIntEquals(tc, 75, adjust_damage_for_creature_wards(&undead, &victim, 100));
  CuAssertIntEquals(tc, 100, adjust_damage_for_creature_wards(&humanoid, &victim, 100));

  remove_test_affects(&victim);
}
