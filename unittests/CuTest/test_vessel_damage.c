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
#include "../../src/database/mysql.h"
#include "../../src/magic/spells.h"
#include "../../src/net/protocol.h"
#include "../../src/vessels/vessels.h"

#include <stdlib.h>
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

  /* One breached arc stops her dead at once, not by shedding way; aloft
   * she keeps half speed. */
  ship->rarmor = 0;
  ship->speed = 17.0;
  vessel_damage_hull(NULL, ship, 30, GREYHAWK_REAR, FALSE);
  vessel_update_condition(ship, NULL);
  CuAssertIntEquals(tc, 1, vessel_breached_arcs(ship));
  CuAssertTrue(tc, !vessel_is_sinking(ship));
  CuAssertDblEquals(tc, 0.0, vessel_max_speed(ship), 0.0001);
  CuAssertDblEquals(tc, 0.0, ship->speed, 0.0001);
  ship->z = 100.0;
  ship->speed = 17.0;
  vessel_update_condition(ship, NULL);
  CuAssertDblEquals(tc, 8.5, vessel_max_speed(ship), 0.0001);
  CuAssertDblEquals(tc, 17.0, ship->speed, 0.0001);

  /* A second breach starts the owned hull's 75-150 s sink timer and stops
   * her even aloft. */
  strlcpy(ship->owner, "Mara", sizeof(ship->owner));
  ship->setspeed = 12;
  ship->parmor = 0;
  ship->pinternal = 0;
  vessel_update_condition(ship, NULL);
  CuAssertTrue(tc, vessel_is_sinking(ship));
  CuAssertTrue(tc, ship->sink_ticks >= VESSEL_SINK_TICKS_OWNED_MIN &&
                       ship->sink_ticks <= VESSEL_SINK_TICKS_OWNED_MAX);
  CuAssertIntEquals(tc, 0, ship->setspeed);
  CuAssertDblEquals(tc, 0.0, ship->speed, 0.0001);
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

/* Two ship rooms (bridge, hold) and two characters for the prize rules. */
struct prize_fixture
{
  struct room_data rooms[2];
  struct char_data captain;
  struct player_special_data captain_specials;
  struct char_data hand;
  struct player_special_data hand_specials;
  struct room_data *saved_world;
  room_rnum saved_top_of_world;
};

static struct greyhawk_ship_data *prize_begin(struct prize_fixture *fixture)
{
  struct greyhawk_ship_data *ship;

  memset(fixture, 0, sizeof(*fixture));
  fixture->saved_world = world;
  fixture->saved_top_of_world = top_of_world;
  world = fixture->rooms;
  top_of_world = 1;
  fixture->rooms[0].number = DAMAGE_ROOM_VNUM;
  fixture->rooms[1].number = DAMAGE_ROOM_VNUM + 1;

  ship = damage_warship(DAMAGE_TARGET_SLOT, "TG");
  ship->num_rooms = 2;
  ship->room_vnums[0] = DAMAGE_ROOM_VNUM;
  ship->room_vnums[1] = DAMAGE_ROOM_VNUM + 1;
  ship->bridge_room = DAMAGE_ROOM_VNUM;
  ship->maxspeed = 17;
  ship->position_speed_percent = 100;
  fixture->rooms[0].ship = ship;
  fixture->rooms[1].ship = ship;

  /* The captain stands on the bridge, a hand in the hold. */
  fixture->captain.player_specials = &fixture->captain_specials;
  fixture->captain.player.name = CuMutableString("Mara");
  GET_LEVEL(&fixture->captain) = 20;
  GET_POS(&fixture->captain) = POS_STANDING;
  IN_ROOM(&fixture->captain) = 0;
  fixture->rooms[0].people = &fixture->captain;
  fixture->hand.player_specials = &fixture->hand_specials;
  SET_BIT_AR(MOB_FLAGS(&fixture->hand), MOB_ISNPC);
  GET_POS(&fixture->hand) = POS_STANDING;
  IN_ROOM(&fixture->hand) = 1;
  fixture->rooms[1].people = &fixture->hand;
  return ship;
}

static void prize_end(struct prize_fixture *fixture)
{
  world = fixture->saved_world;
  top_of_world = fixture->saved_top_of_world;
  damage_clear();
}

