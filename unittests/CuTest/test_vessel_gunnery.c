/* Vessel gunnery rules: who may fire, owner consent, harbor immunity, the
 * shared contact list, command lag on every shot, and the S4 gunnery: the
 * geometry hit model, locks, battle stations, arc fire, sighting, and
 * scanning. */

#include "CuTest.h"

#include "conf.h"
#include "../../src/core/sysdep.h"
#include "../../src/core/structs.h"
#include "../../src/core/utils.h"
#include "../../src/core/comm.h"
#include "../../src/core/db.h"
#include "../../src/core/interpreter.h"
#include "../../src/database/mysql.h"
#include "../../src/net/protocol.h"
#include "../../src/vessels/vessels.h"

#include <string.h>

extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];

/* High fleet slots keep these fixtures clear of the rest of the suite. */
#define GUNNERY_SHIP_A 480
#define GUNNERY_SHIP_B 481
#define GUNNERY_SHIP_C 482

struct gunnery_player
{
  struct char_data ch;
  struct player_special_data specials;
};

static void gunnery_player_init(struct gunnery_player *player, const char *name)
{
  memset(player, 0, sizeof(*player));
  player->ch.player_specials = &player->specials;
  player->ch.player.name = CuMutableString(name);
  IN_ROOM(&player->ch) = NOWHERE;
  GET_POS(&player->ch) = POS_STANDING;
}

static void gunnery_arm_ship(int shipnum, const char *name, const char *id, double x, double y)
{
  struct greyhawk_ship_data *ship = &greyhawk_ships[shipnum];

  memset(ship, 0, sizeof(*ship));
  ship->active = TRUE;
  ship->shipnum = shipnum;
  ship->x = x;
  ship->y = y;
  ship->docked_to_ship = -1;
  strlcpy(ship->name, name, sizeof(ship->name));
  strlcpy(ship->id, id, sizeof(ship->id));
  ship->vessel_type = VESSEL_WARSHIP;
  ship->maxspeed = 17;
  ship->position_speed_percent = 100;
  vessel_initialize_condition(ship, 40);
  vessel_set_weapon(&ship->slot[0], VESSEL_WEAPON_LARGE_BALLISTA, GREYHAWK_FORE);
}

static void gunnery_clear_ships(void)
{
  memset(&greyhawk_ships[GUNNERY_SHIP_A], 0, sizeof(greyhawk_ships[0]));
  memset(&greyhawk_ships[GUNNERY_SHIP_B], 0, sizeof(greyhawk_ships[0]));
  memset(&greyhawk_ships[GUNNERY_SHIP_C], 0, sizeof(greyhawk_ships[0]));
}

void Test_vessel_speed_modifier_uses_storm_bands(CuTest *tc)
{
  /* Clear and overcast skies are band 0; the old code read raw weather
   * above 50 of 255 as a storm. */
  CuAssertIntEquals(tc, 0, vessel_weather_severity_from_value(127));
  CuAssertIntEquals(tc, 0, vessel_weather_severity_from_value(VESSEL_WEATHER_CLOUDY));
  CuAssertIntEquals(tc, 1, vessel_weather_severity_from_value(VESSEL_WEATHER_SQUALL));
  CuAssertIntEquals(tc, 2, vessel_weather_severity_from_value(VESSEL_WEATHER_STORM));
  CuAssertIntEquals(tc, 3, vessel_weather_severity_from_value(VESSEL_WEATHER_GALE));

  /* A squall costs 5% of speed, never the old 25% for clear weather. */
  CuAssertIntEquals(tc, get_terrain_speed_modifier(VESSEL_SHIP, SECT_OCEAN, 0) - 5,
                    get_terrain_speed_modifier(VESSEL_SHIP, SECT_OCEAN, 1));
}

