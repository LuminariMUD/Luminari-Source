/* Vessel crew (vessels-ships study S5): experience from the tier floor,
 * promotion, the gains of 3.3.5, casualties, and the hiring gate. */

#include "CuTest.h"

#include "conf.h"
#include "../../src/core/sysdep.h"
#include <math.h> /* before utils.h, which defines log() as a macro */
#include "../../src/core/structs.h"
#include "../../src/core/utils.h"
#include "../../src/core/comm.h"
#include "../../src/core/db.h"
#include "../../src/core/interpreter.h"
#include "../../src/database/mysql.h"
#include "../../src/net/protocol.h"
#include "../../src/vessels/vessels.h"

#include <stdlib.h>
#include <string.h>

extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];

/* High fleet slots keep these fixtures clear of the rest of the suite. */
#define CREW_SHIP 483
#define CREW_TARGET 484
#define CREW_DOCK_VNUM 169970
#define CREW_BRIDGE_VNUM 169971

static struct greyhawk_ship_data *crew_warship(int slot, const char *id)
{
  struct greyhawk_ship_data *ship = &greyhawk_ships[slot];

  memset(ship, 0, sizeof(*ship));
  ship->active = TRUE;
  ship->shipnum = slot;
  ship->vessel_type = VESSEL_WARSHIP;
  ship->maxspeed = 17;
  ship->docked_to_ship = -1;
  ship->position_speed_percent = 100;
  strlcpy(ship->id, id, sizeof(ship->id));
  strlcpy(ship->name, id, sizeof(ship->name));
  vessel_initialize_condition(ship, 109);
  return ship;
}

static void crew_sign_on(struct greyhawk_ship_data *ship, int position, int tier)
{
  ship->crew_tier[position] = tier;
  ship->crew_xp[position] = vessel_crew_floor(position, tier);
  vessel_apply_crew_bonuses(ship);
}

static void crew_clear(void)
{
  memset(&greyhawk_ships[CREW_SHIP], 0, sizeof(greyhawk_ships[CREW_SHIP]));
  memset(&greyhawk_ships[CREW_TARGET], 0, sizeof(greyhawk_ships[CREW_TARGET]));
}

void Test_vessel_crew_floors_follow_the_duris_skills(CuTest *tc)
{
  CuAssertDblEquals(tc, 200.0, vessel_crew_floor(CREW_SAILMASTER, CREW_TIER_GREEN), 0.0);
  CuAssertDblEquals(tc, 800.0, vessel_crew_floor(CREW_SAILMASTER, CREW_TIER_ABLE), 0.0);
  CuAssertDblEquals(tc, 2500.0, vessel_crew_floor(CREW_GUNNER, CREW_TIER_VETERAN), 0.0);
  CuAssertDblEquals(tc, 900.0, vessel_crew_floor(CREW_BOSUN, CREW_TIER_ABLE), 0.0);
  CuAssertDblEquals(tc, 2000.0, vessel_crew_floor(CREW_QUARTERMASTER, CREW_TIER_VETERAN), 0.0);
  CuAssertDblEquals(tc, 0.0, vessel_crew_floor(CREW_GUNNER, CREW_TIER_NONE), 0.0);
  CuAssertDblEquals(tc, 0.0, vessel_crew_floor(NUM_CREW_POSITIONS, CREW_TIER_GREEN), 0.0);
}