void Test_vessel_only_a_beaten_prize_can_be_taken(CuTest *tc)
{
  struct prize_fixture fixture;
  struct greyhawk_ship_data *ship;

  /* A sound hull with a hand aboard is no prize, even to her captain. */
  ship = prize_begin(&fixture);
  CuAssertTrue(tc, !vessel_prize_disabled(ship, &fixture.captain));

  /* Holed, immobile, or struck colors make her one. */
  ship->rarmor = ship->rinternal = 0;
  CuAssertTrue(tc, vessel_prize_disabled(ship, &fixture.captain));
  ship->rarmor = 65;
  ship->rinternal = 23;
  ship->mainsail = 0;
  CuAssertTrue(tc, vessel_prize_disabled(ship, &fixture.captain));
  ship->mainsail = 140;
  ship->colors_struck_ticks = 5;
  CuAssertTrue(tc, vessel_prize_disabled(ship, &fixture.captain));
  ship->colors_struck_ticks = 0;

  /* Abandoned at sea: nobody conscious aboard but the claimant. */
  GET_POS(&fixture.hand) = POS_STUNNED;
  CuAssertTrue(tc, vessel_abandoned_at_sea(ship, &fixture.captain));
  CuAssertTrue(tc, vessel_prize_disabled(ship, &fixture.captain));
  CuAssertTrue(tc, !vessel_abandoned_at_sea(ship, NULL));
  GET_POS(&fixture.hand) = POS_STANDING;

  /* The claimant on a sound, manned, unowned hull is refused outright. */
  do_claimship(&fixture.captain, "", 0, 0);
  CuAssertStrEquals(tc, "", ship->owner);

  prize_end(&fixture);
}

void Test_vessel_struck_colors_hold_until_she_moves_or_time_runs_out(CuTest *tc)
{
  struct prize_fixture fixture;
  struct greyhawk_ship_data *ship;
  int ticks;

  /* An unowned hull has no captain to strike her colors. */
  ship = prize_begin(&fixture);
  do_strikecolors(&fixture.captain, "", 0, 0);
  CuAssertTrue(tc, !vessel_colors_struck(ship));

  /* Her owner strikes them only once she is stopped. */
  strlcpy(ship->owner, "Mara", sizeof(ship->owner));
  ship->speed = 4.0;
  do_strikecolors(&fixture.captain, "", 0, 0);
  CuAssertTrue(tc, !vessel_colors_struck(ship));
  ship->speed = 0.0;
  do_strikecolors(&fixture.captain, "", 0, 0);
  CuAssertTrue(tc, vessel_colors_struck(ship));
  CuAssertIntEquals(tc, VESSEL_COLORS_STRUCK_TICKS, ship->colors_struck_ticks);

  /* Ten minutes later they fly again. */
  for (ticks = 0; ticks < VESSEL_COLORS_STRUCK_TICKS; ticks++)
  {
    vessel_damage_tick_one(ship);
  }
  CuAssertTrue(tc, !vessel_colors_struck(ship));

  /* Getting under way hoists them at once. */
  do_strikecolors(&fixture.captain, "", 0, 0);
  CuAssertTrue(tc, vessel_colors_struck(ship));
  ship->speed = 1.0;
  vessel_damage_tick_one(ship);
  CuAssertTrue(tc, !vessel_colors_struck(ship));

  prize_end(&fixture);
}

void Test_vessel_boarding_needs_a_slow_or_beaten_hull(CuTest *tc)
{
  struct prize_fixture fixture;
  struct greyhawk_ship_data *raider;
  struct greyhawk_ship_data *prize;

  /* The captain stands aboard the raider; the prize lies alongside. */
  raider = prize_begin(&fixture);
  prize = damage_warship(DAMAGE_ATTACKER_SLOT, "AT");
  prize->x = 1.0;
  prize->maxspeed = 17;
  prize->position_speed_percent = 100;
  prize->num_rooms = 1;
  prize->room_vnums[0] = DAMAGE_ROOM_VNUM + 1; /* the hand crews her */
  (void)raider;

  prize->speed = 5.0;
  CuAssertTrue(tc, !can_attempt_boarding(&fixture.captain, prize));
  prize->speed = 3.0;
  CuAssertTrue(tc, can_attempt_boarding(&fixture.captain, prize));
  prize->speed = 5.0;
  prize->sarmor = prize->sinternal = 0;
  CuAssertTrue(tc, can_attempt_boarding(&fixture.captain, prize));

  prize_end(&fixture);
}