void Test_vessel_gunnery_answers_to_owner_permits_and_group(CuTest *tc)
{
  struct greyhawk_ship_data *ship = &greyhawk_ships[GUNNERY_SHIP_A];
  struct gunnery_player owner;
  struct gunnery_player permit;
  struct gunnery_player passenger;
  struct gunnery_player staff;
  struct char_data npc;
  struct group_data group;
  struct char_data *saved_list;

  gunnery_player_init(&owner, "Corr");
  gunnery_player_init(&permit, "Mira");
  gunnery_player_init(&passenger, "Vex");
  gunnery_player_init(&staff, "Zusuk");
  staff.ch.player.level = LVL_IMMORT;
  memset(&npc, 0, sizeof(npc));
  SET_BIT_AR(MOB_FLAGS(&npc), MOB_ISNPC);
  memset(&group, 0, sizeof(group));
  saved_list = character_list;
  character_list = NULL;
  gunnery_arm_ship(GUNNERY_SHIP_A, "the Gull", "SA", 0.0, 0.0);

  /* Unowned hulls fire only through their NPC crews (and staff). */
  CuAssertTrue(tc, !vessel_gunnery_permitted(&passenger.ch, ship));
  CuAssertTrue(tc, !vessel_gunnery_permitted(&npc, ship));
  CuAssertTrue(tc, vessel_gunnery_permitted(&staff.ch, ship));
  CuAssertTrue(tc, !vessel_gunnery_permitted(NULL, ship));
  CuAssertTrue(tc, !vessel_gunnery_permitted(&staff.ch, NULL));

  strlcpy(ship->owner, "Corr", sizeof(ship->owner));
  strlcpy(ship->helm_permits[0], "Mira", sizeof(ship->helm_permits[0]));
  ship->num_permits = 1;
  CuAssertTrue(tc, vessel_gunnery_permitted(&owner.ch, ship));
  CuAssertTrue(tc, vessel_gunnery_permitted(&permit.ch, ship));
  CuAssertTrue(tc, !vessel_gunnery_permitted(&passenger.ch, ship));
  CuAssertTrue(tc, vessel_gunnery_permitted(&staff.ch, ship));

  /* A member of the owner's group may fire while the owner is online. */
  owner.ch.group = &group;
  passenger.ch.group = &group;
  CuAssertTrue(tc, !vessel_gunnery_permitted(&passenger.ch, ship));
  character_list = &owner.ch;
  CuAssertTrue(tc, vessel_gunnery_permitted(&passenger.ch, ship));
  passenger.ch.group = NULL;
  CuAssertTrue(tc, !vessel_gunnery_permitted(&passenger.ch, ship));

  character_list = saved_list;
  gunnery_clear_ships();
}

void Test_vessel_fire_needs_the_hull_owners_consent(CuTest *tc)
{
  struct greyhawk_ship_data *ship = &greyhawk_ships[GUNNERY_SHIP_A];
  struct greyhawk_ship_data *target = &greyhawk_ships[GUNNERY_SHIP_B];
  struct gunnery_player owner;
  struct gunnery_player gunner;
  struct gunnery_player rival;
  struct room_data room;
  struct room_data *saved_world;
  room_rnum saved_top_of_world;
  struct char_data *saved_list;
  int saved_pk_allowed;

  gunnery_player_init(&owner, "Corr");
  gunnery_player_init(&gunner, "Mira");
  gunnery_player_init(&rival, "Vex");
  memset(&room, 0, sizeof(room));
  room.number = 100;
  saved_world = world;
  saved_top_of_world = top_of_world;
  saved_list = character_list;
  saved_pk_allowed = CONFIG_PK_ALLOWED;
  world = &room;
  top_of_world = 0;
  IN_ROOM(&owner.ch) = 0;
  IN_ROOM(&gunner.ch) = 0;
  IN_ROOM(&rival.ch) = 0;
  CONFIG_PK_ALLOWED = TRUE;
  SET_BIT_AR(PRF_FLAGS(&gunner.ch), PRF_PVP);
  SET_BIT_AR(PRF_FLAGS(&rival.ch), PRF_PVP);
  character_list = &gunner.ch;
  gunner.ch.next = &rival.ch;

  gunnery_arm_ship(GUNNERY_SHIP_A, "the Gull", "SA", 0.0, 0.0);
  gunnery_arm_ship(GUNNERY_SHIP_B, "the Tern", "SB", 0.0, 10.0);
  strlcpy(ship->owner, "Corr", sizeof(ship->owner));

  /* An unowned target is PvE: the hull owner's consent is not needed. */
  CuAssertTrue(tc, vessel_fire_permitted(&gunner.ch, ship, target, FALSE));

  /* A consenting gunner cannot drag an absent owner's hull into PvP. */
  strlcpy(target->owner, "Vex", sizeof(target->owner));
  CuAssertTrue(tc, vessel_pvp_permitted(&gunner.ch, target, FALSE));
  CuAssertTrue(tc, !vessel_fire_permitted(&gunner.ch, ship, target, FALSE));

  /* Nor the hull of an owner who is online without PvP enabled. */
  rival.ch.next = &owner.ch;
  CuAssertTrue(tc, !vessel_fire_permitted(&gunner.ch, ship, target, FALSE));

  /* Once the owner consents, the gunner may open fire. */
  SET_BIT_AR(PRF_FLAGS(&owner.ch), PRF_PVP);
  CuAssertTrue(tc, vessel_fire_permitted(&gunner.ch, ship, target, FALSE));
  CuAssertTrue(tc, vessel_fire_permitted(&owner.ch, ship, target, FALSE));

  CONFIG_PK_ALLOWED = saved_pk_allowed;
  character_list = saved_list;
  world = saved_world;
  top_of_world = saved_top_of_world;
  gunnery_clear_ships();
}

