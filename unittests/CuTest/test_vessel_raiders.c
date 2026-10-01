/* NPC raiders and ramming (vessels-ships study S6): the ambush odds and
 * tiers, the raider brains, neutral colors, a dead captain, the despawn
 * watch, NPC merchants that run, and the Duris ram. Launching a raider,
 * boarding, and looting need a booted world; the live raider gate
 * (scripts/vessels/test_vessel_raider_in_game.sh) runs them. */

#include "CuTest.h"

#include "conf.h"
#include "../../src/core/sysdep.h"
#include "../../src/core/structs.h"
#include "../../src/core/utils.h"
#include "../../src/core/comm.h"
#include "../../src/core/db.h"
#include "../../src/core/handler.h"
#include "../../src/database/mysql.h"
#include "../../src/net/protocol.h"
#include "../../src/vessels/vessels.h"
#include "../../src/wilderness/wilderness.h"

#include <stdlib.h>
#include <string.h>

extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];

/* High fleet slots keep these fixtures clear of the rest of the suite. */
#define RAIDER_SHIP 488
#define QUARRY_SHIP 489
#define RAIDER_BRIDGE_VNUM 169990
#define QUARRY_BRIDGE_VNUM 169991

/* Ship slots, a bridge for each, and a captain on the raider's bridge. */
struct raider_sea
{
  struct room_data rooms[2];
  struct index_data captain_index;
  struct char_data captain;
  struct room_data *saved_world;
  struct index_data *saved_mob_index;
  room_rnum saved_top_of_world;
  mob_rnum saved_top_of_mobt;
};

static bool raider_open_water(struct greyhawk_ship_data *ship, int x, int y, int z)
{
  (void)ship;
  (void)y;
  (void)z;
  return x < 100; /* Land lies east of x = 100 */
}

static struct greyhawk_ship_data *raider_hull(int shipnum, const char *name, const char *owner,
                                              double x, double y, double heading)
{
  struct greyhawk_ship_data *ship = &greyhawk_ships[shipnum];

  memset(ship, 0, sizeof(*ship));
  ship->active = TRUE;
  ship->shipnum = shipnum;
  ship->vessel_type = VESSEL_WARSHIP;
  ship->maxspeed = 17;
  ship->position_speed_percent = 100;
  ship->docked_to_ship = -1;
  ship->x = x;
  ship->y = y;
  ship->heading = heading;
  ship->setheading = (short int)heading;
  strlcpy(ship->name, name, sizeof(ship->name));
  strlcpy(ship->owner, owner, sizeof(ship->owner));
  ship->id[0] = 'R';
  ship->id[1] = (char)('A' + shipnum % 26);
  vessel_initialize_condition(ship, vessel_class_condition(VESSEL_WARSHIP)->beam_armor);
  return ship;
}

/* Armor, structure, and sail left on a hull. */
static int raider_hull_points(struct greyhawk_ship_data *ship)
{
  int points;
  int arc;

  points = ship->mainsail;
  for (arc = 0; arc < VESSEL_NUM_ARCS; arc++)
  {
    points += *vessel_arc_armor(ship, arc) + *vessel_arc_internal(ship, arc);
  }
  return points;
}

/* A tier 0 raider at (0, 0) heading north, her captain on the bridge, and a
 * player's warship at (x, y) sailing heading at speed. */
