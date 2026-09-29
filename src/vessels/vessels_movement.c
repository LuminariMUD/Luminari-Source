/* ************************************************************************
 *      File:   vessels_movement.c                    Part of LuminariMUD  *
 *   Purpose:   Vessel movement and pacing: momentum sailing, per-room     *
 *              movement, harbor maneuvers, departure, and anchoring       *
 *              (vessels-ships study S2, decision D1)                      *
 * ********************************************************************** */

/*
 * Orders set targets and the vessel tick converges on them, as DurisMUD
 * does. `speed` and `heading` set setspeed and setheading; every 0.5 s
 * vessel_movement_tick_one() accelerates by the class accel and turns by the
 * class turn rate, both scaled by the sailmaster, and the rudder scales the
 * turn. The hull then covers speed / 90 rooms along its heading. Its place
 * inside the current room lives in dx/dy; it crosses room edges in the order
 * its track meets them (diagonally only through a corner) and enters each room
 * through update_ship_wilderness_position(), which validates every room
 * entered. A refused room stops the hull at its edge. Scheduled-route
 * validation sails a copy of the hull through the same vessel_sail_tick(),
 * checking rooms with vessel_chart_cell() instead of entering them.
 *
 * A hull at rest in a port room is berthed (dock holds the port room vnum).
 * A berthed or anchored hull holds position until `undock` completes: 30 s
 * from a berth, 13 s from anchor. `setsail` is the harbor maneuver: one room
 * at speed 6 or less, every 5 s.
 */

#include "conf.h"
#include "core/sysdep.h"
#include <math.h>
#include "core/structs.h"
#include "core/utils.h"
#include "core/comm.h"
#include "core/db.h"
#include "core/handler.h"
#include "core/interpreter.h"
#include "vessels.h"
#include "wilderness/wilderness.h"

extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];

/* Class order follows enum vessel_class. */
static const struct vessel_class_handling class_handling[NUM_VESSEL_TYPES] = {
    /* speed accel turn  load free  hold free */
    {5, 5.0, 25.0, 5, 0, 2, 0},       /* RAFT: Duris sloop */
    {30, 4.0, 22.0, 12, 2, 6, 0},     /* BOAT: yacht */
    {20, 2.0, 6.5, 100, 13, 70, 12},  /* SHIP: caravel */
    {17, 1.5, 4.0, 142, 20, 56, 0},   /* WARSHIP: frigate */
    {22, 3.0, 10.0, 82, 13, 32, 0},   /* AIRSHIP: corvette */
    {12, 2.0, 6.5, 110, 16, 44, 0},   /* SUBMARINE: destroyer */
    {15, 1.2, 3.0, 165, 19, 140, 40}, /* TRANSPORT: galleon */
    {14, 1.2, 2.5, 200, 25, 80, 0}    /* MAGICAL: cruiser */
};

/* Crossings of a room's two edges this close together (in ticks) are one
 * crossing through its corner. */
#define VESSEL_CORNER_TOLERANCE 1e-9

static bool vessel_enter_cell_default(struct greyhawk_ship_data *ship, int x, int y, int z)
{
  return update_ship_wilderness_position(ship->shipnum, x, y, z);
}

static bool vessel_chart_cell_default(struct greyhawk_ship_data *ship, int x, int y, int z)
{
  return vessel_can_occupy_coordinates(ship->vessel_type, x, y, z);
}

static vessel_cell_entry_fn vessel_enter_cell = vessel_enter_cell_default;
static vessel_cell_entry_fn vessel_chart_check = vessel_chart_cell_default;

#ifdef LUMINARI_CUTEST
void vessel_movement_set_cell_entry_for_test(vessel_cell_entry_fn entry)
{
  vessel_enter_cell = entry != NULL ? entry : vessel_enter_cell_default;
  vessel_chart_check = entry != NULL ? entry : vessel_chart_cell_default;
}
#endif

/**
 * Whether the hull could enter a room, checked without allocating or
 * entering it; the cell check for route validation.
 */
