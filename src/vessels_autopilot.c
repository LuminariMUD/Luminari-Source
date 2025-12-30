/* ************************************************************************
 *      File:   vessels_autopilot.c                     Part of LuminariMUD  *
 *   Purpose:   Autopilot system for vessel navigation                       *
 *              Phase 3 - Automation Layer                                   *
 *              Session 01: Data structures                                  *
 *              Session 02: Waypoint/Route database persistence              *
 *              Session 03: Path-following logic                             *
 * ********************************************************************** */

/* CRITICAL: math.h must be included before utils.h (included via vessels.h)
 * to avoid conflict between math.h's log() function and the log() macro
 * defined in utils.h. The sequence below ensures correct include order. */
#include "conf.h"
#include "sysdep.h"
#include <math.h>
#include "vessels.h"
#include "mysql.h"
#include "wilderness.h"

/* External MySQL connection variables */
extern MYSQL *conn;
extern bool mysql_available;

/* External time structure for schedule timing */
extern struct time_info_data time_info;

/* External ship data array from vessels.c */
extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];

/* ========================================================================= */
/* GLOBAL CACHE VARIABLES                                                    */
/* ========================================================================= */

/* In-memory cache list heads */
struct waypoint_node *waypoint_list = NULL;
struct route_node *route_list = NULL;

/* ========================================================================= */
/* AUTOPILOT LIFECYCLE FUNCTIONS                                            */
/* ========================================================================= */

/**
 * Initialize autopilot data for a ship.
 * Allocates and initializes the autopilot_data structure.
 *
 * @param ship The ship to initialize autopilot for
 * @return Pointer to the new autopilot_data, or NULL on failure
 */
struct autopilot_data *autopilot_init(struct greyhawk_ship_data *ship)
{
  struct autopilot_data *ap;

  if (ship == NULL)
  {
    log("SYSERR: autopilot_init called with NULL ship");
    return NULL;
  }

  /* TODO: Session 03 - Implement full initialization logic */
  CREATE(ap, struct autopilot_data, 1);
  if (ap == NULL)
  {
    log("SYSERR: Unable to allocate memory for autopilot_data");
    return NULL;
  }

  ap->state = AUTOPILOT_OFF;
  ap->current_route = NULL;
  ap->current_waypoint_index = 0;
  ap->tick_counter = 0;
  ap->wait_remaining = 0;
  ap->last_update = 0;
  ap->pilot_mob_vnum = -1;

  ship->autopilot = ap;
  return ap;
}

/**
 * Clean up and free autopilot data for a ship.
 *
 * @param ship The ship to clean up autopilot for
 */
void autopilot_cleanup(struct greyhawk_ship_data *ship)
{
  if (ship == NULL)
  {
    log("SYSERR: autopilot_cleanup called with NULL ship");
    return;
  }

  if (ship->autopilot != NULL)
  {
    /* TODO: Session 03 - Implement full cleanup logic */
    free(ship->autopilot);
    ship->autopilot = NULL;
  }
}

/**
 * Start autopilot navigation on a route.
 *
 * @param ship The ship to start autopilot for
 * @param route The route to follow
 * @return 1 on success, 0 on failure
 */
int autopilot_start(struct greyhawk_ship_data *ship, struct ship_route *route)
{
  if (ship == NULL || route == NULL)
  {
    log("SYSERR: autopilot_start called with NULL parameter");
    return 0;
  }

  if (ship->autopilot == NULL)
  {
    log("SYSERR: autopilot_start called on ship without autopilot data");
    return 0;
  }

  /* TODO: Session 03 - Implement start logic */
  ship->autopilot->current_route = route;
  ship->autopilot->current_waypoint_index = 0;
  ship->autopilot->state = AUTOPILOT_TRAVELING;
  ship->autopilot->last_update = time(0);

  return 1;
}

/**
 * Stop autopilot navigation completely.
 *
 * @param ship The ship to stop autopilot for
 * @return 1 on success, 0 on failure
 */
int autopilot_stop(struct greyhawk_ship_data *ship)
{
  if (ship == NULL)
  {
    log("SYSERR: autopilot_stop called with NULL ship");
    return 0;
  }

  if (ship->autopilot == NULL)
  {
    return 0;
  }

  /* TODO: Session 03 - Implement stop logic */
  ship->autopilot->state = AUTOPILOT_OFF;
  ship->autopilot->current_route = NULL;
  ship->autopilot->current_waypoint_index = 0;

  return 1;
}

/**
 * Pause autopilot navigation temporarily.
 *
 * @param ship The ship to pause autopilot for
 * @return 1 on success, 0 on failure
 */
int autopilot_pause(struct greyhawk_ship_data *ship)
{
  if (ship == NULL)
  {
    log("SYSERR: autopilot_pause called with NULL ship");
    return 0;
  }

  if (ship->autopilot == NULL)
  {
    return 0;
  }

  /* TODO: Session 03 - Implement pause logic */
  if (ship->autopilot->state == AUTOPILOT_TRAVELING || ship->autopilot->state == AUTOPILOT_WAITING)
  {
    ship->autopilot->state = AUTOPILOT_PAUSED;
    return 1;
  }

  return 0;
}

/**
 * Resume paused autopilot navigation.
 *
 * @param ship The ship to resume autopilot for
 * @return 1 on success, 0 on failure
 */
int autopilot_resume(struct greyhawk_ship_data *ship)
{
  if (ship == NULL)
  {
    log("SYSERR: autopilot_resume called with NULL ship");
    return 0;
  }

  if (ship->autopilot == NULL)
  {
    return 0;
  }

  /* TODO: Session 03 - Implement resume logic */
  if (ship->autopilot->state == AUTOPILOT_PAUSED)
  {
    ship->autopilot->state = AUTOPILOT_TRAVELING;
    return 1;
  }

  return 0;
}

/* ========================================================================= */
/* WAYPOINT MANAGEMENT FUNCTIONS                                            */
/* ========================================================================= */

/**
 * Add a waypoint to a route.
 *
 * @param route The route to add waypoint to
 * @param x X coordinate of waypoint
 * @param y Y coordinate of waypoint
 * @param z Z coordinate of waypoint
 * @param name Name of the waypoint
 * @return Index of new waypoint, or -1 on failure
 */
int waypoint_add(struct ship_route *route, float x, float y, float z, const char *name)
{
  int idx;

  if (route == NULL)
  {
    log("SYSERR: waypoint_add called with NULL route");
    return -1;
  }

  if (route->num_waypoints >= MAX_WAYPOINTS_PER_ROUTE)
  {
    log("SYSERR: waypoint_add - route is full");
    return -1;
  }

  /* TODO: Session 02 - Implement full waypoint add logic */
  idx = route->num_waypoints;
  route->waypoints[idx].x = x;
  route->waypoints[idx].y = y;
  route->waypoints[idx].z = z;
  route->waypoints[idx].tolerance = 5.0f;
  route->waypoints[idx].wait_time = 0;
  route->waypoints[idx].flags = 0;

  if (name != NULL)
  {
    strncpy(route->waypoints[idx].name, name, AUTOPILOT_NAME_LENGTH - 1);
    route->waypoints[idx].name[AUTOPILOT_NAME_LENGTH - 1] = '\0';
  }
  else
  {
    route->waypoints[idx].name[0] = '\0';
  }

  route->num_waypoints++;
  return idx;
}

/**
 * Remove a waypoint from a route by index.
 *
 * @param route The route to remove waypoint from
 * @param index Index of waypoint to remove
 * @return 1 on success, 0 on failure
 */
int waypoint_remove(struct ship_route *route, int index)
{
  int i;

  if (route == NULL)
  {
    log("SYSERR: waypoint_remove called with NULL route");
    return 0;
  }

  if (index < 0 || index >= route->num_waypoints)
  {
    log("SYSERR: waypoint_remove - invalid index %d", index);
    return 0;
  }

  /* TODO: Session 02 - Implement full waypoint remove logic */
  /* Shift remaining waypoints down */
  for (i = index; i < route->num_waypoints - 1; i++)
  {
    route->waypoints[i] = route->waypoints[i + 1];
  }

  route->num_waypoints--;
  return 1;
}

/**
 * Clear all waypoints from a route.
 *
 * @param route The route to clear
 */
void waypoint_clear_all(struct ship_route *route)
{
  if (route == NULL)
  {
    log("SYSERR: waypoint_clear_all called with NULL route");
    return;
  }

  /* TODO: Session 02 - Implement full clear logic */
  route->num_waypoints = 0;
}

/**
 * Get the current waypoint for a ship's autopilot.
 *
 * @param ship The ship to get current waypoint for
 * @return Pointer to current waypoint, or NULL if none
 */
struct waypoint *waypoint_get_current(struct greyhawk_ship_data *ship)
{
  if (ship == NULL || ship->autopilot == NULL)
  {
    return NULL;
  }

  if (ship->autopilot->current_route == NULL)
  {
    return NULL;
  }

  if (ship->autopilot->current_waypoint_index < 0 ||
      ship->autopilot->current_waypoint_index >= ship->autopilot->current_route->num_waypoints)
  {
    return NULL;
  }

  /* TODO: Session 03 - Implement full get current logic */
  return &ship->autopilot->current_route->waypoints[ship->autopilot->current_waypoint_index];
}

/**
 * Get the next waypoint for a ship's autopilot.
 *
 * @param ship The ship to get next waypoint for
 * @return Pointer to next waypoint, or NULL if none
 */
struct waypoint *waypoint_get_next(struct greyhawk_ship_data *ship)
{
  int next_idx;

  if (ship == NULL || ship->autopilot == NULL)
  {
    return NULL;
  }

  if (ship->autopilot->current_route == NULL)
  {
    return NULL;
  }

  next_idx = ship->autopilot->current_waypoint_index + 1;

  /* Handle route looping */
  if (next_idx >= ship->autopilot->current_route->num_waypoints)
  {
    if (ship->autopilot->current_route->loop)
    {
      next_idx = 0;
    }
    else
    {
      return NULL;
    }
  }

  /* TODO: Session 03 - Implement full get next logic */
  return &ship->autopilot->current_route->waypoints[next_idx];
}

/* ========================================================================= */
/* ROUTE MANAGEMENT FUNCTIONS                                               */
/* ========================================================================= */

/**
 * Create a new route with the given name.
 *
 * @param name Name for the new route
 * @return Pointer to new route, or NULL on failure
 */
struct ship_route *route_create(const char *name)
{
  struct ship_route *route;

  /* TODO: Session 02 - Implement full route creation logic */
  CREATE(route, struct ship_route, 1);
  if (route == NULL)
  {
    log("SYSERR: Unable to allocate memory for ship_route");
    return NULL;
  }

  route->route_id = 0; /* TODO: Generate unique ID */
  route->num_waypoints = 0;
  route->loop = FALSE;
  route->active = TRUE;

  if (name != NULL)
  {
    strncpy(route->name, name, AUTOPILOT_NAME_LENGTH - 1);
    route->name[AUTOPILOT_NAME_LENGTH - 1] = '\0';
  }
  else
  {
    route->name[0] = '\0';
  }

  return route;
}

/**
 * Destroy a route and free its memory.
 *
 * @param route The route to destroy
 */
void route_destroy(struct ship_route *route)
{
  if (route == NULL)
  {
    log("SYSERR: route_destroy called with NULL route");
    return;
  }

  /* TODO: Session 02 - Implement full route destruction logic */
  free(route);
}

/**
 * Load a route from the database.
 *
 * @param route The route structure to load into
 * @param route_id The ID of the route to load
 * @return 1 on success, 0 on failure
 */
int route_load(struct ship_route *route, int route_id)
{
  if (route == NULL)
  {
    log("SYSERR: route_load called with NULL route");
    return 0;
  }

  /* TODO: Session 02 - Implement database load logic */
  (void)route_id; /* Suppress unused parameter warning */
  return 0;
}

/**
 * Save a route to the database.
 *
 * @param route The route to save
 * @return 1 on success, 0 on failure
 */
int route_save(struct ship_route *route)
{
  if (route == NULL)
  {
    log("SYSERR: route_save called with NULL route");
    return 0;
  }

  /* TODO: Session 02 - Implement database save logic */
  return 0;
}

/**
 * Activate a route for use.
 *
 * @param route The route to activate
 * @return 1 on success, 0 on failure
 */
int route_activate(struct ship_route *route)
{
  if (route == NULL)
  {
    log("SYSERR: route_activate called with NULL route");
    return 0;
  }

  /* TODO: Session 02 - Implement full activation logic */
  route->active = TRUE;
  return 1;
}

