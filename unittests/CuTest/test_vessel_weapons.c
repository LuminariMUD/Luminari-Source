/* Vessel weapons (vessels-ships study S4): the Duris weapon catalogue, class
 * armament and fitting rules, the shipyard commands, and weapon rows. */

#include "CuTest.h"

#include "conf.h"
#include "../../src/core/sysdep.h"
#include "../../src/core/structs.h"
#include "../../src/core/utils.h"
#include "../../src/core/comm.h"
#include "../../src/core/db.h"
#include "../../src/database/mysql.h"
#include "../../src/net/protocol.h"
#include "../../src/vessels/vessels.h"

#include <stdlib.h>
#include <string.h>

extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];

/* High fleet slots keep these fixtures clear of the rest of the suite. */
#define WEAPONS_SHIP 483
#define WEAPONS_ATTACKER 484

static struct greyhawk_ship_data *weapons_hull(int slot, enum vessel_class vessel_type)
{
  struct greyhawk_ship_data *ship = &greyhawk_ships[slot];

  memset(ship, 0, sizeof(*ship));
  ship->active = TRUE;
  ship->shipnum = slot;
  ship->vessel_type = vessel_type;
  ship->docked_to_ship = -1;
  strlcpy(ship->id, "WP", sizeof(ship->id));
  strlcpy(ship->name, "the Weapons Test", sizeof(ship->name));
  vessel_initialize_condition(ship, vessel_class_condition(vessel_type)->beam_armor);
  return ship;
}

static void weapons_clear(void)
{
  memset(&greyhawk_ships[WEAPONS_SHIP], 0, sizeof(greyhawk_ships[0]));
  memset(&greyhawk_ships[WEAPONS_ATTACKER], 0, sizeof(greyhawk_ships[0]));
}

void Test_vessel_catalogue_keeps_the_duris_weapons(CuTest *tc)
{
  const struct vessel_weapon_type *weapon;

  CuAssertTrue(tc, vessel_weapon_type(VESSEL_WEAPON_NONE) == NULL);
  CuAssertTrue(tc, vessel_weapon_type(NUM_VESSEL_WEAPONS) == NULL);

  /* Prices at 2 gold per pp; Duris's 30 s and 45 s reloads, tuned to the
   * D2 time to kill, are 20 s and 30 s in 0.5 s ticks. */
  weapon = vessel_weapon_type(VESSEL_WEAPON_LARGE_BALLISTA);
  CuAssertStrEquals(tc, "Large Ballista", weapon->name);
  CuAssertIntEquals(tc, 1000, weapon->price);
  CuAssertIntEquals(tc, 10, weapon->weight);
  CuAssertIntEquals(tc, 30, weapon->ammo);
  CuAssertIntEquals(tc, 12, weapon->max_range);
  CuAssertIntEquals(tc, 40, weapon->reload);
  CuAssertIntEquals(tc, 0xF, weapon->arcs);
  weapon = vessel_weapon_type(VESSEL_WEAPON_HEAVY_BEAMCANNON);
  CuAssertIntEquals(tc, 10000, weapon->price);
  CuAssertIntEquals(tc, 60, weapon->reload);
  CuAssertIntEquals(tc, VESSEL_WEAPON_RANGE_DAMAGE | VESSEL_WEAPON_CAPITAL, weapon->flags);

  /* Catapults fire from the ends, heavy ballistae from the beams. */
  weapon = vessel_weapon_type(VESSEL_WEAPON_LONG_TOM);
  CuAssertIntEquals(tc, (1 << GREYHAWK_FORE) | (1 << GREYHAWK_REAR), weapon->arcs);
  CuAssertIntEquals(tc, 12, weapon->min_range);
  CuAssertIntEquals(tc, 32, weapon->max_range);
  CuAssertTrue(tc, IS_SET(weapon->flags, VESSEL_WEAPON_BALLISTIC));
  weapon = vessel_weapon_type(VESSEL_WEAPON_HEAVY_BALLISTA);
  CuAssertIntEquals(tc, (1 << GREYHAWK_PORT) | (1 << GREYHAWK_STARBOARD), weapon->arcs);
  CuAssertTrue(
      tc, IS_SET(vessel_weapon_type(VESSEL_WEAPON_MIND_BLAST)->flags, VESSEL_WEAPON_CREW_STUN));
}

