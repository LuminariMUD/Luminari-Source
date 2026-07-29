#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/vessels.h"

#include <stdlib.h>
#include <string.h>

extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];

void Test_vessel_hull_keywords_split_readable_name_tokens(CuTest *tc)
{
  char keywords[128];

  vessel_build_hull_keywords(keywords, sizeof(keywords), "Persistence_Dinghy");
  CuAssertTrue(tc, isname("Persistence_Dinghy", keywords));
  CuAssertTrue(tc, isname("persistence", keywords));
  CuAssertTrue(tc, isname("dinghy", keywords));
  CuAssertTrue(tc, isname("ship", keywords));

  vessel_build_hull_keywords(keywords, sizeof(keywords), "The Sea-Witch");
  CuAssertTrue(tc, isname("sea-witch", keywords));
  CuAssertTrue(tc, isname("witch", keywords));
}

void Test_vessel_managed_hulls_survive_zone_cleanup(CuTest *tc)
{
  const int slot = 487;
  struct greyhawk_ship_data *ship = &greyhawk_ships[slot];
  struct obj_data hull;

  memset(ship, 0, sizeof(*ship));
  memset(&hull, 0, sizeof(hull));
  ship->active = TRUE;
  ship->shipnum = slot;
  GET_OBJ_TYPE(&hull) = ITEM_GREYHAWK_SHIP;
  GET_OBJ_VAL(&hull, 1) = slot;

  CuAssertTrue(tc, !vessel_hull_is_managed(&hull));
  ship->shipobj = &hull;
  CuAssertTrue(tc, vessel_hull_is_managed(&hull));

  GET_OBJ_VAL(&hull, 1) = slot - 1;
  CuAssertTrue(tc, !vessel_hull_is_managed(&hull));

  memset(ship, 0, sizeof(*ship));
}

void Test_vehicle_production_lifecycle_and_lookup(CuTest *tc)
{
  struct vehicle_data *vehicle;
  int vehicle_id;

  vehicle = vehicle_create(VEHICLE_CART, "coverage cart");
  CuAssertPtrNotNull(tc, vehicle);

  vehicle_id = vehicle->id;
  CuAssertTrue(tc, vehicle_id > 0);
  CuAssertIntEquals(tc, VEHICLE_CART, vehicle->type);
  CuAssertStrEquals(tc, "coverage cart", vehicle->name);
  CuAssertIntEquals(tc, VEHICLE_PASSENGERS_CART, vehicle->max_passengers);
  CuAssertPtrEquals(tc, vehicle, vehicle_find_by_id(vehicle_id));

  vehicle_destroy(vehicle);
  CuAssertPtrEquals(tc, NULL, vehicle_find_by_id(vehicle_id));
}

void Test_vehicle_production_capacity_and_state_transitions(CuTest *tc)
{
  struct vehicle_data *vehicle;

  vehicle = vehicle_create(VEHICLE_CART, NULL);
  CuAssertPtrNotNull(tc, vehicle);

  CuAssertIntEquals(tc, VSTATE_IDLE, vehicle_get_state(vehicle));
  CuAssertTrue(tc, vehicle_add_passenger(vehicle));
  CuAssertIntEquals(tc, VSTATE_LOADED, vehicle_get_state(vehicle));
  CuAssertTrue(tc, vehicle_add_passenger(vehicle));
  CuAssertTrue(tc, !vehicle_add_passenger(vehicle));
  CuAssertTrue(tc, vehicle_remove_passenger(vehicle));
  CuAssertTrue(tc, vehicle_remove_passenger(vehicle));
  CuAssertIntEquals(tc, VSTATE_IDLE, vehicle_get_state(vehicle));

  CuAssertTrue(tc, vehicle_add_weight(vehicle, VEHICLE_WEIGHT_CART));
  CuAssertTrue(tc, !vehicle_add_weight(vehicle, 1));
  CuAssertTrue(tc, vehicle_remove_weight(vehicle, VEHICLE_WEIGHT_CART));
  CuAssertIntEquals(tc, VSTATE_IDLE, vehicle_get_state(vehicle));

  vehicle_destroy(vehicle);
}

void Test_vehicle_production_damage_and_repair(CuTest *tc)
{
  struct vehicle_data *vehicle;

  vehicle = vehicle_create(VEHICLE_WAGON, "coverage wagon");
  CuAssertPtrNotNull(tc, vehicle);

  CuAssertIntEquals(tc, VEHICLE_CONDITION_BROKEN,
                    vehicle_damage(vehicle, VEHICLE_CONDITION_MAX + 1));
  CuAssertIntEquals(tc, VSTATE_DAMAGED, vehicle_get_state(vehicle));
  CuAssertTrue(tc, !vehicle_is_operational(vehicle));
  CuAssertTrue(tc, !vehicle_set_state(vehicle, VSTATE_MOVING));

  CuAssertIntEquals(tc, VEHICLE_CONDITION_GOOD, vehicle_repair(vehicle, VEHICLE_CONDITION_GOOD));
  CuAssertIntEquals(tc, VSTATE_IDLE, vehicle_get_state(vehicle));
  CuAssertTrue(tc, vehicle_is_operational(vehicle));

  vehicle_destroy(vehicle);
}

void Test_vehicle_production_direction_and_terrain_helpers(CuTest *tc)
{
  struct vehicle_data mount;
  int dx;
  int dy;

  memset(&mount, 0, sizeof(mount));
  vehicle_init(&mount, VEHICLE_MOUNT);

  vehicle_get_direction_delta(NORTHEAST, &dx, &dy);
  CuAssertIntEquals(tc, 1, dx);
  CuAssertIntEquals(tc, 1, dy);

  vehicle_get_direction_delta(UP, &dx, &dy);
  CuAssertIntEquals(tc, 0, dx);
  CuAssertIntEquals(tc, 0, dy);

  CuAssertIntEquals(tc, VTERRAIN_ROAD, sector_to_vterrain(SECT_ROAD_NS));
  CuAssertIntEquals(tc, VTERRAIN_ROAD, sector_to_vterrain(SECT_SEAPORT));
  CuAssertIntEquals(tc, 0, sector_to_vterrain(SECT_OCEAN));
  CuAssertTrue(tc, vehicle_can_traverse_terrain(&mount, SECT_FOREST));
  CuAssertTrue(tc, !vehicle_can_traverse_terrain(&mount, SECT_OCEAN));
  CuAssertIntEquals(tc, VEHICLE_SPEED_MOD_FOREST, get_vehicle_speed_modifier(&mount, SECT_FOREST));
}