/**
 * Deactivate a route.
 *
 * @param route The route to deactivate
 * @return 1 on success, 0 on failure
 */
int route_deactivate(struct ship_route *route)
{
  if (route == NULL)
  {
    log("SYSERR: route_deactivate called with NULL route");
    return 0;
  }

  /* TODO: Session 02 - Implement full deactivation logic */
  route->active = FALSE;
  return 1;
}

/* ========================================================================= */
/* CACHE MANAGEMENT FUNCTIONS                                                */
/* ========================================================================= */

/**
 * Clear all waypoints from the in-memory cache.
 * Frees all allocated memory for waypoint nodes.
 */
void waypoint_cache_clear(void)
{
  struct waypoint_node *current;
  struct waypoint_node *next;

  current = waypoint_list;
  while (current != NULL)
  {
    next = current->next;
    free(current);
    current = next;
  }

  waypoint_list = NULL;
  log("Info: Waypoint cache cleared");
}

/**
 * Clear all routes from the in-memory cache.
 * Frees all allocated memory for route nodes including waypoint_ids arrays.
 */
void route_cache_clear(void)
{
  struct route_node *current;
  struct route_node *next;

  current = route_list;
  while (current != NULL)
  {
    next = current->next;
    if (current->waypoint_ids != NULL)
    {
      free(current->waypoint_ids);
    }
    free(current);
    current = next;
  }

  route_list = NULL;
  log("Info: Route cache cleared");
}

/**
 * Find a waypoint in the cache by its database ID.
 *
 * @param waypoint_id The database ID to search for
 * @return Pointer to the waypoint_node if found, NULL otherwise
 */
struct waypoint_node *waypoint_cache_find(int waypoint_id)
{
  struct waypoint_node *current;

  for (current = waypoint_list; current != NULL; current = current->next)
  {
    if (current->waypoint_id == waypoint_id)
    {
      return current;
    }
  }

  return NULL;
}

/**
 * Find a route in the cache by its database ID.
 *
 * @param route_id The database ID to search for
 * @return Pointer to the route_node if found, NULL otherwise
 */
struct route_node *route_cache_find(int route_id)
{
  struct route_node *current;

  for (current = route_list; current != NULL; current = current->next)
  {
    if (current->route_id == route_id)
    {
      return current;
    }
  }

  return NULL;
}

/**
 * Add a waypoint node to the cache.
 * Inserts at the head of the linked list.
 *
 * @param node The waypoint_node to add
 */
static void waypoint_cache_add(struct waypoint_node *node)
{
  if (node == NULL)
  {
    return;
  }

  node->next = waypoint_list;
  waypoint_list = node;
}

/**
 * Add a route node to the cache.
 * Inserts at the head of the linked list.
 *
 * @param node The route_node to add
 */
static void route_cache_add(struct route_node *node)
{
  if (node == NULL)
  {
    return;
  }

  node->next = route_list;
  route_list = node;
}

/**
 * Remove a waypoint from the cache by ID.
 *
 * @param waypoint_id The ID of the waypoint to remove
 */
static void waypoint_cache_remove(int waypoint_id)
{
  struct waypoint_node *current;
  struct waypoint_node *prev;

  prev = NULL;
  current = waypoint_list;

  while (current != NULL)
  {
    if (current->waypoint_id == waypoint_id)
    {
      if (prev == NULL)
      {
        waypoint_list = current->next;
      }
      else
      {
        prev->next = current->next;
      }
      free(current);
      return;
    }
    prev = current;
    current = current->next;
  }
}

/**
 * Remove a route from the cache by ID.
 *
 * @param route_id The ID of the route to remove
 */
static void route_cache_remove(int route_id)
{
  struct route_node *current;
  struct route_node *prev;

  prev = NULL;
  current = route_list;

  while (current != NULL)
  {
    if (current->route_id == route_id)
    {
      if (prev == NULL)
      {
        route_list = current->next;
      }
      else
      {
        prev->next = current->next;
      }
      if (current->waypoint_ids != NULL)
      {
        free(current->waypoint_ids);
      }
      free(current);
      return;
    }
    prev = current;
    current = current->next;
  }
}

/* ========================================================================= */
/* WAYPOINT DATABASE CRUD FUNCTIONS                                          */
/* ========================================================================= */

/**
 * Create a new waypoint in the database.
 *
 * @param wp The waypoint data to insert
 * @return The new waypoint_id on success, -1 on failure
 */
int waypoint_db_create(const struct waypoint *wp)
{
  char query[MAX_STRING_LENGTH];
  char escaped_name[AUTOPILOT_NAME_LENGTH * 2 + 1];
  struct waypoint_node *node;
  int new_id;

  if (wp == NULL)
  {
    log("SYSERR: waypoint_db_create called with NULL waypoint");
    return -1;
  }

  if (!mysql_available || conn == NULL)
  {
    log("SYSERR: waypoint_db_create - MySQL not available");
    return -1;
  }

  /* Escape the name string */
  mysql_real_escape_string(conn, escaped_name, wp->name, strlen(wp->name));

  /* Build INSERT query */
  snprintf(query, sizeof(query),
           "INSERT INTO ship_waypoints (name, x, y, z, tolerance, wait_time, flags) "
           "VALUES ('%s', %.2f, %.2f, %.2f, %.2f, %d, %d)",
           escaped_name, wp->x, wp->y, wp->z, wp->tolerance, wp->wait_time, wp->flags);

  if (mysql_query(conn, query))
  {
    log("SYSERR: waypoint_db_create failed: %s", mysql_error(conn));
    return -1;
  }

  /* Get the auto-generated ID */
  new_id = (int)mysql_insert_id(conn);

  /* Add to cache */
  CREATE(node, struct waypoint_node, 1);
  if (node != NULL)
  {
    node->waypoint_id = new_id;
    node->data = *wp;
    waypoint_cache_add(node);
  }

  log("Info: Created waypoint %d '%s' at (%.2f, %.2f, %.2f)", new_id, wp->name, wp->x, wp->y,
      wp->z);

  return new_id;
}

/**
 * Load a waypoint from the database by ID.
 *
 * @param waypoint_id The ID of the waypoint to load
 * @return Pointer to a new waypoint_node on success, NULL on failure
 */
struct waypoint_node *waypoint_db_load(int waypoint_id)
{
  char query[MAX_STRING_LENGTH];
  MYSQL_RES *result;
  MYSQL_ROW row;
  struct waypoint_node *node;

  /* Check cache first */
  node = waypoint_cache_find(waypoint_id);
  if (node != NULL)
  {
    return node;
  }

  if (!mysql_available || conn == NULL)
  {
    log("SYSERR: waypoint_db_load - MySQL not available");
    return NULL;
  }

  snprintf(query, sizeof(query),
           "SELECT waypoint_id, name, x, y, z, tolerance, wait_time, flags "
           "FROM ship_waypoints WHERE waypoint_id = %d",
           waypoint_id);

  if (mysql_query(conn, query))
  {
    log("SYSERR: waypoint_db_load failed: %s", mysql_error(conn));
    return NULL;
  }

  result = mysql_store_result(conn);
  if (result == NULL)
  {
    log("SYSERR: waypoint_db_load - failed to store result: %s", mysql_error(conn));
    return NULL;
  }

  row = mysql_fetch_row(result);
  if (row == NULL)
  {
    mysql_free_result(result);
    return NULL;
  }

  /* Create node and populate */
  CREATE(node, struct waypoint_node, 1);
  if (node == NULL)
  {
    log("SYSERR: waypoint_db_load - failed to allocate memory");
    mysql_free_result(result);
    return NULL;
  }

  node->waypoint_id = atoi(row[0]);
  if (row[1] != NULL)
  {
    strncpy(node->data.name, row[1], AUTOPILOT_NAME_LENGTH - 1);
    node->data.name[AUTOPILOT_NAME_LENGTH - 1] = '\0';
  }
  else
  {
    node->data.name[0] = '\0';
  }
  node->data.x = (float)atof(row[2]);
  node->data.y = (float)atof(row[3]);
  node->data.z = (float)atof(row[4]);
  node->data.tolerance = (float)atof(row[5]);
  node->data.wait_time = atoi(row[6]);
  node->data.flags = atoi(row[7]);
  node->next = NULL;

  mysql_free_result(result);

  /* Add to cache */
  waypoint_cache_add(node);

  return node;
}

/**
 * Update an existing waypoint in the database.
 *
 * @param waypoint_id The ID of the waypoint to update
 * @param wp The new waypoint data
 * @return 1 on success, 0 on failure
 */
int waypoint_db_update(int waypoint_id, const struct waypoint *wp)
{
  char query[MAX_STRING_LENGTH];
  char escaped_name[AUTOPILOT_NAME_LENGTH * 2 + 1];
  struct waypoint_node *node;

  if (wp == NULL)
  {
    log("SYSERR: waypoint_db_update called with NULL waypoint");
    return 0;
  }

  if (!mysql_available || conn == NULL)
  {
    log("SYSERR: waypoint_db_update - MySQL not available");
    return 0;
  }

  /* Escape the name string */
  mysql_real_escape_string(conn, escaped_name, wp->name, strlen(wp->name));

  /* Build UPDATE query */
  snprintf(query, sizeof(query),
           "UPDATE ship_waypoints SET name='%s', x=%.2f, y=%.2f, z=%.2f, "
           "tolerance=%.2f, wait_time=%d, flags=%d WHERE waypoint_id=%d",
           escaped_name, wp->x, wp->y, wp->z, wp->tolerance, wp->wait_time, wp->flags, waypoint_id);

  if (mysql_query(conn, query))
  {
    log("SYSERR: waypoint_db_update failed: %s", mysql_error(conn));
    return 0;
  }

  /* Update cache */
  node = waypoint_cache_find(waypoint_id);
  if (node != NULL)
  {
    node->data = *wp;
  }

  log("Info: Updated waypoint %d", waypoint_id);

  return 1;
}

/**
 * Delete a waypoint from the database.
 *
 * @param waypoint_id The ID of the waypoint to delete
 * @return 1 on success, 0 on failure
 */
int waypoint_db_delete(int waypoint_id)
{
  char query[MAX_STRING_LENGTH];

  if (!mysql_available || conn == NULL)
  {
    log("SYSERR: waypoint_db_delete - MySQL not available");
    return 0;
  }

  snprintf(query, sizeof(query), "DELETE FROM ship_waypoints WHERE waypoint_id = %d", waypoint_id);

  if (mysql_query(conn, query))
  {
    log("SYSERR: waypoint_db_delete failed: %s", mysql_error(conn));
    return 0;
  }

  /* Remove from cache */
  waypoint_cache_remove(waypoint_id);

  log("Info: Deleted waypoint %d", waypoint_id);

  return 1;
}

/* ========================================================================= */
/* ROUTE DATABASE CRUD FUNCTIONS                                             */
/* ========================================================================= */

/**
 * Create a new route in the database.
 *
 * @param name The name of the route
 * @param loop_route Whether the route should loop
 * @return The new route_id on success, -1 on failure
 */
int route_db_create(const char *name, bool loop_route)
{
  char query[MAX_STRING_LENGTH];
  char escaped_name[AUTOPILOT_NAME_LENGTH * 2 + 1];
  struct route_node *node;
  int new_id;

  if (name == NULL)
  {
    log("SYSERR: route_db_create called with NULL name");
    return -1;
  }

  if (!mysql_available || conn == NULL)
  {
    log("SYSERR: route_db_create - MySQL not available");
    return -1;
  }

  /* Escape the name string */
  mysql_real_escape_string(conn, escaped_name, name, strlen(name));

  /* Build INSERT query */
  snprintf(query, sizeof(query),
           "INSERT INTO ship_routes (name, loop_route, active) "
           "VALUES ('%s', %d, 1)",
           escaped_name, loop_route ? 1 : 0);

  if (mysql_query(conn, query))
  {
    log("SYSERR: route_db_create failed: %s", mysql_error(conn));
    return -1;
  }

  /* Get the auto-generated ID */
  new_id = (int)mysql_insert_id(conn);

  /* Add to cache */
  CREATE(node, struct route_node, 1);
  if (node != NULL)
  {
    node->route_id = new_id;
    strncpy(node->name, name, AUTOPILOT_NAME_LENGTH - 1);
    node->name[AUTOPILOT_NAME_LENGTH - 1] = '\0';
    node->loop = loop_route;
    node->active = TRUE;
    node->num_waypoints = 0;
    node->waypoint_ids = NULL;
    route_cache_add(node);
  }

  log("Info: Created route %d '%s' (loop=%d)", new_id, name, loop_route);

  return new_id;
}

