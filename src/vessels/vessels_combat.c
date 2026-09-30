/* ************************************************************************
 *      File:   vessels_combat.c                      Part of LuminariMUD  *
 *   Purpose:   Naval combat (Phase 05): hostile-act consent, the combat   *
 *              tick, sinking, repair, and capture. Gunnery lives in       *
 *              vessels_gunnery.c and the damage model in vessels_damage.c *
 * ********************************************************************** */

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
#include "vessel_periodic.h"
#include "wilderness/wilderness.h"
#include "core/constants.h"
#include "act/act.h"

extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];

/* Repair amounts per shiprepair invocation (dockside pace lands in the
 * Phase 06 economy; this is the slow at-sea patch job). */
#define SHIP_REPAIR_ARMOR 5
#define SHIP_REPAIR_INTERNAL 2
#define SHIP_REPAIR_SUBSYS 5

/**
 * Find an online player by exact name.
 *
 * @return The character, or NULL if not currently in the game
 */
static struct char_data *vessel_find_online_player(const char *name)
{
  struct char_data *tch;

  if (name == NULL || !*name)
  {
    return NULL;
  }

  for (tch = character_list; tch; tch = tch->next)
  {
    if (!IS_NPC(tch) && GET_NAME(tch) != NULL && !str_cmp(GET_NAME(tch), name))
    {
      return tch;
    }
  }

  return NULL;
}

/**
 * Resolve the player responsible for a hostile vessel action.
 */
static struct char_data *vessel_effective_aggressor(struct char_data *ch)
{
  if (ch != NULL && IS_NPC(ch) && ch->master != NULL && !IS_NPC(ch->master))
  {
    return ch->master;
  }
  return ch;
}

/**
 * Does a stored logout-grace record cover this exact aggressor now?
 */
bool vessel_pvp_grace_active(const struct greyhawk_ship_data *target, const char *attacker_name,
                             time_t now)
{
  if (target == NULL || attacker_name == NULL || !*attacker_name ||
      target->pvp_grace_attacker[0] == '\0')
  {
    return FALSE;
  }
  return target->pvp_grace_until >= now && !str_cmp(target->pvp_grace_attacker, attacker_name);
}

/**
 * Invalidate consent inherited from a previous owner or opponent.
 */
void vessel_clear_pvp_grace(struct greyhawk_ship_data *ship)
{
  if (ship == NULL)
  {
    return;
  }
  ship->pvp_grace_until = 0;
  ship->pvp_grace_attacker[0] = '\0';
}

/**
 * Snapshot both sides of a consented vessel engagement.
 */
static void vessel_record_pvp_engagement(struct char_data *ch, struct greyhawk_ship_data *target)
{
  struct char_data *aggressor;
  struct greyhawk_ship_data *aggressor_ship;
  time_t until;

  aggressor = vessel_effective_aggressor(ch);
  if (aggressor == NULL || IS_NPC(aggressor) || GET_NAME(aggressor) == NULL || target == NULL ||
      target->owner[0] == '\0')
  {
    return;
  }

  until = time(0) + VESSEL_PVP_LOGOUT_GRACE;
  target->pvp_grace_until = until;
  strlcpy(target->pvp_grace_attacker, GET_NAME(aggressor), sizeof(target->pvp_grace_attacker));

  aggressor_ship = get_ship_from_room(IN_ROOM(aggressor));
  if (aggressor_ship != NULL && aggressor_ship != target)
  {
    aggressor_ship->pvp_grace_until = until;
    strlcpy(aggressor_ship->pvp_grace_attacker, target->owner,
            sizeof(aggressor_ship->pvp_grace_attacker));
    vessel_db_save_runtime(aggressor_ship);
  }
  vessel_db_save_runtime(target);
}

/**
 * The consent half of vessel_pvp_permitted(), with no side effects.
 *
 * @param engaged Set TRUE when both players consented live, which starts an
 *        engagement the caller should record
 */
