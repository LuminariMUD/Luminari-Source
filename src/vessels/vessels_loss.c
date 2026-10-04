/* ************************************************************************
 *      File:   vessels_loss.c                        Part of LuminariMUD  *
 *   Purpose:   Losing and recovering hulls (vessels-ships study S5,       *
 *              3.3.7 and 3.3.9): hull value and insurance, stowed hulls,  *
 *              the wreck registry, shipsummon, and the in-place rebuild   *
 *              that a wreck and a trade-in share                          *
 * ********************************************************************** */

/*
 * Decision D3: when a player's hull goes down the ship's identity survives.
 * She is rebuilt in her own fleet slot as the cheapest boat the shipyard
 * sells, stripped, and waits out of the world in the wreck registry until her
 * owner summons her. A summoned hull is out of the world too while she sails
 * to the shipyard. Both are stowed: not active, so is_valid_ship() and every
 * contact, tick, and command pass them by, but their slot stays reserved and
 * their persistence whole, and a reboot puts them back where they were.
 */

#include "conf.h"
#include "core/sysdep.h"
#include <math.h> /* before utils.h, which defines log() as a macro */
#include "core/structs.h"
#include "core/utils.h"
#include "core/comm.h"
#include "core/db.h"
#include "core/handler.h"
#include "core/interpreter.h"
#include "act/act.h"
#include "character/rewards.h"
#include "database/mysql.h"
#include "vessels.h"
#include "vessel_periodic.h"

extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];

/** A hull's value: the shipyard price of her class, design speed, and beam armor. */
int vessel_hull_price(const struct greyhawk_ship_data *ship)
{
  return vessel_prototype_price((int)ship->vessel_type, ship->maxspeed, ship->maxparmor);
}

/**
 * What the underwriters pay when an owned hull is lost (study 3.3.1): the
 * class share of her value, 90% when an NPC hull sank her, nothing for a raft
 * or a hull rebuilt from a wreck.
 */
int vessel_insurance_payout(const struct greyhawk_ship_data *ship, bool npc_kill)
{
  int share;

  if (ship == NULL || ship->owner[0] == '\0' || ship->wreck_hull)
  {
    return 0;
  }
  share = vessel_class_condition(ship->vessel_type)->insurance;
  if (share > 0 && npc_kill)
  {
    share = VESSEL_NPC_KILL_INSURANCE;
  }
  return vessel_hull_price(ship) * share / 100;
}

/** A summons costs a tenth of a gold piece per point of hull weight, at least 1. */
int vessel_summon_fee(const struct greyhawk_ship_data *ship)
{
  return MAX(1, vessel_class_handling(ship->vessel_type)->hull_weight / 10);
}

/**
 * How long a summoned hull takes to reach the shipyard, in seconds (Duris
 * summon_ship()): 50 mud hours over her empty-hold maximum speed in Duris
 * units for a raft or boat, 70 over that speed less 20 for the rest, each at
 * least 2; twice that from the wreck registry; at most 60 mud hours. A holed
 * hull makes no way and takes the longest.
 */
int vessel_summon_seconds(struct greyhawk_ship_data *ship)
{
  struct greyhawk_ship_data empty;
  double speed;
  double hours;

  empty = *ship;
  memset(empty.cargo, 0, sizeof(empty.cargo));
  speed = vessel_breached_arcs(ship) > 0
              ? 0.0
              : vessel_max_speed_from(ship->maxspeed, vessel_sailmaster_multiplier(ship),
                                      vessel_load_factor(&empty), ship->mainsail, ship->maxmainsail,
                                      100, 0) /
                    VESSEL_DURIS_SPEED_SCALE;
  if (ship->vessel_type == VESSEL_RAFT || ship->vessel_type == VESSEL_BOAT)
  {
    hours = 50.0 / fmax(speed, 2.0);
  }
  else
  {
    hours = 70.0 / fmax(speed - 20.0, 2.0);
  }
  if (ship->stowed && ship->summon_due == 0)
  {
    hours *= 2.0;
  }
  return (int)(fmin(hours, (double)VESSEL_SUMMON_MAX_MUD_HOURS) * SECS_PER_MUD_HOUR);
}

/**
 * Rebuild a hull in place from a prototype's class, design speed, and armor,
 * keeping her fleet slot and identity: owner, name, display ID, cosmetics,
 * helm permits, and crew. Her old interior must already be reclaimed; a new
 * one is generated. She comes out at rest with no weapons, refits, or cargo,
 * her crew rested and her stores full.
 *
 * @return FALSE when no interior could be generated
 */
