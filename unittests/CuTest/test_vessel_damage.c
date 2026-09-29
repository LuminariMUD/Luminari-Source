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
#include "../../src/magic/spells.h"
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

extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];

/* High fleet slots keep these fixtures clear of the rest of the suite. */
#define DAMAGE_TARGET_SLOT 480
#define DAMAGE_ATTACKER_SLOT 481
#define DAMAGE_ROOM_VNUM 169960

/* A default warship at (0,0) heading north, with a ballista on each beam. */
static struct greyhawk_ship_data *damage_warship(int slot, const char *id)
{
  struct greyhawk_ship_data *ship = &greyhawk_ships[slot];

  memset(ship, 0, sizeof(*ship));
  ship->active = TRUE;
  ship->shipnum = slot;
  ship->vessel_type = VESSEL_WARSHIP;
  ship->docked_to_ship = -1;
  strlcpy(ship->id, id, sizeof(ship->id));
  strlcpy(ship->name, id, sizeof(ship->name));
  vessel_initialize_condition(ship, 109);
  ship->slot[0].type = 1;
  ship->slot[0].position = GREYHAWK_PORT;
  ship->slot[0].val2 = 2;
  ship->slot[0].val3 = 8;
  strlcpy(ship->slot[0].desc, "the port battery", sizeof(ship->slot[0].desc));
  ship->slot[1] = ship->slot[0];
  ship->slot[1].position = GREYHAWK_STARBOARD;
  strlcpy(ship->slot[1].desc, "the starboard battery", sizeof(ship->slot[1].desc));
  return ship;
}

static void damage_clear(void)
{
  memset(&greyhawk_ships[DAMAGE_TARGET_SLOT], 0, sizeof(greyhawk_ships[DAMAGE_TARGET_SLOT]));
  memset(&greyhawk_ships[DAMAGE_ATTACKER_SLOT], 0, sizeof(greyhawk_ships[DAMAGE_ATTACKER_SLOT]));
}

void Test_vessel_armor_holds_a_hit_unless_it_is_critical(CuTest *tc)
{
  struct greyhawk_ship_data *ship;

  ship = damage_warship(DAMAGE_TARGET_SLOT, "TG");

  /* Armor that holds stops the hit. */
  CuAssertIntEquals(tc, 10, vessel_damage_hull(NULL, ship, 10, GREYHAWK_PORT, FALSE));
  CuAssertIntEquals(tc, 99, ship->parmor);
  CuAssertIntEquals(tc, 47, ship->pinternal);

  /* A confirmed critical carries half the damage past it. */
  vessel_damage_hull(NULL, ship, 10, GREYHAWK_PORT, TRUE);
  CuAssertIntEquals(tc, 89, ship->parmor);
  CuAssertIntEquals(tc, 42, ship->pinternal);

  /* Overkill spills into the structure. */
  ship->parmor = 4;
  vessel_damage_hull(NULL, ship, 10, GREYHAWK_PORT, FALSE);
  CuAssertIntEquals(tc, 0, ship->parmor);
  CuAssertIntEquals(tc, 36, ship->pinternal);

  /* Every hit lands at least one point. */
  vessel_damage_hull(NULL, ship, 0, GREYHAWK_FORE, FALSE);
  CuAssertIntEquals(tc, 86, ship->farmor);

  /* Stern structure hits foul the rudder. */
  ship->rarmor = 0;
  vessel_damage_hull(NULL, ship, 6, GREYHAWK_REAR, FALSE);
  CuAssertIntEquals(tc, 17, ship->rinternal);
  CuAssertIntEquals(tc, VESSEL_RUDDER_MAX - 6, ship->turnrate);

  damage_clear();
}

void Test_vessel_gutted_arc_hits_wreck_its_weapons(CuTest *tc)
{
  struct greyhawk_ship_data *ship;
  int arc;

  /* With no structure left anywhere nothing deflects, and every hit on the
   * gutted port side damages the port battery by five times the damage. */
  ship = damage_warship(DAMAGE_TARGET_SLOT, "TG");
  for (arc = 0; arc < VESSEL_NUM_ARCS; arc++)
  {
    *vessel_arc_armor(ship, arc) = 0;
    *vessel_arc_internal(ship, arc) = 0;
  }
  CuAssertTrue(tc, vessel_weapon_ready(&ship->slot[0]));
  vessel_damage_hull(NULL, ship, 6, GREYHAWK_PORT, FALSE);
  CuAssertIntEquals(tc, 30, ship->slot[0].damage);
  CuAssertTrue(tc, !vessel_weapon_ready(&ship->slot[0]));
  CuAssertIntEquals(tc, 0, ship->slot[1].damage);

  /* Damage accumulates to destruction at 100 and no further; a destroyed
   * weapon is no longer a candidate. */
  vessel_damage_hull(NULL, ship, 20, GREYHAWK_PORT, FALSE);
  CuAssertIntEquals(tc, VESSEL_WEAPON_DESTROYED, ship->slot[0].damage);
  vessel_damage_weapon(NULL, ship, GREYHAWK_PORT, 40);
  CuAssertIntEquals(tc, VESSEL_WEAPON_DESTROYED, ship->slot[0].damage);

  damage_clear();
}

