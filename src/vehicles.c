/* ************************************************************************
 *   File:   vehicles.c                                Part of LuminariMUD *
 *  Usage:   Simple Vehicle System - land-based transport vehicles          *
 *                                                                          *
 *  Phase 02, Session 02: Vehicle Creation System                           *
 *  Implements vehicle lifecycle, state management, capacity tracking,      *
 *  condition handling, lookup functions, and database persistence.         *
 *                                                                          *
 *  All rights reserved.  See license for complete information.            *
 *                                                                          *
 *  LuminariMUD is based on CircleMUD, Copyright (C) 1993, 94.             *
 *  CircleMUD (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "vessels.h"
#include "mysql.h"
#include "wilderness.h"

/* ========================================================================= */
/* EXTERNAL VARIABLES                                                         */
/* ========================================================================= */

extern MYSQL *conn;
extern bool mysql_available;

/* ========================================================================= */
/* GLOBAL VARIABLES                                                           */
/* ========================================================================= */

/**
 * Global vehicle tracking array.
 * NULL entries indicate available slots.
 */
static struct vehicle_data *vehicle_list[MAX_VEHICLES];

/**
 * Counter for unique vehicle ID assignment.
 * Initialized from MAX(vehicle_id) + 1 at boot.
 */
static int next_vehicle_id = 1;

/**
 * Count of active vehicles in the vehicle_list array.
 */
static int vehicle_count = 0;

/* ========================================================================= */
/* FORWARD DECLARATIONS                                                       */
/* ========================================================================= */

static int find_empty_vehicle_slot(void);
static int add_vehicle_to_list(struct vehicle_data *vehicle);
static int remove_vehicle_from_list(struct vehicle_data *vehicle);
void ensure_vehicle_table_exists(void);

/* ========================================================================= */
/* STRING CONVERSION UTILITIES (T005, T006)                                   */
/* ========================================================================= */

/**
 * Convert vehicle type enum to human-readable string.
 *
 * @param type The vehicle type enum value
 * @return Pointer to static string describing the vehicle type
 */
const char *vehicle_type_name(enum vehicle_type type)
{
  switch (type)
  {
  case VEHICLE_NONE:
    return "none";
  case VEHICLE_CART:
    return "cart";
  case VEHICLE_WAGON:
    return "wagon";
  case VEHICLE_MOUNT:
    return "mount";
  case VEHICLE_CARRIAGE:
    return "carriage";
  default:
    return "unknown";
  }
}

/**
 * Convert vehicle state enum to human-readable string.
 *
 * @param state The vehicle state enum value
 * @return Pointer to static string describing the vehicle state
 */
const char *vehicle_state_name(enum vehicle_state state)
{
  switch (state)
  {
  case VSTATE_IDLE:
    return "idle";
  case VSTATE_MOVING:
    return "moving";
  case VSTATE_LOADED:
    return "loaded";
  case VSTATE_HITCHED:
    return "hitched";
  case VSTATE_DAMAGED:
    return "damaged";
  case VSTATE_ON_VESSEL:
    return "on vessel";
  default:
    return "unknown";
  }
}

/* ========================================================================= */
/* VEHICLE INITIALIZATION (T007)                                              */
/* ========================================================================= */

/**
 * Initialize vehicle with default values based on type.
 *
 * Sets capacity, speed, terrain capabilities, and condition
 * according to the vehicle type constants defined in vessels.h.
 *
 * @param vehicle Pointer to vehicle structure to initialize
 * @param type The vehicle type to initialize as
 */
void vehicle_init(struct vehicle_data *vehicle, enum vehicle_type type)
{
  if (vehicle == NULL)
  {
    log("SYSERR: vehicle_init called with NULL vehicle");
    return;
  }

  /* Set type */
  vehicle->type = type;

  /* Set state to idle */
  vehicle->state = VSTATE_IDLE;

  /* Initialize location */
  vehicle->location = NOWHERE;
  vehicle->direction = 0;
  vehicle->x_coord = 0;
  vehicle->y_coord = 0;
  vehicle->parent_vessel_id = 0; /* Not loaded on any vessel (S0205) */

  /* Set type-specific defaults */
  switch (type)
  {
  case VEHICLE_CART:
    vehicle->max_passengers = VEHICLE_PASSENGERS_CART;
    vehicle->max_weight = VEHICLE_WEIGHT_CART;
    vehicle->base_speed = VEHICLE_SPEED_CART;
    vehicle->terrain_flags = VTERRAIN_CART_DEFAULT;
    break;

  case VEHICLE_WAGON:
    vehicle->max_passengers = VEHICLE_PASSENGERS_WAGON;
    vehicle->max_weight = VEHICLE_WEIGHT_WAGON;
    vehicle->base_speed = VEHICLE_SPEED_WAGON;
    vehicle->terrain_flags = VTERRAIN_WAGON_DEFAULT;
    break;

  case VEHICLE_MOUNT:
    vehicle->max_passengers = VEHICLE_PASSENGERS_MOUNT;
    vehicle->max_weight = VEHICLE_WEIGHT_MOUNT;
    vehicle->base_speed = VEHICLE_SPEED_MOUNT;
    vehicle->terrain_flags = VTERRAIN_MOUNT_DEFAULT;
    break;

  case VEHICLE_CARRIAGE:
    vehicle->max_passengers = VEHICLE_PASSENGERS_CARRIAGE;
    vehicle->max_weight = VEHICLE_WEIGHT_CARRIAGE;
    vehicle->base_speed = VEHICLE_SPEED_CARRIAGE;
    vehicle->terrain_flags = VTERRAIN_CARRIAGE_DEFAULT;
    break;

  default:
    /* Default to cart values for unknown types */
    vehicle->max_passengers = VEHICLE_PASSENGERS_CART;
    vehicle->max_weight = VEHICLE_WEIGHT_CART;
    vehicle->base_speed = VEHICLE_SPEED_CART;
    vehicle->terrain_flags = VTERRAIN_CART_DEFAULT;
    break;
  }

  /* Initialize current values */
  vehicle->current_passengers = 0;
  vehicle->current_weight = 0;
  vehicle->current_speed = vehicle->base_speed;

  /* Initialize condition */
  vehicle->max_condition = VEHICLE_CONDITION_MAX;
  vehicle->condition = VEHICLE_CONDITION_MAX;

  /* Initialize ownership */
  vehicle->owner_id = 0;
  vehicle->obj = NULL;
}