void Test_vessel_production_geometry_and_type_data(CuTest *tc)
{
  const struct vessel_terrain_caps *caps;

  CuAssertIntEquals(tc, 0, greyhawk_bearing(0.0f, 0.0f, 0.0f, 1.0f));
  CuAssertIntEquals(tc, 90, greyhawk_bearing(0.0f, 0.0f, 1.0f, 0.0f));
  CuAssertIntEquals(tc, 180, greyhawk_bearing(0.0f, 1.0f, 0.0f, 0.0f));
  CuAssertIntEquals(tc, 270, greyhawk_bearing(1.0f, 0.0f, 0.0f, 0.0f));
  CuAssertDblEquals(tc, 13.0, greyhawk_range(0.0f, 0.0f, 0.0f, 3.0f, 4.0f, 12.0f), 0.001);

  CuAssertStrEquals(tc, "Airship", get_vessel_type_name(VESSEL_AIRSHIP));
  CuAssertStrEquals(tc, "Unknown Vessel", get_vessel_type_name((enum vessel_class)99));

  caps = get_vessel_terrain_caps(VESSEL_SUBMARINE);
  CuAssertPtrNotNull(tc, caps);
  CuAssertTrue(tc, caps->can_traverse_underwater);
  CuAssertTrue(tc, get_terrain_speed_modifier(VESSEL_SUBMARINE, SECT_UNDERWATER, 4) > 0);
  CuAssertIntEquals(tc, 0, get_terrain_speed_modifier(VESSEL_SHIP, -1, 0));
}

void Test_vessel_name_lookup_accepts_player_facing_identifiers(CuTest *tc)
{
  const int slot = 488;
  struct greyhawk_ship_data *ship = &greyhawk_ships[slot];

  memset(ship, 0, sizeof(*ship));
  ship->active = TRUE;
  ship->shipnum = slot;
  strlcpy(ship->name, "The Tern", sizeof(ship->name));
  strlcpy(ship->id, "SU", sizeof(ship->id));

  CuAssertPtrEquals(tc, ship, find_ship_by_name("The Tern"));
  CuAssertPtrEquals(tc, ship, find_ship_by_name("tern"));
  CuAssertPtrEquals(tc, ship, find_ship_by_name("SU"));
  CuAssertPtrEquals(tc, ship, find_ship_by_name("488"));

  memset(ship, 0, sizeof(*ship));
}

void Test_vessel_slot_identity_and_occupancy_are_separate(CuTest *tc)
{
  const int slot = 497;
  struct greyhawk_ship_data *ship = &greyhawk_ships[slot];

  memset(ship, 0, sizeof(*ship));
  ship->shipnum = slot;

  CuAssertTrue(tc, !is_valid_ship(ship));

  ship->active = TRUE;
  CuAssertTrue(tc, is_valid_ship(ship));

  ship->shipnum = slot - 1;
  CuAssertTrue(tc, !is_valid_ship(ship));

  memset(ship, 0, sizeof(*ship));
}

void Test_vessel_debug_categories_are_runtime_selectable(CuTest *tc)
{
  CuAssertIntEquals(tc, VESSEL_DEBUG_CAT_CORE, vessel_debug_category_from_name("core"));
  CuAssertIntEquals(tc, VESSEL_DEBUG_CAT_VEHICLE_MOVE,
                    vessel_debug_category_from_name("vehicle_move"));
  CuAssertIntEquals(tc, VESSEL_DEBUG_CAT_TRANSPORT, vessel_debug_category_from_name("xport"));
  CuAssertIntEquals(tc, VESSEL_DEBUG_CAT_ALL, vessel_debug_category_from_name("all"));
  CuAssertIntEquals(tc, 0, vessel_debug_category_from_name("not-a-category"));

  vessel_debug_mask = VESSEL_DEBUG_CAT_CORE;
#if VESSEL_SYSTEM_DEBUG
  CuAssertTrue(tc, vessel_debug_enabled(VESSEL_DEBUG_CAT_CORE));
  CuAssertTrue(tc, !vessel_debug_enabled(VESSEL_DEBUG_CAT_MOVE));
#else
  CuAssertTrue(tc, !vessel_debug_enabled(VESSEL_DEBUG_CAT_CORE));
#endif
  vessel_debug_mask = 0;
}

void Test_vessel_production_autopilot_lifecycle(CuTest *tc)
{
  struct greyhawk_ship_data ship;
  struct ship_route *route;
  struct waypoint *waypoint;

  memset(&ship, 0, sizeof(ship));
  ship.shipnum = 7;
  route = route_create("coverage route");

  CuAssertPtrNotNull(tc, route);
  CuAssertPtrNotNull(tc, autopilot_init(&ship));
  CuAssertIntEquals(tc, 0, waypoint_add(route, 3.0f, 4.0f, 12.0f, "first"));
  CuAssertIntEquals(tc, 1, waypoint_add(route, 8.0f, 4.0f, 12.0f, "second"));
  CuAssertTrue(tc, autopilot_start(&ship, route));

  waypoint = waypoint_get_current(&ship);
  CuAssertPtrNotNull(tc, waypoint);
  CuAssertStrEquals(tc, "first", waypoint->name);
  CuAssertDblEquals(tc, 13.0, calculate_distance_to_waypoint(&ship, waypoint), 0.001);
  CuAssertTrue(tc, !check_waypoint_arrival(&ship, waypoint));

  CuAssertTrue(tc, autopilot_pause(&ship));
  CuAssertIntEquals(tc, AUTOPILOT_PAUSED, ship.autopilot->state);
  CuAssertTrue(tc, autopilot_resume(&ship));
  CuAssertTrue(tc, advance_to_next_waypoint(&ship));
  CuAssertStrEquals(tc, "second", waypoint_get_current(&ship)->name);
  CuAssertTrue(tc, !advance_to_next_waypoint(&ship));
  CuAssertIntEquals(tc, AUTOPILOT_COMPLETE, ship.autopilot->state);

  CuAssertTrue(tc, autopilot_stop(&ship));
  autopilot_cleanup(&ship);
  route_destroy(route);
  CuAssertPtrEquals(tc, NULL, ship.autopilot);
}