void Test_vessel_new_hulls_carry_the_class_armament(CuTest *tc)
{
  struct greyhawk_ship_data ship;
  int i;

  /* A warship: large ballistae on the bow and both beams, fully loaded. */
  memset(&ship, 0, sizeof(ship));
  ship.vessel_type = VESSEL_WARSHIP;
  vessel_fit_default_weapons(&ship);
  CuAssertIntEquals(tc, VESSEL_WEAPON_LARGE_BALLISTA, ship.slot[0].item);
  CuAssertIntEquals(tc, GREYHAWK_FORE, ship.slot[0].position);
  CuAssertIntEquals(tc, GREYHAWK_PORT, ship.slot[1].position);
  CuAssertIntEquals(tc, GREYHAWK_STARBOARD, ship.slot[2].position);
  CuAssertIntEquals(tc, 30, ship.slot[2].ammo);
  CuAssertIntEquals(tc, VESSEL_SLOT_EMPTY, ship.slot[3].type);
  CuAssertTrue(tc, vessel_weapon_ready(&ship.slot[1]));

  /* Other armed classes: one medium ballista on the bow. */
  memset(&ship, 0, sizeof(ship));
  ship.vessel_type = VESSEL_TRANSPORT;
  vessel_fit_default_weapons(&ship);
  CuAssertIntEquals(tc, VESSEL_WEAPON_MEDIUM_BALLISTA, ship.slot[0].item);
  CuAssertIntEquals(tc, 50, ship.slot[0].ammo);
  CuAssertIntEquals(tc, VESSEL_SLOT_EMPTY, ship.slot[1].type);

  /* Rafts and boats sail unarmed. */
  ship.vessel_type = VESSEL_BOAT;
  vessel_fit_default_weapons(&ship);
  for (i = 0; i < GREYHAWK_MAXSLOTS; i++)
  {
    CuAssertIntEquals(tc, VESSEL_SLOT_EMPTY, ship.slot[i].type);
  }

  /* A weapon without a round cannot fire. */
  ship.vessel_type = VESSEL_WARSHIP;
  vessel_fit_default_weapons(&ship);
  ship.slot[0].ammo = 0;
  CuAssertTrue(tc, !vessel_weapon_ready(&ship.slot[0]));
}

void Test_vessel_beam_damage_falls_with_range(CuTest *tc)
{
  struct greyhawk_ship_data *target;
  struct greyhawk_ship_data *attacker;

  /* Due east of a stripped target heading north, every beam shot strikes
   * her starboard side: 22 at point blank, 5 at the 23-room maximum. */
  target = weapons_hull(WEAPONS_SHIP, VESSEL_WARSHIP);
  attacker = weapons_hull(WEAPONS_ATTACKER, VESSEL_WARSHIP);
  vessel_set_weapon(&attacker->slot[0], VESSEL_WEAPON_HEAVY_BEAMCANNON, GREYHAWK_PORT);
  attacker->x = 5.0;
  target->mainsail = 0;
  CuAssertIntEquals(tc, 22, vessel_resolve_hit(attacker, target, &attacker->slot[0], 0.0, FALSE));
  CuAssertIntEquals(tc, 5, vessel_resolve_hit(attacker, target, &attacker->slot[0], 23.0, FALSE));
  CuAssertIntEquals(tc, 109 - 27, target->sarmor);

  /* A crew-stun weapon does no damage. */
  vessel_set_weapon(&attacker->slot[1], VESSEL_WEAPON_MIND_BLAST, GREYHAWK_PORT);
  CuAssertIntEquals(tc, 0, vessel_resolve_hit(attacker, target, &attacker->slot[1], 0.0, FALSE));
  CuAssertIntEquals(tc, 109 - 27, target->sarmor);

  weapons_clear();
}

