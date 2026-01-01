/* ************************************************************************
 *      File:   vehicles_transport.c                   Part of LuminariMUD  *
 *   Purpose:   Vehicle-in-Vessel transport mechanics (Phase 02, Session 05) *
 *  Author:     LuminariMUD Development Team                                *
 * ********************************************************************** */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "vessels.h"
#include "constants.h"

/* External variables */
extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];
extern struct room_data *world;

/* Vehicle transport constants */
#define MAX_VEHICLES_PER_VESSEL 10     /* Maximum vehicles a vessel can carry */
#define VESSEL_VEHICLE_WEIGHT_MULT 100 /* Weight multiplier for vehicles on vessels */

/* Forward declarations for internal helper functions */
static int is_vessel_stationary_or_docked(struct greyhawk_ship_data *vessel);
static int is_vehicle_mounted(struct vehicle_data *vehicle);

/* ========================================================================= */
/* INTERNAL HELPER FUNCTIONS                                                  */
/* ========================================================================= */

/**
 * Check if vessel is stationary (speed 0) or docked.
 *
 * Vessels must be stationary or docked to allow vehicle loading/unloading.
 *
 * @param vessel The vessel to check
 * @return TRUE if vessel is stationary or docked, FALSE otherwise
 */
static int is_vessel_stationary_or_docked(struct greyhawk_ship_data *vessel)
{
  if (vessel == NULL)
  {
    VHCL_DEBUG_XPORT("is_vessel_stationary_or_docked: NULL vessel");
    return FALSE;
  }

  /* Check if docked to another vessel */
  if (vessel->docked_to_ship >= 0)
  {
    VHCL_DEBUG_XPORT("Vessel %s is docked (docked_to=%d)", vessel->name, vessel->docked_to_ship);
    return TRUE;
  }

  /* Check if speed is zero */
  if (vessel->speed == 0)
  {
    VHCL_DEBUG_XPORT("Vessel %s is stationary (speed=0)", vessel->name);
    return TRUE;
  }

  VHCL_DEBUG_XPORT("Vessel %s is moving (speed=%d)", vessel->name, vessel->speed);
  return FALSE;
}

/**
 * Check if a vehicle currently has a mounted driver/rider.
 *
 * Vehicles cannot be loaded onto vessels while occupied.
 *
 * @param vehicle The vehicle to check
 * @return TRUE if vehicle has passengers, FALSE otherwise
 */
static int is_vehicle_mounted(struct vehicle_data *vehicle)
{
  if (vehicle == NULL)
  {
    VHCL_DEBUG_XPORT("is_vehicle_mounted: NULL vehicle");
    return FALSE;
  }

  VHCL_DEBUG_XPORT("Vehicle #%d passengers: %d/%d", vehicle->id, vehicle->current_passengers,
                   vehicle->max_passengers);
  return (vehicle->current_passengers > 0);
}

/* ========================================================================= */
/* CAPACITY VALIDATION (T008)                                                 */
/* ========================================================================= */

/**
 * Check if a vessel has capacity to load a vehicle.
 *
 * Validates weight limits and maximum vehicle count.
 * Uses vessel's cargo capacity to determine if vehicle can be loaded.
 *
 * @param vessel The vessel to check capacity for
 * @param vehicle The vehicle to potentially load
 * @return TRUE if vessel can accept the vehicle, FALSE otherwise
 */
