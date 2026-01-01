/* ************************************************************************
 *      File:   vessels_docking.c                    Part of LuminariMUD  *
 *   Purpose:   Phase 2 Ship docking and boarding mechanics               *
 *  Author:     Zusuk                                                     *
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
#include "act.h"
#include "fight.h"
#include "spells.h"

/* External variables */
extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];
extern struct room_data *world;

/* Docking constants */
#define MAX_DOCKING_RANGE 2.0  /* Maximum distance for docking */
#define MAX_DOCKING_SPEED 2    /* Maximum speed for safe docking */
#define BOARDING_DIFFICULTY 15 /* Base difficulty for hostile boarding */
#define DIR_GANGWAY 10         /* Special direction for ship connections */

/* Weather display thresholds (matching vessels.c/wilderness system) */
#define VESSEL_WEATHER_CLEAR_MAX 127
#define VESSEL_WEATHER_CLOUDY_MAX 177
#define VESSEL_WEATHER_RAIN_MAX 199
#define VESSEL_WEATHER_STORM_MAX 224

/* External function declarations */
extern int get_weather(int x, int y);

/**
 * Convert raw weather value (0-255) to descriptive string.
 *
 * @param weather_val Raw weather value from get_weather()
 * @return Pointer to static weather description string
 */
static const char *get_weather_desc_string(int weather_val)
{
  if (weather_val <= VESSEL_WEATHER_CLEAR_MAX)
    return "Clear skies";
  else if (weather_val <= VESSEL_WEATHER_CLOUDY_MAX)
    return "Overcast and cloudy";
  else if (weather_val <= VESSEL_WEATHER_RAIN_MAX)
    return "Light rain falling";
  else if (weather_val <= VESSEL_WEATHER_STORM_MAX)
    return "Heavy storm conditions";
  else
    return "Thunderstorm with lightning";
}

/* Check if two ships are in docking range */
bool ships_in_docking_range(struct greyhawk_ship_data *ship1, struct greyhawk_ship_data *ship2)
{
  float distance;

  VSSL_DEBUG_ENTER("ships_in_docking_range");

  if (!ship1 || !ship2)
  {
    VSSL_DEBUG_DOCK("Range check failed: NULL ship pointer (ship1=%p, ship2=%p)", (void *)ship1,
                    (void *)ship2);
    return FALSE;
  }

  /* Calculate distance between ships */
  distance = greyhawk_range(ship1->x, ship1->y, ship1->z, ship2->x, ship2->y, ship2->z);

  VSSL_DEBUG_DOCK("Range check: %s (%.1f,%.1f) to %s (%.1f,%.1f) = %.2f (max %.1f)", ship1->name,
                  ship1->x, ship1->y, ship2->name, ship2->x, ship2->y, distance, MAX_DOCKING_RANGE);

  return (distance <= MAX_DOCKING_RANGE);
}

/* Find a suitable docking room on a ship */
room_rnum find_docking_room(struct greyhawk_ship_data *ship)
{
  room_rnum room;
  int i;

  VSSL_DEBUG_ENTER("find_docking_room");

  if (!ship)
  {
    VSSL_DEBUG_DOCK("find_docking_room: NULL ship pointer");
    return NOWHERE;
  }

  VSSL_DEBUG_DOCK("Finding dock room for %s (%d rooms, entrance=%d, bridge=%d)", ship->name,
                  ship->num_rooms, ship->entrance_room, ship->bridge_room);

  /* Prefer airlock if available */
  for (i = 0; i < ship->num_rooms; i++)
  {
    room = real_room(ship->room_vnums[i]);
    if (room != NOWHERE)
    {
      if (strstr(world[room].name, "Airlock"))
      {
        VSSL_DEBUG_DOCK("Found airlock at vnum %d (rnum %d)", ship->room_vnums[i], room);
        return room;
      }
    }
  }

  /* Use entrance room if set */
  if (ship->entrance_room > 0)
  {
    room = real_room(ship->entrance_room);
    if (room != NOWHERE)
    {
      VSSL_DEBUG_DOCK("Using entrance room vnum %d (rnum %d)", ship->entrance_room, room);
      return room;
    }
  }

  /* Use deck if available */
  for (i = 0; i < ship->num_rooms; i++)
  {
    room = real_room(ship->room_vnums[i]);
    if (room != NOWHERE)
    {
      if (strstr(world[room].name, "Deck"))
      {
        VSSL_DEBUG_DOCK("Found deck at vnum %d (rnum %d)", ship->room_vnums[i], room);
        return room;
      }
    }
  }

  /* Default to first room that isn't the bridge */
  for (i = 0; i < ship->num_rooms; i++)
  {
    if (ship->room_vnums[i] != ship->bridge_room)
    {
      room = real_room(ship->room_vnums[i]);
      if (room != NOWHERE)
      {
        VSSL_DEBUG_DOCK("Using non-bridge room vnum %d (rnum %d)", ship->room_vnums[i], room);
        return room;
      }
    }
  }

  /* Last resort: use bridge */
  room = real_room(ship->bridge_room);
  VSSL_DEBUG_DOCK("Last resort: using bridge vnum %d (rnum %d)", ship->bridge_room, room);
  return room;
}