static bool vessel_pvp_consented(struct char_data *ch, struct greyhawk_ship_data *target,
                                 bool display, bool *engaged)
{
  struct char_data *aggressor;
  struct char_data *owner;

  *engaged = FALSE;
  if (ch == NULL || target == NULL)
  {
    return FALSE; /* Fail closed */
  }

  /* Nobody's property, nobody to wrong */
  if (target->owner[0] == '\0')
  {
    return TRUE;
  }

  /* Your own hull */
  if (!IS_NPC(ch) && !str_cmp(target->owner, GET_NAME(ch)))
  {
    return TRUE;
  }

  /* Staff need to be able to test and intervene */
  if (!IS_NPC(ch) && GET_LEVEL(ch) >= LVL_IMMORT)
  {
    return TRUE;
  }

  /* NPC crews acting under a player master answer to that master's flag;
   * ownerless NPC aggression (navy, monsters) is PvE and always allowed. */
  if (IS_NPC(ch) && (ch->master == NULL || IS_NPC(ch->master)))
  {
    return TRUE;
  }

  owner = vessel_find_online_player(target->owner);
  if (owner == NULL)
  {
    aggressor = vessel_effective_aggressor(ch);
    if (aggressor != NULL && !IS_NPC(aggressor) && CONFIG_PK_ALLOWED &&
        pvp_ok_single(aggressor, FALSE) &&
        vessel_pvp_grace_active(target, GET_NAME(aggressor), time(0)))
    {
      if (display)
      {
        send_to_char(aggressor,
                     "%s's owner has left, but your consented engagement remains "
                     "active for a short time.\r\n",
                     target->name);
      }
      return TRUE;
    }

    if (display)
    {
      send_to_char(ch,
                   "%s belongs to %s, who is not here to answer for her. You leave "
                   "her be.\r\n",
                   target->name, target->owner);
    }
    return FALSE;
  }

  aggressor = vessel_effective_aggressor(ch);
  *engaged = pvp_ok(aggressor, owner, display);
  return *engaged;
}

/**
 * May this character take a hostile action against this vessel?
 *
 * Ship-level aggression (gunfire, plunder, hostile boarding) can destroy
 * another player's property, drown their crew, and take their cargo, so it
 * must answer to the same consent rules as any other PvP action. This
 * routes the ship's owner through pvp_ok(), which requires both parties to
 * have PVP enabled (arena excepted) when pk_allowed is on, and forbids PvP
 * outright when it is off.
 *
 * Unowned hulls (test vessels, unclaimed NPC ferries) are fair game - there
 * is no player behind them. An owner who is not logged in cannot consent,
 * so their ship is protected while they are away.
 *
 * @param ch The aggressor
 * @param target The vessel being acted against
 * @param display TRUE to explain the refusal to ch
 * @return TRUE if the action is permitted
 */
bool vessel_pvp_permitted(struct char_data *ch, struct greyhawk_ship_data *target, bool display)
{
  bool engaged;

  if (!vessel_pvp_consented(ch, target, display, &engaged))
  {
    return FALSE;
  }
  if (engaged)
  {
    vessel_record_pvp_engagement(ch, target);
  }
  return TRUE;
}

/**
 * May this character work the ship's guns?
 *
 * Gunnery answers to the owner, the owner's helm permit holders, members of
 * the online owner's group, and staff. Passengers cannot turn a hull's
 * weapons, and unowned hulls fire only through their NPC crews
 * (vessel_npc_return_fire() in vessels_gunnery.c).
 */
bool vessel_gunnery_permitted(struct char_data *ch, const struct greyhawk_ship_data *ship)
{
  struct char_data *owner;
  int i;

  if (ch == NULL || ship == NULL || IS_NPC(ch))
  {
    return FALSE;
  }
  if (GET_LEVEL(ch) >= LVL_IMMORT)
  {
    return TRUE;
  }
  if (ship->owner[0] == '\0')
  {
    return FALSE;
  }
  if (!str_cmp(ship->owner, GET_NAME(ch)))
  {
    return TRUE;
  }
  for (i = 0; i < ship->num_permits && i < MAX_HELM_PERMITS; i++)
  {
    if (!str_cmp(ship->helm_permits[i], GET_NAME(ch)))
    {
      return TRUE;
    }
  }

  owner = vessel_find_online_player(ship->owner);
  return owner != NULL && GROUP(ch) != NULL && GROUP(ch) == GROUP(owner);
}

/**
 * Does a hull's online owner consent to her fighting target?
 *
 * With the target's owner online, both must consent as for any PvP. Once
 * that owner has logged out, the gunner passed only on their own logout
 * grace, so the hull owner need only still be PvP-enabled.
 */
static bool vessel_hull_owner_consents(const struct greyhawk_ship_data *ship,
                                       struct greyhawk_ship_data *target)
{
  struct char_data *owner;
  bool engaged;

  owner = vessel_find_online_player(ship->owner);
  if (owner == NULL)
  {
    return FALSE;
  }
  if (vessel_find_online_player(target->owner) == NULL)
  {
    return CONFIG_PK_ALLOWED && pvp_ok_single(owner, FALSE);
  }
  return vessel_pvp_consented(owner, target, FALSE, &engaged);
}