bool vessel_chart_cell(struct greyhawk_ship_data *ship, int x, int y, int z)
{
  return vessel_chart_check(ship, x, y, z);
}

const struct vessel_class_handling *vessel_class_handling(enum vessel_class vessel_type)
{
  if (vessel_type < 0 || vessel_type >= NUM_VESSEL_TYPES)
  {
    vessel_type = VESSEL_SHIP;
  }
  return &class_handling[vessel_type];
}

/** Round a heading into 0..359 for display. */
int vessel_display_heading(double heading)
{
  long rounded;

  rounded = lround(heading) % 360;
  return (int)(rounded < 0 ? rounded + 360 : rounded);
}

/** Round a speed for display and integer rules. */
int vessel_display_speed(double speed)
{
  return (int)lround(speed);
}

/** Signed turn from one heading to another, the short way round (-180..180). */
double vessel_heading_difference(double from, double to)
{
  double difference;

  difference = fmod(to - from, 360.0);
  if (difference > 180.0)
  {
    difference -= 360.0;
  }
  else if (difference < -180.0)
  {
    difference += 360.0;
  }
  return difference;
}

static double vessel_normalize_heading(double heading)
{
  heading = fmod(heading, 360.0);
  return heading < 0.0 ? heading + 360.0 : heading;
}

/* seadog (Sep 2026 racial innate): +1 maximum speed while at the helm */
int vessel_pilot_speed_bonus(struct char_data *ch)
{
  if (ch == NULL || !HAS_FEAT(ch, FEAT_SEADOG))
  {
    return 0;
  }
  return 1;
}

/**
 * The best speed bonus among the characters at this hull's helm.
 */
int vessel_helm_speed_bonus(struct greyhawk_ship_data *ship)
{
  struct char_data *ch;
  room_rnum bridge;
  int bonus;

  if (ship == NULL || ship->bridge_room <= 0)
  {
    return 0;
  }
  bridge = real_room(ship->bridge_room);
  if (bridge == NOWHERE)
  {
    return 0;
  }

  bonus = 0;
  for (ch = world[bridge].people; ch != NULL; ch = ch->next_in_room)
  {
    if (is_pilot(ch, ship))
    {
      bonus = MAX(bonus, vessel_pilot_speed_bonus(ch));
    }
  }
  return bonus;
}

/** Sailmaster tiers multiply speed, accel, and turn by 1.1, 1.2, or 1.3. */
double vessel_sailmaster_multiplier(const struct greyhawk_ship_data *ship)
{
  if (ship == NULL)
  {
    return 1.0;
  }
  return 1.0 + 0.1 * (double)MAX(0, MIN(CREW_TIER_VETERAN, ship->crew_tier[CREW_SAILMASTER]));
}

/**
 * Share of the design speed left after weight: 1 minus the fit-out and hold
 * weight above their free allowances, divided by the class weight budget.
 * Hold weight is the bulk cargo's share of the class hold capacity times the
 * weight of a full hold.
 */
double vessel_load_factor(const struct greyhawk_ship_data *ship)
{
  const struct vessel_class_handling *handling;
  double hold_weight;
  double excess;
  int fitout_weight;
  int capacity;
  int i;

  if (ship == NULL)
  {
    return 1.0;
  }
  handling = vessel_class_handling(ship->vessel_type);
  if (handling->max_load <= 0)
  {
    return 1.0;
  }

  fitout_weight = 0;
  for (i = 0; i < GREYHAWK_MAXSLOTS; i++)
  {
    if (ship->slot[i].type != 0)
    {
      fitout_weight += ship->slot[i].weight;
    }
  }

  capacity = get_vessel_cargo_capacity(ship->vessel_type);
  hold_weight = capacity > 0 ? (double)vessel_cargo_weight(ship) * (double)handling->full_hold /
                                   (double)capacity
                             : 0.0;

  excess = (double)MAX(0, fitout_weight - handling->free_fitout) +
           fmax(0.0, hold_weight - (double)handling->free_hold);
  return 1.0 - excess / (double)handling->max_load;
}

