/* ************************************************************************
 *      File:   vessels_admin.c                       Part of LuminariMUD  *
 *   Purpose:   Operator tooling and client protocol (Phase 09).           *
 *              Fleet overview, teleport-to-ship, forced maintenance, room *
 *              pool monitoring, and MSDP ship variables.                  *
 * ********************************************************************** */

#include "conf.h"
#include "core/sysdep.h"
#include <math.h> /* before utils.h, which defines log() as a macro */
#include "core/structs.h"
#include "core/utils.h"
#include "core/comm.h"
#include "core/db.h"
#include "core/handler.h"
#include "core/interpreter.h"
#include "vessels.h"
#include "vessel_periodic.h"
#include "wilderness/wilderness.h"
#include "net/protocol.h"
#include "act/act.h"

extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];

/* Warn operators when the shared wilderness dynamic room pool crosses this
 * utilization percentage (vessel product requirements Section 5, invariant 3). */
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
 * vesseldebug [status|on <category>|off [category]|encounter|ambient|balance|
 *              raider <tier> [hunter]]
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
  struct greyhawk_ship_data *ship;

  /* "on" is a global parser fill word, so the normal one_argument helpers
   * would silently skip it. Runtime control syntax must preserve fill words. */
  remainder = any_one_arg_c(argument, action, sizeof(action));
  any_one_arg_c(remainder, category, sizeof(category));
  if (!*action || !strcasecmp(action, "status"))
  {
    vessel_debug_status(ch);
    return;
  }
  if (!strcasecmp(action, "encounter"))
  {
    vessel_encounter_force_check();
    send_to_char(ch, "Forced the next normal vessel encounter check.\r\n");
    return;
  }
  if (!strcasecmp(action, "raider"))
  {
    char kind[MAX_INPUT_LENGTH];
    int slot;

    any_one_arg_c(any_one_arg_c(remainder, category, sizeof(category)), kind, sizeof(kind));
    ship = get_ship_from_room(IN_ROOM(ch));
    if (!is_valid_ship(ship) || ship->owner[0] == '\0')
    {
      send_to_char(ch, "Board a player's hull at sea to have raiders ambush her.\r\n");
      return;
    }
    if (strlen(category) != 1 || category[0] < '0' || category[0] >= '0' + VESSEL_RAIDER_TIERS ||
        (*kind && strcasecmp(kind, "hunter") != 0))
    {
      send_to_char(ch, "Usage: vesseldebug raider <0-%d> [hunter]\r\n", VESSEL_RAIDER_TIERS - 1);
      return;
    }
    slot = vessel_raider_spawn(ship, category[0] - '0', *kind != '\0');
    if (slot < 0)
    {
      send_to_char(ch, "No raider could be launched; see the syslog.\r\n");
      return;
    }
    send_to_char(ch, "Raider %d [%s] %s comes for %s.\r\n", slot, greyhawk_ships[slot].id,
                 greyhawk_ships[slot].name, ship->name);
    return;
  }
  if (!strcasecmp(action, "ambient"))
  {
    ship = get_ship_from_room(IN_ROOM(ch));
    if (!is_valid_ship(ship))
    {
      send_to_char(ch, "You need to be on a vessel to force its ambient message.\r\n");
      return;
    }
    if (!vessel_narrative_force_ship(ship))
    {
      send_to_char(ch, "The vessel ambient message could not be generated.\r\n");
      return;
    }
    send_to_char(ch, "Forced this vessel's contextual ambient message.\r\n");
    return;
  }
  if (!strcasecmp(action, "balance"))
  {
    int duel_count;
    long requested_duels;
    char *end;

    duel_count = VESSEL_BALANCE_DEFAULT_DUELS;
    if (*category)
    {
      end = NULL;
      requested_duels = strtol(category, &end, 10);
      if (end == category || *end != '\0' || requested_duels < 1 ||
          requested_duels > VESSEL_BALANCE_MAX_DUELS)
      {
        send_to_char(ch, "Usage: vesseldebug balance [1-%d]\r\n", VESSEL_BALANCE_MAX_DUELS);
        return;
      }
      duel_count = (int)requested_duels;
    }
    vessel_balance_report(ch, duel_count);
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
      send_to_char(
          ch, "Usage: vesseldebug [status|on <category>|off [category]|encounter|ambient|balance|"
              "raider <tier> [hunter]]\r\n");
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

/* MSDP table content for one condition by arc: each arc name holds its CURRENT and MAX. */
static void vessel_msdp_arcs(struct greyhawk_ship_data *ship,
                             unsigned char *(*current)(struct greyhawk_ship_data *, int),
                             unsigned char *(*maximum)(struct greyhawk_ship_data *, int),
                             char *buffer, size_t size)
{
  char entry[128];
  int arc;

  buffer[0] = '\0';
  for (arc = 0; arc < VESSEL_NUM_ARCS; arc++)
  {
    snprintf(entry, sizeof(entry), "%c%s%c%c%cCURRENT%c%d%cMAX%c%d%c", (char)MSDP_VAR,
             vessel_arc_name(arc), (char)MSDP_VAL, (char)MSDP_TABLE_OPEN, (char)MSDP_VAR,
             (char)MSDP_VAL, *current(ship, arc), (char)MSDP_VAR, (char)MSDP_VAL,
             *maximum(ship, arc), (char)MSDP_TABLE_CLOSE);
    strlcat(buffer, entry, size);
  }
}

/* MSDP array content: one table per mounted weapon, in slot order. READY
 * means what shipstatus calls ready: undamaged, rounds left, reloaded. */
static void vessel_msdp_weapons(const struct greyhawk_ship_data *ship, char *buffer, size_t size)
{
  const struct greyhawk_ship_slot *slot;
  const struct vessel_weapon_type *weapon;
  char entry[256];
  int i;

  buffer[0] = '\0';
  for (i = 0; i < GREYHAWK_MAXSLOTS; i++)
  {
    slot = &ship->slot[i];
    weapon = vessel_slot_weapon(slot);
    if (weapon == NULL)
    {
      continue;
    }
    snprintf(entry, sizeof(entry),
             "%c%c%cSLOT%c%d%cNAME%c%s%cARC%c%s%cAMMO%c%d%cREADY%c%d%cDAMAGE%c%d%c", (char)MSDP_VAL,
             (char)MSDP_TABLE_OPEN, (char)MSDP_VAR, (char)MSDP_VAL, i, (char)MSDP_VAR,
             (char)MSDP_VAL, weapon->name, (char)MSDP_VAR, (char)MSDP_VAL,
             vessel_arc_name(slot->position), (char)MSDP_VAR, (char)MSDP_VAL, slot->ammo,
             (char)MSDP_VAR, (char)MSDP_VAL,
             slot->damage == 0 && slot->ammo > 0 && slot->timer <= 0, (char)MSDP_VAR,
             (char)MSDP_VAL, slot->damage, (char)MSDP_TABLE_CLOSE);
    strlcat(buffer, entry, size);
  }
}

/* MSDP array content: the contacts list, one table per contact, nearest first. */
static void vessel_msdp_contacts(const struct greyhawk_ship_data *ship, char *buffer, size_t size)
{
  struct vessel_contact contacts[VESSEL_CONTACT_DISPLAY_LIMIT];
  const struct greyhawk_ship_data *contact_ship;
  char entry[512];
  int count;
  int i;

  buffer[0] = '\0';
  count = MIN(vessel_collect_contacts(ship, contacts, VESSEL_CONTACT_DISPLAY_LIMIT),
              VESSEL_CONTACT_DISPLAY_LIMIT);
  for (i = 0; i < count; i++)
  {
    contact_ship = &greyhawk_ships[contacts[i].shipnum];
    snprintf(entry, sizeof(entry), "%c%c%cID%c%s%cNAME%c%s%cRANGE%c%.1f%cBEARING%c%d%cARC%c%s%c",
             (char)MSDP_VAL, (char)MSDP_TABLE_OPEN, (char)MSDP_VAR, (char)MSDP_VAL,
             contact_ship->id, (char)MSDP_VAR, (char)MSDP_VAL,
             contact_ship->name[0] ? contact_ship->name : "Unknown Vessel", (char)MSDP_VAR,
             (char)MSDP_VAL, contacts[i].range, (char)MSDP_VAR, (char)MSDP_VAL, contacts[i].bearing,
             (char)MSDP_VAR, (char)MSDP_VAL, vessel_arc_name(vessel_arc_toward(ship, contact_ship)),
             (char)MSDP_TABLE_CLOSE);
    strlcat(buffer, entry, size);
  }
}

/**
 * Publish the character's vessel state to their client via native MSDP.
 *
 * Called from the vessel tick for every playing character. Client gauges
 * track position, heading, speed, condition by arc, sails, rudder, crew
 * stamina, the weapons, the contacts, and the lock without polling, and
 * receive an explicit empty state after the character leaves a vessel.
 */
void vessel_msdp_update(struct char_data *ch)
{
  struct greyhawk_ship_data *ship;
  struct descriptor_data *d;
  char buffer[MAX_VARIABLE_LENGTH];

  if (ch == NULL || IS_NPC(ch) || (d = ch->desc) == NULL)
  {
    return;
  }

  ship = get_ship_from_room(IN_ROOM(ch));
  if (ship == NULL)
  {
    MSDPSetString(d, eMSDP_SHIP_NAME, "");
    MSDPSetNumber(d, eMSDP_SHIP_X, 0);
    MSDPSetNumber(d, eMSDP_SHIP_Y, 0);
    MSDPSetNumber(d, eMSDP_SHIP_Z, 0);
    MSDPSetNumber(d, eMSDP_SHIP_HEADING, 0);
    MSDPSetNumber(d, eMSDP_SHIP_SPEED, 0);
    MSDPSetNumber(d, eMSDP_SHIP_HULL, 0);
    MSDPSetNumber(d, eMSDP_SHIP_HULL_MAX, 0);
    MSDPSetString(d, eMSDP_SHIP_STATUS, "");
    MSDPSetString(d, eMSDP_SHIP_ID, "");
    MSDPSetString(d, eMSDP_SHIP_TARGET, "");
    MSDPSetString(d, eMSDP_SHIP_ARMOR, "");
    MSDPSetString(d, eMSDP_SHIP_INTERNAL, "");
    MSDPSetNumber(d, eMSDP_SHIP_SAIL, 0);
    MSDPSetNumber(d, eMSDP_SHIP_SAIL_MAX, 0);
    MSDPSetNumber(d, eMSDP_SHIP_RUDDER, 0);
    MSDPSetNumber(d, eMSDP_SHIP_RUDDER_MAX, 0);
    MSDPSetNumber(d, eMSDP_SHIP_STAMINA, 0);
    MSDPSetNumber(d, eMSDP_SHIP_STAMINA_MAX, 0);
    MSDPSetString(d, eMSDP_SHIP_WEAPONS, "");
    MSDPSetString(d, eMSDP_SHIP_CONTACTS, "");
    return;
  }

  MSDPSetString(d, eMSDP_SHIP_NAME, ship->name);
  MSDPSetNumber(d, eMSDP_SHIP_X, (int)ship->x);
  MSDPSetNumber(d, eMSDP_SHIP_Y, (int)ship->y);
  MSDPSetNumber(d, eMSDP_SHIP_Z, (int)ship->z);
  MSDPSetNumber(d, eMSDP_SHIP_HEADING, vessel_display_heading(ship->heading));
  MSDPSetNumber(d, eMSDP_SHIP_SPEED, vessel_display_speed(ship->speed));
  MSDPSetNumber(d, eMSDP_SHIP_HULL, vessel_total_internal(ship));
  MSDPSetNumber(d, eMSDP_SHIP_HULL_MAX, vessel_max_internal(ship));
  MSDPSetString(d, eMSDP_SHIP_STATUS, vessel_status_name(vessel_status(ship)));
  MSDPSetString(d, eMSDP_SHIP_ID, ship->id);
  /* The gunnery tick drops a lost lock; contacts marks the lock the same way. */
  MSDPSetString(d, eMSDP_SHIP_TARGET,
                ship->lock_target != 0 ? greyhawk_ships[ship->lock_target].id : "");
  vessel_msdp_arcs(ship, vessel_arc_armor, vessel_arc_max_armor, buffer, sizeof(buffer));
  MSDPSetTable(d, eMSDP_SHIP_ARMOR, buffer);
  vessel_msdp_arcs(ship, vessel_arc_internal, vessel_arc_max_internal, buffer, sizeof(buffer));
  MSDPSetTable(d, eMSDP_SHIP_INTERNAL, buffer);
  MSDPSetNumber(d, eMSDP_SHIP_SAIL, ship->mainsail);
  MSDPSetNumber(d, eMSDP_SHIP_SAIL_MAX, ship->maxmainsail);
  MSDPSetNumber(d, eMSDP_SHIP_RUDDER, ship->turnrate);
  MSDPSetNumber(d, eMSDP_SHIP_RUDDER_MAX, ship->maxturnrate);
  MSDPSetNumber(d, eMSDP_SHIP_STAMINA, vessel_stamina_max(ship) - (int)ceil(ship->stamina_spent));
  MSDPSetNumber(d, eMSDP_SHIP_STAMINA_MAX, vessel_stamina_max(ship));
  vessel_msdp_weapons(ship, buffer, sizeof(buffer));
  MSDPSetArray(d, eMSDP_SHIP_WEAPONS, buffer);
  vessel_msdp_contacts(ship, buffer, sizeof(buffer));
  MSDPSetArray(d, eMSDP_SHIP_CONTACTS, buffer);
}

/**
 * Refresh native MSDP ship state for every playing character.
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
 * shiplist [summary] - fleet overview for operators.
 */
ACMD(do_shiplist)
{
  struct greyhawk_ship_data *ship;
  char arg[MAX_INPUT_LENGTH];
  char registry[64];
  bool summary_only;
  int listed = 0;
  int in_use = 0;
  int pool_total = 0;
  int i;

  one_argument(argument, arg, sizeof(arg));
  summary_only = !str_cmp(arg, "summary");
  if (*arg && !summary_only)
  {
    send_to_char(ch, "Usage: shiplist [summary]\r\n");
    return;
  }

  if (!summary_only)
  {
    send_to_char(ch, "Slot Name                           Class          Pos           Hdg Spd "
                     "Hull    Owner\r\n");
    send_to_char(ch, "---- ------------------------------ -------------- ------------- --- --- "
                     "------- ----------\r\n");
  }

  for (i = 0; i < GREYHAWK_MAXSHIPS; i++)
  {
    ship = &greyhawk_ships[i];
    if (!is_valid_ship(ship) && !ship->stowed)
    {
      continue;
    }

    if (!summary_only)
    {
      if (ship->owner[0] != '\0')
      {
        strlcpy(registry, ship->owner, sizeof(registry));
      }
      else if (ship->merchant_id > 0 && ship->merchant_faction_id >= FACTION_NONE &&
               ship->merchant_faction_id < NUM_FACTIONS)
      {
        snprintf(registry, sizeof(registry), "merchant/%s", factions[ship->merchant_faction_id]);
      }
      else
      {
        strlcpy(registry, "-", sizeof(registry));
      }
      if (ship->stowed)
      {
        send_to_char(ch, "%4d %-30.30s %-14.14s %-13s   -   - %3d/%-3d %s\r\n", i, ship->name,
                     get_vessel_type_name(ship->vessel_type),
                     ship->summon_due > 0 ? "summoned" : "wreck registry",
                     vessel_total_internal(ship), vessel_max_internal(ship), registry);
      }
      else
      {
        send_to_char(ch, "%4d %-30.30s %-14.14s (%5d,%5d) %3d %3d %3d/%-3d %s\r\n", i, ship->name,
                     get_vessel_type_name(ship->vessel_type), (int)ship->x, (int)ship->y,
                     vessel_display_heading(ship->heading), vessel_display_speed(ship->speed),
                     vessel_total_internal(ship), vessel_max_internal(ship), registry);
      }
    }
    listed++;
  }

  if (listed == 0 && !summary_only)
  {
    send_to_char(ch, "  (no active vessels)\r\n");
  }

  send_to_char(ch, "\r\n%d of %d active fleet slots in use.\r\n", listed,
               GREYHAWK_ACTIVE_SHIP_CAPACITY);

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

  one_argument(argument, arg, sizeof(arg));
  if (!*arg)
  {
    send_to_char(ch, "Go to which ship slot? See 'shiplist'.\r\n");
    return;
  }

  slot = parse_int(arg);
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
  int s;

  one_argument(argument, arg, sizeof(arg));
  if (!*arg)
  {
    send_to_char(ch, "Repair which ship slot? See 'shiplist'.\r\n");
    return;
  }

  slot = parse_int(arg);
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

  for (s = 0; s < GREYHAWK_MAXSLOTS; s++)
  {
    ship->slot[s].damage = 0;
  }
  ship->sink_ticks = 0;
  if (!vessel_db_save_weapons(ship))
  {
    log("SYSERR: %s could not persist the weapon repair for ship %d", GET_NAME(ch), slot);
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

  one_argument(argument, arg, sizeof(arg));
  parsed_slot = strtol(arg, &end, 10);
  if (!*arg || *end != '\0' || parsed_slot < 2 || parsed_slot >= GREYHAWK_MAXSHIPS)
  {
    send_to_char(ch, "Usage: shippurge <slot 2-%d> (see 'shiplist').\r\n", GREYHAWK_MAXSHIPS - 1);
    return;
  }

  slot = (int)parsed_slot;
  ship = &greyhawk_ships[slot];
  if (!is_valid_ship(ship) && !ship->stowed)
  {
    send_to_char(ch, "Slot %d is empty.\r\n", slot);
    return;
  }

  if (!vessel_delete_persistence(slot))
  {
    send_to_char(ch, "Database cleanup failed; ship %d was left intact.\r\n", slot);
    return;
  }

  vessel_merchant_handle_purge(ship, GET_NAME(ch));
  vessel_hunter_handle_purge(ship, GET_NAME(ch));
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

  vessel_periodic_forget(ship);
  memset(ship, 0, sizeof(*ship));

  send_to_char(ch, "Purged ship %d '%s': reclaimed %d room%s and released %d vehicle%s.\r\n", slot,
               ship_name, reclaimed, reclaimed == 1 ? "" : "s", released, released == 1 ? "" : "s");
  log("Info: %s purged ship %d '%s' (%d rooms, %d vehicles)", GET_NAME(ch), slot, ship_name,
      reclaimed, released);
}