bool vessel_rebuild_hull(struct greyhawk_ship_data *ship, int prototype_id, int vclass,
                         int max_speed, int armor)
{
  ship->prototype_id = prototype_id;
  ship->vessel_type = (enum vessel_class)vclass;
  ship->minspeed = 0;
  ship->maxspeed = (short int)max_speed;
  ship->speed = 0.0;
  ship->setspeed = 0;
  vessel_initialize_condition(ship, armor);
  memset(ship->slot, 0, sizeof(ship->slot));
  memset(ship->cargo, 0, sizeof(ship->cargo));
  ship->num_cargo_lots = 0;
  ship->upgrades = 0;
  ship->wreck_hull = FALSE;
  ship->wear_ticks = 0;
  ship->sink_ticks = 0;
  ship->colors_struck_ticks = 0;
  ship->lock_target = 0;
  ship->battle_ticks = 0;
  ship->stun_ticks = 0;
  ship->maintenance_ticks = 0;
  ship->departure_ticks = 0;
  ship->maneuver_ticks = 0;
  ship->stamina_spent = 0.0;
  ship->repair_used = 0;
  ship->last_attacker = 0;
  ship->position_speed_percent = 0;
  generate_ship_interior(ship);
  return ship->num_rooms > 0 && ship->entrance_room > 0;
}

/**
 * The hull a lost ship is rebuilt as (study 3.3.7): the cheapest boat the
 * shipyard sells, else its cheapest hull, else a boat to the `vedit`
 * defaults.
 */
void vessel_wreck_prototype(int *id, int *vclass, int *speed, int *armor)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  bool best_boat;
  bool boat;
  int best_price;
  int price;

  *id = 0;
  *vclass = VESSEL_BOAT;
  *speed = vessel_class_handling(VESSEL_BOAT)->speed;
  *armor = vessel_class_condition(VESSEL_BOAT)->beam_armor;
  if (!mysql_available || conn == NULL ||
      mysql_query(conn, "SELECT prototype_id, vessel_class, max_speed, armor "
                        "FROM ship_prototypes WHERE for_sale = 1"))
  {
    return;
  }
  result = mysql_store_result(conn);
  if (result == NULL)
  {
    return;
  }

  best_boat = FALSE;
  best_price = 0;
  while ((row = mysql_fetch_row(result)) != NULL)
  {
    if (row[0] == NULL || row[1] == NULL || row[2] == NULL || row[3] == NULL)
    {
      continue;
    }
    boat = parse_int(row[1]) == VESSEL_BOAT;
    price = vessel_prototype_price(parse_int(row[1]), parse_int(row[2]), parse_int(row[3]));
    if (*id == 0 || (boat && !best_boat) || (boat == best_boat && price < best_price))
    {
      *id = parse_int(row[0]);
      *vclass = parse_int(row[1]);
      *speed = parse_int(row[2]);
      *armor = parse_int(row[3]);
      best_boat = boat;
      best_price = price;
    }
  }
  mysql_free_result(result);
}

/**
 * Take a hull out of the world with her state saved: she keeps her fleet slot,
 * interior, and persistence, but no exterior object and no ticks. A hull
 * already stowed has her new state saved. A failed save is retried by
 * save_all_vessels(), which saves stowed hulls too.
 */
static void vessel_stow(struct greyhawk_ship_data *ship)
{
  if (ship->shipobj != NULL)
  {
    extract_obj(ship->shipobj);
    ship->shipobj = NULL;
  }
  ship->stowed = TRUE;
  ship->active = FALSE;
  if (!vessel_save_one(ship))
  {
    log("SYSERR: Stowed ship %d could not be saved completely", ship->shipnum);
  }
  vessel_periodic_forget(ship);
}

/** Stow a hull that boot restored in the world, as she was saved. */
void vessel_restow(struct greyhawk_ship_data *ship)
{
  if (ship->shipobj != NULL)
  {
    extract_obj(ship->shipobj);
    ship->shipobj = NULL;
  }
  ship->active = FALSE;
  vessel_periodic_forget(ship);
}

/** Delete a hull's rows with one statement that binds her fleet slot. */
static void vessel_delete_hull_rows(const char *sql, int shipnum)
{
  PREPARED_STMT *statement;

  if (!mysql_available || conn == NULL)
  {
    return;
  }
  statement = mysql_stmt_create(conn);
  if (statement == NULL || !mysql_stmt_prepare_query(statement, sql) ||
      !mysql_stmt_bind_param_int(statement, 0, shipnum) || !mysql_stmt_execute_prepared(statement))
  {
    log("SYSERR: Could not clear rows of lost ship %d", shipnum);
  }
  mysql_stmt_cleanup(statement);
}

