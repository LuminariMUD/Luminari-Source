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

struct vessel_debug_category_entry
{
  const char *name;
  unsigned int bit;
};

static const struct vessel_debug_category_entry vessel_debug_categories[] = {
    {"core", VESSEL_DEBUG_CAT_CORE},
    {"move", VESSEL_DEBUG_CAT_MOVE},
    {"auto", VESSEL_DEBUG_CAT_AUTO},
    {"dock", VESSEL_DEBUG_CAT_DOCK},
    {"db", VESSEL_DEBUG_CAT_DB},
    {"func", VESSEL_DEBUG_CAT_FUNC},
    {"state", VESSEL_DEBUG_CAT_STATE},
    {"vehicle", VESSEL_DEBUG_CAT_VEHICLE},
    {"vehicle_move", VESSEL_DEBUG_CAT_VEHICLE_MOVE},
    {"transport", VESSEL_DEBUG_CAT_TRANSPORT},
    {NULL, 0}};

unsigned int vessel_debug_mask = 0;

bool vessel_debug_enabled(unsigned int category)
{
#if VESSEL_SYSTEM_DEBUG
  return (vessel_debug_mask & category) != 0;
#else
  (void)category;
  return false;
#endif
}

unsigned int vessel_debug_category_from_name(const char *name)
{
  int i;

  if (name == NULL || *name == '\0')
  {
    return 0;
  }
  if (!strcasecmp(name, "all"))
  {
    return VESSEL_DEBUG_CAT_ALL;
  }
  if (!strcasecmp(name, "xport"))
  {
    return VESSEL_DEBUG_CAT_TRANSPORT;
  }

  for (i = 0; vessel_debug_categories[i].name != NULL; i++)
  {
    if (!strcasecmp(name, vessel_debug_categories[i].name))
    {
      return vessel_debug_categories[i].bit;
    }
  }

  return 0;
}

static void vessel_debug_status(struct char_data *ch)
{
  int i;

#if VESSEL_SYSTEM_DEBUG
  send_to_char(ch, "Vessel debug support: compiled in; runtime mask 0x%03x.\r\n",
               vessel_debug_mask);
  for (i = 0; vessel_debug_categories[i].name != NULL; i++)
  {
    send_to_char(ch, "  %-13s %s\r\n", vessel_debug_categories[i].name,
                 vessel_debug_enabled(vessel_debug_categories[i].bit) ? "ON" : "off");
  }
#else
  (void)i;
  send_to_char(ch, "Vessel debug support: compiled out (production-safe default).\r\n");
#endif
}

/**
 * vesseldebug [status|on <category>|off [category]]
 *
 * Runtime category control is available only in an explicit development
 * build compiled with -DVESSEL_SYSTEM_DEBUG=1. Production builds retain no
 * debug call-site overhead.
 */
ACMD(do_vesseldebug)
{
  char action[MAX_INPUT_LENGTH];
  char category[MAX_INPUT_LENGTH];
  const char *remainder;

  /* "on" is a global parser fill word, so the normal one_argument helpers
   * would silently skip it. Runtime control syntax must preserve fill words. */
  remainder = any_one_arg_c(argument, action, sizeof(action));
  any_one_arg_c(remainder, category, sizeof(category));
  if (!*action || !strcasecmp(action, "status"))
  {
    vessel_debug_status(ch);
    return;
  }

#if !VESSEL_SYSTEM_DEBUG
  send_to_char(ch, "Vessel debug support is compiled out. Rebuild development with "
                   "-DVESSEL_SYSTEM_DEBUG=1.\r\n");
  return;
#else
  {
    unsigned int bit;

    if (!strcasecmp(action, "on"))
    {
      bit = vessel_debug_category_from_name(category);
      if (bit == 0)
      {
        send_to_char(ch, "Usage: vesseldebug on <core|move|auto|dock|db|func|state|"
                         "vehicle|vehicle_move|transport|all>\r\n");
        return;
      }
      vessel_debug_mask |= bit;
    }
    else if (!strcasecmp(action, "off"))
    {
      if (!*category)
      {
        vessel_debug_mask = 0;
      }
      else
      {
        bit = vessel_debug_category_from_name(category);
        if (bit == 0)
        {
          send_to_char(ch, "Unknown vessel debug category '%s'.\r\n", category);
          return;
        }
        vessel_debug_mask &= ~bit;
      }
    }
    else
    {
      send_to_char(ch, "Usage: vesseldebug [status|on <category>|off [category]]\r\n");
      return;
    }
  }

  vessel_debug_status(ch);
#endif
}

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
    if (!is_valid_ship(ship))
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
  if (!is_valid_ship(ship))
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
  unsigned char old_farmor;
  unsigned char old_rarmor;
  unsigned char old_parmor;
  unsigned char old_sarmor;
  unsigned char old_finternal;
  unsigned char old_rinternal;
  unsigned char old_pinternal;
  unsigned char old_sinternal;
  unsigned char old_mainsail;
  unsigned char old_turnrate;
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
  if (!is_valid_ship(ship))
  {
    send_to_char(ch, "Slot %d is empty.\r\n", slot);
    return;
  }

  old_farmor = ship->farmor;
  old_rarmor = ship->rarmor;
  old_parmor = ship->parmor;
  old_sarmor = ship->sarmor;
  old_finternal = ship->finternal;
  old_rinternal = ship->rinternal;
  old_pinternal = ship->pinternal;
  old_sinternal = ship->sinternal;
  old_mainsail = ship->mainsail;
  old_turnrate = ship->turnrate;

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

  if (!vessel_db_save_runtime(ship))
  {
    ship->farmor = old_farmor;
    ship->rarmor = old_rarmor;
    ship->parmor = old_parmor;
    ship->sarmor = old_sarmor;
    ship->finternal = old_finternal;
    ship->rinternal = old_rinternal;
    ship->pinternal = old_pinternal;
    ship->sinternal = old_sinternal;
    ship->mainsail = old_mainsail;
    ship->turnrate = old_turnrate;
    send_to_char(ch, "The repair could not be saved, so the prior condition was restored.\r\n");
    log("SYSERR: %s could not persist force-repair for ship %d '%s'", GET_NAME(ch), slot,
        ship->name);
    return;
  }

  send_to_char(ch, "%s (slot %d) restored to full condition.\r\n", ship->name, slot);
  send_to_ship(ship, "A divine hand mends every timber and line.");
  log("Info: %s force-repaired ship %d '%s'", GET_NAME(ch), slot, ship->name);
}

