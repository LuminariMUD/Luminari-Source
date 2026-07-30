/* ************************************************************************
 *      File:   vessels_combat.c                      Part of LuminariMUD  *
 *   Purpose:   Naval combat (Phase 05): ship damage model, weapon fire,   *
 *              sinking, groundings, repair, and capture.                  *
 *              Builds on the greyhawk per-side armor/internal fields and  *
 *              weapon slot data already present in greyhawk_ship_data.    *
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
#include "constants.h"
#include "act.h"

extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];
extern struct room_data *world;
extern int wild_waterline;

/* Reload time (combat ticks) applied to a weapon slot after firing. The
 * combat tick shares AUTOPILOT_TICK_INTERVAL cadence in comm.c. */
#define SHIP_WEAPON_RELOAD_TICKS 6

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
bool vessel_pvp_grace_active(const struct greyhawk_ship_data *target,
                             const char *attacker_name, time_t now)
{
  if (target == NULL || attacker_name == NULL || !*attacker_name ||
      target->pvp_grace_attacker[0] == '\0')
  {
    return FALSE;
  }
  return target->pvp_grace_until >= now &&
         !str_cmp(target->pvp_grace_attacker, attacker_name);
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
static void vessel_record_pvp_engagement(struct char_data *ch,
                                         struct greyhawk_ship_data *target)
{
  struct char_data *aggressor;
  struct greyhawk_ship_data *aggressor_ship;
  time_t until;

  aggressor = vessel_effective_aggressor(ch);
  if (aggressor == NULL || IS_NPC(aggressor) || GET_NAME(aggressor) == NULL ||
      target == NULL || target->owner[0] == '\0')
  {
    return;
  }

  until = time(0) + VESSEL_PVP_LOGOUT_GRACE;
  target->pvp_grace_until = until;
  strlcpy(target->pvp_grace_attacker, GET_NAME(aggressor),
          sizeof(target->pvp_grace_attacker));

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
  struct char_data *aggressor;
  struct char_data *owner;
  bool permitted;

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
  permitted = pvp_ok(aggressor, owner, display);
  if (permitted)
  {
    vessel_record_pvp_engagement(aggressor, target);
  }
  return permitted;
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

  if (max <= 0)
  {
    return VESSEL_STATUS_SOUND; /* No damage model data - treat as sound */
  }

  if (cur <= 0)
  {
    return VESSEL_STATUS_SINKING;
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
 * Determine which arc (side) of ship1 faces ship2.
 *
 * Computes the bearing from ship1 to ship2, offsets it by ship1's heading,
 * and buckets the relative bearing into the four firing arcs.
 *
 * @return GREYHAWK_FORE, GREYHAWK_STARBOARD, GREYHAWK_REAR, or GREYHAWK_PORT
 */
int greyhawk_getarc(int ship1, int ship2)
{
  int bearing;
  int relative;

  if (ship1 < 0 || ship1 >= GREYHAWK_MAXSHIPS || ship2 < 0 || ship2 >= GREYHAWK_MAXSHIPS)
  {
    return GREYHAWK_FORE;
  }

  bearing = greyhawk_bearing(greyhawk_ships[ship1].x, greyhawk_ships[ship1].y,
                             greyhawk_ships[ship2].x, greyhawk_ships[ship2].y);
  relative = (bearing - greyhawk_ships[ship1].heading + 360) % 360;

  if (relative >= 315 || relative < 45)
  {
    return GREYHAWK_FORE;
  }
  if (relative < 135)
  {
    return GREYHAWK_STARBOARD;
  }
  if (relative < 225)
  {
    return GREYHAWK_REAR;
  }
  return GREYHAWK_PORT;
}

/**
 * Sink a ship: evacuate everyone aboard into the water, convert the ship
 * object into inert wreckage, and free the fleet slot.
 */
void vessel_sink(int shipnum)
{
  struct greyhawk_ship_data *ship;
  struct char_data *tch;
  struct char_data *next_tch;
  room_rnum interior;
  room_rnum water_room = NOWHERE;
  char buf[MAX_STRING_LENGTH];
  int i;

  if (shipnum < 0 || shipnum >= GREYHAWK_MAXSHIPS ||
      !is_valid_ship(&greyhawk_ships[shipnum]))
  {
    return;
  }
  ship = &greyhawk_ships[shipnum];

  log("Info: Ship %d '%s' is sinking at (%d,%d)", shipnum, ship->name, (int)ship->x, (int)ship->y);
  send_to_ship(ship, "The hull gives way - %s is SINKING!", ship->name);

  /* Merchant definitions outlive their killable hulls. Record the responsible
   * player and schedule replacement while identity, cargo, and geography are
   * still available. */
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
  memset(ship, 0, sizeof(*ship));
}

/**
 * Apply damage to one arc of a ship.
 *
 * Side armor absorbs first; overflow hits that section's internal
 * structure. Section damage degrades subsystems: fore hits tear rigging
 * (mainsail - speed), rear hits foul the rudder (turnrate). Total internal
 * reaching zero sinks the ship.
 *
 * @param shipnum Target ship index
 * @param amount Raw damage
 * @param arc GREYHAWK_FORE/PORT/REAR/STARBOARD - which side is struck
 * @param cause Short description for messages/logs (e.g. "a ballista bolt")
 */
void vessel_apply_damage(int shipnum, int amount, int arc, const char *cause)
{
  struct greyhawk_ship_data *ship;
  unsigned char *armor;
  unsigned char *internal_hp;
  int spill;
  const char *side_name;

  if (shipnum < 0 || shipnum >= GREYHAWK_MAXSHIPS ||
      !is_valid_ship(&greyhawk_ships[shipnum]) || amount <= 0)
  {
    return;
  }
  ship = &greyhawk_ships[shipnum];

  switch (arc)
  {
  case GREYHAWK_PORT:
    armor = &ship->parmor;
    internal_hp = &ship->pinternal;
    side_name = "port side";
    break;
  case GREYHAWK_REAR:
    armor = &ship->rarmor;
    internal_hp = &ship->rinternal;
    side_name = "stern";
    break;
  case GREYHAWK_STARBOARD:
    armor = &ship->sarmor;
    internal_hp = &ship->sinternal;
    side_name = "starboard side";
    break;
  case GREYHAWK_FORE:
  default:
    armor = &ship->farmor;
    internal_hp = &ship->finternal;
    side_name = "bow";
    break;
  }

  spill = amount - (int)*armor;
  if (spill < 0)
  {
    spill = 0;
  }
  if ((int)*armor >= amount)
  {
    *armor -= (unsigned char)amount;
  }
  else
  {
    *armor = 0;
  }

  if (spill > 0)
  {
    int breach = 0;

    if ((int)*internal_hp > spill)
    {
      *internal_hp -= (unsigned char)spill;
    }
    else
    {
      /* Section destroyed - the remainder tears through into the rest of
       * the hull, so a pounded side eventually takes the whole ship down. */
      breach = spill - (int)*internal_hp;
      *internal_hp = 0;
    }

    if (breach > 0)
    {
      unsigned char *sections[4] = {&ship->finternal, &ship->rinternal, &ship->pinternal,
                                    &ship->sinternal};
      int sec;

      for (sec = 0; sec < 4 && breach > 0; sec++)
      {
        if (sections[sec] == internal_hp || *sections[sec] == 0)
        {
          continue;
        }
        if ((int)*sections[sec] > breach)
        {
          *sections[sec] -= (unsigned char)breach;
          breach = 0;
        }
        else
        {
          breach -= (int)*sections[sec];
          *sections[sec] = 0;
        }
      }
    }

    /* Subsystem degradation from structural hits */
    if (arc == GREYHAWK_FORE && ship->mainsail > 0)
    {
      ship->mainsail = (ship->mainsail > spill) ? ship->mainsail - spill : 0;
      if (ship->mainsail == 0)
      {
        send_to_ship(ship, "The rigging collapses! The ship is dead in the water.");
        ship->speed = 0;
        ship->setspeed = 0;
      }
    }
    if (arc == GREYHAWK_REAR && ship->turnrate > 0)
    {
      ship->turnrate = (ship->turnrate > spill) ? ship->turnrate - spill : 0;
      if (ship->turnrate == 0)
      {
        send_to_ship(ship, "The rudder is smashed! The helm no longer answers.");
      }
    }
  }

  send_to_ship_throttled(ship, VESSEL_MESSAGE_COMBAT_DAMAGE, VESSEL_COMBAT_MESSAGE_COOLDOWN,
                         "%s strikes the %s! (%s: armor %d, structure %d)",
                         cause ? cause : "Something", side_name, side_name, (int)*armor,
                         (int)*internal_hp);
  VSSL_DEBUG("Ship %d took %d damage on arc %d (%s): armor %d internal %d", shipnum, amount, arc,
             side_name, (int)*armor, (int)*internal_hp);

  if (vessel_total_internal(ship) <= 0)
  {
    vessel_sink(shipnum);
  }
}

/**
 * Check for grounding after a surface vessel moves.
 *
 * Uses real wilderness bathymetry: depth below the waterline at the ship's
 * coordinates, compared against the class's min_water_depth. Airborne and
 * submerged classes are exempt. A grounding stops the ship and damages the
 * bow.
 */
void vessel_check_grounding(int shipnum)
{
  struct greyhawk_ship_data *ship;
  const struct vessel_terrain_caps *caps;
  int elevation;
  int depth_units;
  int sector;

  if (shipnum < 0 || shipnum >= GREYHAWK_MAXSHIPS ||
      !is_valid_ship(&greyhawk_ships[shipnum]))
  {
    return;
  }
  ship = &greyhawk_ships[shipnum];

  if (ship->vessel_type == VESSEL_AIRSHIP || ship->vessel_type == VESSEL_MAGICAL)
  {
    return;
  }

  caps = get_vessel_terrain_caps(ship->vessel_type);
  if (caps == NULL || caps->min_water_depth <= 0)
  {
    return;
  }

  /* Only meaningful in water sectors */
  sector = get_ship_terrain_type(shipnum);
  if (sector != SECT_WATER_SWIM && sector != SECT_WATER_NOSWIM && sector != SECT_OCEAN &&
      sector != SECT_UNDERWATER)
  {
    return;
  }

  /* Depth in raw wilderness elevation units below the waterline */
  elevation = get_modified_elevation((int)ship->x, (int)ship->y);
  depth_units = wild_waterline - elevation;

  if (depth_units < caps->min_water_depth)
  {
    send_to_ship(ship, "The hull GRINDS across the shallows - you've run aground!");
    ship->speed = 0;
    ship->setspeed = 0;
    vessel_apply_damage(shipnum, dice(2, 4), GREYHAWK_FORE, "The seabed");
    log("Info: Ship %d '%s' ran aground at (%d,%d): depth %d < required %d", shipnum, ship->name,
        (int)ship->x, (int)ship->y, depth_units, caps->min_water_depth);
  }
}

/**
 * Auto-defense doctrine: an NPC-piloted ship returns fire at its last
 * attacker with every ready weapon that bears and is in range.
 */
static void vessel_ai_return_fire(int shipnum)
{
  struct greyhawk_ship_data *ship = &greyhawk_ships[shipnum];
  struct greyhawk_ship_data *target;
  struct greyhawk_ship_slot *weapon;
  float range;
  int target_num;
  int fire_arc;
  int attack_roll;
  int defense_dc;
  int dmg;
  int s;

  if (ship->autopilot == NULL || ship->autopilot->pilot_mob_vnum == -1)
  {
    return; /* No NPC pilot - players fight their own battles */
  }

  target_num = ship->last_attacker;
  if (target_num <= 0 || target_num >= GREYHAWK_MAXSHIPS)
  {
    return;
  }
  target = &greyhawk_ships[target_num];
  if (!is_valid_ship(target))
  {
    ship->last_attacker = 0; /* Attacker sank or despawned */
    return;
  }

  fire_arc = greyhawk_getarc(shipnum, target_num);
  range = greyhawk_range(ship->x, ship->y, ship->z, target->x, target->y, target->z);

  for (s = 0; s < GREYHAWK_MAXSLOTS; s++)
  {
    weapon = &ship->slot[s];
    if (weapon->type != 1 || weapon->timer > 0 || weapon->position != fire_arc)
    {
      continue;
    }
    if (weapon->val0 > 0 && range > (float)weapon->val0)
    {
      continue;
    }

    weapon->timer = SHIP_WEAPON_RELOAD_TICKS;
    target->last_attacker = shipnum;

    attack_roll = rand_number(1, 20) + ship->guncrew.gunadjust + 5; /* trained crews */
    defense_dc = 10 + target->speed / 5;

    send_to_ship_throttled(ship, VESSEL_MESSAGE_COMBAT_RETURN_FIRE,
                           VESSEL_COMBAT_MESSAGE_COOLDOWN,
                           "The crew RETURNS FIRE at %s!", target->name);
    if (attack_roll < defense_dc)
    {
      send_to_ship_throttled(target, VESSEL_MESSAGE_COMBAT_RETURN_FIRE_MISS,
                             VESSEL_COMBAT_MESSAGE_COOLDOWN,
                             "Return fire from %s splashes wide!", ship->name);
      continue;
    }

    dmg = (weapon->val2 > 0 && weapon->val3 > 0) ? dice(weapon->val2, weapon->val3) : dice(2, 6);
    vessel_apply_damage(target_num, dmg, greyhawk_getarc(target_num, shipnum), "Return fire");
    VSSL_DEBUG("AI ship %d return-fired slot %d at ship %d for %d", shipnum, s, target_num, dmg);
  }
}

/**
 * Combat tick: count down weapon reload timers for every active ship and
 * run NPC return-fire doctrine. Shares the autopilot tick cadence in comm.c.
 */
void vessel_combat_tick(void)
{
  int i, s;

  for (i = 0; i < GREYHAWK_MAXSHIPS; i++)
  {
    if (!is_valid_ship(&greyhawk_ships[i]))
    {
      continue;
    }
    for (s = 0; s < GREYHAWK_MAXSLOTS; s++)
    {
      if (greyhawk_ships[i].slot[s].timer > 0)
      {
        greyhawk_ships[i].slot[s].timer--;
        if (greyhawk_ships[i].slot[s].timer == 0 && greyhawk_ships[i].slot[s].type == 1)
        {
          send_to_ship_throttled(
              &greyhawk_ships[i], VESSEL_MESSAGE_COMBAT_RELOAD, VESSEL_COMBAT_MESSAGE_COOLDOWN,
              "%s is reloaded and ready.",
              greyhawk_ships[i].slot[s].desc[0] ? greyhawk_ships[i].slot[s].desc : "A weapon");
        }
      }
    }

    vessel_ai_return_fire(i);
  }
}

/**
 * Find a target ship by name or two-letter ID, excluding the given index.
 *
 * @return Target ship index, or -1 if not found
 */
static int vessel_find_target(const char *arg, int exclude_shipnum)
{
  int i;

  if (arg == NULL || !*arg)
  {
    return -1;
  }

  for (i = 0; i < GREYHAWK_MAXSHIPS; i++)
  {
    if (i == exclude_shipnum)
    {
      continue;
    }
    if (!is_valid_ship(&greyhawk_ships[i]))
    {
      continue;
    }
    if (!str_cmp(arg, greyhawk_ships[i].id) || is_abbrev(arg, greyhawk_ships[i].name))
    {
      return i;
    }
  }

  return -1;
}

/**
 * shipfire <slot> <target> - fire a weapon slot at another ship.
 */
ACMD(do_shipfire)
{
  struct greyhawk_ship_data *ship;
  struct greyhawk_ship_data *target;
  struct greyhawk_ship_slot *weapon;
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  float range;
  int slot_num;
  int target_num;
  int fire_arc;
  int struck_arc;
  int attack_roll;
  int defense_dc;
  int dmg;

  ship = get_ship_from_room(IN_ROOM(ch));
  if (ship == NULL)
  {
    send_to_char(ch, "You must be aboard a ship to fire its weapons.\r\n");
    return;
  }

  two_arguments_u((char *)argument, arg1, arg2);
  if (!*arg1 || !*arg2)
  {
    send_to_char(ch, "Usage: shipfire <slot 0-%d> <target ship>\r\n", GREYHAWK_MAXSLOTS - 1);
    return;
  }

  slot_num = atoi(arg1);
  if (!isdigit((unsigned char)*arg1) || slot_num < 0 || slot_num >= GREYHAWK_MAXSLOTS)
  {
    send_to_char(ch, "Weapon slots run 0-%d.\r\n", GREYHAWK_MAXSLOTS - 1);
    return;
  }

  weapon = &ship->slot[slot_num];
  if (weapon->type != 1)
  {
    send_to_char(ch, "Slot %d holds no weapon.\r\n", slot_num);
    return;
  }
  if (weapon->timer > 0)
  {
    send_to_char(ch, "That weapon is still reloading (%d).\r\n", weapon->timer);
    return;
  }

  target_num = vessel_find_target(arg2, ship->shipnum);
  if (target_num < 0)
  {
    send_to_char(ch, "No such ship in the fleet registry.\r\n");
    return;
  }
  target = &greyhawk_ships[target_num];

  /* Consent gate: sinking a hull drowns her crew and destroys her cargo, so
   * it answers to the same PvP rules as drawing a blade. */
  if (!vessel_pvp_permitted(ch, target, TRUE))
  {
    return;
  }

  /* Range gate: use the weapon's long range (val0) */
  range = greyhawk_range(ship->x, ship->y, ship->z, target->x, target->y, target->z);
  if (weapon->val0 > 0 && range > (float)weapon->val0)
  {
    send_to_char(ch, "%s is out of range (%.1f vs %d).\r\n", target->name, range,
                 (int)weapon->val0);
    return;
  }

  /* Arc gate: the weapon's mounted side must face the target */
  fire_arc = greyhawk_getarc(ship->shipnum, target_num);
  if (weapon->position != fire_arc)
  {
    send_to_char(ch, "That weapon cannot bear - the target lies off a different arc.\r\n");
    return;
  }

  /* Resolve the shot: d20 + gunnery vs a speed-based defense DC */
  vessel_merchant_note_attacker(ch, target);
  attack_roll = d20(ch) + GET_LEVEL(ch) / 2 + ship->guncrew.gunadjust;
  defense_dc = 10 + target->speed / 5;

  /* Mark the aggression so NPC-piloted victims return fire */
  target->last_attacker = ship->shipnum;

  weapon->timer = SHIP_WEAPON_RELOAD_TICKS;

  send_to_ship(ship, "%s FIRES at %s!", weapon->desc[0] ? weapon->desc : "A weapon", target->name);

  if (attack_roll < defense_dc)
  {
    send_to_ship(ship, "The shot goes wide, splashing harmlessly.");
    send_to_ship(target, "A projectile from %s splashes into the water nearby!", ship->name);
    return;
  }

  dmg = (weapon->val2 > 0 && weapon->val3 > 0) ? dice(weapon->val2, weapon->val3) : dice(2, 6);
  struck_arc = greyhawk_getarc(target_num, ship->shipnum);
  send_to_ship(ship, "Direct hit on %s!", target->name);
  vessel_apply_damage(target_num, dmg, struck_arc, "Incoming fire");

  WAIT_STATE(ch, PULSE_VIOLENCE);
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

  ship = get_ship_from_room(IN_ROOM(ch));
  if (ship == NULL)
  {
    send_to_char(ch, "You must be aboard a ship to make repairs.\r\n");
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
      (cur) = ((max) - (cur) > (amt)) ? (cur) + (amt) : (max);                                     \
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
    if (tch != ch && GET_POS(tch) > POS_STUNNED)
    {
      send_to_char(ch, "The bridge is still contested - deal with %s first.\r\n", PERS(tch, ch));
      return;
    }
  }

  log("Info: %s captured ship %d '%s' (previous owner: %s)", GET_NAME(ch), ship->shipnum,
      ship->name, ship->owner[0] ? ship->owner : "none");
  if (!vessel_transfer_owner(ship, GET_NAME(ch)))
  {
    send_to_char(ch, "The ship's registry rejects your claim; ownership remains unchanged.\r\n");
    return;
  }
  vessel_merchant_handle_capture(ch, ship);
  /* A capture voids the old crew's helm clearances */
  ship->num_permits = 0;
  vessel_db_save_permits(ship);
  send_to_ship(ship, "%s seizes control of %s!", GET_NAME(ch), ship->name);
  send_to_char(ch, "You take the helm - %s is yours now.\r\n", ship->name);
}
