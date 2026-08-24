#include "CuTest.h"

#include <string.h>

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/character/class.h"
#include "../../src/character/race.h"
#include "../../src/magic/spells.h"

static void ensure_race_equivalence_registry(void)
{
  static bool initialized = FALSE;

  if (!initialized)
  {
    assign_races();
    initialized = TRUE;
  }
}

static int count_racial_feat(int race, int feat)
{
  struct race_feat_assign *assignment = NULL;
  int count = 0;

  for (assignment = race_list[race].featassign_list; assignment != NULL;
       assignment = assignment->next)
    if (assignment->feat_num == feat)
      count++;

  return count;
}

void TestRaceEquivalenceIdsAreUniqueAndRepresentable(CuTest *tc)
{
  struct char_data ch;

  memset(&ch, 0, sizeof(ch));
  GET_REAL_RACE(&ch) = RACE_YUAN_TI;

  CuAssertTrue(tc, sizeof(ch.player.race) >= 2);
  CuAssertIntEquals(tc, 28, RACE_HALF_OGRE);
  CuAssertIntEquals(tc, 114, RACE_MYCONID);
  CuAssertIntEquals(tc, 149, RACE_WEMIC);
  CuAssertIntEquals(tc, 150, RACE_HALF_ILLITHID);
  CuAssertIntEquals(tc, 151, RACE_YUAN_TI);
  CuAssertIntEquals(tc, RACE_YUAN_TI, GET_REAL_RACE(&ch));
  CuAssertTrue(tc, RACE_WEMIC != RACE_HALF_ILLITHID);
  CuAssertTrue(tc, RACE_HALF_ILLITHID != RACE_YUAN_TI);
  CuAssertTrue(tc, RACE_YUAN_TI < NUM_EXTENDED_RACES);
}

void TestRaceEquivalenceRegistryMatchesApprovedTiers(CuTest *tc)
{
  const int races[] = {RACE_WEMIC, RACE_HALF_OGRE, RACE_HALF_ILLITHID, RACE_YUAN_TI, RACE_MYCONID};
  int count = 0;
  int race = 0;
  size_t i = 0;

  ensure_race_equivalence_registry();

  for (race = 0; race < NUM_EXTENDED_RACES; race++)
    if (race_is_creation_eligible(race))
      count++;
  CuAssertIntEquals(tc, NUM_CREATION_RACES, count);

  for (i = 0; i < sizeof(races) / sizeof(races[0]); i++)
  {
    CuAssertTrue(tc, race_list[races[i]].is_pc);
    CuAssertTrue(tc, race_is_creation_eligible(races[i]));
    CuAssertPtrNotNull(tc, race_list[races[i]].descrip);
    CuAssertTrue(tc, race_list[races[i]].racial_language >= SKILL_LANG_COMMON);
  }

  CuAssertTrue(tc, !race_is_creation_eligible(RACE_LICH));
  CuAssertTrue(tc, !race_is_creation_eligible(RACE_VAMPIRE));
  CuAssertIntEquals(tc, 1, race_list[RACE_WEMIC].epic_adv);
  CuAssertIntEquals(tc, 1, race_list[RACE_HALF_OGRE].epic_adv);
  CuAssertIntEquals(tc, 1, race_list[RACE_YUAN_TI].epic_adv);
  CuAssertIntEquals(tc, 2, race_list[RACE_HALF_ILLITHID].epic_adv);
  CuAssertIntEquals(tc, 2, race_list[RACE_MYCONID].epic_adv);
  CuAssertIntEquals(tc, 1000, race_list[RACE_WEMIC].unlock_cost);
  CuAssertIntEquals(tc, 30000, race_list[RACE_HALF_ILLITHID].unlock_cost);
}