/**
 * May ch turn this hull's guns on target?
 *
 * The gunner must pass the consent gate, and so must the hull's owner when
 * someone else fires her: a hull fights only with its owner's consent, so
 * the target can always answer in kind. A permitted shot records one
 * engagement for the gunner, so call this after every other firing check.
 */
bool vessel_fire_permitted(struct char_data *ch, struct greyhawk_ship_data *ship,
                           struct greyhawk_ship_data *target, bool display)
{
  bool engaged;

  if (ch == NULL || ship == NULL || target == NULL)
  {
    return FALSE;
  }
  if (!vessel_pvp_consented(ch, target, display, &engaged))
  {
    return FALSE;
  }
  if (target->owner[0] != '\0' && ship->owner[0] != '\0' && !IS_NPC(ch) &&
      str_cmp(ship->owner, GET_NAME(ch)) != 0 && GET_LEVEL(ch) < LVL_IMMORT &&
      !vessel_hull_owner_consents(ship, target))
  {
    if (display)
    {
      send_to_char(ch, "%s's owner, %s, has not consented to fight %s. The guns stay silent.\r\n",
                   ship->name, ship->owner, target->name);
    }
    return FALSE;
  }

  if (engaged)
  {
    vessel_record_pvp_engagement(ch, target);
  }
  return TRUE;
}

/**
 * Sum of a ship's current internal structure across all four sections.
 */
int vessel_total_internal(const struct greyhawk_ship_data *ship)
{
  if (ship == NULL)
  {
    return 0;
  }
  return (int)ship->finternal + (int)ship->rinternal + (int)ship->pinternal + (int)ship->sinternal;
}

/**
 * Sum of a ship's maximum internal structure across all four sections.
 */
int vessel_max_internal(const struct greyhawk_ship_data *ship)
{
  if (ship == NULL)
  {
    return 0;
  }
  return (int)ship->maxfinternal + (int)ship->maxrinternal + (int)ship->maxpinternal +
         (int)ship->maxsinternal;
}

/**
 * Derive the ship's damage status band from remaining internal structure.
 *
 * @return VESSEL_STATUS_* value
 */
int vessel_status(const struct greyhawk_ship_data *ship)
{
  int max = vessel_max_internal(ship);
  int cur = vessel_total_internal(ship);
  int pct;

  if (vessel_is_sinking(ship))
  {
    return VESSEL_STATUS_SINKING;
  }

  if (max <= 0)
  {
    return VESSEL_STATUS_SOUND; /* No damage model data - treat as sound */
  }

  pct = cur * 100 / max;
  if (pct > 70)
  {
    return VESSEL_STATUS_SOUND;
  }
  if (pct > 35)
  {
    return VESSEL_STATUS_BATTERED;
  }
  return VESSEL_STATUS_CRIPPLED;
}

/**
 * Human-readable status band name.
 */
const char *vessel_status_name(int status)
{
  switch (status)
  {
  case VESSEL_STATUS_SOUND:
    return "sound";
  case VESSEL_STATUS_BATTERED:
    return "battered";
  case VESSEL_STATUS_CRIPPLED:
    return "crippled";
  case VESSEL_STATUS_SINKING:
    return "sinking";
  default:
    return "unknown";
  }
}

/**
 * The arc of `from` that `to` lies off: the bearing between their exact
 * positions, relative to from's heading, in the Duris arcs
 * (vessel_arc_for_relative_bearing()).
 *
 * @return GREYHAWK_FORE, GREYHAWK_STARBOARD, GREYHAWK_REAR, or GREYHAWK_PORT
 */
int vessel_arc_toward(const struct greyhawk_ship_data *from, const struct greyhawk_ship_data *to)
{
  return vessel_arc_for_relative_bearing(
      (int)lround(vessel_bearing_between(from, to) - from->heading));
}

/** The hull that sank ship: her last attacker, while still afloat. */
static struct greyhawk_ship_data *vessel_sink_victor(const struct greyhawk_ship_data *ship)
{
  struct greyhawk_ship_data *victor;

  if (ship->last_attacker <= 0 || ship->last_attacker >= GREYHAWK_MAXSHIPS ||
      ship->last_attacker == ship->shipnum)
  {
    return NULL;
  }
  victor = &greyhawk_ships[ship->last_attacker];
  return is_valid_ship(victor) ? victor : NULL;
}