/* Create a connection between two ship rooms */
void create_ship_connection(room_rnum room1, room_rnum room2, int dir)
{
  struct room_direction_data *exit;

  VSSL_DEBUG_ENTER("create_ship_connection");

  if (room1 == NOWHERE || room2 == NOWHERE)
  {
    VSSL_DEBUG_DOCK("create_ship_connection: invalid rooms (room1=%d, room2=%d)", room1, room2);
    return;
  }

  VSSL_DEBUG_DOCK("Creating connection: room %d <-> room %d (dir %d/%s)", world[room1].number,
                  world[room2].number, dir, dirs[dir]);

  /* Create exit from room1 to room2 */
  if (world[room1].dir_option[dir] == NULL)
  {
    CREATE(exit, struct room_direction_data, 1);
    exit->to_room = room2;
    exit->exit_info = 0;
    exit->keyword = strdup("gangway plank");
    exit->general_description = strdup("A gangway connects to the other vessel.");
    world[room1].dir_option[dir] = exit;
    VSSL_DEBUG_DOCK("Created exit %s from room %d to room %d", dirs[dir], world[room1].number,
                    world[room2].number);
  }

  /* Create return exit from room2 to room1 */
  int rev = rev_dir[dir];
  if (world[room2].dir_option[rev] == NULL)
  {
    CREATE(exit, struct room_direction_data, 1);
    exit->to_room = room1;
    exit->exit_info = 0;
    exit->keyword = strdup("gangway plank");
    exit->general_description = strdup("A gangway connects to the other vessel.");
    world[room2].dir_option[rev] = exit;
    VSSL_DEBUG_DOCK("Created return exit %s from room %d to room %d", dirs[rev],
                    world[room2].number, world[room1].number);
  }

  VSSL_DEBUG_EXIT("create_ship_connection");
}

/* Remove a connection between two ship rooms */
void remove_ship_connection(room_rnum room1, room_rnum room2)
{
  int dir;
  int removed_count = 0;

  VSSL_DEBUG_ENTER("remove_ship_connection");

  if (room1 == NOWHERE || room2 == NOWHERE)
  {
    VSSL_DEBUG_DOCK("remove_ship_connection: invalid rooms (room1=%d, room2=%d)", room1, room2);
    return;
  }

  VSSL_DEBUG_DOCK("Removing connections between room %d and room %d", world[room1].number,
                  world[room2].number);

  /* Find and remove all connections between these rooms */
  for (dir = 0; dir < NUM_OF_DIRS; dir++)
  {
    if (world[room1].dir_option[dir])
    {
      if (world[room1].dir_option[dir]->to_room == room2)
      {
        VSSL_DEBUG_DOCK("Removing exit %s from room %d", dirs[dir], world[room1].number);
        /* Free the exit data */
        if (world[room1].dir_option[dir]->keyword)
          free(world[room1].dir_option[dir]->keyword);
        if (world[room1].dir_option[dir]->general_description)
          free(world[room1].dir_option[dir]->general_description);
        free(world[room1].dir_option[dir]);
        world[room1].dir_option[dir] = NULL;
        removed_count++;
      }
    }

    if (world[room2].dir_option[dir])
    {
      if (world[room2].dir_option[dir]->to_room == room1)
      {
        VSSL_DEBUG_DOCK("Removing exit %s from room %d", dirs[dir], world[room2].number);
        /* Free the exit data */
        if (world[room2].dir_option[dir]->keyword)
          free(world[room2].dir_option[dir]->keyword);
        if (world[room2].dir_option[dir]->general_description)
          free(world[room2].dir_option[dir]->general_description);
        free(world[room2].dir_option[dir]);
        world[room2].dir_option[dir] = NULL;
        removed_count++;
      }
    }
  }

  VSSL_DEBUG_DOCK("Removed %d connection(s)", removed_count);
  VSSL_DEBUG_EXIT("remove_ship_connection");
}