/**
 * A player's hull has gone down (decision D3). Her crew takes its casualties,
 * 10% plus 1% per 30 renown lost to a player's hull, 5% plus 1% per 100 hull
 * weight otherwise; she is rebuilt from the wreck prototype with her sails
 * gone, unless an NPC hull sank her and she outweighed the boat; and she
 * waits in the wreck registry at the wreck site. Her interior must already
 * be reclaimed.
 *
 * @return FALSE when she could not be rebuilt and is lost outright
 */
bool vessel_wreck_hull(struct greyhawk_ship_data *ship, const struct greyhawk_ship_data *victor,
                       int renown_lost)
{
  int old_weight;
  int id;
  int vclass;
  int speed;
  int armor;

  old_weight = vessel_class_handling(ship->vessel_type)->hull_weight;
  vessel_crew_casualties(ship, victor != NULL && victor->owner[0] != '\0'
                                   ? 10.0 + (double)renown_lost / 30.0
                                   : 5.0 + (double)old_weight / 100.0);

  vessel_wreck_prototype(&id, &vclass, &speed, &armor);
  if (!vessel_rebuild_hull(ship, id, vclass, speed, armor))
  {
    log("SYSERR: Lost ship %d could not be rebuilt for the wreck registry", ship->shipnum);
    return FALSE;
  }
  if (victor == NULL || victor->owner[0] != '\0' ||
      old_weight <= vessel_class_handling(ship->vessel_type)->hull_weight)
  {
    ship->mainsail = 0;
  }
  ship->wreck_hull = TRUE;
  ship->summon_due = 0;

  /* Legacy cargo objects and NPC crew went down with her. */
  vessel_delete_hull_rows("DELETE FROM ship_cargo_manifest WHERE ship_id = ?", ship->shipnum);
  vessel_delete_hull_rows("DELETE FROM ship_crew_roster WHERE ship_id = ? AND npc_vnum >= 0",
                          ship->shipnum);
  vessel_stow(ship);
  log("Info: Ship %d '%s' of %s waits in the wreck registry as a %s", ship->shipnum, ship->name,
      ship->owner, get_vessel_type_name(ship->vessel_type));
  return TRUE;
}

/**
 * Put everyone aboard but her NPC pilot into `room`: the crew makes sail
 * without them.
 */
static void vessel_put_ashore(struct greyhawk_ship_data *ship, room_rnum room)
{
  struct char_data *pilot;
  struct char_data *tch;
  struct char_data *next_tch;
  room_rnum interior;
  int i;

  pilot = get_pilot_from_ship(ship);
  for (i = 0; i < ship->num_rooms && i < MAX_SHIP_ROOMS; i++)
  {
    interior = real_room(ship->room_vnums[i]);
    if (interior == NOWHERE)
    {
      continue;
    }
    for (tch = world[interior].people; tch != NULL; tch = next_tch)
    {
      next_tch = tch->next_in_room;
      if (tch == pilot)
      {
        continue;
      }
      send_to_char(tch, "%s is answering a summons; you are put over the side.\r\n", ship->name);
      char_from_room(tch);
      vessel_char_to_room(tch, room);
      look_at_room(tch, 0);
    }
  }
}

/**
 * Tell the dock a summoned hull has made port, and her owner wherever they
 * are: the passage can take an hour.
 */
void vessel_summon_announce(const struct greyhawk_ship_data *ship)
{
  struct char_data *owner;
  room_rnum room;

  room = IN_ROOM(ship->shipobj);
  send_to_room(room, "%s arrives at port.\r\n", ship->name);
  owner = vessel_find_online_player(ship->owner);
  if (owner != NULL && IN_ROOM(owner) != room)
  {
    send_to_char(owner, "Word comes from the harbor: %s has made port at %s.\r\n", ship->name,
                 world[room].name);
  }
}

/** A summoned hull makes port: back into the world, berthed at the shipyard. */
static void vessel_summon_arrive(struct greyhawk_ship_data *ship)
{
  ship->active = TRUE;
  ship->stowed = FALSE;
  ship->summon_due = 0;
  if (!vessel_create_runtime_hull(ship))
  {
    log("SYSERR: Summoned ship %d could not make port at room %d", ship->shipnum, ship->location);
    ship->active = FALSE;
    ship->stowed = TRUE;
    return;
  }
  vessel_sync_berth(ship);
  vessel_db_save_runtime(ship);
  vessel_periodic_sync(ship);
  vessel_summon_announce(ship);
  log("Info: Summoned ship %d '%s' made port at room %d", ship->shipnum, ship->name,
      ship->location);
}