void Test_vessel_crew_earns_promotion_at_the_next_floor(CuTest *tc)
{
  struct greyhawk_ship_data *ship;

  ship = crew_warship(CREW_SHIP, "CA");
  crew_sign_on(ship, CREW_GUNNER, CREW_TIER_GREEN);
  CuAssertIntEquals(tc, 2, ship->guncrew.gunadjust);

  /* Just short of the able floor she stays green; reaching it promotes her. */
  vessel_crew_gain(ship, CREW_GUNNER, 749.0);
  CuAssertIntEquals(tc, CREW_TIER_GREEN, ship->crew_tier[CREW_GUNNER]);
  vessel_crew_gain(ship, CREW_GUNNER, 1.0);
  CuAssertIntEquals(tc, CREW_TIER_ABLE, ship->crew_tier[CREW_GUNNER]);
  CuAssertDblEquals(tc, 1000.0, ship->crew_xp[CREW_GUNNER], 0.0001);
  CuAssertIntEquals(tc, 4, ship->guncrew.gunadjust);

  /* One large gain can carry a green hand past both floors. */
  crew_sign_on(ship, CREW_BOSUN, CREW_TIER_GREEN);
  vessel_crew_gain(ship, CREW_BOSUN, 5000.0);
  CuAssertIntEquals(tc, CREW_TIER_VETERAN, ship->crew_tier[CREW_BOSUN]);

  /* An empty position learns nothing. */
  vessel_crew_gain(ship, CREW_SAILMASTER, 5000.0);
  CuAssertIntEquals(tc, CREW_TIER_NONE, ship->crew_tier[CREW_SAILMASTER]);
  CuAssertDblEquals(tc, 0.0, ship->crew_xp[CREW_SAILMASTER], 0.0);

  crew_clear();
}

void Test_vessel_crew_learns_from_kills_and_sales(CuTest *tc)
{
  struct greyhawk_ship_data *victor;
  struct greyhawk_ship_data *target;

  victor = crew_warship(CREW_SHIP, "CA");
  target = crew_warship(CREW_TARGET, "CB");
  crew_sign_on(victor, CREW_SAILMASTER, CREW_TIER_GREEN);
  crew_sign_on(victor, CREW_QUARTERMASTER, CREW_TIER_GREEN);

  /* Sinking an NPC frigate teaches every hand a tenth of her hull weight
   * (285); a player's frigate teaches all of it. */
  vessel_crew_credit_kill(victor, target);
  CuAssertDblEquals(tc, 228.5, victor->crew_xp[CREW_SAILMASTER], 0.0001);
  strlcpy(target->owner, "Mira", sizeof(target->owner));
  vessel_crew_credit_kill(victor, target);
  CuAssertDblEquals(tc, 513.5, victor->crew_xp[CREW_SAILMASTER], 0.0001);
  CuAssertDblEquals(tc, 513.5, victor->crew_xp[CREW_QUARTERMASTER], 0.0001);
  CuAssertDblEquals(tc, 0.0, victor->crew_xp[CREW_GUNNER], 0.0);

  /* 4,000 gold of cargo sold: the sailmaster and quartermaster 3 points
   * each, the bosun 1. */
  crew_sign_on(victor, CREW_BOSUN, CREW_TIER_GREEN);
  vessel_crew_sale_gain(victor, 4000);
  CuAssertDblEquals(tc, 516.5, victor->crew_xp[CREW_SAILMASTER], 0.0001);
  CuAssertDblEquals(tc, 516.5, victor->crew_xp[CREW_QUARTERMASTER], 0.0001);
  CuAssertDblEquals(tc, 221.0, victor->crew_xp[CREW_BOSUN], 0.0001);

  crew_clear();
}