/* ========================================================================= */
/* ARRAY SLOT MANAGEMENT (T008)                                               */
/* ========================================================================= */

/**
 * Find an empty slot in the vehicle_list array.
 *
 * @return Index of empty slot, or -1 if array is full
 */
static int find_empty_vehicle_slot(void)
{
  int i;

  for (i = 0; i < MAX_VEHICLES; i++)
  {
    if (vehicle_list[i] == NULL)
    {
      return i;
    }
  }

  return -1;
}

/**
 * Add a vehicle to the tracking array.
 *
 * @param vehicle Pointer to vehicle to add
 * @return Index where vehicle was added, or -1 on failure
 */
static int add_vehicle_to_list(struct vehicle_data *vehicle)
{
  int slot;

  if (vehicle == NULL)
  {
    log("SYSERR: add_vehicle_to_list called with NULL vehicle");
    return -1;
  }

  slot = find_empty_vehicle_slot();
  if (slot < 0)
  {
    log("SYSERR: Vehicle array is full (MAX_VEHICLES=%d)", MAX_VEHICLES);
    return -1;
  }

  vehicle_list[slot] = vehicle;
  vehicle_count++;

  return slot;
}

/**
 * Remove a vehicle from the tracking array.
 *
 * @param vehicle Pointer to vehicle to remove
 * @return 1 on success, 0 if vehicle not found
 */
static int remove_vehicle_from_list(struct vehicle_data *vehicle)
{
  int i;

  if (vehicle == NULL)
  {
    return 0;
  }

  for (i = 0; i < MAX_VEHICLES; i++)
  {
    if (vehicle_list[i] == vehicle)
    {
      vehicle_list[i] = NULL;
      vehicle_count--;
      return 1;
    }
  }

  return 0;
}

/* ========================================================================= */
/* VEHICLE LIFECYCLE FUNCTIONS (T009, T010)                                   */
/* ========================================================================= */

/**
 * Create a new vehicle of the specified type.
 *
 * Allocates memory, initializes with type defaults, assigns unique ID,
 * and registers in the global tracking array.
 *
 * @param type The vehicle type to create
 * @param name Optional name for the vehicle (can be NULL)
 * @return Pointer to new vehicle, or NULL on failure
 */
struct vehicle_data *vehicle_create(enum vehicle_type type, const char *name)
{
  struct vehicle_data *vehicle;

  /* Validate type */
  if (type <= VEHICLE_NONE || type >= NUM_VEHICLE_TYPES)
  {
    log("SYSERR: vehicle_create called with invalid type %d", type);
    return NULL;
  }

  /* Check for available slot */
  if (vehicle_count >= MAX_VEHICLES)
  {
    log("SYSERR: Cannot create vehicle, array is full");
    return NULL;
  }

  /* Allocate memory */
  CREATE(vehicle, struct vehicle_data, 1);
  if (vehicle == NULL)
  {
    log("SYSERR: Out of memory in vehicle_create");
    return NULL;
  }

  /* Clear memory */
  memset(vehicle, 0, sizeof(struct vehicle_data));

  /* Assign unique ID */
  vehicle->id = next_vehicle_id++;

  /* Set name */
  if (name != NULL && name[0] != '\0')
  {
    strncpy(vehicle->name, name, VEHICLE_NAME_LENGTH - 1);
    vehicle->name[VEHICLE_NAME_LENGTH - 1] = '\0';
  }
  else
  {
    /* Generate default name based on type */
    snprintf(vehicle->name, VEHICLE_NAME_LENGTH, "a %s", vehicle_type_name(type));
  }

  /* Initialize with type defaults */
  vehicle_init(vehicle, type);

  /* Add to tracking array */
  if (add_vehicle_to_list(vehicle) < 0)
  {
    free(vehicle);
    return NULL;
  }

  log("Info: Created vehicle #%d (%s) type=%s", vehicle->id, vehicle->name,
      vehicle_type_name(type));

  return vehicle;
}

/**
 * Destroy a vehicle and free all associated resources.
 *
 * Removes from tracking array, clears references, and frees memory.
 *
 * @param vehicle Pointer to vehicle to destroy
 */
void vehicle_destroy(struct vehicle_data *vehicle)
{
  if (vehicle == NULL)
  {
    return;
  }

  log("Info: Destroying vehicle #%d (%s)", vehicle->id, vehicle->name);

  /* Remove from tracking array */
  remove_vehicle_from_list(vehicle);

  /* Clear object reference (don't free the obj, just clear the pointer) */
  vehicle->obj = NULL;

  /* Free the vehicle structure */
  free(vehicle);
}

/* ========================================================================= */
/* STATE MANAGEMENT FUNCTIONS (T011)                                          */
/* ========================================================================= */

