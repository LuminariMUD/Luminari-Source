/* Production-linked tests for intentionally unassigned spells. */

#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/act.h"
#include "../../src/character/evolutions.h"
#include "../../src/character/perks.h"
#include "../../src/combat/fight.h"
#include "../../src/dgscript/dg_scripts.h"
#include "../../src/handler.h"
#include "../../src/magic/domains_schools.h"
#include "../../src/magic/spell_prep.h"
#include "../../src/magic/spells.h"
#include "../../src/character/class.h"
#include "../../src/mud_event.h"

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

static const struct spell_registration_expectation summon_event_spells[] = {
    {SPELL_CALL_LYCANTHROPE, MAG_MANUAL},
    {SPELL_TAZRIKS_FRENZIED_HOUND, MAG_MANUAL},
};

static const struct spell_registration_expectation elemental_embodiment_spells[] = {
    {SPELL_ELEMENTAL_WATER_EMBODIMENT, MAG_MANUAL},
    {SPELL_ELEMENTAL_FIRE_EMBODIMENT, MAG_MANUAL},
    {SPELL_ELEMENTAL_EARTH_EMBODIMENT, MAG_MANUAL},
    {SPELL_ELEMENTAL_AIR_EMBODIMENT, MAG_MANUAL},
};

struct rol_spell_access_expectation
{
  int spellnum;
  int class_num;
  int min_level;
};