void Test_vessel_fitout_rules_follow_the_class(CuTest *tc)
{
  struct greyhawk_ship_data *ship;
  const char *problem;

  /* A warship's default armament is legal; two more large ballistae fill
   * her port beam, and a fourth weapon there is one too many. */
  ship = weapons_hull(WEAPONS_SHIP, VESSEL_WARSHIP);
  vessel_fit_default_weapons(ship);
  CuAssertTrue(tc, vessel_fitout_problem(ship) == NULL);
  vessel_set_weapon(&ship->slot[3], VESSEL_WEAPON_LARGE_BALLISTA, GREYHAWK_PORT);
  vessel_set_weapon(&ship->slot[4], VESSEL_WEAPON_LARGE_BALLISTA, GREYHAWK_PORT);
  CuAssertTrue(tc, vessel_fitout_problem(ship) == NULL);
  vessel_set_weapon(&ship->slot[5], VESSEL_WEAPON_SMALL_BALLISTA, GREYHAWK_PORT);
  problem = vessel_fitout_problem(ship);
  CuAssertTrue(tc, problem != NULL && strstr(problem, "at most 3 weapons on the port arc"));

  /* Three heavy ballistae outweigh the port cap of 44. */
  vessel_fit_default_weapons(ship);
  vessel_set_weapon(&ship->slot[1], VESSEL_WEAPON_HEAVY_BALLISTA, GREYHAWK_PORT);
  vessel_set_weapon(&ship->slot[3], VESSEL_WEAPON_HEAVY_BALLISTA, GREYHAWK_PORT);
  vessel_set_weapon(&ship->slot[4], VESSEL_WEAPON_HEAVY_BALLISTA, GREYHAWK_PORT);
  problem = vessel_fitout_problem(ship);
  CuAssertTrue(tc, problem != NULL && strstr(problem, "at most 44 weight of weapons on the port "
                                                      "arc; this fit puts 45 there"));

  /* Catapults fire from the ends, and one capital weapon is the limit. */
  vessel_fit_default_weapons(ship);
  vessel_set_weapon(&ship->slot[3], VESSEL_WEAPON_SMALL_CATAPULT, GREYHAWK_PORT);
  problem = vessel_fitout_problem(ship);
  CuAssertTrue(tc, problem != NULL && strstr(problem, "Small Catapult cannot be mounted on the "
                                                      "port arc"));
  vessel_set_weapon(&ship->slot[3], VESSEL_WEAPON_LIGHT_BEAMCANNON, GREYHAWK_FORE);
  CuAssertTrue(tc, vessel_fitout_problem(ship) == NULL);
  vessel_set_weapon(&ship->slot[4], VESSEL_WEAPON_HEAVY_BEAMCANNON, GREYHAWK_REAR);
  problem = vessel_fitout_problem(ship);
  CuAssertTrue(tc, problem != NULL && strstr(problem, "only one capital weapon"));

  /* A boat mounts small ballistae only and carries no ram. */
  ship = weapons_hull(WEAPONS_ATTACKER, VESSEL_BOAT);
  vessel_set_weapon(&ship->slot[0], VESSEL_WEAPON_SMALL_BALLISTA, GREYHAWK_FORE);
  CuAssertTrue(tc, vessel_fitout_problem(ship) == NULL);
  vessel_set_weapon(&ship->slot[0], VESSEL_WEAPON_MEDIUM_BALLISTA, GREYHAWK_FORE);
  problem = vessel_fitout_problem(ship);
  CuAssertTrue(tc, problem != NULL && strstr(problem, "cannot mount a Medium Ballista"));
  memset(&ship->slot[0], 0, sizeof(ship->slot[0]));
  ship->slot[0].type = VESSEL_SLOT_EQUIPMENT;
  ship->slot[0].item = VESSEL_EQUIPMENT_RAM;
  problem = vessel_fitout_problem(ship);
  CuAssertTrue(tc, problem != NULL && strstr(problem, "cannot carry a ram"));

  /* The fit-out stays within the hull's weight budget: 82 for an airship. */
  ship = weapons_hull(WEAPONS_ATTACKER, VESSEL_AIRSHIP);
  vessel_set_weapon(&ship->slot[0], VESSEL_WEAPON_MEDIUM_CATAPULT, GREYHAWK_FORE);
  vessel_set_weapon(&ship->slot[1], VESSEL_WEAPON_LARGE_BALLISTA, GREYHAWK_PORT);
  vessel_set_weapon(&ship->slot[2], VESSEL_WEAPON_LARGE_BALLISTA, GREYHAWK_PORT);
  vessel_set_weapon(&ship->slot[3], VESSEL_WEAPON_LARGE_BALLISTA, GREYHAWK_PORT);
  vessel_set_weapon(&ship->slot[4], VESSEL_WEAPON_LARGE_BALLISTA, GREYHAWK_STARBOARD);
  vessel_set_weapon(&ship->slot[5], VESSEL_WEAPON_LARGE_BALLISTA, GREYHAWK_STARBOARD);
  vessel_set_weapon(&ship->slot[6], VESSEL_WEAPON_LARGE_BALLISTA, GREYHAWK_STARBOARD);
  CuAssertTrue(tc, vessel_fitout_problem(ship) == NULL);
  vessel_set_weapon(&ship->slot[7], VESSEL_WEAPON_MEDIUM_CATAPULT, GREYHAWK_REAR);
  problem = vessel_fitout_problem(ship);
  CuAssertTrue(tc, problem != NULL && strstr(problem, "would weigh 86, more than the 82"));

  weapons_clear();
}