void Test_vessel_sails_take_hits_and_warships_shrug_some_off(CuTest *tc)
{
  struct greyhawk_ship_data *ship;

  ship = damage_warship(DAMAGE_TARGET_SLOT, "TG");
  CuAssertIntEquals(tc, 8, vessel_damage_sail(NULL, ship, 10));
  CuAssertIntEquals(tc, 132, ship->mainsail);
  CuAssertIntEquals(tc, 1, vessel_damage_sail(NULL, ship, 0));

  ship->vessel_type = VESSEL_BOAT;
  CuAssertIntEquals(tc, 10, vessel_damage_sail(NULL, ship, 10));
  ship->mainsail = 3;
  vessel_damage_sail(NULL, ship, 10);
  CuAssertIntEquals(tc, 0, ship->mainsail);

  damage_clear();
}

void Test_vessel_hit_lands_on_the_arc_facing_the_shooter(CuTest *tc)
{
  struct greyhawk_ship_data *target;
  struct greyhawk_ship_data *attacker;
  int dealt;

  /* The shooter lies due east of a target heading north, so every bolt
   * strikes her starboard beam whatever its scatter; with no sail left
   * nothing hits the rigging. */
  target = damage_warship(DAMAGE_TARGET_SLOT, "TG");
  attacker = damage_warship(DAMAGE_ATTACKER_SLOT, "AT");
  attacker->x = 5.0;
  target->mainsail = 0;
  dealt = vessel_resolve_hit(attacker, target, &attacker->slot[1], FALSE);
  CuAssertTrue(tc, dealt >= 2 && dealt <= 16);
  CuAssertIntEquals(tc, 109 - dealt, target->sarmor);
  CuAssertIntEquals(tc, 109, target->parmor);
  CuAssertIntEquals(tc, 87, target->farmor);

  damage_clear();
}

void Test_vessel_critical_threat_follows_armor_pierce(CuTest *tc)
{
  CuAssertIntEquals(tc, 21, vessel_critical_threat(0));
  CuAssertIntEquals(tc, 20, vessel_critical_threat(2));
  CuAssertIntEquals(tc, 20, vessel_critical_threat(3));
  CuAssertIntEquals(tc, 19, vessel_critical_threat(10));
  CuAssertIntEquals(tc, 18, vessel_critical_threat(15));
  CuAssertIntEquals(tc, 19, vessel_critical_threat(vessel_weapon_profile(NULL)->pierce));
}

void Test_vessel_hull_blast_knocks_the_unsure_footed_down(CuTest *tc)
{
  struct greyhawk_ship_data *ship;
  struct room_data deck;
  struct char_data sailor;
  struct char_data bosun;
  struct player_special_data sailor_specials;
  struct player_special_data bosun_specials;
  struct room_data *saved_world;
  room_rnum saved_top_of_world;

  saved_world = world;
  saved_top_of_world = top_of_world;
  memset(&deck, 0, sizeof(deck));
  deck.number = DAMAGE_ROOM_VNUM;
  world = &deck;
  top_of_world = 0;

  ship = damage_warship(DAMAGE_TARGET_SLOT, "TG");
  ship->num_rooms = 1;
  ship->room_vnums[0] = DAMAGE_ROOM_VNUM;

  /* A deckhand who cannot make the Reflex save falls; one who cannot fail
   * it keeps her footing. */
  memset(&sailor, 0, sizeof(sailor));
  memset(&bosun, 0, sizeof(bosun));
  memset(&sailor_specials, 0, sizeof(sailor_specials));
  memset(&bosun_specials, 0, sizeof(bosun_specials));
  sailor.player_specials = &sailor_specials;
  bosun.player_specials = &bosun_specials;
  SET_BIT_AR(MOB_FLAGS(&sailor), MOB_ISNPC);
  SET_BIT_AR(MOB_FLAGS(&bosun), MOB_ISNPC);
  GET_POS(&sailor) = GET_POS(&bosun) = POS_STANDING;
  GET_SAVE(&sailor, SAVING_REFL) = -100;
  GET_SAVE(&bosun, SAVING_REFL) = 100;
  IN_ROOM(&sailor) = IN_ROOM(&bosun) = 0;
  deck.people = &sailor;
  sailor.next_in_room = &bosun;

  vessel_knockdown_aboard(ship);
  CuAssertIntEquals(tc, POS_SITTING, GET_POS(&sailor));
  CuAssertIntEquals(tc, PULSE_VIOLENCE * 2, GET_WAIT_STATE(&sailor));
  CuAssertIntEquals(tc, POS_STANDING, GET_POS(&bosun));

  deck.people = NULL;
  world = saved_world;
  top_of_world = saved_top_of_world;
  damage_clear();
}

