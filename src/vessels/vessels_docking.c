/* ************************************************************************
 *      File:   vessels_docking.c                    Part of LuminariMUD  *
 *   Purpose:   Phase 2 Ship docking and boarding mechanics               *
 *  Author:     Zusuk                                                     *
 * ********************************************************************** */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "movement/door_state.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "vessels.h"
#include "act.h"
#include "character/abilities.h"
#include "combat/fight.h"
#include "magic/spells.h"

/* External variables */
extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];
extern struct room_data *world;

/* Docking constants */
#define MAX_DOCKING_RANGE 2.0 /* Maximum distance for docking */
#define MAX_DOCKING_SPEED 2   /* Maximum speed for safe docking */
#define BOARDING_CRITICAL_MARGIN 10
#define BOARDING_SWIM_DC 15
#define BOARDING_DEFENSE_MIN -8
#define BOARDING_DEFENSE_MAX 15
#define DIR_GANGWAY 10 /* Special direction for ship connections */

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
  int rev;

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
  rev = rev_dir[dir];
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
  struct door_state_operation operations[NUM_OF_DIRS * 2] = {0};
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

  for (dir = 0; dir < NUM_OF_DIRS; dir++)
  {
    door_state_begin(&operations[dir], room1, dir, false, DOMAIN_DOOR_EDIT);
    door_state_begin(&operations[NUM_OF_DIRS + dir], room2, dir, false, DOMAIN_DOOR_EDIT);
  }

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
  for (dir = 0; dir < NUM_OF_DIRS * 2; dir++)
    door_state_finish(&operations[dir]);
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
  int i;

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

  /*
   * Docked ships occupy the same wilderness cell. Use the canonical movement
   * helper so the target's exterior object, coordinates, and location remain
   * synchronized.
   */
  if (!update_ship_wilderness_position(ship2->shipnum, (int)ship1->x, (int)ship1->y, (int)ship1->z))
  {
    VSSL_DEBUG_DOCK("DOCKING FAILED: Could not synchronize exterior positions");
    send_to_ship(ship1, "Docking failed - the target could not move alongside!");
    return;
  }

  /* Find an available direction for the gangway */
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

  VSSL_DEBUG_DOCK("Separating %s from %s in wilderness cell (%d,%d)", ship1->name, ship2->name,
                  (int)ship1->x, (int)ship1->y);

  /*
   * Removing the gangway does not itself sail either vessel away. Both hulls
   * remain alongside in the same wilderness cell until a helm moves one.
   */
  update_ship_room_coordinates(ship1);
  update_ship_room_coordinates(ship2);

  VSSL_DEBUG_DOCK("Vessels separated successfully");
  VSSL_DEBUG_EXIT("separate_vessels");
}

/**
 * Tear down any active gangway involving a ship leaving service.
 *
 * Unlike the player `undock` command this performs no movement and succeeds
 * even when one side of the persisted relationship is already invalid.
 */
void vessel_abort_docking(struct greyhawk_ship_data *ship)
{
  struct greyhawk_ship_data *other;
  room_rnum dock1;
  room_rnum dock2;

  if (ship == NULL || ship->docked_to_ship < 0)
  {
    return;
  }

  other = get_ship_by_id(ship->docked_to_ship);
  if (other != NULL)
  {
    dock1 = real_room(ship->docking_room);
    dock2 = real_room(other->docking_room);
    if (dock1 != NOWHERE && dock2 != NOWHERE)
    {
      remove_ship_connection(dock1, dock2);
    }

    end_docking_record(ship, other);
    other->docked_to_ship = -1;
    other->docking_room = 0;
    send_to_ship(other, "%s is removed from service; the gangway is withdrawn.", ship->name);
  }

  ship->docked_to_ship = -1;
  ship->docking_room = 0;
}

/**
 * Return the target-vessel modifier for one hostile boarding stage.
 *
 * Character training is resolved separately in the opposed Boarding check.
 * This modifier covers only the hull, its motion, condition, and hired crew.
 */