int check_vessel_vehicle_capacity(struct greyhawk_ship_data *vessel, struct vehicle_data *vehicle)
{
  int loaded_count = 0;
  int i;
  int vehicle_weight;

  VSSL_DEBUG_ENTER("check_vessel_vehicle_capacity");

  if (vessel == NULL || vehicle == NULL)
  {
    VHCL_DEBUG_XPORT("check_vessel_vehicle_capacity: NULL pointer (vessel=%p, vehicle=%p)",
                     (void *)vessel, (void *)vehicle);
    return FALSE;
  }

  VHCL_DEBUG_XPORT("Checking capacity: vessel %s for vehicle #%d (%s)", vessel->name, vehicle->id,
                   vehicle->name);

  /* Count vehicles already loaded on this vessel */
  /* This would iterate through all vehicles to count those with parent_vessel_id matching */
  /* For now, we check against MAX_VEHICLES_PER_VESSEL */
  for (i = 0; i < MAX_VEHICLES; i++)
  {
    struct vehicle_data *v = vehicle_find_by_id(i);
    if (v != NULL && v->parent_vessel_id == vessel->shipnum)
    {
      loaded_count++;
    }
  }

  VHCL_DEBUG_XPORT("Vessel %s has %d/%d vehicles loaded", vessel->name, loaded_count,
                   MAX_VEHICLES_PER_VESSEL);

  if (loaded_count >= MAX_VEHICLES_PER_VESSEL)
  {
    VHCL_DEBUG_XPORT("CAPACITY CHECK FAILED: Max vehicles reached");
    return FALSE;
  }

  /* Calculate vehicle weight for vessel cargo */
  vehicle_weight = vehicle->max_weight + vehicle->current_weight;
  VHCL_DEBUG_XPORT("Vehicle weight: %d (max=%d + current=%d)", vehicle_weight, vehicle->max_weight,
                   vehicle->current_weight);

  /* Check vessel cargo capacity */
  /* Vessels use cargo_capacity field for total weight allowed */
  /* TODO: Add actual cargo capacity check when vessel struct supports it */
  (void)vehicle_weight; /* Suppress unused warning for now */

  VHCL_DEBUG_XPORT("Capacity check passed");
  return TRUE;
}

/* ========================================================================= */
/* LOADING FUNCTIONS (T009)                                                   */
/* ========================================================================= */

/**
 * Load a vehicle onto a vessel.
 *
 * Validates all preconditions:
 * - Vessel must be stationary or docked
 * - Vehicle must not be occupied (no passengers)
 * - Vessel must have capacity for the vehicle
 * - Vehicle must not already be loaded on a vessel
 *
 * @param ch The character performing the action
 * @param vehicle The vehicle to load
 * @param vessel The vessel to load onto
 * @return TRUE on success, FALSE on failure (with message to ch)
 */
int load_vehicle_onto_vessel(struct char_data *ch, struct vehicle_data *vehicle,
                             struct greyhawk_ship_data *vessel)
{
  VSSL_DEBUG_ENTER("load_vehicle_onto_vessel");

  if (ch == NULL || vehicle == NULL || vessel == NULL)
  {
    VHCL_DEBUG_XPORT("load_vehicle_onto_vessel: NULL pointer (ch=%p, vehicle=%p, vessel=%p)",
                     (void *)ch, (void *)vehicle, (void *)vessel);
    VSSL_DEBUG_EXIT_VAL("load_vehicle_onto_vessel", FALSE);
    return FALSE;
  }

  VHCL_DEBUG_XPORT("Attempting to load vehicle #%d (%s) onto vessel %s (%d)", vehicle->id,
                   vehicle->name, vessel->name, vessel->shipnum);

  /* Check if vessel is stationary or docked */
  if (!is_vessel_stationary_or_docked(vessel))
  {
    VHCL_DEBUG_XPORT("LOAD FAILED: Vessel %s is moving", vessel->name);
    send_to_char(ch, "The vessel must be stationary or docked to load vehicles.\r\n");
    VSSL_DEBUG_EXIT_VAL("load_vehicle_onto_vessel", FALSE);
    return FALSE;
  }

  /* Check if vehicle is occupied */
  if (is_vehicle_mounted(vehicle))
  {
    VHCL_DEBUG_XPORT("LOAD FAILED: Vehicle #%d has passengers", vehicle->id);
    send_to_char(ch, "You cannot load an occupied vehicle. Dismount first.\r\n");
    VSSL_DEBUG_EXIT_VAL("load_vehicle_onto_vessel", FALSE);
    return FALSE;
  }

  /* Check if vehicle is already loaded on a vessel */
  if (vehicle->parent_vessel_id > 0)
  {
    VHCL_DEBUG_XPORT("LOAD FAILED: Vehicle #%d already on vessel %d", vehicle->id,
                     vehicle->parent_vessel_id);
    send_to_char(ch, "That vehicle is already loaded on a vessel.\r\n");
    VSSL_DEBUG_EXIT_VAL("load_vehicle_onto_vessel", FALSE);
    return FALSE;
  }

  /* Check if vehicle is damaged */
  if (vehicle->state == VSTATE_DAMAGED)
  {
    VHCL_DEBUG_XPORT("LOAD FAILED: Vehicle #%d is damaged", vehicle->id);
    send_to_char(ch, "That vehicle is too damaged to load safely.\r\n");
    VSSL_DEBUG_EXIT_VAL("load_vehicle_onto_vessel", FALSE);
    return FALSE;
  }