/**
 * Maximum speed from its parts (study 3.3.2): the design speed times the
 * sailmaster, load, and sail factors and the terrain, weather, and lane
 * percentage, at least 1, plus the helm bonus, at most VESSEL_SPEED_LIMIT.
 * A hull with its sail shot away cannot make way.
 */
double vessel_max_speed_from(int design_speed, double sailmaster_multiplier, double load_factor,
                             int mainsail, int maxmainsail, int position_percent, int helm_bonus)
{
  double speed;

  if (design_speed <= 0 || (maxmainsail > 0 && mainsail <= 0))
  {
    return 0.0;
  }

  speed = (double)design_speed * sailmaster_multiplier * load_factor;
  if (maxmainsail > 0)
  {
    speed = speed * (double)mainsail / (double)maxmainsail;
  }
  speed = speed * (double)MAX(0, position_percent) / 100.0;
  speed = fmax(1.0, speed) + (double)MAX(0, helm_bonus);
  return fmin((double)VESSEL_SPEED_LIMIT, speed);
}

static int vessel_position_speed_percent(struct greyhawk_ship_data *ship)
{
  if (ship->position_speed_percent <= 0)
  {
    ship->position_speed_percent = (short int)get_vessel_position_speed_modifier(
        ship->vessel_type, get_ship_terrain_type(ship->shipnum), vessel_storm_severity(ship),
        (int)ship->x, (int)ship->y, (int)ship->z, NULL);
  }
  return ship->position_speed_percent;
}

double vessel_max_speed(struct greyhawk_ship_data *ship)
{
  if (ship == NULL)
  {
    return 0.0;
  }
  return vessel_max_speed_from(ship->maxspeed, vessel_sailmaster_multiplier(ship),
                               vessel_load_factor(ship), ship->mainsail, ship->maxmainsail,
                               vessel_position_speed_percent(ship), vessel_helm_speed_bonus(ship));
}

/** Speed gained or shed per tick. */
double vessel_acceleration(const struct greyhawk_ship_data *ship)
{
  if (ship == NULL)
  {
    return 0.0;
  }
  return vessel_class_handling(ship->vessel_type)->accel * vessel_sailmaster_multiplier(ship);
}

/**
 * Degrees the hull can turn this tick.
 *
 * The class turn rate, times the sailmaster, times three quarters at
 * steerage speed rising to the full rate at design speed, times the share of
 * rudder left. A smashed rudder cannot turn; an immobile hull warps round
 * one degree a tick.
 */
double vessel_turn_rate(const struct greyhawk_ship_data *ship, double max_speed)
{
  double speed_factor;
  double rudder;

  if (ship == NULL)
  {
    return 0.0;
  }
  if (ship->maxturnrate > 0 && ship->turnrate == 0)
  {
    return 0.0;
  }
  if (max_speed <= 0.0)
  {
    return 1.0;
  }

  speed_factor = 0.75;
  if (ship->maxspeed > VESSEL_STEERAGE_SPEED + 1)
  {
    speed_factor += 0.25 * (ship->speed - (double)(VESSEL_STEERAGE_SPEED + 1)) /
                    (double)(ship->maxspeed - (VESSEL_STEERAGE_SPEED + 1));
  }
  speed_factor = fmax(0.25, speed_factor);
  rudder = ship->maxturnrate > 0 ? (double)ship->turnrate / (double)ship->maxturnrate : 1.0;

  return vessel_class_handling(ship->vessel_type)->turn * vessel_sailmaster_multiplier(ship) *
         speed_factor * rudder;
}

/**
 * Berthed, anchored, or made fast alongside another hull. Fleet slot 0 is
 * reserved, so docked_to_ship names a partner only when positive.
 */
bool vessel_is_moored(const struct greyhawk_ship_data *ship)
{
  return ship != NULL && (ship->dock > 0 || ship->anchored || ship->docked_to_ship > 0);
}