void Test_vessel_legacy_prototype_armor_rescales_by_class(CuTest *tc)
{
  /* The class beam armor over the old default armor, rounded, at most 229. */
  CuAssertIntEquals(tc, 8, vessel_rescale_legacy_armor(VESSEL_RAFT, 5));
  CuAssertIntEquals(tc, 150, vessel_rescale_legacy_armor(VESSEL_RAFT, 100));
  CuAssertIntEquals(tc, 13, vessel_rescale_legacy_armor(VESSEL_BOAT, 8));
  CuAssertIntEquals(tc, 66, vessel_rescale_legacy_armor(VESSEL_SHIP, 20));
  CuAssertIntEquals(tc, 50, vessel_rescale_legacy_armor(VESSEL_SHIP, 15));
  CuAssertIntEquals(tc, 109, vessel_rescale_legacy_armor(VESSEL_WARSHIP, 40));
  CuAssertIntEquals(tc, 95, vessel_rescale_legacy_armor(VESSEL_WARSHIP, 35));
  CuAssertIntEquals(tc, 63, vessel_rescale_legacy_armor(VESSEL_AIRSHIP, 15));
  CuAssertIntEquals(tc, 84, vessel_rescale_legacy_armor(VESSEL_SUBMARINE, 25));
  CuAssertIntEquals(tc, 110, vessel_rescale_legacy_armor(VESSEL_TRANSPORT, 20));
  CuAssertIntEquals(tc, 153, vessel_rescale_legacy_armor(VESSEL_MAGICAL, 20));
  CuAssertIntEquals(tc, 229, vessel_rescale_legacy_armor(VESSEL_WARSHIP, 200));
  CuAssertIntEquals(tc, 0, vessel_rescale_legacy_armor(VESSEL_WARSHIP, -5));
  CuAssertIntEquals(tc, 50, vessel_rescale_legacy_armor(99, 15));
}

/* A pre-S3 warship of prototype armor 40 with every refit: plating made her
 * arcs 60, reinforcement her structure 45, rigging her speed 15 + 5. */
static void damage_legacy_warship(struct greyhawk_ship_data *ship)
{
  memset(ship, 0, sizeof(*ship));
  ship->vessel_type = VESSEL_WARSHIP;
  ship->maxspeed = 20;
  ship->maxfarmor = ship->maxparmor = ship->maxrarmor = ship->maxsarmor = 60;
  ship->maxfinternal = ship->maxpinternal = ship->maxrinternal = ship->maxsinternal = 45;
  ship->farmor = 30;
  ship->finternal = 30;
  ship->parmor = 0;
  ship->pinternal = 0; /* the old model left a shot-out section afloat */
  ship->rarmor = 60;
  ship->rinternal = 45;
  ship->sarmor = 15;
  ship->sinternal = 20;
  ship->maxmainsail = 20;
  ship->mainsail = 10;
  ship->maxturnrate = 20;
  ship->turnrate = 5;
}