/**
 * Load a route from the database by ID.
 *
 * @param route_id The ID of the route to load
 * @return Pointer to a new route_node on success, NULL on failure
 */
struct route_node *route_db_load(int route_id)
{
  char query[MAX_STRING_LENGTH];
  MYSQL_RES *result;
  MYSQL_ROW row;
  struct route_node *node;

  /* Check cache first */
  node = route_cache_find(route_id);
  if (node != NULL)
  {
    return node;
  }

  if (!mysql_available || conn == NULL)
  {
    log("SYSERR: route_db_load - MySQL not available");
    return NULL;
  }

  snprintf(query, sizeof(query),
           "SELECT route_id, name, loop_route, active "
           "FROM ship_routes WHERE route_id = %d",
           route_id);

  if (mysql_query(conn, query))
  {
    log("SYSERR: route_db_load failed: %s", mysql_error(conn));
    return NULL;
  }

  result = mysql_store_result(conn);
  if (result == NULL)
  {
    log("SYSERR: route_db_load - failed to store result: %s", mysql_error(conn));
    return NULL;
  }

  row = mysql_fetch_row(result);
  if (row == NULL)
  {
    mysql_free_result(result);
    return NULL;
  }

  /* Create node and populate */
  CREATE(node, struct route_node, 1);
  if (node == NULL)
  {
    log("SYSERR: route_db_load - failed to allocate memory");
    mysql_free_result(result);
    return NULL;
  }

  node->route_id = atoi(row[0]);
  if (row[1] != NULL)
  {
    strncpy(node->name, row[1], AUTOPILOT_NAME_LENGTH - 1);
    node->name[AUTOPILOT_NAME_LENGTH - 1] = '\0';
  }
  else
  {
    node->name[0] = '\0';
  }
  node->loop = atoi(row[2]) ? TRUE : FALSE;
  node->active = atoi(row[3]) ? TRUE : FALSE;
  node->num_waypoints = 0;
  node->waypoint_ids = NULL;
  node->next = NULL;

  mysql_free_result(result);

  /* Load waypoint associations */
  if (route_get_waypoint_ids(route_id, &node->waypoint_ids, &node->num_waypoints) != 1)
  {
    node->num_waypoints = 0;
    node->waypoint_ids = NULL;
  }

  /* Add to cache */
  route_cache_add(node);

  return node;
}

/**
 * Update an existing route in the database.
 *
 * @param route_id The ID of the route to update
 * @param name The new name (or NULL to keep existing)
 * @param loop_route Whether the route should loop
 * @param active Whether the route is active
 * @return 1 on success, 0 on failure
 */
int route_db_update(int route_id, const char *name, bool loop_route, bool active)
{
  char query[MAX_STRING_LENGTH];
  char escaped_name[AUTOPILOT_NAME_LENGTH * 2 + 1];
  struct route_node *node;

  if (!mysql_available || conn == NULL)
  {
    log("SYSERR: route_db_update - MySQL not available");
    return 0;
  }

  if (name != NULL)
  {
    /* Escape the name string */
    mysql_real_escape_string(conn, escaped_name, name, strlen(name));

    /* Build UPDATE query with name */
    snprintf(query, sizeof(query),
             "UPDATE ship_routes SET name='%s', loop_route=%d, active=%d "
             "WHERE route_id=%d",
             escaped_name, loop_route ? 1 : 0, active ? 1 : 0, route_id);
  }
  else
  {
    /* Build UPDATE query without name */
    snprintf(query, sizeof(query),
             "UPDATE ship_routes SET loop_route=%d, active=%d "
             "WHERE route_id=%d",
             loop_route ? 1 : 0, active ? 1 : 0, route_id);
  }

  if (mysql_query(conn, query))
  {
    log("SYSERR: route_db_update failed: %s", mysql_error(conn));
    return 0;
  }

  /* Update cache */
  node = route_cache_find(route_id);
  if (node != NULL)
  {
    if (name != NULL)
    {
      strncpy(node->name, name, AUTOPILOT_NAME_LENGTH - 1);
      node->name[AUTOPILOT_NAME_LENGTH - 1] = '\0';
    }
    node->loop = loop_route;
    node->active = active;
  }

  log("Info: Updated route %d", route_id);

  return 1;
}

/**
 * Delete a route from the database.
 * Also removes all route-waypoint associations (via CASCADE).
 *
 * @param route_id The ID of the route to delete
 * @return 1 on success, 0 on failure
 */
int route_db_delete(int route_id)
{
  char query[MAX_STRING_LENGTH];

  if (!mysql_available || conn == NULL)
  {
    log("SYSERR: route_db_delete - MySQL not available");
    return 0;
  }

  snprintf(query, sizeof(query), "DELETE FROM ship_routes WHERE route_id = %d", route_id);

  if (mysql_query(conn, query))
  {
    log("SYSERR: route_db_delete failed: %s", mysql_error(conn));
    return 0;
  }

  /* Remove from cache */
  route_cache_remove(route_id);

  log("Info: Deleted route %d", route_id);

  return 1;
}

/* ========================================================================= */
/* ROUTE-WAYPOINT ASSOCIATION FUNCTIONS                                      */
/* ========================================================================= */

/**
 * Add a waypoint to a route at a specific sequence position.
 *
 * @param route_id The route to add the waypoint to
 * @param waypoint_id The waypoint to add
 * @param sequence_num The position in the route (0-based)
 * @return 1 on success, 0 on failure
 */
int route_add_waypoint_db(int route_id, int waypoint_id, int sequence_num)
{
  char query[MAX_STRING_LENGTH];
  struct route_node *node;
  int *new_ids;
  int i;

  if (!mysql_available || conn == NULL)
  {
    log("SYSERR: route_add_waypoint_db - MySQL not available");
    return 0;
  }

  snprintf(query, sizeof(query),
           "INSERT INTO ship_route_waypoints (route_id, waypoint_id, sequence_num) "
           "VALUES (%d, %d, %d)",
           route_id, waypoint_id, sequence_num);

  if (mysql_query(conn, query))
  {
    log("SYSERR: route_add_waypoint_db failed: %s", mysql_error(conn));
    return 0;
  }

  /* Update cache */
  node = route_cache_find(route_id);
  if (node != NULL)
  {
    /* Resize waypoint_ids array */
    CREATE(new_ids, int, node->num_waypoints + 1);
    if (new_ids != NULL)
    {
      /* Copy existing IDs up to insertion point */
      for (i = 0; i < sequence_num && i < node->num_waypoints; i++)
      {
        new_ids[i] = node->waypoint_ids[i];
      }
      /* Insert new ID */
      new_ids[sequence_num] = waypoint_id;
      /* Copy remaining IDs */
      for (i = sequence_num; i < node->num_waypoints; i++)
      {
        new_ids[i + 1] = node->waypoint_ids[i];
      }
      /* Replace old array */
      if (node->waypoint_ids != NULL)
      {
        free(node->waypoint_ids);
      }
      node->waypoint_ids = new_ids;
      node->num_waypoints++;
    }
  }

  log("Info: Added waypoint %d to route %d at position %d", waypoint_id, route_id, sequence_num);

  return 1;
}

/**
 * Remove a waypoint from a route.
 * Automatically resequences remaining waypoints.
 *
 * @param route_id The route to remove the waypoint from
 * @param waypoint_id The waypoint to remove
 * @return 1 on success, 0 on failure
 */
int route_remove_waypoint_db(int route_id, int waypoint_id)
{
  char query[MAX_STRING_LENGTH];
  struct route_node *node;
  int i;
  int j;
  int found_idx;

  if (!mysql_available || conn == NULL)
  {
    log("SYSERR: route_remove_waypoint_db - MySQL not available");
    return 0;
  }

  /* Get the sequence number of the waypoint being removed */
  snprintf(query, sizeof(query),
           "SELECT sequence_num FROM ship_route_waypoints "
           "WHERE route_id = %d AND waypoint_id = %d",
           route_id, waypoint_id);

  if (mysql_query(conn, query))
  {
    log("SYSERR: route_remove_waypoint_db - failed to get sequence: %s", mysql_error(conn));
    return 0;
  }

  /* Delete the waypoint association */
  snprintf(query, sizeof(query),
           "DELETE FROM ship_route_waypoints "
           "WHERE route_id = %d AND waypoint_id = %d",
           route_id, waypoint_id);

  if (mysql_query(conn, query))
  {
    log("SYSERR: route_remove_waypoint_db failed: %s", mysql_error(conn));
    return 0;
  }

  /* Resequence remaining waypoints */
  snprintf(query, sizeof(query),
           "SET @seq := -1; "
           "UPDATE ship_route_waypoints "
           "SET sequence_num = (@seq := @seq + 1) "
           "WHERE route_id = %d "
           "ORDER BY sequence_num",
           route_id);

  /* Note: This is two statements; execute separately */
  if (mysql_query(conn, "SET @seq := -1"))
  {
    log("SYSERR: route_remove_waypoint_db - failed to set variable: %s", mysql_error(conn));
  }

  snprintf(query, sizeof(query),
           "UPDATE ship_route_waypoints "
           "SET sequence_num = (@seq := @seq + 1) "
           "WHERE route_id = %d "
           "ORDER BY sequence_num",
           route_id);

  if (mysql_query(conn, query))
  {
    log("SYSERR: route_remove_waypoint_db - failed to resequence: %s", mysql_error(conn));
  }

  /* Update cache */
  node = route_cache_find(route_id);
  if (node != NULL && node->waypoint_ids != NULL)
  {
    /* Find and remove the waypoint from the array */
    found_idx = -1;
    for (i = 0; i < node->num_waypoints; i++)
    {
      if (node->waypoint_ids[i] == waypoint_id)
      {
        found_idx = i;
        break;
      }
    }

    if (found_idx >= 0)
    {
      /* Shift remaining IDs down */
      for (j = found_idx; j < node->num_waypoints - 1; j++)
      {
        node->waypoint_ids[j] = node->waypoint_ids[j + 1];
      }
      node->num_waypoints--;
    }
  }

  log("Info: Removed waypoint %d from route %d", waypoint_id, route_id);

  return 1;
}

/**
 * Reorder all waypoints in a route.
 *
 * @param route_id The route to reorder
 * @param waypoint_ids Array of waypoint IDs in the new order
 * @param count Number of waypoints in the array
 * @return 1 on success, 0 on failure
 */
int route_reorder_waypoints_db(int route_id, int *waypoint_ids, int count)
{
  char query[MAX_STRING_LENGTH];
  struct route_node *node;
  int *new_ids;
  int i;

  if (waypoint_ids == NULL || count <= 0)
  {
    log("SYSERR: route_reorder_waypoints_db - invalid parameters");
    return 0;
  }

  if (!mysql_available || conn == NULL)
  {
    log("SYSERR: route_reorder_waypoints_db - MySQL not available");
    return 0;
  }

  /* Delete all existing associations */
  snprintf(query, sizeof(query), "DELETE FROM ship_route_waypoints WHERE route_id = %d", route_id);

  if (mysql_query(conn, query))
  {
    log("SYSERR: route_reorder_waypoints_db - failed to delete: %s", mysql_error(conn));
    return 0;
  }

  /* Re-insert in new order */
  for (i = 0; i < count; i++)
  {
    snprintf(query, sizeof(query),
             "INSERT INTO ship_route_waypoints (route_id, waypoint_id, sequence_num) "
             "VALUES (%d, %d, %d)",
             route_id, waypoint_ids[i], i);

    if (mysql_query(conn, query))
    {
      log("SYSERR: route_reorder_waypoints_db - failed to insert: %s", mysql_error(conn));
      return 0;
    }
  }

  /* Update cache */
  node = route_cache_find(route_id);
  if (node != NULL)
  {
    /* Replace waypoint_ids array */
    CREATE(new_ids, int, count);
    if (new_ids != NULL)
    {
      for (i = 0; i < count; i++)
      {
        new_ids[i] = waypoint_ids[i];
      }
      if (node->waypoint_ids != NULL)
      {
        free(node->waypoint_ids);
      }
      node->waypoint_ids = new_ids;
      node->num_waypoints = count;
    }
  }

  log("Info: Reordered %d waypoints for route %d", count, route_id);

  return 1;
}