static struct greyhawk_ship_data *raider_sea_begin(struct raider_sea *sea, double x, double y,
                                                   double heading, double speed)
{
  struct greyhawk_ship_data *raider;
  struct greyhawk_ship_data *quarry;

  memset(sea, 0, sizeof(*sea));
  sea->saved_world = world;
  sea->saved_top_of_world = top_of_world;
  sea->saved_mob_index = mob_index;
  sea->saved_top_of_mobt = top_of_mobt;
  sea->rooms[0].number = RAIDER_BRIDGE_VNUM;
  sea->rooms[1].number = QUARRY_BRIDGE_VNUM;
  world = sea->rooms;
  top_of_world = 1;
  sea->captain_index.vnum = VESSEL_RAIDER_CAPTAIN_VNUM;
  mob_index = &sea->captain_index;
  top_of_mobt = 0;
  clear_char(&sea->captain);
  SET_BIT_AR(MOB_FLAGS(&sea->captain), MOB_ISNPC);
  sea->captain.nr = 0;
  GET_POS(&sea->captain) = POS_STANDING;
  IN_ROOM(&sea->captain) = 0;
  sea->rooms[0].people = &sea->captain;
  vessel_movement_set_cell_entry_for_test(raider_open_water);

  raider = raider_hull(RAIDER_SHIP, "the Corsair Frigate", "", 0.0, 0.0, 0.0);
  raider->num_rooms = 1;
  raider->room_vnums[0] = RAIDER_BRIDGE_VNUM;
  raider->bridge_room = RAIDER_BRIDGE_VNUM;
  autopilot_init(raider);
  raider->autopilot->pilot_mob_vnum = VESSEL_RAIDER_CAPTAIN_VNUM;
  raider->raider_mode = VESSEL_RAIDER_ENGAGING;
  raider->raider_target = QUARRY_SHIP;

  quarry = raider_hull(QUARRY_SHIP, "the Tern", "Corr", x, y, heading);
  quarry->num_rooms = 1;
  quarry->room_vnums[0] = QUARRY_BRIDGE_VNUM;
  quarry->bridge_room = QUARRY_BRIDGE_VNUM;
  quarry->speed = speed;
  quarry->setspeed = (short int)speed;
  return raider;
}

static void raider_sea_end(struct raider_sea *sea)
{
  autopilot_cleanup(&greyhawk_ships[RAIDER_SHIP]);
  memset(&greyhawk_ships[RAIDER_SHIP], 0, sizeof(greyhawk_ships[0]));
  memset(&greyhawk_ships[QUARRY_SHIP], 0, sizeof(greyhawk_ships[0]));
  vessel_movement_set_cell_entry_for_test(NULL);
  world = sea->saved_world;
  top_of_world = sea->saved_top_of_world;
  mob_index = sea->saved_mob_index;
  top_of_mobt = sea->saved_top_of_mobt;
}

void Test_vessel_raider_ambush_odds_follow_the_hull(CuTest *tc)
{
  struct raider_sea sea;
  struct greyhawk_ship_data *ship;

  /* A player's warship under way at sea: one ambush in 2002 ticks. */
  raider_sea_begin(&sea, 0.0, 0.0, 0.0, 10.0);
  ship = &greyhawk_ships[QUARRY_SHIP];
  CuAssertIntEquals(tc, VESSEL_RAIDER_AMBUSH_ODDS, vessel_raider_ambush_odds(ship));

  /* A warship may be ambushed again this voyage; a ship may not. */
  ship->raided = TRUE;
  CuAssertIntEquals(tc, VESSEL_RAIDER_AMBUSH_ODDS, vessel_raider_ambush_odds(ship));
  ship->vessel_type = VESSEL_SHIP;
  CuAssertIntEquals(tc, 0, vessel_raider_ambush_odds(ship));
  ship->raided = FALSE;
  CuAssertIntEquals(tc, VESSEL_RAIDER_AMBUSH_ODDS, vessel_raider_ambush_odds(ship));

  /* Neutral colors make it sixty times rarer. */
  ship->slot[5].type = VESSEL_SLOT_EQUIPMENT;
  ship->slot[5].item = VESSEL_EQUIPMENT_COLORS;
  CuAssertIntEquals(tc, VESSEL_RAIDER_AMBUSH_ODDS * 60, vessel_raider_ambush_odds(ship));
  memset(&ship->slot[5], 0, sizeof(ship->slot[5]));

  /* Never a raft or boat, an unowned hull, a stopped or locked one, or one
   * off the surface. */
  ship->vessel_type = VESSEL_BOAT;
  CuAssertIntEquals(tc, 0, vessel_raider_ambush_odds(ship));
  ship->vessel_type = VESSEL_SHIP;
  ship->lock_target = RAIDER_SHIP;
  CuAssertIntEquals(tc, 0, vessel_raider_ambush_odds(ship));
  ship->lock_target = 0;
  ship->z = 5.0;
  CuAssertIntEquals(tc, 0, vessel_raider_ambush_odds(ship));
  ship->z = 0.0;
  ship->speed = 0.0;
  CuAssertIntEquals(tc, 0, vessel_raider_ambush_odds(ship));
  ship->speed = 10.0;
  ship->owner[0] = '\0';
  CuAssertIntEquals(tc, 0, vessel_raider_ambush_odds(ship));

  raider_sea_end(&sea);
}