/* Initiate docking between two ships */
void initiate_docking(struct greyhawk_ship_data *ship1, struct greyhawk_ship_data *ship2)
{
  VSSL_DEBUG_ENTER("initiate_docking");

  if (!ship1 || !ship2)
  {
    VSSL_DEBUG_DOCK("initiate_docking: NULL ship pointer (ship1=%p, ship2=%p)", (void *)ship1,
                    (void *)ship2);
    return;
  }

  VSSL_DEBUG_DOCK("=== DOCKING ATTEMPT: %s (%d) -> %s (%d) ===", ship1->name, ship1->shipnum,
                  ship2->name, ship2->shipnum);
  VSSL_DEBUG_DOCK("Ship1 pos: (%.1f,%.1f,%.1f) speed=%d docked_to=%d", ship1->x, ship1->y, ship1->z,
                  ship1->speed, ship1->docked_to_ship);
  VSSL_DEBUG_DOCK("Ship2 pos: (%.1f,%.1f,%.1f) speed=%d docked_to=%d", ship2->x, ship2->y, ship2->z,
                  ship2->speed, ship2->docked_to_ship);

  /* Ships must be close enough */
  if (!ships_in_docking_range(ship1, ship2))
  {
    VSSL_DEBUG_DOCK("DOCKING FAILED: Ships out of range");
    send_to_ship(ship1, "Target vessel is too far away for docking!");
    return;
  }

  /* Ships must be moving slowly */
  if (ship1->speed > MAX_DOCKING_SPEED || ship2->speed > MAX_DOCKING_SPEED)
  {
    VSSL_DEBUG_DOCK("DOCKING FAILED: Speed too high (ship1=%d, ship2=%d, max=%d)", ship1->speed,
                    ship2->speed, MAX_DOCKING_SPEED);
    send_to_ship(ship1, "Ships must be nearly stationary to dock!");
    return;
  }

  /* Check if either ship is already docked */
  if (ship1->docked_to_ship >= 0)
  {
    VSSL_DEBUG_DOCK("DOCKING FAILED: Ship1 already docked to ship %d", ship1->docked_to_ship);
    send_to_ship(ship1, "You must undock from your current vessel first!");
    return;
  }

  if (ship2->docked_to_ship >= 0)
  {
    VSSL_DEBUG_DOCK("DOCKING FAILED: Ship2 already docked to ship %d", ship2->docked_to_ship);
    send_to_ship(ship1, "Target vessel is already docked to another ship!");
    return;
  }

  VSSL_DEBUG_DOCK("Pre-conditions passed, proceeding to complete_docking");

  /* Proceed with docking */
  complete_docking(ship1, ship2);
}