void Test_vessel_legacy_hull_keeps_its_damage_fractions(CuTest *tc)
{
  struct greyhawk_ship_data ship;

  damage_legacy_warship(&ship);
  vessel_convert_legacy_condition(&ship, SHIP_UPGRADE_PLATING | SHIP_UPGRADE_REINFORCED |
                                             SHIP_UPGRADE_RIGGING);

  /* The warship profile at armor 109, each refit a fifth larger. */
  CuAssertIntEquals(tc, 104, ship.maxfarmor);
  CuAssertIntEquals(tc, 130, ship.maxparmor);
  CuAssertIntEquals(tc, 78, ship.maxrarmor);
  CuAssertIntEquals(tc, 130, ship.maxsarmor);
  CuAssertIntEquals(tc, 45, ship.maxfinternal);
  CuAssertIntEquals(tc, 56, ship.maxpinternal);
  CuAssertIntEquals(tc, 27, ship.maxrinternal);
  CuAssertIntEquals(tc, 56, ship.maxsinternal);
  CuAssertIntEquals(tc, 17, ship.maxspeed);
  CuAssertIntEquals(tc, 140, ship.maxmainsail);
  CuAssertIntEquals(tc, VESSEL_RUDDER_MAX, ship.maxturnrate);

  /* Each arc, the sails, and the rudder keep their share. */
  CuAssertIntEquals(tc, 52, ship.farmor);
  CuAssertIntEquals(tc, 30, ship.finternal);
  CuAssertIntEquals(tc, 0, ship.parmor);
  CuAssertIntEquals(tc, 78, ship.rarmor);
  CuAssertIntEquals(tc, 27, ship.rinternal);
  CuAssertIntEquals(tc, 33, ship.sarmor);
  CuAssertIntEquals(tc, 25, ship.sinternal);
  CuAssertIntEquals(tc, 70, ship.mainsail);
  CuAssertIntEquals(tc, 5, ship.turnrate);

  /* The old model had no holes: a shot-out section comes back afloat. */
  CuAssertIntEquals(tc, 1, ship.pinternal);
  CuAssertIntEquals(tc, 0, vessel_breached_arcs(&ship));

  /* Without refits a whole raft of armor 5 becomes a whole raft of 8. */
  memset(&ship, 0, sizeof(ship));
  ship.vessel_type = VESSEL_RAFT;
  ship.maxspeed = 10;
  ship.maxfarmor = ship.farmor = ship.maxparmor = ship.parmor = 5;
  ship.maxrarmor = ship.rarmor = ship.maxsarmor = ship.sarmor = 5;
  ship.maxfinternal = ship.finternal = ship.maxpinternal = ship.pinternal = 12;
  ship.maxrinternal = ship.rinternal = ship.maxsinternal = ship.sinternal = 12;
  ship.maxmainsail = ship.mainsail = 20;
  ship.maxturnrate = ship.turnrate = 20;
  vessel_convert_legacy_condition(&ship, 0);
  CuAssertIntEquals(tc, 5, ship.maxfarmor);
  CuAssertIntEquals(tc, 5, ship.farmor);
  CuAssertIntEquals(tc, 8, ship.maxparmor);
  CuAssertIntEquals(tc, 8, ship.parmor);
  CuAssertIntEquals(tc, 3, ship.rarmor);
  CuAssertIntEquals(tc, 3, ship.finternal);
  CuAssertIntEquals(tc, 3, ship.maxsinternal);
  CuAssertIntEquals(tc, 20, ship.mainsail);
  CuAssertIntEquals(tc, 10, ship.maxspeed);
}

static MYSQL *damage_open_test_database(void)
{
  const char *port_text;
  MYSQL *connection;

  if (getenv("LUMINARI_TEST_MYSQL_HOST") == NULL || getenv("LUMINARI_TEST_MYSQL_USER") == NULL ||
      getenv("LUMINARI_TEST_MYSQL_PASSWORD") == NULL ||
      getenv("LUMINARI_TEST_MYSQL_DATABASE") == NULL)
  {
    return NULL;
  }
  port_text = getenv("LUMINARI_TEST_MYSQL_PORT");
  connection = mysql_init(NULL);
  if (connection == NULL)
  {
    return NULL;
  }
  if (mysql_real_connect(
          connection, getenv("LUMINARI_TEST_MYSQL_HOST"), getenv("LUMINARI_TEST_MYSQL_USER"),
          getenv("LUMINARI_TEST_MYSQL_PASSWORD"), getenv("LUMINARI_TEST_MYSQL_DATABASE"),
          port_text != NULL ? (unsigned int)strtoul(port_text, NULL, 10) : 3306, NULL, 0) == NULL)
  {
    mysql_close(connection);
    return NULL;
  }
  return connection;
}

/* The first row of a one-value query, or -1. */
static int damage_query_int(MYSQL *connection, const char *query)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  int value;

  if (mysql_query(connection, query))
  {
    return -1;
  }
  result = mysql_store_result(connection);
  if (result == NULL)
  {
    return -1;
  }
  row = mysql_fetch_row(result);
  value = row != NULL && row[0] != NULL ? (int)strtol(row[0], NULL, 10) : -1;
  mysql_free_result(result);
  return value;
}

