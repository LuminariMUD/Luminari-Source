/* ************************************************************************
 *      File:   vessels_admin.c                       Part of LuminariMUD  *
 *   Purpose:   Operator tooling and client protocol (Phase 09).           *
 *              Fleet overview, teleport-to-ship, forced maintenance, room *
 *              pool monitoring, and MSDP ship variables.                  *
 * ********************************************************************** */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "vessels.h"
#include "wilderness.h"
#include "protocol.h"
#include "act.h"

extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];
extern struct room_data *world;
extern room_rnum top_of_world;

/* Warn operators when the shared wilderness dynamic room pool crosses this
 * utilization percentage (PRD Section 4, ground rule 3). */
#define ROOM_POOL_WARN_PERCENT 80

/**
 * Count how much of the shared wilderness dynamic room pool is in use.
 *
 * The pool (WILD_DYNAMIC_ROOM_VNUM_START..END) is shared with every walker
 * in the wilderness, so vessels must not quietly exhaust it.
 *
 * @param in_use Out: occupied rooms
 * @param total Out: pool size
 */
static void wilderness_pool_usage(int *in_use, int *total)
{
  room_rnum rnum;
  int used = 0;
  int size = 0;

  for (rnum = 0; rnum <= top_of_world; rnum++)
  {
    if (world[rnum].number < WILD_DYNAMIC_ROOM_VNUM_START ||
        world[rnum].number > WILD_DYNAMIC_ROOM_VNUM_END)
    {
      continue;
    }
    size++;
    if (ROOM_FLAGGED(rnum, ROOM_OCCUPIED))
    {
      used++;
    }
  }

  *in_use = used;
  *total = size;
}

/**
 * Publish the character's vessel state to their client via MSDP.
 *
 * Called from the vessel tick for everyone aboard a ship, so client gauges
 * track position, heading, speed, and hull without polling.
 */
void vessel_msdp_update(struct char_data *ch)
{
  struct greyhawk_ship_data *ship;
  struct descriptor_data *d;

  if (ch == NULL || IS_NPC(ch) || (d = ch->desc) == NULL)
  {
    return;
  }

  ship = get_ship_from_room(IN_ROOM(ch));
  if (ship == NULL)
  {
    return;
  }

  MSDPSetString(d, eMSDP_SHIP_NAME, ship->name);
  MSDPSetNumber(d, eMSDP_SHIP_X, (int)ship->x);
  MSDPSetNumber(d, eMSDP_SHIP_Y, (int)ship->y);
  MSDPSetNumber(d, eMSDP_SHIP_Z, (int)ship->z);
  MSDPSetNumber(d, eMSDP_SHIP_HEADING, ship->heading);
  MSDPSetNumber(d, eMSDP_SHIP_SPEED, ship->speed);
  MSDPSetNumber(d, eMSDP_SHIP_HULL, vessel_total_internal(ship));
  MSDPSetNumber(d, eMSDP_SHIP_HULL_MAX, vessel_max_internal(ship));
  MSDPSetString(d, eMSDP_SHIP_STATUS, vessel_status_name(vessel_status(ship)));
}

/**
 * Push MSDP ship state to every player currently aboard a vessel.
 * Runs on the vessel tick alongside the other subsystems.
 */
void vessel_msdp_tick(void)
{
  struct descriptor_data *d;

  for (d = descriptor_list; d; d = d->next)
  {
    if (STATE(d) != CON_PLAYING || d->character == NULL)
    {
      continue;
    }
    vessel_msdp_update(d->character);
  }
}

/**
 * shiplist - fleet overview for operators.
 */