void Test_vessel_gunner_learns_from_reloads_and_shots_at_a_lock(CuTest *tc)
{
  struct greyhawk_ship_data *ship;
  struct greyhawk_ship_data *target;
  struct room_data sea;
  struct room_data *saved_world;
  room_rnum saved_top_of_world;

  memset(&sea, 0, sizeof(sea));
  sea.number = CREW_DOCK_VNUM;
  saved_world = world;
  saved_top_of_world = top_of_world;
  world = &sea;
  top_of_world = 0;

  /* The target lies 8 rooms off the port beam, inside a large ballista's
   * band. */
  ship = crew_warship(CREW_SHIP, "CA");
  target = crew_warship(CREW_TARGET, "CB");
  target->x = -8.0;
  vessel_set_weapon(&ship->slot[0], VESSEL_WEAPON_LARGE_BALLISTA, GREYHAWK_PORT);
  crew_sign_on(ship, CREW_GUNNER, CREW_TIER_GREEN);

  /* Reloading teaches nothing without a lock. */
  ship->slot[0].timer = 5;
  vessel_gunnery_tick_one(ship);
  CuAssertDblEquals(tc, 250.0, ship->crew_xp[CREW_GUNNER], 0.0);

  /* With the target locked, each reload tick teaches 0.0015 and each shot
   * 0.1. */
  ship->lock_target = CREW_TARGET;
  vessel_gunnery_tick_one(ship);
  CuAssertDblEquals(tc, 250.0015, ship->crew_xp[CREW_GUNNER], 0.000001);
  ship->slot[0].timer = 0;
  vessel_fire_weapon(ship, 0, target, NULL);
  CuAssertDblEquals(tc, 250.1015, ship->crew_xp[CREW_GUNNER], 0.000001);

  world = saved_world;
  top_of_world = saved_top_of_world;
  crew_clear();
}

void Test_vessel_crew_casualties_cost_experience_and_tiers(CuTest *tc)
{
  struct greyhawk_ship_data *ship;

  ship = crew_warship(CREW_SHIP, "CA");
  crew_sign_on(ship, CREW_SAILMASTER, CREW_TIER_VETERAN);
  crew_sign_on(ship, CREW_GUNNER, CREW_TIER_GREEN);
  crew_sign_on(ship, CREW_BOSUN, CREW_TIER_ABLE);
  ship->crew_xp[CREW_BOSUN] = 1500.0;

  /* A 10% loss drops the fresh veteran below her floor to able, costs the
   * seasoned bosun 150 points without a demotion, and leaves the green
   * gunner at the green floor. */
  vessel_crew_casualties(ship, 10.0);
  CuAssertIntEquals(tc, CREW_TIER_ABLE, ship->crew_tier[CREW_SAILMASTER]);
  CuAssertDblEquals(tc, 1800.0, ship->crew_xp[CREW_SAILMASTER], 0.0001);
  CuAssertIntEquals(tc, CREW_TIER_ABLE, ship->crew_tier[CREW_BOSUN]);
  CuAssertDblEquals(tc, 1350.0, ship->crew_xp[CREW_BOSUN], 0.0001);
  CuAssertIntEquals(tc, CREW_TIER_GREEN, ship->crew_tier[CREW_GUNNER]);
  CuAssertDblEquals(tc, 250.0, ship->crew_xp[CREW_GUNNER], 0.0001);
  CuAssertIntEquals(tc, CREW_TIER_NONE, ship->crew_tier[CREW_QUARTERMASTER]);

  crew_clear();
}

struct crew_harbor
{
  struct room_data rooms[2]; /* the dock, then the bridge */
  struct obj_data hull;
  struct char_data captain;
  struct player_special_data specials;
  struct descriptor_data descriptor;
  char output[8192];
  struct room_data *saved_world;
  room_rnum saved_top_of_world;
};