/* A captain aboard her warship, berthed in a one-room harbor. */
struct weapons_berth
{
  struct room_data room;
  struct obj_data hull;
  struct char_data captain;
  struct player_special_data specials;
  struct descriptor_data descriptor;
  char output[8192];
  struct room_data *saved_world;
  room_rnum saved_top_of_world;
};

static struct greyhawk_ship_data *weapons_berth_begin(CuTest *tc, struct weapons_berth *berth)
{
  struct greyhawk_ship_data *ship;

  memset(berth, 0, sizeof(*berth));
  berth->room.number = 100;
  SET_BIT_AR(berth->room.room_flags, ROOM_DOCKABLE);
  berth->saved_world = world;
  berth->saved_top_of_world = top_of_world;
  world = &berth->room;
  top_of_world = 0;

  ship = weapons_hull(WEAPONS_SHIP, VESSEL_WARSHIP);
  vessel_fit_default_weapons(ship);
  strlcpy(ship->owner, "Mara", sizeof(ship->owner));
  ship->shipobj = &berth->hull;
  IN_ROOM(&berth->hull) = 0;
  ship->dock = 100;
  berth->room.ship = ship;

  berth->captain.player_specials = &berth->specials;
  berth->captain.player.name = CuMutableString("Mara");
  berth->captain.player.level = 20;
  IN_ROOM(&berth->captain) = 0;
  GET_POS(&berth->captain) = POS_STANDING;
  GET_GOLD(&berth->captain) = 100000;
  berth->captain.desc = &berth->descriptor;
  berth->descriptor.character = &berth->captain;
  berth->descriptor.output = berth->output;
  berth->descriptor.bufspace = sizeof(berth->output) - 1;
  berth->descriptor.pProtocol = ProtocolCreate();
  CuAssertPtrNotNull(tc, berth->descriptor.pProtocol);
  return ship;
}

/* Run a command and return what it printed. */
static void weapons_berth_clear_output(struct weapons_berth *berth)
{
  memset(berth->output, 0, sizeof(berth->output));
  berth->descriptor.bufptr = 0;
  berth->descriptor.bufspace = sizeof(berth->output) - 1;
}

static const char *weapons_berth_command(struct weapons_berth *berth, ACMD_DECL((*command)),
                                         const char *argument)
{
  weapons_berth_clear_output(berth);
  command(&berth->captain, argument, 0, 0);
  return berth->output;
}

static void weapons_berth_end(struct weapons_berth *berth)
{
  ProtocolDestroy(berth->descriptor.pProtocol);
  world = berth->saved_world;
  top_of_world = berth->saved_top_of_world;
  weapons_clear();
}