void Test_vessel_raider_tier_follows_the_quarry(CuTest *tc)
{
  struct greyhawk_ship_data *ship;
  bool hunter;
  int counts[VESSEL_RAIDER_TIERS];
  int unnoticed;
  int tier;
  int i;

  /* A ship (hull 200) of no renown always draws a tier 0 pirate; with 900
   * renown, a tier 1 pirate. */
  ship = raider_hull(QUARRY_SHIP, "the Tern", "Corr", 0.0, 0.0, 0.0);
  ship->vessel_type = VESSEL_SHIP;
  for (i = 0; i < 200; i++)
  {
    CuAssertIntEquals(tc, 0, vessel_raider_pick_tier(ship, &hunter));
    CuAssertTrue(tc, !hunter);
  }
  ship->renown = 900;
  for (i = 0; i < 200; i++)
  {
    CuAssertIntEquals(tc, 1, vessel_raider_pick_tier(ship, &hunter));
    CuAssertTrue(tc, !hunter);
  }
  ship->renown = 0;

  /* A transport (330) draws tier 0 below 250, else tier 1. */
  ship->vessel_type = VESSEL_TRANSPORT;
  memset(counts, 0, sizeof(counts));
  for (i = 0; i < 1000; i++)
  {
    tier = vessel_raider_pick_tier(ship, &hunter);
    CuAssertTrue(tc, tier == 0 || tier == 1);
    CuAssertTrue(tc, !hunter);
    counts[tier]++;
  }
  CuAssertTrue(tc, counts[0] > 600 && counts[1] > 100);

  /* A warship (285) is noticed about one time in seven, and then always by
   * hunters: tier 3 one time in three, else tier 2. */
  ship->vessel_type = VESSEL_WARSHIP;
  memset(counts, 0, sizeof(counts));
  unnoticed = 0;
  for (i = 0; i < 4000; i++)
  {
    tier = vessel_raider_pick_tier(ship, &hunter);
    if (tier < 0)
    {
      unnoticed++;
      continue;
    }
    CuAssertTrue(tc, hunter && (tier == 2 || tier == 3));
    counts[tier]++;
  }
  CuAssertTrue(tc, unnoticed > 3300 && unnoticed < 3550);
  CuAssertTrue(tc, counts[3] > 100 && counts[2] > counts[3]);

  memset(ship, 0, sizeof(*ship));
}

void Test_vessel_raider_basic_ai_works_her_guns(CuTest *tc)
{
  struct raider_sea sea;
  struct greyhawk_ship_data *raider;
  struct greyhawk_ship_data *quarry;

  /* Two rooms ahead of her, the quarry sails east. The starboard ballista
   * is ready inside its band, so she turns it onto her, at full speed. */
  raider = raider_sea_begin(&sea, 0.0, 2.0, 90.0, 10.0);
  quarry = &greyhawk_ships[QUARRY_SHIP];
  vessel_set_weapon(&raider->slot[0], VESSEL_WEAPON_LARGE_BALLISTA, GREYHAWK_STARBOARD);
  vessel_raider_tick_one(raider);
  CuAssertIntEquals(tc, 270, raider->setheading);
  CuAssertIntEquals(tc, 17, raider->setspeed);
  CuAssertIntEquals(tc, QUARRY_SHIP, raider->last_attacker);

  /* Her facing side shot away, she works round to her weakest side still
   * standing (the stern): a point 3 rooms astern of her, at (-3, 2). */
  *vessel_arc_armor(quarry, GREYHAWK_STARBOARD) = 0;
  *vessel_arc_internal(quarry, GREYHAWK_STARBOARD) = 0;
  vessel_raider_tick_one(raider);
  CuAssertIntEquals(tc, 304, raider->setheading);
  vessel_initialize_condition(quarry, vessel_class_condition(VESSEL_WARSHIP)->beam_armor);

  /* A ready catapult too close to fire opens the range. */
  memset(raider->slot, 0, sizeof(raider->slot));
  vessel_set_weapon(&raider->slot[0], VESSEL_WEAPON_SMALL_CATAPULT, GREYHAWK_FORE);
  vessel_raider_tick_one(raider);
  CuAssertIntEquals(tc, 180, raider->setheading);

  /* Out of every band she leads the quarry: between her bearing and her
   * heading. */
  quarry->y = 20.0;
  vessel_raider_tick_one(raider);
  CuAssertIntEquals(tc, 45, raider->setheading);

  /* Half a room off a coast to the east, she swings clear of it and
   * brakes to speed 1. */
  raider->x = 99.0;
  quarry->x = 99.0;
  raider->heading = 90.0;
  vessel_raider_tick_one(raider);
  CuAssertIntEquals(tc, 345, raider->setheading);
  CuAssertIntEquals(tc, 1, raider->setspeed);

  raider_sea_end(&sea);
}