static const struct rol_spell_access_expectation rol_spell_access[] = {
    {SPELL_MINOR_CREATE, CLASS_BARD, 4},
    {SPELL_SONG_OF_TRAVEL, CLASS_BARD, 16},
    {SPELL_COMMAND_UNDEAD, CLASS_BLACKGUARD, 6},
    {SPELL_CURSE_OBJ, CLASS_BLACKGUARD, 10},
    {SPELL_SPECTRAL_HAND, CLASS_BLACKGUARD, 10},
    {SPELL_TAZRIKS_FRENZIED_HOUND, CLASS_BLACKGUARD, 12},
    {SPELL_DARK_WRATH, CLASS_BLACKGUARD, 15},
    {SPELL_UNHOLY_AURA, CLASS_BLACKGUARD, 15},
    {SPELL_PRESERVE, CLASS_CLERIC, 3},
    {SPELL_SLOW_POISON, CLASS_CLERIC, 3},
    {SPELL_COMMAND_UNDEAD, CLASS_CLERIC, 5},
    {SPELL_CURSE_OBJ, CLASS_CLERIC, 11},
    {SPELL_FARSEE, CLASS_CLERIC, 11},
    {SPELL_SOUL_TEMPEST, CLASS_CLERIC, 13},
    {SPELL_ANCESTRAL_SHIELD, CLASS_CLERIC, 17},
    {SPELL_GREATER_REALM_OF_PROTECTION, CLASS_CLERIC, 17},
    {SPELL_SPIRIT_WALK, CLASS_CLERIC, 17},
    {SPELL_PRESERVE, CLASS_DRUID, 3},
    {SPELL_PROTECTION_FROM_ANIMALS, CLASS_DRUID, 3},
    {SPELL_CREATE_SPRING, CLASS_DRUID, 7},
    {SPELL_DUST_DEVIL, CLASS_DRUID, 7},
    {SPELL_SUFFOCATE, CLASS_DRUID, 11},
    {SPELL_CYCLONE, CLASS_DRUID, 13},
    {SPELL_PASS_WITHOUT_TRACE, CLASS_DRUID, 13},
    {SPELL_MUD_TO_ROCK, CLASS_DRUID, 15},
    {SPELL_ROCK_TO_MUD, CLASS_DRUID, 15},
    {SPELL_MOONWELL, CLASS_DRUID, 17},
    {SPELL_COMMAND_UNDEAD, CLASS_RANGER, 6},
    {SPELL_CREATE_SPRING, CLASS_RANGER, 6},
    {SPELL_PROTECTION_FROM_ANIMALS, CLASS_RANGER, 6},
    {SPELL_DUST_DEVIL, CLASS_RANGER, 10},
    {SPELL_NATURES_BLESSING, CLASS_RANGER, 12},
    {SPELL_FARSEE, CLASS_RANGER, 15},
    {SPELL_PASS_WITHOUT_TRACE, CLASS_RANGER, 15},
    {SPELL_POLTERGEIST, CLASS_RANGER, 15},
    {SPELL_VENTRILOQUATE, CLASS_SORCERER, 1},
    {SPELL_MINOR_CREATE, CLASS_SORCERER, 4},
    {SPELL_FARSEE, CLASS_SORCERER, 8},
    {SPELL_MINOR_CREATE, CLASS_WIZARD, 1},
    {SPELL_PRESERVE, CLASS_WIZARD, 1},
    {SPELL_SHADOW_BOLT, CLASS_WIZARD, 1},
    {SPELL_VENTRILOQUATE, CLASS_WIZARD, 1},
    {SPELL_BLACKTHORNS, CLASS_WIZARD, 3},
    {SPELL_COMMAND_UNDEAD, CLASS_WIZARD, 3},
    {SPELL_PROT_FROM_UNDEAD, CLASS_WIZARD, 3},
    {SPELL_AIR_BLAST, CLASS_WIZARD, 5},
    {SPELL_BLINK, CLASS_WIZARD, 5},
    {SPELL_MINUTE_METEORS, CLASS_WIZARD, 5},
    {SPELL_REJUVENATE_MINOR, CLASS_WIZARD, 5},
    {SPELL_SOUL_BIND, CLASS_WIZARD, 5},
    {SPELL_COMMAND_HORDE, CLASS_WIZARD, 7},
    {SPELL_EMBALM, CLASS_WIZARD, 7},
    {SPELL_FARSEE, CLASS_WIZARD, 7},
    {SPELL_FUMBLE, CLASS_WIZARD, 7},
    {SPELL_SPECTRAL_HAND, CLASS_WIZARD, 7},
    {SPELL_HEAL_UNDEAD, CLASS_WIZARD, 9},
    {SPELL_SHADOW_BURST, CLASS_WIZARD, 9},
    {SPELL_SHADOW_MAGIC, CLASS_WIZARD, 9},
    {SPELL_STUMBLE, CLASS_WIZARD, 9},
    {SPELL_THUNDER_LANCE, CLASS_WIZARD, 9},
    {SPELL_AGE, CLASS_WIZARD, 11},
    {SPELL_ENERVATE, CLASS_WIZARD, 11},
    {SPELL_NERVE_DANCE, CLASS_WIZARD, 11},
    {SPELL_REJUVENATE_MAJOR, CLASS_WIZARD, 11},
    {SPELL_TRANQUILITY, CLASS_WIZARD, 11},
    {SPELL_BELTYNS_BURNING_BLOOD, CLASS_WIZARD, 13},
    {SPELL_CAMOUFLAGE, CLASS_WIZARD, 13},
    {SPELL_CORPSE_GLAMOR, CLASS_WIZARD, 13},
    {SPELL_ELEMENTAL_WATER_EMBODIMENT, CLASS_WIZARD, 13},
    {SPELL_ICE_LAYER, CLASS_WIZARD, 13},
    {SPELL_PHANTASMAL_BLADES, CLASS_WIZARD, 13},
    {SPELL_PROT_UNDEAD, CLASS_WIZARD, 13},
    {SPELL_SEQUESTER, CLASS_WIZARD, 13},
    {SPELL_SHADECHILL, CLASS_WIZARD, 13},
    {SPELL_SHADOW_FLUX, CLASS_WIZARD, 13},
    {SPELL_AIRY_WATER, CLASS_WIZARD, 15},
    {SPELL_BLACKLIGHT_BURST, CLASS_WIZARD, 15},
    {SPELL_BLACKMANTLE, CLASS_WIZARD, 15},
    {SPELL_EARTH_FOG, CLASS_WIZARD, 15},
    {SPELL_ELEMENTAL_AIR_EMBODIMENT, CLASS_WIZARD, 15},
    {SPELL_FEIGN_DEATH, CLASS_WIZARD, 15},
    {SPELL_FIRE_FOG, CLASS_WIZARD, 15},
    {SPELL_MISLEAD, CLASS_WIZARD, 15},
    {SPELL_PHANTOM_HEAL, CLASS_WIZARD, 15},
    {SPELL_RAIN_OF_BLOOD, CLASS_WIZARD, 15},
    {SPELL_SUN_SHADOW, CLASS_WIZARD, 15},
    {SPELL_CONSTRICTION, CLASS_WIZARD, 17},
    {SPELL_DEATH_PACT, CLASS_WIZARD, 17},
    {SPELL_DIMENSION_SHIFT, CLASS_WIZARD, 17},
    {SPELL_EARTHBLOOD, CLASS_WIZARD, 17},
    {SPELL_ELEMENTAL_EARTH_EMBODIMENT, CLASS_WIZARD, 17},
    {SPELL_ELEMENTAL_FIRE_EMBODIMENT, CLASS_WIZARD, 17},
    {SPELL_FELL_FROST, CLASS_WIZARD, 17},
    {SPELL_ICE_TOMB, CLASS_WIZARD, 17},
    {SPELL_LAVA_BURST, CLASS_WIZARD, 17},
    {SPELL_LICH_TOUCH, CLASS_WIZARD, 17},
    {SPELL_ROT, CLASS_WIZARD, 17},
    {SPELL_SANDBLAST, CLASS_WIZARD, 17},
    {SPELL_SANDSTORM, CLASS_WIZARD, 17},
};

static const int rol_content_only_spells[] = {
    SPELL_COMPREHEND_LANGUAGES, SPELL_WRAITHFORM, SPELL_UNSEEN_SERVANT,   SPELL_NEEDLE_SWARM,
    SPELL_SNAPPING_TEETH,       SPELL_AGILITY,    SPELL_CALL_LYCANTHROPE,
};