/* Complete the docking process */
void complete_docking(struct greyhawk_ship_data *ship1, struct greyhawk_ship_data *ship2)
{
  room_rnum dock1, dock2;
  int dir;

  VSSL_DEBUG_ENTER("complete_docking");

  if (!ship1 || !ship2)
  {
    VSSL_DEBUG_DOCK("complete_docking: NULL ship pointer");
    return;
  }

  /* Find docking rooms */
  dock1 = find_docking_room(ship1);
  dock2 = find_docking_room(ship2);

  VSSL_DEBUG_DOCK("Docking rooms found: dock1=%d, dock2=%d", dock1, dock2);

  if (dock1 == NOWHERE || dock2 == NOWHERE)
  {
    VSSL_DEBUG_DOCK("DOCKING FAILED: Missing docking rooms (dock1=%d, dock2=%d)", dock1, dock2);
    log("SYSERR: Ships lack docking rooms!");
    send_to_ship(ship1, "Docking failed - no suitable connection points!");
    return;
  }

  /* Find an available direction for the gangway */
  int i;
  dir = -1;
  for (i = 0; i < NUM_OF_DIRS; i++)
  {
    if (world[dock1].dir_option[i] == NULL && world[dock2].dir_option[rev_dir[i]] == NULL)
    {
      dir = i;
      VSSL_DEBUG_DOCK("Found available direction: %d (%s)", dir, dirs[dir]);
      break;
    }
  }

  if (dir == -1)
  {
    VSSL_DEBUG_DOCK("DOCKING FAILED: No available exit direction");
    send_to_ship(ship1, "No available connection point for docking!");
    return;
  }

  /* Create bidirectional connection */
  create_ship_connection(dock1, dock2, dir);

  /* Update ship states */
  VSSL_DEBUG_DOCK("Updating ship states: ship1->docked_to=%d, ship2->docked_to=%d", ship2->shipnum,
                  ship1->shipnum);
  ship1->docked_to_ship = ship2->shipnum;
  ship1->docking_room = world[dock1].number;
  ship2->docked_to_ship = ship1->shipnum;
  ship2->docking_room = world[dock2].number;

  /* Match ship positions exactly */
  VSSL_DEBUG_DOCK("Position sync: ship2 (%.1f,%.1f) -> (%.1f,%.1f)", ship2->x, ship2->y,
                  ship1->x + 0.5, ship1->y);
  ship2->x = ship1->x + 0.5;
  ship2->y = ship1->y;
  ship2->z = ship1->z;

  /* Stop both ships */
  ship1->speed = 0;
  ship1->setspeed = 0;
  ship2->speed = 0;
  ship2->setspeed = 0;
  VSSL_DEBUG_DOCK("Both ships stopped");

  /* Notify crews */
  send_to_ship(ship1, "Docking complete with %s.", ship2->name);
  send_to_ship(ship2, "Docking complete with %s.", ship1->name);

  /* Log the event */
  log("Ships docked: %s (%d) <-> %s (%d)", ship1->name, ship1->shipnum, ship2->name,
      ship2->shipnum);
  VSSL_DEBUG_DOCK("=== DOCKING COMPLETE: %s <-> %s ===", ship1->name, ship2->name);

  /* Save docking record to database */
  save_docking_record(ship1, ship2, "standard");

  VSSL_DEBUG_EXIT("complete_docking");
}

/* Separate two vessels after undocking */
void separate_vessels(struct greyhawk_ship_data *ship1, struct greyhawk_ship_data *ship2)
{
  VSSL_DEBUG_ENTER("separate_vessels");

  if (!ship1 || !ship2)
  {
    VSSL_DEBUG_DOCK("separate_vessels: NULL ship pointer");
    return;
  }

  VSSL_DEBUG_DOCK("Separating %s from %s", ship1->name, ship2->name);
  VSSL_DEBUG_DOCK("Ship2 position: (%.1f,%.1f) -> (%.1f,%.1f)", ship2->x, ship2->y, ship1->x + 3.0,
                  ship1->y + 1.0);

  /* Move ships slightly apart */
  ship2->x = ship1->x + 3.0;
  ship2->y = ship1->y + 1.0;

  /* Update room coordinates */
  update_ship_room_coordinates(ship1);
  update_ship_room_coordinates(ship2);

  VSSL_DEBUG_DOCK("Vessels separated successfully");
  VSSL_DEBUG_EXIT("separate_vessels");
}