void Test_vessel_raider_advanced_ai_works_round_to_the_weak_side(CuTest *tc)
{
  struct raider_sea sea;
  struct greyhawk_ship_data *raider;
  struct greyhawk_ship_data *quarry;
  int heading;

  /* Five rooms ahead, the quarry sails north with her port side battered:
   * the advanced brain steers for a point 3 rooms off that side of where
   * she will be in 3 seconds, near (-3, 5.4). */
  raider = raider_sea_begin(&sea, 0.0, 5.0, 0.0, 6.0);
  raider->raider_advanced = TRUE;
  quarry = &greyhawk_ships[QUARRY_SHIP];
  vessel_set_weapon(&raider->slot[0], VESSEL_WEAPON_LARGE_BALLISTA, GREYHAWK_STARBOARD);
  vessel_set_weapon(&raider->slot[1], VESSEL_WEAPON_LARGE_BALLISTA, GREYHAWK_PORT);
  *vessel_arc_armor(quarry, GREYHAWK_PORT) = 10;
  vessel_raider_tick_one(raider);
  heading = raider->setheading;
  CuAssertTrue(tc, heading >= 328 && heading <= 334);

  /* Off that side with her starboard battery ready, she turns it on her. */
  raider->x = -2.0;
  raider->y = 5.0;
  raider->heading = 90.0;
  raider->setheading = 90;
  vessel_raider_tick_one(raider);
  CuAssertIntEquals(tc, 0, raider->setheading);

  raider_sea_end(&sea);
}

void Test_vessel_raider_modes_follow_her_state(CuTest *tc)
{
  struct raider_sea sea;
  struct greyhawk_ship_data *raider;
  struct greyhawk_ship_data *quarry;

  /* Out of ammunition she runs from the quarry, the way she came. */
  raider = raider_sea_begin(&sea, 0.0, 5.0, 90.0, 10.0);
  quarry = &greyhawk_ships[QUARRY_SHIP];
  vessel_set_weapon(&raider->slot[0], VESSEL_WEAPON_LARGE_BALLISTA, GREYHAWK_STARBOARD);
  raider->slot[0].ammo = 0;
  vessel_raider_tick_one(raider);
  CuAssertIntEquals(tc, VESSEL_RAIDER_RUNNING, raider->raider_mode);
  vessel_raider_tick_one(raider);
  CuAssertIntEquals(tc, 180, raider->setheading);

  /* Cruising, she takes no quarry under neutral colors, and the nearest
   * player's hull without them. */
  raider->slot[0].ammo = 30;
  raider->raider_mode = VESSEL_RAIDER_CRUISING;
  raider->raider_target = 0;
  quarry->slot[5].type = VESSEL_SLOT_EQUIPMENT;
  quarry->slot[5].item = VESSEL_EQUIPMENT_COLORS;
  vessel_raider_tick_one(raider);
  CuAssertIntEquals(tc, VESSEL_RAIDER_CRUISING, raider->raider_mode);
  CuAssertIntEquals(tc, VESSEL_RAIDER_DESPAWN_TICKS - 1, raider->raider_ticks);
  memset(&quarry->slot[5], 0, sizeof(quarry->slot[5]));
  vessel_raider_tick_one(raider);
  CuAssertIntEquals(tc, VESSEL_RAIDER_ENGAGING, raider->raider_mode);
  CuAssertIntEquals(tc, QUARRY_SHIP, raider->raider_target);
  CuAssertIntEquals(tc, 0, raider->raider_ticks);

  /* Leaving, she stays at sea 20 ticks more while a player's hull watches. */
  raider->raider_mode = VESSEL_RAIDER_LEAVING;
  raider->raider_target = 0;
  raider->raider_ticks = 1;
  vessel_raider_tick_one(raider);
  CuAssertIntEquals(tc, VESSEL_RAIDER_WITNESS_TICKS, raider->raider_ticks);
  CuAssertTrue(tc, is_valid_ship(raider));

  /* With her captain dead nobody gives orders: she heaves to, her guns fall
   * silent, and she counts down to leaving. */
  raider->raider_mode = VESSEL_RAIDER_ENGAGING;
  raider->raider_target = QUARRY_SHIP;
  raider->raider_ticks = 0;
  raider->setspeed = 17;
  sea.rooms[0].people = NULL;
  vessel_raider_tick_one(raider);
  CuAssertIntEquals(tc, 0, raider->setspeed);
  CuAssertIntEquals(tc, 0, raider->raider_target);
  CuAssertIntEquals(tc, -1, raider->autopilot->pilot_mob_vnum);
  CuAssertIntEquals(tc, VESSEL_RAIDER_DESPAWN_TICKS - 1, raider->raider_ticks);

  raider_sea_end(&sea);
}