ACMD(do_shiplist)
{
  struct greyhawk_ship_data *ship;
  int listed = 0;
  int in_use = 0;
  int pool_total = 0;
  int i;

  send_to_char(ch, "Slot Name                      Class      Pos           Hdg Spd Hull    "
                   "Owner\r\n");
  send_to_char(ch, "---- ------------------------- ---------- ------------- --- --- ------- "
                   "----------\r\n");

  for (i = 0; i < GREYHAWK_MAXSHIPS; i++)
  {
    ship = &greyhawk_ships[i];
    if (ship->name[0] == '\0')
    {
      continue;
    }

    send_to_char(ch, "%4d %-25.25s %-10.10s (%5d,%5d) %3d %3d %3d/%-3d %s\r\n", i, ship->name,
                 get_vessel_type_name(ship->vessel_type), (int)ship->x, (int)ship->y, ship->heading,
                 ship->speed, vessel_total_internal(ship), vessel_max_internal(ship),
                 ship->owner[0] ? ship->owner : "-");
    listed++;
  }

  if (listed == 0)
  {
    send_to_char(ch, "  (no active vessels)\r\n");
  }

  send_to_char(ch, "\r\n%d of %d fleet slots in use.\r\n", listed, GREYHAWK_MAXSHIPS);

  wilderness_pool_usage(&in_use, &pool_total);
  if (pool_total > 0)
  {
    int percent = in_use * 100 / pool_total;

    send_to_char(ch, "Wilderness dynamic room pool: %d/%d occupied (%d%%)%s\r\n", in_use,
                 pool_total, percent,
                 percent >= ROOM_POOL_WARN_PERCENT ? "  *** PRESSURE ***" : "");
    if (percent >= ROOM_POOL_WARN_PERCENT)
    {
      send_to_char(ch, "  The pool is shared with every wilderness traveller. At exhaustion, "
                       "ship movement degrades to reusing the nearest room.\r\n");
    }
  }
}

/**
 * shipgoto <slot> - teleport to a ship's bridge (or its wilderness room).
 */
ACMD(do_shipgoto)
{
  struct greyhawk_ship_data *ship;
  char arg[MAX_INPUT_LENGTH];
  room_rnum target = NOWHERE;
  int slot;

  one_argument_u((char *)argument, arg);
  if (!*arg)
  {
    send_to_char(ch, "Go to which ship slot? See 'shiplist'.\r\n");
    return;
  }

  slot = atoi(arg);
  if (slot < 0 || slot >= GREYHAWK_MAXSHIPS)
  {
    send_to_char(ch, "Ship slots run 0-%d.\r\n", GREYHAWK_MAXSHIPS - 1);
    return;
  }

  ship = &greyhawk_ships[slot];
  if (ship->name[0] == '\0')
  {
    send_to_char(ch, "Slot %d is empty.\r\n", slot);
    return;
  }

  /* Prefer the bridge; fall back to the water the ship floats in */
  if (ship->bridge_room > 0)
  {
    target = real_room(ship->bridge_room);
  }
  if (target == NOWHERE && ship->shipobj != NULL)
  {
    target = IN_ROOM(ship->shipobj);
  }

  if (target == NOWHERE)
  {
    send_to_char(ch, "%s has no reachable rooms - it may be mid-generation.\r\n", ship->name);
    return;
  }

  act("$n vanishes in a nautical whirl.", TRUE, ch, 0, 0, TO_ROOM);
  char_from_room(ch);
  char_to_room(ch, target);
  act("$n appears, dripping seawater.", TRUE, ch, 0, 0, TO_ROOM);
  look_at_room(ch, 0);
  send_to_char(ch, "Aboard %s (slot %d).\r\n", ship->name, slot);
}

/**
 * shipfix <slot> - operator repair: restore a ship to full condition.
 */
ACMD(do_shipfix)
{
  struct greyhawk_ship_data *ship;
  char arg[MAX_INPUT_LENGTH];
  int slot;

  one_argument_u((char *)argument, arg);
  if (!*arg)
  {
    send_to_char(ch, "Repair which ship slot? See 'shiplist'.\r\n");
    return;
  }

  slot = atoi(arg);
  if (slot < 0 || slot >= GREYHAWK_MAXSHIPS)
  {
    send_to_char(ch, "Ship slots run 0-%d.\r\n", GREYHAWK_MAXSHIPS - 1);
    return;
  }

  ship = &greyhawk_ships[slot];
  if (ship->name[0] == '\0')
  {
    send_to_char(ch, "Slot %d is empty.\r\n", slot);
    return;
  }

  ship->farmor = ship->maxfarmor;
  ship->rarmor = ship->maxrarmor;
  ship->parmor = ship->maxparmor;
  ship->sarmor = ship->maxsarmor;
  ship->finternal = ship->maxfinternal;
  ship->rinternal = ship->maxrinternal;
  ship->pinternal = ship->maxpinternal;
  ship->sinternal = ship->maxsinternal;
  ship->mainsail = ship->maxmainsail;
  ship->turnrate = ship->maxturnrate;

  send_to_char(ch, "%s (slot %d) restored to full condition.\r\n", ship->name, slot);
  send_to_ship(ship, "A divine hand mends every timber and line.");
  log("Info: %s force-repaired ship %d '%s'", GET_NAME(ch), slot, ship->name);
}