void Test_vessel_production_waypoint_mutation_and_heading(CuTest *tc)
{
  struct greyhawk_ship_data ship;
  struct ship_route *route;
  float dx;
  float dy;

  memset(&ship, 0, sizeof(ship));
  route = route_create(NULL);
  CuAssertPtrNotNull(tc, route);

  CuAssertIntEquals(tc, 0, waypoint_add(route, 3.0f, 4.0f, 0.0f, "first"));
  CuAssertIntEquals(tc, 1, waypoint_add(route, 9.0f, 9.0f, 0.0f, "second"));
  calculate_heading_to_waypoint(&ship, &route->waypoints[0], &dx, &dy);
  CuAssertDblEquals(tc, 0.6, dx, 0.001);
  CuAssertDblEquals(tc, 0.8, dy, 0.001);

  CuAssertTrue(tc, waypoint_remove(route, 0));
  CuAssertStrEquals(tc, "second", route->waypoints[0].name);
  waypoint_clear_all(route);
  CuAssertIntEquals(tc, 0, route->num_waypoints);

  route_destroy(route);
}

void Test_transport_production_capacity_validation(CuTest *tc)
{
  struct greyhawk_ship_data vessel;
  struct vehicle_data vehicle;

  memset(&vessel, 0, sizeof(vessel));
  memset(&vehicle, 0, sizeof(vehicle));
  vessel.shipnum = 5001;
  strlcpy(vessel.name, "coverage vessel", sizeof(vessel.name));
  vehicle_init(&vehicle, VEHICLE_CART);
  vehicle.id = 5001;
  strlcpy(vehicle.name, "coverage vehicle", sizeof(vehicle.name));

  /* A transport freighter easily carries a cart. */
  vessel.vessel_type = VESSEL_TRANSPORT;
  CuAssertTrue(tc, check_vessel_vehicle_capacity(&vessel, &vehicle));

  /* A raft (300 lbs cargo) cannot take a cart (500 lbs structure). */
  vessel.vessel_type = VESSEL_RAFT;
  CuAssertTrue(tc, !check_vessel_vehicle_capacity(&vessel, &vehicle));

  /* Loaded cargo weight counts against the limit too. */
  vessel.vessel_type = VESSEL_BOAT; /* 2000 lbs capacity */
  vehicle.current_weight = 0;
  CuAssertTrue(tc, check_vessel_vehicle_capacity(&vessel, &vehicle));
  vehicle.current_weight = VESSEL_CARGO_BOAT; /* pushes past the limit */
  CuAssertTrue(tc, !check_vessel_vehicle_capacity(&vessel, &vehicle));
  vehicle.current_weight = 0;

  CuAssertTrue(tc, !check_vessel_vehicle_capacity(NULL, &vehicle));
  CuAssertTrue(tc, !check_vessel_vehicle_capacity(&vessel, NULL));
}

void Test_vessel_combat_status_bands(CuTest *tc)
{
  struct greyhawk_ship_data ship;

  memset(&ship, 0, sizeof(ship));
  ship.maxfinternal = ship.maxrinternal = ship.maxpinternal = ship.maxsinternal = 25; /* 100 */

  ship.finternal = ship.rinternal = ship.pinternal = ship.sinternal = 25;
  CuAssertIntEquals(tc, VESSEL_STATUS_SOUND, vessel_status(&ship));

  ship.finternal = ship.rinternal = 25;
  ship.pinternal = ship.sinternal = 0; /* 50% */
  CuAssertIntEquals(tc, VESSEL_STATUS_BATTERED, vessel_status(&ship));

  ship.finternal = 20;
  ship.rinternal = ship.pinternal = ship.sinternal = 0; /* 20% */
  CuAssertIntEquals(tc, VESSEL_STATUS_CRIPPLED, vessel_status(&ship));

  ship.finternal = 0; /* 0% */
  CuAssertIntEquals(tc, VESSEL_STATUS_SINKING, vessel_status(&ship));

  CuAssertStrEquals(tc, "battered", vessel_status_name(VESSEL_STATUS_BATTERED));
}

void Test_vessel_condition_initialization_is_damage_complete(CuTest *tc)
{
  struct greyhawk_ship_data ship;

  memset(&ship, 0, sizeof(ship));
  vessel_initialize_condition(&ship, 100);

  CuAssertIntEquals(tc, 100, ship.farmor);
  CuAssertIntEquals(tc, 100, ship.rarmor);
  CuAssertIntEquals(tc, 100, ship.parmor);
  CuAssertIntEquals(tc, 100, ship.sarmor);
  CuAssertIntEquals(tc, 240, vessel_total_internal(&ship));
  CuAssertIntEquals(tc, 240, vessel_max_internal(&ship));
  CuAssertIntEquals(tc, 20, ship.mainsail);
  CuAssertIntEquals(tc, 20, ship.maxmainsail);
  CuAssertIntEquals(tc, 20, ship.turnrate);
  CuAssertIntEquals(tc, 20, ship.maxturnrate);

  memset(&ship, 0, sizeof(ship));
  vessel_initialize_condition(&ship, 0);
  CuAssertIntEquals(tc, 40, vessel_total_internal(&ship));
  CuAssertIntEquals(tc, 40, vessel_max_internal(&ship));
}