/**
 * Get the ordered list of waypoint IDs for a route.
 *
 * @param route_id The route to get waypoints for
 * @param waypoint_ids Output pointer to array of waypoint IDs (caller must free)
 * @param count Output pointer to count of waypoints
 * @return 1 on success, 0 on failure
 */
int route_get_waypoint_ids(int route_id, int **waypoint_ids, int *count)
{
  char query[MAX_STRING_LENGTH];
  MYSQL_RES *result;
  MYSQL_ROW row;
  int num_rows;
  int *ids;
  int i;

  if (waypoint_ids == NULL || count == NULL)
  {
    log("SYSERR: route_get_waypoint_ids - invalid parameters");
    return 0;
  }

  *waypoint_ids = NULL;
  *count = 0;

  if (!mysql_available || conn == NULL)
  {
    log("SYSERR: route_get_waypoint_ids - MySQL not available");
    return 0;
  }

  snprintf(query, sizeof(query),
           "SELECT waypoint_id FROM ship_route_waypoints "
           "WHERE route_id = %d ORDER BY sequence_num",
           route_id);

  if (mysql_query(conn, query))
  {
    log("SYSERR: route_get_waypoint_ids failed: %s", mysql_error(conn));
    return 0;
  }

  result = mysql_store_result(conn);
  if (result == NULL)
  {
    log("SYSERR: route_get_waypoint_ids - failed to store result: %s", mysql_error(conn));
    return 0;
  }

  num_rows = (int)mysql_num_rows(result);
  if (num_rows == 0)
  {
    mysql_free_result(result);
    return 1; /* Success with empty result */
  }

  /* Allocate array */
  CREATE(ids, int, num_rows);
  if (ids == NULL)
  {
    log("SYSERR: route_get_waypoint_ids - failed to allocate memory");
    mysql_free_result(result);
    return 0;
  }

  /* Populate array */
  i = 0;
  while ((row = mysql_fetch_row(result)) && i < num_rows)
  {
    ids[i++] = atoi(row[0]);
  }

  mysql_free_result(result);

  *waypoint_ids = ids;
  *count = num_rows;

  return 1;
}

/* ========================================================================= */
/* BOOT-TIME LOADING FUNCTIONS                                               */
/* ========================================================================= */

/**
 * Load all waypoints from the database into the cache.
 * Called during server boot.
 */
void load_all_waypoints(void)
{
  char query[MAX_STRING_LENGTH];
  MYSQL_RES *result;
  MYSQL_ROW row;
  struct waypoint_node *node;
  int count;

  if (!mysql_available || conn == NULL)
  {
    log("Info: MySQL not available, skipping waypoint loading");
    return;
  }

  /* Clear existing cache */
  waypoint_cache_clear();

  snprintf(query, sizeof(query),
           "SELECT waypoint_id, name, x, y, z, tolerance, wait_time, flags "
           "FROM ship_waypoints ORDER BY waypoint_id");

  if (mysql_query(conn, query))
  {
    log("SYSERR: load_all_waypoints failed: %s", mysql_error(conn));
    return;
  }

  result = mysql_store_result(conn);
  if (result == NULL)
  {
    log("SYSERR: load_all_waypoints - failed to store result: %s", mysql_error(conn));
    return;
  }

  count = 0;
  while ((row = mysql_fetch_row(result)))
  {
    CREATE(node, struct waypoint_node, 1);
    if (node == NULL)
    {
      log("SYSERR: load_all_waypoints - failed to allocate memory");
      continue;
    }

    node->waypoint_id = atoi(row[0]);
    if (row[1] != NULL)
    {
      strncpy(node->data.name, row[1], AUTOPILOT_NAME_LENGTH - 1);
      node->data.name[AUTOPILOT_NAME_LENGTH - 1] = '\0';
    }
    else
    {
      node->data.name[0] = '\0';
    }
    node->data.x = (float)atof(row[2]);
    node->data.y = (float)atof(row[3]);
    node->data.z = (float)atof(row[4]);
    node->data.tolerance = (float)atof(row[5]);
    node->data.wait_time = atoi(row[6]);
    node->data.flags = atoi(row[7]);
    node->next = NULL;

    waypoint_cache_add(node);
    count++;
  }

  mysql_free_result(result);

  log("Info: Loaded %d waypoints from database", count);
}

/**
 * Load all routes from the database into the cache.
 * Called during server boot.
 */
void load_all_routes(void)
{
  char query[MAX_STRING_LENGTH];
  MYSQL_RES *result;
  MYSQL_ROW row;
  struct route_node *node;
  int count;

  if (!mysql_available || conn == NULL)
  {
    log("Info: MySQL not available, skipping route loading");
    return;
  }

  /* Clear existing cache */
  route_cache_clear();

  snprintf(query, sizeof(query),
           "SELECT route_id, name, loop_route, active "
           "FROM ship_routes ORDER BY route_id");

  if (mysql_query(conn, query))
  {
    log("SYSERR: load_all_routes failed: %s", mysql_error(conn));
    return;
  }

  result = mysql_store_result(conn);
  if (result == NULL)
  {
    log("SYSERR: load_all_routes - failed to store result: %s", mysql_error(conn));
    return;
  }

  count = 0;
  while ((row = mysql_fetch_row(result)))
  {
    CREATE(node, struct route_node, 1);
    if (node == NULL)
    {
      log("SYSERR: load_all_routes - failed to allocate memory");
      continue;
    }

    node->route_id = atoi(row[0]);
    if (row[1] != NULL)
    {
      strncpy(node->name, row[1], AUTOPILOT_NAME_LENGTH - 1);
      node->name[AUTOPILOT_NAME_LENGTH - 1] = '\0';
    }
    else
    {
      node->name[0] = '\0';
    }
    node->loop = atoi(row[2]) ? TRUE : FALSE;
    node->active = atoi(row[3]) ? TRUE : FALSE;
    node->num_waypoints = 0;
    node->waypoint_ids = NULL;
    node->next = NULL;

    /* Load waypoint associations */
    if (route_get_waypoint_ids(node->route_id, &node->waypoint_ids, &node->num_waypoints) != 1)
    {
      node->num_waypoints = 0;
      node->waypoint_ids = NULL;
    }

    route_cache_add(node);
    count++;
  }

  mysql_free_result(result);

  log("Info: Loaded %d routes from database", count);
}

/* ========================================================================= */
/* SHUTDOWN SAVING FUNCTIONS                                                 */
/* ========================================================================= */

/**
 * Save all cached waypoints to the database.
 * Called during server shutdown.
 * Note: Since we update the database on each change, this is mainly
 * for verification and logging.
 */
void save_all_waypoints(void)
{
  struct waypoint_node *current;
  int count;

  if (!mysql_available || conn == NULL)
  {
    log("Info: MySQL not available, skipping waypoint saving");
    return;
  }

  count = 0;
  for (current = waypoint_list; current != NULL; current = current->next)
  {
    /* Waypoints are saved on each update, so just count */
    count++;
  }

  log("Info: Verified %d waypoints in cache at shutdown", count);
}

/**
 * Save all cached routes to the database.
 * Called during server shutdown.
 * Note: Since we update the database on each change, this is mainly
 * for verification and logging.
 */
void save_all_routes(void)
{
  struct route_node *current;
  int count;

  if (!mysql_available || conn == NULL)
  {
    log("Info: MySQL not available, skipping route saving");
    return;
  }

  count = 0;
  for (current = route_list; current != NULL; current = current->next)
  {
    /* Routes are saved on each update, so just count */
    count++;
  }

  log("Info: Verified %d routes in cache at shutdown", count);
}

/* ========================================================================= */
/* PATH-FOLLOWING FUNCTIONS (Session 03)                                     */
/* ========================================================================= */

/**
 * Calculate the Euclidean distance from the ship to a waypoint.
 *
 * Uses 2D distance formula: sqrt((x2-x1)^2 + (y2-y1)^2)
 * Z coordinate is included for 3D calculations when applicable.
 *
 * @param ship The ship to calculate distance from
 * @param wp The waypoint to calculate distance to
 * @return The distance in coordinate units, or -1.0 on error
 */
float calculate_distance_to_waypoint(struct greyhawk_ship_data *ship, struct waypoint *wp)
{
  float dx, dy, dz;
  float distance;

  if (ship == NULL)
  {
    log("SYSERR: calculate_distance_to_waypoint called with NULL ship");
    return -1.0f;
  }

  if (wp == NULL)
  {
    log("SYSERR: calculate_distance_to_waypoint called with NULL waypoint");
    return -1.0f;
  }

  dx = wp->x - ship->x;
  dy = wp->y - ship->y;
  dz = wp->z - ship->z;

  /* Calculate 3D Euclidean distance */
  distance = (float)sqrt((double)(dx * dx + dy * dy + dz * dz));

  return distance;
}

/**
 * Calculate the heading direction vector from ship to waypoint.
 *
 * Returns a normalized direction vector (dx, dy) that points from
 * the ship's current position toward the waypoint. The vector length
 * represents one unit of travel distance.
 *
 * @param ship The ship to calculate heading for
 * @param wp The waypoint to head toward
 * @param dx Output: normalized X direction component
 * @param dy Output: normalized Y direction component
 */
void calculate_heading_to_waypoint(struct greyhawk_ship_data *ship, struct waypoint *wp, float *dx,
                                   float *dy)
{
  float raw_dx, raw_dy;
  float distance;

  if (ship == NULL || wp == NULL || dx == NULL || dy == NULL)
  {
    log("SYSERR: calculate_heading_to_waypoint called with NULL parameter");
    if (dx != NULL)
      *dx = 0.0f;
    if (dy != NULL)
      *dy = 0.0f;
    return;
  }

  raw_dx = wp->x - ship->x;
  raw_dy = wp->y - ship->y;

  /* Calculate 2D distance for normalization */
  distance = (float)sqrt((double)(raw_dx * raw_dx + raw_dy * raw_dy));

  if (distance < 0.001f)
  {
    /* Already at waypoint or very close */
    *dx = 0.0f;
    *dy = 0.0f;
    return;
  }

  /* Normalize to unit vector */
  *dx = raw_dx / distance;
  *dy = raw_dy / distance;
}

/**
 * Check if a ship has arrived at a waypoint within tolerance.
 *
 * @param ship The ship to check
 * @param wp The waypoint to check arrival at
 * @return TRUE if within tolerance, FALSE otherwise
 */
int check_waypoint_arrival(struct greyhawk_ship_data *ship, struct waypoint *wp)
{
  float distance;
  float tolerance;

  if (ship == NULL || wp == NULL)
  {
    log("SYSERR: check_waypoint_arrival called with NULL parameter");
    return FALSE;
  }

  distance = calculate_distance_to_waypoint(ship, wp);
  if (distance < 0.0f)
  {
    /* Error in distance calculation */
    return FALSE;
  }

  /* Use waypoint tolerance, default to 5.0 if not set */
  tolerance = wp->tolerance;
  if (tolerance <= 0.0f)
  {
    tolerance = 5.0f;
  }

  return (distance <= tolerance) ? TRUE : FALSE;
}

/**
 * Advance the ship to the next waypoint in the route.
 *
 * Handles route progression logic including:
 * - Advancing to next waypoint in sequence
 * - Loop routes: restart from beginning
 * - Non-loop routes: set COMPLETE state
 *
 * @param ship The ship to advance
 * @return 1 if advanced to next waypoint, 0 if route complete or error
 */
int advance_to_next_waypoint(struct greyhawk_ship_data *ship)
{
  struct autopilot_data *ap;
  struct ship_route *route;
  int next_index;

  if (ship == NULL)
  {
    log("SYSERR: advance_to_next_waypoint called with NULL ship");
    return 0;
  }

  ap = ship->autopilot;
  if (ap == NULL)
  {
    log("SYSERR: advance_to_next_waypoint called on ship without autopilot");
    return 0;
  }

  route = ap->current_route;
  if (route == NULL)
  {
    log("SYSERR: advance_to_next_waypoint called with NULL route");
    ap->state = AUTOPILOT_OFF;
    return 0;
  }

  /* Calculate next waypoint index */
  next_index = ap->current_waypoint_index + 1;

  if (next_index >= route->num_waypoints)
  {
    /* Reached end of route */
    if (route->loop)
    {
      /* Loop route: restart from beginning */
      ap->current_waypoint_index = 0;
      ap->state = AUTOPILOT_TRAVELING;
      log("Info: Ship %d route '%s' looping to waypoint 0", ship->shipnum, route->name);
      return 1;
    }
    else
    {
      /* Non-loop route: mark complete */
      ap->state = AUTOPILOT_COMPLETE;
      log("Info: Ship %d completed route '%s'", ship->shipnum, route->name);
      return 0;
    }
  }

  /* Advance to next waypoint */
  ap->current_waypoint_index = next_index;
  ap->state = AUTOPILOT_TRAVELING;

  return 1;
}