void Test_vessel_merchant_under_fire_runs(CuTest *tc)
{
  struct raider_sea sea;
  struct greyhawk_ship_data *merchant;

  /* An NPC merchant fired on from five rooms north runs south while her
   * crew is at battle stations, and leaves her autopilot's course alone
   * once they stand down. */
  merchant = raider_sea_begin(&sea, 0.0, 5.0, 0.0, 0.0);
  merchant->raider_mode = 0;
  merchant->merchant_id = 1;
  merchant->last_attacker = QUARRY_SHIP;
  merchant->setheading = 45;
  vessel_raider_tick_one(merchant);
  CuAssertIntEquals(tc, 45, merchant->setheading);
  merchant->battle_ticks = VESSEL_BATTLE_STATIONS_TICKS;
  vessel_raider_tick_one(merchant);
  CuAssertIntEquals(tc, 180, merchant->setheading);
  CuAssertIntEquals(tc, 17, merchant->setspeed);

  raider_sea_end(&sea);
}

void Test_vessel_ram_chance_follows_duris(CuTest *tc)
{
  struct greyhawk_ship_data *ship;
  struct greyhawk_ship_data *target;

  /* A frigate at speed 10 against a stopped caravel: 99%. */
  ship = raider_hull(RAIDER_SHIP, "the Gull", "Corr", 0.0, 0.0, 0.0);
  ship->speed = 10.0;
  target = raider_hull(QUARRY_SHIP, "the Tern", "", 0.0, 0.5, 0.0);
  target->vessel_type = VESSEL_SHIP;
  CuAssertIntEquals(tc, 99, vessel_ram_chance(ship, target));

  /* Against a frigate at full speed, 83%, and 89% with a veteran
   * sailmaster. */
  target->vessel_type = VESSEL_WARSHIP;
  target->speed = 17.0;
  CuAssertIntEquals(tc, 83, vessel_ram_chance(ship, target));
  ship->crew_tier[CREW_SAILMASTER] = CREW_TIER_VETERAN;
  CuAssertIntEquals(tc, 89, vessel_ram_chance(ship, target));

  memset(ship, 0, sizeof(*ship));
  memset(target, 0, sizeof(*target));
}

