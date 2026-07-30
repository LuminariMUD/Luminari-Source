#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/comm.h"
#include "../../src/db.h"
#include "../../src/perfmon.h"
#include "../../src/protocol.h"
#include "../../src/vessels.h"
#include "../../src/wilderness.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];

void Test_vessel_fleet_supports_500_active_slots(CuTest *tc)
{
  int final_room_vnum;

  final_room_vnum = SHIP_INTERIOR_VNUM_BASE +
                    ((GREYHAWK_MAXSHIPS - 1) * MAX_SHIP_ROOMS) +
                    (MAX_SHIP_ROOMS - 1);

  CuAssertIntEquals(tc, 501, GREYHAWK_MAXSHIPS);
  CuAssertIntEquals(tc, 500, GREYHAWK_ACTIVE_SHIP_CAPACITY);
  CuAssertIntEquals(tc, 80019, SHIP_INTERIOR_VNUM_MAX);
  CuAssertIntEquals(tc, SHIP_INTERIOR_VNUM_MAX, final_room_vnum);
}

void Test_vessel_msdp_state_clears_after_disembark(CuTest *tc)
{
  const int slot = 498;
  struct greyhawk_ship_data saved_ship;
  struct greyhawk_ship_data *ship;
  struct room_data test_room;
  struct room_data *saved_world;
  struct descriptor_data descriptor;
  struct char_data character;
  room_rnum saved_top_of_world;
  int aboard_matches;
  int ashore_matches;
  int ashore_dirty;
  int variable;

  saved_ship = greyhawk_ships[slot];
  saved_world = world;
  saved_top_of_world = top_of_world;
  ship = &greyhawk_ships[slot];
  memset(ship, 0, sizeof(*ship));
  memset(&test_room, 0, sizeof(test_room));
  memset(&descriptor, 0, sizeof(descriptor));
  memset(&character, 0, sizeof(character));

  ship->active = TRUE;
  ship->shipnum = slot;
  ship->x = -66;
  ship->y = 92;
  ship->z = 120;
  ship->heading = 3;
  ship->speed = 7;
  strlcpy(ship->name, "Protocol Cutter", sizeof(ship->name));
  vessel_initialize_condition(ship, 60);

  world = &test_room;
  top_of_world = 0;
  world[0].ship = ship;
  descriptor.output = descriptor.small_outbuf;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  descriptor.character = &character;
  descriptor.pProtocol = ProtocolCreate();
  character.desc = &descriptor;
  character.in_room = 0;

  if (descriptor.pProtocol == NULL)
  {
    greyhawk_ships[slot] = saved_ship;
    world = saved_world;
    top_of_world = saved_top_of_world;
    CuFail(tc, "could not initialize the vessel MSDP fixture");
    return;
  }

  vessel_msdp_update(&character);
  aboard_matches =
      !strcmp(descriptor.pProtocol->pVariables[eMSDP_SHIP_NAME]->pValueString,
              "Protocol Cutter") &&
      descriptor.pProtocol->pVariables[eMSDP_SHIP_X]->ValueInt == -66 &&
      descriptor.pProtocol->pVariables[eMSDP_SHIP_Y]->ValueInt == 92 &&
      descriptor.pProtocol->pVariables[eMSDP_SHIP_Z]->ValueInt == 120 &&
      descriptor.pProtocol->pVariables[eMSDP_SHIP_HEADING]->ValueInt == 3 &&
      descriptor.pProtocol->pVariables[eMSDP_SHIP_SPEED]->ValueInt == 7 &&
      descriptor.pProtocol->pVariables[eMSDP_SHIP_HULL]->ValueInt == 160 &&
      descriptor.pProtocol->pVariables[eMSDP_SHIP_HULL_MAX]->ValueInt == 160 &&
      !strcmp(descriptor.pProtocol->pVariables[eMSDP_SHIP_STATUS]->pValueString,
              "sound");

  for (variable = eMSDP_SHIP_NAME; variable <= eMSDP_SHIP_STATUS; variable++)
  {
    descriptor.pProtocol->pVariables[variable]->bDirty = false;
  }
  character.in_room = NOWHERE;
  vessel_msdp_update(&character);

  ashore_matches =
      descriptor.pProtocol->pVariables[eMSDP_SHIP_NAME]->pValueString[0] == '\0' &&
      descriptor.pProtocol->pVariables[eMSDP_SHIP_X]->ValueInt == 0 &&
      descriptor.pProtocol->pVariables[eMSDP_SHIP_Y]->ValueInt == 0 &&
      descriptor.pProtocol->pVariables[eMSDP_SHIP_Z]->ValueInt == 0 &&
      descriptor.pProtocol->pVariables[eMSDP_SHIP_HEADING]->ValueInt == 0 &&
      descriptor.pProtocol->pVariables[eMSDP_SHIP_SPEED]->ValueInt == 0 &&
      descriptor.pProtocol->pVariables[eMSDP_SHIP_HULL]->ValueInt == 0 &&
      descriptor.pProtocol->pVariables[eMSDP_SHIP_HULL_MAX]->ValueInt == 0 &&
      descriptor.pProtocol->pVariables[eMSDP_SHIP_STATUS]->pValueString[0] == '\0';
  ashore_dirty = TRUE;
  for (variable = eMSDP_SHIP_NAME; variable <= eMSDP_SHIP_STATUS; variable++)
  {
    if (!descriptor.pProtocol->pVariables[variable]->bDirty)
    {
      ashore_dirty = FALSE;
    }
  }

  ProtocolDestroy(descriptor.pProtocol);
  greyhawk_ships[slot] = saved_ship;
  world = saved_world;
  top_of_world = saved_top_of_world;

  CuAssertTrue(tc, aboard_matches);
  CuAssertTrue(tc, ashore_matches);
  CuAssertTrue(tc, ashore_dirty);
}