void Test_vessel_fire_records_grace_for_the_gunner_only(CuTest *tc)
{
  struct greyhawk_ship_data *ship = &greyhawk_ships[GUNNERY_SHIP_A];
  struct greyhawk_ship_data *target = &greyhawk_ships[GUNNERY_SHIP_B];
  struct gunnery_player owner;
  struct gunnery_player gunner;
  struct gunnery_player rival;
  struct room_data room;
  struct room_data *saved_world;
  room_rnum saved_top_of_world;
  struct char_data *saved_list;
  int saved_pk_allowed;
  bool saved_mysql_available;

  gunnery_player_init(&owner, "Corr");
  gunnery_player_init(&gunner, "Mira");
  gunnery_player_init(&rival, "Vex");
  memset(&room, 0, sizeof(room));
  room.number = 100;
  saved_world = world;
  saved_top_of_world = top_of_world;
  saved_list = character_list;
  saved_pk_allowed = CONFIG_PK_ALLOWED;
  saved_mysql_available = mysql_available;
  mysql_available = FALSE;
  world = &room;
  top_of_world = 0;
  IN_ROOM(&owner.ch) = 0;
  IN_ROOM(&gunner.ch) = 0;
  IN_ROOM(&rival.ch) = 0;
  CONFIG_PK_ALLOWED = TRUE;
  SET_BIT_AR(PRF_FLAGS(&gunner.ch), PRF_PVP);
  SET_BIT_AR(PRF_FLAGS(&rival.ch), PRF_PVP);
  character_list = &gunner.ch;
  gunner.ch.next = &rival.ch;
  rival.ch.next = &owner.ch;

  gunnery_arm_ship(GUNNERY_SHIP_A, "the Gull", "SA", 0.0, 0.0);
  gunnery_arm_ship(GUNNERY_SHIP_B, "the Tern", "SB", 0.0, 10.0);
  strlcpy(ship->owner, "Corr", sizeof(ship->owner));
  strlcpy(target->owner, "Vex", sizeof(target->owner));
  room.ship = ship; /* the gunner stands aboard the Gull */

  /* A shot the hull owner refuses leaves no grace on either hull. */
  CuAssertTrue(tc, !vessel_fire_permitted(&gunner.ch, ship, target, FALSE));
  CuAssertStrEquals(tc, "", target->pvp_grace_attacker);
  CuAssertTrue(tc, target->pvp_grace_until == 0);
  CuAssertStrEquals(tc, "", ship->pvp_grace_attacker);
  CuAssertTrue(tc, ship->pvp_grace_until == 0);

  /* An approved shot records the gunner, not the consenting hull owner. */
  SET_BIT_AR(PRF_FLAGS(&owner.ch), PRF_PVP);
  CuAssertTrue(tc, vessel_fire_permitted(&gunner.ch, ship, target, FALSE));
  CuAssertStrEquals(tc, "Mira", target->pvp_grace_attacker);
  CuAssertTrue(tc, target->pvp_grace_until > 0);
  CuAssertStrEquals(tc, "Vex", ship->pvp_grace_attacker);

  /* With the target's owner gone, only that gunner fights on, and only
   * while the hull owner stays PvP-enabled. */
  gunner.ch.next = &owner.ch;
  CuAssertTrue(tc, vessel_fire_permitted(&gunner.ch, ship, target, FALSE));
  CuAssertStrEquals(tc, "Mira", target->pvp_grace_attacker);
  CuAssertTrue(tc, !vessel_fire_permitted(&owner.ch, ship, target, FALSE));
  REMOVE_BIT_AR(PRF_FLAGS(&owner.ch), PRF_PVP);
  CuAssertTrue(tc, !vessel_fire_permitted(&gunner.ch, ship, target, FALSE));

  mysql_available = saved_mysql_available;
  CONFIG_PK_ALLOWED = saved_pk_allowed;
  character_list = saved_list;
  world = saved_world;
  top_of_world = saved_top_of_world;
  gunnery_clear_ships();
}

