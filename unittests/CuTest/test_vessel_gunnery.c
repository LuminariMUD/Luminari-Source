/* Vessel gunnery rules: who may fire, owner consent, harbor immunity, the
 * shared contact list, and command lag on every shot. */

#include "CuTest.h"

#include "conf.h"
#include "../../src/core/sysdep.h"
#include "../../src/core/structs.h"
#include "../../src/core/utils.h"
#include "../../src/core/comm.h"
#include "../../src/core/db.h"
#include "../../src/core/interpreter.h"
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
  vessel_initialize_condition(ship, 40);
  ship->slot[0].type = 1;
  ship->slot[0].position = GREYHAWK_FORE;
  ship->slot[0].val0 = 50;
  ship->slot[0].val2 = 2;
  ship->slot[0].val3 = 8;
}

static void gunnery_clear_ships(void)
{
  memset(&greyhawk_ships[GUNNERY_SHIP_A], 0, sizeof(greyhawk_ships[0]));
  memset(&greyhawk_ships[GUNNERY_SHIP_B], 0, sizeof(greyhawk_ships[0]));
  memset(&greyhawk_ships[GUNNERY_SHIP_C], 0, sizeof(greyhawk_ships[0]));
}

void Test_vessel_manual_move_distance_uses_storm_bands(CuTest *tc)
{
  /* Clear and overcast skies are band 0; the old code read raw weather
   * above 50 of 255 as a storm. */
  CuAssertIntEquals(tc, 0, vessel_weather_severity_from_value(127));
  CuAssertIntEquals(tc, 0, vessel_weather_severity_from_value(VESSEL_WEATHER_CLOUDY));
  CuAssertIntEquals(tc, 1, vessel_weather_severity_from_value(VESSEL_WEATHER_SQUALL));
  CuAssertIntEquals(tc, 2, vessel_weather_severity_from_value(VESSEL_WEATHER_STORM));
  CuAssertIntEquals(tc, 3, vessel_weather_severity_from_value(VESSEL_WEATHER_GALE));

  CuAssertIntEquals(tc, 4, vessel_manual_move_distance(40, 0, 0));
  CuAssertIntEquals(tc, 4, vessel_manual_move_distance(40, 0, 1));
  CuAssertIntEquals(tc, 3, vessel_manual_move_distance(40, 0, 2));
  CuAssertIntEquals(tc, 3, vessel_manual_move_distance(40, 0, 3));
  CuAssertIntEquals(tc, 5, vessel_manual_move_distance(40, 1, 0));
  CuAssertIntEquals(tc, 1, vessel_manual_move_distance(0, 0, 3));

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
  target->speed = 250; /* defense DC 60: every shot misses */
  bridge.ship = ship;

  do_shipfire(&gunner.ch, "0 SB", 0, 0);
  CuAssertIntEquals(tc, VESSEL_WEAPON_RELOAD_TICKS, ship->slot[0].timer);
  CuAssertIntEquals(tc, PULSE_VIOLENCE, GET_WAIT_STATE(&gunner.ch));
  CuAssertIntEquals(tc, vessel_max_internal(target), vessel_total_internal(target));

  /* A passenger cannot fire at all, so no shot is spent. */
  ship->slot[0].timer = 0;
  GET_WAIT_STATE(&gunner.ch) = 0;
  strlcpy(ship->owner, "Mira", sizeof(ship->owner));
  do_shipfire(&gunner.ch, "0 SB", 0, 0);
  CuAssertIntEquals(tc, 0, ship->slot[0].timer);
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
  CuAssertIntEquals(tc, VESSEL_WEAPON_RELOAD_TICKS, ship->slot[0].timer);

  world = saved_world;
  top_of_world = saved_top_of_world;
  autopilot_cleanup(ship);
  gunnery_clear_ships();
}