static int vessel_port_room_vnum(const struct greyhawk_ship_data *ship)
{
  room_rnum room;

  room = ship->shipobj != NULL ? IN_ROOM(ship->shipobj) : NOWHERE;
  if (vessel_room_is_port(room))
  {
    return (int)world[room].number;
  }
  return ship->location > 0 ? ship->location : 1;
}

/**
 * Make the hull fast at the port it rests in.
 *
 * Public and NPC hulls have no owner to repair them, so the harbor makes good
 * their rigging and rudder whenever they berth; wear and weather would
 * otherwise leave a scheduled hull crawling with its sail in rags.
 */
void vessel_berth(struct greyhawk_ship_data *ship)
{
  if (ship == NULL)
  {
    return;
  }
  if (ship->owner[0] == '\0')
  {
    ship->mainsail = ship->maxmainsail;
    ship->turnrate = ship->maxturnrate;
  }
  ship->dock = vessel_port_room_vnum(ship);
  ship->anchored = FALSE;
  ship->departure_ticks = 0;
  ship->speed = 0.0;
  ship->setspeed = 0;
  ship->dx = 0.0;
  ship->dy = 0.0;
}

/**
 * Reconcile the berth with the hull's position after spawn or reboot: a hull
 * at rest in port is berthed, and a berth away from port is stale.
 */
void vessel_sync_berth(struct greyhawk_ship_data *ship)
{
  bool in_port;

  if (!is_valid_ship(ship))
  {
    return;
  }
  in_port = vessel_ship_is_in_port(ship);
  if (ship->dock > 0 && !in_port)
  {
    ship->dock = 0;
  }
  else if (ship->dock <= 0 && in_port && ship->speed <= 0.0)
  {
    vessel_berth(ship);
  }
}

/**
 * Start casting off from a berth or weighing anchor.
 *
 * ch is the character giving the order, or NULL for a pilot. Departure from
 * a berth needs a whole sail, settled dock fees, and a captain of the hull's
 * minimum level.
 *
 * @return TRUE when the crew set to work
 */
bool vessel_begin_departure(struct greyhawk_ship_data *ship, struct char_data *ch)
{
  bool berthed;

  if (!is_valid_ship(ship))
  {
    return FALSE;
  }
  if (ship->dock <= 0 && !ship->anchored)
  {
    if (ch != NULL)
    {
      send_to_char(ch, "%s is neither berthed nor at anchor.\r\n", ship->name);
    }
    return FALSE;
  }
  if (ship->departure_ticks > 0)
  {
    if (ch != NULL)
    {
      send_to_char(ch, "The crew is already working on it.\r\n");
    }
    return FALSE;
  }

  berthed = ship->dock > 0;
  if (berthed)
  {
    if (ship->maxmainsail > 0 && ship->mainsail == 0)
    {
      if (ch != NULL)
      {
        send_to_char(ch, "You cannot make sail: the rigging is destroyed.\r\n");
      }
      return FALSE;
    }
    if (ship->dock_fee_balance > 0)
    {
      if (ch != NULL)
      {
        send_to_char(ch,
                     "The harbor master withholds clearance: %d gold in dock fees remains due.\r\n",
                     ship->dock_fee_balance);
      }
      return FALSE;
    }
    if (vessel_helm_level_refused(ch, ship))
    {
      return FALSE;
    }
  }

  ship->departure_ticks = (short int)(berthed ? VESSEL_UNDOCK_TICKS : VESSEL_WEIGH_ANCHOR_TICKS);
  send_to_ship(ship, berthed ? "The crew begins casting off." : "The crew begins weighing anchor.");
  return TRUE;
}

static void vessel_departure_tick(struct greyhawk_ship_data *ship)
{
  ship->departure_ticks--;
  if (ship->departure_ticks > 0)
  {
    return;
  }
  ship->dock = 0;
  ship->anchored = FALSE;
  send_to_ship(ship, "The first officer reports %s ready to get under way.", ship->name);
}

