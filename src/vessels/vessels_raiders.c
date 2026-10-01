/* ************************************************************************
 *      File:   vessels_raiders.c                     Part of LuminariMUD  *
 *   Purpose:   NPC raiders (vessels-ships study S6, 3.3.8): ambushes,     *
 *              raider tiers, their AI, boarding and looting, and NPC      *
 *              merchants that run                                         *
 * ********************************************************************** */

/*
 * Each tick a moving player's hull at sea may be ambushed: one chance in
 * 2002, twice that in a pirate cove, half in territorial waters, and sixty
 * times rarer under neutral colors. Her hull weight picks a raider tier and
 * whether she draws a pirate, who loots and leaves, or a hunter, who fights
 * on. The raider is a public hull built from one of the tier's prototypes
 * (the vessel_raider_tiers rows) and fitted with the tier's weapons, with a
 * captain on the bridge as her NPC pilot, a crew, and a chest in the hold
 * whose key the captain carries. She appears beyond sight off the quarry's
 * bow and closes at full speed.
 *
 * Her AI is DurisMUD's NPCShipAI. Engaging, the basic brain turns the arc
 * that will be ready soonest onto the quarry and keeps within its weapons'
 * band; the advanced one projects both hulls and works round to the
 * quarry's weakest side. Her guns fire as NPC return fire at the quarry. She
 * rams when it pays and boards a slow quarry through the boarding contest.
 * Out of ammunition or holed, she runs. Having lost her quarry she cruises
 * for another and leaves the sea 600 ticks later, once no player can watch
 * her go. Killing her captain stops all of it. Raiders are never kept: boot
 * retires any hull a restart restored from a raider prototype.
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
#include "character/abilities.h"
#include "database/mysql.h"
#include "magic/spells.h"

extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];

/* One raider tier (study 3.3.8): her crew, brain, and fit-out */
struct vessel_raider_tier
{
  int crew_min; /* Mobiles aboard, the captain included */
  int crew_max;
  int advanced;  /* Percent chance of the advanced AI */
  int crew_tier; /* Of her sailmaster, gunner, and bosun */
  int gold_min;  /* In her chest: twice Duris's platinum */
  int gold_max;
  int renown_min; /* The renown she carries */
  int renown_max;
  struct
  {
    int weapon;
    int count;
  } arms[VESSEL_NUM_ARCS]; /* Per arc, GREYHAWK_FORE order */
};

static const struct vessel_raider_tier raider_tiers[VESSEL_RAIDER_TIERS] = {
    {8,
     12,
     0,
     CREW_TIER_GREEN,
     800,
     1600,
     150,
     300,
     {{VESSEL_WEAPON_SMALL_CATAPULT, 1},
      {VESSEL_WEAPON_SMALL_BALLISTA, 1},
      {VESSEL_WEAPON_SMALL_BALLISTA, 1},
      {VESSEL_WEAPON_SMALL_BALLISTA, 1}}},
    {9,
     12,
     20,
     CREW_TIER_GREEN,
     1200,
     2000,
     500,
     600,
     {{VESSEL_WEAPON_SMALL_CATAPULT, 1},
      {VESSEL_WEAPON_MEDIUM_BALLISTA, 2},
      {VESSEL_WEAPON_NONE, 0},
      {VESSEL_WEAPON_MEDIUM_BALLISTA, 2}}},
    {12,
     15,
     50,
     CREW_TIER_ABLE,
     2400,
     3000,
     700,
     1000,
     {{VESSEL_WEAPON_MEDIUM_CATAPULT, 2},
      {VESSEL_WEAPON_LARGE_BALLISTA, 2},
      {VESSEL_WEAPON_NONE, 0},
      {VESSEL_WEAPON_LARGE_BALLISTA, 2}}},
    {12,
     18,
     100,
     CREW_TIER_VETERAN,
     3000,
     6000,
     2000,
     3000,
     {{VESSEL_WEAPON_LARGE_CATAPULT, 1},
      {VESSEL_WEAPON_LARGE_BALLISTA, 2},
      {VESSEL_WEAPON_SMALL_CATAPULT, 1},
      {VESSEL_WEAPON_LARGE_BALLISTA, 2}}}};

/* The bearing of each arc's centre off the bow, GREYHAWK_FORE order */
static const double arc_bearing[VESSEL_NUM_ARCS] = {0.0, 270.0, 180.0, 90.0};