/**
 * Sink a ship: evacuate everyone aboard into the water, convert the ship
 * object into inert wreckage, and free the fleet slot.
 */
void vessel_sink(int shipnum)
{
  struct greyhawk_ship_data *ship;
  struct greyhawk_ship_data *victor;
  struct char_data *tch;
  struct char_data *next_tch;
  room_rnum interior;
  room_rnum water_room = NOWHERE;
  char buf[MAX_STRING_LENGTH];
  int i;

  if (shipnum < 0 || shipnum >= GREYHAWK_MAXSHIPS || !is_valid_ship(&greyhawk_ships[shipnum]))
  {
    return;
  }
  ship = &greyhawk_ships[shipnum];

  log("Info: Ship %d '%s' is sinking at (%d,%d)", shipnum, ship->name, (int)ship->x, (int)ship->y);
  send_to_ship(ship, "The hull gives way - %s is SINKING!", ship->name);
  victor = vessel_sink_victor(ship);
  vessel_crew_credit_kill(victor, ship);
  vessel_event_handle_sink(shipnum);

  /* Merchant definitions outlive their killable hulls. Record the responsible
   * player and schedule replacement while identity, cargo, and geography are
   * still available. */
  vessel_hunter_handle_sink(ship);
  vessel_merchant_handle_sink(ship);

  if (ship->shipobj != NULL && IN_ROOM(ship->shipobj) != NOWHERE)
  {
    water_room = IN_ROOM(ship->shipobj);
  }

  /* Evacuate every interior room into the water (or bridge fallback when
   * the ship object is roomless - should not happen in practice). */
  for (i = 0; i < ship->num_rooms && i < MAX_SHIP_ROOMS; i++)
  {
    interior = real_room(ship->room_vnums[i]);
    if (interior == NOWHERE)
    {
      continue;
    }

    for (tch = world[interior].people; tch; tch = next_tch)
    {
      next_tch = tch->next_in_room;
      if (water_room != NOWHERE)
      {
        send_to_char(tch, "You are thrown into the water as the ship goes down!\r\n");
        char_from_room(tch);
        char_to_room(tch, water_room);
        act("$n surfaces amid the wreckage, gasping.", TRUE, tch, 0, 0, TO_ROOM);
        look_at_room(tch, 0);
      }
      else
      {
        send_to_char(tch, "The ship lurches violently beneath you!\r\n");
      }
    }

    /* Static legacy rooms remain in the world; generated rooms are detached
     * by vessel_reclaim_interior_rooms() below. */
    if (ship->shipnum < 2)
    {
      world[interior].ship = NULL;
    }
  }

  /* Convert the ship object into salvageable wreckage; clearing the item
   * type disarms the boarding spec proc, which checks ITEM_GREYHAWK_SHIP. */
  if (ship->shipobj != NULL)
  {
    GET_OBJ_TYPE(ship->shipobj) = ITEM_OTHER;
    GET_OBJ_VAL(ship->shipobj, 0) = 0;
    GET_OBJ_VAL(ship->shipobj, 1) = -1;
    snprintf(buf, sizeof(buf), "wreckage wreck %s", ship->name);
    ship->shipobj->name = strdup(buf);
    snprintf(buf, sizeof(buf), "the wreckage of %s", ship->name);
    ship->shipobj->short_description = strdup(buf);
    snprintf(buf, sizeof(buf), "The shattered wreckage of %s floats here.", ship->name);
    ship->shipobj->description = strdup(buf);
    if (water_room != NOWHERE && world[water_room].people != NULL)
    {
      act("$p settles into the water, breaking apart.", FALSE, world[water_room].people,
          ship->shipobj, 0, TO_CHAR);
      act("$p settles into the water, breaking apart.", FALSE, world[water_room].people,
          ship->shipobj, 0, TO_ROOM);
    }
  }

  /* Settle insurance before the record is gone */
  vessel_pay_insurance(ship);
  vessel_abort_docking(ship);
  vehicle_release_all_from_vessel(ship, water_room);

  /* Clear this slot from every other ship's grudge list. The slot is about
   * to be freed for reuse, and a stale index would aim the AI's return fire
   * at whatever innocent hull is created there next. */
  for (i = 0; i < GREYHAWK_MAXSHIPS; i++)
  {
    if (greyhawk_ships[i].last_attacker == shipnum)
    {
      greyhawk_ships[i].last_attacker = 0;
    }
  }

  /* Release attached automation before clearing the slot */
  autopilot_cleanup(ship);
  if (ship->schedule != NULL)
  {
    free(ship->schedule);
    ship->schedule = NULL;
  }

  vessel_reclaim_interior_rooms(ship, water_room);
  if (!vessel_delete_persistence(shipnum))
  {
    log("SYSERR: Could not remove persistence for sunk ship %d", shipnum);
  }

  /* Free the fleet slot after every runtime and persistent reference is gone. */
  vessel_periodic_forget(ship);
  memset(ship, 0, sizeof(*ship));
}