void Test_vessel_prototype_armor_is_rescaled_once(CuTest *tc)
{
  static const int legacy_armor[NUM_VESSEL_TYPES] = {5, 8, 20, 35, 15, 25, 20, 20};
  const char *enabled = getenv("LUMINARI_TEST_MYSQL_ENABLE");
  char query[256];
  MYSQL *saved_conn;
  MYSQL *connection;
  bool saved_mysql_available;
  bool had_base_table;
  bool prepared;
  int vessel_type;

  if (enabled == NULL || strcmp(enabled, "1") != 0)
  {
    return;
  }

  connection = damage_open_test_database();
  if (connection == NULL)
  {
    CuFail(tc, "could not connect to the explicitly configured test database");
    return;
  }

  /* A prototype table from before S3, shadowing any real one. The schema
   * guard's CREATE TABLE IF NOT EXISTS does not see a temporary table, so
   * note whether it will leave a base table behind. */
  had_base_table = damage_query_int(connection, "SELECT COUNT(*) FROM information_schema.TABLES "
                                                "WHERE TABLE_SCHEMA = DATABASE() "
                                                "AND TABLE_NAME = 'ship_prototypes'") > 0;
  prepared = mysql_query(connection, "CREATE TEMPORARY TABLE ship_prototypes ("
                                     "prototype_id INT AUTO_INCREMENT PRIMARY KEY, "
                                     "name VARCHAR(127) NOT NULL, "
                                     "vessel_class INT NOT NULL DEFAULT 2, "
                                     "max_speed INT NOT NULL DEFAULT 10, "
                                     "armor INT NOT NULL DEFAULT 10, "
                                     "for_sale TINYINT(1) NOT NULL DEFAULT 0, "
                                     "min_level INT NOT NULL DEFAULT 0, "
                                     "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP)") == 0;
  for (vessel_type = 0; prepared && vessel_type < NUM_VESSEL_TYPES; vessel_type++)
  {
    snprintf(query, sizeof(query),
             "INSERT INTO ship_prototypes (prototype_id, name, vessel_class, armor) "
             "VALUES (%d, 'Legacy', %d, %d)",
             vessel_type + 1, vessel_type, legacy_armor[vessel_type]);
    prepared = mysql_query(connection, query) == 0;
  }
  if (!prepared)
  {
    mysql_close(connection);
    CuFail(tc, "could not create the isolated ship prototype fixture");
    return;
  }

  saved_conn = conn;
  saved_mysql_available = mysql_available;
  conn = connection;
  mysql_available = TRUE;

  /* The first boot rescales every class; later boots change nothing. */
  CuAssertTrue(tc, vessel_prototype_ensure_schema());
  CuAssertTrue(tc, vessel_prototype_ensure_schema());
  for (vessel_type = 0; vessel_type < NUM_VESSEL_TYPES; vessel_type++)
  {
    snprintf(query, sizeof(query),
             "SELECT armor FROM ship_prototypes WHERE prototype_id = %d AND armor_scale = 1",
             vessel_type + 1);
    CuAssertIntEquals(tc, vessel_rescale_legacy_armor(vessel_type, legacy_armor[vessel_type]),
                      damage_query_int(connection, query));
  }

  /* A prototype written afterwards is already on the new scale. */
  CuAssertIntEquals(tc, 0,
                    mysql_query(connection, "INSERT INTO ship_prototypes (prototype_id, name, "
                                            "vessel_class, armor) VALUES (20, 'New', 3, 109)"));
  CuAssertTrue(tc, vessel_prototype_ensure_schema());
  CuAssertIntEquals(tc, 109,
                    damage_query_int(connection, "SELECT armor FROM ship_prototypes "
                                                 "WHERE prototype_id = 20 AND armor_scale = 1"));

  conn = saved_conn;
  mysql_available = saved_mysql_available;
  mysql_query(connection, "DROP TEMPORARY TABLE ship_prototypes");
  if (!had_base_table)
  {
    mysql_query(connection, "DROP TABLE IF EXISTS ship_prototypes");
  }
  mysql_close(connection);
}