void TestRaceEquivalenceStatsSizesAndFamilies(CuTest *tc)
{
  struct char_data ch;

  ensure_race_equivalence_registry();
  memset(&ch, 0, sizeof(ch));

  CuAssertIntEquals(tc, 8, get_race_stat(RACE_WEMIC, R_STR_MOD));
  CuAssertIntEquals(tc, 4, get_race_stat(RACE_WEMIC, R_CON_MOD));
  CuAssertIntEquals(tc, 6, get_race_stat(RACE_HALF_OGRE, R_STR_MOD));
  CuAssertIntEquals(tc, -2, get_race_stat(RACE_HALF_OGRE, R_DEX_MOD));
  CuAssertIntEquals(tc, 4, get_race_stat(RACE_HALF_ILLITHID, R_INTEL_MOD));
  CuAssertIntEquals(tc, 2, get_race_stat(RACE_YUAN_TI, R_CHA_MOD));
  CuAssertIntEquals(tc, 8, get_race_stat(RACE_MYCONID, R_STR_MOD));
  CuAssertIntEquals(tc, -4, get_race_stat(RACE_MYCONID, R_DEX_MOD));
  CuAssertIntEquals(tc, SIZE_LARGE, race_list[RACE_WEMIC].size);
  CuAssertIntEquals(tc, SIZE_LARGE, race_list[RACE_HALF_OGRE].size);
  CuAssertIntEquals(tc, SIZE_MEDIUM, race_list[RACE_HALF_ILLITHID].size);
  CuAssertIntEquals(tc, SIZE_LARGE, race_list[RACE_MYCONID].size);

  GET_REAL_RACE(&ch) = RACE_WEMIC;
  CuAssertTrue(tc, IS_MONSTROUS_HUMANOID(&ch));
  CuAssertTrue(tc, is_furry(RACE_WEMIC));
  GET_REAL_RACE(&ch) = RACE_HALF_OGRE;
  CuAssertTrue(tc, IS_GIANT(&ch));
  GET_REAL_RACE(&ch) = RACE_HALF_ILLITHID;
  CuAssertTrue(tc, IS_ABERRATION(&ch));
  GET_REAL_RACE(&ch) = RACE_YUAN_TI;
  CuAssertTrue(tc, IS_MONSTROUS_HUMANOID(&ch));
  CuAssertTrue(tc, has_scales(RACE_YUAN_TI));
  GET_REAL_RACE(&ch) = RACE_MYCONID;
  CuAssertTrue(tc, IS_PLANT(&ch));
  CuAssertTrue(tc, race_has_no_hair(RACE_MYCONID));
  CuAssertTrue(tc, !IS_HUMANOID(&ch));
}

void TestRaceEquivalenceParsersAndRacialFeats(CuTest *tc)
{
  ensure_race_equivalence_registry();

  CuAssertIntEquals(tc, RACE_WEMIC, parse_race_long("Wemic"));
  CuAssertIntEquals(tc, RACE_HALF_OGRE, parse_race_long("Half-Ogre"));
  CuAssertIntEquals(tc, RACE_HALF_ILLITHID, parse_race_long("Half-Illithid"));
  CuAssertIntEquals(tc, RACE_HALF_ILLITHID, parse_race_long("Illithid"));
  CuAssertIntEquals(tc, RACE_YUAN_TI, parse_race_long("Yuan-Ti"));
  CuAssertIntEquals(tc, RACE_MYCONID, parse_race_long("Myconid"));
  CuAssertIntEquals(tc, RACE_MYCONID, parse_race_long("Mycanoid"));

  CuAssertIntEquals(tc, 1, count_racial_feat(RACE_WEMIC, FEAT_CLAWS_AND_BITE));
  CuAssertIntEquals(tc, 2, count_racial_feat(RACE_HALF_OGRE, FEAT_ARMOR_SKIN));
  CuAssertIntEquals(tc, 1, count_racial_feat(RACE_HALF_ILLITHID, FEAT_SLA_LEVITATE));
  CuAssertIntEquals(tc, 3, count_racial_feat(RACE_HALF_ILLITHID, FEAT_ARMOR_SKIN));
  CuAssertIntEquals(tc, 1, count_racial_feat(RACE_YUAN_TI, FEAT_POISON_BITE));
  CuAssertIntEquals(tc, 1, count_racial_feat(RACE_YUAN_TI, FEAT_POISON_IMMUNITY));
  CuAssertIntEquals(tc, 4, count_racial_feat(RACE_MYCONID, FEAT_ARMOR_SKIN));
  CuAssertIntEquals(tc, 1, count_racial_feat(RACE_MYCONID, FEAT_PARALYSIS_IMMUNITY));
}

void TestRaceEquivalenceExperienceMultipliers(CuTest *tc)
{
  struct char_data ch;
  long normal = 0;
  int old_multiplier = CONFIG_EXPERIENCE_MULTIPLIER;

  memset(&ch, 0, sizeof(ch));
  GET_CLASS(&ch) = CLASS_WARRIOR;
  CONFIG_EXPERIENCE_MULTIPLIER = 100;

  GET_REAL_RACE(&ch) = RACE_HUMAN;
  normal = level_exp(&ch, 10);
  GET_REAL_RACE(&ch) = RACE_WEMIC;
  CuAssertTrue(tc, level_exp(&ch, 10) == normal * 2);
  GET_REAL_RACE(&ch) = RACE_HALF_OGRE;
  CuAssertTrue(tc, level_exp(&ch, 10) == normal * 2);
  GET_REAL_RACE(&ch) = RACE_YUAN_TI;
  CuAssertTrue(tc, level_exp(&ch, 10) == normal * 2);
  GET_REAL_RACE(&ch) = RACE_HALF_ILLITHID;
  CuAssertTrue(tc, level_exp(&ch, 10) == normal * 7);
  GET_REAL_RACE(&ch) = RACE_MYCONID;
  CuAssertTrue(tc, level_exp(&ch, 10) == normal * 7);

  CONFIG_EXPERIENCE_MULTIPLIER = old_multiplier;
}
