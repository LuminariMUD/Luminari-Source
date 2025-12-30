/* ************************************************************************
 *      File:   vessels_autopilot.c                     Part of LuminariMUD  *
 *   Purpose:   Autopilot system for vessel navigation                       *
 *              Phase 3 - Automation Layer                                   *
 *              Session 01: Data structures                                  *
 *              Session 02: Waypoint/Route database persistence              *
 * ********************************************************************** */

#include "vessels.h"
#include "mysql.h"

/* External MySQL connection variables */
extern MYSQL *conn;
extern bool mysql_available;

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
  if (ship->autopilot->state == AUTOPILOT_TRAVELING ||
      ship->autopilot->state == AUTOPILOT_WAITING)
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

  route->route_id = 0;  /* TODO: Generate unique ID */
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
  (void)route_id;  /* Suppress unused parameter warning */
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

  log("Info: Created waypoint %d '%s' at (%.2f, %.2f, %.2f)",
      new_id, wp->name, wp->x, wp->y, wp->z);

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
    escaped_name, wp->x, wp->y, wp->z, wp->tolerance, wp->wait_time,
    wp->flags, waypoint_id);

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

  snprintf(query, sizeof(query),
    "DELETE FROM ship_waypoints WHERE waypoint_id = %d",
    waypoint_id);

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

  snprintf(query, sizeof(query),
    "DELETE FROM ship_routes WHERE route_id = %d",
    route_id);

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

  log("Info: Added waypoint %d to route %d at position %d",
      waypoint_id, route_id, sequence_num);

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
  snprintf(query, sizeof(query),
    "DELETE FROM ship_route_waypoints WHERE route_id = %d",
    route_id);

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
    return 1;  /* Success with empty result */
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