/**
 * Handle arrival at a waypoint.
 *
 * When a ship arrives at a waypoint, this function:
 * - Sets state to WAITING if wait_time > 0
 * - Otherwise advances to next waypoint
 *
 * @param ship The ship that has arrived
 */
void handle_waypoint_arrival(struct greyhawk_ship_data *ship)
{
  struct autopilot_data *ap;
  struct waypoint *wp;

  if (ship == NULL)
  {
    log("SYSERR: handle_waypoint_arrival called with NULL ship");
    return;
  }

  ap = ship->autopilot;
  if (ap == NULL)
  {
    return;
  }

  wp = waypoint_get_current(ship);
  if (wp == NULL)
  {
    log("SYSERR: handle_waypoint_arrival - no current waypoint");
    ap->state = AUTOPILOT_OFF;
    return;
  }

  /* Pilot announcement if pilot is assigned */
  if (ap->pilot_mob_vnum != -1)
  {
    pilot_announce_waypoint(ship, wp);
  }

  /* Check if we need to wait at this waypoint */
  if (wp->wait_time > 0)
  {
    ap->state = AUTOPILOT_WAITING;
    ap->wait_remaining = wp->wait_time;
    ap->last_update = time(0);
    log("Info: Ship %d arrived at waypoint '%s', waiting %d seconds", ship->shipnum, wp->name,
        wp->wait_time);
  }
  else
  {
    /* No wait time, advance immediately */
    log("Info: Ship %d arrived at waypoint '%s', advancing", ship->shipnum, wp->name);
    advance_to_next_waypoint(ship);
  }
}

/**
 * Move a vessel toward its current waypoint.
 *
 * Calculates movement direction and distance, validates terrain,
 * and updates ship position. Uses the wilderness room allocation
 * pattern for terrain validation.
 *
 * @param ship The ship to move
 * @return 1 if moved successfully, 0 if movement failed
 */
int move_vessel_toward_waypoint(struct greyhawk_ship_data *ship)
{
  struct autopilot_data *ap;
  struct waypoint *wp;
  float dx, dy;
  float speed;
  float new_x, new_y;
  int target_x, target_y;

  if (ship == NULL)
  {
    log("SYSERR: move_vessel_toward_waypoint called with NULL ship");
    return 0;
  }

  ap = ship->autopilot;
  if (ap == NULL)
  {
    return 0;
  }

  wp = waypoint_get_current(ship);
  if (wp == NULL)
  {
    log("SYSERR: move_vessel_toward_waypoint - no current waypoint");
    return 0;
  }

  /* Get ship speed (use current_speed or a default) */
  speed = (float)ship->speed;
  if (speed <= 0.0f)
  {
    speed = 1.0f; /* Minimum movement speed */
  }

  /* Calculate heading to waypoint */
  calculate_heading_to_waypoint(ship, wp, &dx, &dy);

  /* If already at destination (heading is zero) */
  if (dx == 0.0f && dy == 0.0f)
  {
    return 0;
  }

  /* Calculate new position */
  new_x = ship->x + (dx * speed);
  new_y = ship->y + (dy * speed);

  /* Round to integer coordinates for wilderness system */
  target_x = (int)(new_x + 0.5f);
  target_y = (int)(new_y + 0.5f);

  /* Check if terrain is valid for this vessel before moving */
  if (!can_vessel_traverse_terrain(ship->vessel_type, target_x, target_y, (int)ship->z))
  {
    log("Info: Autopilot ship %d - impassable terrain at (%d, %d)", ship->shipnum, target_x,
        target_y);
    return 0;
  }

  /* Update ship position using the centralized wilderness position function.
   * This handles:
   * - Coordinate updates (ship->x, ship->y, ship->z)
   * - Dynamic wilderness room allocation via get_or_allocate_wilderness_room()
   * - Updating ship->location to the new room vnum
   * - Moving ship object via obj_from_room()/obj_to_room() to allow room recycling
   */
  if (!update_ship_wilderness_position(ship->shipnum, target_x, target_y, (int)ship->z))
  {
    log("SYSERR: Autopilot ship %d - failed to update position to (%d, %d)", ship->shipnum,
        target_x, target_y);
    return 0;
  }

  return 1;
}

/**
 * Process a vessel in WAITING state.
 *
 * Decrements the wait timer and transitions to TRAVELING
 * when the wait is complete, then advances to next waypoint.
 *
 * @param ship The ship to process
 */
void process_waiting_vessel(struct greyhawk_ship_data *ship)
{
  struct autopilot_data *ap;
  time_t now;
  int elapsed;

  if (ship == NULL)
  {
    return;
  }

  ap = ship->autopilot;
  if (ap == NULL || ap->state != AUTOPILOT_WAITING)
  {
    return;
  }

  /* Calculate elapsed time since last update */
  now = time(0);
  elapsed = (int)(now - ap->last_update);
  ap->last_update = now;

  /* Decrement wait timer */
  ap->wait_remaining -= elapsed;

  if (ap->wait_remaining <= 0)
  {
    /* Wait complete, advance to next waypoint */
    ap->wait_remaining = 0;
    ap->state = AUTOPILOT_TRAVELING;
    log("Info: Ship %d wait complete, advancing to next waypoint", ship->shipnum);
    advance_to_next_waypoint(ship);
  }
}

/**
 * Process a vessel in TRAVELING state.
 *
 * Main movement loop for a single vessel:
 * - Check if arrived at current waypoint
 * - If arrived: handle arrival (wait or advance)
 * - If not arrived: move toward waypoint
 *
 * @param ship The ship to process
 */
void process_traveling_vessel(struct greyhawk_ship_data *ship)
{
  struct autopilot_data *ap;
  struct waypoint *wp;

  if (ship == NULL)
  {
    return;
  }

  ap = ship->autopilot;
  if (ap == NULL || ap->state != AUTOPILOT_TRAVELING)
  {
    return;
  }

  /* Get current waypoint */
  wp = waypoint_get_current(ship);
  if (wp == NULL)
  {
    log("SYSERR: process_traveling_vessel - no current waypoint for ship %d", ship->shipnum);
    ap->state = AUTOPILOT_OFF;
    return;
  }

  /* Check if we've arrived at the waypoint */
  if (check_waypoint_arrival(ship, wp))
  {
    handle_waypoint_arrival(ship);
    return;
  }

  /* Not arrived yet, move toward waypoint */
  move_vessel_toward_waypoint(ship);
}

/**
 * Main autopilot tick function.
 *
 * Called from the game heartbeat every AUTOPILOT_TICK_INTERVAL pulses.
 * Iterates through all ships in greyhawk_ships[] and processes
 * those with active autopilot.
 */
void autopilot_tick(void)
{
  struct greyhawk_ship_data *ship;
  struct autopilot_data *ap;
  int i;
  int active_count;

  active_count = 0;

  for (i = 0; i < GREYHAWK_MAXSHIPS; i++)
  {
    ship = &greyhawk_ships[i];

    /* Skip uninitialized ships (check for valid shipnum) */
    if (ship->shipnum <= 0)
    {
      continue;
    }

    /* Skip ships without autopilot */
    ap = ship->autopilot;
    if (ap == NULL)
    {
      continue;
    }

    /* Process based on autopilot state */
    switch (ap->state)
    {
    case AUTOPILOT_TRAVELING:
      process_traveling_vessel(ship);
      active_count++;
      break;

    case AUTOPILOT_WAITING:
      process_waiting_vessel(ship);
      active_count++;
      break;

    case AUTOPILOT_PAUSED:
      /* Paused ships don't move but count as active */
      active_count++;
      break;

    case AUTOPILOT_OFF:
      /* Auto-engage if pilot assigned with route */
      if (ap->pilot_mob_vnum != -1 && ap->current_route != NULL &&
          ap->current_route->num_waypoints > 0)
      {
        /* Verify pilot is still present */
        if (get_pilot_from_ship(ship) != NULL)
        {
          autopilot_start(ship, ap->current_route);
          send_to_ship(ship, "The pilot engages autopilot.\r\n");
          active_count++;
        }
      }
      break;

    case AUTOPILOT_COMPLETE:
    default:
      /* No processing needed */
      break;
    }
  }

  /* Optional: Log active autopilot count periodically for debugging */
  /* if (active_count > 0) log("Info: Autopilot tick - %d active ships", active_count); */
}

/* ========================================================================= */
/* PLAYER COMMAND HELPER FUNCTIONS                                            */
/* ========================================================================= */

/**
 * Get the vessel a character is currently on.
 * Sends an error message if not on a vessel.
 *
 * @param ch The character to check
 * @return Pointer to ship data, or NULL if not on a vessel
 */
static struct greyhawk_ship_data *get_vessel_for_command(struct char_data *ch)
{
  struct greyhawk_ship_data *ship;

  if (ch == NULL)
  {
    return NULL;
  }

  ship = get_ship_from_room(IN_ROOM(ch));
  if (ship == NULL)
  {
    send_to_char(ch, "You must be aboard a vessel to use this command.\r\n");
    return NULL;
  }

  return ship;
}

/**
 * Check if character has permission to control the vessel's autopilot.
 * Must be at the helm (bridge) or be the ship owner.
 * Sends an error message if permission denied.
 *
 * @param ch The character to check
 * @param ship The ship to check permissions for
 * @return TRUE if has permission, FALSE otherwise
 */
static int check_vessel_captain(struct char_data *ch, struct greyhawk_ship_data *ship)
{
  if (ch == NULL || ship == NULL)
  {
    return FALSE;
  }

  /* Check if at the helm (bridge room) */
  if (is_pilot(ch, ship))
  {
    return TRUE;
  }

  /* Check if character is the owner */
  if (ship->owner[0] != '\0' && !strcmp(ship->owner, GET_NAME(ch)))
  {
    return TRUE;
  }

  /* Immortals always have access */
  if (GET_LEVEL(ch) >= LVL_IMMORT)
  {
    return TRUE;
  }

  send_to_char(ch, "You must be at the helm or be the ship's owner to control the autopilot.\r\n");
  return FALSE;
}

/* ========================================================================= */
/* AUTOPILOT STATE NAME HELPER                                                */
/* ========================================================================= */

/**
 * Get a human-readable name for an autopilot state.
 *
 * @param state The autopilot state
 * @return String name of the state
 */
static const char *autopilot_state_name(enum autopilot_state state)
{
  switch (state)
  {
  case AUTOPILOT_OFF:
    return "Off";
  case AUTOPILOT_TRAVELING:
    return "Traveling";
  case AUTOPILOT_WAITING:
    return "Waiting at Waypoint";
  case AUTOPILOT_PAUSED:
    return "Paused";
  case AUTOPILOT_COMPLETE:
    return "Route Complete";
  default:
    return "Unknown";
  }
}

/* ========================================================================= */
/* PLAYER COMMAND HANDLERS - AUTOPILOT CONTROL                                */
/* ========================================================================= */

/**
 * ACMD handler for autopilot command.
 * Usage: autopilot [on|off|status]
 */