/* The raider tier table (3.3.8): the prototypes each tier sails */
void vessel_raider_ensure_schema(void)
{
  if (!mysql_available || conn == NULL)
  {
    return;
  }
  if (mysql_query(conn, "CREATE TABLE IF NOT EXISTS vessel_raider_tiers ("
                        "tier TINYINT NOT NULL,"
                        "prototype_id INT NOT NULL,"
                        "PRIMARY KEY (tier, prototype_id)"
                        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4"))
  {
    log("SYSERR: Could not create vessel_raider_tiers: %s", mysql_error(conn));
  }
}

/** Duris's merchant hulls: the raft, boat, ship, and transport. */
bool vessel_merchant_class(enum vessel_class vessel_type)
{
  return vessel_type == VESSEL_RAFT || vessel_type == VESSEL_BOAT || vessel_type == VESSEL_SHIP ||
         vessel_type == VESSEL_TRANSPORT;
}

/**
 * One in how many ticks this hull is ambushed, or 0 when she cannot be: a
 * moving player's hull bigger than a boat, on the surface, out of port,
 * with no lock set, and, for a ship or transport, not yet ambushed this
 * voyage. A pirate cove halves the odds, territorial waters double them,
 * and neutral colors multiply them by 60.
 */
int vessel_raider_ambush_odds(struct greyhawk_ship_data *ship)
{
  struct vessel_piracy_law law;
  int odds;

  if (!is_valid_ship(ship) || ship->owner[0] == '\0' || ship->speed <= 0.0 ||
      ship->lock_target != 0 || (int)ship->z != 0 || ship->vessel_type == VESSEL_RAFT ||
      ship->vessel_type == VESSEL_BOAT || vessel_ship_is_in_port(ship) ||
      (vessel_merchant_class(ship->vessel_type) && ship->raided))
  {
    return 0;
  }

  odds = VESSEL_RAIDER_AMBUSH_ODDS;
  vessel_piracy_law_for_ship(ship, &law);
  if (law.waters_type == VESSEL_WATERS_PIRATE_COVE)
  {
    odds /= 2;
  }
  else if (law.waters_type == VESSEL_WATERS_TERRITORIAL)
  {
    odds *= 2;
  }
  if (vessel_equipment_slot(ship, VESSEL_EQUIPMENT_COLORS) >= 0)
  {
    odds *= 60;
  }
  return odds;
}

/**
 * The tier a quarry draws (Duris try_load_pirate_ship()) from
 * n = random(0, hull weight) + her renown. A merchant class
 * draws tier 0 below 250 and tier 1 below 1,200, then tier 2 three times in
 * four (a hunter one time in three), else a tier 3 hunter. Any other class
 * is noticed only when n >= random(1, 1000), and then draws a tier 2 hunter,
 * or tier 3 one time in three.
 *
 * @return the tier, or -1 when the raiders pass her by
 */
int vessel_raider_pick_tier(const struct greyhawk_ship_data *target, bool *hunter)
{
  int n;

  n = rand_number(0, vessel_class_handling(target->vessel_type)->hull_weight) + target->renown;
  *hunter = FALSE;
  if (vessel_merchant_class(target->vessel_type))
  {
    if (n < 250)
    {
      return 0;
    }
    if (n < 1200)
    {
      return 1;
    }
    if (n < 2200 || rand_number(1, 4) != 1)
    {
      *hunter = rand_number(1, 3) == 1;
      return 2;
    }
    *hunter = TRUE;
    return 3;
  }
  if (n < rand_number(1, 1000))
  {
    return -1;
  }
  *hunter = TRUE;
  return rand_number(1, 3) == 1 ? 3 : 2;
}

/**
 * One of the tier's prototypes, at random among those at least as fast as
 * the quarry's design speed less 3, or among all when none is.
 *
 * @return FALSE when the tier has no prototype
 */
bool vessel_raider_pick_prototype(int tier, int quarry_speed, int *prototype_id, char *name,
                                  size_t name_size)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  int total;
  int fast;
  int pick;

  if (mysql_query(conn, "SELECT raider.tier, prototype.prototype_id, prototype.max_speed, "
                        "prototype.name FROM vessel_raider_tiers AS raider "
                        "JOIN ship_prototypes AS prototype "
                        "ON prototype.prototype_id = raider.prototype_id "
                        "ORDER BY prototype.prototype_id"))
  {
    log("SYSERR: Could not read the raider tiers: %s", mysql_error(conn));
    return FALSE;
  }
  result = mysql_store_result(conn);
  if (result == NULL)
  {
    return FALSE;
  }

  total = 0;
  fast = 0;
  while ((row = mysql_fetch_row(result)) != NULL)
  {
    if (parse_int(row[0]) != tier)
    {
      continue;
    }
    total++;
    if (parse_int(row[2]) >= quarry_speed - 3)
    {
      fast++;
    }
  }
  if (total == 0)
  {
    mysql_free_result(result);
    return FALSE;
  }

  pick = rand_number(1, fast > 0 ? fast : total);
  *prototype_id = 0;
  mysql_data_seek(result, 0);
  while ((row = mysql_fetch_row(result)) != NULL)
  {
    if (parse_int(row[0]) != tier || (fast > 0 && parse_int(row[2]) < quarry_speed - 3))
    {
      continue;
    }
    pick--;
    if (pick == 0)
    {
      *prototype_id = parse_int(row[1]);
      strlcpy(name, row[3], name_size);
      break;
    }
  }
  mysql_free_result(result);
  return *prototype_id > 0;
}

/** Load a raider mobile into a room of a hull, when both exist. */
static void vessel_raider_load_mobile(int vnum, int room_number)
{
  struct char_data *mob;
  room_rnum room;

  room = real_room(room_number);
  mob = room != NOWHERE ? read_mobile_reason(vnum, VIRTUAL, PERF_ENTITY_VESSEL) : NULL;
  if (mob != NULL)
  {
    char_to_room(mob, room);
  }
}

/** Stow her chest of gold in the hold (the bridge on a hull without one), its key on her captain. */
static void vessel_raider_stow_chest(struct greyhawk_ship_data *raider, struct char_data *captain,
                                     const struct vessel_raider_tier *tier)
{
  struct obj_data *chest;
  struct obj_data *key;
  room_rnum hold;

  hold = real_room(raider->cargo_rooms[0] > 0 ? raider->cargo_rooms[0] : raider->bridge_room);
  chest = read_object_reason(VESSEL_RAIDER_CHEST_VNUM, VIRTUAL, PERF_ENTITY_VESSEL);
  key = read_object_reason(VESSEL_RAIDER_KEY_VNUM, VIRTUAL, PERF_ENTITY_VESSEL);
  if (chest == NULL || key == NULL || hold == NOWHERE)
  {
    if (chest != NULL)
    {
      extract_obj(chest);
    }
    if (key != NULL)
    {
      extract_obj(key);
    }
    return;
  }
  obj_to_obj(create_money(rand_number(tier->gold_min, tier->gold_max)), chest);
  obj_to_room(chest, hold);
  obj_to_char(key, captain);
}

/**
 * Launch a raider of `tier` against target: one of the tier's prototypes,
 * her sight range plus 10 rooms off target's bow within 45 degrees, fitted,
 * crewed, carrying the tier's renown, and sailing at her at full speed.
 *
 * @return the raider's fleet slot, or -1
 */
int vessel_raider_spawn(struct greyhawk_ship_data *target, int tier, bool hunter)
{
  const struct vessel_raider_tier *stats;
  struct greyhawk_ship_data *raider;
  struct char_data *captain;
  char name[128];
  double direction;
  double distance;
  int prototype_id;
  int crew;
  int slot;
  int arc;
  int n;
  int s;

  if (!mysql_available || conn == NULL || !is_valid_ship(target) || tier < 0 ||
      tier >= VESSEL_RAIDER_TIERS ||
      !vessel_raider_pick_prototype(tier, target->maxspeed, &prototype_id, name, sizeof(name)))
  {
    return -1;
  }
  stats = &raider_tiers[tier];

  direction = target->heading + rand_number(-45, 45);
  distance = vessel_sight_range(target) + VESSEL_RAIDER_SPAWN_MARGIN;
  slot = vessel_spawn_public_from_prototype_at(
      prototype_id, name, (int)lround(target->x + sin(direction * M_PI / 180.0) * distance),
      (int)lround(target->y + cos(direction * M_PI / 180.0) * distance), 0);
  if (slot < 0)
  {
    return -1;
  }
  raider = &greyhawk_ships[slot];
  if (!vessel_assign_npc_pilot(raider, VESSEL_RAIDER_CAPTAIN_VNUM + tier))
  {
    log("SYSERR: Raider %d '%s' has no captain (mobile %d)", slot, name,
        VESSEL_RAIDER_CAPTAIN_VNUM + tier);
    vessel_retire_npc_hull(slot, NULL);
    return -1;
  }
  captain = get_pilot_from_ship(raider);

  /* The tier's fit-out replaces her class armament, each weapon mounted
   * only while the fit-out stays legal for her class. */
  memset(raider->slot, 0, sizeof(raider->slot));
  for (arc = 0; arc < VESSEL_NUM_ARCS; arc++)
  {
    for (n = 0; n < stats->arms[arc].count; n++)
    {
      s = vessel_free_slot(raider);
      vessel_set_weapon(&raider->slot[s], stats->arms[arc].weapon, arc);
      if (vessel_fitout_problem(raider) != NULL)
      {
        memset(&raider->slot[s], 0, sizeof(raider->slot[s]));
      }
    }
  }
  for (n = CREW_SAILMASTER; n <= CREW_BOSUN; n++)
  {
    raider->crew_tier[n] = stats->crew_tier;
    raider->crew_xp[n] = vessel_crew_floor(n, stats->crew_tier);
  }
  vessel_apply_crew_bonuses(raider);

  crew = rand_number(stats->crew_min, stats->crew_max);
  for (n = 1; n < crew; n++)
  {
    vessel_raider_load_mobile(VESSEL_RAIDER_CREW_VNUM + tier,
                              raider->room_vnums[rand_number(0, raider->num_rooms - 1)]);
  }
  vessel_raider_stow_chest(raider, captain, stats);

  raider->raider_mode = VESSEL_RAIDER_ENGAGING;
  raider->raider_tier = (unsigned char)tier;
  raider->renown = rand_number(stats->renown_min, stats->renown_max);
  raider->raider_hunter = hunter;
  raider->raider_advanced = rand_number(1, 100) <= stats->advanced;
  raider->raider_target = target->shipnum;
  raider->heading = fmod(direction + 540.0, 360.0);
  raider->setheading = (short int)raider->heading;
  raider->setspeed = raider->maxspeed;
  raider->speed = vessel_max_speed(raider);
  target->raided = TRUE;
  log("Info: A tier %d %s, raider %d '%s', ambushes ship %d '%s'", tier,
      hunter ? "hunter" : "pirate", slot, raider->name, target->shipnum, target->name);
  return slot;
}

/**
 * Rooms of water she can sail ahead on heading, probed every half room up
 * to limit.
 */
static double vessel_raider_open_water(struct greyhawk_ship_data *ship, double heading,
                                       double limit)
{
  double radians;
  double distance;
  int step;

  radians = heading * M_PI / 180.0;
  for (step = 1; step * 0.5 <= limit; step++)
  {
    distance = step * 0.5;
    if (!vessel_chart_cell(ship, (int)lround(ship->x + ship->dx + sin(radians) * distance),
                           (int)lround(ship->y + ship->dy + cos(radians) * distance), (int)ship->z))
    {
      return distance - 0.5;
    }
  }
  return limit;
}

/**
 * Order a heading and speed with Duris's seamanship: a heading that meets
 * land or water she cannot sail within `lookout` rooms swings 30 degrees at
 * a time, to each side in turn, to open water; land within 2 rooms on the
 * new or the present heading brakes her (within half a room to speed 1, a
 * room 6, two rooms 12: Duris's speeds times 0.3).
 */
static void vessel_raider_set_course(struct greyhawk_ship_data *ship, double heading, int speed,
                                     double lookout)
{
  double course;
  double open;
  int swing;
  int i;

  course = heading;
  for (i = 1; i <= 10 && vessel_raider_open_water(ship, course, lookout) < lookout; i++)
  {
    swing = 30 * ((i + 1) / 2);
    course = heading + (i % 2 == 1 ? swing : -swing);
  }
  if (i > 10)
  {
    course = heading; /* No open water anywhere: hold on and brake */
  }

  open = fmin(vessel_raider_open_water(ship, ship->heading, 2.0),
              vessel_raider_open_water(ship, course, 2.0));
  if (open < 0.5)
  {
    speed = MIN(speed, 1);
  }
  else if (open < 1.0)
  {
    speed = MIN(speed, 6);
  }
  else if (open < 2.0)
  {
    speed = MIN(speed, 12);
  }
  ship->setheading = (short int)(lround(fmod(course + 720.0, 360.0)) % 360);
  ship->setspeed = (short int)MIN(speed, ship->maxspeed);
}

/**
 * Whether she may pursue target: a player's hull afloat, out of port and
 * above water, within her sight range plus the spawn margin.
 */
static bool vessel_raider_can_pursue(struct greyhawk_ship_data *ship,
                                     struct greyhawk_ship_data *target)
{
  return is_valid_ship(target) && target != ship && target->owner[0] != '\0' &&
         !vessel_is_sinking(target) && vessel_target_problem(target) == NULL &&
         vessel_range_between(ship, target) <=
             (double)(vessel_sight_range(ship) + VESSEL_RAIDER_SPAWN_MARGIN);
}

/**
 * A quarry for a cruising raider: the player's hull that last fired on her,
 * else the nearest player's hull in sight that does not fly neutral colors.
 */
static struct greyhawk_ship_data *vessel_raider_find_quarry(struct greyhawk_ship_data *ship)
{
  struct vessel_contact contacts[VESSEL_CONTACT_DISPLAY_LIMIT];
  struct greyhawk_ship_data *other;
  int count;
  int i;

  if (ship->last_attacker > 0 && ship->last_attacker < GREYHAWK_MAXSHIPS &&
      vessel_raider_can_pursue(ship, &greyhawk_ships[ship->last_attacker]))
  {
    return &greyhawk_ships[ship->last_attacker];
  }
  count = MIN(vessel_collect_contacts(ship, contacts, VESSEL_CONTACT_DISPLAY_LIMIT),
              VESSEL_CONTACT_DISPLAY_LIMIT);
  for (i = 0; i < count; i++)
  {
    other = &greyhawk_ships[contacts[i].shipnum];
    if (vessel_raider_can_pursue(ship, other) &&
        vessel_equipment_slot(other, VESSEL_EQUIPMENT_COLORS) < 0)
    {
      return other;
    }
  }
  return NULL;
}

/** Whether she still has a round for any weapon. */
static bool vessel_raider_has_ammo(const struct greyhawk_ship_data *ship)
{
  int s;

  for (s = 0; s < GREYHAWK_MAXSLOTS; s++)
  {
    if (vessel_slot_weapon(&ship->slot[s]) != NULL && ship->slot[s].ammo > 0)
    {
      return TRUE;
    }
  }
  return FALSE;
}

/** Slow enough to board: speed 3 or less for a merchant class, stopped for the rest. */
static bool vessel_raider_boardable(const struct greyhawk_ship_data *target)
{
  if (vessel_merchant_class(target->vessel_type))
  {
    return vessel_display_speed(target->speed) <= VESSEL_BOARDING_MAX_SPEED;
  }
  return target->speed <= 0.0;
}

/** A sound, loaded weapon on the arc. */
static bool vessel_raider_usable(const struct greyhawk_ship_slot *slot, int arc)
{
  return vessel_slot_weapon(slot) != NULL && slot->position == arc && slot->damage == 0 &&
         slot->ammo > 0;
}

/**
 * How soon the arc can fire at range: the least reload, in ticks, of its
 * usable weapons whose good band holds the range (the minimum range to a
 * quarter of the maximum, at least a room past the minimum), or -1.
 */
static int vessel_raider_arc_ready(const struct greyhawk_ship_data *ship, int arc, double range)
{
  const struct vessel_weapon_type *type;
  int ready;
  int s;

  ready = -1;
  for (s = 0; s < GREYHAWK_MAXSLOTS; s++)
  {
    type = vessel_slot_weapon(&ship->slot[s]);
    if (!vessel_raider_usable(&ship->slot[s], arc) || range < type->min_range ||
        range > fmax(type->min_range + 1.0, type->max_range / 4.0))
    {
      continue;
    }
    if (ready < 0 || ship->slot[s].timer < ready)
    {
      ready = ship->slot[s].timer;
    }
  }
  return ready;
}

/** Whether a loaded, ready weapon is too close to the quarry to fire (Duris: minimum under 10). */
static bool vessel_raider_too_close(const struct greyhawk_ship_data *ship, double range)
{
  const struct vessel_weapon_type *type;
  int s;

  for (s = 0; s < GREYHAWK_MAXSLOTS; s++)
  {
    type = vessel_slot_weapon(&ship->slot[s]);
    if (vessel_raider_usable(&ship->slot[s], ship->slot[s].position) && ship->slot[s].timer <= 0 &&
        range < type->min_range && type->min_range < 10)
    {
      return TRUE;
    }
  }
  return FALSE;
}

/**
 * The arc to turn onto the quarry (Duris b_turn_active_weapon() and
 * b_turn_reloading_weapon()): of the arcs that can fire at this range, the
 * one ready within 4 s that needs the least turn, else the one ready
 * soonest; -1 when none can. *soon says it is ready within 4 s.
 */
static int vessel_raider_best_arc(const struct greyhawk_ship_data *ship, double range,
                                  double bearing, bool *soon)
{
  double turn;
  double best_turn;
  int best_ready;
  int ready;
  int best;
  int arc;

  best = -1;
  best_ready = 0;
  best_turn = 0.0;
  for (arc = 0; arc < VESSEL_NUM_ARCS; arc++)
  {
    ready = vessel_raider_arc_ready(ship, arc, range);
    if (ready < 0)
    {
      continue;
    }
    turn = fabs(vessel_heading_difference(ship->heading, bearing - arc_bearing[arc]));
    if (best < 0 || (ready <= 8 && (best_ready > 8 || turn < best_turn)) ||
        (ready > 8 && best_ready > 8 && ready < best_ready))
    {
      best = arc;
      best_ready = ready;
      best_turn = turn;
    }
  }
  *soon = best >= 0 && best_ready <= 8;
  return best;
}

/** The quarry's weakest side that still has armor or structure (her fore when none has). */
static int vessel_raider_weakest_side(struct greyhawk_ship_data *target)
{
  int best;
  int left;
  int arc;

  best = GREYHAWK_FORE;
  for (arc = 0; arc < VESSEL_NUM_ARCS; arc++)
  {
    left = *vessel_arc_armor(target, arc) + *vessel_arc_internal(target, arc);
    if (left > 0 && (*vessel_arc_armor(target, best) + *vessel_arc_internal(target, best) == 0 ||
                     left < *vessel_arc_armor(target, best) + *vessel_arc_internal(target, best)))
    {
      best = arc;
    }
  }
  return best;
}

/** The heading from ship to a point `distance` rooms off target's `side`. */
static double vessel_raider_heading_to_side(const struct greyhawk_ship_data *ship,
                                            const struct greyhawk_ship_data *target, int side,
                                            double distance)
{
  double radians;

  radians = (target->heading + arc_bearing[side]) * M_PI / 180.0;
  return greyhawk_bearing(ship->x + ship->dx, ship->y + ship->dy,
                          target->x + target->dx + sin(radians) * distance,
                          target->y + target->dy + cos(radians) * distance);
}

/**
 * A heading that leads the quarry (Duris calc_intercept_heading()): between
 * her bearing and her heading, leading her further when she crosses.
 */
static double vessel_raider_intercept(double bearing, double heading)
{
  if (bearing >= heading && bearing - heading > 180.0)
  {
    heading += 360.0;
  }
  else if (heading > bearing && heading - bearing > 180.0)
  {
    bearing += 360.0;
  }
  if (bearing - heading > 90.0)
  {
    heading += (bearing - heading - 90.0) * 2.0;
  }
  else if (bearing - heading < -90.0)
  {
    heading -= (heading - bearing - 90.0) * 2.0;
  }
  return (bearing + heading) / 2.0;
}

/**
 * The basic AI (Duris basic_combat_maneuver()): work round a quarry whose
 * facing side is shot away, else turn an arc ready within 4 s onto her,
 * open the range when a ready gun is too close, turn the arc that reloads
 * soonest, or chase her.
 */
static void vessel_raider_basic(struct greyhawk_ship_data *ship, struct greyhawk_ship_data *target)
{
  double bearing;
  double range;
  double heading;
  bool close;
  bool soon;
  int facing;
  int arc;

  bearing = vessel_bearing_between(ship, target);
  range = vessel_range_between(ship, target);
  facing = vessel_arc_toward(target, ship);
  arc = vessel_raider_best_arc(ship, range, bearing, &soon);
  close = vessel_raider_too_close(ship, range);
  if (*vessel_arc_armor(target, facing) == 0 && *vessel_arc_internal(target, facing) == 0)
  {
    heading = range >= 8.0 ? bearing
                           : vessel_raider_heading_to_side(ship, target,
                                                           vessel_raider_weakest_side(target), 3.0);
  }
  else if (arc >= 0 && (soon || !close))
  {
    heading = bearing - arc_bearing[arc];
  }
  else if (close)
  {
    heading = bearing + 180.0;
  }
  else
  {
    heading = vessel_raider_intercept(bearing, target->heading);
  }
  vessel_raider_set_course(ship, heading, ship->maxspeed, 2.0);
}

/**
 * Her broadside for the advanced AI: the arc whose usable weapons reload
 * soonest, the beams before the ends on a tie; -1 without a usable weapon.
 */
static int vessel_raider_broadside(const struct greyhawk_ship_data *ship)
{
  static const int order[VESSEL_NUM_ARCS] = {GREYHAWK_STARBOARD, GREYHAWK_PORT, GREYHAWK_FORE,
                                             GREYHAWK_REAR};
  int best_ready;
  int best;
  int i;
  int s;

  best = -1;
  best_ready = 0;
  for (i = 0; i < VESSEL_NUM_ARCS; i++)
  {
    for (s = 0; s < GREYHAWK_MAXSLOTS; s++)
    {
      if (vessel_raider_usable(&ship->slot[s], order[i]) &&
          (best < 0 || ship->slot[s].timer < best_ready))
      {
        best = order[i];
        best_ready = ship->slot[s].timer;
      }
    }
  }
  return best;
}

/** The range an arc fights at: the least good range of its usable weapons (3 rooms without). */
static double vessel_raider_arc_range(const struct greyhawk_ship_data *ship, int arc)
{
  const struct vessel_weapon_type *type;
  double range;
  int s;

  range = 0.0;
  for (s = 0; s < GREYHAWK_MAXSLOTS; s++)
  {
    type = vessel_slot_weapon(&ship->slot[s]);
    if (vessel_raider_usable(&ship->slot[s], arc) &&
        (range <= 0.0 || fmax(type->min_range + 1.0, type->max_range / 4.0) < range))
    {
      range = fmax(type->min_range + 1.0, type->max_range / 4.0);
    }
  }
  return range > 0.0 ? range : 3.0;
}

/**
 * The advanced AI (Duris advanced_combat_maneuver()): beyond 10 rooms it
 * chases; closer, it projects both hulls 3 s ahead, takes the quarry's
 * weakest side and its own broadside, and once off that side turns the
 * broadside onto her when it is ready, else steers for a point off that side
 * at the broadside's range.
 */
static void vessel_raider_advanced(struct greyhawk_ship_data *ship,
                                   struct greyhawk_ship_data *target)
{
  struct greyhawk_ship_data ship_next;
  struct greyhawk_ship_data target_next;
  struct autopilot_data ship_pilot;
  struct autopilot_data target_pilot;
  double bearing;
  double range;
  double heading;
  int side;
  int arc;

  bearing = vessel_bearing_between(ship, target);
  range = vessel_range_between(ship, target);
  arc = vessel_raider_broadside(ship);
  if (range > 10.0 || arc < 0)
  {
    vessel_raider_set_course(ship, vessel_raider_intercept(bearing, target->heading),
                             ship->maxspeed, 2.0);
    return;
  }

  vessel_project(ship, &ship_next, &ship_pilot, 6);
  vessel_project(target, &target_next, &target_pilot, 6);
  side = vessel_raider_weakest_side(target);
  if (vessel_arc_toward(&target_next, &ship_next) == side &&
      vessel_raider_arc_ready(ship, arc, range) == 0)
  {
    heading = bearing - arc_bearing[arc];
  }
  else
  {
    heading =
        vessel_raider_heading_to_side(ship, &target_next, side, vessel_raider_arc_range(ship, arc));
  }
  vessel_raider_set_course(ship, heading, ship->maxspeed, 2.0);
}

/**
 * Whether to ram now (Duris worth_ramming() and check_ram()): off cooldown,
 * inside a room, under way, the quarry before her bow at her altitude and
 * under half again her weight, with no arc gutted and the fore and beam
 * armor at more than half. A basic brain rams a quarry it cannot board only
 * one time in three.
 */
static bool vessel_raider_rams(struct greyhawk_ship_data *ship, struct greyhawk_ship_data *target,
                               double range)
{
  int arc;

  if (ship->ram_ticks > 0 || range >= 1.0 ||
      vessel_display_speed(ship->speed) <= VESSEL_BOARDING_MAX_SPEED ||
      (int)ship->z != (int)target->z ||
      fabs(vessel_heading_difference(ship->heading, vessel_bearing_between(ship, target))) >
          VESSEL_RAM_CONE / 2.0 ||
      vessel_class_handling(target->vessel_type)->hull_weight * 2 >=
          vessel_class_handling(ship->vessel_type)->hull_weight * 3)
  {
    return FALSE;
  }
  for (arc = 0; arc < VESSEL_NUM_ARCS; arc++)
  {
    if (*vessel_arc_internal(ship, arc) == 0 ||
        (arc != GREYHAWK_REAR &&
         *vessel_arc_armor(ship, arc) * 2 <= *vessel_arc_max_armor(ship, arc)))
    {
      return FALSE;
    }
  }
  return ship->raider_advanced || vessel_raider_boardable(target) || rand_number(1, 3) == 1;
}

/**
 * Pirates strip the hold (Duris steal_target_cargo()): of each lot a random
 * 40-60% share of what they take is lost in the transfer and a 40-60% share
 * is left behind, and their own hold takes what fits.
 */
static void vessel_raider_loot(struct greyhawk_ship_data *ship, struct greyhawk_ship_data *target)
{
  int lost;
  int left;
  int taken;
  int total;
  int i;

  lost = rand_number(40, 60);
  left = rand_number(40, 60);
  total = 0;
  for (i = 0; i < MAX_CARGO_LOTS; i++)
  {
    if (target->cargo[i].commodity_id == 0 || target->cargo[i].quantity <= 0)
    {
      continue;
    }
    taken = vessel_stow_cargo(ship, target->cargo[i].commodity_id,
                              MAX(1, target->cargo[i].quantity * 100 / (100 + lost + left)));
    target->cargo[i].quantity -= MIN(target->cargo[i].quantity, taken + taken * lost / 100);
    if (target->cargo[i].quantity == 0)
    {
      target->cargo[i].commodity_id = 0;
    }
    total += taken;
  }
  if (total > 0)
  {
    vessel_db_save_cargo(target);
    send_to_ship(target, "The raiders carry off %d unit%s of cargo, spilling more over the side!",
                 total, total == 1 ? "" : "s");
  }
}

/**
 * Board the quarry (Duris board_target()), once: her captain leads the
 * grapple and the crossing against the best defender aboard. Boarders take
 * the bridge and three quarters of the rooms (tiers 0-1) or half (tiers
 * 2-3); a pirate then loots the hold. A pirate leaves after her one attempt
 * either way, a hunter fights on.
 */
static void vessel_raider_board(struct greyhawk_ship_data *ship, struct greyhawk_ship_data *target)
{
  static const enum vessel_boarding_stage stages[] = {VESSEL_BOARDING_GRAPPLE,
                                                      VESSEL_BOARDING_CROSSING};
  struct vessel_boarding_contest contest;
  struct char_data *captain;
  struct char_data *defender;
  int defender_skill;
  int skill;
  int count;
  int i;
  bool won;

  ship->raider_boarded = target->shipnum;
  captain = get_pilot_from_ship(ship);
  skill = MAX(0, compute_ability(captain, ABILITY_BOARDING));
  defender = vessel_best_boarding_defender(captain, target, &defender_skill);
  send_to_ship(target, "WARNING: %s throws grappling lines across!", ship->name);
  won = TRUE;
  for (i = 0; i < 2 && won; i++)
  {
    won = vessel_resolve_boarding_contest(skill, d20(captain), defender_skill, d20(defender),
                                          vessel_boarding_defense_modifier(target, stages[i]),
                                          &contest) &&
          contest.attacker_wins;
  }

  if (!won)
  {
    send_to_ship(target, "The crew beats off %s's boarders!", ship->name);
  }
  else
  {
    send_to_ship(target, "Raiders from %s swarm aboard%s!", ship->name,
                 ship->raider_hunter ? "" : " in search of plunder");
    count = MAX(1, target->num_rooms * (ship->raider_tier <= 1 ? 3 : 2) / 4);
    vessel_raider_load_mobile(VESSEL_RAIDER_CREW_VNUM + ship->raider_tier, target->bridge_room);
    for (i = 1; i < count; i++)
    {
      vessel_raider_load_mobile(VESSEL_RAIDER_CREW_VNUM + ship->raider_tier,
                                target->room_vnums[rand_number(0, target->num_rooms - 1)]);
    }
    if (!ship->raider_hunter)
    {
      vessel_raider_loot(ship, target);
    }
  }

  if (!ship->raider_hunter)
  {
    ship->raider_mode = VESSEL_RAIDER_LEAVING;
    ship->raider_target = 0;
    ship->last_attacker = 0;
  }
}

/**
 * Engage the quarry: run without ammunition or with an arc holed; else
 * mark her for the guns, ram when it pays, close to board a slow quarry,
 * and maneuver with the basic or advanced brain.
 */
static void vessel_raider_engage(struct greyhawk_ship_data *ship, struct greyhawk_ship_data *target)
{
  double range;
  int speed;

  if (!vessel_raider_has_ammo(ship) || vessel_breached_arcs(ship) > 0)
  {
    ship->raider_mode = VESSEL_RAIDER_RUNNING;
    return;
  }
  ship->last_attacker = target->shipnum; /* Her guns fire as NPC return fire */
  range = vessel_range_between(ship, target);
  if (vessel_raider_rams(ship, target, range))
  {
    vessel_ram(ship, target);
  }

  if (ship->raider_boarded != target->shipnum && vessel_raider_boardable(target))
  {
    if (range < 1.0 && vessel_display_speed(ship->speed) <= VESSEL_BOARDING_MAX_SPEED)
    {
      vessel_raider_board(ship, target);
      return;
    }
    speed = ship->maxspeed;
    if (range < 1.0)
    {
      speed = VESSEL_BOARDING_MAX_SPEED;
    }
    else if (range < 3.0)
    {
      speed = 12;
    }
    vessel_raider_set_course(ship, vessel_bearing_between(ship, target), speed, 2.0);
    return;
  }

  if (ship->raider_advanced)
  {
    vessel_raider_advanced(ship, target);
  }
  else
  {
    vessel_raider_basic(ship, target);
  }
}

/** Take her own crew, and with `objects` everything else aboard, out of the world. */
static void vessel_raider_clear(struct greyhawk_ship_data *ship, bool objects)
{
  struct char_data *ch;
  struct char_data *next_ch;
  struct obj_data *obj;
  struct obj_data *next_obj;
  room_rnum room;
  int i;

  for (i = 0; i < ship->num_rooms && i < MAX_SHIP_ROOMS; i++)
  {
    room = real_room(ship->room_vnums[i]);
    if (room == NOWHERE)
    {
      continue;
    }
    for (ch = world[room].people; ch != NULL; ch = next_ch)
    {
      next_ch = ch->next_in_room;
      if (IS_NPC(ch) && GET_MOB_VNUM(ch) >= VESSEL_RAIDER_CAPTAIN_VNUM &&
          GET_MOB_VNUM(ch) < VESSEL_RAIDER_CREW_VNUM + VESSEL_RAIDER_TIERS)
      {
        extract_char(ch);
      }
    }
    for (obj = world[room].contents; objects && obj != NULL; obj = next_obj)
    {
      next_obj = obj->next_content;
      extract_obj(obj);
    }
  }
  if (ship->autopilot != NULL)
  {
    ship->autopilot->pilot_mob_vnum = -1; /* Her captain is gone */
  }
}

/** A player aboard her, or a player's hull in her sight. */
static bool vessel_raider_witnessed(struct greyhawk_ship_data *ship)
{
  struct vessel_contact contacts[VESSEL_CONTACT_DISPLAY_LIMIT];
  struct greyhawk_ship_data *other;
  struct char_data *ch;
  room_rnum room;
  int count;
  int i;

  for (i = 0; i < ship->num_rooms && i < MAX_SHIP_ROOMS; i++)
  {
    room = real_room(ship->room_vnums[i]);
    for (ch = room != NOWHERE ? world[room].people : NULL; ch != NULL; ch = ch->next_in_room)
    {
      if (!IS_NPC(ch))
      {
        return TRUE;
      }
    }
  }
  count = MIN(vessel_collect_contacts(ship, contacts, VESSEL_CONTACT_DISPLAY_LIMIT),
              VESSEL_CONTACT_DISPLAY_LIMIT);
  for (i = 0; i < count; i++)
  {
    other = &greyhawk_ships[contacts[i].shipnum];
    if (other->owner[0] != '\0' && !vessel_ship_is_in_port(other))
    {
      return TRUE;
    }
  }
  return FALSE;
}

/**
 * Count down to her leaving the sea, 600 ticks after she lost her quarry,
 * 20 more at a time while she is watched. She takes her crew and whatever
 * is aboard with her; her slot is free once she has gone.
 */
static void vessel_raider_countdown(struct greyhawk_ship_data *ship)
{
  if (ship->raider_ticks == 0)
  {
    ship->raider_ticks = VESSEL_RAIDER_DESPAWN_TICKS;
  }
  ship->raider_ticks--;
  if (ship->raider_ticks > 0)
  {
    return;
  }
  if (vessel_raider_witnessed(ship))
  {
    ship->raider_ticks = VESSEL_RAIDER_WITNESS_TICKS;
    return;
  }
  log("Info: Raider %d '%s' leaves the sea", ship->shipnum, ship->name);
  vessel_raider_clear(ship, TRUE);
  vessel_retire_npc_hull(ship->shipnum, NULL);
}

/**
 * An NPC merchant under fire (3.3.8) runs from her attacker while her crew
 * is at battle stations, her guns answering as NPC return fire; when they
 * stand down her autopilot takes her back to her route.
 */
static void vessel_raider_merchant_run(struct greyhawk_ship_data *ship)
{
  struct greyhawk_ship_data *attacker;

  if (!vessel_at_battle_stations(ship) || get_pilot_from_ship(ship) == NULL ||
      ship->last_attacker <= 0 || ship->last_attacker >= GREYHAWK_MAXSHIPS)
  {
    return;
  }
  attacker = &greyhawk_ships[ship->last_attacker];
  if (is_valid_ship(attacker) &&
      vessel_range_between(ship, attacker) <= (double)vessel_sight_range(ship))
  {
    vessel_raider_set_course(ship, vessel_bearing_between(attacker, ship), ship->maxspeed, 10.0);
  }
}

/**
 * The raider tick: roll an ambush for a player's hull, run an NPC merchant
 * under fire, and give a raider her orders. Runs after the autopilot, so a
 * running merchant's course stands.
 */
void vessel_raider_tick_one(struct greyhawk_ship_data *ship)
{
  struct greyhawk_ship_data *target;
  bool hunter;
  int odds;
  int tier;

  if (!is_valid_ship(ship))
  {
    return;
  }
  if (ship->merchant_id > 0)
  {
    vessel_raider_merchant_run(ship);
    return;
  }
  if (ship->raider_mode == 0)
  {
    odds = vessel_raider_ambush_odds(ship);
    if (odds > 0 && rand_number(1, odds) == 1)
    {
      ship->raided = TRUE;
      tier = vessel_raider_pick_tier(ship, &hunter);
      if (tier >= 0)
      {
        vessel_raider_spawn(ship, tier, hunter);
      }
    }
    return;
  }

  /* Without her captain nobody gives orders, and her guns fall silent. */
  if (get_pilot_from_ship(ship) == NULL)
  {
    if (ship->autopilot != NULL)
    {
      ship->autopilot->pilot_mob_vnum = -1;
    }
    ship->setspeed = 0;
    ship->raider_target = 0;
    vessel_raider_countdown(ship);
    return;
  }
  if (vessel_is_sinking(ship) || vessel_crew_stunned(ship))
  {
    return;
  }

  target = NULL;
  if (ship->raider_target > 0 && ship->raider_target < GREYHAWK_MAXSHIPS &&
      vessel_raider_can_pursue(ship, &greyhawk_ships[ship->raider_target]))
  {
    target = &greyhawk_ships[ship->raider_target];
  }
  if (target == NULL &&
      (ship->raider_mode == VESSEL_RAIDER_ENGAGING || ship->raider_mode == VESSEL_RAIDER_RUNNING))
  {
    ship->raider_mode = VESSEL_RAIDER_CRUISING;
    ship->raider_target = 0;
    ship->last_attacker = 0;
  }
  if (ship->raider_mode == VESSEL_RAIDER_CRUISING)
  {
    target = vessel_raider_find_quarry(ship);
    if (target != NULL)
    {
      ship->raider_mode = VESSEL_RAIDER_ENGAGING;
      ship->raider_target = target->shipnum;
    }
  }

  if (target == NULL)
  {
    /* Cruising or leaving: she sails on, clear of land */
    vessel_raider_set_course(ship, ship->heading, ship->maxspeed, 5.0);
    vessel_raider_countdown(ship);
    return;
  }
  ship->raider_ticks = 0;
  if (ship->raider_mode == VESSEL_RAIDER_RUNNING)
  {
    vessel_raider_set_course(ship, vessel_bearing_between(target, ship), ship->maxspeed, 10.0);
  }
  else
  {
    vessel_raider_engage(ship, target);
  }
}

/** A raider going down takes her crew with her, rather than leave them swimming. */
void vessel_raider_handle_sink(struct greyhawk_ship_data *ship)
{
  if (ship->raider_mode != 0)
  {
    vessel_raider_clear(ship, FALSE);
  }
}

/**
 * Raiders are never kept (3.3.8): retire every public hull a restart
 * restored from a raider prototype.
 */
void vessel_raider_boot(void)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  int prototype_id;
  int retired;
  int i;

  if (!mysql_available || conn == NULL)
  {
    return;
  }
  if (mysql_query(conn, "SELECT DISTINCT prototype_id FROM vessel_raider_tiers"))
  {
    log("SYSERR: Could not read the raider tiers: %s", mysql_error(conn));
    return;
  }
  result = mysql_store_result(conn);
  if (result == NULL)
  {
    return;
  }

  retired = 0;
  while ((row = mysql_fetch_row(result)) != NULL)
  {
    prototype_id = parse_int(row[0]);
    for (i = 2; i < GREYHAWK_MAXSHIPS; i++)
    {
      if (is_valid_ship(&greyhawk_ships[i]) && greyhawk_ships[i].owner[0] == '\0' &&
          greyhawk_ships[i].prototype_id == prototype_id && vessel_retire_npc_hull(i, NULL))
      {
        retired++;
      }
    }
  }
  mysql_free_result(result);
  if (retired > 0)
  {
    log("Info: Retired %d raider%s restored by the restart", retired, retired == 1 ? "" : "s");
  }
}
