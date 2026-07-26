#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/vessels.h"

#include <stdlib.h>
#include <string.h>

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

  CuAssertTrue(tc, check_vessel_vehicle_capacity(&vessel, &vehicle));
  CuAssertTrue(tc, !check_vessel_vehicle_capacity(NULL, &vehicle));
  CuAssertTrue(tc, !check_vessel_vehicle_capacity(&vessel, NULL));
}