void Test_vessel_legacy_snapshot_converts_once_at_load(CuTest *tc)
{
  const char *enabled = getenv("LUMINARI_TEST_MYSQL_ENABLE");
  struct greyhawk_ship_data *ship;
  struct greyhawk_ship_data legacy;
  char query[1024];
  MYSQL *saved_conn;
  MYSQL *connection;
  bool saved_mysql_available;
  bool prepared;

  if (enabled == NULL || strcmp(enabled, "1") != 0)
  {
    return;
  }

  connection = damage_open_test_database();
  if (connection == NULL)
  {
    CuFail(tc, "could not connect to the explicitly configured test database");
    return;
  }

  /* A snapshot saved before S3, shadowing the real tables. */
  damage_legacy_warship(&legacy);
  snprintf(query, sizeof(query),
           "INSERT INTO ship_runtime_state (ship_id, maxspeed, "
           "maxfarmor, maxrarmor, maxparmor, maxsarmor, farmor, rarmor, parmor, sarmor, "
           "maxfinternal, maxrinternal, maxpinternal, maxsinternal, "
           "finternal, rinternal, pinternal, sinternal, "
           "maxturnrate, turnrate, maxmainsail, mainsail, condition_model) VALUES "
           "(%d, %d, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, "
           "%u, %u, %u, %u, 0)",
           DAMAGE_TARGET_SLOT, legacy.maxspeed, legacy.maxfarmor, legacy.maxrarmor,
           legacy.maxparmor, legacy.maxsarmor, legacy.farmor, legacy.rarmor, legacy.parmor,
           legacy.sarmor, legacy.maxfinternal, legacy.maxrinternal, legacy.maxpinternal,
           legacy.maxsinternal, legacy.finternal, legacy.rinternal, legacy.pinternal,
           legacy.sinternal, legacy.maxturnrate, legacy.turnrate, legacy.maxmainsail,
           legacy.mainsail);
  prepared = mysql_query(connection, "CREATE TEMPORARY TABLE ship_runtime_state "
                                     "(PRIMARY KEY (ship_id)) "
                                     "SELECT * FROM ship_runtime_state LIMIT 0") == 0 &&
             mysql_query(connection, "CREATE TEMPORARY TABLE ship_interiors ("
                                     "ship_id INT NOT NULL PRIMARY KEY, "
                                     "upgrades INT NOT NULL DEFAULT 0)") == 0 &&
             mysql_query(connection, query) == 0;
  snprintf(query, sizeof(query), "INSERT INTO ship_interiors (ship_id, upgrades) VALUES (%d, %d)",
           DAMAGE_TARGET_SLOT,
           SHIP_UPGRADE_PLATING | SHIP_UPGRADE_REINFORCED | SHIP_UPGRADE_RIGGING);
  prepared = prepared && mysql_query(connection, query) == 0;
  if (!prepared)
  {
    mysql_close(connection);
    CuFail(tc, "could not create the isolated runtime snapshot fixture");
    return;
  }

  saved_conn = conn;
  saved_mysql_available = mysql_available;
  conn = connection;
  mysql_available = TRUE;

  /* The load converts her with her refits, as the unit case does. */
  ship = &greyhawk_ships[DAMAGE_TARGET_SLOT];
  memset(ship, 0, sizeof(*ship));
  ship->active = TRUE;
  ship->shipnum = DAMAGE_TARGET_SLOT;
  ship->vessel_type = VESSEL_WARSHIP;
  CuAssertTrue(tc, vessel_db_load_runtime(ship));
  CuAssertIntEquals(tc, 104, ship->maxfarmor);
  CuAssertIntEquals(tc, 52, ship->farmor);
  CuAssertIntEquals(tc, 1, ship->pinternal);
  CuAssertIntEquals(tc, 17, ship->maxspeed);
  CuAssertIntEquals(tc, 70, ship->mainsail);

  /* The save records the model, so the next boot loads her as she is. */
  ship->farmor = 40;
  CuAssertTrue(tc, vessel_db_save_runtime(ship));
  snprintf(query, sizeof(query),
           "SELECT condition_model FROM ship_runtime_state WHERE ship_id = %d", DAMAGE_TARGET_SLOT);
  CuAssertIntEquals(tc, VESSEL_CONDITION_MODEL, damage_query_int(connection, query));
  memset(ship, 0, sizeof(*ship));
  ship->active = TRUE;
  ship->shipnum = DAMAGE_TARGET_SLOT;
  ship->vessel_type = VESSEL_WARSHIP;
  CuAssertTrue(tc, vessel_db_load_runtime(ship));
  CuAssertIntEquals(tc, 104, ship->maxfarmor);
  CuAssertIntEquals(tc, 40, ship->farmor);
  CuAssertIntEquals(tc, 17, ship->maxspeed);

  conn = saved_conn;
  mysql_available = saved_mysql_available;
  damage_clear();
  mysql_query(connection, "DROP TEMPORARY TABLE ship_runtime_state");
  mysql_query(connection, "DROP TEMPORARY TABLE ship_interiors");
  mysql_close(connection);
}

