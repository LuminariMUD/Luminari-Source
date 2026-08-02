#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/interpreter.h"
#include "../../src/magic/spells.h"
#include "../../src/vessels/vessels.h"

void Test_vessel_boarding_uses_stable_dedicated_ability(CuTest *tc)
{
  bool listed = FALSE;
  int i;

  CuAssertIntEquals(tc, 27, ABILITY_BOARDING);
  CuAssertStrEquals(tc, "Boarding", ability_names[ABILITY_BOARDING]);
  CuAssertTrue(tc, is_valid_skill(ABILITY_BOARDING));

  for (i = 0; i < NUM_SKILLS_IN_GAME; i++)
  {
    if (skills_alphabetic[i] == ABILITY_BOARDING)
    {
      listed = TRUE;
      break;
    }
  }
  CuAssertTrue(tc, listed);
}

void Test_vessel_boarding_contest_is_opposed_and_ties_defend(CuTest *tc)
{
  struct vessel_boarding_contest result;

  CuAssertTrue(tc, vessel_resolve_boarding_contest(8, 12, 6, 8, 2, &result));
  CuAssertIntEquals(tc, 20, result.attacker_total);
  CuAssertIntEquals(tc, 16, result.defender_total);
  CuAssertTrue(tc, result.attacker_wins);
  CuAssertTrue(tc, !result.critical_failure);

  CuAssertTrue(tc, vessel_resolve_boarding_contest(5, 10, 5, 8, 2, &result));
  CuAssertIntEquals(tc, 15, result.attacker_total);
  CuAssertIntEquals(tc, 15, result.defender_total);
  CuAssertTrue(tc, !result.attacker_wins);
}

void Test_vessel_boarding_contest_marks_only_decisive_failures_critical(CuTest *tc)
{
  struct vessel_boarding_contest result;

  CuAssertTrue(tc, !vessel_resolve_boarding_contest(5, 0, 5, 10, 0, &result));
  CuAssertTrue(tc, !vessel_resolve_boarding_contest(5, 10, 5, 0, 0, &result));
  CuAssertTrue(tc, !vessel_resolve_boarding_contest(5, 10, 5, 10, 0, NULL));

  CuAssertTrue(tc, vessel_resolve_boarding_contest(0, 10, 10, 10, 0, &result));
  CuAssertTrue(tc, !result.attacker_wins);
  CuAssertTrue(tc, result.critical_failure);

  CuAssertTrue(tc, vessel_resolve_boarding_contest(20, 1, 5, 17, 0, &result));
  CuAssertTrue(tc, !result.attacker_wins);
  CuAssertTrue(tc, result.critical_failure);

  CuAssertTrue(tc, vessel_resolve_boarding_contest(5, 10, 5, 11, 0, &result));
  CuAssertTrue(tc, !result.critical_failure);
}

void Test_vessel_boarding_defense_distinguishes_grapple_and_crossing(CuTest *tc)
{
  struct greyhawk_ship_data ship;

  memset(&ship, 0, sizeof(ship));
  ship.vessel_type = VESSEL_WARSHIP;
  ship.speed = 4;
  ship.maxfinternal = 25;
  ship.maxrinternal = 25;
  ship.maxpinternal = 25;
  ship.maxsinternal = 25;
  ship.finternal = 25;
  ship.rinternal = 25;
  ship.pinternal = 25;
  ship.sinternal = 25;
  ship.crew_tier[CREW_SAILMASTER] = CREW_TIER_ABLE;
  ship.crew_tier[CREW_BOSUN] = CREW_TIER_GREEN;

  CuAssertIntEquals(tc, 12, vessel_boarding_defense_modifier(&ship, VESSEL_BOARDING_GRAPPLE));
  CuAssertIntEquals(tc, 8, vessel_boarding_defense_modifier(&ship, VESSEL_BOARDING_CROSSING));
}

void Test_vessel_boarding_defense_handles_damage_and_empty_maximum(CuTest *tc)
{
  struct greyhawk_ship_data ship;

  memset(&ship, 0, sizeof(ship));
  ship.vessel_type = VESSEL_RAFT;
  ship.maxfinternal = 25;
  ship.maxrinternal = 25;
  ship.maxpinternal = 25;
  ship.maxsinternal = 25;
  ship.finternal = 5;
  ship.rinternal = 5;
  ship.pinternal = 5;
  ship.sinternal = 5;
  CuAssertIntEquals(tc, -8, vessel_boarding_defense_modifier(&ship, VESSEL_BOARDING_GRAPPLE));

  memset(&ship, 0, sizeof(ship));
  ship.vessel_type = VESSEL_TRANSPORT;
  ship.maxfinternal = 25;
  ship.maxrinternal = 25;
  ship.maxpinternal = 25;
  ship.maxsinternal = 25;
  ship.finternal = 15;
  ship.rinternal = 15;
  ship.pinternal = 15;
  ship.sinternal = 15;
  CuAssertIntEquals(tc, -4, vessel_boarding_defense_modifier(&ship, VESSEL_BOARDING_CROSSING));

  memset(&ship, 0, sizeof(ship));
  ship.vessel_type = VESSEL_WARSHIP;
  CuAssertIntEquals(tc, 4, vessel_boarding_defense_modifier(&ship, VESSEL_BOARDING_CROSSING));
  CuAssertIntEquals(tc, 0, vessel_boarding_defense_modifier(&ship, (enum vessel_boarding_stage)99));
  CuAssertIntEquals(tc, 0, vessel_boarding_defense_modifier(NULL, VESSEL_BOARDING_GRAPPLE));
}