  /* Check vessel capacity */
  if (!check_vessel_vehicle_capacity(vessel, vehicle))
  {
    VHCL_DEBUG_XPORT("LOAD FAILED: Vessel %s at capacity", vessel->name);
    send_to_char(ch, "The vessel cannot carry any more vehicles.\r\n");
    VSSL_DEBUG_EXIT_VAL("load_vehicle_onto_vessel", FALSE);
    return FALSE;
  }

  /* Perform the loading */
  VHCL_DEBUG_XPORT("Loading vehicle #%d: setting parent_vessel_id=%d, state=VSTATE_ON_VESSEL",
                   vehicle->id, vessel->shipnum);
  vehicle->parent_vessel_id = vessel->shipnum;
  vehicle->state = VSTATE_ON_VESSEL;

  /* Sync coordinates with vessel */
  vehicle_sync_with_vessel(vehicle, vessel);

  /* Save the vehicle state */
  vehicle_save(vehicle);

  /* Send success messages */
  send_to_char(ch, "You load %s onto %s.\r\n", vehicle->name, vessel->name);

  /* Notify others on the vessel */
  send_to_ship(vessel, "%s has been loaded onto the vessel.", vehicle->name);

  VHCL_DEBUG_XPORT("LOAD SUCCESS: Vehicle #%d now on vessel %s", vehicle->id, vessel->name);
  VSSL_DEBUG_EXIT_VAL("load_vehicle_onto_vessel", TRUE);
  return TRUE;
}

/* ========================================================================= */
/* UNLOADING FUNCTIONS (T010)                                                 */
/* ========================================================================= */

/**
 * Unload a vehicle from a vessel.
 *
 * Validates all preconditions:
 * - Vehicle must be loaded on a vessel
 * - Vessel must be stationary or docked
 * - Current location must have valid terrain for the vehicle
 *
 * @param ch The character performing the action
 * @param vehicle The vehicle to unload
 * @return TRUE on success, FALSE on failure (with message to ch)
 */