/* A warship berthed at a dock with her owner on the bridge. */
static struct greyhawk_ship_data *crew_harbor_begin(CuTest *tc, struct crew_harbor *harbor,
                                                    int level)
{
  struct greyhawk_ship_data *ship;

  memset(harbor, 0, sizeof(*harbor));
  harbor->saved_world = world;
  harbor->saved_top_of_world = top_of_world;
  harbor->rooms[0].number = CREW_DOCK_VNUM;
  SET_BIT_AR(harbor->rooms[0].room_flags, ROOM_DOCKABLE);
  harbor->rooms[1].number = CREW_BRIDGE_VNUM;
  world = harbor->rooms;
  top_of_world = 1;

  harbor->captain.player_specials = &harbor->specials;
  harbor->captain.player.name = CuMutableString("Corr");
  GET_LEVEL(&harbor->captain) = level;
  GET_POS(&harbor->captain) = POS_STANDING;
  IN_ROOM(&harbor->captain) = 1;
  GET_GOLD(&harbor->captain) = 100000;
  harbor->captain.desc = &harbor->descriptor;
  harbor->descriptor.character = &harbor->captain;
  harbor->descriptor.output = harbor->output;
  harbor->descriptor.bufspace = sizeof(harbor->output) - 1;
  harbor->descriptor.pProtocol = ProtocolCreate();
  CuAssertPtrNotNull(tc, harbor->descriptor.pProtocol);

  ship = crew_warship(CREW_SHIP, "CA");
  strlcpy(ship->owner, "Corr", sizeof(ship->owner));
  ship->num_rooms = 1;
  ship->room_vnums[0] = CREW_BRIDGE_VNUM;
  ship->bridge_room = CREW_BRIDGE_VNUM;
  ship->dock = CREW_DOCK_VNUM;
  ship->shipobj = &harbor->hull;
  IN_ROOM(&harbor->hull) = 0;
  harbor->rooms[1].ship = ship;
  harbor->rooms[1].people = &harbor->captain;
  return ship;
}

static const char *crew_harbor_command(struct crew_harbor *harbor, ACMD_DECL((*command)),
                                       const char *argument)
{
  memset(harbor->output, 0, sizeof(harbor->output));
  harbor->descriptor.bufptr = 0;
  harbor->descriptor.bufspace = sizeof(harbor->output) - 1;
  GET_WAIT_STATE(&harbor->captain) = 0;
  command(&harbor->captain, argument, 0, 0);
  return harbor->output;
}

static void crew_harbor_end(struct crew_harbor *harbor)
{
  ProtocolDestroy(harbor->descriptor.pProtocol);
  world = harbor->saved_world;
  top_of_world = harbor->saved_top_of_world;
  crew_clear();
}

void Test_vessel_hire_takes_green_hands_at_their_floor(CuTest *tc)
{
  struct crew_harbor harbor;
  struct greyhawk_ship_data *ship;
  const char *output;

  ship = crew_harbor_begin(tc, &harbor, 20);

  /* Without renown only green hands sign on, and nothing is charged. */
  output = crew_harbor_command(&harbor, do_shiphire, "gunner able");
  CuAssertTrue(tc, strstr(output, "No able gunner will sign on with a hull of less than 700 "
                                  "renown") != NULL);
  CuAssertIntEquals(tc, CREW_TIER_NONE, ship->crew_tier[CREW_GUNNER]);
  CuAssertIntEquals(tc, 100000, GET_GOLD(&harbor.captain));

  /* A green hand starts at the green floor. */
  output = crew_harbor_command(&harbor, do_shiphire, "gunner green");
  CuAssertTrue(tc, strstr(output, "You sign on a green gunner for 2400 gold") != NULL);
  CuAssertIntEquals(tc, CREW_TIER_GREEN, ship->crew_tier[CREW_GUNNER]);
  CuAssertDblEquals(tc, 250.0, ship->crew_xp[CREW_GUNNER], 0.0);
  CuAssertIntEquals(tc, 97600, GET_GOLD(&harbor.captain));

  /* shipcrew shows the experience and the next floor. */
  output = crew_harbor_command(&harbor, do_shipcrew, "");
  CuAssertTrue(tc, strstr(output, "gunner         green, 250 experience (able at 1000)") != NULL);

  /* Dismissal clears the experience; staff may still hire a veteran. */
  crew_harbor_command(&harbor, do_shipdismiss, "gunner");
  CuAssertDblEquals(tc, 0.0, ship->crew_xp[CREW_GUNNER], 0.0);
  GET_LEVEL(&harbor.captain) = LVL_IMMORT;
  crew_harbor_command(&harbor, do_shiphire, "gunner veteran");
  CuAssertIntEquals(tc, CREW_TIER_VETERAN, ship->crew_tier[CREW_GUNNER]);
  CuAssertDblEquals(tc, 2500.0, ship->crew_xp[CREW_GUNNER], 0.0);

  crew_harbor_end(&harbor);
}