/**
 * Apply damage with no ship behind it (pressure, weather) to one arc,
 * through the same armor-then-structure rules as gunfire.
 *
 * @param shipnum Target ship index
 * @param amount Raw damage
 * @param arc GREYHAWK_FORE/PORT/REAR/STARBOARD - which side is struck
 * @param cause Short description for messages (e.g. "The gale")
 */
void vessel_apply_damage(int shipnum, int amount, int arc, const char *cause)
{
  struct greyhawk_ship_data *ship;

  if (shipnum < 0 || shipnum >= GREYHAWK_MAXSHIPS || !is_valid_ship(&greyhawk_ships[shipnum]) ||
      amount <= 0)
  {
    return;
  }
  ship = &greyhawk_ships[shipnum];

  send_to_ship(ship, "%s strikes the hull!", cause ? cause : "Something");
  vessel_damage_hull(NULL, ship, amount, arc, FALSE);
  VSSL_DEBUG("Ship %d took %d damage on arc %d", shipnum, amount, arc);
  vessel_update_condition(ship, NULL);
}

/**
 * Combat tick: gunnery (reloads, locks, battle stations, NPC return fire)
 * and the damage model's sink and colors timers. Runs on the vessel tick.
 */
void vessel_combat_tick_one(struct greyhawk_ship_data *ship)
{
  if (!is_valid_ship(ship))
    return;
  vessel_gunnery_tick_one(ship);
  vessel_damage_tick_one(ship);
}

void vessel_combat_tick(void)
{
  int i;

  for (i = 0; i < GREYHAWK_MAXSHIPS; i++)
    vessel_combat_tick_one(&greyhawk_ships[i]);
}

/**
 * shiprepair - slow at-sea repairs while stationary.
 */
ACMD(do_shiprepair)
{
  struct greyhawk_ship_data *ship;
  int repaired = 0;
  int armor_amt;
  int internal_amt;
  int subsys_amt;
  int s;

  ship = get_ship_from_room(IN_ROOM(ch));
  if (ship == NULL)
  {
    send_to_char(ch, "You must be aboard a ship to make repairs.\r\n");
    return;
  }

  if (vessel_is_sinking(ship))
  {
    send_to_char(ch, "She is holed on two sides and going down - no patch will save her.\r\n");
    return;
  }

  if (vessel_crew_stunned(ship))
  {
    send_to_char(ch, "The crew reels from a mental blast; nobody can hold a tool steady.\r\n");
    return;
  }

  if (ship->speed > 0)
  {
    send_to_char(ch, "Repairs require the ship to be stationary.\r\n");
    return;
  }

  /* The bosun's crew works faster (vessels_crew.c sets repairspeed) */
  armor_amt = SHIP_REPAIR_ARMOR + ship->sailcrew.repairspeed;
  internal_amt = SHIP_REPAIR_INTERNAL + ship->sailcrew.repairspeed / 2;
  subsys_amt = SHIP_REPAIR_SUBSYS + ship->sailcrew.repairspeed;

#define VESSEL_REPAIR_FIELD(cur, max, amt)                                                         \
  do                                                                                               \
  {                                                                                                \
    if ((cur) < (max))                                                                             \
    {                                                                                              \
      (cur) = (typeof(cur))(((max) - (cur) > (amt)) ? (cur) + (amt) : (max));                      \
      repaired = 1;                                                                                \
    }                                                                                              \
  } while (0)

  VESSEL_REPAIR_FIELD(ship->farmor, ship->maxfarmor, armor_amt);
  VESSEL_REPAIR_FIELD(ship->rarmor, ship->maxrarmor, armor_amt);
  VESSEL_REPAIR_FIELD(ship->parmor, ship->maxparmor, armor_amt);
  VESSEL_REPAIR_FIELD(ship->sarmor, ship->maxsarmor, armor_amt);
  VESSEL_REPAIR_FIELD(ship->finternal, ship->maxfinternal, internal_amt);
  VESSEL_REPAIR_FIELD(ship->rinternal, ship->maxrinternal, internal_amt);
  VESSEL_REPAIR_FIELD(ship->pinternal, ship->maxpinternal, internal_amt);
  VESSEL_REPAIR_FIELD(ship->sinternal, ship->maxsinternal, internal_amt);
  VESSEL_REPAIR_FIELD(ship->mainsail, ship->maxmainsail, subsys_amt);
  VESSEL_REPAIR_FIELD(ship->turnrate, ship->maxturnrate, subsys_amt);

#undef VESSEL_REPAIR_FIELD

  /* Damaged weapons mend a little each time; a destroyed one is refitted only
   * by the port's shipwrights, until S4 sells weapons and S5 prices repairs. */
  for (s = 0; s < GREYHAWK_MAXSLOTS; s++)
  {
    if (ship->slot[s].type != VESSEL_SLOT_WEAPON || ship->slot[s].damage == 0)
    {
      continue;
    }
    if (ship->slot[s].damage < VESSEL_WEAPON_DESTROYED)
    {
      ship->slot[s].damage = (unsigned char)MAX(0, ship->slot[s].damage - subsys_amt);
      repaired = 1;
    }
    else if (ship->dock > 0)
    {
      ship->slot[s].damage = 0;
      repaired = 1;
    }
  }

  if (!repaired)
  {
    send_to_char(ch, "%s is already in fine trim.\r\n", ship->name);
    return;
  }

  act("$n works on the ship's repairs.", TRUE, ch, 0, 0, TO_ROOM);
  send_to_char(ch, "You patch armor and shore up timbers. The ship is %s.\r\n",
               vessel_status_name(vessel_status(ship)));
  WAIT_STATE(ch, PULSE_VIOLENCE * 2);
}