void Test_vessel_contact_list_is_nearest_first_within_sight(CuTest *tc)
{
  struct greyhawk_ship_data *ship = &greyhawk_ships[GUNNERY_SHIP_A];
  struct vessel_contact contacts[2];
  struct vessel_contact one[1];
  int count;

  gunnery_arm_ship(GUNNERY_SHIP_A, "the Gull", "SA", 0.0, 0.0);
  gunnery_arm_ship(GUNNERY_SHIP_B, "Tern Runner", "SB", 0.0, 8.0);
  gunnery_arm_ship(GUNNERY_SHIP_C, "Tern Chaser", "SC", 0.0, 4.0);

  count = vessel_collect_contacts(ship, contacts, 2);
  CuAssertTrue(tc, count >= 2);
  CuAssertIntEquals(tc, GUNNERY_SHIP_C, contacts[0].shipnum);
  CuAssertIntEquals(tc, GUNNERY_SHIP_B, contacts[1].shipnum);
  CuAssertTrue(tc, contacts[0].range < contacts[1].range);
  CuAssertIntEquals(tc, 0, contacts[0].bearing);

  /* A full list keeps the nearest contact and still counts the rest. */
  CuAssertIntEquals(tc, count, vessel_collect_contacts(ship, one, 1));
  CuAssertIntEquals(tc, GUNNERY_SHIP_C, one[0].shipnum);

  /* IDs match exactly; names match by prefix, nearest first. */
  CuAssertIntEquals(tc, GUNNERY_SHIP_B, vessel_find_contact(ship, "sb"));
  CuAssertIntEquals(tc, GUNNERY_SHIP_C, vessel_find_contact(ship, "Tern"));
  CuAssertIntEquals(tc, GUNNERY_SHIP_B, vessel_find_contact(ship, "Tern R"));
  CuAssertIntEquals(tc, -1, vessel_find_contact(ship, "the Gull"));
  CuAssertIntEquals(tc, -1, vessel_find_contact(ship, ""));

  /* A vessel beyond sight is no contact, even by its exact ID. */
  greyhawk_ships[GUNNERY_SHIP_B].y = (double)(vessel_sight_range(ship) + 5);
  CuAssertIntEquals(tc, -1, vessel_find_contact(ship, "SB"));
  CuAssertIntEquals(tc, GUNNERY_SHIP_C, vessel_find_contact(ship, "Tern"));

  gunnery_clear_ships();
}

void Test_vessel_shipfire_lags_on_a_miss(CuTest *tc)
{
  struct greyhawk_ship_data *ship = &greyhawk_ships[GUNNERY_SHIP_A];
  struct greyhawk_ship_data *target = &greyhawk_ships[GUNNERY_SHIP_B];
  struct gunnery_player gunner;
  struct room_data bridge;
  struct room_data *saved_world;
  room_rnum saved_top_of_world;

  memset(&bridge, 0, sizeof(bridge));
  bridge.number = 100;
  saved_world = world;
  saved_top_of_world = top_of_world;
  world = &bridge;
  top_of_world = 0;
  gunnery_player_init(&gunner, "Corr");
  IN_ROOM(&gunner.ch) = 0;

  gunnery_arm_ship(GUNNERY_SHIP_A, "the Gull", "SA", 0.0, 0.0);
  gunnery_arm_ship(GUNNERY_SHIP_B, "the Tern", "SB", 0.0, 10.0);
  strlcpy(ship->owner, "Corr", sizeof(ship->owner));
  bridge.ship = ship;

  /* Every shot, hit or miss, costs a round, starts the reload, and lags. */
  do_shipfire(&gunner.ch, "0 SB", 0, 0);
  CuAssertIntEquals(tc, vessel_weapon_type(VESSEL_WEAPON_LARGE_BALLISTA)->reload,
                    ship->slot[0].timer);
  CuAssertIntEquals(tc, 29, ship->slot[0].ammo);
  CuAssertIntEquals(tc, PULSE_VIOLENCE, GET_WAIT_STATE(&gunner.ch));
  CuAssertIntEquals(tc, GUNNERY_SHIP_A, target->last_attacker);

  /* A passenger cannot fire at all, so no shot is spent. */
  ship->slot[0].timer = 0;
  GET_WAIT_STATE(&gunner.ch) = 0;
  strlcpy(ship->owner, "Mira", sizeof(ship->owner));
  do_shipfire(&gunner.ch, "0 SB", 0, 0);
  CuAssertIntEquals(tc, 0, ship->slot[0].timer);
  CuAssertIntEquals(tc, 29, ship->slot[0].ammo);
  CuAssertIntEquals(tc, 0, GET_WAIT_STATE(&gunner.ch));

  world = saved_world;
  top_of_world = saved_top_of_world;
  gunnery_clear_ships();
}