static MYSQL *crew_open_test_database(void)
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

void Test_vessel_crew_experience_survives_a_restart(CuTest *tc)
{
  const char *enabled = getenv("LUMINARI_TEST_MYSQL_ENABLE");
  struct greyhawk_ship_data *ship;
  MYSQL *saved_conn;
  MYSQL *connection;
  bool saved_mysql_available;

  if (enabled == NULL || strcmp(enabled, "1") != 0)
  {
    return;
  }

  connection = crew_open_test_database();
  if (connection == NULL)
  {
    CuFail(tc, "could not connect to the explicitly configured test database");
    return;
  }
  if (mysql_query(connection, "CREATE TEMPORARY TABLE ship_crew_roster ("
                              "roster_id INT NOT NULL AUTO_INCREMENT PRIMARY KEY, "
                              "ship_id INT NOT NULL, npc_vnum INT NOT NULL, "
                              "npc_name VARCHAR(100), crew_role VARCHAR(16), "
                              "loyalty_rating INT DEFAULT 50, "
                              "experience DOUBLE NOT NULL DEFAULT 0)") != 0)
  {
    mysql_close(connection);
    CuFail(tc, "could not create the isolated crew roster fixture");
    return;
  }

  saved_conn = conn;
  saved_mysql_available = mysql_available;
  conn = connection;
  mysql_available = TRUE;

  ship = crew_warship(CREW_SHIP, "CA");
  crew_sign_on(ship, CREW_GUNNER, CREW_TIER_VETERAN);
  ship->crew_xp[CREW_GUNNER] = 2612.25;
  crew_sign_on(ship, CREW_BOSUN, CREW_TIER_GREEN);
  vessel_db_save_crew(ship);

  crew_warship(CREW_SHIP, "CA");
  vessel_db_load_crew(ship);
  CuAssertIntEquals(tc, CREW_TIER_VETERAN, ship->crew_tier[CREW_GUNNER]);
  CuAssertDblEquals(tc, 2612.25, ship->crew_xp[CREW_GUNNER], 0.0001);
  CuAssertIntEquals(tc, CREW_TIER_GREEN, ship->crew_tier[CREW_BOSUN]);
  CuAssertDblEquals(tc, 220.0, ship->crew_xp[CREW_BOSUN], 0.0001);
  CuAssertIntEquals(tc, 6, ship->guncrew.gunadjust);

  /* A hand saved before S5 has no experience and starts at the floor. */
  CuAssertIntEquals(tc, 0,
                    mysql_query(connection, "UPDATE ship_crew_roster SET experience = 0 "
                                            "WHERE npc_name = 'gunner'"));
  crew_warship(CREW_SHIP, "CA");
  vessel_db_load_crew(ship);
  CuAssertDblEquals(tc, 2500.0, ship->crew_xp[CREW_GUNNER], 0.0001);

  conn = saved_conn;
  mysql_available = saved_mysql_available;
  mysql_close(connection);
  crew_clear();
}