static const char *vessel_blocked_text(enum vessel_class vessel_type, int z)
{
  switch (vessel_type)
  {
  case VESSEL_RAFT:
    return "Your raft cannot navigate these waters! It's only suitable for rivers and shallow "
           "water.";
  case VESSEL_BOAT:
    return "Your boat cannot handle these conditions! It's designed for coastal waters only.";
  case VESSEL_SHIP:
  case VESSEL_WARSHIP:
    return "The ship cannot navigate this terrain! It requires deep water to sail.";
  case VESSEL_AIRSHIP:
    return z < 100 ? "The airship cannot fly through this terrain at low altitude! Gain more "
                     "altitude."
                   : "The airship cannot fly here - perhaps it's underground or the altitude is "
                     "too extreme.";
  case VESSEL_SUBMARINE:
    return z >= 0 ? "The submarine must dive to navigate underwater terrain! Use 'setsail down' "
                    "to submerge."
                  : "The submarine cannot traverse this area while submerged!";
  case VESSEL_TRANSPORT:
    return "The transport vessel draws too much water for this area!";
  case VESSEL_MAGICAL:
    return "Even magical forces cannot penetrate this barrier!";
  default:
    return "The vessel cannot navigate that terrain!";
  }
}

static int vessel_heading_for_direction(int direction, int current_heading)
{
  switch (direction)
  {
  case NORTH:
    return 0;
  case NORTHEAST:
    return 45;
  case EAST:
    return 90;
  case SOUTHEAST:
    return 135;
  case SOUTH:
    return 180;
  case SOUTHWEST:
    return 225;
  case WEST:
    return 270;
  case NORTHWEST:
    return 315;
  default:
    return current_heading;
  }
}

/* Tell the helm when a maneuver brings the hull into a high-altitude lane. */
static void vessel_report_lane(struct greyhawk_ship_data *ship, struct char_data *ch)
{
  struct vessel_region_feature lane;

  if ((ship->vessel_type == VESSEL_AIRSHIP || ship->vessel_type == VESSEL_MAGICAL) &&
      vessel_region_feature_at_coordinates(REGION_ALTITUDE_LANE, (int)ship->x, (int)ship->y,
                                           (int)ship->z, &lane))
  {
    send_to_char(ch, "The high currents of %s lend speed to the vessel.\r\n", lane.name);
  }
}

/**
 * setsail: move the hull one room in a direction, or ten units up or down.
 *
 * The harbor maneuver: once every VESSEL_MANEUVER_COOLDOWN_TICKS, and across
 * the map only at speed VESSEL_MANEUVER_MAX_SPEED or less. A horizontal
 * maneuver leaves the hull stopped on the new heading, berthed if the room is
 * a port. Climbing and diving keep the hull under way.
 *
 * @return TRUE when the hull moved
 */
