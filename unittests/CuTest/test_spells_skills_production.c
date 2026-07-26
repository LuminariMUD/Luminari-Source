#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/spells.h"

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
