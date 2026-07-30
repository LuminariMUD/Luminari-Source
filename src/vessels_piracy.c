/* ************************************************************************
 *      File:   vessels_piracy.c                      Part of LuminariMUD  *
 *   Purpose:   Piracy, plunder, and bounty (Phase 07, Session 05).        *
 *              Taking cargo by force pays now and costs later: bounty     *
 *              closes lawful ports and eventually draws the navy. A       *
 *              letter of marque makes the same act legal.                 *
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
#include "mysql.h"

extern MYSQL *conn;
extern bool mysql_available;
extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];
extern struct room_data *world;

/* A letter of marque costs this multiple of the WANTED threshold */
#define MARQUE_COST (BOUNTY_WANTED * 4)

/* Marque duration in real seconds (one real day) */
#define MARQUE_DURATION 86400

struct vessel_piracy_law_cache_entry
{
  bool valid;
  int region_vnum;
  int waters_type;
  int priority;
  int bounty_percent;
  char authority[64];
};

static struct vessel_piracy_law_cache_entry *vessel_law_cache = NULL;
static size_t vessel_law_cache_count = 0;

/**
 * Release the builder-authored vessel-law cache.
 */
void vessel_piracy_clear_laws(void)
{
  free(vessel_law_cache);
  vessel_law_cache = NULL;
  vessel_law_cache_count = 0;
}

/**
 * Reload vessel-law metadata while wilderness geometry remains in the
 * canonical in-memory region table.
 */
bool vessel_piracy_reload_laws(void)
{
  struct vessel_piracy_law_cache_entry *new_cache;
  MYSQL_RES *result;
  MYSQL_ROW row;
  my_ulonglong row_count;
  size_t index;
  int invalid_count;
  int ship_index;

  if (!mysql_available || conn == NULL)
  {
    return FALSE;
  }

  if (mysql_query(conn,
                  "SELECT region_vnum, waters_type, priority, bounty_percent, authority "
                  "FROM vessel_region_law ORDER BY region_vnum"))
  {
    log("SYSERR: vessel law cache query failed: %s", mysql_error(conn));
    return FALSE;
  }

  result = mysql_store_result(conn);
  if (result == NULL)
  {
    log("SYSERR: vessel law cache result failed: %s", mysql_error(conn));
    return FALSE;
  }

  row_count = mysql_num_rows(result);
  if (row_count > (my_ulonglong)(SIZE_MAX / sizeof(*new_cache)))
  {
    log("SYSERR: vessel law cache is too large to allocate");
    mysql_free_result(result);
    return FALSE;
  }

  new_cache = NULL;
  if (row_count > 0)
  {
    CREATE(new_cache, struct vessel_piracy_law_cache_entry, (size_t)row_count);
  }

  index = 0;
  invalid_count = 0;
  while ((row = mysql_fetch_row(result)) != NULL && index < (size_t)row_count)
  {
    new_cache[index].region_vnum = row[0] ? atoi(row[0]) : 0;
    new_cache[index].waters_type = row[1] ? atoi(row[1]) : VESSEL_WATERS_UNCLAIMED;
    new_cache[index].priority = row[2] ? atoi(row[2]) : 0;
    new_cache[index].bounty_percent = row[3] ? atoi(row[3]) : 100;
    if (row[4] != NULL && *row[4])
    {
      strlcpy(new_cache[index].authority, row[4],
              sizeof(new_cache[index].authority));
    }
    else
    {
      strlcpy(new_cache[index].authority, "maritime law",
              sizeof(new_cache[index].authority));
    }

    new_cache[index].valid =
        new_cache[index].region_vnum > 0 &&
        new_cache[index].waters_type >= VESSEL_WATERS_TERRITORIAL &&
        new_cache[index].waters_type <= VESSEL_WATERS_PIRATE_COVE &&
        new_cache[index].bounty_percent >= 0 &&
        new_cache[index].bounty_percent <= VESSEL_PIRACY_BOUNTY_PERCENT_MAX;
    if (!new_cache[index].valid)
    {
      invalid_count++;
      log("SYSERR: Ignoring invalid vessel law for region %d (type %d, bounty %d%%)",
          new_cache[index].region_vnum, new_cache[index].waters_type,
          new_cache[index].bounty_percent);
    }
    index++;
  }
  mysql_free_result(result);

  vessel_piracy_clear_laws();
  vessel_law_cache = new_cache;
  vessel_law_cache_count = index;
  for (ship_index = 0; ship_index < GREYHAWK_MAXSHIPS; ship_index++)
  {
    if (is_valid_ship(&greyhawk_ships[ship_index]))
    {
      greyhawk_ships[ship_index].waters_region_initialized = FALSE;
      vessel_piracy_track_waters(&greyhawk_ships[ship_index], FALSE);
    }
  }
  log("Info: Loaded %zu vessel law row%s (%d invalid)", index,
      index == 1 ? "" : "s", invalid_count);
  return TRUE;
}