int vessel_boarding_defense_modifier(const struct greyhawk_ship_data *target,
                                     enum vessel_boarding_stage stage)
{
  int modifier = 0;
  int maximum_structure;
  int structure_percent;
  int speed;
  int crew_tier;

  if (target == NULL)
  {
    return 0;
  }

  switch (target->vessel_type)
  {
  case VESSEL_RAFT:
    modifier -= 4;
    break;
  case VESSEL_BOAT:
    modifier -= 2;
    break;
  case VESSEL_WARSHIP:
    modifier += 4;
    break;
  case VESSEL_TRANSPORT:
    modifier -= 2;
    break;
  case VESSEL_AIRSHIP:
    modifier += 2;
    break;
  case VESSEL_SUBMARINE:
  case VESSEL_MAGICAL:
    modifier += 3;
    break;
  default:
    break;
  }

  maximum_structure = vessel_max_internal(target);
  if (maximum_structure > 0)
  {
    structure_percent = vessel_total_internal(target) * 100 / maximum_structure;
    if (structure_percent < 25)
      modifier -= 6;
    else if (structure_percent < 50)
      modifier -= 4;
    else if (structure_percent < 75)
      modifier -= 2;
  }

  speed = MAX(0, (int)target->speed);
  if (stage == VESSEL_BOARDING_GRAPPLE)
  {
    modifier += MIN(6, speed);
    crew_tier = MAX(CREW_TIER_NONE, MIN(CREW_TIER_VETERAN, target->crew_tier[CREW_SAILMASTER]));
    modifier += crew_tier * 2;
  }
  else if (stage == VESSEL_BOARDING_CROSSING)
  {
    modifier += MIN(3, (speed + 1) / 2);
    crew_tier = MAX(CREW_TIER_NONE, MIN(CREW_TIER_VETERAN, target->crew_tier[CREW_BOSUN]));
    modifier += crew_tier * 2;
  }
  else
  {
    return 0;
  }

  return MAX(BOARDING_DEFENSE_MIN, MIN(BOARDING_DEFENSE_MAX, modifier));
}

/** Resolve one d20-plus-Boarding opposed check. Ties favor the defender. */
bool vessel_resolve_boarding_contest(int attacker_skill, int attacker_roll, int defender_skill,
                                     int defender_roll, int vessel_modifier,
                                     struct vessel_boarding_contest *result)
{
  if (result == NULL || attacker_roll < 1 || defender_roll < 1)
  {
    return FALSE;
  }

  memset(result, 0, sizeof(*result));
  result->attacker_skill = attacker_skill;
  result->attacker_roll = attacker_roll;
  result->defender_skill = defender_skill;
  result->defender_roll = defender_roll;
  result->vessel_modifier = vessel_modifier;
  result->attacker_total = attacker_skill + attacker_roll;
  result->defender_total = defender_skill + defender_roll + vessel_modifier;
  result->attacker_wins = result->attacker_total > result->defender_total;
  result->critical_failure =
      !result->attacker_wins &&
      (attacker_roll == 1 ||
       result->defender_total - result->attacker_total >= BOARDING_CRITICAL_MARGIN);

  return TRUE;
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

  if (ch_ship == target)
  {
    VSSL_DEBUG_DOCK("BOARDING CHECK FAILED: Target is character's own ship");
    send_to_char(ch, "You are already aboard that vessel!\r\n");
    return FALSE;
  }

  /* Ships must be in range */
  if (!ships_in_docking_range(ch_ship, target))
  {
    VSSL_DEBUG_DOCK("BOARDING CHECK FAILED: Target out of range");
    send_to_char(ch, "The target vessel is too far away!\r\n");
    return FALSE;
  }

  if (ch_ship->docked_to_ship >= 0)
  {
    VSSL_DEBUG_DOCK("BOARDING CHECK FAILED: Attacker already docked");
    send_to_char(ch, "You must undock before launching a hostile boarding action!\r\n");
    return FALSE;
  }