bool vessel_maneuver(struct greyhawk_ship_data *ship, struct char_data *ch, int direction)
{
  int x;
  int y;
  int z;

  if (!is_valid_ship(ship) || ch == NULL)
  {
    return FALSE;
  }
  if (ship->docked_to_ship > 0)
  {
    send_to_char(ch, "%s is made fast alongside another vessel; 'undock' first.\r\n", ship->name);
    return FALSE;
  }
  if (ship->dock > 0)
  {
    send_to_char(ch, "%s is berthed; order 'undock' to cast off first.\r\n", ship->name);
    return FALSE;
  }
  if (ship->anchored)
  {
    send_to_char(ch, "%s rides at anchor; order 'undock' to weigh anchor first.\r\n", ship->name);
    return FALSE;
  }
  if (ship->maneuver_ticks > 0)
  {
    send_to_char(ch, "The crew is not ready to maneuver again yet.\r\n");
    return FALSE;
  }
  if (direction != UP && direction != DOWN && ship->speed > (double)VESSEL_MANEUVER_MAX_SPEED)
  {
    send_to_char(ch, "You're coming in too fast! Slow to speed %d or less to maneuver.\r\n",
                 VESSEL_MANEUVER_MAX_SPEED);
    return FALSE;
  }
  if (vessel_max_speed(ship) <= 0.0)
  {
    send_to_char(ch, "%s is immobile and cannot maneuver.\r\n", ship->name);
    return FALSE;
  }

  x = (int)ship->x;
  y = (int)ship->y;
  z = (int)ship->z;
  switch (direction)
  {
  case NORTH:
    y++;
    break;
  case SOUTH:
    y--;
    break;
  case EAST:
    x++;
    break;
  case WEST:
    x--;
    break;
  case NORTHEAST:
    x++;
    y++;
    break;
  case NORTHWEST:
    x--;
    y++;
    break;
  case SOUTHEAST:
    x++;
    y--;
    break;
  case SOUTHWEST:
    x--;
    y--;
    break;
  case UP:
    z += 10;
    break;
  case DOWN:
    z -= 10;
    break;
  default:
    return FALSE;
  }

  if (!vessel_enter_cell(ship, x, y, z))
  {
    send_to_char(ch, "%s\r\n", vessel_blocked_text(ship->vessel_type, z));
    return FALSE;
  }

  ship->maneuver_ticks = VESSEL_MANEUVER_COOLDOWN_TICKS;
  if (direction == UP || direction == DOWN)
  {
    send_to_char(ch, "The vessel %s to %d.\r\n", direction == UP ? "climbs" : "descends", z);
    vessel_report_lane(ship, ch);
    return TRUE;
  }

  ship->speed = 0.0;
  ship->setspeed = 0;
  ship->heading =
      (double)vessel_heading_for_direction(direction, vessel_display_heading(ship->heading));
  ship->setheading = (short int)ship->heading;
  ship->dx = 0.0;
  ship->dy = 0.0;
  send_to_char(ch, "The vessel maneuvers %s.\r\n", dirs[direction]);
  send_to_char(ch, "Current position: (%d, %d, %d)\r\n", x, y, z);
  vessel_report_lane(ship, ch);
  if (vessel_ship_is_in_port(ship))
  {
    vessel_berth(ship);
    send_to_ship(ship, "Lines go ashore; %s is made fast at the berth.", ship->name);
  }
  return TRUE;
}

/**
 * The hull could not enter the next room and lies at its edge: stop it, and
 * hand an autopilot back to its crew.
 */
static void vessel_stop_at_edge(struct greyhawk_ship_data *ship)
{
  struct waypoint *wp;

  ship->speed = 0.0;
  ship->setspeed = 0;
  send_to_ship(ship, "%s The crew brings her up short.",
               vessel_blocked_text(ship->vessel_type, (int)ship->z));
  VSSL_DEBUG_MOVE("Ship %d stopped at the edge of (%d,%d)", ship->shipnum, (int)ship->x,
                  (int)ship->y);

  if (ship->autopilot != NULL && ship->autopilot->state == AUTOPILOT_TRAVELING)
  {
    wp = waypoint_get_current(ship);
    autopilot_pause(ship);
    send_to_ship(ship, "Autopilot pauses: the route to '%s' is not traversable from here.",
                 wp != NULL ? wp->name : "its waypoint");
    if (!vessel_db_save_runtime(ship))
    {
      log("SYSERR: Ship %d could not persist the movement-failure pause", ship->shipnum);
    }
  }
}

/* How many ticks ago the track, covering move each tick, passed the room
 * edge that offset now lies beyond. */
static double vessel_time_past_edge(double offset, double move)
{
  return fabs(move) > 0.0 ? (fabs(offset) - 0.5) / fabs(move) : 0.0;
}

/**
 * Carry the hull across the room edges its position has passed this tick,
 * in the order its track met them, checking every room it enters with
 * entry(); it crosses into the diagonal room only when the track runs through
 * the corner. move_x and move_y are this tick's travel.
 *
 * @return FALSE when a room refused the hull, which then lies where its track
 *         met that room's edge; refused_x and refused_y name the room
 */