void Test_vessel_harbor_hulls_neither_fire_nor_take_fire(CuTest *tc)
{
  struct greyhawk_ship_data *ship = &greyhawk_ships[GUNNERY_SHIP_A];
  struct greyhawk_ship_data *target = &greyhawk_ships[GUNNERY_SHIP_B];
  struct obj_data hull;
  struct room_data berth;
  struct room_data *saved_world;
  room_rnum saved_top_of_world;

  memset(&berth, 0, sizeof(berth));
  berth.number = 100;
  SET_BIT_AR(berth.room_flags, ROOM_DOCKABLE);
  saved_world = world;
  saved_top_of_world = top_of_world;
  world = &berth;
  top_of_world = 0;
  memset(&hull, 0, sizeof(hull));
  IN_ROOM(&hull) = 0;

  gunnery_arm_ship(GUNNERY_SHIP_A, "the Gull", "SA", 0.0, 0.0);
  gunnery_arm_ship(GUNNERY_SHIP_B, "the Tern", "SB", 0.0, 10.0);
  autopilot_init(ship);
  ship->autopilot->pilot_mob_vnum = 1; /* NPC crew returns fire */
  ship->last_attacker = GUNNERY_SHIP_B;

  /* The attacker sits in harbor: the NPC crew holds its fire. */
  target->shipobj = &hull;
  CuAssertTrue(tc, vessel_ship_is_in_port(target));
  vessel_combat_tick_one(ship);
  CuAssertIntEquals(tc, 0, ship->slot[0].timer);
  CuAssertIntEquals(tc, vessel_max_internal(target), vessel_total_internal(target));

  /* The crew's own hull in harbor holds its fire too. */
  target->shipobj = NULL;
  ship->shipobj = &hull;
  vessel_combat_tick_one(ship);
  CuAssertIntEquals(tc, 0, ship->slot[0].timer);

  /* At sea the crew fires. */
  ship->shipobj = NULL;
  vessel_combat_tick_one(ship);
  CuAssertIntEquals(tc, vessel_weapon_type(VESSEL_WEAPON_LARGE_BALLISTA)->reload,
                    ship->slot[0].timer);

  world = saved_world;
  top_of_world = saved_top_of_world;
  autopilot_cleanup(ship);
  gunnery_clear_ships();
}

void Test_vessel_gunnery_dc_follows_the_duris_geometry(CuTest *tc)
{
  const struct vessel_weapon_type *ballista = vessel_weapon_type(VESSEL_WEAPON_LARGE_BALLISTA);
  struct greyhawk_ship_data *ship = &greyhawk_ships[GUNNERY_SHIP_A];
  struct greyhawk_ship_data *target = &greyhawk_ships[GUNNERY_SHIP_B];
  int surface;

  /* Duris's 2d50 roll: a sure shot stays sure, a 63 becomes 74.8%. */
  CuAssertDblEquals(tc, 1.0, vessel_volley_chance(99), 0.0001);
  CuAssertDblEquals(tc, 0.748, vessel_volley_chance(63), 0.0001);
  CuAssertDblEquals(tc, 3.0 / 2500.0, vessel_volley_chance(1), 0.0001);

  /* The calibration table (study 3.3.4): a stopped frigate is DC 1 inside a
   * quarter of the large ballista's range and DC 6 at its maximum. */
  gunnery_arm_ship(GUNNERY_SHIP_A, "the Gull", "SA", 0.0, 0.0);
  gunnery_arm_ship(GUNNERY_SHIP_B, "the Tern", "SB", 0.0, 3.0);
  CuAssertIntEquals(tc, 1, vessel_gunnery_dc(ship, ballista, target));
  target->y = 12.0;
  CuAssertIntEquals(tc, 6, vessel_gunnery_dc(ship, ballista, target));

  /* Crossing the line of fire at full speed puts her near motion 70. */
  target->heading = 90.0;
  target->setheading = 90;
  target->speed = 17.0;
  target->setspeed = 17;
  CuAssertIntEquals(tc, 16, vessel_gunnery_dc(ship, ballista, target));

  /* A hull aloft is half again harder to hit. */
  target->heading = 0.0;
  target->setheading = 0;
  target->speed = 0.0;
  target->setspeed = 0;
  target->y = 8.0;
  surface = vessel_gunnery_dc(ship, ballista, target);
  target->z = 10.0;
  CuAssertTrue(tc, vessel_gunnery_dc(ship, ballista, target) > surface);

  /* d20 + bonus against the DC; a natural 1 misses and a 20 hits. */
  CuAssertIntEquals(tc, 95, vessel_hit_percent(1, 0));
  CuAssertIntEquals(tc, 5, vessel_hit_percent(21, 0));
  CuAssertIntEquals(tc, 65, vessel_hit_percent(11, 3));

  /* The bonus is the gunner's tier plus Dexterity, or Intelligence for a
   * lofted shot, at most +7; an NPC crew fires at +5. */
  ship->guncrew.gunadjust = 6;
  CuAssertIntEquals(tc, 7, vessel_gunnery_bonus(ship, NULL, ballista));
  ship->guncrew.gunadjust = 0;
  CuAssertIntEquals(tc, VESSEL_NPC_GUNNERY_BONUS, vessel_gunnery_bonus(ship, NULL, ballista));

  gunnery_clear_ships();
}

