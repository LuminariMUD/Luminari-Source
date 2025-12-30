/* ************************************************************************
 *      File:   vessels_autopilot.c                     Part of LuminariMUD  *
 *   Purpose:   Autopilot system for vessel navigation                       *
 *              Phase 3 - Automation Layer Session 01                        *
 * ********************************************************************** */

#include "vessels.h"

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