/**
 * Set the vehicle's current state.
 *
 * Validates state transitions and updates the state.
 *
 * @param vehicle Pointer to vehicle
 * @param new_state The state to transition to
 * @return 1 on success, 0 on failure (invalid transition)
 */
int vehicle_set_state(struct vehicle_data *vehicle, enum vehicle_state new_state)
{
  if (vehicle == NULL)
  {
    log("SYSERR: vehicle_set_state called with NULL vehicle");
    return 0;
  }

  /* Validate new state */
  if (new_state < VSTATE_IDLE || new_state >= NUM_VEHICLE_STATES)
  {
    log("SYSERR: vehicle_set_state invalid state %d", new_state);
    return 0;
  }

  /* Validate state transitions */
  switch (vehicle->state)
  {
  case VSTATE_DAMAGED:
    /* Damaged vehicles can only transition to idle (after repair) */
    if (new_state != VSTATE_IDLE && new_state != VSTATE_DAMAGED)
    {
      return 0;
    }
    break;

  case VSTATE_HITCHED:
    /* Hitched vehicles can only unhitch (to idle) or stay hitched */
    if (new_state != VSTATE_IDLE && new_state != VSTATE_HITCHED && new_state != VSTATE_DAMAGED)
    {
      return 0;
    }
    break;

  default:
    /* Other states allow most transitions */
    break;
  }

  vehicle->state = new_state;
  return 1;
}

/**
 * Get the vehicle's current state.
 *
 * @param vehicle Pointer to vehicle
 * @return Current vehicle state, or VSTATE_IDLE if NULL
 */
enum vehicle_state vehicle_get_state(struct vehicle_data *vehicle)
{
  if (vehicle == NULL)
  {
    return VSTATE_IDLE;
  }

  return vehicle->state;
}

/* ========================================================================= */
/* CAPACITY FUNCTIONS (T012, T013)                                            */
/* ========================================================================= */

/**
 * Check if vehicle can accept another passenger.
 *
 * @param vehicle Pointer to vehicle
 * @return 1 if space available, 0 if full or invalid
 */
int vehicle_can_add_passenger(struct vehicle_data *vehicle)
{
  if (vehicle == NULL)
  {
    return 0;
  }

  if (vehicle->state == VSTATE_DAMAGED)
  {
    return 0;
  }

  return (vehicle->current_passengers < vehicle->max_passengers);
}

/**
 * Add a passenger to the vehicle.
 *
 * @param vehicle Pointer to vehicle
 * @return 1 on success, 0 if vehicle is full or invalid
 */
int vehicle_add_passenger(struct vehicle_data *vehicle)
{
  if (!vehicle_can_add_passenger(vehicle))
  {
    return 0;
  }

  vehicle->current_passengers++;

  /* Update state if vehicle now has passengers */
  if (vehicle->state == VSTATE_IDLE && vehicle->current_passengers > 0)
  {
    vehicle->state = VSTATE_LOADED;
  }

  return 1;
}

/**
 * Remove a passenger from the vehicle.
 *
 * @param vehicle Pointer to vehicle
 * @return 1 on success, 0 if no passengers or invalid
 */
int vehicle_remove_passenger(struct vehicle_data *vehicle)
{
  if (vehicle == NULL)
  {
    return 0;
  }

  if (vehicle->current_passengers <= 0)
  {
    return 0;
  }

  vehicle->current_passengers--;

  /* Update state if vehicle now empty */
  if (vehicle->current_passengers == 0 && vehicle->current_weight == 0 &&
      vehicle->state == VSTATE_LOADED)
  {
    vehicle->state = VSTATE_IDLE;
  }

  return 1;
}

/**
 * Check if vehicle can accept additional weight.
 *
 * @param vehicle Pointer to vehicle
 * @param weight Amount of weight to add
 * @return 1 if within capacity, 0 if overweight or invalid
 */
int vehicle_can_add_weight(struct vehicle_data *vehicle, int weight)
{
  if (vehicle == NULL || weight < 0)
  {
    return 0;
  }

  if (vehicle->state == VSTATE_DAMAGED)
  {
    return 0;
  }

  return ((vehicle->current_weight + weight) <= vehicle->max_weight);
}

/**
 * Add weight to the vehicle.
 *
 * @param vehicle Pointer to vehicle
 * @param weight Amount of weight to add
 * @return 1 on success, 0 if overweight or invalid
 */
int vehicle_add_weight(struct vehicle_data *vehicle, int weight)
{
  if (!vehicle_can_add_weight(vehicle, weight))
  {
    return 0;
  }

  vehicle->current_weight += weight;

  /* Update state if vehicle now has cargo */
  if (vehicle->state == VSTATE_IDLE &&
      (vehicle->current_weight > 0 || vehicle->current_passengers > 0))
  {
    vehicle->state = VSTATE_LOADED;
  }

  return 1;
}

/**
 * Remove weight from the vehicle.
 *
 * @param vehicle Pointer to vehicle
 * @param weight Amount of weight to remove
 * @return 1 on success, 0 if insufficient weight or invalid
 */
int vehicle_remove_weight(struct vehicle_data *vehicle, int weight)
{
  if (vehicle == NULL || weight < 0)
  {
    return 0;
  }

  if (vehicle->current_weight < weight)
  {
    return 0;
  }

  vehicle->current_weight -= weight;

  /* Update state if vehicle now empty */
  if (vehicle->current_passengers == 0 && vehicle->current_weight == 0 &&
      vehicle->state == VSTATE_LOADED)
  {
    vehicle->state = VSTATE_IDLE;
  }

  return 1;
}