/* Calculate boarding difficulty */
int calculate_boarding_difficulty(struct greyhawk_ship_data *target)
{
  int difficulty = BOARDING_DIFFICULTY;
  int result;

  VSSL_DEBUG_ENTER("calculate_boarding_difficulty");

  if (!target)
  {
    VSSL_DEBUG_DOCK("calculate_boarding_difficulty: NULL target");
    return 99; /* Impossible */
  }

  VSSL_DEBUG_DOCK("Calculating boarding difficulty for %s (base=%d)", target->name,
                  BOARDING_DIFFICULTY);

  /* Adjust for ship speed */
  difficulty += target->speed * 2;
  VSSL_DEBUG_DOCK("  Speed modifier: +%d (speed=%d)", target->speed * 2, target->speed);

  /* Adjust for ship type */
  switch (target->vessel_type)
  {
  case VESSEL_WARSHIP:
    difficulty += 10;
    VSSL_DEBUG_DOCK("  Vessel type modifier: +10 (warship)");
    break;
  case VESSEL_TRANSPORT:
    difficulty -= 5;
    VSSL_DEBUG_DOCK("  Vessel type modifier: -5 (transport)");
    break;
  case VESSEL_RAFT:
    difficulty -= 10;
    VSSL_DEBUG_DOCK("  Vessel type modifier: -10 (raft)");
    break;
  default:
    VSSL_DEBUG_DOCK("  Vessel type modifier: 0 (type=%d)", target->vessel_type);
    break;
  }

  /* Adjust for damage */
  int damage_percent =
      100 - ((target->farmor + target->rarmor + target->parmor + target->sarmor) * 100 /
             (target->maxfarmor + target->maxrarmor + target->maxparmor + target->maxsarmor));

  if (damage_percent > 50)
  {
    difficulty -= 10;
    VSSL_DEBUG_DOCK("  Damage modifier: -10 (damage=%d%%)", damage_percent);
  }
  else if (damage_percent > 25)
  {
    difficulty -= 5;
    VSSL_DEBUG_DOCK("  Damage modifier: -5 (damage=%d%%)", damage_percent);
  }
  else
  {
    VSSL_DEBUG_DOCK("  Damage modifier: 0 (damage=%d%%)", damage_percent);
  }

  result = MAX(5, MIN(95, difficulty));
  VSSL_DEBUG_DOCK("Final boarding difficulty: %d (raw=%d)", result, difficulty);

  return result;
}

/* Check if character can attempt boarding */
bool can_attempt_boarding(struct char_data *ch, struct greyhawk_ship_data *target)
{
  struct greyhawk_ship_data *ch_ship;

  VSSL_DEBUG_ENTER("can_attempt_boarding");

  if (!ch || !target)
  {
    VSSL_DEBUG_DOCK("can_attempt_boarding: NULL pointer (ch=%p, target=%p)", (void *)ch,
                    (void *)target);
    return FALSE;
  }

  VSSL_DEBUG_DOCK("Checking boarding eligibility: %s -> %s", GET_NAME(ch), target->name);

  /* Must be on a ship */
  ch_ship = get_ship_from_room(IN_ROOM(ch));
  if (!ch_ship)
  {
    VSSL_DEBUG_DOCK("BOARDING CHECK FAILED: %s not on a ship (room %d)", GET_NAME(ch),
                    GET_ROOM_VNUM(IN_ROOM(ch)));
    send_to_char(ch, "You must be on a ship to board another vessel!\r\n");
    return FALSE;
  }

  VSSL_DEBUG_DOCK("Player on ship: %s (%d)", ch_ship->name, ch_ship->shipnum);

  /* Ships must be in range */
  if (!ships_in_docking_range(ch_ship, target))
  {
    VSSL_DEBUG_DOCK("BOARDING CHECK FAILED: Target out of range");
    send_to_char(ch, "The target vessel is too far away!\r\n");
    return FALSE;
  }

  /* Can't board allied ships this way */
  if (ch_ship->docked_to_ship == target->shipnum)
  {
    VSSL_DEBUG_DOCK("BOARDING CHECK FAILED: Already docked with target");
    send_to_char(ch, "You're already docked with that vessel!\r\n");
    return FALSE;
  }

  VSSL_DEBUG_DOCK("Boarding check passed for %s", GET_NAME(ch));
  return TRUE;
}