  if (target->docked_to_ship >= 0)
  {
    VSSL_DEBUG_DOCK("BOARDING CHECK FAILED: Target already docked");
    send_to_char(ch, "That vessel is already secured alongside another hull.\r\n");
    return FALSE;
  }

  VSSL_DEBUG_DOCK("Boarding check passed for %s", GET_NAME(ch));
  return TRUE;
}

/* Perform combat boarding */
bool perform_combat_boarding(struct char_data *ch, struct greyhawk_ship_data *target)
{
  room_rnum target_room;
  struct char_data *vict;
  int defenders_found = 0;

  VSSL_DEBUG_ENTER("perform_combat_boarding");

  if (!ch || !target)
  {
    VSSL_DEBUG_DOCK("perform_combat_boarding: NULL pointer");
    return FALSE;
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
    return FALSE;
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

  /* Start combat with defenders.
   *
   * Each defender is checked individually: a passenger who has not enabled
   * PVP is not dragged into a fight just for standing on the deck, even
   * when the ship's owner is a willing combatant. */
  for (vict = world[target_room].people; vict; vict = vict->next_in_room)
  {
    if (vict == ch || GET_POS(vict) <= POS_STUNNED)
    {
      continue;
    }

    if (!IS_NPC(vict) && !pvp_ok(ch, vict, FALSE))
    {
      VSSL_DEBUG_DOCK("Skipping non-consenting defender: %s", GET_NAME(vict));
      send_to_char(vict, "Boarders swarm aboard, but pay you no heed.\r\n");
      continue;
    }

    defenders_found++;
    VSSL_DEBUG_DOCK("Defender found: %s", GET_NAME(vict));
    if (!FIGHTING(vict))
    {
      set_fighting(vict, ch);
      if (!IS_NPC(vict))
      {
        send_to_char(vict, "You are under attack by boarders!\r\n");
      }
    }
    if (!FIGHTING(ch))
    {
      set_fighting(ch, vict);
    }
  }

  VSSL_DEBUG_DOCK("Combat initiated with %d defender(s)", defenders_found);
  VSSL_DEBUG_EXIT("perform_combat_boarding");
  return TRUE;
}

/* Setup boarding defenses */
void setup_boarding_defenses(struct greyhawk_ship_data *ship)
{
  struct char_data *mob;
  struct char_data *next_mob;
  room_rnum entrance;
  room_rnum bridge;
  room_rnum rnum;
  room_rnum dest;
  int defenders_moved = 0;
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

  /* Position idle NPC crew at the boarding chokepoints: alternate between
   * the entrance (primary breach point) and the bridge (capture objective). */
  entrance = real_room(ship->entrance_room);
  bridge = real_room(ship->bridge_room);
  if (entrance != NOWHERE || bridge != NOWHERE)
  {
    for (i = 0; i < ship->num_rooms && i < MAX_SHIP_ROOMS; i++)
    {
      rnum = real_room(ship->room_vnums[i]);
      if (rnum == NOWHERE || rnum == entrance || rnum == bridge)
        continue;

      for (mob = world[rnum].people; mob; mob = next_mob)
      {
        next_mob = mob->next_in_room;
        if (!IS_NPC(mob) || FIGHTING(mob))
          continue;

        if (bridge == NOWHERE || (entrance != NOWHERE && defenders_moved % 2 == 0))
          dest = entrance;
        else
          dest = bridge;

        act("$n rushes off to repel the boarders!", TRUE, mob, 0, 0, TO_ROOM);
        char_from_room(mob);
        char_to_room(mob, dest);
        act("$n takes up a defensive position!", TRUE, mob, 0, 0, TO_ROOM);
        defenders_moved++;
      }
    }
  }

  VSSL_DEBUG_DOCK("Positioned %d defender(s) at chokepoints", defenders_moved);
  VSSL_DEBUG_DOCK("Boarding defenses activated");
  VSSL_DEBUG_EXIT("setup_boarding_defenses");
}

/** Find the strongest conscious, consenting defender anywhere aboard. */
static struct char_data *vessel_best_boarding_defender(struct char_data *attacker,
                                                       struct greyhawk_ship_data *target,
                                                       int *best_skill)
{
  struct char_data *best = NULL;
  struct char_data *vict;
  room_rnum room;
  int candidate_skill;
  int i;

  if (best_skill != NULL)
  {
    *best_skill = 0;
  }
  if (attacker == NULL || target == NULL || best_skill == NULL)
  {
    return NULL;
  }

  for (i = 0; i < target->num_rooms && i < MAX_SHIP_ROOMS; i++)
  {
    room = real_room(target->room_vnums[i]);
    if (room == NOWHERE)
    {
      continue;
    }

    for (vict = world[room].people; vict; vict = vict->next_in_room)
    {
      if (vict == attacker || GET_POS(vict) <= POS_STUNNED)
      {
        continue;
      }
      if (!IS_NPC(vict) && !pvp_ok(attacker, vict, FALSE))
      {
        continue;
      }

      candidate_skill = compute_ability(vict, ABILITY_BOARDING);
      if (candidate_skill >= 0 && (best == NULL || candidate_skill > *best_skill))
      {
        best = vict;
        *best_skill = candidate_skill;
      }
    }
  }

  return best;
}

/** Show both sides of one opposed boarding roll to the attacker. */
static void vessel_report_boarding_contest(struct char_data *ch, struct greyhawk_ship_data *target,
                                           struct char_data *defender, const char *stage_name,
                                           const struct vessel_boarding_contest *contest)
{
  const char *defender_name;

  if (ch == NULL || target == NULL || stage_name == NULL || contest == NULL)
  {
    return;
  }

  defender_name = defender != NULL ? GET_NAME(defender) : target->name;
  send_to_char(ch,
               "%s contest: Boarding %d + d20 %d = %d; %s Boarding %d + d20 %d "
               "+ vessel modifier (%+d) = %d. %s\r\n",
               stage_name, contest->attacker_skill, contest->attacker_roll, contest->attacker_total,
               defender_name, contest->defender_skill, contest->defender_roll,
               contest->vessel_modifier, contest->defender_total,
               contest->attacker_wins ? "SUCCESS" : "FAILURE");
}

/** Apply the critical crossing-failure water and Athletics consequence. */
static void vessel_boarding_fall_into_water(struct char_data *ch)
{
  struct greyhawk_ship_data *ch_ship;
  room_rnum water_room = NOWHERE;
  int swim_roll;
  int swim_value;

  if (ch == NULL)
  {
    return;
  }

  send_to_char(ch, "The grappling line snaps taut and throws you into the water!\r\n");
  act("$n is torn from the line and falls into the water!", TRUE, ch, 0, 0, TO_ROOM);

  ch_ship = get_ship_from_room(IN_ROOM(ch));
  if (ch_ship != NULL && ch_ship->shipobj != NULL && IN_ROOM(ch_ship->shipobj) != NOWHERE)
  {
    water_room = IN_ROOM(ch_ship->shipobj);
  }

  if (water_room != NOWHERE)
  {
    char_from_room(ch);
    char_to_room(ch, water_room);
    act("$n plunges into the water beside the ship!", TRUE, ch, 0, 0, TO_ROOM);
    look_at_room(ch, 0);
  }

  swim_roll = d20(ch);
  swim_value = compute_ability(ch, ABILITY_SWIM);
  send_to_char(ch, "Swimming: Athletics Skill (%d) + d20 roll (%d) = Total (%d) vs. DC (%d)\r\n",
               swim_value, swim_roll, swim_value + swim_roll, BOARDING_SWIM_DC);
  if (swim_roll + swim_value < BOARDING_SWIM_DC)
  {
    send_to_char(ch, "You flounder in the water, battered against the hull!\r\n");
    damage(ch, ch, dice(2, 6), TYPE_UNDEFINED, DAM_FORCE, FALSE);
  }
  else
  {
    send_to_char(ch, "You manage to stay afloat.\r\n");
  }
}

/* COMMAND: Dock with another vessel */
ACMD(do_dock)
{
  struct greyhawk_ship_data *ship, *target;
  char arg[MAX_INPUT_LENGTH];
  bool found;
  int i;

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
    found = FALSE;

    for (i = 0; i < GREYHAWK_MAXSHIPS; i++)
    {
      if (is_valid_ship(&greyhawk_ships[i]) && &greyhawk_ships[i] != ship)
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

  if (target == ship)
  {
    send_to_char(ch, "You cannot dock a vessel with itself.\r\n");
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
  struct char_data *defender;
  struct vessel_boarding_contest grapple_contest;
  struct vessel_boarding_contest crossing_contest;
  char arg[MAX_INPUT_LENGTH];
  int attacker_skill;
  int defender_skill;
  int attacker_roll;
  int defender_roll;
  int vessel_modifier;

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

  /* Hostile boarding forces combat on whoever is aboard, so it answers to
   * the same consent rules as any other PvP action. */
  if (!vessel_pvp_permitted(ch, target, TRUE))
  {
    return;
  }

  WAIT_STATE(ch, PULSE_VIOLENCE * 2);
  send_to_ship(target, "WARNING: %s is attempting a hostile boarding!", GET_NAME(ch));
  setup_boarding_defenses(target);

  attacker_skill = MAX(0, compute_ability(ch, ABILITY_BOARDING));
  defender = vessel_best_boarding_defender(ch, target, &defender_skill);

  attacker_roll = d20(ch);
  defender_roll = d20(defender);
  vessel_modifier = vessel_boarding_defense_modifier(target, VESSEL_BOARDING_GRAPPLE);
  if (!vessel_resolve_boarding_contest(attacker_skill, attacker_roll, defender_skill, defender_roll,
                                       vessel_modifier, &grapple_contest))
  {
    log("SYSERR: Unable to resolve hostile boarding grapple contest");
    send_to_char(ch, "The boarding attempt cannot be resolved.\r\n");
    return;
  }
  vessel_report_boarding_contest(ch, target, defender, "Grapple", &grapple_contest);

  if (!grapple_contest.attacker_wins)
  {
    send_to_char(ch, "The defenders cast off your grappling lines before they take hold.\r\n");
    act("$n's grappling lines fall short of the enemy vessel.", TRUE, ch, 0, 0, TO_ROOM);
    send_to_ship(target, "%s's grappling lines are repelled.", GET_NAME(ch));
    return;
  }

  send_to_char(ch, "Your grappling lines bite home; you commit to the crossing!\r\n");
  act("$n's grappling lines bite into the enemy vessel!", TRUE, ch, 0, 0, TO_ROOM);
  send_to_ship(target, "%s's grappling lines bite home!", GET_NAME(ch));

  attacker_roll = d20(ch);
  defender_roll = d20(defender);
  vessel_modifier = vessel_boarding_defense_modifier(target, VESSEL_BOARDING_CROSSING);
  if (!vessel_resolve_boarding_contest(attacker_skill, attacker_roll, defender_skill, defender_roll,
                                       vessel_modifier, &crossing_contest))
  {
    log("SYSERR: Unable to resolve hostile boarding crossing contest");
    send_to_char(ch, "The boarding attempt cannot be resolved.\r\n");
    return;
  }
  vessel_report_boarding_contest(ch, target, defender, "Crossing", &crossing_contest);

  if (!crossing_contest.attacker_wins)
  {
    send_to_char(ch, "The defenders drive you back and the grappling lines are cut!\r\n");
    act("$n is driven back from the enemy vessel!", TRUE, ch, 0, 0, TO_ROOM);
    send_to_ship(target, "%s is driven back from the rail.", GET_NAME(ch));
    if (crossing_contest.critical_failure)
    {
      vessel_boarding_fall_into_water(ch);
    }
    return;
  }

  if (perform_combat_boarding(ch, target))
  {
    send_to_char(ch, "You haul yourself across and breach the enemy vessel!\r\n");
    send_to_ship(target, "WARNING: Hostile boarders have breached the vessel!");
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