/* ========================================================================= */
/* CONDITION FUNCTIONS (T014)                                                  */
/* ========================================================================= */

/**
 * Apply damage to a vehicle.
 *
 * Reduces condition by the specified amount, clamped to 0.
 * Sets state to DAMAGED if condition reaches 0.
 *
 * @param vehicle Pointer to vehicle
 * @param amount Amount of damage to apply
 * @return New condition value, or -1 on error
 */
int vehicle_damage(struct vehicle_data *vehicle, int amount)
{
  if (vehicle == NULL)
  {
    return -1;
  }

  if (amount < 0)
  {
    amount = 0;
  }

  vehicle->condition -= amount;

  /* Clamp to minimum */
  if (vehicle->condition < VEHICLE_CONDITION_BROKEN)
  {
    vehicle->condition = VEHICLE_CONDITION_BROKEN;
  }

  /* Update state if broken */
  if (vehicle->condition <= VEHICLE_CONDITION_BROKEN)
  {
    vehicle->state = VSTATE_DAMAGED;
  }

  return vehicle->condition;
}

/**
 * Repair a vehicle.
 *
 * Increases condition by the specified amount, clamped to max.
 * Clears DAMAGED state if condition is restored above 0.
 *
 * @param vehicle Pointer to vehicle
 * @param amount Amount to repair
 * @return New condition value, or -1 on error
 */
int vehicle_repair(struct vehicle_data *vehicle, int amount)
{
  if (vehicle == NULL)
  {
    return -1;
  }

  if (amount < 0)
  {
    amount = 0;
  }

  vehicle->condition += amount;

  /* Clamp to maximum */
  if (vehicle->condition > vehicle->max_condition)
  {
    vehicle->condition = vehicle->max_condition;
  }

  /* Clear damaged state if repaired */
  if (vehicle->state == VSTATE_DAMAGED && vehicle->condition > VEHICLE_CONDITION_BROKEN)
  {
    vehicle->state = VSTATE_IDLE;
  }

  return vehicle->condition;
}

/**
 * Check if a vehicle is operational.
 *
 * A vehicle is operational if it is not damaged (condition > 0).
 *
 * @param vehicle Pointer to vehicle
 * @return 1 if operational, 0 if damaged or invalid
 */
int vehicle_is_operational(struct vehicle_data *vehicle)
{
  if (vehicle == NULL)
  {
    return 0;
  }

  return (vehicle->condition > VEHICLE_CONDITION_BROKEN && vehicle->state != VSTATE_DAMAGED);
}

/* ========================================================================= */
/* LOOKUP FUNCTIONS (T015)                                                     */
/* ========================================================================= */

/**
 * Find a vehicle by its unique ID.
 *
 * @param id The vehicle ID to search for
 * @return Pointer to vehicle, or NULL if not found
 */
struct vehicle_data *vehicle_find_by_id(int id)
{
  int i;

  if (id <= 0)
  {
    return NULL;
  }

  for (i = 0; i < MAX_VEHICLES; i++)
  {
    if (vehicle_list[i] != NULL && vehicle_list[i]->id == id)
    {
      return vehicle_list[i];
    }
  }

  return NULL;
}

/**
 * Find the first vehicle in a specific room.
 *
 * @param room The room number to search
 * @return Pointer to first vehicle in room, or NULL if none
 */
struct vehicle_data *vehicle_find_in_room(room_rnum room)
{
  int i;

  if (room == NOWHERE)
  {
    return NULL;
  }

  for (i = 0; i < MAX_VEHICLES; i++)
  {
    if (vehicle_list[i] != NULL && vehicle_list[i]->location == room)
    {
      return vehicle_list[i];
    }
  }

  return NULL;
}

/**
 * Find a vehicle by its associated object.
 *
 * @param obj Pointer to object to search for
 * @return Pointer to vehicle, or NULL if not found
 */
struct vehicle_data *vehicle_find_by_obj(struct obj_data *obj)
{
  int i;

  if (obj == NULL)
  {
    return NULL;
  }

  for (i = 0; i < MAX_VEHICLES; i++)
  {
    if (vehicle_list[i] != NULL && vehicle_list[i]->obj == obj)
    {
      return vehicle_list[i];
    }
  }

  return NULL;
}

/* ========================================================================= */
/* DATABASE PERSISTENCE (T016, T017, T018)                                     */
/* ========================================================================= */

/**
 * Ensure the vehicle_data table exists in the database.
 * Creates the table if it does not exist.
 */
void ensure_vehicle_table_exists(void)
{
  const char *create_query =
      "CREATE TABLE IF NOT EXISTS vehicle_data ("
      "  vehicle_id INT AUTO_INCREMENT PRIMARY KEY,"
      "  vehicle_type INT NOT NULL DEFAULT 0,"
      "  vehicle_state INT NOT NULL DEFAULT 0,"
      "  vehicle_name VARCHAR(64) NOT NULL DEFAULT '',"
      "  location INT NOT NULL DEFAULT 0,"
      "  direction INT NOT NULL DEFAULT 0,"
      "  x_coord INT NOT NULL DEFAULT 0,"
      "  y_coord INT NOT NULL DEFAULT 0,"
      "  max_passengers INT NOT NULL DEFAULT 0,"
      "  current_passengers INT NOT NULL DEFAULT 0,"
      "  max_weight INT NOT NULL DEFAULT 0,"
      "  current_weight INT NOT NULL DEFAULT 0,"
      "  base_speed INT NOT NULL DEFAULT 0,"
      "  current_speed INT NOT NULL DEFAULT 0,"
      "  terrain_flags INT NOT NULL DEFAULT 0,"
      "  max_condition INT NOT NULL DEFAULT 100,"
      "  vehicle_condition INT NOT NULL DEFAULT 100,"
      "  owner_id BIGINT NOT NULL DEFAULT 0,"
      "  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
      "  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,"
      "  INDEX idx_location (location),"
      "  INDEX idx_owner (owner_id),"
      "  INDEX idx_coords (x_coord, y_coord)"
      ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";

  if (!mysql_available)
  {
    log("Info: MySQL not available, vehicle table check skipped");
    return;
  }

  if (mysql_query(conn, create_query))
  {
    log("SYSERR: Unable to create vehicle_data table: %s", mysql_error(conn));
  }
  else
  {
    log("Info: vehicle_data table verified");
  }
}