/* Perform combat boarding */
void perform_combat_boarding(struct char_data *ch, struct greyhawk_ship_data *target)
{
  room_rnum target_room;
  struct char_data *vict;
  int defenders_found = 0;

  VSSL_DEBUG_ENTER("perform_combat_boarding");

  if (!ch || !target)
  {
    VSSL_DEBUG_DOCK("perform_combat_boarding: NULL pointer");
    return;
  }

  VSSL_DEBUG_DOCK("=== COMBAT BOARDING: %s -> %s ===", GET_NAME(ch), target->name);

  /* Find entry point on target ship */
  target_room = find_docking_room(target);
  if (target_room == NOWHERE)
  {
    VSSL_DEBUG_DOCK("No docking room found, trying bridge");
    target_room = real_room(target->bridge_room);
  }

  if (target_room == NOWHERE)
  {
    VSSL_DEBUG_DOCK("BOARDING FAILED: No entry point found");
    send_to_char(ch, "You can't find a way onto that vessel!\r\n");
    return;
  }

  VSSL_DEBUG_DOCK("Entry point: room %d (%s)", GET_ROOM_VNUM(target_room), world[target_room].name);

  /* Move character to target ship */
  VSSL_DEBUG_DOCK("Moving %s from room %d to room %d", GET_NAME(ch), GET_ROOM_VNUM(IN_ROOM(ch)),
                  GET_ROOM_VNUM(target_room));
  act("$n leaps aboard the enemy vessel!", TRUE, ch, 0, 0, TO_ROOM);
  char_from_room(ch);
  char_to_room(ch, target_room);
  act("$n boards the ship with weapons drawn!", TRUE, ch, 0, 0, TO_ROOM);

  look_at_room(ch, 0);

  /* Start combat with defenders */
  for (vict = world[target_room].people; vict; vict = vict->next_in_room)
  {
    if (vict != ch && !IS_NPC(vict))
    {
      defenders_found++;
      VSSL_DEBUG_DOCK("Defender found: %s", GET_NAME(vict));
      if (!FIGHTING(vict))
      {
        set_fighting(vict, ch);
        send_to_char(vict, "You are under attack by boarders!\r\n");
      }
      if (!FIGHTING(ch))
      {
        set_fighting(ch, vict);
      }
    }
  }

  VSSL_DEBUG_DOCK("Combat initiated with %d defender(s)", defenders_found);
  VSSL_DEBUG_EXIT("perform_combat_boarding");
}

/* Setup boarding defenses */
void setup_boarding_defenses(struct greyhawk_ship_data *ship)
{
  int i;
  int hatches_sealed = 0;

  VSSL_DEBUG_ENTER("setup_boarding_defenses");

  if (!ship)
  {
    VSSL_DEBUG_DOCK("setup_boarding_defenses: NULL ship pointer");
    return;
  }

  VSSL_DEBUG_DOCK("Setting up boarding defenses for %s (%d connections)", ship->name,
                  ship->num_connections);

  /* Seal all hatches */
  for (i = 0; i < ship->num_connections; i++)
  {
    if (ship->connections[i].is_hatch)
    {
      ship->connections[i].is_locked = TRUE;
      hatches_sealed++;
    }
  }

  VSSL_DEBUG_DOCK("Sealed %d hatches", hatches_sealed);

  /* Alert crew */
  send_to_ship(ship, "BATTLE STATIONS! Prepare to repel boarders!");

  VSSL_DEBUG_DOCK("Boarding defenses activated");
  VSSL_DEBUG_EXIT("setup_boarding_defenses");

  /* TODO: Position NPC defenders at key points */
  /* TODO: Activate anti-boarding measures if available */
}

/* COMMAND: Dock with another vessel */
ACMD(do_dock)
{
  struct greyhawk_ship_data *ship, *target;
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg, sizeof(arg));

  /* Get player's ship */
  ship = get_ship_from_room(IN_ROOM(ch));
  if (!ship)
  {
    send_to_char(ch, "You must be on a vessel to dock.\r\n");
    return;
  }

  /* Check if player is at helm */
  if (!is_pilot(ch, ship))
  {
    send_to_char(ch, "You must be at the helm to control docking.\r\n");
    return;
  }

  /* No argument - show nearby vessels */
  if (!*arg)
  {
    send_to_char(ch, "Vessels within docking range:\r\n");
    bool found = FALSE;
    int i;

    for (i = 0; i < GREYHAWK_MAXSHIPS; i++)
    {
      if (greyhawk_ships[i].shipnum > 0 && &greyhawk_ships[i] != ship)
      {
        if (ships_in_docking_range(ship, &greyhawk_ships[i]))
        {
          send_to_char(ch, "  %s (ID: %s)\r\n", greyhawk_ships[i].name, greyhawk_ships[i].id);
          found = TRUE;
        }
      }
    }

    if (!found)
    {
      send_to_char(ch, "  No vessels in range.\r\n");
    }
    return;
  }

  /* Find target vessel */
  target = find_ship_by_name(arg);
  if (!target)
  {
    send_to_char(ch, "No vessel by that name is nearby.\r\n");
    return;
  }

  /* Attempt docking */
  initiate_docking(ship, target);
}

