/* Production-linked tests for intentionally unassigned spells. */

#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/character/evolutions.h"
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