struct elemental_embodiment_expectation
{
  int spellnum;
  int hp_factor;
  int armor_bonus;
  int size_percent;
  int resistance_count;
  int resistances[3];
  int flag_count;
  int flags[3];
};

static const struct elemental_embodiment_expectation elemental_embodiment_effects[] = {
    {SPELL_ELEMENTAL_WATER_EMBODIMENT,
     5,
     0,
     25,
     3,
     {APPLY_RES_FIRE, APPLY_RES_POISON, APPLY_RES_ACID},
     1,
     {AFF_WATER_BREATH, 0, 0}},
    {SPELL_ELEMENTAL_FIRE_EMBODIMENT,
     7,
     6,
     35,
     2,
     {APPLY_RES_POISON, APPLY_RES_FIRE, 0},
     3,
     {AFF_FSHIELD, AFF_HASTE, AFF_FLYING}},
    {SPELL_ELEMENTAL_EARTH_EMBODIMENT,
     7,
     0,
     50,
     2,
     {APPLY_RES_POISON, APPLY_RES_COLD, 0},
     0,
     {0, 0, 0}},
    {SPELL_ELEMENTAL_AIR_EMBODIMENT,
     3,
     5,
     15,
     2,
     {APPLY_RES_POISON, APPLY_RES_ACID, 0},
     2,
     {AFF_HASTE, AFF_FLYING, 0}},
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

void TestSummonEventSpellsAreNativeAndUnassigned(CuTest *tc)
{
  size_t index;
  int class_num;
  int domain_num;
  int spellnum;

  mag_assign_spells();

  for (index = 0; index < sizeof(summon_event_spells) / sizeof(summon_event_spells[0]); index++)
  {
    spellnum = summon_event_spells[index].spellnum;
    CuAssertTrue(tc, spell_info[spellnum].name != NULL);
    CuAssertTrue(tc, spell_info[spellnum].name != unused_spellname);
    CuAssertIntEquals(tc, summon_event_spells[index].routines, spell_info[spellnum].routines);

    for (class_num = 0; class_num < NUM_CLASSES; class_num++)
      CuAssertIntEquals(tc, LVL_IMMORT, spell_info[spellnum].min_level[class_num]);
    for (domain_num = 0; domain_num < NUM_DOMAINS; domain_num++)
      CuAssertIntEquals(tc, LVL_IMMORT, spell_info[spellnum].domain[domain_num]);
  }
}

void TestElementalEmbodimentSpellsAreNativeAndUnassigned(CuTest *tc)
{
  size_t index;
  int class_num;
  int domain_num;
  int spellnum;

  mag_assign_spells();

  for (index = 0;
       index < sizeof(elemental_embodiment_spells) / sizeof(elemental_embodiment_spells[0]);
       index++)
  {
    spellnum = elemental_embodiment_spells[index].spellnum;
    CuAssertTrue(tc, spell_info[spellnum].name != NULL);
    CuAssertTrue(tc, spell_info[spellnum].name != unused_spellname);
    CuAssertIntEquals(tc, elemental_embodiment_spells[index].routines,
                      spell_info[spellnum].routines);

    for (class_num = 0; class_num < NUM_CLASSES; class_num++)
      CuAssertIntEquals(tc, LVL_IMMORT, spell_info[spellnum].min_level[class_num]);
    for (domain_num = 0; domain_num < NUM_DOMAINS; domain_num++)
      CuAssertIntEquals(tc, LVL_IMMORT, spell_info[spellnum].domain[domain_num]);
  }
}

static void initialize_rol_spell_access(void)
{
  mag_assign_spells();
  if (class_list[CLASS_WIZARD].name == NULL)
    load_class_list();
  init_spell_levels();
}

static int expected_rol_spell_level(int spellnum, int class_num)
{
  size_t index;

  for (index = 0; index < sizeof(rol_spell_access) / sizeof(rol_spell_access[0]); index++)
    if (rol_spell_access[index].spellnum == spellnum &&
        rol_spell_access[index].class_num == class_num)
      return rol_spell_access[index].min_level;

  return LVL_IMMORT;
}

static bool first_rol_spell_access(size_t index)
{
  size_t prior;

  for (prior = 0; prior < index; prior++)
    if (rol_spell_access[prior].spellnum == rol_spell_access[index].spellnum)
      return FALSE;

  return TRUE;
}

void TestRolSpellKitsHaveExactInitializedClassAccess(CuTest *tc)
{
  char message[256];
  size_t index;
  size_t content_index;
  int class_num;
  int domain_num;
  int expected_level;
  int unique_spells = 0;

  CuAssertIntEquals(tc, 99, sizeof(rol_spell_access) / sizeof(rol_spell_access[0]));
  initialize_rol_spell_access();

  for (index = 0; index < sizeof(rol_spell_access) / sizeof(rol_spell_access[0]); index++)
  {
    if (!first_rol_spell_access(index))
      continue;

    unique_spells++;
    for (class_num = 0; class_num < NUM_CLASSES; class_num++)
    {
      expected_level = expected_rol_spell_level(rol_spell_access[index].spellnum, class_num);
      snprintf(message, sizeof(message), "%s class %d access",
               spell_info[rol_spell_access[index].spellnum].name, class_num);
      CuAssertIntEquals_Msg(tc, message, expected_level,
                            spell_info[rol_spell_access[index].spellnum].min_level[class_num]);
    }
    for (domain_num = 0; domain_num < NUM_DOMAINS; domain_num++)
      CuAssertIntEquals(tc, LVL_IMMORT,
                        spell_info[rol_spell_access[index].spellnum].domain[domain_num]);
  }
  CuAssertIntEquals(tc, 82, unique_spells);

  for (content_index = 0;
       content_index < sizeof(rol_content_only_spells) / sizeof(rol_content_only_spells[0]);
       content_index++)
  {
    for (class_num = 0; class_num < NUM_CLASSES; class_num++)
      CuAssertIntEquals(tc, LVL_IMMORT,
                        spell_info[rol_content_only_spells[content_index]].min_level[class_num]);
    for (domain_num = 0; domain_num < NUM_DOMAINS; domain_num++)
      CuAssertIntEquals(tc, LVL_IMMORT,
                        spell_info[rol_content_only_spells[content_index]].domain[domain_num]);
  }
}

void TestRolSpellKitLevelAndMulticlassBoundaries(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data specials;

  initialize_rol_spell_access();
  clear_char(&ch);
  memset(&specials, 0, sizeof(specials));
  ch.player_specials = &specials;
  ch.player.name = "spell access tester";
  GET_LEVEL(&ch) = 30;

  CLASS_LEVEL((&ch), CLASS_CLERIC) = 10;
  CuAssertTrue(tc, !is_min_level_for_spell(&ch, CLASS_CLERIC, SPELL_FARSEE));
  CLASS_LEVEL((&ch), CLASS_CLERIC) = 11;
  CuAssertTrue(tc, is_min_level_for_spell(&ch, CLASS_CLERIC, SPELL_FARSEE));
  CLASS_LEVEL((&ch), CLASS_CLERIC) = 0;

  CLASS_LEVEL((&ch), CLASS_WIZARD) = 8;
  CuAssertTrue(tc, !is_min_level_for_spell(&ch, CLASS_WIZARD, SPELL_THUNDER_LANCE));
  CLASS_LEVEL((&ch), CLASS_WIZARD) = 9;
  CuAssertTrue(tc, is_min_level_for_spell(&ch, CLASS_WIZARD, SPELL_THUNDER_LANCE));
  CLASS_LEVEL((&ch), CLASS_WIZARD) = 0;

  CLASS_LEVEL((&ch), CLASS_BARD) = 15;
  CuAssertTrue(tc, !is_min_level_for_spell(&ch, CLASS_BARD, SPELL_SONG_OF_TRAVEL));
  CLASS_LEVEL((&ch), CLASS_BARD) = 16;
  CuAssertTrue(tc, is_min_level_for_spell(&ch, CLASS_BARD, SPELL_SONG_OF_TRAVEL));
  CLASS_LEVEL((&ch), CLASS_BARD) = 0;

  CLASS_LEVEL((&ch), CLASS_RANGER) = 14;
  CuAssertTrue(tc, !is_min_level_for_spell(&ch, CLASS_RANGER, SPELL_POLTERGEIST));
  CLASS_LEVEL((&ch), CLASS_RANGER) = 15;
  CuAssertTrue(tc, is_min_level_for_spell(&ch, CLASS_RANGER, SPELL_POLTERGEIST));
  CLASS_LEVEL((&ch), CLASS_RANGER) = 0;

  CLASS_LEVEL((&ch), CLASS_BLACKGUARD) = 11;
  CuAssertTrue(tc, !is_min_level_for_spell(&ch, CLASS_BLACKGUARD, SPELL_TAZRIKS_FRENZIED_HOUND));
  CLASS_LEVEL((&ch), CLASS_BLACKGUARD) = 12;
  CuAssertTrue(tc, is_min_level_for_spell(&ch, CLASS_BLACKGUARD, SPELL_TAZRIKS_FRENZIED_HOUND));
  CLASS_LEVEL((&ch), CLASS_BLACKGUARD) = 0;

  CLASS_LEVEL((&ch), CLASS_WARRIOR) = 30;
  CuAssertTrue(tc, !is_min_level_for_spell(&ch, CLASS_WARRIOR, SPELL_FARSEE));
  CuAssertTrue(tc, !is_domain_spell_of_ch(&ch, SPELL_FARSEE));
}

void TestElementalistEmbodimentsRequireMasterOfElementsForPreparation(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data specials;
  struct char_perk_data master;

  initialize_rol_spell_access();
  clear_char(&ch);
  memset(&specials, 0, sizeof(specials));
  memset(&master, 0, sizeof(master));
  ch.player_specials = &specials;
  ch.player.name = "elementalist";
  GET_LEVEL(&ch) = 30;
  CLASS_LEVEL((&ch), CLASS_WIZARD) = 13;
  init_class(&ch, CLASS_WIZARD, 13);

  CuAssertIntEquals(tc, 99, GET_SKILL(&ch, SPELL_ELEMENTAL_WATER_EMBODIMENT));
  CuAssertTrue(tc, !meets_spell_access_prerequisites(&ch, SPELL_ELEMENTAL_WATER_EMBODIMENT));
  CuAssertTrue(tc, !is_min_level_for_spell(&ch, CLASS_WIZARD, SPELL_ELEMENTAL_WATER_EMBODIMENT));

  master.perk_id = PERK_WIZARD_MASTER_OF_ELEMENTS;
  master.perk_class = CLASS_WIZARD;
  master.current_rank = 1;
  specials.saved.perks = &master;
  CuAssertTrue(tc, meets_spell_access_prerequisites(&ch, SPELL_ELEMENTAL_WATER_EMBODIMENT));
  CuAssertTrue(tc, is_min_level_for_spell(&ch, CLASS_WIZARD, SPELL_ELEMENTAL_WATER_EMBODIMENT));

  collection_add(&ch, CLASS_WIZARD, SPELL_ELEMENTAL_WATER_EMBODIMENT, METAMAGIC_NONE, 0,
                 DOMAIN_UNDEFINED);
  CuAssertIntEquals(tc, CLASS_WIZARD,
                    spell_prep_gen_check(&ch, SPELL_ELEMENTAL_WATER_EMBODIMENT, METAMAGIC_NONE));
  clear_collection_by_class(&ch, CLASS_WIZARD);
  specials.saved.perks = NULL;
}

void TestBattlechanterSpellUsesBardKnownSpellPath(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data specials;

  initialize_rol_spell_access();
  clear_char(&ch);
  memset(&specials, 0, sizeof(specials));
  ch.player_specials = &specials;
  ch.player.name = "battlechanter";
  GET_LEVEL(&ch) = 16;
  GET_CLASS(&ch) = CLASS_BARD;
  CLASS_LEVEL((&ch), CLASS_BARD) = 16;

  CuAssertIntEquals(tc, 6,
                    compute_spells_circle(&ch, CLASS_BARD, SPELL_SONG_OF_TRAVEL, METAMAGIC_NONE,
                                          DOMAIN_UNDEFINED));
  CuAssertTrue(tc, known_spells_add(&ch, CLASS_BARD, SPELL_SONG_OF_TRAVEL, FALSE));
  CuAssertTrue(tc, is_a_known_spell(&ch, CLASS_BARD, SPELL_SONG_OF_TRAVEL));
  clear_known_spells_by_class(&ch, CLASS_BARD);
}

void TestMasterOfElementsRequiresTwoFocusedElementPerks(CuTest *tc)
{
  char error[MAX_STRING_LENGTH];
  struct char_data ch;
  struct player_special_data specials;
  struct char_perk_data fire;
  struct char_perk_data cold;

  if (get_perk_by_id(PERK_WIZARD_MASTER_OF_ELEMENTS) == NULL)
    init_perks();
  if (class_list[CLASS_WIZARD].name == NULL)
    load_class_list();
  clear_char(&ch);
  memset(&specials, 0, sizeof(specials));
  memset(&fire, 0, sizeof(fire));
  memset(&cold, 0, sizeof(cold));
  ch.player_specials = &specials;
  ch.player.name = "perk tester";
  GET_LEVEL(&ch) = 1;
  CLASS_LEVEL((&ch), CLASS_WIZARD) = 1;
  specials.saved.perk_points[CLASS_WIZARD] = 100;

  CuAssertTrue(tc, !can_purchase_perk(&ch, PERK_WIZARD_MASTER_OF_ELEMENTS, CLASS_WIZARD, error,
                                      sizeof(error)));
  CuAssertTrue(tc, strstr(error, "any two Focused Element perks") != NULL);

  fire.perk_id = PERK_WIZARD_FOCUSED_ELEMENT_FIRE;
  fire.perk_class = CLASS_WIZARD;
  fire.current_rank = 1;
  specials.saved.perks = &fire;
  CuAssertTrue(tc, !can_purchase_perk(&ch, PERK_WIZARD_MASTER_OF_ELEMENTS, CLASS_WIZARD, error,
                                      sizeof(error)));

  cold.perk_id = PERK_WIZARD_FOCUSED_ELEMENT_COLD;
  cold.perk_class = CLASS_WIZARD;
  cold.current_rank = 1;
  fire.next = &cold;
  CuAssertTrue(tc, can_purchase_perk(&ch, PERK_WIZARD_MASTER_OF_ELEMENTS, CLASS_WIZARD, error,
                                     sizeof(error)));
  specials.saved.perks = NULL;
}

void TestDireRaiderWolfBondRequiresRangerWarriorMulticlass(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data specials;

  clear_char(&ch);
  memset(&specials, 0, sizeof(specials));
  ch.player_specials = &specials;
  ch.player.name = "dire raider";
  SET_FEAT(&ch, FEAT_ANIMAL_COMPANION, 1);

  CLASS_LEVEL((&ch), CLASS_RANGER) = 3;
  CLASS_LEVEL((&ch), CLASS_WARRIOR) = 1;
  CuAssertTrue(tc, !can_select_dire_wolf_companion(&ch));
  CLASS_LEVEL((&ch), CLASS_RANGER) = 4;
  CLASS_LEVEL((&ch), CLASS_WARRIOR) = 0;
  CuAssertTrue(tc, !can_select_dire_wolf_companion(&ch));
  CLASS_LEVEL((&ch), CLASS_WARRIOR) = 1;
  CuAssertTrue(tc, can_select_dire_wolf_companion(&ch));
  CuAssertTrue(tc, ok_call_mob_vnum(MOB_DIRE_WOLF));
}

void TestCallLycanthropePreservesLevelAndCharmCheckBounds(CuTest *tc)
{
  CuAssertIntEquals(tc, 1, test_call_lycanthrope_level(1));
  CuAssertIntEquals(tc, 20, test_call_lycanthrope_level(30));
  CuAssertIntEquals(tc, 40, test_call_lycanthrope_level(75));
  CuAssertIntEquals(tc, 1, test_call_lycanthrope_charm_save_target(1));
  CuAssertIntEquals(tc, 8, test_call_lycanthrope_charm_save_target(10));
  CuAssertIntEquals(tc, 20, test_call_lycanthrope_charm_save_target(30));
}

void TestTazriksEventStateAllowsExactlyThreeStrikeIndices(CuTest *tc)
{
  room_vnum room = 0;
  int strike = -1;

  CuAssertTrue(tc, test_tazriks_event_state("2000123 0", &room, &strike));
  CuAssertIntEquals(tc, 2000123, room);
  CuAssertIntEquals(tc, 0, strike);
  CuAssertTrue(tc, test_tazriks_event_state("2000123 2", &room, &strike));
  CuAssertIntEquals(tc, 2, strike);
  CuAssertTrue(tc, !test_tazriks_event_state("2000123 3", &room, &strike));
  CuAssertTrue(tc, !test_tazriks_event_state("not event state", &room, &strike));
}

void TestSummonEventRegistryUsesDedicatedHandlers(CuTest *tc)
{
  CuAssertTrue(tc, mud_event_index[eROL_CALL_LYCANTHROPE_CHARM].func ==
                       event_rol_call_lycanthrope_charm);
  CuAssertTrue(tc, mud_event_index[eROL_TAZRIKS_FRENZIED_HOUND].func ==
                       event_rol_tazriks_frenzied_hound);
}

void TestElementalEmbodimentsPreserveProfilesAndLinkedCleanup(CuTest *tc)
{
  struct affected_type *af;
  struct char_data caster;
  struct char_data target;
  struct player_special_data caster_specials;
  struct player_special_data target_specials;
  const struct elemental_embodiment_expectation *expected;
  long caster_id;
  long target_id;
  size_t index;
  int base_hp;
  int flag_index;
  int resistance_index;
  int variance;

  for (index = 0;
       index < sizeof(elemental_embodiment_effects) / sizeof(elemental_embodiment_effects[0]);
       index++)
  {
    expected = &elemental_embodiment_effects[index];
    clear_char(&caster);
    clear_char(&target);
    memset(&caster_specials, 0, sizeof(caster_specials));
    memset(&target_specials, 0, sizeof(target_specials));
    caster.player_specials = &caster_specials;
    target.player_specials = &target_specials;
    caster.player.name = "elementalist";
    target.player.name = "subject";
    GET_LEVEL(&caster) = 20;
    GET_LEVEL(&target) = 10;
    GET_REAL_RACE(&caster) = RACE_HUMAN;
    GET_REAL_RACE(&target) = RACE_HUMAN;
    GET_REAL_MAX_HIT(&target) = GET_MAX_HIT(&target) = 500;
    GET_HIT(&target) = 100;
    GET_HEIGHT(&target) = 100;
    GET_WEIGHT(&target) = 100;

    switch (expected->spellnum)
    {
    case SPELL_ELEMENTAL_WATER_EMBODIMENT:
      spell_elemental_water_embodiment(20, &caster, &target, NULL, CAST_SPELL);
      break;
    case SPELL_ELEMENTAL_FIRE_EMBODIMENT:
      spell_elemental_fire_embodiment(20, &caster, &target, NULL, CAST_SPELL);
      break;
    case SPELL_ELEMENTAL_EARTH_EMBODIMENT:
      spell_elemental_earth_embodiment(20, &caster, &target, NULL, CAST_SPELL);
      break;
    case SPELL_ELEMENTAL_AIR_EMBODIMENT:
      spell_elemental_air_embodiment(20, &caster, &target, NULL, CAST_SPELL);
      break;
    }

    caster_id = GET_ID(&caster);
    target_id = GET_ID(&target);
    CuAssertTrue(tc, caster_id > 0);
    CuAssertTrue(tc, target_id > 0);
    CuAssertTrue(tc, rol_elemental_embodiment_active(&target));

    af = find_test_affect(&target, expected->spellnum, APPLY_HIT);
    CuAssertPtrNotNull(tc, af);
    base_hp = 10 * expected->hp_factor;
    variance = base_hp * 5 / 100;
    CuAssertTrue(tc, af->modifier >= base_hp - variance);
    CuAssertTrue(tc, af->modifier <= base_hp + variance);
    CuAssertIntEquals(tc, 10, af->duration);
    CuAssertTrue(tc, af->source_id == caster_id);
    CuAssertIntEquals(tc, 100 + af->modifier, GET_HIT(&target));

    af = find_test_affect(&target, expected->spellnum, APPLY_CHAR_HEIGHT);
    CuAssertPtrNotNull(tc, af);
    CuAssertIntEquals(tc, expected->size_percent, af->modifier);
    af = find_test_affect(&target, expected->spellnum, APPLY_CHAR_WEIGHT);
    CuAssertPtrNotNull(tc, af);
    CuAssertIntEquals(tc, expected->size_percent, af->modifier);

    if (expected->armor_bonus > 0)
    {
      af = find_test_affect(&target, expected->spellnum, APPLY_AC_NEW);
      CuAssertPtrNotNull(tc, af);
      CuAssertIntEquals(tc, expected->armor_bonus, af->modifier);
      CuAssertIntEquals(tc, BONUS_TYPE_NATURALARMOR, af->bonus_type);
    }
    else
    {
      CuAssertPtrEquals(tc, NULL, find_test_affect(&target, expected->spellnum, APPLY_AC_NEW));
    }

    for (resistance_index = 0; resistance_index < expected->resistance_count; resistance_index++)
    {
      af = find_test_affect(&target, expected->spellnum, expected->resistances[resistance_index]);
      CuAssertPtrNotNull(tc, af);
      CuAssertIntEquals(tc, 50, af->modifier);
    }
    for (flag_index = 0; flag_index < expected->flag_count; flag_index++)
      CuAssertTrue(tc, AFF_FLAGGED(&target, expected->flags[flag_index]));

    af = find_test_affect(&caster, AFFECT_ROL_ELEMENTAL_EMBODIMENT_MAINTAIN, APPLY_NONE);
    CuAssertPtrNotNull(tc, af);
    CuAssertIntEquals(tc, expected->spellnum, af->specific);
    CuAssertTrue(tc, af->source_id == target_id);

    if (index % 2 == 0)
      affect_from_char(&caster, AFFECT_ROL_ELEMENTAL_EMBODIMENT_MAINTAIN);
    else
      affect_from_char(&target, expected->spellnum);
    CuAssertTrue(tc, !rol_elemental_embodiment_active(&target));
    CuAssertTrue(tc, !affected_by_spell(&caster, AFFECT_ROL_ELEMENTAL_EMBODIMENT_MAINTAIN));
    CuAssertIntEquals(tc, 100, GET_HEIGHT(&target));
    CuAssertIntEquals(tc, 100, GET_WEIGHT(&target));

    remove_from_lookup_table(caster_id);
    remove_from_lookup_table(target_id);
  }
}

void TestElementalEmbodimentExpiryClearsBothEndsWithoutRetickingOtherAffects(CuTest *tc)
{
  struct char_data caster;
  struct char_data target;
  struct char_data *recipient;
  struct char_data *expired;
  struct player_special_data caster_specials;
  struct player_special_data target_specials;
  struct affected_type *af;
  int scenario;
  bool linked;
  int remaining_duration;

  for (scenario = 0; scenario < 3; scenario++)
  {
    clear_char(&caster);
    clear_char(&target);
    memset(&caster_specials, 0, sizeof(caster_specials));
    memset(&target_specials, 0, sizeof(target_specials));
    caster.player_specials = &caster_specials;
    target.player_specials = &target_specials;
    GET_LEVEL(&caster) = GET_LEVEL(&target) = 20;
    GET_REAL_RACE(&caster) = GET_REAL_RACE(&target) = RACE_HUMAN;
    GET_REAL_MAX_HIT(&caster) = GET_MAX_HIT(&caster) = GET_HIT(&caster) = 100;
    GET_REAL_MAX_HIT(&target) = GET_MAX_HIT(&target) = GET_HIT(&target) = 100;
    recipient = scenario == 2 ? &caster : &target;
    expired = scenario == 1 ? &target : &caster;
    spell_elemental_water_embodiment(20, &caster, recipient, NULL, CAST_SPELL);
    add_test_affect(expired, SPELL_DARK_WRATH, APPLY_DAMROLL, 1);
    for (af = expired->affected; af != NULL; af = af->next)
      if (rol_elemental_embodiment_affect_is_transient(af->spell))
      {
        af->duration = 0;
        break;
      }

    affect_update_character_one(expired);
    linked = rol_elemental_embodiment_active(recipient) ||
             affected_by_spell(&caster, AFFECT_ROL_ELEMENTAL_EMBODIMENT_MAINTAIN);
    af = find_test_affect(expired, SPELL_DARK_WRATH, APPLY_DAMROLL);
    remaining_duration = af == NULL ? -1 : af->duration;

    remove_all_rol_elemental_embodiments(&caster);
    remove_all_rol_elemental_embodiments(&target);
    remove_test_affects(&caster);
    remove_test_affects(&target);
    remove_from_lookup_table(GET_ID(&caster));
    if (GET_ID(&target) > 0)
      remove_from_lookup_table(GET_ID(&target));
    CuAssertTrue(tc, !linked);
    CuAssertIntEquals(tc, 9, remaining_duration);
  }
}

void TestElementalEmbodimentSharedEligibilityAndTransientIdentity(CuTest *tc)
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
  GET_LEVEL(&caster) = 20;
  GET_LEVEL(&target) = 20;
  GET_REAL_RACE(&caster) = RACE_HUMAN;
  GET_REAL_RACE(&target) = RACE_DROW;

  CuAssertTrue(tc, !test_rol_elemental_embodiment_same_side(&caster, &target));
  GET_LEVEL(&caster) = LVL_IMMORT;
  CuAssertTrue(tc, test_rol_elemental_embodiment_same_side(&caster, &target));

  CuAssertTrue(tc, rol_elemental_embodiment_affect_is_transient(SPELL_ELEMENTAL_WATER_EMBODIMENT));
  CuAssertTrue(tc, rol_elemental_embodiment_affect_is_transient(SPELL_ELEMENTAL_FIRE_EMBODIMENT));
  CuAssertTrue(tc, rol_elemental_embodiment_affect_is_transient(SPELL_ELEMENTAL_EARTH_EMBODIMENT));
  CuAssertTrue(tc, rol_elemental_embodiment_affect_is_transient(SPELL_ELEMENTAL_AIR_EMBODIMENT));
  CuAssertTrue(
      tc, rol_elemental_embodiment_affect_is_transient(AFFECT_ROL_ELEMENTAL_EMBODIMENT_MAINTAIN));
  CuAssertTrue(tc, !rol_elemental_embodiment_affect_is_transient(SPELL_GENIEKIND));
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

void TestPhantomHealingIsRepaidExactlyOnceOnExpiryOrRemoval(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data specials;
  struct affected_type *af;
  int removal;
  int wounded;
  int remaining_hit;
  int repeated_hit;
  bool remaining_affect;

  mag_assign_spells();
  for (removal = 0; removal < 3; removal++)
  {
    for (wounded = 0; wounded < 2; wounded++)
    {
      clear_char(&ch);
      memset(&specials, 0, sizeof(specials));
      ch.player_specials = &specials;
      GET_LEVEL(&ch) = 20;
      GET_REAL_RACE(&ch) = RACE_HUMAN;
      GET_REAL_MAX_HIT(&ch) = GET_MAX_HIT(&ch) = 100;
      GET_HIT(&ch) = 20;
      spell_phantom_heal(20, &ch, &ch, NULL, CAST_SPELL);
      af = find_test_affect(&ch, SPELL_PHANTOM_HEAL, APPLY_SPECIAL);
      CuAssertPtrNotNull(tc, af);
      CuAssertIntEquals(tc, 50, GET_HIT(&ch));
      if (wounded)
        GET_HIT(&ch) = 5;

      if (removal == 0)
      {
        af->duration = 0;
        affect_update_character_one(&ch);
      }
      else if (removal == 1)
        affect_from_char(&ch, SPELL_PHANTOM_HEAL);
      else
        spell_dispel_magic(20, &ch, &ch, NULL, CAST_SPELL);

      remaining_hit = GET_HIT(&ch);
      remaining_affect = affected_by_spell(&ch, SPELL_PHANTOM_HEAL);
      affect_from_char(&ch, SPELL_PHANTOM_HEAL);
      affect_update_character_one(&ch);
      repeated_hit = GET_HIT(&ch);
      remove_test_affects(&ch);
      CuAssertTrue(tc, !remaining_affect);
      CuAssertIntEquals(tc, wounded ? -10 : 20, remaining_hit);
      CuAssertIntEquals(tc, remaining_hit, repeated_hit);
    }
  }
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
