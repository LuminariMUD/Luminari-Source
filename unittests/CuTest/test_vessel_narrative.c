#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/interpreter.h"
#include "../../src/vessels.h"

void Test_vessel_narrative_weather_boundaries_match_wilderness(CuTest *tc)
{
  CuAssertStrEquals(tc, "clear skies", vessel_weather_condition_name(127));
  CuAssertStrEquals(tc, "overcast skies", vessel_weather_condition_name(128));
  CuAssertStrEquals(tc, "overcast skies", vessel_weather_condition_name(177));
  CuAssertStrEquals(tc, "rain", vessel_weather_condition_name(178));
  CuAssertStrEquals(tc, "rain", vessel_weather_condition_name(199));
  CuAssertStrEquals(tc, "a heavy storm", vessel_weather_condition_name(200));
  CuAssertStrEquals(tc, "a heavy storm", vessel_weather_condition_name(224));
  CuAssertStrEquals(tc, "a thunderstorm", vessel_weather_condition_name(225));

  CuAssertIntEquals(tc, 0, vessel_weather_severity_from_value(177));
  CuAssertIntEquals(tc, 1, vessel_weather_severity_from_value(178));
  CuAssertIntEquals(tc, 1, vessel_weather_severity_from_value(199));
  CuAssertIntEquals(tc, 2, vessel_weather_severity_from_value(200));
  CuAssertIntEquals(tc, 2, vessel_weather_severity_from_value(224));
  CuAssertIntEquals(tc, 3, vessel_weather_severity_from_value(225));
}

void Test_vessel_narrative_distinguishes_every_class(CuTest *tc)
{
  static const char *expected_detail[NUM_VESSEL_TYPES] = {"raft flexes",
                                                          "boat's light hull",
                                                          "ship's timbers",
                                                          "warship's armored hull",
                                                          "airship's envelope",
                                                          "submarine's low hull",
                                                          "transport's laden hull",
                                                          "Arcane currents"};
  char message[512];
  int i;

  for (i = 0; i < NUM_VESSEL_TYPES; i++)
  {
    CuAssertTrue(tc, vessel_build_ambient_message((enum vessel_class)i, 0, 5, 10, 0, message,
                                                  sizeof(message)));
    CuAssertTrue(tc, strstr(message, expected_detail[i]) != NULL);
  }
}

void Test_vessel_narrative_distinguishes_speed_states(CuTest *tc)
{
  char message[512];

  CuAssertTrue(tc,
               vessel_build_ambient_message(VESSEL_SHIP, 0, 0, 10, 0, message, sizeof(message)));
  CuAssertTrue(tc, strstr(message, "lies still") != NULL);

  CuAssertTrue(tc,
               vessel_build_ambient_message(VESSEL_SHIP, 0, 2, 10, 0, message, sizeof(message)));
  CuAssertTrue(tc, strstr(message, "cautious headway") != NULL);

  CuAssertTrue(tc,
               vessel_build_ambient_message(VESSEL_SHIP, 0, 5, 10, 0, message, sizeof(message)));
  CuAssertTrue(tc, strstr(message, "steady pace") != NULL);

  CuAssertTrue(tc,
               vessel_build_ambient_message(VESSEL_SHIP, 0, 9, 10, 0, message, sizeof(message)));
  CuAssertTrue(tc, strstr(message, "near full speed") != NULL);

  CuAssertTrue(tc, vessel_build_ambient_message(VESSEL_SHIP, 0, 5, 0, 0, message, sizeof(message)));
  CuAssertTrue(tc, strstr(message, "cautious headway") != NULL);
}

void Test_vessel_narrative_shelters_submerged_submarine(CuTest *tc)
{
  char message[512];

  CuAssertTrue(
      tc, vessel_build_ambient_message(VESSEL_SUBMARINE, 225, 5, 10, -2, message, sizeof(message)));
  CuAssertTrue(tc, strstr(message, "pressure hull") != NULL);
  CuAssertTrue(tc, strstr(message, "Thunder above") != NULL);
  CuAssertTrue(tc, strstr(message, "deck") == NULL);

  CuAssertTrue(
      tc, vessel_build_ambient_message(VESSEL_SUBMARINE, 225, 5, 10, 0, message, sizeof(message)));
  CuAssertTrue(tc, strstr(message, "low hull") != NULL);
  CuAssertTrue(tc, strstr(message, "Lightning") != NULL);
}

void Test_vessel_narrative_rejects_invalid_or_short_output(CuTest *tc)
{
  char message[8];

  memset(message, 'x', sizeof(message));
  CuAssertTrue(tc, !vessel_build_ambient_message((enum vessel_class) - 1, 0, 0, 0, 0, message,
                                                 sizeof(message)));
  CuAssertIntEquals(tc, '\0', message[0]);
  CuAssertTrue(tc, !vessel_build_ambient_message(VESSEL_SHIP, 0, 0, 0, 0, NULL, 0));

  memset(message, 'x', sizeof(message));
  CuAssertTrue(tc,
               !vessel_build_ambient_message(VESSEL_SHIP, 0, 0, 0, 0, message, sizeof(message)));
  CuAssertIntEquals(tc, '\0', message[sizeof(message) - 1]);
}