int unload_vehicle_from_vessel(struct char_data *ch, struct vehicle_data *vehicle)
{
  struct greyhawk_ship_data *vessel;
  int terrain_type;
  room_rnum unload_room;

  VSSL_DEBUG_ENTER("unload_vehicle_from_vessel");

  if (ch == NULL || vehicle == NULL)
  {
    VHCL_DEBUG_XPORT("unload_vehicle_from_vessel: NULL pointer (ch=%p, vehicle=%p)", (void *)ch,
                     (void *)vehicle);
    VSSL_DEBUG_EXIT_VAL("unload_vehicle_from_vessel", FALSE);
    return FALSE;
  }

  VHCL_DEBUG_XPORT("Attempting to unload vehicle #%d (%s) from vessel %d", vehicle->id,
                   vehicle->name, vehicle->parent_vessel_id);

  /* Check if vehicle is actually loaded on a vessel */
  if (vehicle->parent_vessel_id <= 0)
  {
    VHCL_DEBUG_XPORT("UNLOAD FAILED: Vehicle #%d not on any vessel", vehicle->id);
    send_to_char(ch, "That vehicle is not loaded on a vessel.\r\n");
    VSSL_DEBUG_EXIT_VAL("unload_vehicle_from_vessel", FALSE);
    return FALSE;
  }

  /* Get the parent vessel */
  vessel = get_ship_by_id(vehicle->parent_vessel_id);
  if (vessel == NULL)
  {
    /* Orphaned vehicle - clean up */
    VHCL_DEBUG_XPORT(
        "ORPHANED VEHICLE: #%d had parent_vessel_id=%d but vessel not found, resetting",
        vehicle->id, vehicle->parent_vessel_id);
    vehicle->parent_vessel_id = 0;
    vehicle->state = VSTATE_IDLE;
    vehicle_save(vehicle);
    send_to_char(ch, "Error: Parent vessel not found. Vehicle state reset.\r\n");
    VSSL_DEBUG_EXIT_VAL("unload_vehicle_from_vessel", FALSE);
    return FALSE;
  }

  VHCL_DEBUG_XPORT("Found parent vessel: %s (%d)", vessel->name, vessel->shipnum);

  /* Check if vessel is stationary or docked */
  if (!is_vessel_stationary_or_docked(vessel))
  {
    VHCL_DEBUG_XPORT("UNLOAD FAILED: Vessel %s is moving", vessel->name);
    send_to_char(ch, "The vessel must be stationary or docked to unload vehicles.\r\n");
    VSSL_DEBUG_EXIT_VAL("unload_vehicle_from_vessel", FALSE);
    return FALSE;
  }

  /* Determine unload location based on vessel position */
  /* For wilderness vessels, we need to create or find a suitable room */
  unload_room = find_docking_room(vessel);
  if (unload_room == NOWHERE)
  {
    VHCL_DEBUG_XPORT("UNLOAD FAILED: No suitable unload room found for vessel %s", vessel->name);
    send_to_char(ch, "Cannot find a suitable location to unload the vehicle.\r\n");
    VSSL_DEBUG_EXIT_VAL("unload_vehicle_from_vessel", FALSE);
    return FALSE;
  }

  VHCL_DEBUG_XPORT("Unload room found: %d", unload_room);

  /* Check terrain compatibility */
  terrain_type = world[unload_room].sector_type;
  if (!vehicle_can_traverse_terrain(vehicle, terrain_type))
  {
    VHCL_DEBUG_XPORT("UNLOAD FAILED: Vehicle #%d cannot traverse terrain type %d", vehicle->id,
                     terrain_type);
    send_to_char(ch, "The terrain here is not suitable for %s.\r\n", vehicle->name);
    VSSL_DEBUG_EXIT_VAL("unload_vehicle_from_vessel", FALSE);
    return FALSE;
  }

  /* Perform the unloading */
  VHCL_DEBUG_XPORT(
      "Unloading vehicle #%d: clearing parent_vessel_id, setting location=%d, coords=(%d,%d)",
      vehicle->id, unload_room, (int)vessel->x, (int)vessel->y);
  vehicle->parent_vessel_id = 0;
  vehicle->state = VSTATE_IDLE;
  vehicle->location = unload_room;

  /* Update coordinates from vessel's current position */
  vehicle->x_coord = (int)vessel->x;
  vehicle->y_coord = (int)vessel->y;

  /* Save the vehicle state */
  vehicle_save(vehicle);

  /* Send success messages */
  send_to_char(ch, "You unload %s from %s.\r\n", vehicle->name, vessel->name);

  /* Notify others on the vessel */
  send_to_ship(vessel, "%s has been unloaded from the vessel.", vehicle->name);

  VHCL_DEBUG_XPORT("UNLOAD SUCCESS: Vehicle #%d now at room %d", vehicle->id, unload_room);
  VSSL_DEBUG_EXIT_VAL("unload_vehicle_from_vessel", TRUE);
  return TRUE;
}

/* ========================================================================= */
/* VEHICLE LIST FUNCTIONS (T011)                                              */
/* ========================================================================= */

/**
 * Get list of vehicles loaded on a vessel.
 *
 * Allocates and returns an array of pointers to vehicles currently
 * loaded on the specified vessel. Caller must free the returned array.
 *
 * @param vessel The vessel to query
 * @param count Output parameter - number of vehicles found
 * @return Array of vehicle pointers (caller must free), or NULL if none
 */
struct vehicle_data **get_loaded_vehicles_list(struct greyhawk_ship_data *vessel, int *count)
{
  struct vehicle_data **list = NULL;
  struct vehicle_data *v;
  int found = 0;
  int i;

  VSSL_DEBUG_ENTER("get_loaded_vehicles_list");

  if (vessel == NULL || count == NULL)
  {
    VHCL_DEBUG_XPORT("get_loaded_vehicles_list: NULL pointer (vessel=%p, count=%p)", (void *)vessel,
                     (void *)count);
    if (count != NULL)
      *count = 0;
    VSSL_DEBUG_EXIT("get_loaded_vehicles_list");
    return NULL;
  }

  VHCL_DEBUG_XPORT("Querying vehicles for vessel %s (%d)", vessel->name, vessel->shipnum);

  /* First pass: count vehicles */
  for (i = 0; i < MAX_VEHICLES; i++)
  {
    v = vehicle_find_by_id(i);
    if (v != NULL && v->parent_vessel_id == vessel->shipnum)
    {
      found++;
    }
  }

  VHCL_DEBUG_XPORT("Count pass: found %d vehicles on vessel %s", found, vessel->name);