static bool vessel_cross_room_edges(struct greyhawk_ship_data *ship, double move_x, double move_y,
                                    vessel_cell_entry_fn entry, int *refused_x, int *refused_y)
{
  double past_x;
  double past_y;
  double back;
  int step_x;
  int step_y;
  int x;
  int y;

  while (ship->dx > 0.5 || ship->dx < -0.5 || ship->dy > 0.5 || ship->dy < -0.5)
  {
    step_x = ship->dx > 0.5 ? 1 : (ship->dx < -0.5 ? -1 : 0);
    step_y = ship->dy > 0.5 ? 1 : (ship->dy < -0.5 ? -1 : 0);
    past_x = step_x != 0 ? vessel_time_past_edge(ship->dx, move_x) : 0.0;
    past_y = step_y != 0 ? vessel_time_past_edge(ship->dy, move_y) : 0.0;
    if (step_x != 0 && step_y != 0 && fabs(past_x - past_y) > VESSEL_CORNER_TOLERANCE)
    {
      /* The edge passed longer ago was crossed first. */
      if (past_x > past_y)
      {
        step_y = 0;
      }
      else
      {
        step_x = 0;
      }
    }

    x = (int)ship->x + step_x;
    y = (int)ship->y + step_y;
    if (!entry(ship, x, y, (int)ship->z))
    {
      back = fmax(step_x != 0 ? past_x : 0.0, step_y != 0 ? past_y : 0.0);
      ship->dx = fmax(-0.5, fmin(0.5, ship->dx - move_x * back));
      ship->dy = fmax(-0.5, fmin(0.5, ship->dy - move_y * back));
      if (refused_x != NULL)
      {
        *refused_x = x;
      }
      if (refused_y != NULL)
      {
        *refused_y = y;
      }
      return FALSE;
    }
    ship->x = (double)x;
    ship->y = (double)y;
    ship->dx -= (double)step_x;
    ship->dy -= (double)step_y;
    if (ship->autopilot != NULL)
    {
      ship->autopilot->movement_steps++;
    }
  }
  return TRUE;
}

/**
 * Climb or dive to one altitude or depth without moving across the map.
 *
 * @return TRUE when the hull reached z
 */
bool vessel_change_altitude(struct greyhawk_ship_data *ship, int z)
{
  return is_valid_ship(ship) && vessel_enter_cell(ship, (int)ship->x, (int)ship->y, z);
}

static bool vessel_autopilot_steering(const struct greyhawk_ship_data *ship)
{
  return ship->autopilot != NULL && (ship->autopilot->state == AUTOPILOT_TRAVELING ||
                                     ship->autopilot->state == AUTOPILOT_WAITING);
}

static bool vessel_autopilot_paused(const struct greyhawk_ship_data *ship)
{
  return ship->autopilot != NULL && ship->autopilot->state == AUTOPILOT_PAUSED;
}

/**
 * One tick of the helm, for every mover and for route validation.
 *
 * Speed converges on its order at the class acceleration and the heading on
 * its order at the turn rate; the hull then covers speed / 90 rooms along
 * its heading, entering each room through entry(). An autopilot cruises at
 * the ordered speed, or at full speed when none is ordered, within its
 * steering cap; a paused autopilot holds the hull.
 *
 * @return FALSE when entry() refused a room (see vessel_cross_room_edges())
 */