/** Bring summoned hulls whose passage is done into port. */
void vessel_summon_tick(void)
{
  time_t now;
  int i;

  now = time(0);
  for (i = 2; i < GREYHAWK_MAXSHIPS; i++)
  {
    if (greyhawk_ships[i].stowed && greyhawk_ships[i].summon_due > 0 &&
        greyhawk_ships[i].summon_due <= now)
    {
      vessel_summon_arrive(&greyhawk_ships[i]);
    }
  }
}

static bool vessel_summon_owned(const struct greyhawk_ship_data *ship, const char *owner)
{
  return (ship->active || ship->stowed) && ship->shipnum >= 2 && ship->owner[0] != '\0' &&
         !str_cmp(ship->owner, owner);
}

/** Minutes or seconds, for a passage time. */
static void vessel_describe_wait(char *buf, size_t size, int seconds)
{
  if (seconds >= 120)
  {
    snprintf(buf, size, "about %d minutes", (seconds + 30) / 60);
  }
  else
  {
    snprintf(buf, size, "%d second%s", seconds, seconds == 1 ? "" : "s");
  }
}

static void vessel_list_summonable(struct char_data *ch)
{
  struct greyhawk_ship_data *ship;
  char wait[64];
  int listed;
  int i;

  listed = 0;
  for (i = 0; i < GREYHAWK_MAXSHIPS; i++)
  {
    ship = &greyhawk_ships[i];
    if (!vessel_summon_owned(ship, GET_NAME(ch)))
    {
      continue;
    }
    listed++;
    if (ship->summon_due > 0)
    {
      vessel_describe_wait(wait, sizeof(wait),
                           ship->summon_due > time(0) ? (int)(ship->summon_due - time(0)) : 0);
      send_to_char(ch, "%2d. %s (%s): under summons, due in %s.\r\n", listed, ship->name,
                   get_vessel_type_name(ship->vessel_type), wait);
      continue;
    }
    vessel_describe_wait(wait, sizeof(wait), vessel_summon_seconds(ship));
    send_to_char(ch, "%2d. %s (%s): %s; %d gold, %s.\r\n", listed, ship->name,
                 get_vessel_type_name(ship->vessel_type),
                 ship->stowed     ? "in the wreck registry"
                 : ship->dock > 0 ? "berthed"
                                  : "at sea",
                 vessel_summon_fee(ship), wait);
  }
  if (listed == 0)
  {
    send_to_char(ch, "You own no hull to summon.\r\n");
    return;
  }
  send_to_char(ch, "Usage: shipsummon <number | name>\r\n");
}

/**
 * shipsummon [<number | name>] - at a shipyard, list the hulls you own with
 * the fee and passage time, or call one here. She leaves at once with an
 * empty hold, anyone aboard put over the side, and makes port when her
 * passage is done, even across a reboot.
 */