/* A gunner aboard her own armed warship at sea, with her output captured. */
struct gunnery_deck
{
  struct gunnery_player gunner;
  struct room_data bridge;
  struct descriptor_data descriptor;
  char output[8192];
  struct room_data *saved_world;
  room_rnum saved_top_of_world;
};

static struct greyhawk_ship_data *gunnery_deck_begin(CuTest *tc, struct gunnery_deck *deck)
{
  struct greyhawk_ship_data *ship = &greyhawk_ships[GUNNERY_SHIP_A];

  memset(deck, 0, sizeof(*deck));
  deck->bridge.number = 100;
  deck->saved_world = world;
  deck->saved_top_of_world = top_of_world;
  world = &deck->bridge;
  top_of_world = 0;

  gunnery_player_init(&deck->gunner, "Corr");
  IN_ROOM(&deck->gunner.ch) = 0;
  deck->gunner.ch.desc = &deck->descriptor;
  deck->descriptor.character = &deck->gunner.ch;
  deck->descriptor.output = deck->output;
  deck->descriptor.bufspace = sizeof(deck->output) - 1;
  deck->descriptor.pProtocol = ProtocolCreate();
  CuAssertPtrNotNull(tc, deck->descriptor.pProtocol);

  /* The Gull heads north with ballistae fore, port, and starboard; the
   * Tern lies 8 rooms off her port beam. */
  gunnery_arm_ship(GUNNERY_SHIP_A, "the Gull", "SA", 0.0, 0.0);
  vessel_set_weapon(&ship->slot[1], VESSEL_WEAPON_LARGE_BALLISTA, GREYHAWK_PORT);
  vessel_set_weapon(&ship->slot[2], VESSEL_WEAPON_LARGE_BALLISTA, GREYHAWK_STARBOARD);
  strlcpy(ship->owner, "Corr", sizeof(ship->owner));
  ship->num_rooms = 1;
  ship->room_vnums[0] = 100;
  deck->bridge.ship = ship;
  deck->bridge.people = &deck->gunner.ch;
  gunnery_arm_ship(GUNNERY_SHIP_B, "the Tern", "SB", -8.0, 0.0);
  return ship;
}

static const char *gunnery_deck_command(struct gunnery_deck *deck, ACMD_DECL((*command)),
                                        const char *argument)
{
  memset(deck->output, 0, sizeof(deck->output));
  deck->descriptor.bufptr = 0;
  deck->descriptor.bufspace = sizeof(deck->output) - 1;
  GET_WAIT_STATE(&deck->gunner.ch) = 0;
  command(&deck->gunner.ch, argument, 0, 0);
  return deck->output;
}

static void gunnery_deck_end(struct gunnery_deck *deck)
{
  ProtocolDestroy(deck->descriptor.pProtocol);
  world = deck->saved_world;
  top_of_world = deck->saved_top_of_world;
  gunnery_clear_ships();
}