void Test_vessel_stamina_grows_with_the_crew_and_rests_in_port(CuTest *tc)
{
  struct greyhawk_ship_data *ship;

  ship = crew_warship(CREW_SHIP, "CA");
  CuAssertIntEquals(tc, 500, vessel_stamina_max(ship));
  crew_sign_on(ship, CREW_GUNNER, CREW_TIER_ABLE);
  crew_sign_on(ship, CREW_BOSUN, CREW_TIER_GREEN);
  CuAssertIntEquals(tc, 800, vessel_stamina_max(ship));

  /* The crew works at full effort until its stamina runs out; a deficit of
   * three times the maximum halves it. */
  ship->stamina_spent = 800.0;
  CuAssertDblEquals(tc, 1.0, vessel_stamina_modifier(ship), 0.0);
  ship->stamina_spent = 800.0 + 2400.0;
  CuAssertDblEquals(tc, 0.5, vessel_stamina_modifier(ship), 0.0001);
  CuAssertDblEquals(tc, 0.5 * 1.5, vessel_acceleration(ship), 0.0001);

  /* It rests 1.5 a tick at sea and four times that at anchor, never past
   * full. */
  ship->stamina_spent = 10.0;
  vessel_crew_tick_one(ship);
  CuAssertDblEquals(tc, 8.5, ship->stamina_spent, 0.0001);
  ship->anchored = TRUE;
  vessel_crew_tick_one(ship);
  CuAssertDblEquals(tc, 2.5, ship->stamina_spent, 0.0001);
  vessel_crew_tick_one(ship);
  CuAssertDblEquals(tc, 0.0, ship->stamina_spent, 0.0);

  crew_clear();
}

void Test_vessel_helm_and_guns_tire_the_crew(CuTest *tc)
{
  struct greyhawk_ship_data *ship;
  struct greyhawk_ship_data *target;
  struct room_data sea;
  struct room_data *saved_world;
  room_rnum saved_top_of_world;
  double effort;
  double before;

  memset(&sea, 0, sizeof(sea));
  sea.number = CREW_DOCK_VNUM;
  saved_world = world;
  saved_top_of_world = top_of_world;
  world = &sea;
  top_of_world = 0;
  ship = crew_warship(CREW_SHIP, "CA");
  target = crew_warship(CREW_TARGET, "CB");
  target->x = -8.0;
  effort = sqrt(285.0) / 10.0;

  /* A full tick of acceleration costs half of Duris's (2 + effort) a second. */
  ship->setspeed = 17;
  vessel_sail_tick(ship, 17.0, vessel_open_water, NULL, NULL);
  CuAssertDblEquals(tc, 1.5, ship->speed, 0.0001);
  CuAssertDblEquals(tc, (2.0 + effort) / 2.0, ship->stamina_spent, 0.0001);

  /* Coming about costs its share of the class turn times (3 + effort). */
  ship->stamina_spent = 0.0;
  ship->setspeed = 0;
  ship->speed = 0.0;
  ship->setheading = 90;
  vessel_sail_tick(ship, 17.0, vessel_open_water, NULL, NULL);
  CuAssertTrue(tc, ship->heading > 0.0);
  CuAssertDblEquals(tc, ship->heading / 4.0 * (3.0 + effort) / 2.0, ship->stamina_spent, 0.0001);

  /* A shot costs the weapon's weight over the effort, a tick of its reload a
   * twentieth of that, and a tired crew reloads more slowly. */
  vessel_set_weapon(&ship->slot[0], VESSEL_WEAPON_LARGE_BALLISTA, GREYHAWK_PORT);
  ship->stamina_spent = 0.0;
  vessel_fire_weapon(ship, 0, target, NULL);
  CuAssertIntEquals(tc, 34, ship->slot[0].timer);
  CuAssertDblEquals(tc, 10.0 / effort, ship->stamina_spent, 0.0001);
  before = ship->stamina_spent;
  vessel_reload_tick(ship);
  CuAssertDblEquals(tc, before + 10.0 / effort / 20.0, ship->stamina_spent, 0.0001);
  ship->stamina_spent = 500.0 + 1500.0;
  ship->slot[0].timer = 0;
  vessel_fire_weapon(ship, 0, target, NULL);
  CuAssertIntEquals(tc, 68, ship->slot[0].timer);

  world = saved_world;
  top_of_world = saved_top_of_world;
  crew_clear();
}