ACMD(do_autopilot)
{
  struct greyhawk_ship_data *ship;
  struct autopilot_data *ap;
  struct waypoint *wp;
  char arg[MAX_INPUT_LENGTH];

  /* Get vessel context */
  ship = get_vessel_for_command(ch);
  if (ship == NULL)
  {
    return;
  }

  /* Parse argument */
  one_argument(argument, arg, sizeof(arg));

  /* Handle status subcommand (no permission needed) */
  if (!*arg || !str_cmp(arg, "status"))
  {
    ap = ship->autopilot;
    if (ap == NULL)
    {
      send_to_char(ch, "This vessel does not have autopilot capability.\r\n");
      return;
    }

    send_to_char(ch, "\r\n--- Autopilot Status ---\r\n");
    send_to_char(ch, "State: %s\r\n", autopilot_state_name(ap->state));

    if (ap->current_route != NULL)
    {
      send_to_char(ch, "Route: %s (%d waypoints%s)\r\n", ap->current_route->name,
                   ap->current_route->num_waypoints, ap->current_route->loop ? ", looping" : "");
      send_to_char(ch, "Progress: Waypoint %d of %d\r\n", ap->current_waypoint_index + 1,
                   ap->current_route->num_waypoints);

      wp = waypoint_get_current(ship);
      if (wp != NULL)
      {
        send_to_char(ch, "Current Target: %s (%.1f, %.1f)\r\n", wp->name[0] ? wp->name : "Unnamed",
                     wp->x, wp->y);
        send_to_char(ch, "Distance: %.1f\r\n", calculate_distance_to_waypoint(ship, wp));
      }

      if (ap->state == AUTOPILOT_WAITING && ap->wait_remaining > 0)
      {
        send_to_char(ch, "Wait Time Remaining: %d seconds\r\n", ap->wait_remaining);
      }
    }
    else
    {
      send_to_char(ch, "Route: None assigned\r\n");
    }

    send_to_char(ch, "Position: (%.1f, %.1f, %.1f)\r\n", ship->x, ship->y, ship->z);
    send_to_char(ch, "-------------------------\r\n");
    return;
  }

  /* For on/off commands, need captain permission */
  if (!check_vessel_captain(ch, ship))
  {
    return;
  }

  if (!str_cmp(arg, "on"))
  {
    ap = ship->autopilot;
    if (ap == NULL)
    {
      /* Initialize autopilot if not present */
      ap = autopilot_init(ship);
      if (ap == NULL)
      {
        send_to_char(ch, "Failed to initialize autopilot system.\r\n");
        return;
      }
    }

    if (ap->current_route == NULL)
    {
      send_to_char(ch, "No route assigned. Use 'setroute <name>' first.\r\n");
      return;
    }

    if (ap->current_route->num_waypoints == 0)
    {
      send_to_char(ch, "The assigned route has no waypoints.\r\n");
      return;
    }

    if (ap->state == AUTOPILOT_TRAVELING || ap->state == AUTOPILOT_WAITING)
    {
      send_to_char(ch, "Autopilot is already active.\r\n");
      return;
    }

    if (ap->state == AUTOPILOT_PAUSED)
    {
      autopilot_resume(ship);
      send_to_char(ch, "Autopilot resumed.\r\n");
      send_to_ship(ship, "The vessel's autopilot has been resumed.\r\n");
    }
    else
    {
      autopilot_start(ship, ap->current_route);
      send_to_char(ch, "Autopilot engaged on route '%s'.\r\n", ap->current_route->name);
      send_to_ship(ship, "The vessel's autopilot has been engaged.\r\n");
    }
    return;
  }

  if (!str_cmp(arg, "off"))
  {
    ap = ship->autopilot;
    if (ap == NULL || ap->state == AUTOPILOT_OFF)
    {
      send_to_char(ch, "Autopilot is not active.\r\n");
      return;
    }

    autopilot_stop(ship);
    send_to_char(ch, "Autopilot disengaged.\r\n");
    send_to_ship(ship, "The vessel's autopilot has been disengaged.\r\n");
    return;
  }

  if (!str_cmp(arg, "pause"))
  {
    ap = ship->autopilot;
    if (ap == NULL || (ap->state != AUTOPILOT_TRAVELING && ap->state != AUTOPILOT_WAITING))
    {
      send_to_char(ch, "Autopilot is not currently navigating.\r\n");
      return;
    }

    autopilot_pause(ship);
    send_to_char(ch, "Autopilot paused.\r\n");
    send_to_ship(ship, "The vessel's autopilot has been paused.\r\n");
    return;
  }

  send_to_char(ch, "Usage: autopilot [on|off|pause|status]\r\n");
}

/* ========================================================================= */
/* PLAYER COMMAND HANDLERS - WAYPOINT MANAGEMENT                              */
/* ========================================================================= */

/**
 * ACMD handler for setwaypoint command.
 * Creates a new waypoint at the vessel's current position.
 * Usage: setwaypoint <name>
 */
ACMD(do_setwaypoint)
{
  struct greyhawk_ship_data *ship;
  struct waypoint wp;
  char arg[MAX_INPUT_LENGTH];
  int waypoint_id;
  int i;

  /* Get vessel context */
  ship = get_vessel_for_command(ch);
  if (ship == NULL)
  {
    return;
  }

  /* Check permission */
  if (!check_vessel_captain(ch, ship))
  {
    return;
  }

  /* Get waypoint name */
  one_argument(argument, arg, sizeof(arg));
  if (!*arg)
  {
    send_to_char(ch, "Usage: setwaypoint <name>\r\n");
    return;
  }

  /* Validate name length */
  if (strlen(arg) >= AUTOPILOT_NAME_LENGTH)
  {
    send_to_char(ch, "Waypoint name too long (max %d characters).\r\n", AUTOPILOT_NAME_LENGTH - 1);
    return;
  }

  /* Validate name characters (alphanumeric and underscore only) */
  for (i = 0; arg[i]; i++)
  {
    if (!isalnum(arg[i]) && arg[i] != '_' && arg[i] != '-')
    {
      send_to_char(
          ch, "Waypoint name can only contain letters, numbers, underscores, and hyphens.\r\n");
      return;
    }
  }

  /* Initialize waypoint data */
  memset(&wp, 0, sizeof(struct waypoint));
  wp.x = ship->x;
  wp.y = ship->y;
  wp.z = ship->z;
  wp.tolerance = 5.0f;
  wp.wait_time = 0;
  wp.flags = 0;
  strncpy(wp.name, arg, AUTOPILOT_NAME_LENGTH - 1);
  wp.name[AUTOPILOT_NAME_LENGTH - 1] = '\0';

  /* Create waypoint in database */
  waypoint_id = waypoint_db_create(&wp);
  if (waypoint_id < 0)
  {
    send_to_char(ch, "Failed to create waypoint.\r\n");
    return;
  }

  send_to_char(ch, "Waypoint '%s' created at position (%.1f, %.1f, %.1f).\r\n", arg, wp.x, wp.y,
               wp.z);
}

/**
 * ACMD handler for listwaypoints command.
 * Lists all waypoints in the database.
 * Usage: listwaypoints
 */
ACMD(do_listwaypoints)
{
  struct waypoint_node *current;
  int count;

  (void)argument; /* Unused */

  /* Must be on a vessel */
  if (!is_in_ship_interior(ch))
  {
    send_to_char(ch, "You must be aboard a vessel to use this command.\r\n");
    return;
  }

  send_to_char(ch, "\r\n--- Waypoints ---\r\n");
  send_to_char(ch, "%-4s %-20s %10s %10s %10s\r\n", "ID", "Name", "X", "Y", "Z");
  send_to_char(ch, "---- -------------------- ---------- ---------- ----------\r\n");

  count = 0;
  for (current = waypoint_list; current != NULL; current = current->next)
  {
    send_to_char(ch, "%-4d %-20s %10.1f %10.1f %10.1f\r\n", current->waypoint_id,
                 current->data.name[0] ? current->data.name : "(unnamed)", current->data.x,
                 current->data.y, current->data.z);
    count++;
  }

  if (count == 0)
  {
    send_to_char(ch, "No waypoints defined.\r\n");
  }
  else
  {
    send_to_char(ch, "----------------------------\r\n");
    send_to_char(ch, "Total: %d waypoint%s\r\n", count, count == 1 ? "" : "s");
  }
}

/**
 * ACMD handler for delwaypoint command.
 * Deletes a waypoint by name.
 * Usage: delwaypoint <name>
 */
ACMD(do_delwaypoint)
{
  struct greyhawk_ship_data *ship;
  struct waypoint_node *current;
  char arg[MAX_INPUT_LENGTH];
  int found_id;

  /* Get vessel context */
  ship = get_vessel_for_command(ch);
  if (ship == NULL)
  {
    return;
  }

  /* Check permission */
  if (!check_vessel_captain(ch, ship))
  {
    return;
  }

  /* Get waypoint name */
  one_argument(argument, arg, sizeof(arg));
  if (!*arg)
  {
    send_to_char(ch, "Usage: delwaypoint <name>\r\n");
    return;
  }

  /* Find waypoint by name */
  found_id = -1;
  for (current = waypoint_list; current != NULL; current = current->next)
  {
    if (!str_cmp(current->data.name, arg))
    {
      found_id = current->waypoint_id;
      break;
    }
  }

  if (found_id < 0)
  {
    send_to_char(ch, "Waypoint '%s' not found.\r\n", arg);
    return;
  }

  /* Delete from database */
  if (waypoint_db_delete(found_id))
  {
    send_to_char(ch, "Waypoint '%s' deleted.\r\n", arg);
  }
  else
  {
    send_to_char(ch, "Failed to delete waypoint '%s'.\r\n", arg);
  }
}

/* ========================================================================= */
/* PLAYER COMMAND HANDLERS - ROUTE MANAGEMENT                                 */
/* ========================================================================= */

/**
 * ACMD handler for createroute command.
 * Creates a new empty route.
 * Usage: createroute <name>
 */
ACMD(do_createroute)
{
  struct greyhawk_ship_data *ship;
  char arg[MAX_INPUT_LENGTH];
  int route_id;
  int i;

  /* Get vessel context */
  ship = get_vessel_for_command(ch);
  if (ship == NULL)
  {
    return;
  }

  /* Check permission */
  if (!check_vessel_captain(ch, ship))
  {
    return;
  }

  /* Get route name */
  one_argument(argument, arg, sizeof(arg));
  if (!*arg)
  {
    send_to_char(ch, "Usage: createroute <name>\r\n");
    return;
  }

  /* Validate name length */
  if (strlen(arg) >= AUTOPILOT_NAME_LENGTH)
  {
    send_to_char(ch, "Route name too long (max %d characters).\r\n", AUTOPILOT_NAME_LENGTH - 1);
    return;
  }

  /* Validate name characters */
  for (i = 0; arg[i]; i++)
  {
    if (!isalnum(arg[i]) && arg[i] != '_' && arg[i] != '-')
    {
      send_to_char(ch,
                   "Route name can only contain letters, numbers, underscores, and hyphens.\r\n");
      return;
    }
  }

  /* Create route in database */
  route_id = route_db_create(arg, FALSE);
  if (route_id < 0)
  {
    send_to_char(ch, "Failed to create route.\r\n");
    return;
  }

  send_to_char(ch, "Route '%s' created (ID: %d).\r\n", arg, route_id);
}

/**
 * ACMD handler for addtoroute command.
 * Adds a waypoint to a route.
 * Usage: addtoroute <route> <waypoint>
 */
ACMD(do_addtoroute)
{
  struct greyhawk_ship_data *ship;
  struct route_node *route;
  struct waypoint_node *waypoint;
  char route_arg[MAX_INPUT_LENGTH];
  char wp_arg[MAX_INPUT_LENGTH];

  /* Get vessel context */
  ship = get_vessel_for_command(ch);
  if (ship == NULL)
  {
    return;
  }

  /* Check permission */
  if (!check_vessel_captain(ch, ship))
  {
    return;
  }

  /* Parse arguments */
  two_arguments_u((char *)argument, route_arg, wp_arg);
  if (!*route_arg || !*wp_arg)
  {
    send_to_char(ch, "Usage: addtoroute <route> <waypoint>\r\n");
    return;
  }

  /* Find route by name */
  route = NULL;
  for (route = route_list; route != NULL; route = route->next)
  {
    if (!str_cmp(route->name, route_arg))
    {
      break;
    }
  }

  if (route == NULL)
  {
    send_to_char(ch, "Route '%s' not found.\r\n", route_arg);
    return;
  }

  /* Find waypoint by name */
  waypoint = NULL;
  for (waypoint = waypoint_list; waypoint != NULL; waypoint = waypoint->next)
  {
    if (!str_cmp(waypoint->data.name, wp_arg))
    {
      break;
    }
  }

  if (waypoint == NULL)
  {
    send_to_char(ch, "Waypoint '%s' not found.\r\n", wp_arg);
    return;
  }

  /* Check route capacity */
  if (route->num_waypoints >= MAX_WAYPOINTS_PER_ROUTE)
  {
    send_to_char(ch, "Route '%s' is full (max %d waypoints).\r\n", route_arg,
                 MAX_WAYPOINTS_PER_ROUTE);
    return;
  }

  /* Add waypoint to route */
  if (route_add_waypoint_db(route->route_id, waypoint->waypoint_id, route->num_waypoints))
  {
    send_to_char(ch, "Waypoint '%s' added to route '%s' at position %d.\r\n", wp_arg, route_arg,
                 route->num_waypoints);
  }
  else
  {
    send_to_char(ch, "Failed to add waypoint to route.\r\n");
  }
}

/**
 * ACMD handler for listroutes command.
 * Lists all routes in the database.
 * Usage: listroutes
 */