/**
 * Save a single vehicle to the database.
 *
 * @param vehicle Pointer to vehicle to save
 * @return 1 on success, 0 on failure
 */
int vehicle_save(struct vehicle_data *vehicle)
{
  char query[MAX_STRING_LENGTH];
  char escaped_name[VEHICLE_NAME_LENGTH * 2 + 1];

  if (!mysql_available || vehicle == NULL)
  {
    return 0;
  }

  /* Escape vehicle name */
  mysql_real_escape_string(conn, escaped_name, vehicle->name, strlen(vehicle->name));

  /* Build query - use REPLACE for upsert */
  snprintf(query, sizeof(query),
           "REPLACE INTO vehicle_data "
           "(vehicle_id, vehicle_type, vehicle_state, vehicle_name, "
           "location, direction, x_coord, y_coord, max_passengers, current_passengers, "
           "max_weight, current_weight, base_speed, current_speed, "
           "terrain_flags, max_condition, vehicle_condition, owner_id, parent_vessel_id) "
           "VALUES (%d, %d, %d, '%s', %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %ld, %d)",
           vehicle->id, vehicle->type, vehicle->state, escaped_name, vehicle->location,
           vehicle->direction, vehicle->x_coord, vehicle->y_coord, vehicle->max_passengers,
           vehicle->current_passengers, vehicle->max_weight, vehicle->current_weight,
           vehicle->base_speed, vehicle->current_speed, vehicle->terrain_flags,
           vehicle->max_condition, vehicle->condition, vehicle->owner_id,
           vehicle->parent_vessel_id);

  if (mysql_query(conn, query))
  {
    log("SYSERR: Unable to save vehicle #%d: %s", vehicle->id, mysql_error(conn));
    return 0;
  }

  return 1;
}

/**
 * Load a vehicle from the database.
 *
 * @param vehicle_id The ID of the vehicle to load
 * @param vehicle Pointer to vehicle structure to populate
 * @return 1 on success, 0 on failure or not found
 */
int vehicle_load(int vehicle_id, struct vehicle_data *vehicle)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  char query[MAX_STRING_LENGTH];

  if (!mysql_available || vehicle == NULL || vehicle_id <= 0)
  {
    return 0;
  }

  snprintf(query, sizeof(query),
           "SELECT vehicle_id, vehicle_type, vehicle_state, vehicle_name, "
           "location, direction, x_coord, y_coord, max_passengers, current_passengers, "
           "max_weight, current_weight, base_speed, current_speed, "
           "terrain_flags, max_condition, vehicle_condition, owner_id, parent_vessel_id "
           "FROM vehicle_data WHERE vehicle_id = %d",
           vehicle_id);

  if (mysql_query(conn, query))
  {
    log("SYSERR: Unable to load vehicle #%d: %s", vehicle_id, mysql_error(conn));
    return 0;
  }

  result = mysql_store_result(conn);
  if (!result)
  {
    log("SYSERR: Unable to store result for vehicle #%d: %s", vehicle_id, mysql_error(conn));
    return 0;
  }

  if ((row = mysql_fetch_row(result)))
  {
    vehicle->id = atoi(row[0]);
    vehicle->type = atoi(row[1]);
    vehicle->state = atoi(row[2]);

    if (row[3])
    {
      strncpy(vehicle->name, row[3], VEHICLE_NAME_LENGTH - 1);
      vehicle->name[VEHICLE_NAME_LENGTH - 1] = '\0';
    }

    vehicle->location = atoi(row[4]);
    vehicle->direction = atoi(row[5]);
    vehicle->x_coord = atoi(row[6]);
    vehicle->y_coord = atoi(row[7]);
    vehicle->max_passengers = atoi(row[8]);
    vehicle->current_passengers = atoi(row[9]);
    vehicle->max_weight = atoi(row[10]);
    vehicle->current_weight = atoi(row[11]);
    vehicle->base_speed = atoi(row[12]);
    vehicle->current_speed = atoi(row[13]);
    vehicle->terrain_flags = atoi(row[14]);
    vehicle->max_condition = atoi(row[15]);
    vehicle->condition = atoi(row[16]);
    vehicle->owner_id = atol(row[17]);
    vehicle->parent_vessel_id = row[18] ? atoi(row[18]) : 0;
    vehicle->obj = NULL;

    mysql_free_result(result);
    return 1;
  }

  mysql_free_result(result);
  return 0;
}

/**
 * Save all active vehicles to the database.
 * Called at server shutdown and during auto-save.
 */
void vehicle_save_all(void)
{
  int i;
  int saved_count = 0;

  if (!mysql_available)
  {
    log("Info: MySQL not available, skipping vehicle save");
    return;
  }

  log("Info: Saving all vehicles to database...");

  for (i = 0; i < MAX_VEHICLES; i++)
  {
    if (vehicle_list[i] != NULL)
    {
      if (vehicle_save(vehicle_list[i]))
      {
        saved_count++;
      }
    }
  }

  log("Info: Saved %d vehicles to database", saved_count);
}