void Test_vessel_crew_repairs_at_sea_only_to_their_caps(CuTest *tc)
{
  struct greyhawk_ship_data *ship;
  int tick;

  /* A frigate at anchor with a green bosun: port side holed, sails at 50,
   * rudder smashed, a gun damaged, armor gone on the bow. */
  ship = crew_warship(CREW_SHIP, "CA");
  crew_sign_on(ship, CREW_BOSUN, CREW_TIER_GREEN);
  vessel_set_weapon(&ship->slot[0], VESSEL_WEAPON_LARGE_BALLISTA, GREYHAWK_PORT);
  ship->slot[0].damage = 30;
  ship->parmor = 0;
  ship->pinternal = 0;
  ship->farmor = 0;
  ship->mainsail = 50;
  ship->turnrate = 0;
  ship->anchored = TRUE;
  CuAssertIntEquals(tc, 285, vessel_repair_stock(ship));

  for (tick = 0; tick < 4000; tick++)
  {
    vessel_repair_tick_one(ship);
  }

  /* The bosun's 0.15 caps structure below 25% (12 of 47) and the rigging
   * below 55% (77 of 140 sail, 11 of 20 rudder); the gun is mended; armor is
   * never repaired at sea. */
  CuAssertIntEquals(tc, 12, ship->pinternal);
  CuAssertIntEquals(tc, 77, ship->mainsail);
  CuAssertIntEquals(tc, 11, ship->turnrate);
  CuAssertIntEquals(tc, 0, ship->slot[0].damage);
  CuAssertIntEquals(tc, 0, ship->farmor);
  CuAssertIntEquals(tc, 0, ship->parmor);
  CuAssertTrue(tc, 285 - vessel_repair_stock(ship) >= 12 + 27 + 11);
  CuAssertTrue(tc, ship->stamina_spent > 0.0);
  CuAssertTrue(tc, ship->crew_xp[CREW_BOSUN] > 220.0);

  /* Under way a shot-away sail cannot be mended, and without stores
   * nothing is. */
  ship->anchored = FALSE;
  ship->mainsail = 0;
  ship->pinternal = 0;
  for (tick = 0; tick < 200; tick++)
  {
    vessel_repair_tick_one(ship);
  }
  CuAssertIntEquals(tc, 0, ship->mainsail);
  CuAssertIntEquals(tc, 0, ship->pinternal);
  ship->anchored = TRUE;
  ship->repair_used = 285;
  for (tick = 0; tick < 200; tick++)
  {
    vessel_repair_tick_one(ship);
  }
  CuAssertIntEquals(tc, 0, ship->mainsail);

  /* The stores refill when she berths. */
  ship->anchored = FALSE;
  vessel_berth(ship);
  CuAssertIntEquals(tc, 285, vessel_repair_stock(ship));

  crew_clear();
}

void Test_vessel_character_repair_patches_one_point_at_sea(CuTest *tc)
{
  struct crew_harbor harbor;
  struct greyhawk_ship_data *ship;
  const char *output;
  int attempt;

  /* At sea: no berth. The weakest structure below the cap takes the patch. */
  ship = crew_harbor_begin(tc, &harbor, 20);
  ship->dock = 0;
  REMOVE_BIT_AR(harbor.rooms[0].room_flags, ROOM_DOCKABLE);
  ship->farmor = 0;
  output = crew_harbor_command(&harbor, do_shiprepair, "");
  CuAssertTrue(tc,
               strstr(output, "Nothing aboard needs a patch the stores can make at sea") != NULL);

  ship->pinternal = 3;
  ship->sinternal = 2;
  output = "";
  for (attempt = 0; attempt < 200 && strstr(output, "You patch the timbers") == NULL; attempt++)
  {
    output = crew_harbor_command(&harbor, do_shiprepair, "");
  }
  CuAssertTrue(tc, strstr(output, "You patch the timbers") != NULL);
  CuAssertIntEquals(tc, 3, ship->sinternal);
  CuAssertIntEquals(tc, 3, ship->pinternal);
  CuAssertIntEquals(tc, 284, vessel_repair_stock(ship));
  CuAssertIntEquals(tc, PULSE_VIOLENCE * 2, GET_WAIT_STATE(&harbor.captain));

  ship->repair_used = 285;
  output = crew_harbor_command(&harbor, do_shiprepair, "");
  CuAssertTrue(tc, strstr(output, "The repair stores are spent") != NULL);

  crew_harbor_end(&harbor);
}