/* Clear the captured output before the next command. */
static void damage_reset_output(struct descriptor_data *descriptor, char *output, size_t size)
{
  memset(output, 0, size);
  descriptor->bufptr = 0;
  descriptor->bufspace = (int)(size - 1);
}

void Test_vessel_status_shows_damage_and_weapons(CuTest *tc)
{
  struct greyhawk_ship_data *ship;
  struct descriptor_data descriptor;
  struct char_data captain;
  struct player_special_data specials;
  char output[4096];

  memset(&captain, 0, sizeof(captain));
  memset(&specials, 0, sizeof(specials));
  memset(&descriptor, 0, sizeof(descriptor));
  captain.player_specials = &specials;
  captain.player.name = CuMutableString("Mara");
  captain.desc = &descriptor;
  descriptor.character = &captain;
  descriptor.output = output;
  descriptor.pProtocol = ProtocolCreate();
  CuAssertPtrNotNull(tc, descriptor.pProtocol);

  /* Holed on two sides and going down, colors struck, weapons knocked about. */
  ship = damage_warship(DAMAGE_TARGET_SLOT, "TG");
  ship->parmor = ship->pinternal = 0;
  ship->rarmor = ship->rinternal = 0;
  ship->sink_ticks = 179;
  ship->colors_struck_ticks = VESSEL_COLORS_STRUCK_TICKS;
  ship->slot[0].damage = 35;
  ship->slot[1].timer = 3;
  ship->slot[2] = ship->slot[0];
  ship->slot[2].position = GREYHAWK_FORE;
  ship->slot[2].damage = VESSEL_WEAPON_DESTROYED;
  strlcpy(ship->slot[2].desc, "the bow chaser", sizeof(ship->slot[2].desc));
  damage_reset_output(&descriptor, output, sizeof(output));
  vessel_show_condition(&captain, ship);
  CuAssertTrue(tc, strstr(output, "Structure: bow 38/38, port 0/47, starboard 47/47, "
                                  "stern 0/23\r\n") != NULL);
  CuAssertTrue(tc, strstr(output, "Sails: 140/140\r\nRudder: 20/20\r\n") != NULL);
  CuAssertTrue(tc, strstr(output, "Holed: port side and stern. SINKING: she goes down in about "
                                  "90 seconds.\r\n") != NULL);
  CuAssertTrue(tc, strstr(output, "Colors: struck, for about 600 more seconds.") != NULL);
  CuAssertTrue(tc, strstr(output, "the port battery (port side): disabled, 35% damaged") != NULL);
  CuAssertTrue(tc, strstr(output, "the starboard battery (starboard side): reloading") != NULL);
  CuAssertTrue(tc, strstr(output, "the bow chaser (bow): destroyed") != NULL);

  /* One hole stops a hull afloat and halves one aloft. */
  ship->sink_ticks = 0;
  ship->colors_struck_ticks = 0;
  ship->rarmor = 65;
  ship->slot[0].damage = 0;
  ship->slot[1].timer = 0;
  damage_reset_output(&descriptor, output, sizeof(output));
  vessel_show_condition(&captain, ship);
  CuAssertTrue(tc, strstr(output, "Holed: port side. She cannot move.\r\n") != NULL);
  CuAssertTrue(tc, strstr(output, "Colors:") == NULL);
  CuAssertTrue(tc, strstr(output, "the port battery (port side): ready") != NULL);
  ship->z = 5.0;
  damage_reset_output(&descriptor, output, sizeof(output));
  vessel_show_condition(&captain, ship);
  CuAssertTrue(tc, strstr(output, "Holed: port side. She makes half speed aloft.") != NULL);

  /* A sound, unarmed hull. */
  vessel_initialize_condition(ship, 109);
  memset(ship->slot, 0, sizeof(ship->slot));
  damage_reset_output(&descriptor, output, sizeof(output));
  vessel_show_condition(&captain, ship);
  CuAssertTrue(tc, strstr(output, "Holed:") == NULL);
  CuAssertTrue(tc, strstr(output, "== Weapons ==\r\nNone mounted.\r\n") != NULL);

  ProtocolDestroy(descriptor.pProtocol);
  damage_clear();
}