/* COMMAND: Undock from vessel */
ACMD(do_undock)
{
  struct greyhawk_ship_data *ship, *docked;
  room_rnum dock1, dock2;

  /* Get player's ship */
  ship = get_ship_from_room(IN_ROOM(ch));
  if (!ship)
  {
    send_to_char(ch, "You must be on a vessel to undock.\r\n");
    return;
  }

  /* Check if player is at helm */
  if (!is_pilot(ch, ship))
  {
    send_to_char(ch, "You must be at the helm to control undocking.\r\n");
    return;
  }

  /* Check if docked */
  if (ship->docked_to_ship < 0)
  {
    send_to_char(ch, "Your vessel is not docked.\r\n");
    return;
  }

  /* Get docked ship */
  docked = get_ship_by_id(ship->docked_to_ship);
  if (!docked)
  {
    /* Clean up invalid docking state */
    ship->docked_to_ship = -1;
    ship->docking_room = 0;
    send_to_char(ch, "Docking records cleaned.\r\n");
    return;
  }

  /* Find docking rooms */
  dock1 = real_room(ship->docking_room);
  dock2 = real_room(docked->docking_room);

  /* Remove connections */
  if (dock1 != NOWHERE && dock2 != NOWHERE)
  {
    remove_ship_connection(dock1, dock2);
  }

  /* Update states */
  ship->docked_to_ship = -1;
  ship->docking_room = 0;
  docked->docked_to_ship = -1;
  docked->docking_room = 0;

  /* Separate vessels */
  separate_vessels(ship, docked);

  /* Mark docking record as completed in database */
  end_docking_record(ship, docked);

  /* Notifications */
  send_to_ship(ship, "Undocking complete.");
  send_to_ship(docked, "%s has undocked.", ship->name);

  send_to_char(ch, "You have successfully undocked from %s.\r\n", docked->name);
}

/* COMMAND: Hostile boarding */
ACMD(do_board_hostile)
{
  struct greyhawk_ship_data *target;
  char arg[MAX_INPUT_LENGTH];
  int skill, difficulty;

  one_argument(argument, arg, sizeof(arg));

  if (!*arg)
  {
    send_to_char(ch, "Board which vessel?\r\n");
    return;
  }

  /* Find target vessel */
  target = find_ship_by_name(arg);
  if (!target)
  {
    send_to_char(ch, "No vessel by that name is nearby.\r\n");
    return;
  }

  /* Check if can attempt boarding */
  if (!can_attempt_boarding(ch, target))
  {
    return;
  }

  /* Calculate difficulty */
  difficulty = calculate_boarding_difficulty(target);

  /* For now, use level as skill (TODO: add proper boarding skill) */
  skill = GET_LEVEL(ch);

  /* Attempt boarding */
  if (rand_number(1, 100) <= (skill * 100 / difficulty))
  {
    /* Success */
    send_to_char(ch, "You leap across to the enemy vessel!\r\n");
    perform_combat_boarding(ch, target);

    /* Alert target ship */
    send_to_ship(target, "WARNING: Hostile boarders detected!");
    setup_boarding_defenses(target);
  }
  else
  {
    /* Failure */
    send_to_char(ch, "You fail to board the enemy vessel!\r\n");
    act("$n attempts to board the enemy ship but fails!", TRUE, ch, 0, 0, TO_ROOM);

    /* Critical failure - fall in water */
    if (rand_number(1, 100) <= 10)
    {
      send_to_char(ch, "You lose your footing and fall into the water!\r\n");
      act("$n falls into the water with a splash!", TRUE, ch, 0, 0, TO_ROOM);

      /* TODO: Move character to water room or apply swimming */
      damage(ch, ch, dice(2, 6), TYPE_UNDEFINED, DAM_FORCE, FALSE);
    }
  }
}