void Test_vessel_ram_strikes_and_waits(CuTest *tc)
{
  struct raider_sea sea;
  struct greyhawk_ship_data *ship;
  struct greyhawk_ship_data *target;
  int attempt;
  int before;

  /* No ram outside her bow cone, across altitudes, below speed 4, or at a
   * hull running away faster than she closes; the cooldown is untouched. */
  ship = raider_sea_begin(&sea, 0.0, -0.5, 0.0, 0.0);
  sea.rooms[0].people = NULL; /* Nobody aboard to be knocked down */
  target = &greyhawk_ships[QUARRY_SHIP];
  target->vessel_type = VESSEL_SHIP;
  ship->speed = 10.0;
  strlcpy(ship->ram_order, "Corr", sizeof(ship->ram_order));
  CuAssertTrue(tc, !vessel_ram(ship, target));
  CuAssertTrue(tc, !*ship->ram_order);
  target->y = 0.5;
  target->z = 10.0;
  CuAssertTrue(tc, !vessel_ram(ship, target));
  target->z = 0.0;
  ship->speed = 3.0;
  CuAssertTrue(tc, !vessel_ram(ship, target));
  /* That last attempt was on a hostile, which teaches the sailmaster. */
  ship->speed = 10.0;
  target->speed = 12.0;
  ship->crew_tier[CREW_SAILMASTER] = CREW_TIER_GREEN;
  CuAssertTrue(tc, !vessel_ram(ship, target));
  CuAssertIntEquals(tc, 0, ship->ram_ticks);
  CuAssertTrue(tc, ship->crew_xp[CREW_SAILMASTER] >= 1.0);
  ship->crew_tier[CREW_SAILMASTER] = CREW_TIER_NONE;

  /* A stopped caravel is struck (99%): both hulls are hurt, the heavier
   * frigate spins her and both slow to 3, and the frigate waits 100 ticks
   * to ram again with her guns locked for 50. */
  target->speed = 0.0;
  before = raider_hull_points(target);
  for (attempt = 0; attempt < 20 && !vessel_ram(ship, target); attempt++)
  {
    ship->ram_ticks = 0;
  }
  CuAssertTrue(tc, attempt < 20);
  CuAssertTrue(tc, raider_hull_points(target) < before);
  CuAssertTrue(tc, ship->speed <= VESSEL_BOARDING_MAX_SPEED);
  CuAssertIntEquals(tc, VESSEL_RAM_HIT_TICKS, ship->ram_ticks);
  CuAssertIntEquals(tc, VESSEL_RAM_GUN_TICKS, ship->ram_gun_ticks);
  CuAssertIntEquals(tc, RAIDER_SHIP, target->last_attacker);
  CuAssertTrue(tc, vessel_hull_fire_problem(ship) != NULL);

  /* A raft sailing at her is hard to catch (2%): a miss waits 50 ticks. */
  target->vessel_type = VESSEL_RAFT;
  for (attempt = 0; attempt < 50; attempt++)
  {
    ship->speed = 17.0;
    ship->ram_ticks = 0;
    target->heading = 180.0;
    target->speed = 5.0;
    if (!vessel_ram(ship, target))
    {
      break;
    }
  }
  CuAssertTrue(tc, attempt < 50);
  CuAssertIntEquals(tc, VESSEL_RAM_MISS_TICKS, ship->ram_ticks);

  raider_sea_end(&sea);
}

void Test_vessel_ram_tick_rams_the_locked_contact(CuTest *tc)
{
  struct player_special_data specials;
  struct raider_sea sea;
  struct greyhawk_ship_data *ship;
  struct greyhawk_ship_data *target;
  struct char_data *saved_character_list;
  struct char_data orderer;
  int before;

  /* Corr, online, braces her own frigate to ram a locked NPC hull; while
   * the crew is braced the reload waits. */
  ship = raider_sea_begin(&sea, 0.0, 2.0, 0.0, 0.0);
  sea.rooms[0].people = NULL; /* Nobody aboard to be knocked down */
  target = &greyhawk_ships[QUARRY_SHIP];
  target->owner[0] = '\0';
  strlcpy(ship->owner, "Corr", sizeof(ship->owner));
  memset(&orderer, 0, sizeof(orderer));
  memset(&specials, 0, sizeof(specials));
  orderer.player_specials = &specials;
  orderer.player.name = CuMutableString("Corr");
  IN_ROOM(&orderer) = NOWHERE;
  GET_POS(&orderer) = POS_STANDING;
  saved_character_list = character_list;
  orderer.next = character_list;
  character_list = &orderer;
  vessel_set_weapon(&ship->slot[0], VESSEL_WEAPON_LARGE_BALLISTA, GREYHAWK_FORE);
  ship->slot[0].timer = 10;
  ship->speed = 10.0;
  ship->lock_target = QUARRY_SHIP;
  strlcpy(ship->ram_order, "Corr", sizeof(ship->ram_order));
  vessel_reload_tick(ship);
  CuAssertIntEquals(tc, 10, ship->slot[0].timer);
  vessel_ram_tick_one(ship);
  CuAssertTrue(tc, *ship->ram_order);

  /* Her lock now on a hull whose owner is away, the order no longer stands
   * at the impact: the crew stands down and no blow is struck. */
  strlcpy(target->owner, "Tern", sizeof(target->owner));
  target->y = 0.5;
  before = raider_hull_points(target);
  vessel_ram_tick_one(ship);
  CuAssertTrue(tc, !*ship->ram_order);
  CuAssertIntEquals(tc, before, raider_hull_points(target));
  CuAssertIntEquals(tc, 0, ship->ram_ticks);

  /* On the NPC hull she rams within a room. */
  target->owner[0] = '\0';
  strlcpy(ship->ram_order, "Corr", sizeof(ship->ram_order));
  vessel_ram_tick_one(ship);
  CuAssertTrue(tc, !*ship->ram_order);
  CuAssertTrue(tc, ship->ram_ticks > 0);

  /* The order lapses with its giver gone from the game. */
  character_list = saved_character_list;
  ship->ram_ticks = 0;
  ship->speed = 10.0;
  strlcpy(ship->ram_order, "Corr", sizeof(ship->ram_order));
  vessel_ram_tick_one(ship);
  CuAssertTrue(tc, !*ship->ram_order);
  CuAssertIntEquals(tc, 0, ship->ram_ticks);

  /* Slowed to speed 3, the crew stands down. */
  strlcpy(ship->ram_order, "Corr", sizeof(ship->ram_order));
  ship->speed = 3.0;
  vessel_ram_tick_one(ship);
  CuAssertTrue(tc, !*ship->ram_order);

  raider_sea_end(&sea);
}