bool vessel_sail_tick(struct greyhawk_ship_data *ship, double max_speed, vessel_cell_entry_fn entry,
                      int *refused_x, int *refused_y)
{
  double target;
  double change;
  double limit;
  double radians;
  double distance;
  double move_x;
  double move_y;

  target = fmin((double)MAX(0, ship->setspeed), max_speed);
  if (vessel_autopilot_steering(ship))
  {
    if (ship->setspeed <= 0)
    {
      target = max_speed;
    }
    target = fmin(target, ship->autopilot->speed_limit);
  }
  else if (vessel_autopilot_paused(ship))
  {
    target = 0.0;
  }
  target = fmax(0.0, target);

  limit = vessel_acceleration(ship);
  change = target - ship->speed;
  ship->speed = fabs(change) <= limit ? target : ship->speed + copysign(limit, change);

  change = vessel_heading_difference(ship->heading, (double)ship->setheading);
  limit = vessel_turn_rate(ship, max_speed);
  if (fabs(change) <= limit)
  {
    ship->heading = vessel_normalize_heading((double)ship->setheading);
  }
  else
  {
    ship->heading = vessel_normalize_heading(ship->heading + copysign(limit, change));
  }

  if (ship->speed <= 0.0)
  {
    return TRUE;
  }
  radians = ship->heading * M_PI / 180.0;
  distance = ship->speed / VESSEL_SPEED_PER_ROOM;
  move_x = distance * sin(radians);
  move_y = distance * cos(radians);
  ship->dx += move_x;
  ship->dy += move_y;
  return vessel_cross_room_edges(ship, move_x, move_y, entry, refused_x, refused_y);
}

/**
 * Advance one hull by one vessel tick: finish departures, converge speed and
 * heading on their orders, and sail along the heading.
 */
void vessel_movement_tick_one(struct greyhawk_ship_data *ship)
{
  bool was_moving;

  if (!is_valid_ship(ship))
  {
    return;
  }
  if (ship->maneuver_ticks > 0)
  {
    ship->maneuver_ticks--;
  }
  if (ship->departure_ticks > 0)
  {
    vessel_departure_tick(ship);
    return;
  }
  if (vessel_is_moored(ship))
  {
    ship->speed = 0.0;
    return;
  }

  /* An idle hull has nothing to converge on. */
  if (ship->speed <= 0.0 && ship->setspeed <= 0 && !vessel_autopilot_steering(ship) &&
      fabs(vessel_heading_difference(ship->heading, (double)ship->setheading)) < 0.001)
  {
    return;
  }

  was_moving = ship->speed > 0.0;
  if (!vessel_sail_tick(ship, vessel_max_speed(ship), vessel_enter_cell, NULL, NULL))
  {
    vessel_stop_at_edge(ship);
    was_moving = TRUE;
  }

  /* A hull coming to rest records where she lies, berthed in port. */
  if (was_moving && ship->speed <= 0.0)
  {
    if (vessel_ship_is_in_port(ship))
    {
      vessel_berth(ship);
      send_to_ship(ship, "Lines go ashore; %s is made fast at the berth.", ship->name);
    }
    vessel_db_save_runtime(ship);
  }
}

/**
 * anchor: drop anchor where the hull lies stopped at sea.
 */
ACMD(do_vessel_anchor)
{
  struct greyhawk_ship_data *ship;

  ship = get_ship_from_room(IN_ROOM(ch));
  if (!is_valid_ship(ship))
  {
    send_to_char(ch, "You must be aboard a vessel to drop anchor.\r\n");
    return;
  }
  if (!is_pilot(ch, ship))
  {
    send_to_char(ch, "You must be at an authorized helm to drop anchor.\r\n");
    return;
  }
  if (ship->anchored)
  {
    send_to_char(ch, "%s already rides at anchor.\r\n", ship->name);
    return;
  }
  if (ship->dock > 0 || ship->docked_to_ship > 0)
  {
    send_to_char(ch, "%s is made fast already; there is no need to anchor.\r\n", ship->name);
    return;
  }
  if (ship->speed > 0.0)
  {
    send_to_char(ch, "Bring her to a stop before dropping anchor.\r\n");
    return;
  }
  if ((int)ship->z != 0)
  {
    send_to_char(ch, "%s must be on the surface to anchor.\r\n", ship->name);
    return;
  }

  if (ship->autopilot != NULL && (ship->autopilot->state == AUTOPILOT_TRAVELING ||
                                  ship->autopilot->state == AUTOPILOT_WAITING))
  {
    autopilot_stop(ship);
    send_to_ship(ship, "The vessel's autopilot has been disengaged.");
  }
  ship->anchored = TRUE;
  ship->departure_ticks = 0;
  ship->setspeed = 0;
  send_to_ship(ship, "%s drops anchor.", ship->name);
}