/**
 * Load all vehicles from the database.
 * Called at server startup.
 */
void vehicle_load_all(void)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  const char *query;
  struct vehicle_data *vehicle;
  int loaded_count = 0;
  int max_id = 0;

  if (!mysql_available)
  {
    log("Info: MySQL not available, skipping vehicle load");
    return;
  }

  /* Ensure table exists */
  ensure_vehicle_table_exists();

  log("Info: Loading vehicles from database...");

  /* Query all vehicles */
  query = "SELECT vehicle_id, vehicle_type, vehicle_state, vehicle_name, "
          "location, direction, x_coord, y_coord, max_passengers, current_passengers, "
          "max_weight, current_weight, base_speed, current_speed, "
          "terrain_flags, max_condition, vehicle_condition, owner_id "
          "FROM vehicle_data ORDER BY vehicle_id";

  if (mysql_query(conn, query))
  {
    log("SYSERR: Unable to load vehicles: %s", mysql_error(conn));
    return;
  }

  result = mysql_store_result(conn);
  if (!result)
  {
    log("SYSERR: Unable to store vehicle result: %s", mysql_error(conn));
    return;
  }

  while ((row = mysql_fetch_row(result)))
  {
    /* Allocate new vehicle */
    CREATE(vehicle, struct vehicle_data, 1);
    if (vehicle == NULL)
    {
      log("SYSERR: Out of memory loading vehicles");
      break;
    }

    /* Clear and populate */
    memset(vehicle, 0, sizeof(struct vehicle_data));

    vehicle->id = atoi(row[0]);
    vehicle->type = atoi(row[1]);
    vehicle->state = atoi(row[2]);

    if (row[3])
    {
      strncpy(vehicle->name, row[3], VEHICLE_NAME_LENGTH - 1);
      vehicle->name[VEHICLE_NAME_LENGTH - 1] = '\0';
    }

    vehicle->location = atoi(row[4]);
    vehicle->direction = atoi(row[5]);
    vehicle->x_coord = atoi(row[6]);
    vehicle->y_coord = atoi(row[7]);
    vehicle->max_passengers = atoi(row[8]);
    vehicle->current_passengers = atoi(row[9]);
    vehicle->max_weight = atoi(row[10]);
    vehicle->current_weight = atoi(row[11]);
    vehicle->base_speed = atoi(row[12]);
    vehicle->current_speed = atoi(row[13]);
    vehicle->terrain_flags = atoi(row[14]);
    vehicle->max_condition = atoi(row[15]);
    vehicle->condition = atoi(row[16]);
    vehicle->owner_id = atol(row[17]);
    vehicle->obj = NULL;

    /* Track max ID for next_vehicle_id */
    if (vehicle->id > max_id)
    {
      max_id = vehicle->id;
    }

    /* Add to tracking array */
    if (add_vehicle_to_list(vehicle) < 0)
    {
      log("SYSERR: Could not add loaded vehicle #%d to list", vehicle->id);
      free(vehicle);
      continue;
    }

    loaded_count++;
  }

  mysql_free_result(result);

  /* Set next_vehicle_id to avoid collisions */
  next_vehicle_id = max_id + 1;

  log("Info: Loaded %d vehicles (next_vehicle_id=%d)", loaded_count, next_vehicle_id);
}

/* ========================================================================= */
/* VEHICLE MOVEMENT FUNCTIONS (Phase 02, Session 03)                          */
/* ========================================================================= */

/**
 * Get the X/Y coordinate delta for a given direction.
 *
 * Converts MUD direction constants to wilderness coordinate changes.
 * Supports all 8 cardinal and diagonal directions.
 *
 * @param direction The direction constant (NORTH, EAST, etc.)
 * @param dx Pointer to store X delta (-1, 0, or +1)
 * @param dy Pointer to store Y delta (-1, 0, or +1)
 */
void vehicle_get_direction_delta(int direction, int *dx, int *dy)
{
  if (dx == NULL || dy == NULL)
  {
    log("SYSERR: vehicle_get_direction_delta called with NULL pointer");
    return;
  }

  /* Initialize to no movement */
  *dx = 0;
  *dy = 0;

  switch (direction)
  {
  case NORTH:
    *dy = 1;
    break;
  case SOUTH:
    *dy = -1;
    break;
  case EAST:
    *dx = 1;
    break;
  case WEST:
    *dx = -1;
    break;
  case NORTHEAST:
    *dx = 1;
    *dy = 1;
    break;
  case NORTHWEST:
    *dx = -1;
    *dy = 1;
    break;
  case SOUTHEAST:
    *dx = 1;
    *dy = -1;
    break;
  case SOUTHWEST:
    *dx = -1;
    *dy = -1;
    break;
  default:
    /* Invalid direction (UP, DOWN, etc.) - no movement */
    break;
  }
}

/**
 * Map sector type to vehicle terrain capability flag.
 *
 * Converts SECT_* constants to VTERRAIN_* bitfield values.
 * Returns 0 for terrain that vehicles cannot traverse (water, air, etc.).
 *
 * @param sector_type The SECT_* sector type constant
 * @return The corresponding VTERRAIN_* flag, or 0 if impassable
 */
