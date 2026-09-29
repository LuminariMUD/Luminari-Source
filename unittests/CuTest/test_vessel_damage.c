/* Vessel damage model (vessels-ships study S3): class condition profiles,
 * Duris arcs, shipyard prices, hull and sail damage, breaches, sinking,
 * salvage, and the disabled-prize rules. */

#include "CuTest.h"

#include "conf.h"
#include "../../src/core/sysdep.h"
#include <math.h> /* before utils.h, which defines log() as a macro */
#include "../../src/core/structs.h"
#include "../../src/core/utils.h"
#include "../../src/core/comm.h"
#include "../../src/core/db.h"
#include "../../src/vessels/vessels.h"

#include <string.h>

void Test_vessel_arcs_follow_the_duris_bands(CuTest *tc)
{
  /* Fore and rear span 80 degrees, the beams 100. */
  CuAssertIntEquals(tc, GREYHAWK_FORE, vessel_arc_for_relative_bearing(0));
  CuAssertIntEquals(tc, GREYHAWK_FORE, vessel_arc_for_relative_bearing(39));
  CuAssertIntEquals(tc, GREYHAWK_STARBOARD, vessel_arc_for_relative_bearing(40));
  CuAssertIntEquals(tc, GREYHAWK_STARBOARD, vessel_arc_for_relative_bearing(139));
  CuAssertIntEquals(tc, GREYHAWK_REAR, vessel_arc_for_relative_bearing(140));
  CuAssertIntEquals(tc, GREYHAWK_REAR, vessel_arc_for_relative_bearing(219));
  CuAssertIntEquals(tc, GREYHAWK_PORT, vessel_arc_for_relative_bearing(220));
  CuAssertIntEquals(tc, GREYHAWK_PORT, vessel_arc_for_relative_bearing(319));
  CuAssertIntEquals(tc, GREYHAWK_FORE, vessel_arc_for_relative_bearing(320));

  /* Bearings outside 0-359 wrap. */
  CuAssertIntEquals(tc, GREYHAWK_FORE, vessel_arc_for_relative_bearing(-30));
  CuAssertIntEquals(tc, GREYHAWK_STARBOARD, vessel_arc_for_relative_bearing(400));
  CuAssertIntEquals(tc, GREYHAWK_PORT, vessel_arc_for_relative_bearing(-90));
}

void Test_vessel_class_profiles_keep_the_duris_shape(CuTest *tc)
{
  const struct vessel_class_condition *condition;
  int vessel_type;

  /* Every class states its profile at its beam armor: the beams are the
   * beam armor, the bow and stern are lighter, and a default hull costs the
   * class price. */
  for (vessel_type = 0; vessel_type < NUM_VESSEL_TYPES; vessel_type++)
  {
    condition = vessel_class_condition((enum vessel_class)vessel_type);
    CuAssertIntEquals(tc, condition->beam_armor, condition->armor[GREYHAWK_PORT]);
    CuAssertIntEquals(tc, condition->beam_armor, condition->armor[GREYHAWK_STARBOARD]);
    CuAssertTrue(tc, condition->armor[GREYHAWK_FORE] <= condition->beam_armor);
    CuAssertTrue(tc, condition->armor[GREYHAWK_REAR] < condition->armor[GREYHAWK_FORE]);
    CuAssertTrue(tc, condition->beam_armor <= VESSEL_MAX_PROTOTYPE_ARMOR);
    CuAssertIntEquals(tc, condition->price,
                      vessel_prototype_price(
                          vessel_type, vessel_class_handling((enum vessel_class)vessel_type)->speed,
                          condition->beam_armor));
  }

  condition = vessel_class_condition(VESSEL_TRANSPORT);
  CuAssertIntEquals(tc, 110, condition->beam_armor);
  CuAssertIntEquals(tc, 27, condition->internal[GREYHAWK_REAR]);
  CuAssertIntEquals(tc, 130, condition->sail);
  CuAssertIntEquals(tc, 24000, condition->price);
}

void Test_vessel_prototype_price_scales_with_armor_and_speed(CuTest *tc)
{
  /* class price * (0.5 + 0.25 * armor / class armor + 0.25 * speed / class speed) */
  CuAssertIntEquals(tc, 44000, vessel_prototype_price(VESSEL_WARSHIP, 17, 109));
  CuAssertIntEquals(tc, 37037, vessel_prototype_price(VESSEL_WARSHIP, 17, 40));
  CuAssertIntEquals(tc, 55000, vessel_prototype_price(VESSEL_WARSHIP, 17, 218));
  CuAssertIntEquals(tc, 22000, vessel_prototype_price(VESSEL_WARSHIP, 0, 0));
  CuAssertIntEquals(tc, 200, vessel_prototype_price(VESSEL_RAFT, 5, 3));
  CuAssertIntEquals(tc, 8000, vessel_prototype_price(99, 20, 66));
}