void Test_vessel_shipwrights_price_dock_repairs(CuTest *tc)
{
  struct crew_harbor harbor;
  struct greyhawk_ship_data *ship;
  const char *output;

  ship = crew_harbor_begin(tc, &harbor, 20);
  ship->parmor = 89;    /* 20 armor points */
  ship->sinternal = 37; /* 10 structure points */
  ship->mainsail = 130; /* 10 sail points */
  ship->turnrate = 15;  /* 5 rudder points */
  vessel_set_weapon(&ship->slot[0], VESSEL_WEAPON_LARGE_BALLISTA, GREYHAWK_PORT);
  vessel_set_weapon(&ship->slot[1], VESSEL_WEAPON_LARGE_BALLISTA, GREYHAWK_STARBOARD);
  ship->slot[0].damage = 30;
  ship->slot[1].damage = VESSEL_WEAPON_DESTROYED;

  output = crew_harbor_command(&harbor, do_shiprepair, "");
  CuAssertTrue(tc, strstr(output, "armor        20 points       40 gold,   95 seconds") != NULL);
  CuAssertTrue(tc, strstr(output, "weapons       2 weapons     560 gold,  225 seconds") != NULL);

  /* Armor at 2 gold a point, 75 s plus a second a point. */
  output = crew_harbor_command(&harbor, do_shiprepair, "armor");
  CuAssertTrue(tc, strstr(output, "You pay 40 gold") != NULL);
  CuAssertIntEquals(tc, 109, ship->parmor);
  CuAssertIntEquals(tc, 37, ship->sinternal);
  CuAssertIntEquals(tc, 190, ship->maintenance_ticks);
  CuAssertIntEquals(tc, 99960, GET_GOLD(&harbor.captain));

  /* Everything else in one order: structure 20, sails 40, rudder 20, the
   * damaged gun 60 and the destroyed one half its 1,000 price. */
  output = crew_harbor_command(&harbor, do_shiprepair, "all");
  CuAssertTrue(tc, strstr(output, "You pay 640 gold") != NULL);
  CuAssertIntEquals(tc, 47, ship->sinternal);
  CuAssertIntEquals(tc, 140, ship->mainsail);
  CuAssertIntEquals(tc, 20, ship->turnrate);
  CuAssertIntEquals(tc, 0, ship->slot[0].damage);
  CuAssertIntEquals(tc, 0, ship->slot[1].damage);
  CuAssertIntEquals(tc, 190 + 170 + 170 + 160 + 150 + 300, ship->maintenance_ticks);

  output = crew_harbor_command(&harbor, do_shiprepair, "sails");
  CuAssertTrue(tc, strstr(output, "needs no such work") != NULL);

  crew_harbor_end(&harbor);
}

void Test_vessel_refits_add_their_points_but_repair_nothing(CuTest *tc)
{
  struct greyhawk_ship_data *ship;

  /* Plating adds a fifth of each arc's armor, to the ceiling and to the
   * plates she has left: a damaged arc stays as damaged (L10). */
  ship = crew_warship(CREW_SHIP, "CA");
  ship->parmor = 50;
  vessel_refit_arcs(ship, FALSE);
  CuAssertIntEquals(tc, 130, ship->maxparmor);
  CuAssertIntEquals(tc, 71, ship->parmor);
  CuAssertIntEquals(tc, 130, ship->sarmor);

  crew_clear();
}