void Test_vessel_runtime_slot_state_round_trip(CuTest *tc)
{
  struct greyhawk_ship_data source;
  struct greyhawk_ship_data restored;
  char serialized[8192];

  memset(&source, 0, sizeof(source));
  memset(&restored, 0, sizeof(restored));
  source.shipnum = 42;
  source.slot[0].type = 1;
  source.slot[0].position = GREYHAWK_PORT;
  source.slot[0].weight = 37;
  source.slot[0].val0 = 50;
  source.slot[0].val1 = 4;
  source.slot[0].val2 = 2;
  source.slot[0].val3 = 8;
  source.slot[0].x = 11;
  source.slot[0].y = 19;
  source.slot[0].timer = 5;
  strlcpy(source.slot[0].desc, "port battery | loaded:yes", sizeof(source.slot[0].desc));
  source.slot[GREYHAWK_MAXSLOTS - 1].type = 3;
  source.slot[GREYHAWK_MAXSLOTS - 1].timer = -2;
  strlcpy(source.slot[GREYHAWK_MAXSLOTS - 1].desc, "reserve bolts",
          sizeof(source.slot[GREYHAWK_MAXSLOTS - 1].desc));

  CuAssertTrue(tc, vessel_serialize_slot_state(&source, serialized, sizeof(serialized)) > 0);
  CuAssertIntEquals(tc, GREYHAWK_MAXSLOTS,
                    vessel_deserialize_slot_state(&restored, serialized));
  CuAssertIntEquals(tc, source.slot[0].type, restored.slot[0].type);
  CuAssertIntEquals(tc, source.slot[0].position, restored.slot[0].position);
  CuAssertIntEquals(tc, source.slot[0].weight, restored.slot[0].weight);
  CuAssertIntEquals(tc, source.slot[0].val0, restored.slot[0].val0);
  CuAssertIntEquals(tc, source.slot[0].val1, restored.slot[0].val1);
  CuAssertIntEquals(tc, source.slot[0].val2, restored.slot[0].val2);
  CuAssertIntEquals(tc, source.slot[0].val3, restored.slot[0].val3);
  CuAssertIntEquals(tc, source.slot[0].x, restored.slot[0].x);
  CuAssertIntEquals(tc, source.slot[0].y, restored.slot[0].y);
  CuAssertIntEquals(tc, source.slot[0].timer, restored.slot[0].timer);
  CuAssertStrEquals(tc, source.slot[0].desc, restored.slot[0].desc);
  CuAssertIntEquals(tc, -2, restored.slot[GREYHAWK_MAXSLOTS - 1].timer);
  CuAssertStrEquals(tc, "reserve bolts", restored.slot[GREYHAWK_MAXSLOTS - 1].desc);
}

void Test_vessel_combat_firing_arcs(CuTest *tc)
{
  /* Use high fleet slots so nothing else in the suite collides. */
  const int A = 490, B = 491;

  memset(&greyhawk_ships[A], 0, sizeof(greyhawk_ships[A]));
  memset(&greyhawk_ships[B], 0, sizeof(greyhawk_ships[B]));
  greyhawk_ships[A].x = 0.0f;
  greyhawk_ships[A].y = 0.0f;
  greyhawk_ships[A].heading = 0; /* facing north (+y) */

  greyhawk_ships[B].x = 0.0f;
  greyhawk_ships[B].y = 10.0f; /* due north */
  CuAssertIntEquals(tc, GREYHAWK_FORE, greyhawk_getarc(A, B));

  greyhawk_ships[B].x = 10.0f;
  greyhawk_ships[B].y = 0.0f; /* due east */
  CuAssertIntEquals(tc, GREYHAWK_STARBOARD, greyhawk_getarc(A, B));

  greyhawk_ships[B].x = -10.0f;
  greyhawk_ships[B].y = 0.0f; /* due west */
  CuAssertIntEquals(tc, GREYHAWK_PORT, greyhawk_getarc(A, B));

  greyhawk_ships[B].x = 0.0f;
  greyhawk_ships[B].y = -10.0f; /* due south */
  CuAssertIntEquals(tc, GREYHAWK_REAR, greyhawk_getarc(A, B));

  /* Heading east flips north to the port arc */
  greyhawk_ships[A].heading = 90;
  greyhawk_ships[B].x = 0.0f;
  greyhawk_ships[B].y = 10.0f;
  CuAssertIntEquals(tc, GREYHAWK_PORT, greyhawk_getarc(A, B));

  memset(&greyhawk_ships[A], 0, sizeof(greyhawk_ships[A]));
  memset(&greyhawk_ships[B], 0, sizeof(greyhawk_ships[B]));
}