void Test_shiplist_summary_remains_bounded_at_full_capacity(CuTest *tc)
{
  struct greyhawk_ship_data *saved_ships;
  struct room_data test_room;
  struct room_data *saved_world;
  struct descriptor_data descriptor;
  struct char_data character;
  room_rnum saved_top_of_world;
  int saved_overflows;
  int has_capacity;
  int has_detail_header;
  int overflowed;
  int i;

  saved_ships = malloc(sizeof(greyhawk_ships));
  if (saved_ships == NULL)
  {
    CuFail(tc, "could not preserve the fleet fixture");
    return;
  }

  memcpy(saved_ships, greyhawk_ships, sizeof(greyhawk_ships));
  saved_world = world;
  saved_top_of_world = top_of_world;
  saved_overflows = buf_overflows;
  memset(greyhawk_ships, 0, sizeof(greyhawk_ships));
  memset(&test_room, 0, sizeof(test_room));
  memset(&descriptor, 0, sizeof(descriptor));
  memset(&character, 0, sizeof(character));

  for (i = 1; i < GREYHAWK_MAXSHIPS; i++)
  {
    greyhawk_ships[i].active = TRUE;
    greyhawk_ships[i].shipnum = i;
  }

  world = &test_room;
  top_of_world = 0;
  descriptor.output = descriptor.small_outbuf;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  descriptor.character = &character;
  descriptor.pProtocol = ProtocolCreate();
  character.desc = &descriptor;
  if (descriptor.pProtocol == NULL)
  {
    memcpy(greyhawk_ships, saved_ships, sizeof(greyhawk_ships));
    world = saved_world;
    top_of_world = saved_top_of_world;
    free(saved_ships);
    CuFail(tc, "could not initialize the protocol fixture");
    return;
  }

  do_shiplist(&character, "summary", 0, 0);
  has_capacity =
      strstr(descriptor.output, "500 of 500 active fleet slots in use.") != NULL;
  has_detail_header = strstr(descriptor.output, "Slot Name") != NULL;
  overflowed = buf_overflows != saved_overflows;

  ProtocolDestroy(descriptor.pProtocol);
  memcpy(greyhawk_ships, saved_ships, sizeof(greyhawk_ships));
  world = saved_world;
  top_of_world = saved_top_of_world;
  buf_overflows = saved_overflows;
  free(saved_ships);

  CuAssertTrue(tc, has_capacity);
  CuAssertTrue(tc, !has_detail_header);
  CuAssertTrue(tc, !overflowed);
}

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
  CuAssertTrue(tc, ship.autopilot->movement_steps == 0);
  CuAssertTrue(tc, ship.autopilot->waypoint_arrivals == 0);
  CuAssertTrue(tc, ship.autopilot->route_completions == 0);
  CuAssertIntEquals(tc, 0, waypoint_add(route, 3.0f, 4.0f, 12.0f, "first"));
  CuAssertIntEquals(tc, 1, waypoint_add(route, 8.0f, 4.0f, 12.0f, "second"));
  CuAssertTrue(tc, autopilot_start(&ship, route));

  waypoint = waypoint_get_current(&ship);
  CuAssertPtrNotNull(tc, waypoint);
  CuAssertStrEquals(tc, "first", waypoint->name);
  CuAssertDblEquals(tc, 13.0, calculate_distance_to_waypoint(&ship, waypoint), 0.001);
  CuAssertTrue(tc, !check_waypoint_arrival(&ship, waypoint));

  ship.x = waypoint->x;
  ship.y = waypoint->y;
  ship.z = waypoint->z;
  process_traveling_vessel(&ship);
  CuAssertTrue(tc, ship.autopilot->waypoint_arrivals == 1);
  CuAssertStrEquals(tc, "second", waypoint_get_current(&ship)->name);

  CuAssertTrue(tc, autopilot_pause(&ship));
  CuAssertIntEquals(tc, AUTOPILOT_PAUSED, ship.autopilot->state);
  CuAssertTrue(tc, autopilot_resume(&ship));
  CuAssertTrue(tc, !advance_to_next_waypoint(&ship));
  CuAssertIntEquals(tc, AUTOPILOT_COMPLETE, ship.autopilot->state);
  CuAssertTrue(tc, ship.autopilot->route_completions == 1);

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

void Test_vessel_autopilot_rounds_signed_wilderness_coordinates(CuTest *tc)
{
  CuAssertIntEquals(tc, 64, vessel_autopilot_grid_coordinate(63.95f));
  CuAssertIntEquals(tc, 63, vessel_autopilot_grid_coordinate(63.40f));
  CuAssertIntEquals(tc, -64, vessel_autopilot_grid_coordinate(-63.95f));
  CuAssertIntEquals(tc, -63, vessel_autopilot_grid_coordinate(-63.40f));
  CuAssertIntEquals(tc, -64, vessel_autopilot_grid_coordinate(-63.50f));
}

void Test_vessel_autopilot_moves_on_all_three_axes_without_overshoot(CuTest *tc)
{
  struct greyhawk_ship_data ship;
  struct waypoint waypoint;
  int target_x;
  int target_y;
  int target_z;

  memset(&ship, 0, sizeof(ship));
  memset(&waypoint, 0, sizeof(waypoint));

  waypoint.z = 50.0f;
  CuAssertTrue(tc, vessel_autopilot_next_position(&ship, &waypoint, 10.0f, &target_x, &target_y,
                                                  &target_z));
  CuAssertIntEquals(tc, 0, target_x);
  CuAssertIntEquals(tc, 0, target_y);
  CuAssertIntEquals(tc, 10, target_z);

  waypoint.x = 3.0f;
  waypoint.y = 4.0f;
  waypoint.z = 0.0f;
  CuAssertTrue(tc, vessel_autopilot_next_position(&ship, &waypoint, 2.0f, &target_x, &target_y,
                                                  &target_z));
  CuAssertIntEquals(tc, 1, target_x);
  CuAssertIntEquals(tc, 2, target_y);
  CuAssertIntEquals(tc, 0, target_z);

  CuAssertTrue(tc, vessel_autopilot_next_position(&ship, &waypoint, 10.0f, &target_x, &target_y,
                                                  &target_z));
  CuAssertIntEquals(tc, 3, target_x);
  CuAssertIntEquals(tc, 4, target_y);
  CuAssertIntEquals(tc, 0, target_z);

  waypoint.z = 12.0f;
  CuAssertTrue(tc, vessel_autopilot_next_position(&ship, &waypoint, 13.0f, &target_x, &target_y,
                                                  &target_z));
  CuAssertIntEquals(tc, 3, target_x);
  CuAssertIntEquals(tc, 4, target_y);
  CuAssertIntEquals(tc, 12, target_z);
}