void Test_vessel_shipyard_fits_weapons_and_equipment(CuTest *tc)
{
  struct weapons_berth berth;
  struct greyhawk_ship_data *ship;
  const char *output;

  ship = weapons_berth_begin(tc, &berth);

  output = weapons_berth_command(&berth, do_shipweapon, "list");
  CuAssertTrue(tc, strstr(output, "Long Tom Catapult") != NULL);
  CuAssertTrue(tc, strstr(output, "fore 1/2, 10/31; port 1/3, 10/44; rear 0/2, 0/31; "
                                  "starboard 1/3, 10/44") != NULL);

  /* A new weapon is paid for, loaded, and keeps her in port while the
   * shipwrights work: 75 s per weight point. */
  output = weapons_berth_command(&berth, do_shipweapon, "buy large ballista rear");
  CuAssertTrue(tc, strstr(output, "mount a Large Ballista on the rear arc") != NULL);
  CuAssertIntEquals(tc, VESSEL_WEAPON_LARGE_BALLISTA, ship->slot[3].item);
  CuAssertIntEquals(tc, GREYHAWK_REAR, ship->slot[3].position);
  CuAssertIntEquals(tc, 30, ship->slot[3].ammo);
  CuAssertIntEquals(tc, 99000, GET_GOLD(&berth.captain));
  CuAssertIntEquals(tc, 10 * VESSEL_INSTALL_TICKS_PER_WEIGHT, ship->maintenance_ticks);

  /* A capital weapon needs a veteran gunner, and a hull carries one. */
  output = weapons_berth_command(&berth, do_shipweapon, "buy heavy beam fore");
  CuAssertTrue(tc, strstr(output, "Only a veteran gunner") != NULL);
  CuAssertIntEquals(tc, VESSEL_SLOT_EMPTY, ship->slot[4].type);
  CuAssertIntEquals(tc, 99000, GET_GOLD(&berth.captain));
  ship->crew_tier[CREW_GUNNER] = CREW_TIER_VETERAN;
  weapons_berth_command(&berth, do_shipweapon, "buy 9 fore");
  CuAssertIntEquals(tc, VESSEL_WEAPON_HEAVY_BEAMCANNON, ship->slot[4].item);
  CuAssertIntEquals(tc, 89000, GET_GOLD(&berth.captain));
  output = weapons_berth_command(&berth, do_shipweapon, "buy long tom rear");
  CuAssertTrue(tc, strstr(output, "only one capital weapon") != NULL);
  CuAssertIntEquals(tc, 89000, GET_GOLD(&berth.captain));

  /* A sound weapon sells for 90%, a damaged one for 10%. */
  weapons_berth_command(&berth, do_shipweapon, "sell 3");
  CuAssertIntEquals(tc, VESSEL_SLOT_EMPTY, ship->slot[3].type);
  CuAssertIntEquals(tc, 89900, GET_GOLD(&berth.captain));
  ship->slot[4].damage = 20;
  weapons_berth_command(&berth, do_shipweapon, "sell 4");
  CuAssertIntEquals(tc, 90900, GET_GOLD(&berth.captain));

  /* Slots trade places whole. */
  weapons_berth_command(&berth, do_shipweapon, "swap 0 1");
  CuAssertIntEquals(tc, GREYHAWK_PORT, ship->slot[0].position);
  CuAssertIntEquals(tc, GREYHAWK_FORE, ship->slot[1].position);

  /* Rearming costs 2 gold a round and 75 s per weapon. */
  ship->maintenance_ticks = 0;
  ship->slot[0].ammo = 10;
  output = weapons_berth_command(&berth, do_shiprearm, "all");
  CuAssertTrue(tc, strstr(output, "rearm 1 weapon for 40 gold") != NULL);
  CuAssertIntEquals(tc, 30, ship->slot[0].ammo);
  CuAssertIntEquals(tc, 90860, GET_GOLD(&berth.captain));
  CuAssertIntEquals(tc, VESSEL_REARM_TICKS, ship->maintenance_ticks);
  output = weapons_berth_command(&berth, do_shiprearm, "");
  CuAssertTrue(tc, strstr(output, "nothing to rearm") != NULL);

  /* A ram costs 2 gold per hull weight; neutral colors are free but stay up
   * while cargo is aboard. */
  ship->maintenance_ticks = 0;
  weapons_berth_command(&berth, do_shipequip, "buy ram");
  CuAssertIntEquals(tc, 90290, GET_GOLD(&berth.captain));
  CuAssertIntEquals(tc, 12 * VESSEL_INSTALL_TICKS_PER_WEIGHT, ship->maintenance_ticks);
  output = weapons_berth_command(&berth, do_shipequip, "buy ram");
  CuAssertTrue(tc, strstr(output, "already carries a Ram") != NULL);
  weapons_berth_command(&berth, do_shipequip, "buy colors");
  ship->cargo[0].commodity_id = 1;
  ship->cargo[0].quantity = 5;
  output = weapons_berth_command(&berth, do_shipequip, "sell colors");
  CuAssertTrue(tc, strstr(output, "stay up while she has cargo aboard") != NULL);
  ship->cargo[0].quantity = 0;
  weapons_berth_command(&berth, do_shipequip, "sell colors");
  output = weapons_berth_command(&berth, do_shipequip, "list");
  CuAssertTrue(tc, strstr(output, " 0 gold, weight 0\r\n") != NULL); /* colors, removed */
  CuAssertTrue(tc, strstr(output, " 570 gold, weight 12 (fitted)") != NULL);

  /* She cannot depart while the shipwrights work or with an illegal fit. */
  CuAssertTrue(tc, !vessel_begin_departure(ship, &berth.captain));
  CuAssertTrue(tc, strstr(berth.output, "still at work") != NULL);
  ship->maintenance_ticks = 1;
  vessel_movement_tick_one(ship);
  CuAssertIntEquals(tc, 0, ship->maintenance_ticks);
  vessel_set_weapon(&ship->slot[6], VESSEL_WEAPON_LONG_TOM, GREYHAWK_PORT);
  weapons_berth_clear_output(&berth);
  CuAssertTrue(tc, !vessel_begin_departure(ship, &berth.captain));
  CuAssertTrue(tc, strstr(berth.output, "withholds clearance: A Long Tom Catapult cannot be "
                                        "mounted on the port arc") != NULL);

  /* The shipwrights take no work once the crew is casting off, nor from a
   * hull cast off in the harbor, so none of it can sail with her. */
  ship->departure_ticks = VESSEL_UNDOCK_TICKS;
  output = weapons_berth_command(&berth, do_shiprearm, "all");
  CuAssertTrue(tc, strstr(output, "The crew is casting off") != NULL);
  ship->departure_ticks = 0;
  ship->dock = 0;
  output = weapons_berth_command(&berth, do_shipweapon, "buy small ballista fore");
  CuAssertTrue(tc, strstr(output, "moor at a dock first") != NULL);
  CuAssertIntEquals(tc, 0, ship->maintenance_ticks);

  /* Staff skip the shipwrights' time. */
  berth.captain.player.level = LVL_IMMORT;
  vessel_add_maintenance(ship, &berth.captain, 100);
  CuAssertIntEquals(tc, 0, ship->maintenance_ticks);

  weapons_berth_end(&berth);
}