void Test_vessel_combat_damage_and_sinking(CuTest *tc)
{
  const int S = 492;
  struct greyhawk_ship_data *ship = &greyhawk_ships[S];

  memset(ship, 0, sizeof(*ship));
  ship->active = TRUE;
  ship->shipnum = S;
  strlcpy(ship->name, "test target", sizeof(ship->name));
  ship->maxfarmor = ship->farmor = 10;
  ship->maxfinternal = ship->finternal = 20;
  ship->maxrinternal = ship->rinternal = 20;
  ship->maxpinternal = ship->pinternal = 20;
  ship->maxsinternal = ship->sinternal = 20;
  ship->maxmainsail = ship->mainsail = 20;
  ship->maxturnrate = ship->turnrate = 20;

  /* Armor absorbs fully: 6 damage vs 10 armor */
  vessel_apply_damage(S, 6, GREYHAWK_FORE, "test shot");
  CuAssertIntEquals(tc, 4, ship->farmor);
  CuAssertIntEquals(tc, 20, ship->finternal);

  /* Spill past armor: 10 damage vs 4 armor -> 6 into internal + rigging */
  vessel_apply_damage(S, 10, GREYHAWK_FORE, "test shot");
  CuAssertIntEquals(tc, 0, ship->farmor);
  CuAssertIntEquals(tc, 14, ship->finternal);
  CuAssertTrue(tc, ship->mainsail < 20); /* fore structural hits tear rigging */

  /* Stern hit fouls the rudder */
  vessel_apply_damage(S, 8, GREYHAWK_REAR, "test shot");
  CuAssertTrue(tc, ship->turnrate < 20);

  /* Burn down all internal structure -> ship sinks, slot cleared */
  vessel_apply_damage(S, 100, GREYHAWK_FORE, "test shot");
  vessel_apply_damage(S, 100, GREYHAWK_REAR, "test shot");
  vessel_apply_damage(S, 100, GREYHAWK_PORT, "test shot");
  vessel_apply_damage(S, 100, GREYHAWK_STARBOARD, "test shot");
  CuAssertIntEquals(tc, 0, vessel_total_internal(ship));
  CuAssertTrue(tc, ship->name[0] == '\0'); /* slot memset by vessel_sink */
}

static void duel_arm_ship(struct greyhawk_ship_data *ship, int shipnum, const char *name)
{
  int arc;

  memset(ship, 0, sizeof(*ship));
  ship->active = TRUE;
  ship->shipnum = shipnum;
  strlcpy(ship->name, name, sizeof(ship->name));
  ship->maxfarmor = ship->farmor = 10;
  ship->maxrarmor = ship->rarmor = 10;
  ship->maxparmor = ship->parmor = 10;
  ship->maxsarmor = ship->sarmor = 10;
  ship->maxfinternal = ship->finternal = 15;
  ship->maxrinternal = ship->rinternal = 15;
  ship->maxpinternal = ship->pinternal = 15;
  ship->maxsinternal = ship->sinternal = 15;
  ship->maxmainsail = ship->mainsail = 20;
  ship->maxturnrate = ship->turnrate = 20;

  /* One weapon per arc so bearing never stalls the duel */
  for (arc = 0; arc < 4; arc++)
  {
    ship->slot[arc].type = 1;
    ship->slot[arc].position = (char)arc;
    ship->slot[arc].val0 = 50;
    ship->slot[arc].val2 = 2;
    ship->slot[arc].val3 = 6;
  }

  autopilot_init(ship);
  ship->autopilot->pilot_mob_vnum = 1; /* NPC-piloted: doctrine engages */
}

void Test_vessel_combat_npc_duel_harness(CuTest *tc)
{
  const int A = 493, B = 494;
  int ticks;

  duel_arm_ship(&greyhawk_ships[A], A, "duelist alpha");
  duel_arm_ship(&greyhawk_ships[B], B, "duelist beta");
  greyhawk_ships[A].x = 0.0f;
  greyhawk_ships[A].y = 0.0f;
  greyhawk_ships[B].x = 10.0f;
  greyhawk_ships[B].y = 0.0f;

  /* Open hostilities in both directions */
  greyhawk_ships[A].last_attacker = B;
  greyhawk_ships[B].last_attacker = A;

  /* A duel between evenly matched hulls must end decisively within a
   * bounded number of ticks (balance smoke test: no stalemate, no
   * instant kill). 120 internal per hull, ~7 avg damage per hit. */
  for (ticks = 0; ticks < 2000; ticks++)
  {
    vessel_combat_tick();
    if (greyhawk_ships[A].name[0] == '\0' || greyhawk_ships[B].name[0] == '\0')
    {
      break;
    }
  }

  CuAssertTrue(tc, ticks < 2000); /* someone sank */
  CuAssertTrue(tc, ticks > 5);    /* but not instantly */
  CuAssertTrue(tc, greyhawk_ships[A].name[0] == '\0' || greyhawk_ships[B].name[0] == '\0');

  /* Cleanup whichever survived (autopilot memory) */
  if (greyhawk_ships[A].name[0] != '\0')
  {
    autopilot_cleanup(&greyhawk_ships[A]);
    memset(&greyhawk_ships[A], 0, sizeof(greyhawk_ships[A]));
  }
  if (greyhawk_ships[B].name[0] != '\0')
  {
    autopilot_cleanup(&greyhawk_ships[B]);
    memset(&greyhawk_ships[B], 0, sizeof(greyhawk_ships[B]));
  }
}

void Test_vessel_ownership_helm_permission_matrix(CuTest *tc)
{
  struct greyhawk_ship_data ship;
  struct char_data owner_ch;
  struct char_data crew_ch;
  struct char_data stranger_ch;

  memset(&ship, 0, sizeof(ship));
  memset(&owner_ch, 0, sizeof(owner_ch));
  memset(&crew_ch, 0, sizeof(crew_ch));
  memset(&stranger_ch, 0, sizeof(stranger_ch));
  owner_ch.player.name = strdup("Corr");
  crew_ch.player.name = strdup("Mira");
  stranger_ch.player.name = strdup("Vex");

  /* Unowned ships are free for anyone */
  CuAssertTrue(tc, vessel_helm_permitted(&stranger_ch, &ship));

  /* Owned: only the owner... */
  strlcpy(ship.owner, "Corr", sizeof(ship.owner));
  CuAssertTrue(tc, vessel_helm_permitted(&owner_ch, &ship));
  CuAssertTrue(tc, !vessel_helm_permitted(&crew_ch, &ship));
  CuAssertTrue(tc, !vessel_helm_permitted(&stranger_ch, &ship));

  /* ...and the permit list */
  strlcpy(ship.helm_permits[0], "Mira", sizeof(ship.helm_permits[0]));
  ship.num_permits = 1;
  CuAssertTrue(tc, vessel_helm_permitted(&crew_ch, &ship));
  CuAssertTrue(tc, !vessel_helm_permitted(&stranger_ch, &ship));

  /* Immortals bypass ownership */
  stranger_ch.player.level = LVL_IMMORT;
  CuAssertTrue(tc, vessel_helm_permitted(&stranger_ch, &ship));

  free(owner_ch.player.name);
  free(crew_ch.player.name);
  free(stranger_ch.player.name);
}