/**
 * claimship - capture a ship by holding its bridge uncontested.
 */
ACMD(do_claimship)
{
  struct greyhawk_ship_data *ship;
  struct char_data *tch;

  ship = get_ship_from_room(IN_ROOM(ch));
  if (ship == NULL)
  {
    send_to_char(ch, "You must be aboard a ship to claim it.\r\n");
    return;
  }

  if (world[IN_ROOM(ch)].number != (room_vnum)ship->bridge_room)
  {
    send_to_char(ch, "Ships are claimed from the bridge.\r\n");
    return;
  }

  if (!str_cmp(ship->owner, GET_NAME(ch)))
  {
    send_to_char(ch, "%s is already yours.\r\n", ship->name);
    return;
  }

  if (vessel_is_sinking(ship))
  {
    send_to_char(ch, "%s is going down - there is nothing left to claim.\r\n", ship->name);
    return;
  }

  /* Decision D6: only a beaten prize changes hands. */
  if (!vessel_prize_disabled(ship, ch))
  {
    send_to_char(ch,
                 "%s is not beaten. Only a holed or immobile hull, one that has struck her "
                 "colors, or one abandoned at sea can be taken.\r\n",
                 ship->name);
    return;
  }

  /* Seizing a hull takes it from its owner outright, so it answers to the
   * same consent rules as sinking it. Without this, anyone could board a
   * moored ship, walk to the bridge, and claim it while the owner slept. */
  if (!vessel_pvp_permitted(ch, ship, TRUE))
  {
    return;
  }

  /* The bridge must be uncontested: no other conscious characters. */
  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (tch != ch && AWAKE(tch))
    {
      send_to_char(ch, "The bridge is still contested - deal with %s first.\r\n", PERS(tch, ch));
      return;
    }
  }

  if (vessel_owner_at_cap(ch))
  {
    send_to_char(ch, "You already own %d hulls, the most one captain may hold.\r\n",
                 vessel_owned_hull_count(GET_NAME(ch)));
    return;
  }

  log("Info: %s captured ship %d '%s' (previous owner: %s)", GET_NAME(ch), ship->shipnum,
      ship->name, ship->owner[0] ? ship->owner : "none");
  if (!vessel_transfer_owner(ship, GET_NAME(ch)))
  {
    send_to_char(ch, "The ship's registry rejects your claim; ownership remains unchanged.\r\n");
    return;
  }
  vessel_merchant_handle_capture(ch, ship);
  vessel_hunter_handle_capture(ch, ship);
  /* A capture voids the old crew's helm clearances */
  ship->num_permits = 0;
  vessel_db_save_permits(ship);
  send_to_ship(ship, "%s seizes control of %s!", GET_NAME(ch), ship->name);
  send_to_char(ch, "You take the helm - %s is yours now.\r\n", ship->name);
}