ACMD(do_shipsummon)
{
  struct greyhawk_ship_data *ship;
  struct greyhawk_ship_data before;
  const char *arg;
  char wait[64];
  room_rnum here;
  room_rnum exterior;
  int autopilot_state;
  int autopilot_waypoint;
  int wanted;
  int count;
  int fee;
  int seconds;
  int i;

  if (IS_NPC(ch))
  {
    return;
  }
  here = IN_ROOM(ch);
  if (!vessel_room_is_port(here))
  {
    send_to_char(ch, "Hulls are summoned from a shipyard dock.\r\n");
    return;
  }
  if (vessel_port_refuses(ch))
  {
    return;
  }

  arg = argument;
  skip_spaces_c(&arg);
  if (!*arg)
  {
    vessel_list_summonable(ch);
    return;
  }

  wanted = is_number(arg) ? parse_int(arg) : 0;
  ship = NULL;
  count = 0;
  for (i = 0; i < GREYHAWK_MAXSHIPS && ship == NULL; i++)
  {
    if (!vessel_summon_owned(&greyhawk_ships[i], GET_NAME(ch)))
    {
      continue;
    }
    count++;
    if (wanted > 0 ? count == wanted : is_abbrev(arg, greyhawk_ships[i].name))
    {
      ship = &greyhawk_ships[i];
    }
  }
  if (ship == NULL)
  {
    send_to_char(ch, "You own no such hull. Type 'shipsummon' for your list.\r\n");
    return;
  }
  if (ship->summon_due > 0)
  {
    send_to_char(ch, "There is already an order out for %s.\r\n", ship->name);
    return;
  }
  if (!ship->stowed && ship->shipobj != NULL && IN_ROOM(ship->shipobj) == here)
  {
    send_to_char(ch, "%s is already here.\r\n", ship->name);
    return;
  }
  if (vessel_is_sinking(ship))
  {
    send_to_char(ch, "%s is going down; nothing can bring her in now.\r\n", ship->name);
    return;
  }
  if (vessel_at_battle_stations(ship) && GET_LEVEL(ch) < LVL_IMMORT)
  {
    send_to_char(ch, "%s's crew is at battle stations and will not answer a summons.\r\n",
                 ship->name);
    return;
  }
  fee = vessel_summon_fee(ship);
  if (GET_GOLD(ch) < fee)
  {
    send_to_char(ch, "The harbor master wants %d gold to send for her; you have %d.\r\n", fee,
                 GET_GOLD(ch));
    return;
  }

  if (!vessel_charge(ch, fee))
  {
    return;
  }
  seconds = GET_LEVEL(ch) >= LVL_IMMORT ? 1 : vessel_summon_seconds(ship);

  /* She will appear where she is saved: this shipyard. The summons is saved
   * before she leaves the world and before anything alongside or aboard is
   * touched, so one that cannot be saved leaves her as she was. Her
   * autopilot's state lies outside the copy of the hull. */
  before = *ship;
  autopilot_state = AUTOPILOT_OFF;
  autopilot_waypoint = 0;
  if (!ship->stowed && ship->autopilot != NULL)
  {
    autopilot_state = ship->autopilot->state;
    autopilot_waypoint = ship->autopilot->current_waypoint_index;
    autopilot_stop(ship);
  }
  ship->docked_to_ship = -1;
  ship->docking_room = 0;
  memset(ship->cargo, 0, sizeof(ship->cargo));
  ship->num_cargo_lots = 0;
  ship->location = (int)world[here].number;
  ship->x = (double)world[here].coords[0];
  ship->y = (double)world[here].coords[1];
  ship->z = 0.0;
  ship->dx = 0.0;
  ship->dy = 0.0;
  ship->speed = 0.0;
  ship->setspeed = 0;
  ship->dock = 0;
  ship->anchored = FALSE;
  ship->lock_target = 0;
  ship->summon_due = time(0) + seconds;
  ship->stowed = TRUE;

  /* The emptied manifest goes first: a crash between the two writes must not
   * bring her in with the cargo the summons leaves behind. */
  if (!vessel_db_save_cargo(ship) || !vessel_save_hull(ship))
  {
    *ship = before;
    if (!ship->stowed && ship->autopilot != NULL)
    {
      ship->autopilot->state = autopilot_state;
      ship->autopilot->current_waypoint_index = autopilot_waypoint;
    }
    if (!vessel_db_save_cargo(ship) || !vessel_save_hull(ship))
    {
      log("SYSERR: Ship %d could not be written back after a refused summons; her rows may hold "
          "part of it until she is saved again",
          ship->shipnum);
    }
    vessel_refund(ch, fee);
    send_to_char(ch,
                 "The harbor master cannot record the summons, so %s stays where she is and your "
                 "%d gold is returned.\r\n",
                 ship->name, fee);
    return;
  }
  if (!before.stowed)
  {
    /* Saved as cast off: now withdraw the gangway from the hull alongside. */
    exterior = ship->shipobj != NULL ? IN_ROOM(ship->shipobj) : NOWHERE;
    ship->docked_to_ship = before.docked_to_ship;
    ship->docking_room = before.docking_room;
    vessel_abort_docking(ship);
    vehicle_release_all_from_vessel(ship, exterior);
    if (exterior != NOWHERE)
    {
      vessel_put_ashore(ship, exterior);
      send_to_room(exterior, "%s makes sail and is soon out of sight.\r\n", ship->name);
    }
  }
  vessel_restow(ship);

  vessel_describe_wait(wait, sizeof(wait), seconds);
  send_to_char(ch, "You pay %d gold. Word goes out to %s; she should make port here in %s.\r\n",
               fee, ship->name, wait);
  log("Info: %s summoned ship %d '%s' to room %d, due in %d seconds", GET_NAME(ch), ship->shipnum,
      ship->name, ship->location, seconds);
}