void Test_vessel_shipyard_pricing(CuTest *tc)
{
  /* Bigger hulls cost more; investment in armor/speed raises the price */
  CuAssertTrue(tc, vessel_prototype_price(VESSEL_WARSHIP, 20, 40) >
                       vessel_prototype_price(VESSEL_SHIP, 15, 20));
  CuAssertTrue(tc, vessel_prototype_price(VESSEL_SHIP, 15, 40) >
                       vessel_prototype_price(VESSEL_SHIP, 15, 20));
  CuAssertTrue(tc, vessel_prototype_price(VESSEL_SHIP, 25, 20) >
                       vessel_prototype_price(VESSEL_SHIP, 10, 20));

  /* A raft is pocket change; every price is positive */
  CuAssertTrue(tc, vessel_prototype_price(VESSEL_RAFT, 5, 2) < 200);
  CuAssertTrue(tc, vessel_prototype_price(VESSEL_RAFT, 1, 0) > 0);

  /* Invalid class falls back to standard ship pricing */
  CuAssertIntEquals(tc, vessel_prototype_price(VESSEL_SHIP, 10, 10),
                    vessel_prototype_price(-5, 10, 10));
}

void Test_vessel_crew_costs_and_bonuses(CuTest *tc)
{
  struct greyhawk_ship_data ship;

  /* Better crews cost more to hire and to keep */
  CuAssertTrue(tc, vessel_crew_hire_cost(CREW_GUNNER, CREW_TIER_VETERAN) >
                       vessel_crew_hire_cost(CREW_GUNNER, CREW_TIER_GREEN));
  CuAssertTrue(tc, vessel_crew_wage(CREW_GUNNER, CREW_TIER_VETERAN) >
                       vessel_crew_wage(CREW_GUNNER, CREW_TIER_GREEN));
  /* Wages are a fraction of the signing cost, never larger */
  CuAssertTrue(tc, vessel_crew_wage(CREW_BOSUN, CREW_TIER_ABLE) <
                       vessel_crew_hire_cost(CREW_BOSUN, CREW_TIER_ABLE));
  /* Unfilled positions cost nothing */
  CuAssertIntEquals(tc, 0, vessel_crew_wage(CREW_BOSUN, CREW_TIER_NONE));
  CuAssertIntEquals(tc, 0, vessel_crew_hire_cost(-1, CREW_TIER_ABLE));

  /* Hiring feeds the legacy crew-effect fields the game already reads */
  memset(&ship, 0, sizeof(ship));
  ship.vessel_type = VESSEL_SHIP;
  vessel_apply_crew_bonuses(&ship);
  CuAssertIntEquals(tc, 0, ship.guncrew.gunadjust);

  ship.crew_tier[CREW_GUNNER] = CREW_TIER_VETERAN;
  ship.crew_tier[CREW_SAILMASTER] = CREW_TIER_ABLE;
  ship.crew_tier[CREW_BOSUN] = CREW_TIER_GREEN;
  vessel_apply_crew_bonuses(&ship);
  CuAssertTrue(tc, ship.guncrew.gunadjust > 0);
  CuAssertTrue(tc, ship.sailcrew.speedadjust > 0);
  CuAssertTrue(tc, ship.sailcrew.repairspeed > 0);
  CuAssertTrue(tc, ship.guncrew.gunadjust > ship.sailcrew.repairspeed); /* veteran > green */

  /* Quartermaster stows more cargo */
  CuAssertIntEquals(tc, get_vessel_cargo_capacity(VESSEL_SHIP),
                    vessel_effective_cargo_capacity(&ship));
  ship.crew_tier[CREW_QUARTERMASTER] = CREW_TIER_VETERAN;
  CuAssertTrue(tc, vessel_effective_cargo_capacity(&ship) > get_vessel_cargo_capacity(VESSEL_SHIP));
}

void Test_vessel_upgrade_effects(CuTest *tc)
{
  struct greyhawk_ship_data ship;
  int plain_capacity;

  /* Distinct bits, all non-zero, invalid index yields none */
  CuAssertTrue(tc, vessel_upgrade_bit(0) != vessel_upgrade_bit(1));
  CuAssertTrue(tc, vessel_upgrade_bit(2) != vessel_upgrade_bit(3));
  CuAssertIntEquals(tc, 0, vessel_upgrade_bit(99));

  /* Refits scale with hull value but never go free */
  CuAssertTrue(tc, vessel_upgrade_cost(0, VESSEL_WARSHIP) > vessel_upgrade_cost(0, VESSEL_BOAT));
  CuAssertTrue(tc, vessel_upgrade_cost(0, VESSEL_RAFT) >= 100);

  /* The hold refit raises capacity; it stacks with a quartermaster */
  memset(&ship, 0, sizeof(ship));
  ship.vessel_type = VESSEL_SHIP;
  plain_capacity = vessel_effective_cargo_capacity(&ship);
  ship.upgrades = SHIP_UPGRADE_HOLD;
  CuAssertTrue(tc, vessel_effective_cargo_capacity(&ship) > plain_capacity);
  ship.crew_tier[CREW_QUARTERMASTER] = CREW_TIER_VETERAN;
  CuAssertTrue(tc, vessel_effective_cargo_capacity(&ship) > plain_capacity + plain_capacity / 4);
}