/**
 * Create the bounty table.
 * Mirrored by sql/components/vessels_phase7_schema.sql.
 */
void vessel_piracy_ensure_schema(void)
{
  if (!mysql_available || conn == NULL)
  {
    return;
  }

  if (mysql_query(conn, "CREATE TABLE IF NOT EXISTS vessel_bounties ("
                        "  player_name VARCHAR(64) PRIMARY KEY,"
                        "  bounty INT NOT NULL DEFAULT 0,"
                        "  marque_until INT NOT NULL DEFAULT 0,"
                        "  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP "
                        "    ON UPDATE CURRENT_TIMESTAMP"
                        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4"))
  {
    log("SYSERR: vessel_bounties create failed: %s", mysql_error(conn));
  }

  if (mysql_query(conn, "CREATE TABLE IF NOT EXISTS vessel_region_law ("
                        "  region_vnum INT PRIMARY KEY,"
                        "  waters_type TINYINT NOT NULL,"
                        "  priority INT NOT NULL DEFAULT 0,"
                        "  bounty_percent INT NOT NULL DEFAULT 100,"
                        "  authority VARCHAR(63) NOT NULL DEFAULT '',"
                        "  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP "
                        "    ON UPDATE CURRENT_TIMESTAMP,"
                        "  INDEX idx_vessel_region_law_priority (priority)"
                        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4"))
  {
    log("SYSERR: vessel_region_law create failed: %s", mysql_error(conn));
  }

  vessel_piracy_reload_laws();
}

/**
 * Player-facing name for one regional water-law classification.
 */
const char *vessel_waters_type_name(int waters_type)
{
  switch (waters_type)
  {
  case VESSEL_WATERS_TERRITORIAL:
    return "territorial waters";
  case VESSEL_WATERS_FREE:
    return "free seas";
  case VESSEL_WATERS_PIRATE_COVE:
    return "pirate cove";
  case VESSEL_WATERS_UNCLAIMED:
  default:
    return "unclaimed waters";
  }
}

/**
 * Scale the normal cargo bounty by a builder-authored regional percentage.
 */
int vessel_piracy_bounty_for_units(int cargo_units, int bounty_percent)
{
  long long amount;

  if (cargo_units <= 0)
  {
    return 0;
  }
  if (bounty_percent < 0)
  {
    bounty_percent = 100;
  }
  if (bounty_percent == 0)
  {
    return 0;
  }

  bounty_percent = MIN(bounty_percent, VESSEL_PIRACY_BOUNTY_PERCENT_MAX);
  amount = (long long)cargo_units * BOUNTY_PER_CARGO_UNIT * bounty_percent / 100;
  if (amount < 1)
  {
    return 1;
  }
  if (amount > INT_MAX)
  {
    return INT_MAX;
  }
  return (int)amount;
}

/**
 * Pirate-cove services are the sole regional exception to WANTED refusal.
 */
bool vessel_piracy_wanted_port_is_open(const struct vessel_piracy_law *law)
{
  return law != NULL && law->configured &&
         law->waters_type == VESSEL_WATERS_PIRATE_COVE;
}

/**
 * Locate cached law metadata by region VNUM.
 */
static const struct vessel_piracy_law_cache_entry *vessel_piracy_cached_law(
    int region_vnum)
{
  size_t low;
  size_t high;
  size_t middle;

  low = 0;
  high = vessel_law_cache_count;
  while (low < high)
  {
    middle = low + (high - low) / 2;
    if (vessel_law_cache[middle].region_vnum == region_vnum)
    {
      return &vessel_law_cache[middle];
    }
    if (vessel_law_cache[middle].region_vnum < region_vnum)
    {
      low = middle + 1;
    }
    else
    {
      high = middle;
    }
  }

  return NULL;
}

/**
 * Test one point against an in-memory wilderness polygon.
 *
 * Points on an edge or vertex are outside, matching MariaDB ST_Within().
 */
bool vessel_piracy_point_in_polygon(const struct vertex *vertices, int vertex_count,
                                    int x, int y)
{
  bool inside;
  long long cross_product;
  double intersection_x;
  int current;
  int previous;
  int current_x;
  int current_y;
  int previous_x;
  int previous_y;

  if (vertices == NULL || vertex_count < 3)
  {
    return FALSE;
  }

  inside = FALSE;
  previous = vertex_count - 1;
  for (current = 0; current < vertex_count; current++)
  {
    current_x = vertices[current].x;
    current_y = vertices[current].y;
    previous_x = vertices[previous].x;
    previous_y = vertices[previous].y;

    cross_product =
        ((long long)x - current_x) * ((long long)previous_y - current_y) -
        ((long long)y - current_y) * ((long long)previous_x - current_x);
    if (cross_product == 0 &&
        x >= MIN(current_x, previous_x) && x <= MAX(current_x, previous_x) &&
        y >= MIN(current_y, previous_y) && y <= MAX(current_y, previous_y))
    {
      return FALSE;
    }

    if ((current_y > y) != (previous_y > y))
    {
      intersection_x =
          ((double)previous_x - current_x) * ((double)y - current_y) /
              ((double)previous_y - current_y) +
          current_x;
      if ((double)x < intersection_x)
      {
        inside = !inside;
      }
    }
    previous = current;
  }

  return inside;
}

/**
 * Resolve the highest-priority vessel law covering one wilderness coordinate.
 *
 * Geometry comes from the canonical region table loaded at boot. Law metadata
 * is cached separately, so ordinary movement never executes SQL. A configured
 * region outranks an unconfigured overlapping name; equal priorities use the
 * lowest region VNUM for deterministic results.
 */
bool vessel_piracy_law_at_coordinates(int x, int y, struct vessel_piracy_law *law)
{
  const struct vessel_piracy_law_cache_entry *entry;
  const struct vessel_piracy_law_cache_entry *best_entry;
  const struct region_data *region;
  const struct region_data *best_region;
  bool configured;
  bool best_configured;
  int priority;
  int best_priority;
  region_rnum i;

  if (law == NULL)
  {
    return FALSE;
  }

  memset(law, 0, sizeof(*law));
  law->waters_type = VESSEL_WATERS_UNCLAIMED;
  law->bounty_percent = 100;
  strlcpy(law->region_name, "Unnamed open waters", sizeof(law->region_name));
  strlcpy(law->authority, "maritime law", sizeof(law->authority));

  if (region_table == NULL || zone_table == NULL || top_of_region_table == NOWHERE)
  {
    return FALSE;
  }

  best_region = NULL;
  best_entry = NULL;
  best_configured = FALSE;
  best_priority = 0;
  for (i = 0; i <= top_of_region_table; i++)
  {
    region = &region_table[i];
    if (region->region_type != REGION_GEOGRAPHIC || region->zone == NOWHERE ||
        region->zone > top_of_zone_table ||
        zone_table[region->zone].number != WILD_ZONE_VNUM ||
        !vessel_piracy_point_in_polygon(region->vertices, region->num_vertices, x, y))
    {
      continue;
    }

    entry = vessel_piracy_cached_law(region->vnum);
    configured = entry != NULL;
    priority = configured ? entry->priority : 0;
    if (best_region == NULL ||
        (configured && !best_configured) ||
        (configured == best_configured &&
         (priority > best_priority ||
          (priority == best_priority && region->vnum < best_region->vnum))))
    {
      best_region = region;
      best_entry = entry;
      best_configured = configured;
      best_priority = priority;
    }
  }

  if (best_region == NULL)
  {
    return FALSE;
  }

  law->region_vnum = best_region->vnum;
  if (best_region->name != NULL && *best_region->name)
  {
    strlcpy(law->region_name, best_region->name, sizeof(law->region_name));
  }
  if (best_entry != NULL && best_entry->valid)
  {
    law->configured = TRUE;
    law->waters_type = best_entry->waters_type;
    law->priority = best_entry->priority;
    law->bounty_percent = best_entry->bounty_percent;
    strlcpy(law->authority, best_entry->authority, sizeof(law->authority));
  }

  return TRUE;
}

/**
 * Resolve regional law at a live vessel's authoritative wilderness position.
 */
bool vessel_piracy_law_for_ship(const struct greyhawk_ship_data *ship,
                                struct vessel_piracy_law *law)
{
  if (ship == NULL)
  {
    if (law != NULL)
    {
      memset(law, 0, sizeof(*law));
    }
    return FALSE;
  }

  return vessel_piracy_law_at_coordinates((int)ship->x, (int)ship->y, law);
}

/**
 * Track and optionally announce a vessel's canonical named-water boundary.
 */
void vessel_piracy_track_waters(struct greyhawk_ship_data *ship, bool announce)
{
  struct vessel_piracy_law law;
  bool found;
  int current_region_vnum;

  if (ship == NULL)
  {
    return;
  }

  found = vessel_piracy_law_for_ship(ship, &law);
  current_region_vnum = found ? law.region_vnum : 0;
  if (!ship->waters_region_initialized)
  {
    ship->waters_region_vnum = current_region_vnum;
    ship->waters_region_initialized = TRUE;
    return;
  }
  if (ship->waters_region_vnum == current_region_vnum)
  {
    return;
  }

  ship->waters_region_vnum = current_region_vnum;
  if (!announce)
  {
    return;
  }

  if (found && law.configured)
  {
    send_to_ship(ship, "The charts mark our crossing into %s (%s), under %s authority.",
                 law.region_name, vessel_waters_type_name(law.waters_type),
                 law.authority);
  }
  else if (found)
  {
    send_to_ship(ship, "The charts mark our crossing into %s.", law.region_name);
  }
  else
  {
    send_to_ship(ship,
                 "The charted boundary falls astern; the vessel enters unnamed open waters.");
  }
}

/**
 * Read a player's outstanding bounty.
 *
 * @return Bounty in gold, 0 if none or on error
 */
int vessel_get_bounty(const char *player_name)
{
  char query[MAX_STRING_LENGTH];
  char escaped[130];
  MYSQL_RES *result;
  MYSQL_ROW row;
  int bounty = 0;

  if (!mysql_available || conn == NULL || player_name == NULL || !*player_name)
  {
    return 0;
  }

  mysql_real_escape_string(conn, escaped, player_name, strlen(player_name));
  snprintf(query, sizeof(query), "SELECT bounty FROM vessel_bounties WHERE player_name = '%s'",
           escaped);
  if (mysql_query(conn, query))
  {
    return 0;
  }

  result = mysql_store_result(conn);
  if (result == NULL)
  {
    return 0;
  }

  row = mysql_fetch_row(result);
  if (row != NULL && row[0] != NULL)
  {
    bounty = atoi(row[0]);
  }
  mysql_free_result(result);

  return bounty;
}

/**
 * Add to a player's bounty (creating the record if needed).
 */
void vessel_add_bounty(const char *player_name, int amount)
{
  char query[MAX_STRING_LENGTH];
  char escaped[130];

  if (!mysql_available || conn == NULL || player_name == NULL || !*player_name || amount <= 0)
  {
    return;
  }

  mysql_real_escape_string(conn, escaped, player_name, strlen(player_name));
  snprintf(query, sizeof(query),
           "INSERT INTO vessel_bounties (player_name, bounty) VALUES ('%s', %d) "
           "ON DUPLICATE KEY UPDATE bounty = LEAST(%d, "
           "CAST(bounty AS DECIMAL(20,0)) + %d)",
           escaped, amount, INT_MAX, amount);
  if (mysql_query(conn, query))
  {
    log("SYSERR: vessel_add_bounty failed: %s", mysql_error(conn));
    return;
  }

  log("Info: %s bounty increased by %d gold", player_name, amount);
}

/**
 * Clear a player's bounty (pardon or paid off).
 */
void vessel_clear_bounty(const char *player_name)
{
  char query[MAX_STRING_LENGTH];
  char escaped[130];

  if (!mysql_available || conn == NULL || player_name == NULL || !*player_name)
  {
    return;
  }

  mysql_real_escape_string(conn, escaped, player_name, strlen(player_name));
  snprintf(query, sizeof(query), "UPDATE vessel_bounties SET bounty = 0 WHERE player_name = '%s'",
           escaped);
  if (mysql_query(conn, query))
  {
    log("SYSERR: vessel_clear_bounty failed: %s", mysql_error(conn));
  }
}

/**
 * Does this player hold a valid letter of marque?
 */
bool vessel_has_letter_of_marque(const char *player_name)
{
  char query[MAX_STRING_LENGTH];
  char escaped[130];
  MYSQL_RES *result;
  MYSQL_ROW row;
  bool valid = FALSE;

  if (!mysql_available || conn == NULL || player_name == NULL || !*player_name)
  {
    return FALSE;
  }

  mysql_real_escape_string(conn, escaped, player_name, strlen(player_name));
  snprintf(query, sizeof(query),
           "SELECT marque_until FROM vessel_bounties WHERE player_name = '%s'", escaped);
  if (mysql_query(conn, query))
  {
    return FALSE;
  }

  result = mysql_store_result(conn);
  if (result == NULL)
  {
    return FALSE;
  }

  row = mysql_fetch_row(result);
  if (row != NULL && row[0] != NULL)
  {
    valid = (atoi(row[0]) > (int)time(0));
  }
  mysql_free_result(result);

  return valid;
}

/**
 * Will a lawful port refuse this character its services?
 *
 * WANTED pirates get no market, no freight work, no crew hall, and no
 * shipyard. A letter of marque restores standing. Called from every
 * port-service gate so the consequence is uniform.
 *
 * @return TRUE if refused (message already sent to ch)
 */
bool vessel_port_refuses(struct char_data *ch)
{
  struct greyhawk_ship_data *ship;
  struct vessel_piracy_law law;
  room_rnum room;
  bool law_found;
  int bounty;

  if (ch == NULL || IS_NPC(ch) || GET_LEVEL(ch) >= LVL_IMMORT)
  {
    return FALSE;
  }

  bounty = vessel_get_bounty(GET_NAME(ch));
  if (bounty < BOUNTY_WANTED)
  {
    return FALSE;
  }

  if (vessel_has_letter_of_marque(GET_NAME(ch)))
  {
    return FALSE;
  }

  room = IN_ROOM(ch);
  ship = room != NOWHERE ? get_ship_from_room(room) : NULL;
  law_found = FALSE;
  if (ship != NULL)
  {
    law_found = vessel_piracy_law_for_ship(ship, &law);
  }
  else if (room != NOWHERE)
  {
    law_found =
        vessel_piracy_law_at_coordinates(world[room].coords[0], world[room].coords[1], &law);
  }
  if (law_found)
  {
    if (vessel_piracy_wanted_port_is_open(&law))
    {
      return FALSE;
    }
  }

  send_to_char(ch,
               "The harbourmaster knows your face - %d gold is posted for you here. "
               "No lawful business will be done with you in this port.\r\n",
               bounty);
  return TRUE;
}

/**
 * Move cargo from a prize ship into the raider's hold.
 *
 * Takes as much as the raider can carry, lot by lot. Unlawful plunder
 * accrues bounty per unit; a letter of marque exempts the raider.
 *
 * @return Units transferred
 */
int vessel_plunder_cargo(struct char_data *ch, struct greyhawk_ship_data *prize,
                         struct greyhawk_ship_data *raider)
{
  int capacity;
  int taken = 0;
  int i, j;

  if (ch == NULL || prize == NULL || raider == NULL || prize == raider)
  {
    return 0;
  }

  capacity = vessel_effective_cargo_capacity(raider);

  for (i = 0; i < MAX_CARGO_LOTS; i++)
  {
    if (prize->cargo[i].commodity_id == 0 || prize->cargo[i].quantity <= 0)
    {
      continue;
    }

    /* Find or claim a bay in the raider's hold for this commodity */
    for (j = 0; j < MAX_CARGO_LOTS; j++)
    {
      if (raider->cargo[j].commodity_id == prize->cargo[i].commodity_id ||
          raider->cargo[j].commodity_id == 0)
      {
        break;
      }
    }
    if (j >= MAX_CARGO_LOTS)
    {
      continue; /* No room for this kind of goods */
    }

    raider->cargo[j].commodity_id = prize->cargo[i].commodity_id;

    /* Move units one at a time so the weight limit stops us exactly at
     * capacity rather than overshooting. */
    while (prize->cargo[i].quantity > 0)
    {
      raider->cargo[j].quantity++;
      if (vessel_cargo_weight(raider) > capacity)
      {
        raider->cargo[j].quantity--;
        break;
      }
      prize->cargo[i].quantity--;
      taken++;
    }

    if (prize->cargo[i].quantity <= 0)
    {
      prize->cargo[i].commodity_id = 0;
    }
    if (raider->cargo[j].quantity <= 0)
    {
      raider->cargo[j].commodity_id = 0;
    }
  }

  if (taken > 0)
  {
    vessel_db_save_cargo(prize);
    vessel_db_save_cargo(raider);
  }

  return taken;
}

/**
 * plunder - strip cargo from a ship whose bridge you hold.
 */
ACMD(do_plunder)
{
  struct greyhawk_ship_data *prize;
  struct greyhawk_ship_data *raider = NULL;
  struct vessel_piracy_law law;
  struct char_data *tch;
  bool law_found;
  int taken;
  int bounty;
  int i;

  prize = get_ship_from_room(IN_ROOM(ch));
  if (prize == NULL)
  {
    send_to_char(ch, "You must be aboard a ship to plunder her.\r\n");
    return;
  }

  if (world[IN_ROOM(ch)].number != (room_vnum)prize->bridge_room)
  {
    send_to_char(ch, "Take the bridge first - that is where a prize is claimed.\r\n");
    return;
  }

  if (!str_cmp(prize->owner, GET_NAME(ch)))
  {
    send_to_char(ch, "You cannot plunder your own ship; use 'cargosell' in port.\r\n");
    return;
  }

  /* Taking another player's cargo is a hostile act against them, so it
   * answers to the same consent rules as attacking them directly. */
  if (!vessel_pvp_permitted(ch, prize, TRUE))
  {
    return;
  }

  /* The bridge must be uncontested */
  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (tch != ch && GET_POS(tch) > POS_STUNNED)
    {
      send_to_char(ch, "%s still holds the bridge against you.\r\n", PERS(tch, ch));
      return;
    }
  }

  /* Find the raider's own ship: the nearest vessel docked to this one */
  if (prize->docked_to_ship >= 0 && prize->docked_to_ship < GREYHAWK_MAXSHIPS)
  {
    raider = &greyhawk_ships[prize->docked_to_ship];
  }
  else
  {
    for (i = 0; i < GREYHAWK_MAXSHIPS; i++)
    {
      if (is_valid_ship(&greyhawk_ships[i]) && &greyhawk_ships[i] != prize &&
          !str_cmp(greyhawk_ships[i].owner, GET_NAME(ch)) &&
          ships_in_docking_range(&greyhawk_ships[i], prize))
      {
        raider = &greyhawk_ships[i];
        break;
      }
    }
  }

  if (raider == NULL)
  {
    send_to_char(ch, "You need your own ship alongside to carry off the cargo.\r\n");
    return;
  }

  taken = vessel_plunder_cargo(ch, prize, raider);
  if (taken == 0)
  {
    send_to_char(ch, "There is nothing worth taking, or no room to take it.\r\n");
    return;
  }

  send_to_char(ch, "You transfer %d units of cargo from %s to %s.\r\n", taken, prize->name,
               raider->name);
  send_to_ship(prize, "Raiders strip %d units of cargo from the hold!", taken);
  send_to_ship(raider, "Plundered cargo comes aboard: %d units.", taken);

  law_found = vessel_piracy_law_for_ship(prize, &law);
  bounty = vessel_piracy_bounty_for_units(taken, law.bounty_percent);

  /* Regional law comes from shared wilderness geography. Pirate coves may
   * waive the bounty; a valid marque waives any positive regional penalty. */
  if (bounty == 0)
  {
    send_to_char(ch, "No authority claims this prize%s%s.\r\n",
                 law_found ? " in " : "",
                 law_found ? law.region_name : "");
  }
  else if (vessel_has_letter_of_marque(GET_NAME(ch)))
  {
    send_to_char(ch, "Your letter of marque makes this a lawful prize.\r\n");
  }
  else
  {
    vessel_add_bounty(GET_NAME(ch), bounty);
    if (law.configured)
    {
      send_to_char(ch,
                   "Under %s authority in %s, word of the raid spreads - "
                   "your bounty rises by %d gold.\r\n",
                   law.authority, law.region_name, bounty);
    }
    else
    {
      send_to_char(ch, "Word of the raid will spread - your bounty rises by %d gold.\r\n",
                   bounty);
    }

    bounty = vessel_get_bounty(GET_NAME(ch));
    if (bounty >= BOUNTY_HUNTED)
    {
      send_to_char(ch, "You are now HUNTED: the navy has orders to sink you on sight.\r\n");
    }
    else if (bounty >= BOUNTY_WANTED)
    {
      send_to_char(ch, "You are now WANTED: lawful ports will refuse you a berth.\r\n");
    }
  }

  log("Info: %s plundered %d units from ship %d '%s'", GET_NAME(ch), taken, prize->shipnum,
      prize->name);
}

/**
 * bounty [<player>] - check your bounty, or another's.
 */
ACMD(do_bounty)
{
  char arg[MAX_INPUT_LENGTH];
  const char *target;
  int bounty;

  if (!mysql_available || conn == NULL)
  {
    send_to_char(ch, "No bounty records are available.\r\n");
    return;
  }

  one_argument_u((char *)argument, arg);
  target = *arg ? arg : GET_NAME(ch);

  bounty = vessel_get_bounty(target);
  if (bounty <= 0)
  {
    send_to_char(ch, "%s carries no price.\r\n", *arg ? CAP(arg) : "You");
    return;
  }

  send_to_char(
      ch, "%s: %d gold on %s head%s.\r\n", *arg ? CAP(arg) : "You", bounty, *arg ? "their" : "your",
      bounty >= BOUNTY_HUNTED ? " - HUNTED by the navy"
                              : (bounty >= BOUNTY_WANTED ? " - WANTED in lawful ports" : ""));

  if (!*arg && vessel_has_letter_of_marque(GET_NAME(ch)))
  {
    send_to_char(ch, "You hold a valid letter of marque.\r\n");
  }
}

/**
 * marque - buy a letter of marque, or check the one you hold.
 */
ACMD(do_marque)
{
  char query[MAX_STRING_LENGTH];
  char escaped[130];
  int expiry;

  if (!mysql_available || conn == NULL)
  {
    send_to_char(ch, "The admiralty office is closed.\r\n");
    return;
  }

  if (!vessel_room_is_port(IN_ROOM(ch)))
  {
    send_to_char(ch, "Letters of marque are issued at a port's admiralty office.\r\n");
    return;
  }

  if (vessel_has_letter_of_marque(GET_NAME(ch)))
  {
    send_to_char(ch, "You already hold a valid letter of marque.\r\n");
    return;
  }

  if (vessel_get_bounty(GET_NAME(ch)) >= BOUNTY_WANTED)
  {
    send_to_char(ch, "The admiralty does not commission wanted pirates. Settle your bounty "
                     "first.\r\n");
    return;
  }

  if (GET_GOLD(ch) < MARQUE_COST)
  {
    send_to_char(ch, "A letter of marque costs %d gold; you have %d.\r\n", MARQUE_COST,
                 GET_GOLD(ch));
    return;
  }

  expiry = (int)time(0) + MARQUE_DURATION;
  mysql_real_escape_string(conn, escaped, GET_NAME(ch), strlen(GET_NAME(ch)));
  snprintf(query, sizeof(query),
           "INSERT INTO vessel_bounties (player_name, marque_until) VALUES ('%s', %d) "
           "ON DUPLICATE KEY UPDATE marque_until = %d",
           escaped, expiry, expiry);
  if (mysql_query(conn, query))
  {
    log("SYSERR: marque issue failed: %s", mysql_error(conn));
    send_to_char(ch, "The clerk cannot complete the commission.\r\n");
    return;
  }

  GET_GOLD(ch) -= MARQUE_COST;
  send_to_char(ch,
               "You pay %d gold. The admiralty commissions you as a privateer - prizes "
               "taken now are lawful.\r\n",
               MARQUE_COST);
  log("Info: %s bought a letter of marque for %d gold", GET_NAME(ch), MARQUE_COST);
}