int sector_to_vterrain(int sector_type)
{
  switch (sector_type)
  {
  /* Road terrain - fastest travel */
  case SECT_ROAD_NS:
  case SECT_ROAD_EW:
  case SECT_ROAD_INT:
  case SECT_D_ROAD_NS:
  case SECT_D_ROAD_EW:
  case SECT_D_ROAD_INT:
    return VTERRAIN_ROAD;

  /* Plains/field terrain - standard travel */
  case SECT_FIELD:
  case SECT_CITY:
  case SECT_INSIDE:
  case SECT_INSIDE_ROOM:
    return VTERRAIN_PLAINS;

  /* Forest terrain - slower travel */
  case SECT_FOREST:
  case SECT_JUNGLE:
  case SECT_TAIGA:
    return VTERRAIN_FOREST;

  /* Hilly terrain - slower travel */
  case SECT_HILLS:
  case SECT_BEACH:
    return VTERRAIN_HILLS;

  /* Mountain terrain - slowest land travel */
  case SECT_MOUNTAIN:
  case SECT_HIGH_MOUNTAIN:
  case SECT_CAVE:
    return VTERRAIN_MOUNTAIN;

  /* Desert terrain - slow travel */
  case SECT_DESERT:
  case SECT_TUNDRA:
    return VTERRAIN_DESERT;

  /* Swamp terrain - slow and difficult */
  case SECT_MARSHLAND:
    return VTERRAIN_SWAMP;

  /* Impassable terrain for land vehicles */
  case SECT_WATER_SWIM:
  case SECT_WATER_NOSWIM:
  case SECT_OCEAN:
  case SECT_UNDERWATER:
  case SECT_RIVER:
  case SECT_FLYING:
  case SECT_LAVA:
  case SECT_UD_WATER:
  case SECT_UD_NOSWIM:
  case SECT_UD_NOGROUND:
  default:
    return 0; /* Impassable */
  }
}

/**
 * Check if a vehicle can traverse the given sector type.
 *
 * Compares the vehicle's terrain_flags bitfield against the
 * terrain capability required for the sector type.
 *
 * @param vehicle Pointer to the vehicle
 * @param sector_type The SECT_* sector type to check
 * @return 1 if vehicle can traverse, 0 if blocked
 */
int vehicle_can_traverse_terrain(struct vehicle_data *vehicle, int sector_type)
{
  int required_terrain;

  if (vehicle == NULL)
  {
    log("SYSERR: vehicle_can_traverse_terrain called with NULL vehicle");
    return 0;
  }

  /* Get required terrain capability */
  required_terrain = sector_to_vterrain(sector_type);

  /* Impassable terrain */
  if (required_terrain == 0)
  {
    return 0;
  }

  /* Check if vehicle has the required terrain capability */
  return (vehicle->terrain_flags & required_terrain) ? 1 : 0;
}

/**
 * Get the speed modifier for a vehicle on a given terrain type.
 *
 * Returns a percentage (50-150) to multiply against base speed.
 * Higher values = faster travel.
 *
 * @param vehicle Pointer to the vehicle (for future type-specific modifiers)
 * @param sector_type The SECT_* sector type
 * @return Speed modifier percentage (50-150), or 0 if impassable
 */
int get_vehicle_speed_modifier(struct vehicle_data *vehicle, int sector_type)
{
  int terrain;

  if (vehicle == NULL)
  {
    return 0;
  }

  terrain = sector_to_vterrain(sector_type);

  /* Impassable terrain */
  if (terrain == 0)
  {
    return 0;
  }

  /* Return speed modifier based on terrain type */
  switch (terrain)
  {
  case VTERRAIN_ROAD:
    return VEHICLE_SPEED_MOD_ROAD;
  case VTERRAIN_PLAINS:
    return VEHICLE_SPEED_MOD_PLAINS;
  case VTERRAIN_FOREST:
    return VEHICLE_SPEED_MOD_FOREST;
  case VTERRAIN_HILLS:
    return VEHICLE_SPEED_MOD_HILLS;
  case VTERRAIN_MOUNTAIN:
    return VEHICLE_SPEED_MOD_MOUNTAIN;
  case VTERRAIN_SWAMP:
    return VEHICLE_SPEED_MOD_SWAMP;
  case VTERRAIN_DESERT:
    return VEHICLE_SPEED_MOD_DESERT;
  default:
    return VEHICLE_SPEED_MOD_PLAINS; /* Fallback to base speed */
  }
}

/**
 * Get the current effective speed of a vehicle.
 *
 * Applies load and damage modifiers to base speed.
 *
 * @param vehicle Pointer to the vehicle
 * @return Current effective speed, or 0 if unable to move
 */
int vehicle_get_speed(struct vehicle_data *vehicle)
{
  int speed;

  if (vehicle == NULL)
  {
    return 0;
  }

  /* Cannot move if damaged */
  if (vehicle->state == VSTATE_DAMAGED)
  {
    return 0;
  }

  /* Cannot move if hitched */
  if (vehicle->state == VSTATE_HITCHED)
  {
    return 0;
  }

  /* Start with base speed */
  speed = vehicle->base_speed;

  /* Apply damage modifier if condition is poor */
  if (vehicle->condition < VEHICLE_CONDITION_FAIR)
  {
    speed = (speed * VEHICLE_SPEED_MOD_DAMAGED) / 100;
  }

  /* Apply load modifier if heavily loaded */
  if (vehicle->current_weight > (vehicle->max_weight * 75 / 100) ||
      vehicle->current_passengers == vehicle->max_passengers)
  {
    speed = (speed * VEHICLE_SPEED_MOD_LOADED) / 100;
  }

  /* Ensure minimum speed of 1 if moving is possible */
  if (speed < 1)
  {
    speed = 1;
  }

  return speed;
}