void Test_vessel_arc_fire_answers_the_lock(CuTest *tc)
{
  struct gunnery_deck deck;
  struct greyhawk_ship_data *ship;
  struct greyhawk_ship_data *target = &greyhawk_ships[GUNNERY_SHIP_B];
  const char *output;

  ship = gunnery_deck_begin(tc, &deck);

  /* Naming a contact locks onto her and calls battle stations; the
   * starboard battery cannot bear on a hull to port. */
  output = gunnery_deck_command(&deck, do_shipfire, "starboard SB");
  CuAssertIntEquals(tc, GUNNERY_SHIP_B, ship->lock_target);
  CuAssertIntEquals(tc, VESSEL_BATTLE_STATIONS_TICKS, ship->battle_ticks);
  CuAssertTrue(tc, strstr(output, "No weapon on the starboard arc can fire at the Tern") != NULL);
  CuAssertIntEquals(tc, 0, GET_WAIT_STATE(&deck.gunner.ch));
  CuAssertIntEquals(tc, 30, ship->slot[2].ammo);

  /* The port arc fires at the locked contact: only the weapon that bears
   * spends a round and reloads, and both crews go to battle stations. */
  output = gunnery_deck_command(&deck, do_shipfire, "port");
  CuAssertTrue(tc, strstr(output, "The port Large Ballista FIRES at the Tern! Chance to hit: ") !=
                       NULL);
  CuAssertIntEquals(tc, 29, ship->slot[1].ammo);
  CuAssertIntEquals(tc, 40, ship->slot[1].timer);
  CuAssertIntEquals(tc, 30, ship->slot[0].ammo);
  CuAssertIntEquals(tc, 30, ship->slot[2].ammo);
  CuAssertIntEquals(tc, PULSE_VIOLENCE, GET_WAIT_STATE(&deck.gunner.ch));
  CuAssertIntEquals(tc, VESSEL_BATTLE_STATIONS_TICKS, target->battle_ticks);
  CuAssertIntEquals(tc, GUNNERY_SHIP_A, target->last_attacker);

  output = gunnery_deck_command(&deck, do_shipfire, "1");
  CuAssertTrue(tc, strstr(output, "still reloading (20 seconds)") != NULL);
  output = gunnery_deck_command(&deck, do_shipfire, "0");
  CuAssertTrue(tc, strstr(output, "fore Large Ballista cannot bear - the Tern lies off your port "
                                  "arc") != NULL);

  /* A veteran gunner reloads 6.75% faster. */
  ship->slot[1].timer = 0;
  ship->crew_tier[CREW_GUNNER] = CREW_TIER_VETERAN;
  gunnery_deck_command(&deck, do_shipfire, "1");
  CuAssertIntEquals(tc, 37, ship->slot[1].timer);

  /* With the lock cleared there is nothing to fire at. */
  gunnery_deck_command(&deck, do_shiplock, "off");
  CuAssertIntEquals(tc, 0, ship->lock_target);
  output = gunnery_deck_command(&deck, do_shipfire, "port");
  CuAssertTrue(tc, strstr(output, "not locked on anything") != NULL);

  /* Anchored or submerged, the guns stay silent. */
  ship->anchored = TRUE;
  output = gunnery_deck_command(&deck, do_shipfire, "port SB");
  CuAssertTrue(tc, strstr(output, "weigh anchor before opening fire") != NULL);
  ship->anchored = FALSE;
  ship->z = -20.0;
  output = gunnery_deck_command(&deck, do_shiplock, "SB");
  CuAssertTrue(tc, strstr(output, "must surface") != NULL);
  ship->z = 0.0;
  target->z = -20.0;
  output = gunnery_deck_command(&deck, do_shiplock, "SB");
  CuAssertTrue(tc, strstr(output, "submerged, beyond your guns") != NULL);
  CuAssertIntEquals(tc, 0, ship->lock_target);

  gunnery_deck_end(&deck);
}

void Test_vessel_battle_stations_outlast_the_lock(CuTest *tc)
{
  struct gunnery_deck deck;
  struct greyhawk_ship_data *ship;
  struct greyhawk_ship_data *target = &greyhawk_ships[GUNNERY_SHIP_B];
  int tick;

  ship = gunnery_deck_begin(tc, &deck);
  gunnery_deck_command(&deck, do_shiplock, "SB");
  CuAssertIntEquals(tc, GUNNERY_SHIP_B, ship->lock_target);

  /* Battle stations hold while the lock does. */
  for (tick = 0; tick < 10; tick++)
  {
    vessel_gunnery_tick_one(ship);
  }
  CuAssertIntEquals(tc, VESSEL_BATTLE_STATIONS_TICKS, ship->battle_ticks);
  CuAssertTrue(tc, vessel_at_battle_stations(ship));

  /* The lock drops when she passes out of sight; the crew stands down
   * 180 s later. */
  target->y = (double)(vessel_sight_range(ship) + 5);
  vessel_gunnery_tick_one(ship);
  CuAssertIntEquals(tc, 0, ship->lock_target);
  for (tick = 1; tick < VESSEL_BATTLE_STATIONS_TICKS; tick++)
  {
    vessel_gunnery_tick_one(ship);
  }
  CuAssertIntEquals(tc, 0, ship->battle_ticks);
  CuAssertTrue(tc, !vessel_at_battle_stations(ship));

  gunnery_deck_end(&deck);
}