/* COMMAND: Look outside from ship interior */
ACMD(do_look_outside)
{
  struct greyhawk_ship_data *ship;
  int terrain_type;
  int weather_val;

  /* Must be on a ship */
  if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_VEHICLE))
  {
    send_to_char(ch, "You need to be on a vessel to look outside.\r\n");
    return;
  }

  ship = get_ship_from_room(IN_ROOM(ch));
  if (!ship)
  {
    send_to_char(ch, "You can't determine your vessel's position.\r\n");
    return;
  }

  /* Check if room has outside view */
  if (!room_has_outside_view(IN_ROOM(ch)))
  {
    send_to_char(ch, "You can't see outside from here.\r\n");
    return;
  }

  /* Show position and surroundings */
  send_to_char(ch, "\r\nLooking outside:\r\n");
  send_to_char(ch, "================\r\n");

  /* Position */
  send_to_char(ch, "Position: [%d, %d] Altitude: %d\r\n", (int)ship->x, (int)ship->y, (int)ship->z);

  /* Terrain */
  terrain_type = get_ship_terrain_type(ship->shipnum);
  send_to_char(ch, "Terrain: %s\r\n", sector_types[terrain_type]);

  /* Weather - integrated with wilderness weather system */
  weather_val = get_weather((int)ship->x, (int)ship->y);
  send_to_char(ch, "Weather: %s\r\n", get_weather_desc_string(weather_val));

  /* Nearby vessels */
  send_to_char(ch, "\r\nNearby vessels:\r\n");
  bool found = FALSE;
  int i;

  for (i = 0; i < GREYHAWK_MAXSHIPS; i++)
  {
    if (greyhawk_ships[i].shipnum > 0 && &greyhawk_ships[i] != ship)
    {
      float range = greyhawk_range(ship->x, ship->y, ship->z, greyhawk_ships[i].x,
                                   greyhawk_ships[i].y, greyhawk_ships[i].z);
      if (range <= 50.0)
      {
        int bearing = greyhawk_bearing(ship->x, ship->y, greyhawk_ships[i].x, greyhawk_ships[i].y);
        send_to_char(ch, "  %s - bearing %d degrees, range %.1f\r\n", greyhawk_ships[i].name,
                     bearing, range);
        found = TRUE;
      }
    }
  }

  if (!found)
  {
    send_to_char(ch, "  No vessels in sight.\r\n");
  }
}

/* COMMAND: List ship interior rooms */
ACMD(do_ship_rooms)
{
  struct greyhawk_ship_data *ship;
  room_rnum room;
  int i;

  ship = get_ship_from_room(IN_ROOM(ch));
  if (!ship)
  {
    send_to_char(ch, "You must be on a vessel to see its layout.\r\n");
    return;
  }

  send_to_char(ch, "Interior layout of %s:\r\n", ship->name);
  send_to_char(ch, "==================================\r\n");

  /* Handle edge case: ship has no interior rooms */
  if (ship->num_rooms == 0)
  {
    send_to_char(ch, "  This vessel has no interior rooms.\r\n");
    return;
  }

  for (i = 0; i < ship->num_rooms; i++)
  {
    room = real_room(ship->room_vnums[i]);
    if (room != NOWHERE)
    {
      send_to_char(ch, "%2d. %s", i + 1, world[room].name);

      /* Mark special rooms */
      if (ship->room_vnums[i] == ship->bridge_room)
      {
        send_to_char(ch, " [BRIDGE]");
      }
      if (ship->room_vnums[i] == ship->entrance_room)
      {
        send_to_char(ch, " [ENTRANCE]");
      }
      if (real_room(ship->room_vnums[i]) == IN_ROOM(ch))
      {
        send_to_char(ch, " [YOU ARE HERE]");
      }

      send_to_char(ch, "\r\n");
    }
  }

  /* Show vessel type info */
  send_to_char(ch, "\r\nVessel Type: %s (%d rooms)\r\n", get_vessel_type_name(ship->vessel_type),
               ship->num_rooms);

  if (ship->docked_to_ship >= 0)
  {
    struct greyhawk_ship_data *docked = get_ship_by_id(ship->docked_to_ship);
    if (docked)
    {
      send_to_char(ch, "Docked with: %s\r\n", docked->name);
    }
  }
}