/**
 * Check if a vehicle can move in the given direction.
 *
 * Validates:
 * - Vehicle is not NULL and is operational
 * - Direction is valid for wilderness movement
 * - Destination coordinates are within bounds
 * - Destination terrain is traversable
 *
 * @param vehicle Pointer to the vehicle
 * @param direction The direction to move (NORTH, EAST, etc.)
 * @return 1 if movement is possible, 0 if blocked
 */
int vehicle_can_move(struct vehicle_data *vehicle, int direction)
{
  int dx, dy;
  int new_x, new_y;
  room_rnum dest_room;
  int sector;

  if (vehicle == NULL)
  {
    return 0;
  }

  /* Check if vehicle is operational */
  if (!vehicle_is_operational(vehicle))
  {
    return 0;
  }

  /* Check if vehicle can move (not hitched) */
  if (vehicle->state == VSTATE_HITCHED)
  {
    return 0;
  }

  /* Get direction delta */
  vehicle_get_direction_delta(direction, &dx, &dy);

  /* Invalid direction (no movement) */
  if (dx == 0 && dy == 0)
  {
    return 0;
  }

  /* Calculate destination coordinates */
  new_x = vehicle->x_coord + dx;
  new_y = vehicle->y_coord + dy;

  /* Check wilderness bounds */
  if (new_x < VEHICLE_WILDERNESS_MIN_X || new_x > VEHICLE_WILDERNESS_MAX_X ||
      new_y < VEHICLE_WILDERNESS_MIN_Y || new_y > VEHICLE_WILDERNESS_MAX_Y)
  {
    return 0;
  }

  /* Try to find or allocate destination room */
  dest_room = find_room_by_coordinates(new_x, new_y);
  if (dest_room == NOWHERE)
  {
    /* Try to allocate a new room */
    dest_room = find_available_wilderness_room();
    if (dest_room == NOWHERE)
    {
      return 0; /* Room pool exhausted */
    }
  }

  /* Get sector type at destination */
  sector = get_modified_sector_type(real_zone(WILD_ZONE_VNUM), new_x, new_y);

  /* Check terrain traversability */
  if (!vehicle_can_traverse_terrain(vehicle, sector))
  {
    return 0;
  }

  return 1;
}

/**
 * Move a vehicle in the given direction.
 *
 * Core movement function that:
 * 1. Validates movement is possible
 * 2. Calculates new coordinates
 * 3. Allocates/assigns wilderness room
 * 4. Updates vehicle state
 * 5. Applies speed modifiers
 * 6. Persists position to database
 *
 * @param vehicle Pointer to the vehicle
 * @param direction The direction to move (NORTH, EAST, etc.)
 * @return 1 on success, 0 on failure
 */
int move_vehicle(struct vehicle_data *vehicle, int direction)
{
  int dx, dy;
  int new_x, new_y;
  room_rnum dest_room;
  int sector;
  int speed_mod;
  enum vehicle_state prev_state;

  /* Validate vehicle */
  if (vehicle == NULL)
  {
    log("SYSERR: move_vehicle called with NULL vehicle");
    return 0;
  }

  /* Check if movement is possible */
  if (!vehicle_can_move(vehicle, direction))
  {
    return 0;
  }

  /* Get direction delta */
  vehicle_get_direction_delta(direction, &dx, &dy);

  /* Calculate destination coordinates */
  new_x = vehicle->x_coord + dx;
  new_y = vehicle->y_coord + dy;

  /* Get or allocate destination room */
  dest_room = find_room_by_coordinates(new_x, new_y);
  if (dest_room == NOWHERE)
  {
    dest_room = find_available_wilderness_room();
    if (dest_room == NOWHERE)
    {
      log("SYSERR: move_vehicle - room pool exhausted for vehicle #%d", vehicle->id);
      return 0;
    }
    assign_wilderness_room(dest_room, new_x, new_y);
  }

  /* Get sector type at destination */
  sector = get_modified_sector_type(real_zone(WILD_ZONE_VNUM), new_x, new_y);

  /* Calculate speed modifier */
  speed_mod = get_vehicle_speed_modifier(vehicle, sector);

  /* Save previous state for transition */
  prev_state = vehicle->state;

  /* Update vehicle state to MOVING */
  vehicle->state = VSTATE_MOVING;

  /* Update coordinates */
  vehicle->x_coord = new_x;
  vehicle->y_coord = new_y;
  vehicle->location = dest_room;
  vehicle->direction = direction;

  /* Apply speed modifier */
  vehicle->current_speed = (vehicle->base_speed * speed_mod) / 100;
  if (vehicle->current_speed < 1)
  {
    vehicle->current_speed = 1;
  }

  /* Restore state (IDLE or LOADED based on cargo/passengers) */
  if (vehicle->current_passengers > 0 || vehicle->current_weight > 0)
  {
    vehicle->state = VSTATE_LOADED;
  }
  else
  {
    vehicle->state = VSTATE_IDLE;
  }

  /* Persist position to database */
  vehicle_save(vehicle);

  log("Info: Vehicle #%d moved to (%d, %d) room %d, speed=%d", vehicle->id, new_x, new_y, dest_room,
      vehicle->current_speed);

  return 1;
}

/**
 * Simple vehicle_move wrapper for API consistency.
 *
 * @param vehicle Pointer to the vehicle
 * @param direction The direction to move
 * @return 1 on success, 0 on failure
 */
int vehicle_move(struct vehicle_data *vehicle, int direction)
{
  return move_vehicle(vehicle, direction);
}

/* ========================================================================= */
/* END OF FILE                                                                */
/* ========================================================================= */