void Test_vessel_trade_price_bounds(CuTest *tc)
{
  int base = 100;
  int scarce;
  int glut;
  int supply;

  /* Scarcity raises price, glut lowers it, baseline is neutral */
  CuAssertIntEquals(tc, base, vessel_commodity_price(base, TRADE_BASELINE_SUPPLY));
  scarce = vessel_commodity_price(base, TRADE_SUPPLY_MIN);
  glut = vessel_commodity_price(base, TRADE_SUPPLY_MAX);
  CuAssertTrue(tc, scarce > base);
  CuAssertTrue(tc, glut < base);

  /* The swing is hard-bounded: no supply value escapes +/- TRADE_MAX_DRIFT.
   * This is the anti-arbitrage guarantee, so sweep the whole domain and
   * well past its clamps. */
  for (supply = -500; supply <= 2000; supply += 7)
  {
    int price = vessel_commodity_price(base, supply);
    CuAssertTrue(tc, price >= base - (base * TRADE_MAX_DRIFT) / 100);
    CuAssertTrue(tc, price <= base + (base * TRADE_MAX_DRIFT) / 100);
    CuAssertTrue(tc, price >= 1);
  }

  /* Prices are monotonic in scarcity, so routes read sensibly */
  CuAssertTrue(tc, vessel_commodity_price(base, 50) > vessel_commodity_price(base, 150));

  /* Cheap goods never price to zero or below */
  CuAssertTrue(tc, vessel_commodity_price(1, TRADE_SUPPLY_MAX) >= 1);
  CuAssertTrue(tc, vessel_commodity_price(0, TRADE_SUPPLY_MAX) >= 1);
}

void Test_vessel_trade_cargo_weight(CuTest *tc)
{
  struct greyhawk_ship_data ship;

  memset(&ship, 0, sizeof(ship));
  ship.vessel_type = VESSEL_SHIP;

  /* Empty hold weighs nothing; NULL is safe */
  CuAssertIntEquals(tc, 0, vessel_cargo_weight(&ship));
  CuAssertIntEquals(tc, 0, vessel_cargo_weight(NULL));

  /* Unknown commodity ids contribute nothing rather than garbage weight
   * (the commodity cache is empty without a database). */
  ship.cargo[0].commodity_id = 4242;
  ship.cargo[0].quantity = 10;
  CuAssertIntEquals(tc, 0, vessel_cargo_weight(&ship));
}

void Test_vessel_hazard_lookout_and_sight(CuTest *tc)
{
  struct greyhawk_ship_data ship;
  int base_range;

  memset(&ship, 0, sizeof(ship));
  ship.vessel_type = VESSEL_SHIP;

  /* No crew, no lookout bonus; NULL is safe */
  CuAssertIntEquals(tc, 0, vessel_lookout_bonus(&ship));
  CuAssertIntEquals(tc, 0, vessel_lookout_bonus(NULL));
  CuAssertIntEquals(tc, VESSEL_SIGHT_CLEAR, vessel_sight_range(NULL));

  /* A posted lookout extends sight beyond the unaided range */
  base_range = vessel_sight_range(&ship);
  ship.crew_tier[CREW_SAILMASTER] = CREW_TIER_VETERAN;
  CuAssertTrue(tc, vessel_lookout_bonus(&ship) > 0);
  CuAssertTrue(tc, vessel_sight_range(&ship) > base_range);

  /* Fog must close the horizon relative to clear weather */
  CuAssertTrue(tc, VESSEL_SIGHT_FOG < VESSEL_SIGHT_CLEAR);
}

void Test_vessel_hazard_submerged_shelters_from_weather(CuTest *tc)
{
  struct greyhawk_ship_data sub;
  struct greyhawk_ship_data surface;

  memset(&sub, 0, sizeof(sub));
  memset(&surface, 0, sizeof(surface));

  /* A submerged submarine rides out surface weather entirely, whatever the
   * weather field says at its coordinates. */
  sub.vessel_type = VESSEL_SUBMARINE;
  sub.z = -50;
  CuAssertIntEquals(tc, 0, vessel_storm_severity(&sub));

  /* Surfaced, it is exposed like anything else (severity depends on the
   * live weather field, so only assert it is a valid band). */
  sub.z = 0;
  CuAssertTrue(tc, vessel_storm_severity(&sub) >= 0);
  CuAssertTrue(tc, vessel_storm_severity(&sub) <= 3);

  surface.vessel_type = VESSEL_SHIP;
  CuAssertTrue(tc, vessel_storm_severity(&surface) >= 0);
  CuAssertTrue(tc, vessel_storm_severity(&surface) <= 3);
  CuAssertIntEquals(tc, 0, vessel_storm_severity(NULL));
}

void Test_vessel_pvp_consent_gate(CuTest *tc)
{
  struct greyhawk_ship_data ship;
  struct char_data attacker;
  struct char_data staff;

  memset(&ship, 0, sizeof(ship));
  memset(&attacker, 0, sizeof(attacker));
  memset(&staff, 0, sizeof(staff));
  attacker.player.name = strdup("Vex");
  staff.player.name = strdup("Zusuk");
  staff.player.level = LVL_IMMORT;
  strlcpy(ship.name, "the Gull", sizeof(ship.name));

  /* Fail closed on bad input */
  CuAssertTrue(tc, !vessel_pvp_permitted(NULL, &ship, FALSE));
  CuAssertTrue(tc, !vessel_pvp_permitted(&attacker, NULL, FALSE));

  /* Unowned hulls (test vessels, unclaimed NPC ferries) are fair game */
  CuAssertTrue(tc, vessel_pvp_permitted(&attacker, &ship, FALSE));

  /* Your own hull is always actionable */
  strlcpy(ship.owner, "Vex", sizeof(ship.owner));
  CuAssertTrue(tc, vessel_pvp_permitted(&attacker, &ship, FALSE));

  /* An owner who is not logged in cannot consent, so their ship is
   * protected - this is the hole that let a moored ship be sunk, plundered,
   * or claimed while its owner slept. */
  strlcpy(ship.owner, "Corr", sizeof(ship.owner));
  CuAssertTrue(tc, !vessel_pvp_permitted(&attacker, &ship, FALSE));

  /* Staff can always act, for testing and intervention */
  CuAssertTrue(tc, vessel_pvp_permitted(&staff, &ship, FALSE));

  free(attacker.player.name);
  free(staff.player.name);
}