static MYSQL *raider_open_test_database(void)
{
  const char *port_text;
  MYSQL *connection;

  connection = mysql_init(NULL);
  if (connection == NULL)
  {
    return NULL;
  }
  port_text = getenv("LUMINARI_TEST_MYSQL_PORT");
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

void Test_vessel_raider_prefers_a_prototype_fast_enough(CuTest *tc)
{
  const char *enabled = getenv("LUMINARI_TEST_MYSQL_ENABLE");
  MYSQL *saved_conn;
  MYSQL *connection;
  char name[128];
  bool saved_mysql_available;
  int prototype_id;
  int i;

  if (enabled == NULL || strcmp(enabled, "1") != 0)
  {
    return;
  }
  connection = raider_open_test_database();
  if (connection == NULL)
  {
    CuFail(tc, "could not connect to the explicitly configured test database");
    return;
  }
  if (mysql_query(connection, "CREATE TEMPORARY TABLE ship_prototypes ("
                              "prototype_id INT PRIMARY KEY, name VARCHAR(127) NOT NULL, "
                              "max_speed INT NOT NULL) ENGINE=InnoDB") != 0 ||
      mysql_query(connection, "CREATE TEMPORARY TABLE vessel_raider_tiers ("
                              "tier TINYINT NOT NULL, prototype_id INT NOT NULL, "
                              "PRIMARY KEY (tier, prototype_id)) ENGINE=InnoDB") != 0 ||
      mysql_query(connection, "INSERT INTO ship_prototypes VALUES "
                              "(1, 'Corsair Clipper', 26), (2, 'Corsair Caravel', 16)") != 0 ||
      mysql_query(connection, "INSERT INTO vessel_raider_tiers VALUES (1, 1), (1, 2), (2, 2)") != 0)
  {
    mysql_close(connection);
    CuFail(tc, "could not create the isolated raider tier fixture");
    return;
  }
  saved_conn = conn;
  saved_mysql_available = mysql_available;
  conn = connection;
  mysql_available = TRUE;

  /* Against a quarry of design speed 20, tier 1 sends the clipper (26),
   * never the caravel (16, below 17); tier 2 has only the caravel, so she
   * comes; tier 3 has nothing. */
  for (i = 0; i < 20; i++)
  {
    CuAssertTrue(tc, vessel_raider_pick_prototype(1, 20, &prototype_id, name, sizeof(name)));
    CuAssertIntEquals(tc, 1, prototype_id);
    CuAssertStrEquals(tc, "Corsair Clipper", name);
  }
  CuAssertTrue(tc, vessel_raider_pick_prototype(2, 20, &prototype_id, name, sizeof(name)));
  CuAssertIntEquals(tc, 2, prototype_id);
  CuAssertTrue(tc, !vessel_raider_pick_prototype(3, 20, &prototype_id, name, sizeof(name)));

  conn = saved_conn;
  mysql_available = saved_mysql_available;
  mysql_close(connection);
}

void Test_vessel_raider_ambush_odds_follow_the_waters(CuTest *tc)
{
  const char *enabled = getenv("LUMINARI_TEST_MYSQL_ENABLE");
  struct vertex cove_polygon[] = {{0, 0}, {10, 0}, {10, 10}, {0, 10}, {0, 0}};
  struct vertex territorial_polygon[] = {{20, 0}, {30, 0}, {30, 10}, {20, 10}, {20, 0}};
  struct region_data law_regions[2];
  struct region_data *saved_region_table;
  struct zone_data zone;
  struct zone_data *saved_zone_table;
  struct raider_sea sea;
  struct greyhawk_ship_data *ship;
  region_rnum saved_top_of_region_table;
  zone_rnum saved_top_of_zone_table;
  MYSQL *saved_conn;
  MYSQL *connection;
  bool saved_mysql_available;

  if (enabled == NULL || strcmp(enabled, "1") != 0)
  {
    return;
  }
  connection = raider_open_test_database();
  if (connection == NULL)
  {
    CuFail(tc, "could not connect to the explicitly configured test database");
    return;
  }
  if (mysql_query(connection, "CREATE TEMPORARY TABLE vessel_region_law ("
                              "region_vnum INT PRIMARY KEY, waters_type INT NOT NULL, "
                              "priority INT NOT NULL, bounty_percent INT NOT NULL, "
                              "authority VARCHAR(63) NOT NULL) ENGINE=InnoDB") != 0 ||
      mysql_query(connection, "INSERT INTO vessel_region_law VALUES "
                              "(7100900, 3, 0, 100, ''), (7100901, 1, 0, 100, '')") != 0)
  {
    mysql_close(connection);
    CuFail(tc, "could not create the isolated vessel law fixture");
    return;
  }

  memset(law_regions, 0, sizeof(law_regions));
  memset(&zone, 0, sizeof(zone));
  zone.number = WILD_ZONE_VNUM;
  law_regions[0].vnum = 7100900;
  law_regions[0].name = CuMutableString("Raider Test Cove");
  law_regions[0].region_type = REGION_GEOGRAPHIC;
  law_regions[0].vertices = cove_polygon;
  law_regions[0].num_vertices = 5;
  law_regions[1] = law_regions[0];
  law_regions[1].vnum = 7100901;
  law_regions[1].name = CuMutableString("Raider Test Territory");
  law_regions[1].vertices = territorial_polygon;
  saved_region_table = region_table;
  saved_top_of_region_table = top_of_region_table;
  saved_zone_table = zone_table;
  saved_top_of_zone_table = top_of_zone_table;
  region_table = law_regions;
  top_of_region_table = 1;
  zone_table = &zone;
  top_of_zone_table = 0;
  saved_conn = conn;
  saved_mysql_available = mysql_available;
  conn = connection;
  mysql_available = TRUE;
  raider_sea_begin(&sea, 5.0, 5.0, 0.0, 10.0);
  ship = &greyhawk_ships[QUARRY_SHIP];
  vessel_piracy_clear_laws();
  CuAssertTrue(tc, vessel_piracy_reload_laws());

  /* A pirate cove doubles the chance, territorial waters halve it. */
  CuAssertIntEquals(tc, VESSEL_RAIDER_AMBUSH_ODDS / 2, vessel_raider_ambush_odds(ship));
  ship->x = 25.0;
  CuAssertIntEquals(tc, VESSEL_RAIDER_AMBUSH_ODDS * 2, vessel_raider_ambush_odds(ship));
  ship->x = 40.0;
  CuAssertIntEquals(tc, VESSEL_RAIDER_AMBUSH_ODDS, vessel_raider_ambush_odds(ship));

  raider_sea_end(&sea);
  vessel_piracy_clear_laws();
  region_table = saved_region_table;
  top_of_region_table = saved_top_of_region_table;
  zone_table = saved_zone_table;
  top_of_zone_table = saved_top_of_zone_table;
  conn = saved_conn;
  mysql_available = saved_mysql_available;
  mysql_close(connection);
}