ACMD(do_listroutes)
{
  struct route_node *current;
  int count;

  (void)argument; /* Unused */

  /* Must be on a vessel */
  if (!is_in_ship_interior(ch))
  {
    send_to_char(ch, "You must be aboard a vessel to use this command.\r\n");
    return;
  }

  send_to_char(ch, "\r\n--- Routes ---\r\n");
  send_to_char(ch, "%-4s %-20s %5s %6s %6s\r\n", "ID", "Name", "WPs", "Loop", "Active");
  send_to_char(ch, "---- -------------------- ----- ------ ------\r\n");

  count = 0;
  for (current = route_list; current != NULL; current = current->next)
  {
    send_to_char(ch, "%-4d %-20s %5d %6s %6s\r\n", current->route_id,
                 current->name[0] ? current->name : "(unnamed)", current->num_waypoints,
                 current->loop ? "Yes" : "No", current->active ? "Yes" : "No");
    count++;
  }

  if (count == 0)
  {
    send_to_char(ch, "No routes defined.\r\n");
  }
  else
  {
    send_to_char(ch, "----------------------------\r\n");
    send_to_char(ch, "Total: %d route%s\r\n", count, count == 1 ? "" : "s");
  }
}

/**
 * ACMD handler for setroute command.
 * Assigns a route to the vessel's autopilot.
 * Usage: setroute <name>
 */
ACMD(do_setroute)
{
  struct greyhawk_ship_data *ship;
  struct autopilot_data *ap;
  struct route_node *route_node;
  struct ship_route *route;
  struct waypoint_node *wp_node;
  char arg[MAX_INPUT_LENGTH];
  int i;

  /* Get vessel context */
  ship = get_vessel_for_command(ch);
  if (ship == NULL)
  {
    return;
  }

  /* Check permission */
  if (!check_vessel_captain(ch, ship))
  {
    return;
  }

  /* Get route name */
  one_argument(argument, arg, sizeof(arg));
  if (!*arg)
  {
    send_to_char(ch, "Usage: setroute <name>\r\n");
    return;
  }

  /* Find route by name in cache */
  route_node = NULL;
  for (route_node = route_list; route_node != NULL; route_node = route_node->next)
  {
    if (!str_cmp(route_node->name, arg))
    {
      break;
    }
  }

  if (route_node == NULL)
  {
    send_to_char(ch, "Route '%s' not found.\r\n", arg);
    return;
  }

  if (route_node->num_waypoints == 0)
  {
    send_to_char(ch, "Route '%s' has no waypoints. Add waypoints with 'addtoroute'.\r\n", arg);
    return;
  }

  /* Ensure autopilot is initialized */
  ap = ship->autopilot;
  if (ap == NULL)
  {
    ap = autopilot_init(ship);
    if (ap == NULL)
    {
      send_to_char(ch, "Failed to initialize autopilot system.\r\n");
      return;
    }
  }

  /* Stop current autopilot if running */
  if (ap->state != AUTOPILOT_OFF && ap->state != AUTOPILOT_COMPLETE)
  {
    autopilot_stop(ship);
    send_to_char(ch, "Previous autopilot navigation stopped.\r\n");
  }

  /* Create ship_route from route_node data */
  route = route_create(route_node->name);
  if (route == NULL)
  {
    send_to_char(ch, "Failed to load route data.\r\n");
    return;
  }

  route->route_id = route_node->route_id;
  route->loop = route_node->loop;
  route->active = route_node->active;
  route->num_waypoints = 0;

  /* Load waypoints into route */
  for (i = 0; i < route_node->num_waypoints && i < MAX_WAYPOINTS_PER_ROUTE; i++)
  {
    wp_node = waypoint_cache_find(route_node->waypoint_ids[i]);
    if (wp_node != NULL)
    {
      route->waypoints[route->num_waypoints] = wp_node->data;
      route->num_waypoints++;
    }
  }

  /* Assign route to autopilot */
  if (ap->current_route != NULL)
  {
    route_destroy(ap->current_route);
  }
  ap->current_route = route;
  ap->current_waypoint_index = 0;

  send_to_char(ch, "Route '%s' assigned to autopilot (%d waypoints).\r\n", arg,
               route->num_waypoints);
  send_to_char(ch, "Use 'autopilot on' to begin navigation.\r\n");
}

/* ========================================================================= */
/* NPC PILOT FUNCTIONS                                                        */
/* ========================================================================= */

/**
 * Validates if an NPC can serve as pilot for a vessel.
 * Checks: is NPC, is in helm room, vessel doesn't already have pilot.
 *
 * @param ch The captain issuing the assignment
 * @param npc The NPC to validate as pilot
 * @param ship The vessel to assign pilot to
 * @return TRUE if valid pilot, FALSE otherwise (sends error to ch)
 */
int is_valid_pilot_npc(struct char_data *ch, struct char_data *npc, struct greyhawk_ship_data *ship)
{
  if (ch == NULL)
  {
    log("SYSERR: is_valid_pilot_npc called with NULL ch");
    return FALSE;
  }

  if (npc == NULL)
  {
    send_to_char(ch, "You don't see that person here.\r\n");
    return FALSE;
  }

  if (ship == NULL)
  {
    log("SYSERR: is_valid_pilot_npc called with NULL ship");
    send_to_char(ch, "Error: No vessel context.\r\n");
    return FALSE;
  }

  /* Must be an NPC */
  if (!IS_NPC(npc))
  {
    send_to_char(ch, "Only NPCs can be assigned as vessel pilots.\r\n");
    return FALSE;
  }

  /* Must be in the helm/bridge room */
  if (IN_ROOM(npc) != real_room(ship->bridge_room))
  {
    send_to_char(ch, "%s must be in the helm room to be assigned as pilot.\r\n", GET_NAME(npc));
    return FALSE;
  }

  /* Vessel cannot already have a pilot */
  if (ship->autopilot != NULL && ship->autopilot->pilot_mob_vnum != -1)
  {
    send_to_char(ch, "This vessel already has a pilot assigned. "
                     "Use 'unassignpilot' first.\r\n");
    return FALSE;
  }

  return TRUE;
}

/**
 * Finds the pilot NPC for a ship by matching pilot_mob_vnum.
 * Searches the helm/bridge room for an NPC with matching VNUM.
 *
 * @param ship The vessel to find pilot for
 * @return Pointer to pilot NPC, or NULL if not found
 */
struct char_data *get_pilot_from_ship(struct greyhawk_ship_data *ship)
{
  struct char_data *tch;
  room_rnum bridge_room;

  if (ship == NULL)
  {
    log("SYSERR: get_pilot_from_ship called with NULL ship");
    return NULL;
  }

  if (ship->autopilot == NULL || ship->autopilot->pilot_mob_vnum == -1)
  {
    return NULL;
  }

  bridge_room = real_room(ship->bridge_room);
  if (bridge_room == NOWHERE)
  {
    return NULL;
  }

  /* Search bridge room for NPC matching pilot VNUM */
  for (tch = world[bridge_room].people; tch != NULL; tch = tch->next_in_room)
  {
    if (IS_NPC(tch) && GET_MOB_VNUM(tch) == ship->autopilot->pilot_mob_vnum)
    {
      return tch;
    }
  }

  return NULL;
}

/**
 * Announces waypoint arrival to all vessel occupants.
 * Called when a piloted vessel arrives at a waypoint.
 *
 * @param ship The vessel arriving at waypoint
 * @param wp The waypoint being arrived at
 */
void pilot_announce_waypoint(struct greyhawk_ship_data *ship, struct waypoint *wp)
{
  struct char_data *pilot;
  char pilot_name[128];

  if (ship == NULL || wp == NULL)
  {
    return;
  }

  pilot = get_pilot_from_ship(ship);
  if (pilot != NULL)
  {
    snprintf(pilot_name, sizeof(pilot_name), "%s", GET_NAME(pilot));
  }
  else
  {
    snprintf(pilot_name, sizeof(pilot_name), "The pilot");
  }

  /* Announce to all aboard */
  if (wp->name[0] != '\0')
  {
    send_to_ship(ship, "%s announces, 'Arriving at %s!'\r\n", pilot_name, wp->name);
  }
  else
  {
    send_to_ship(ship, "%s announces, 'Arriving at waypoint!'\r\n", pilot_name);
  }
}

/* ========================================================================= */
/* NPC PILOT COMMAND HANDLERS                                                 */
/* ========================================================================= */

/**
 * ACMD handler for assignpilot command.
 * Assigns an NPC in the helm room as the vessel's pilot.
 * Usage: assignpilot <npc name>
 */
ACMD(do_assignpilot)
{
  struct greyhawk_ship_data *ship;
  struct char_data *npc;
  char arg[MAX_INPUT_LENGTH];
  int num;

  /* Get vessel context */
  ship = get_vessel_for_command(ch);
  if (ship == NULL)
  {
    return;
  }

  /* Check captain permission */
  if (!check_vessel_captain(ch, ship))
  {
    return;
  }

  /* Parse NPC name argument */
  one_argument(argument, arg, sizeof(arg));
  if (!*arg)
  {
    send_to_char(ch, "Usage: assignpilot <npc name>\r\n");
    return;
  }

  /* Initialize autopilot if needed */
  if (ship->autopilot == NULL)
  {
    ship->autopilot = autopilot_init(ship);
    if (ship->autopilot == NULL)
    {
      send_to_char(ch, "Failed to initialize autopilot system.\r\n");
      return;
    }
  }

  /* Find NPC in helm room */
  num = 1;
  npc = get_char_room(arg, &num, real_room(ship->bridge_room));

  /* Validate pilot */
  if (!is_valid_pilot_npc(ch, npc, ship))
  {
    return;
  }

  /* Assign pilot */
  ship->autopilot->pilot_mob_vnum = GET_MOB_VNUM(npc);

  send_to_char(ch, "You assign %s as the vessel's pilot.\r\n", GET_NAME(npc));
  send_to_ship(ship, "%s has been assigned as the vessel's pilot.\r\n", GET_NAME(npc));

  /* If a route is already set, auto-engage autopilot */
  if (ship->autopilot->current_route != NULL && ship->autopilot->current_route->num_waypoints > 0 &&
      ship->autopilot->state == AUTOPILOT_OFF)
  {
    autopilot_start(ship, ship->autopilot->current_route);
    send_to_char(ch, "%s takes the helm and engages autopilot.\r\n", GET_NAME(npc));
    send_to_ship(ship, "The vessel's autopilot has been engaged.\r\n");
  }
}

/**
 * ACMD handler for unassignpilot command.
 * Removes the current NPC pilot from the vessel.
 * Usage: unassignpilot
 */
ACMD(do_unassignpilot)
{
  struct greyhawk_ship_data *ship;
  struct char_data *pilot;
  char pilot_name[128];

  /* Get vessel context */
  ship = get_vessel_for_command(ch);
  if (ship == NULL)
  {
    return;
  }

  /* Check captain permission */
  if (!check_vessel_captain(ch, ship))
  {
    return;
  }

  /* Check if vessel has autopilot */
  if (ship->autopilot == NULL)
  {
    send_to_char(ch, "This vessel does not have autopilot capability.\r\n");
    return;
  }

  /* Check if pilot is assigned */
  if (ship->autopilot->pilot_mob_vnum == -1)
  {
    send_to_char(ch, "This vessel does not have a pilot assigned.\r\n");
    return;
  }

  /* Get pilot name for message */
  pilot = get_pilot_from_ship(ship);
  if (pilot != NULL)
  {
    snprintf(pilot_name, sizeof(pilot_name), "%s", GET_NAME(pilot));
  }
  else
  {
    snprintf(pilot_name, sizeof(pilot_name), "The pilot");
  }

  /* Stop autopilot if running */
  if (ship->autopilot->state == AUTOPILOT_TRAVELING || ship->autopilot->state == AUTOPILOT_WAITING)
  {
    autopilot_stop(ship);
    send_to_ship(ship, "The vessel's autopilot has been disengaged.\r\n");
  }

  /* Clear pilot assignment */
  ship->autopilot->pilot_mob_vnum = -1;

  send_to_char(ch, "You relieve %s of pilot duties.\r\n", pilot_name);
  send_to_ship(ship, "%s has been relieved of pilot duties.\r\n", pilot_name);
}

/* ========================================================================= */
/* SCHEDULE MANAGEMENT FUNCTIONS                                              */
/* ========================================================================= */