void Test_vessel_pvp_logout_grace_is_bounded_and_opponent_specific(CuTest *tc)
{
  struct greyhawk_ship_data ship;
  time_t now;

  memset(&ship, 0, sizeof(ship));
  now = time(0);
  strlcpy(ship.pvp_grace_attacker, "Vex", sizeof(ship.pvp_grace_attacker));
  ship.pvp_grace_until = now + VESSEL_PVP_LOGOUT_GRACE;

  CuAssertTrue(tc, vessel_pvp_grace_active(&ship, "Vex", now));
  CuAssertTrue(tc, vessel_pvp_grace_active(&ship, "vex", now));
  CuAssertTrue(tc, !vessel_pvp_grace_active(&ship, "Corr", now));
  CuAssertTrue(tc, !vessel_pvp_grace_active(&ship, "Vex",
                                            ship.pvp_grace_until + 1));

  vessel_clear_pvp_grace(&ship);
  CuAssertIntEquals(tc, 0, (int)ship.pvp_grace_until);
  CuAssertTrue(tc, ship.pvp_grace_attacker[0] == '\0');

  /* The persisted consent snapshot must not break the 5 KiB base budget. */
  CuAssertTrue(tc, sizeof(struct greyhawk_ship_data) <= 5 * 1024);
}

void Test_vessel_dock_fee_is_one_charge_per_owned_port_visit(CuTest *tc)
{
  struct greyhawk_ship_data ship;

  memset(&ship, 0, sizeof(ship));
  ship.vessel_type = VESSEL_TRANSPORT;
  strlcpy(ship.owner, "Kohdee", sizeof(ship.owner));

  CuAssertIntEquals(tc, 35, vessel_dock_fee_for_class(VESSEL_TRANSPORT));
  CuAssertIntEquals(tc, 25, vessel_dock_fee_for_class((enum vessel_class)-1));
  CuAssertIntEquals(tc, 35, vessel_assess_dock_fee(&ship, 70000, 12));
  CuAssertIntEquals(tc, 35, ship.dock_fee_balance);
  CuAssertIntEquals(tc, 70000, ship.dock_fee_port);
  CuAssertIntEquals(tc, 12, ship.dock_fee_clan);

  /* Repeated transition notices cannot double-assess the occupied berth. */
  CuAssertIntEquals(tc, 0, vessel_assess_dock_fee(&ship, 70000, 12));
  ship.dock_fee_balance = 0;
  CuAssertIntEquals(tc, 0, vessel_assess_dock_fee(&ship, 70000, 12));

  memset(&ship, 0, sizeof(ship));
  ship.vessel_type = VESSEL_WARSHIP;
  CuAssertIntEquals(tc, 0, vessel_assess_dock_fee(&ship, 70000, 12));
}

void Test_vessel_sink_clears_stale_attacker_references(CuTest *tc)
{
  const int VICTIM = 495, AGGRESSOR = 496;

  memset(&greyhawk_ships[VICTIM], 0, sizeof(greyhawk_ships[VICTIM]));
  memset(&greyhawk_ships[AGGRESSOR], 0, sizeof(greyhawk_ships[AGGRESSOR]));

  greyhawk_ships[VICTIM].shipnum = VICTIM;
  greyhawk_ships[VICTIM].active = TRUE;
  strlcpy(greyhawk_ships[VICTIM].name, "victim", sizeof(greyhawk_ships[VICTIM].name));
  greyhawk_ships[AGGRESSOR].shipnum = AGGRESSOR;
  greyhawk_ships[AGGRESSOR].active = TRUE;
  strlcpy(greyhawk_ships[AGGRESSOR].name, "aggressor", sizeof(greyhawk_ships[AGGRESSOR].name));

  /* The victim holds a grudge against the aggressor's fleet slot */
  greyhawk_ships[VICTIM].last_attacker = AGGRESSOR;

  /* Sinking the aggressor must clear that reference, or the slot's next
   * occupant inherits the grudge and gets shot at unprovoked. */
  vessel_sink(AGGRESSOR);
  CuAssertIntEquals(tc, 0, greyhawk_ships[VICTIM].last_attacker);
  CuAssertTrue(tc, greyhawk_ships[AGGRESSOR].name[0] == '\0');

  memset(&greyhawk_ships[VICTIM], 0, sizeof(greyhawk_ships[VICTIM]));
}

void Test_transport_production_cargo_capacity_table(CuTest *tc)
{
  /* Per-class capacities come from the data table. */
  CuAssertIntEquals(tc, VESSEL_CARGO_RAFT, get_vessel_cargo_capacity(VESSEL_RAFT));
  CuAssertIntEquals(tc, VESSEL_CARGO_TRANSPORT, get_vessel_cargo_capacity(VESSEL_TRANSPORT));
  CuAssertIntEquals(tc, VESSEL_CARGO_MAGICAL, get_vessel_cargo_capacity(VESSEL_MAGICAL));

  /* Invalid types fall back to the standard ship capacity. */
  CuAssertIntEquals(tc, VESSEL_CARGO_SHIP, get_vessel_cargo_capacity((enum vessel_class) - 1));
  CuAssertIntEquals(tc, VESSEL_CARGO_SHIP, get_vessel_cargo_capacity((enum vessel_class)99));

  /* Freighters must out-haul warships; every class carries something
   * except the raft, which barely floats its passengers. */
  CuAssertTrue(tc, get_vessel_cargo_capacity(VESSEL_TRANSPORT) >
                       get_vessel_cargo_capacity(VESSEL_WARSHIP));
  CuAssertTrue(tc, get_vessel_cargo_capacity(VESSEL_RAFT) > 0);
}