void Test_vessel_autopilot_pauses_after_untraversable_waypoint(CuTest *tc)
{
  struct greyhawk_ship_data ship;
  struct ship_route *route;

  memset(&ship, 0, sizeof(ship));
  ship.shipnum = 7;
  ship.vessel_type = VESSEL_SHIP;
  ship.speed = 2;
  ship.setspeed = 2;
  route = route_create("blocked route");

  CuAssertPtrNotNull(tc, route);
  CuAssertPtrNotNull(tc, autopilot_init(&ship));
  CuAssertIntEquals(tc, 0, waypoint_add(route, 0.0f, 0.0f, 10.0f, "invalid altitude"));
  CuAssertTrue(tc, autopilot_start(&ship, route));

  process_traveling_vessel(&ship);

  CuAssertIntEquals(tc, AUTOPILOT_PAUSED, ship.autopilot->state);
  CuAssertIntEquals(tc, 0, ship.speed);
  CuAssertIntEquals(tc, 0, ship.setspeed);

  autopilot_cleanup(&ship);
  route_destroy(route);
}

void Test_vessel_z_axis_enforces_class_and_wilderness_boundaries(CuTest *tc)
{
  CuAssertTrue(tc, vessel_z_within_class_limits(VESSEL_SHIP, 0));
  CuAssertTrue(tc, !vessel_z_within_class_limits(VESSEL_SHIP, 1));
  CuAssertTrue(tc, !vessel_z_within_class_limits(VESSEL_SHIP, -1));

  CuAssertTrue(tc, vessel_z_within_class_limits(VESSEL_AIRSHIP, 500));
  CuAssertTrue(tc, !vessel_z_within_class_limits(VESSEL_AIRSHIP, 501));
  CuAssertTrue(tc, !vessel_z_within_class_limits(VESSEL_AIRSHIP, -1));

  CuAssertTrue(tc, vessel_z_within_class_limits(VESSEL_SUBMARINE, -1));
  CuAssertTrue(tc, !vessel_z_within_class_limits(VESSEL_SUBMARINE, 1));

  CuAssertTrue(tc, vessel_z_within_class_limits(VESSEL_MAGICAL, -1));
  CuAssertTrue(tc, vessel_z_within_class_limits(VESSEL_MAGICAL, 1000));
  CuAssertTrue(tc, !vessel_z_within_class_limits(VESSEL_MAGICAL, 1001));

  CuAssertTrue(tc, vessel_z_allows_sector(VESSEL_SUBMARINE, SECT_OCEAN, -10));
  CuAssertTrue(tc, vessel_z_allows_sector(VESSEL_SUBMARINE, SECT_UD_WATER, -10));
  CuAssertTrue(tc, !vessel_z_allows_sector(VESSEL_SUBMARINE, SECT_FIELD, -10));
  CuAssertTrue(tc, !vessel_z_allows_sector(VESSEL_SUBMARINE, SECT_UNDERWATER, 0));
  CuAssertTrue(tc, vessel_z_allows_sector(VESSEL_AIRSHIP, SECT_MOUNTAIN, 500));
  CuAssertTrue(tc, !vessel_z_allows_sector(VESSEL_AIRSHIP, SECT_CAVE, 500));
  CuAssertTrue(tc, vessel_z_allows_sector(VESSEL_MAGICAL, SECT_OCEAN, -10));
  CuAssertTrue(tc, !vessel_z_allows_sector(VESSEL_MAGICAL, SECT_FIELD, -10));
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

void Test_vessel_trade_marginal_batch_pricing_closes_reversal_cycle(CuTest *tc)
{
  const int base_price = 100;
  const int quantity = TRADE_SUPPLY_MAX - TRADE_SUPPLY_MIN + 10;
  long long batch_cost;
  long long batch_revenue;
  long long flat_cost;
  long long flat_revenue;

  flat_cost =
      (long long)vessel_commodity_price(base_price, TRADE_SUPPLY_MAX) *
      quantity;
  flat_revenue =
      (long long)vessel_commodity_price(base_price, TRADE_SUPPLY_MIN) *
      TRADE_SELL_PERCENT / 100 * quantity;
  CuAssertTrue(tc, flat_revenue > flat_cost);

  batch_cost =
      vessel_trade_buy_cost(base_price, TRADE_SUPPLY_MAX, quantity);
  batch_revenue =
      vessel_trade_sell_revenue(base_price, TRADE_SUPPLY_MIN, quantity);
  CuAssertTrue(tc, batch_cost > 0);
  CuAssertTrue(tc, batch_revenue > 0);
  CuAssertTrue(tc, batch_revenue <= batch_cost);

  CuAssertIntEquals(
      tc, TRADE_SUPPLY_MIN,
      vessel_trade_adjusted_supply(TRADE_SUPPLY_MAX, -quantity));
  CuAssertIntEquals(
      tc, TRADE_SUPPLY_MAX,
      vessel_trade_adjusted_supply(TRADE_SUPPLY_MIN, quantity));
  CuAssertIntEquals(tc, TRADE_SUPPLY_MIN + 5,
                    vessel_trade_restocked_supply(TRADE_SUPPLY_MIN));
  CuAssertIntEquals(tc, TRADE_SUPPLY_MAX - 5,
                    vessel_trade_restocked_supply(TRADE_SUPPLY_MAX));
  CuAssertIntEquals(tc, TRADE_SUPPLY_MIN + 5,
                    vessel_trade_restocked_supply(-1000));
  CuAssertIntEquals(tc, TRADE_SUPPLY_MAX - 5,
                    vessel_trade_restocked_supply(1000));
  CuAssertIntEquals(tc, INT_MAX,
                    vessel_commodity_price(INT_MAX, TRADE_SUPPLY_MIN));
}

void Test_vessel_trade_thousand_trade_simulation(CuTest *tc)
{
  struct vessel_trade_simulation_result result;

  CuAssertTrue(tc, vessel_trade_run_simulation(1000, &result));
  CuAssertIntEquals(tc, 1000, result.requested_trades);
  CuAssertIntEquals(tc, 1000, result.completed_trades);
  CuAssertIntEquals(tc, TRADE_SUPPLY_MIN, result.minimum_supply);
  CuAssertIntEquals(tc, TRADE_SUPPLY_MAX, result.maximum_supply);
  CuAssertTrue(tc, result.adversarial_profit < 0);
  CuAssertTrue(tc, result.profitable_routes > 0);
  CuAssertTrue(tc, result.profitable_routes < result.completed_trades);
  CuAssertTrue(tc, result.finite_route_profit > 0);
  CuAssertTrue(tc, abs(result.equilibrium_source_supply -
                       result.equilibrium_destination_supply) <
                       TRADE_SUPPLY_MAX - TRADE_SUPPLY_MIN);
  CuAssertIntEquals(tc, TRADE_BASELINE_SUPPLY,
                    result.restocked_source_supply);
  CuAssertIntEquals(tc, TRADE_BASELINE_SUPPLY,
                    result.restocked_destination_supply);
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

void Test_vessel_piracy_polygon_matches_spatial_boundaries(CuTest *tc)
{
  const struct vertex square[] = {
      {-70, 78},
      {-60, 78},
      {-60, 96},
      {-70, 96},
      {-70, 78}
  };

  CuAssertTrue(tc, vessel_piracy_point_in_polygon(square, 5, -66, 92));
  CuAssertTrue(tc, !vessel_piracy_point_in_polygon(square, 5, -71, 92));
  CuAssertTrue(tc, !vessel_piracy_point_in_polygon(square, 5, -70, 92));
  CuAssertTrue(tc, !vessel_piracy_point_in_polygon(square, 5, -70, 78));
  CuAssertTrue(tc, !vessel_piracy_point_in_polygon(NULL, 5, -66, 92));
  CuAssertTrue(tc, !vessel_piracy_point_in_polygon(square, 2, -66, 92));
}

void Test_vessel_piracy_resolves_canonical_regions_in_memory(CuTest *tc)
{
  struct vertex western_polygon[] = {
      {0, 0}, {10, 0}, {10, 10}, {0, 10}, {0, 0}
  };
  struct vertex eastern_polygon[] = {
      {20, 0}, {30, 0}, {30, 10}, {20, 10}, {20, 0}
  };
  struct region_data region_fixture[5];
  struct region_data *saved_region_table;
  struct zone_data zone_fixture[2];
  struct zone_data *saved_zone_table;
  struct room_data room_fixture;
  struct room_data *saved_world;
  struct descriptor_data descriptor;
  struct char_data character;
  struct greyhawk_ship_data ship;
  struct vessel_piracy_law law;
  region_rnum saved_top_of_region_table;
  zone_rnum saved_top_of_zone_table;
  room_rnum saved_top_of_world;
  bool western_found;
  bool eastern_found;
  bool outside_found;
  bool repeated_message_suppressed;
  bool crossing_announced;
  bool open_water_announced;
  int western_region;
  int eastern_region;
  int initialized_region;
  int repeated_region;
  int crossed_region;
  int outside_region;

  memset(region_fixture, 0, sizeof(region_fixture));
  memset(zone_fixture, 0, sizeof(zone_fixture));
  memset(&room_fixture, 0, sizeof(room_fixture));
  memset(&descriptor, 0, sizeof(descriptor));
  memset(&character, 0, sizeof(character));
  memset(&ship, 0, sizeof(ship));
  zone_fixture[0].number = WILD_ZONE_VNUM;
  zone_fixture[1].number = 12345;

  region_fixture[0].vnum = 7100020;
  region_fixture[0].zone = 0;
  region_fixture[0].name = "Western Outer Waters";
  region_fixture[0].region_type = REGION_GEOGRAPHIC;
  region_fixture[0].vertices = western_polygon;
  region_fixture[0].num_vertices = 5;
  region_fixture[1] = region_fixture[0];
  region_fixture[1].vnum = 7100010;
  region_fixture[1].name = "Western Inner Waters";
  region_fixture[2] = region_fixture[0];
  region_fixture[2].vnum = 7100000;
  region_fixture[2].region_type = REGION_ENCOUNTER;
  region_fixture[3] = region_fixture[0];
  region_fixture[3].vnum = 7099990;
  region_fixture[3].zone = 1;
  region_fixture[4] = region_fixture[0];
  region_fixture[4].vnum = 7100030;
  region_fixture[4].name = "Eastern Waters";
  region_fixture[4].vertices = eastern_polygon;

  saved_region_table = region_table;
  saved_top_of_region_table = top_of_region_table;
  saved_zone_table = zone_table;
  saved_top_of_zone_table = top_of_zone_table;
  saved_world = world;
  saved_top_of_world = top_of_world;
  region_table = region_fixture;
  top_of_region_table = 4;
  zone_table = zone_fixture;
  top_of_zone_table = 1;
  world = &room_fixture;
  top_of_world = 0;
  room_fixture.number = 91000;
  room_fixture.people = &character;
  descriptor.output = descriptor.small_outbuf;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  descriptor.character = &character;
  descriptor.pProtocol = ProtocolCreate();
  character.desc = &descriptor;
  character.in_room = 0;
  ship.num_rooms = 1;
  ship.room_vnums[0] = room_fixture.number;
  vessel_piracy_clear_laws();

  if (descriptor.pProtocol == NULL)
  {
    region_table = saved_region_table;
    top_of_region_table = saved_top_of_region_table;
    zone_table = saved_zone_table;
    top_of_zone_table = saved_top_of_zone_table;
    world = saved_world;
    top_of_world = saved_top_of_world;
    CuFail(tc, "could not initialize the named-water message fixture");
    return;
  }

  western_found = vessel_piracy_law_at_coordinates(5, 5, &law);
  western_region = law.region_vnum;
  eastern_found = vessel_piracy_law_at_coordinates(25, 5, &law);
  eastern_region = law.region_vnum;
  outside_found = vessel_piracy_law_at_coordinates(40, 5, &law);

  ship.x = 5;
  ship.y = 5;
  vessel_piracy_track_waters(&ship, FALSE);
  initialized_region = ship.waters_region_vnum;
  vessel_piracy_track_waters(&ship, TRUE);
  repeated_region = ship.waters_region_vnum;
  repeated_message_suppressed = descriptor.output[0] == '\0';
  ship.x = 25;
  vessel_piracy_track_waters(&ship, TRUE);
  crossed_region = ship.waters_region_vnum;
  crossing_announced = strstr(descriptor.output, "crossing into Eastern Waters") != NULL;
  ship.x = 40;
  vessel_piracy_track_waters(&ship, TRUE);
  outside_region = ship.waters_region_vnum;
  open_water_announced = strstr(descriptor.output, "enters unnamed open waters") != NULL;

  ProtocolDestroy(descriptor.pProtocol);
  region_table = saved_region_table;
  top_of_region_table = saved_top_of_region_table;
  zone_table = saved_zone_table;
  top_of_zone_table = saved_top_of_zone_table;
  world = saved_world;
  top_of_world = saved_top_of_world;

  CuAssertTrue(tc, western_found);
  CuAssertTrue(tc, eastern_found);
  CuAssertTrue(tc, !outside_found);
  CuAssertIntEquals(tc, 7100010, western_region);
  CuAssertIntEquals(tc, 7100030, eastern_region);
  CuAssertTrue(tc, ship.waters_region_initialized);
  CuAssertIntEquals(tc, 7100010, initialized_region);
  CuAssertIntEquals(tc, initialized_region, repeated_region);
  CuAssertTrue(tc, repeated_message_suppressed);
  CuAssertIntEquals(tc, 7100030, crossed_region);
  CuAssertTrue(tc, crossing_announced);
  CuAssertIntEquals(tc, 0, outside_region);
  CuAssertTrue(tc, open_water_announced);
}

void Test_vessel_shiptalk_is_scoped_to_one_ship(CuTest *tc)
{
  const int slot = 487;
  struct greyhawk_ship_data saved_ship;
  struct greyhawk_ship_data *ship;
  struct room_data room_fixture[3];
  struct room_data *saved_world;
  struct char_data speaker;
  struct char_data crew;
  struct char_data outsider;
  struct player_special_data speaker_specials;
  struct descriptor_data speaker_descriptor;
  struct descriptor_data crew_descriptor;
  struct descriptor_data outsider_descriptor;
  room_rnum saved_top_of_world;
  bool speaker_received;
  bool crew_received;
  bool outsider_remained_quiet;
  bool ashore_rejected;
  bool silence_rejected;

  saved_ship = greyhawk_ships[slot];
  saved_world = world;
  saved_top_of_world = top_of_world;
  ship = &greyhawk_ships[slot];
  memset(ship, 0, sizeof(*ship));
  memset(room_fixture, 0, sizeof(room_fixture));
  memset(&speaker, 0, sizeof(speaker));
  memset(&crew, 0, sizeof(crew));
  memset(&outsider, 0, sizeof(outsider));
  memset(&speaker_specials, 0, sizeof(speaker_specials));
  memset(&speaker_descriptor, 0, sizeof(speaker_descriptor));
  memset(&crew_descriptor, 0, sizeof(crew_descriptor));
  memset(&outsider_descriptor, 0, sizeof(outsider_descriptor));

  speaker_descriptor.pProtocol = ProtocolCreate();
  crew_descriptor.pProtocol = ProtocolCreate();
  outsider_descriptor.pProtocol = ProtocolCreate();
  if (speaker_descriptor.pProtocol == NULL || crew_descriptor.pProtocol == NULL ||
      outsider_descriptor.pProtocol == NULL)
  {
    if (speaker_descriptor.pProtocol != NULL)
    {
      ProtocolDestroy(speaker_descriptor.pProtocol);
    }
    if (crew_descriptor.pProtocol != NULL)
    {
      ProtocolDestroy(crew_descriptor.pProtocol);
    }
    if (outsider_descriptor.pProtocol != NULL)
    {
      ProtocolDestroy(outsider_descriptor.pProtocol);
    }
    greyhawk_ships[slot] = saved_ship;
    CuFail(tc, "could not initialize the shiptalk descriptor fixtures");
    return;
  }

  ship->active = TRUE;
  ship->shipnum = slot;
  ship->num_rooms = 2;
  ship->room_vnums[0] = 92000;
  ship->room_vnums[1] = 92001;
  strlcpy(ship->name, "Channel Cutter", sizeof(ship->name));

  room_fixture[0].number = 92000;
  room_fixture[0].ship = ship;
  room_fixture[0].people = &speaker;
  room_fixture[1].number = 92001;
  room_fixture[1].ship = ship;
  room_fixture[1].people = &crew;
  room_fixture[2].number = 93000;
  room_fixture[2].people = &outsider;
  world = room_fixture;
  top_of_world = 2;

  speaker.player.name = "Corr";
  speaker.player_specials = &speaker_specials;
  speaker.char_specials.position = POS_STANDING;
  speaker.in_room = 0;
  speaker.desc = &speaker_descriptor;
  speaker_descriptor.character = &speaker;
  speaker_descriptor.output = speaker_descriptor.small_outbuf;
  speaker_descriptor.bufspace = SMALL_BUFSIZE - 1;

  crew.player.name = "Mira";
  crew.player_specials = &speaker_specials;
  crew.char_specials.position = POS_STANDING;
  crew.in_room = 1;
  crew.desc = &crew_descriptor;
  crew_descriptor.character = &crew;
  crew_descriptor.output = crew_descriptor.small_outbuf;
  crew_descriptor.bufspace = SMALL_BUFSIZE - 1;

  outsider.player.name = "Vex";
  outsider.player_specials = &speaker_specials;
  outsider.char_specials.position = POS_STANDING;
  outsider.in_room = 2;
  outsider.desc = &outsider_descriptor;
  outsider_descriptor.character = &outsider;
  outsider_descriptor.output = outsider_descriptor.small_outbuf;
  outsider_descriptor.bufspace = SMALL_BUFSIZE - 1;

  do_shiptalk(&speaker, "All hands report ready.", 0, 0);
  speaker_received =
      strstr(speaker_descriptor.output, "Captain's channel - Channel Cutter") != NULL &&
      strstr(speaker_descriptor.output, "Corr: All hands report ready.") != NULL;
  crew_received =
      strstr(crew_descriptor.output, "Captain's channel - Channel Cutter") != NULL &&
      strstr(crew_descriptor.output, "Corr: All hands report ready.") != NULL;
  outsider_remained_quiet = outsider_descriptor.output[0] == '\0';

  speaker.in_room = 2;
  do_shiptalk(&speaker, "This must not leave the shore.", 0, 0);
  ashore_rejected = strstr(speaker_descriptor.output, "must be aboard a vessel") != NULL;
  speaker.in_room = 0;
  SET_BIT_AR(AFF_FLAGS(&speaker), AFF_SILENCED);
  do_shiptalk(&speaker, "This must not be audible.", 0, 0);
  silence_rejected = strstr(speaker_descriptor.output, "cannot make a sound") != NULL;

  ProtocolDestroy(speaker_descriptor.pProtocol);
  ProtocolDestroy(crew_descriptor.pProtocol);
  ProtocolDestroy(outsider_descriptor.pProtocol);
  greyhawk_ships[slot] = saved_ship;
  world = saved_world;
  top_of_world = saved_top_of_world;

  CuAssertTrue(tc, speaker_received);
  CuAssertTrue(tc, crew_received);
  CuAssertTrue(tc, outsider_remained_quiet);
  CuAssertTrue(tc, ashore_rejected);
  CuAssertTrue(tc, silence_rejected);
}

void Test_vessel_piracy_regional_bounty_policy(CuTest *tc)
{
  struct vessel_piracy_law law;

  memset(&law, 0, sizeof(law));
  CuAssertStrEquals(tc, "unclaimed waters",
                    vessel_waters_type_name(VESSEL_WATERS_UNCLAIMED));
  CuAssertStrEquals(tc, "territorial waters",
                    vessel_waters_type_name(VESSEL_WATERS_TERRITORIAL));
  CuAssertStrEquals(tc, "free seas",
                    vessel_waters_type_name(VESSEL_WATERS_FREE));
  CuAssertStrEquals(tc, "pirate cove",
                    vessel_waters_type_name(VESSEL_WATERS_PIRATE_COVE));
  CuAssertStrEquals(tc, "unclaimed waters", vessel_waters_type_name(INT_MAX));

  CuAssertIntEquals(tc, 0, vessel_piracy_bounty_for_units(0, 100));
  CuAssertIntEquals(tc, 0, vessel_piracy_bounty_for_units(1, 0));
  CuAssertIntEquals(tc, 15, vessel_piracy_bounty_for_units(1, -1));
  CuAssertIntEquals(tc, 15, vessel_piracy_bounty_for_units(1, 100));
  CuAssertIntEquals(tc, 7, vessel_piracy_bounty_for_units(1, 50));
  CuAssertIntEquals(tc, 22, vessel_piracy_bounty_for_units(1, 150));
  CuAssertIntEquals(tc, 75, vessel_piracy_bounty_for_units(1, 1000));
  CuAssertIntEquals(tc, INT_MAX,
                    vessel_piracy_bounty_for_units(INT_MAX,
                                                   VESSEL_PIRACY_BOUNTY_PERCENT_MAX));

  CuAssertTrue(tc, !vessel_piracy_wanted_port_is_open(NULL));
  law.waters_type = VESSEL_WATERS_PIRATE_COVE;
  CuAssertTrue(tc, !vessel_piracy_wanted_port_is_open(&law));
  law.configured = TRUE;
  CuAssertTrue(tc, vessel_piracy_wanted_port_is_open(&law));
  law.waters_type = VESSEL_WATERS_FREE;
  CuAssertTrue(tc, !vessel_piracy_wanted_port_is_open(&law));
}

void Test_vessel_encounter_region_selection_is_order_independent(CuTest *tc)
{
  struct region_data fixture[3];
  struct region_data *saved_region_table;
  struct region_list first;
  struct region_list second;
  struct region_list geographic;
  struct region_list invalid;
  region_rnum saved_top_of_region_table;
  bool first_order_found;
  bool reverse_order_found;
  bool center_found;
  bool invalid_found;
  int first_order_vnum;
  int reverse_order_vnum;
  int center_vnum;
  int invalid_vnum;

  memset(fixture, 0, sizeof(fixture));
  memset(&first, 0, sizeof(first));
  memset(&second, 0, sizeof(second));
  memset(&geographic, 0, sizeof(geographic));
  memset(&invalid, 0, sizeof(invalid));

  fixture[0].vnum = 70030;
  fixture[0].region_type = REGION_ENCOUNTER;
  fixture[1].vnum = 70020;
  fixture[1].region_type = REGION_ENCOUNTER;
  fixture[2].vnum = 70010;
  fixture[2].region_type = REGION_GEOGRAPHIC;

  first.rnum = 0;
  first.pos = REGION_POS_INSIDE;
  second.rnum = 1;
  second.pos = REGION_POS_INSIDE;
  geographic.rnum = 2;
  geographic.pos = REGION_POS_CENTER;
  invalid.rnum = 3;
  invalid.pos = REGION_POS_CENTER;

  saved_region_table = region_table;
  saved_top_of_region_table = top_of_region_table;
  region_table = fixture;
  top_of_region_table = 2;

  geographic.next = &first;
  first.next = &second;
  second.next = NULL;
  first_order_found = vessel_encounter_region_from_list(&geographic, &first_order_vnum);

  second.next = &first;
  first.next = &geographic;
  geographic.next = NULL;
  reverse_order_found = vessel_encounter_region_from_list(&second, &reverse_order_vnum);

  first.pos = REGION_POS_CENTER;
  center_found = vessel_encounter_region_from_list(&second, &center_vnum);
  invalid_found = vessel_encounter_region_from_list(&invalid, &invalid_vnum);

  region_table = saved_region_table;
  top_of_region_table = saved_top_of_region_table;

  CuAssertTrue(tc, first_order_found);
  CuAssertTrue(tc, reverse_order_found);
  CuAssertIntEquals(tc, 70020, first_order_vnum);
  CuAssertIntEquals(tc, first_order_vnum, reverse_order_vnum);
  CuAssertTrue(tc, center_found);
  CuAssertIntEquals(tc, 70030, center_vnum);
  CuAssertTrue(tc, !invalid_found);
  CuAssertIntEquals(tc, 0, invalid_vnum);
}

void Test_vessel_encounter_roll_boundaries_are_deterministic(CuTest *tc)
{
  CuAssertTrue(tc, !vessel_encounter_chance_succeeds(0, 1));
  CuAssertTrue(tc, vessel_encounter_chance_succeeds(25, 25));
  CuAssertTrue(tc, !vessel_encounter_chance_succeeds(25, 26));
  CuAssertTrue(tc, vessel_encounter_chance_succeeds(100, 100));
  CuAssertTrue(tc, vessel_encounter_chance_succeeds(150, 100));
  CuAssertTrue(tc, !vessel_encounter_chance_succeeds(100, 0));
  CuAssertTrue(tc, !vessel_encounter_chance_succeeds(100, 101));
}

void Test_vessel_encounter_shared_room_claims_once(CuTest *tc)
{
  room_rnum claimed_rooms[2];
  int claimed_count = 0;

  CuAssertTrue(tc, vessel_encounter_claim_room(100, claimed_rooms, &claimed_count, 2));
  CuAssertIntEquals(tc, 1, claimed_count);
  CuAssertTrue(tc, !vessel_encounter_claim_room(100, claimed_rooms, &claimed_count, 2));
  CuAssertIntEquals(tc, 1, claimed_count);
  CuAssertTrue(tc, vessel_encounter_claim_room(101, claimed_rooms, &claimed_count, 2));
  CuAssertIntEquals(tc, 2, claimed_count);
  CuAssertTrue(tc, !vessel_encounter_claim_room(102, claimed_rooms, &claimed_count, 2));
  CuAssertTrue(tc, !vessel_encounter_claim_room(NOWHERE, claimed_rooms, &claimed_count, 2));
}

void Test_vessel_hunter_policy_bounds(CuTest *tc)
{
  struct vessel_hunter_config config;

  memset(&config, 0, sizeof(config));
  config.encounter_id = 1;
  config.prototype_id = 2;
  config.pilot_mob_vnum = 70002;
  config.min_bounty = BOUNTY_HUNTED;
  config.pursuit_speed = 5;
  config.hunt_duration_seconds = 300;
  config.target_grace_seconds = 15;
  config.cooldown_seconds = 30;
  config.enabled = TRUE;

  CuAssertTrue(tc, vessel_hunter_config_is_valid(&config));
  CuAssertTrue(tc, !vessel_hunter_config_is_valid(NULL));

  config.min_bounty = BOUNTY_HUNTED - 1;
  CuAssertTrue(tc, !vessel_hunter_config_is_valid(&config));
  config.min_bounty = BOUNTY_HUNTED;

  config.pursuit_speed = 0;
  CuAssertTrue(tc, !vessel_hunter_config_is_valid(&config));
  config.pursuit_speed = VESSEL_HUNTER_PURSUIT_SPEED_MAX + 1;
  CuAssertTrue(tc, !vessel_hunter_config_is_valid(&config));
  config.pursuit_speed = 5;

  config.hunt_duration_seconds = VESSEL_HUNTER_DURATION_MIN - 1;
  CuAssertTrue(tc, !vessel_hunter_config_is_valid(&config));
  config.hunt_duration_seconds = VESSEL_HUNTER_DURATION_MAX + 1;
  CuAssertTrue(tc, !vessel_hunter_config_is_valid(&config));
  config.hunt_duration_seconds = 300;

  config.target_grace_seconds = -1;
  CuAssertTrue(tc, !vessel_hunter_config_is_valid(&config));
  config.target_grace_seconds = VESSEL_HUNTER_GRACE_MAX + 1;
  CuAssertTrue(tc, !vessel_hunter_config_is_valid(&config));
  config.target_grace_seconds = 15;

  config.cooldown_seconds = 0;
  CuAssertTrue(tc, !vessel_hunter_config_is_valid(&config));
  config.cooldown_seconds = VESSEL_HUNTER_COOLDOWN_MAX + 1;
  CuAssertTrue(tc, !vessel_hunter_config_is_valid(&config));

  CuAssertTrue(tc,
               vessel_hunter_lifecycle_allows_spawn("cooldown", 1000, 1000));
  CuAssertTrue(tc,
               vessel_hunter_lifecycle_allows_spawn("cooldown", 999, 1000));
  CuAssertTrue(tc, !vessel_hunter_lifecycle_allows_spawn(
                       "cooldown", 1001, 1000));
  CuAssertTrue(
      tc, !vessel_hunter_lifecycle_allows_spawn("active", 0, 1000));
  CuAssertTrue(
      tc, !vessel_hunter_lifecycle_allows_spawn("spawning", 0, 1000));
  CuAssertTrue(tc,
               !vessel_hunter_lifecycle_allows_spawn("invalid", 0, 1000));
  CuAssertTrue(tc,
               !vessel_hunter_lifecycle_allows_spawn(NULL, 0, 1000));
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

  /* A durable NPC merchant remains PvE even after its registry is attached */
  ship.merchant_id = 7;
  CuAssertTrue(tc, vessel_pvp_permitted(&attacker, &ship, FALSE));
  ship.merchant_id = 0;

  /* The navy hull is also ordinary ownerless PvE, so a HUNTED target can
   * defend itself without requiring mutual player consent. */
  ship.bounty_hunter = TRUE;
  CuAssertTrue(tc, vessel_pvp_permitted(&attacker, &ship, FALSE));
  ship.bounty_hunter = FALSE;

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

void Test_vessel_message_throttling_is_keyed_per_ship(CuTest *tc)
{
  struct greyhawk_ship_data ship;
  struct greyhawk_ship_data other_ship;

  memset(&ship, 0, sizeof(ship));
  memset(&other_ship, 0, sizeof(other_ship));
  PERF_reset();

  CuAssertTrue(tc, vessel_message_allowed(&ship, VESSEL_MESSAGE_AMBIENT_SQUALL, 100, 10));
  CuAssertTrue(tc, !vessel_message_allowed(&ship, VESSEL_MESSAGE_AMBIENT_SQUALL, 109, 10));
  CuAssertTrue(tc, vessel_message_allowed(&ship, VESSEL_MESSAGE_AMBIENT_STORM, 101, 10));
  CuAssertTrue(tc, vessel_message_allowed(&ship, VESSEL_MESSAGE_AMBIENT_SQUALL, 110, 10));
  CuAssertTrue(tc,
               vessel_message_allowed(&other_ship, VESSEL_MESSAGE_AMBIENT_SQUALL, 109, 10));

  /* A process-pulse rollback must not leave a reconstructed ship muted. */
  CuAssertTrue(tc, vessel_message_allowed(&ship, VESSEL_MESSAGE_AMBIENT_SQUALL, 50, 10));
  CuAssertTrue(tc, !vessel_message_allowed(NULL, VESSEL_MESSAGE_AMBIENT_SQUALL, 50, 10));
  CuAssertTrue(tc,
               !vessel_message_allowed(&ship, (enum vessel_message_key)-1, 50, 10));
  CuAssertTrue(tc, PERF_vessel_message_throttled_count() == 1);
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

void Test_vessel_public_schedule_passenger_fare_policy(CuTest *tc)
{
  struct greyhawk_ship_data ship;
  struct vessel_schedule schedule;
  struct char_data passenger;

  memset(&ship, 0, sizeof(ship));
  memset(&schedule, 0, sizeof(schedule));
  memset(&passenger, 0, sizeof(passenger));
  ship.schedule = &schedule;
  schedule.passenger_fare = 10;

  CuAssertIntEquals(tc, 10, vessel_passenger_fare(&ship));
  strlcpy(ship.owner, "Kohdee", sizeof(ship.owner));
  CuAssertIntEquals(tc, 0, vessel_passenger_fare(&ship));
  ship.owner[0] = '\0';

  schedule.passenger_fare = VESSEL_PASSENGER_FARE_MAX + 1;
  CuAssertIntEquals(tc, VESSEL_PASSENGER_FARE_MAX, vessel_passenger_fare(&ship));
  schedule.passenger_fare = -1;
  CuAssertIntEquals(tc, 0, vessel_passenger_fare(&ship));
  schedule.passenger_fare = 10;

  GET_GOLD(&passenger) = 9;
  CuAssertTrue(tc, !vessel_collect_passenger_fare(&passenger, &ship));
  CuAssertIntEquals(tc, 9, GET_GOLD(&passenger));

  GET_GOLD(&passenger) = 20;
  GET_PFILEPOS(&passenger) = -1;
  CuAssertTrue(tc, !vessel_collect_passenger_fare(&passenger, &ship));
  CuAssertIntEquals(tc, 20, GET_GOLD(&passenger));

  SET_BIT_AR(MOB_FLAGS(&passenger), MOB_ISNPC);
  CuAssertTrue(tc, vessel_collect_passenger_fare(&passenger, &ship));
  CuAssertIntEquals(tc, 20, GET_GOLD(&passenger));
  CuAssertTrue(tc, !vessel_collect_passenger_fare(NULL, &ship));
  CuAssertTrue(tc, !vessel_collect_passenger_fare(&passenger, NULL));
}

void Test_vessel_merchant_respawn_gate(CuTest *tc)
{
  const time_t now = (time_t)1000;

  CuAssertTrue(tc, vessel_merchant_should_spawn(TRUE, 0, 0, now));
  CuAssertTrue(tc, vessel_merchant_should_spawn(TRUE, 0, now, now));
  CuAssertTrue(tc, vessel_merchant_should_spawn(TRUE, -1, now - 1, now));
  CuAssertTrue(tc, !vessel_merchant_should_spawn(FALSE, 0, 0, now));
  CuAssertTrue(tc, !vessel_merchant_should_spawn(TRUE, 42, 0, now));
  CuAssertTrue(tc, !vessel_merchant_should_spawn(TRUE, 0, now + 1, now));
}

void Test_vessel_merchant_faction_consequence_scaling(CuTest *tc)
{
  CuAssertIntEquals(tc, 0, vessel_merchant_faction_penalty(-1, FALSE));
  CuAssertIntEquals(tc, 0, vessel_merchant_faction_penalty(0, FALSE));
  CuAssertIntEquals(tc, 25, vessel_merchant_faction_penalty(25, FALSE));
  CuAssertIntEquals(tc, VESSEL_MERCHANT_LOSS_STANDING_PENALTY,
                    vessel_merchant_faction_penalty(0, TRUE));
  CuAssertIntEquals(tc, 125, vessel_merchant_faction_penalty(25, TRUE));
  CuAssertIntEquals(tc, INT_MAX,
                    vessel_merchant_faction_penalty(INT_MAX, TRUE));
}

void Test_vessel_merchant_loss_responsibility_window(CuTest *tc)
{
  const time_t now = (time_t)1000;

  CuAssertTrue(tc, vessel_merchant_responsibility_active(
                       now - VESSEL_MERCHANT_RESPONSIBILITY_SECONDS, now));
  CuAssertTrue(tc, !vessel_merchant_responsibility_active(
                       now - VESSEL_MERCHANT_RESPONSIBILITY_SECONDS - 1, now));
  CuAssertTrue(tc, !vessel_merchant_responsibility_active(0, now));
  CuAssertTrue(tc, !vessel_merchant_responsibility_active(now + 1, now));
}

void Test_vessel_merchant_constructor_rejects_invalid_identity(CuTest *tc)
{
  CuAssertIntEquals(
      tc, -1,
      vessel_spawn_public_from_prototype_at(0, "Invalid Merchant", 0, 0, 0));
  CuAssertIntEquals(
      tc, -1, vessel_spawn_public_from_prototype_at(1, "", 0, 0, 0));
  CuAssertIntEquals(
      tc, -1, vessel_spawn_public_from_prototype_at(1, NULL, 0, 0, 0));
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