/**
 * Calculate the next departure MUD hour based on current time and interval.
 *
 * @param sched The schedule to update
 */
void schedule_calculate_next_departure(struct vessel_schedule *sched)
{
  int current_hour;

  if (sched == NULL)
  {
    return;
  }

  current_hour = time_info.hours;
  sched->next_departure = current_hour + sched->interval_hours;

  /* Wrap around 24-hour day */
  if (sched->next_departure >= 24)
  {
    sched->next_departure = sched->next_departure % 24;
  }
}

/**
 * Create a schedule for a vessel.
 *
 * @param ship The ship to create schedule for
 * @param route_id The route ID to assign
 * @param interval The interval in MUD hours
 * @return 1 on success, 0 on failure
 */
int schedule_create(struct greyhawk_ship_data *ship, int route_id, int interval)
{
  if (ship == NULL)
  {
    log("SYSERR: schedule_create called with NULL ship");
    return 0;
  }

  /* Validate interval */
  if (interval < SCHEDULE_INTERVAL_MIN || interval > SCHEDULE_INTERVAL_MAX)
  {
    log("SYSERR: schedule_create: invalid interval %d", interval);
    return 0;
  }

  /* Allocate schedule if needed */
  if (ship->schedule == NULL)
  {
    CREATE(ship->schedule, struct vessel_schedule, 1);
    if (ship->schedule == NULL)
    {
      log("SYSERR: Unable to allocate schedule for ship %d", ship->shipnum);
      return 0;
    }
  }

  /* Set schedule data */
  ship->schedule->schedule_id = 0;
  ship->schedule->ship_id = ship->shipnum;
  ship->schedule->route_id = route_id;
  ship->schedule->interval_hours = interval;
  ship->schedule->flags = SCHEDULE_FLAG_ENABLED;

  /* Calculate first departure time */
  schedule_calculate_next_departure(ship->schedule);

  /* Save to database */
  schedule_save(ship);

  log("Info: Created schedule for ship %d (route %d, interval %d hours)", ship->shipnum, route_id,
      interval);
  return 1;
}

/**
 * Clear/remove a vessel's schedule.
 *
 * @param ship The ship to clear schedule for
 * @return 1 on success, 0 on failure
 */
int schedule_clear(struct greyhawk_ship_data *ship)
{
  if (ship == NULL)
  {
    log("SYSERR: schedule_clear called with NULL ship");
    return 0;
  }

  if (ship->schedule == NULL)
  {
    return 1; /* Already cleared */
  }

  /* Free memory */
  free(ship->schedule);
  ship->schedule = NULL;

  /* Remove from database */
  schedule_save(ship);

  log("Info: Cleared schedule for ship %d", ship->shipnum);
  return 1;
}

/**
 * Check if a vessel's schedule is enabled.
 *
 * @param ship The ship to check
 * @return 1 if enabled, 0 otherwise
 */
int schedule_is_enabled(struct greyhawk_ship_data *ship)
{
  if (ship == NULL || ship->schedule == NULL)
  {
    return 0;
  }

  return (ship->schedule->flags & SCHEDULE_FLAG_ENABLED) ? 1 : 0;
}

/**
 * Get a vessel's schedule.
 *
 * @param ship The ship to get schedule for
 * @return Pointer to schedule, or NULL if none
 */
struct vessel_schedule *schedule_get(struct greyhawk_ship_data *ship)
{
  if (ship == NULL)
  {
    return NULL;
  }

  return ship->schedule;
}

/**
 * Check if a vessel's schedule should trigger a departure.
 *
 * @param ship The ship to check
 * @return 1 if should trigger, 0 otherwise
 */
int schedule_check_trigger(struct greyhawk_ship_data *ship)
{
  int current_hour;

  if (ship == NULL || ship->schedule == NULL)
  {
    return 0;
  }

  /* Check if enabled */
  if (!(ship->schedule->flags & SCHEDULE_FLAG_ENABLED))
  {
    return 0;
  }

  /* Check if vessel already traveling */
  if (ship->autopilot != NULL && (ship->autopilot->state == AUTOPILOT_TRAVELING ||
                                  ship->autopilot->state == AUTOPILOT_WAITING))
  {
    return 0;
  }

  current_hour = time_info.hours;

  /* Use >= comparison for timer precision */
  if (current_hour >= ship->schedule->next_departure)
  {
    return 1;
  }

  return 0;
}

/**
 * Trigger a scheduled departure for a vessel.
 *
 * @param ship The ship to trigger departure for
 * @return 1 on success, 0 on failure
 */
int schedule_trigger_departure(struct greyhawk_ship_data *ship)
{
  struct route_node *route_node;
  struct ship_route *route;
  struct waypoint_node *wp_node;
  int i;

  if (ship == NULL || ship->schedule == NULL)
  {
    return 0;
  }

  /* Find route in cache */
  route_node = NULL;
  for (route_node = route_list; route_node != NULL; route_node = route_node->next)
  {
    if (route_node->route_id == ship->schedule->route_id)
    {
      break;
    }
  }

  if (route_node == NULL)
  {
    log("SYSERR: schedule_trigger_departure: route %d not found for ship %d",
        ship->schedule->route_id, ship->shipnum);
    return 0;
  }

  /* Initialize autopilot if needed */
  if (ship->autopilot == NULL)
  {
    ship->autopilot = autopilot_init(ship);
    if (ship->autopilot == NULL)
    {
      log("SYSERR: schedule_trigger_departure: failed to init autopilot");
      return 0;
    }
  }

  /* Build route from cache */
  route = route_create(route_node->name);
  if (route == NULL)
  {
    return 0;
  }

  route->route_id = route_node->route_id;
  route->loop = route_node->loop;
  route->active = route_node->active;

  /* Load waypoints into route */
  for (i = 0; i < route_node->num_waypoints && i < MAX_WAYPOINTS_PER_ROUTE; i++)
  {
    wp_node = waypoint_cache_find(route_node->waypoint_ids[i]);
    if (wp_node != NULL)
    {
      route->waypoints[route->num_waypoints] = wp_node->data;
      route->num_waypoints++;
    }
  }

  /* Start autopilot */
  if (!autopilot_start(ship, route))
  {
    route_destroy(route);
    return 0;
  }

  /* Announce departure via pilot if present */
  if (ship->autopilot->pilot_mob_vnum != -1)
  {
    pilot_announce_waypoint(ship, waypoint_get_current(ship));
  }

  /* Calculate next departure */
  schedule_calculate_next_departure(ship->schedule);
  schedule_save(ship);

  log("Info: Scheduled departure triggered for ship %d on route %s", ship->shipnum,
      route_node->name);

  return 1;
}

/**
 * Timer tick for schedule processing.
 * Called once per MUD hour to check and trigger scheduled departures.
 */
void schedule_tick(void)
{
  int i;
  int triggered_count = 0;

  for (i = 0; i < GREYHAWK_MAXSHIPS; i++)
  {
    if (is_valid_ship(&greyhawk_ships[i]) && schedule_check_trigger(&greyhawk_ships[i]))
    {
      if (schedule_trigger_departure(&greyhawk_ships[i]))
      {
        triggered_count++;
      }
    }
  }

  if (triggered_count > 0)
  {
    log("Info: schedule_tick triggered %d departures", triggered_count);
  }
}

/* ========================================================================= */
/* SCHEDULE COMMAND HANDLERS                                                  */
/* ========================================================================= */

/**
 * ACMD handler for setschedule command.
 * Usage: setschedule <route> <interval>
 */
ACMD(do_setschedule)
{
  struct greyhawk_ship_data *ship;
  struct route_node *route_node;
  char route_arg[MAX_INPUT_LENGTH];
  char interval_arg[MAX_INPUT_LENGTH];
  int interval;

  /* Get vessel context */
  ship = get_vessel_for_command(ch);
  if (ship == NULL)
  {
    return;
  }

  /* Check captain permission */
  if (!check_vessel_captain(ch, ship))
  {
    return;
  }

  /* Parse arguments */
  two_arguments_u((char *)argument, route_arg, interval_arg);
  if (!*route_arg || !*interval_arg)
  {
    send_to_char(ch, "Usage: setschedule <route> <interval>\r\n");
    send_to_char(ch, "  route    - Name of the route to run\r\n");
    send_to_char(ch, "  interval - Hours between departures (%d-%d)\r\n", SCHEDULE_INTERVAL_MIN,
                 SCHEDULE_INTERVAL_MAX);
    return;
  }

  /* Validate interval */
  interval = atoi(interval_arg);
  if (interval < SCHEDULE_INTERVAL_MIN || interval > SCHEDULE_INTERVAL_MAX)
  {
    send_to_char(ch, "Interval must be between %d and %d MUD hours.\r\n", SCHEDULE_INTERVAL_MIN,
                 SCHEDULE_INTERVAL_MAX);
    return;
  }

  /* Find route by name */
  route_node = NULL;
  for (route_node = route_list; route_node != NULL; route_node = route_node->next)
  {
    if (!str_cmp(route_node->name, route_arg))
    {
      break;
    }
  }

  if (route_node == NULL)
  {
    send_to_char(ch, "Route '%s' not found. Use 'listroutes' to see available routes.\r\n",
                 route_arg);
    return;
  }

  /* Check route has waypoints */
  if (route_node->num_waypoints < 1)
  {
    send_to_char(ch, "Route '%s' has no waypoints defined.\r\n", route_arg);
    return;
  }

  /* Create schedule */
  if (!schedule_create(ship, route_node->route_id, interval))
  {
    send_to_char(ch, "Failed to create schedule.\r\n");
    return;
  }

  send_to_char(ch, "Schedule set: Route '%s' every %d MUD hours.\r\n", route_arg, interval);
  send_to_char(ch, "Next departure: MUD hour %d\r\n", ship->schedule->next_departure);
  send_to_ship(ship, "A departure schedule has been set for this vessel.\r\n");
}

/**
 * ACMD handler for clearschedule command.
 * Usage: clearschedule
 */
ACMD(do_clearschedule)
{
  struct greyhawk_ship_data *ship;

  /* Get vessel context */
  ship = get_vessel_for_command(ch);
  if (ship == NULL)
  {
    return;
  }

  /* Check captain permission */
  if (!check_vessel_captain(ch, ship))
  {
    return;
  }

  /* Check if schedule exists */
  if (ship->schedule == NULL)
  {
    send_to_char(ch, "This vessel has no schedule to clear.\r\n");
    return;
  }

  /* Clear schedule */
  schedule_clear(ship);

  send_to_char(ch, "Vessel schedule has been cleared.\r\n");
  send_to_ship(ship, "The departure schedule for this vessel has been cancelled.\r\n");
}

/**
 * ACMD handler for showschedule command.
 * Usage: showschedule
 */
ACMD(do_showschedule)
{
  struct greyhawk_ship_data *ship;
  struct vessel_schedule *sched;
  struct route_node *route_node;
  const char *status;

  /* Get vessel context */
  ship = get_vessel_for_command(ch);
  if (ship == NULL)
  {
    return;
  }

  sched = ship->schedule;
  if (sched == NULL)
  {
    send_to_char(ch, "This vessel has no schedule configured.\r\n");
    return;
  }

  /* Find route name */
  route_node = NULL;
  for (route_node = route_list; route_node != NULL; route_node = route_node->next)
  {
    if (route_node->route_id == sched->route_id)
    {
      break;
    }
  }

  /* Determine status */
  if (sched->flags & SCHEDULE_FLAG_PAUSED)
  {
    status = "Paused";
  }
  else if (sched->flags & SCHEDULE_FLAG_ENABLED)
  {
    status = "Active";
  }
  else
  {
    status = "Disabled";
  }

  send_to_char(ch, "\r\n--- Vessel Schedule ---\r\n");
  send_to_char(ch, "Route: %s\r\n", route_node ? route_node->name : "(unknown)");
  send_to_char(ch, "Interval: Every %d MUD hour%s\r\n", sched->interval_hours,
               sched->interval_hours == 1 ? "" : "s");
  send_to_char(ch, "Next Departure: MUD hour %d\r\n", sched->next_departure);
  send_to_char(ch, "Current Time: MUD hour %d\r\n", time_info.hours);
  send_to_char(ch, "Status: %s\r\n", status);

  /* Show pilot status */
  if (ship->autopilot != NULL && ship->autopilot->pilot_mob_vnum != -1)
  {
    send_to_char(ch, "Pilot: Assigned (departures will be announced)\r\n");
  }
  else
  {
    send_to_char(ch, "Pilot: None (silent departures)\r\n");
  }
}