/**
 * shippurge <slot> - remove one prototype-spawned ship and all instance state.
 */
ACMD(do_shippurge)
{
  struct greyhawk_ship_data *ship;
  struct obj_data *hull;
  char arg[MAX_INPUT_LENGTH];
  char ship_name[sizeof(greyhawk_ships[0].name)];
  char *end;
  room_rnum exterior;
  long parsed_slot;
  int released;
  int reclaimed;
  int slot;
  int i;

  one_argument_u((char *)argument, arg);
  parsed_slot = strtol(arg, &end, 10);
  if (!*arg || *end != '\0' || parsed_slot < 2 || parsed_slot >= GREYHAWK_MAXSHIPS)
  {
    send_to_char(ch, "Usage: shippurge <slot 2-%d> (see 'shiplist').\r\n",
                 GREYHAWK_MAXSHIPS - 1);
    return;
  }

  slot = (int)parsed_slot;
  ship = &greyhawk_ships[slot];
  if (!is_valid_ship(ship))
  {
    send_to_char(ch, "Slot %d is empty.\r\n", slot);
    return;
  }

  if (!vessel_delete_persistence(slot))
  {
    send_to_char(ch, "Database cleanup failed; ship %d was left intact.\r\n", slot);
    return;
  }

  strlcpy(ship_name, ship->name, sizeof(ship_name));
  hull = ship->shipobj;
  exterior = hull != NULL ? IN_ROOM(hull) : NOWHERE;

  send_to_ship(ship, "%s is removing this vessel from service.", GET_NAME(ch));
  vessel_abort_docking(ship);
  released = vehicle_release_all_from_vessel(ship, exterior);
  reclaimed = vessel_reclaim_interior_rooms(ship, exterior);

  autopilot_cleanup(ship);
  if (ship->schedule != NULL)
  {
    free(ship->schedule);
    ship->schedule = NULL;
  }

  if (hull != NULL)
  {
    ship->shipobj = NULL;
    extract_obj(hull);
  }

  for (i = 0; i < GREYHAWK_MAXSHIPS; i++)
  {
    if (greyhawk_ships[i].last_attacker == slot)
    {
      greyhawk_ships[i].last_attacker = 0;
    }
    if (greyhawk_ships[i].docked_to_ship == slot)
    {
      greyhawk_ships[i].docked_to_ship = -1;
      greyhawk_ships[i].docking_room = 0;
    }
  }

  memset(ship, 0, sizeof(*ship));

  send_to_char(ch, "Purged ship %d '%s': reclaimed %d room%s and released %d vehicle%s.\r\n",
               slot, ship_name, reclaimed, reclaimed == 1 ? "" : "s", released,
               released == 1 ? "" : "s");
  log("Info: %s purged ship %d '%s' (%d rooms, %d vehicles)", GET_NAME(ch), slot, ship_name,
      reclaimed, released);
}