void Test_vessel_one_breach_immobilizes_and_two_sink(CuTest *tc)
{
  struct greyhawk_ship_data *ship;

  ship = damage_warship(DAMAGE_TARGET_SLOT, "TG");
  ship->maxspeed = 17;
  ship->position_speed_percent = 100;
  CuAssertDblEquals(tc, 17.0, vessel_max_speed(ship), 0.0001);

  /* One breached arc: dead in the water, or half speed aloft. */
  ship->rarmor = 0;
  vessel_damage_hull(NULL, ship, 30, GREYHAWK_REAR, FALSE);
  CuAssertIntEquals(tc, 1, vessel_breached_arcs(ship));
  CuAssertDblEquals(tc, 0.0, vessel_max_speed(ship), 0.0001);
  ship->z = 100.0;
  CuAssertDblEquals(tc, 8.5, vessel_max_speed(ship), 0.0001);
  ship->z = 0.0;
  vessel_update_condition(ship, NULL);
  CuAssertTrue(tc, !vessel_is_sinking(ship));

  /* A second breach starts the owned hull's 75-150 s sink timer. */
  strlcpy(ship->owner, "Mara", sizeof(ship->owner));
  ship->setspeed = 12;
  ship->parmor = 0;
  ship->pinternal = 0;
  vessel_update_condition(ship, NULL);
  CuAssertTrue(tc, vessel_is_sinking(ship));
  CuAssertTrue(tc, ship->sink_ticks >= VESSEL_SINK_TICKS_OWNED_MIN &&
                       ship->sink_ticks <= VESSEL_SINK_TICKS_OWNED_MAX);
  CuAssertIntEquals(tc, 0, ship->setspeed);
  CuAssertIntEquals(tc, VESSEL_STATUS_SINKING, vessel_status(ship));
  CuAssertDblEquals(tc, 0.0, vessel_max_speed(ship), 0.0001);

  /* Structure gone while the armor holds is crippled, not sinking. */
  ship = damage_warship(DAMAGE_TARGET_SLOT, "TG");
  ship->finternal = ship->pinternal = ship->rinternal = ship->sinternal = 0;
  CuAssertIntEquals(tc, 0, vessel_breached_arcs(ship));
  CuAssertIntEquals(tc, VESSEL_STATUS_CRIPPLED, vessel_status(ship));

  damage_clear();
}

void Test_vessel_cargo_spills_as_crates_that_can_be_salvaged(CuTest *tc)
{
  struct greyhawk_ship_data *wreck;
  struct greyhawk_ship_data *salvor;
  struct room_data sea;
  struct room_data *saved_world;
  struct obj_data *crate;
  room_rnum saved_top_of_world;
  int crates;

  saved_world = world;
  saved_top_of_world = top_of_world;
  memset(&sea, 0, sizeof(sea));
  sea.number = DAMAGE_ROOM_VNUM;
  world = &sea;
  top_of_world = 0;

  /* Half of each lot floats off; a single unit goes down with the hull. */
  wreck = damage_warship(DAMAGE_TARGET_SLOT, "TG");
  wreck->cargo[0].commodity_id = 7;
  wreck->cargo[0].quantity = 9;
  wreck->cargo[1].commodity_id = 8;
  wreck->cargo[1].quantity = 1;
  crates = vessel_spill_cargo(wreck, 0);
  CuAssertIntEquals(tc, 1, crates);
  crate = sea.contents;
  CuAssertPtrNotNull(tc, crate);
  CuAssertTrue(tc, vessel_is_salvage_crate(crate));
  CuAssertIntEquals(tc, 7, GET_OBJ_VAL(crate, 0));
  CuAssertIntEquals(tc, 4, GET_OBJ_VAL(crate, 1));
  CuAssertTrue(tc, !CAN_WEAR(crate, ITEM_WEAR_TAKE));
  CuAssertTrue(tc, OBJ_FLAGGED(crate, ITEM_DECAY));

  /* A salvor hauls the crate into her hold and the crate is gone. */
  salvor = damage_warship(DAMAGE_ATTACKER_SLOT, "AT");
  CuAssertIntEquals(tc, 4, vessel_salvage_crates(salvor, 0));
  CuAssertIntEquals(tc, 7, salvor->cargo[0].commodity_id);
  CuAssertIntEquals(tc, 4, salvor->cargo[0].quantity);
  CuAssertPtrEquals(tc, NULL, sea.contents);
  CuAssertIntEquals(tc, 0, vessel_salvage_crates(salvor, 0));

  world = saved_world;
  top_of_world = saved_top_of_world;
  damage_clear();
}