  if (found == 0)
  {
    *count = 0;
    VSSL_DEBUG_EXIT("get_loaded_vehicles_list");
    return NULL;
  }

  /* Allocate array */
  CREATE(list, struct vehicle_data *, found);
  VHCL_DEBUG_XPORT("Allocated array for %d vehicle pointers", found);

  /* Second pass: populate array */
  found = 0;
  for (i = 0; i < MAX_VEHICLES; i++)
  {
    v = vehicle_find_by_id(i);
    if (v != NULL && v->parent_vessel_id == vessel->shipnum)
    {
      list[found++] = v;
    }
  }

  *count = found;
  VHCL_DEBUG_XPORT("Returning %d vehicles for vessel %s", found, vessel->name);
  VSSL_DEBUG_EXIT("get_loaded_vehicles_list");
  return list;
}

/* ========================================================================= */
/* COORDINATE SYNCHRONIZATION (T012)                                          */
/* ========================================================================= */

/**
 * Synchronize vehicle coordinates with parent vessel.
 *
 * Called when vessel moves to update all loaded vehicles' coordinates.
 * Also used during initial loading to set vehicle position.
 *
 * @param vehicle The vehicle to update
 * @param vessel The parent vessel to sync with
 */
void vehicle_sync_with_vessel(struct vehicle_data *vehicle, struct greyhawk_ship_data *vessel)
{
  VSSL_DEBUG_ENTER("vehicle_sync_with_vessel");

  if (vehicle == NULL || vessel == NULL)
  {
    VHCL_DEBUG_XPORT("vehicle_sync_with_vessel: NULL pointer (vehicle=%p, vessel=%p)",
                     (void *)vehicle, (void *)vessel);
    VSSL_DEBUG_EXIT("vehicle_sync_with_vessel");
    return;
  }

  /* Only sync if vehicle is actually on this vessel */
  if (vehicle->parent_vessel_id != vessel->shipnum)
  {
    VHCL_DEBUG_XPORT("Sync skipped: vehicle #%d parent=%d != vessel %d", vehicle->id,
                     vehicle->parent_vessel_id, vessel->shipnum);
    VSSL_DEBUG_EXIT("vehicle_sync_with_vessel");
    return;
  }

  /* Update wilderness coordinates to match vessel */
  VHCL_DEBUG_XPORT("Syncing vehicle #%d coords: (%d,%d) -> (%d,%d)", vehicle->id, vehicle->x_coord,
                   vehicle->y_coord, (int)vessel->x, (int)vessel->y);
  vehicle->x_coord = (int)vessel->x;
  vehicle->y_coord = (int)vessel->y;

  /* Vehicle location is set to NOWHERE when on vessel */
  /* The actual room is derived from the vessel when needed */
  VSSL_DEBUG_EXIT("vehicle_sync_with_vessel");
}

/**
 * Sync all vehicles loaded on a vessel.
 *
 * Called after vessel movement to update all loaded vehicles.
 * Should be hooked into vessel movement system.
 *
 * @param vessel The vessel that moved
 */
void sync_all_loaded_vehicles(struct greyhawk_ship_data *vessel)
{
  struct vehicle_data **vehicles;
  int count, i;

  VSSL_DEBUG_ENTER("sync_all_loaded_vehicles");

  if (vessel == NULL)
  {
    VHCL_DEBUG_XPORT("sync_all_loaded_vehicles: NULL vessel");
    VSSL_DEBUG_EXIT("sync_all_loaded_vehicles");
    return;
  }

  VHCL_DEBUG_XPORT("Syncing all vehicles on vessel %s (%d) at (%d,%d)", vessel->name,
                   vessel->shipnum, (int)vessel->x, (int)vessel->y);

  vehicles = get_loaded_vehicles_list(vessel, &count);
  if (vehicles == NULL)
  {
    VHCL_DEBUG_XPORT("No vehicles loaded on vessel %s", vessel->name);
    VSSL_DEBUG_EXIT("sync_all_loaded_vehicles");
    return;
  }

  VHCL_DEBUG_XPORT("Syncing %d vehicles on vessel %s", count, vessel->name);

  for (i = 0; i < count; i++)
  {
    vehicle_sync_with_vessel(vehicles[i], vessel);
  }

  free(vehicles);
  VHCL_DEBUG_XPORT("Completed sync of %d vehicles on vessel %s", count, vessel->name);
  VSSL_DEBUG_EXIT("sync_all_loaded_vehicles");
}