static MYSQL *weapons_open_test_database(void)
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

void Test_vessel_weapon_rows_keep_every_slot_and_convert_old_weapons(CuTest *tc)
{
  const char *enabled = getenv("LUMINARI_TEST_MYSQL_ENABLE");
  struct greyhawk_ship_data *ship;
  char query[512];
  MYSQL *saved_conn;
  MYSQL *connection;
  bool saved_mysql_available;
  bool prepared;

  if (enabled == NULL || strcmp(enabled, "1") != 0)
  {
    return;
  }

  connection = weapons_open_test_database();
  if (connection == NULL)
  {
    CuFail(tc, "could not connect to the explicitly configured test database");
    return;
  }

  /* Weapon rows shadowing the real table: one saved before S4 (no catalogue
   * row, a damaged starboard ballista) and one unknown leftover slot. */
  prepared = mysql_query(connection, "CREATE TEMPORARY TABLE ship_weapons ("
                                     "ship_id INT NOT NULL, "
                                     "slot_index TINYINT UNSIGNED NOT NULL, "
                                     "slot_type TINYINT UNSIGNED NOT NULL DEFAULT 1, "
                                     "position TINYINT UNSIGNED NOT NULL DEFAULT 0, "
                                     "reload_timer SMALLINT NOT NULL DEFAULT 0, "
                                     "weapon_damage TINYINT UNSIGNED NOT NULL DEFAULT 0, "
                                     "catalog_id TINYINT UNSIGNED NOT NULL DEFAULT 0, "
                                     "ammo SMALLINT UNSIGNED NOT NULL DEFAULT 0, "
                                     "PRIMARY KEY (ship_id, slot_index))") == 0;
  snprintf(query, sizeof(query),
           "INSERT INTO ship_weapons (ship_id, slot_index, slot_type, position, reload_timer, "
           "weapon_damage) VALUES (%d, 2, 1, %d, 4, 35)",
           WEAPONS_SHIP, GREYHAWK_STARBOARD);
  prepared = prepared && mysql_query(connection, query) == 0;
  if (!prepared)
  {
    mysql_close(connection);
    CuFail(tc, "could not create the isolated weapon row fixture");
    return;
  }

  saved_conn = conn;
  saved_mysql_available = mysql_available;
  conn = connection;
  mysql_available = TRUE;

  /* The old weapon becomes a warship's large ballista on the same arc, fully
   * loaded, keeping its damage and reload. */
  ship = weapons_hull(WEAPONS_SHIP, VESSEL_WARSHIP);
  vessel_set_weapon(&ship->slot[5], VESSEL_WEAPON_SMALL_BALLISTA, GREYHAWK_REAR);
  CuAssertTrue(tc, vessel_db_load_weapons(ship));
  CuAssertIntEquals(tc, VESSEL_SLOT_WEAPON, ship->slot[2].type);
  CuAssertIntEquals(tc, VESSEL_WEAPON_LARGE_BALLISTA, ship->slot[2].item);
  CuAssertIntEquals(tc, GREYHAWK_STARBOARD, ship->slot[2].position);
  CuAssertIntEquals(tc, 30, ship->slot[2].ammo);
  CuAssertIntEquals(tc, 35, ship->slot[2].damage);
  CuAssertIntEquals(tc, 4, ship->slot[2].timer);
  CuAssertIntEquals(tc, VESSEL_SLOT_EMPTY, ship->slot[5].type); /* rows are the whole fit */

  /* Weapons and equipment survive a save and reload as they are. */
  ship->slot[2].ammo = 7;
  vessel_set_weapon(&ship->slot[GREYHAWK_MAXSLOTS - 1], VESSEL_WEAPON_LONG_TOM, GREYHAWK_REAR);
  ship->slot[GREYHAWK_MAXSLOTS - 1].timer = 11;
  ship->slot[3].type = VESSEL_SLOT_EQUIPMENT;
  ship->slot[3].item = VESSEL_EQUIPMENT_RAM;
  CuAssertTrue(tc, vessel_db_save_weapons(ship));
  memset(ship->slot, 0, sizeof(ship->slot));
  CuAssertTrue(tc, vessel_db_load_weapons(ship));
  CuAssertIntEquals(tc, 7, ship->slot[2].ammo);
  CuAssertIntEquals(tc, 35, ship->slot[2].damage);
  CuAssertIntEquals(tc, VESSEL_WEAPON_LONG_TOM, ship->slot[GREYHAWK_MAXSLOTS - 1].item);
  CuAssertIntEquals(tc, GREYHAWK_REAR, ship->slot[GREYHAWK_MAXSLOTS - 1].position);
  CuAssertIntEquals(tc, 6, ship->slot[GREYHAWK_MAXSLOTS - 1].ammo);
  CuAssertIntEquals(tc, 11, ship->slot[GREYHAWK_MAXSLOTS - 1].timer);
  CuAssertIntEquals(tc, VESSEL_SLOT_EQUIPMENT, ship->slot[3].type);
  CuAssertIntEquals(tc, VESSEL_EQUIPMENT_RAM, ship->slot[3].item);
  CuAssertIntEquals(tc, VESSEL_SLOT_EMPTY, ship->slot[0].type);

  conn = saved_conn;
  mysql_available = saved_mysql_available;
  weapons_clear();
  mysql_query(connection, "DROP TEMPORARY TABLE ship_weapons");
  mysql_close(connection);
}