void Test_vessel_sight_and_scan_read_the_contact(CuTest *tc)
{
  struct gunnery_deck deck;
  struct greyhawk_ship_data *target = &greyhawk_ships[GUNNERY_SHIP_B];
  const char *output;

  gunnery_deck_begin(tc, &deck);
  output = gunnery_deck_command(&deck, do_shipsight, "");
  CuAssertTrue(tc, strstr(output, "not locked on anything") != NULL);

  /* Each weapon's DC and chance, or why it cannot fire. */
  gunnery_deck_command(&deck, do_shiplock, "SB");
  output = gunnery_deck_command(&deck, do_shipsight, "");
  CuAssertTrue(tc, strstr(output, "Sighting [SB] the Tern, 8.0 rooms off your port arc:") != NULL);
  CuAssertTrue(tc, strstr(output, "Slot 1, port Large Ballista: DC ") != NULL);
  CuAssertTrue(tc, strstr(output, "fore Large Ballista cannot bear") != NULL);

  /* A scan within 20 rooms shows her sides, weapons, and condition. */
  target->slot[0].damage = VESSEL_WEAPON_DESTROYED;
  output = gunnery_deck_command(&deck, do_shipscan, "SB");
  CuAssertTrue(tc, strstr(output, "[SB] the Tern, a Warship") != NULL);
  CuAssertTrue(tc, strstr(output, "Armor/structure: fore 32/32 14/14, port 40/40 17/17") != NULL);
  CuAssertTrue(tc, strstr(output, "Weapons: fore Large Ballista (destroyed)") != NULL);
  CuAssertTrue(tc, strstr(output, "Condition: sound") != NULL);
  target->x = -25.0;
  output = gunnery_deck_command(&deck, do_shipscan, "SB");
  CuAssertTrue(tc, strstr(output, "too far off to make out; close within 20 rooms") != NULL);

  gunnery_deck_end(&deck);
}

static bool gunnery_open_sea(struct greyhawk_ship_data *ship, int x, int y, int z)
{
  (void)ship;
  (void)x;
  (void)y;
  (void)z;
  return TRUE;
}

void Test_vessel_mind_blast_stuns_the_crew(CuTest *tc)
{
  struct gunnery_deck deck;
  struct greyhawk_ship_data *ship;
  struct greyhawk_ship_data *target = &greyhawk_ships[GUNNERY_SHIP_B];
  const char *output;
  int shot;

  /* The Gull's bow blast cannon fires on the Tern 4 rooms ahead until it
   * lands: (20 - 4) / 20 of the way from its 20-room maximum to point blank
   * stuns her crew for 5 + 15 * 0.8 = 17 s. */
  ship = gunnery_deck_begin(tc, &deck);
  vessel_set_weapon(&ship->slot[0], VESSEL_WEAPON_MIND_BLAST, GREYHAWK_FORE);
  target->x = 0.0;
  target->y = 4.0;
  for (shot = 0; shot < 20 && target->stun_ticks == 0; shot++)
  {
    ship->slot[0].timer = 0;
    gunnery_deck_command(&deck, do_shipfire, "0 SB");
  }
  CuAssertIntEquals(tc, 34, target->stun_ticks);
  CuAssertIntEquals(tc, vessel_max_internal(target), vessel_total_internal(target));

  /* A stunned crew neither reloads nor answers the helm. */
  target->slot[0].timer = 10;
  vessel_gunnery_tick_one(target);
  CuAssertIntEquals(tc, 10, target->slot[0].timer);
  CuAssertIntEquals(tc, 33, target->stun_ticks);
  target->setspeed = 17;
  vessel_sail_tick(target, 17.0, gunnery_open_sea, NULL, NULL);
  CuAssertDblEquals(tc, 0.0, target->speed, 0.0001);

  /* Nor fires or repairs. */
  ship->stun_ticks = 2;
  output = gunnery_deck_command(&deck, do_shipfire, "port SB");
  CuAssertTrue(tc, strstr(output, "reels from a mental blast and cannot work the guns") != NULL);
  output = gunnery_deck_command(&deck, do_shiprepair, "");
  CuAssertTrue(tc, strstr(output, "nobody can hold a tool steady") != NULL);

  /* The shock passes and the reload resumes. */
  target->stun_ticks = 1;
  vessel_gunnery_tick_one(target);
  CuAssertIntEquals(tc, 0, target->stun_ticks);
  CuAssertIntEquals(tc, 9, target->slot[0].timer);

  gunnery_deck_end(&deck);
}
