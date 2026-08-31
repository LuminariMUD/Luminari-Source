/* ************************************************************************
 *      File:   vessels.h                            Part of LuminariMUD  *
 *   Purpose:   Unified Vessel/Vehicle system header                      *
 *  Author:     Zusuk                                                     *
 * ********************************************************************** */

#ifndef _VESSELS_H_
#define _VESSELS_H_

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "olc/oasis.h"
#include "screen.h"
#include "interpreter.h"
#include "modify.h"
#include "handler.h"
#include "constants.h"

struct region_list;
struct vertex;

#define VESSEL_REGION_FEATURE_NAME_LENGTH 128
#define VESSEL_ALTITUDE_LANE_SPEED_PERCENT 125

struct vessel_region_feature
{
  region_vnum region_vnum;
  int region_type;
  int threshold;
  char name[VESSEL_REGION_FEATURE_NAME_LENGTH];
};

/* ========================================================================= */
/* ITEM TYPES FOR VESSEL SYSTEM                                              */
/* ========================================================================= */

#define ITEM_VEHICLE 53 /* Vehicle object - represents the actual vehicle */
#define ITEM_CONTROL 54 /* Control mechanism - steering wheel, helm, etc. */
#define ITEM_HATCH 55   /* Exit/entry point - doorway out of vehicle */

/* ========================================================================= */
/* ROOM FLAGS FOR VESSEL SYSTEM                                              */
/* ========================================================================= */

#define ROOM_VEHICLE 40 /* Room that vehicles can move through */

/* ========================================================================= */
/* FUTURE ADVANCED VESSEL SYSTEM CONSTANTS                                   */
/* ========================================================================= */
/* These constants are for the advanced vessel system, not the CWG system    */

/* Vessel Types */
#define VESSEL_TYPE_SAILING_SHIP 1  /* Ocean-going ships */
#define VESSEL_TYPE_SUBMARINE 2     /* Underwater vessels */
#define VESSEL_TYPE_AIRSHIP 3       /* Flying craft */
#define VESSEL_TYPE_STARSHIP 4      /* Space vessels */
#define VESSEL_TYPE_MAGICAL_CRAFT 5 /* Magically powered vehicles */

/* Vessel States */
#define VESSEL_STATE_DOCKED 0    /* Parked/anchored */
#define VESSEL_STATE_TRAVELING 1 /* Moving between locations */
#define VESSEL_STATE_COMBAT 2    /* In battle */
#define VESSEL_STATE_DAMAGED 3   /* Broken down */

/* Vessel Sizes */
#define VESSEL_SIZE_SMALL 1  /* 1-2 passengers */
#define VESSEL_SIZE_MEDIUM 2 /* 3-6 passengers */
#define VESSEL_SIZE_LARGE 3  /* 7-15 passengers */
#define VESSEL_SIZE_HUGE 4   /* 16+ passengers */

/* ========================================================================= */
/* GREYHAWK SHIP SYSTEM CONSTANTS                                           */
/* ========================================================================= */

#ifndef GREYHAWK_MAXSHIPS
#define GREYHAWK_MAXSHIPS 501 /* Fleet slots, including reserved slot 0 */
#endif

#define GREYHAWK_ACTIVE_SHIP_CAPACITY (GREYHAWK_MAXSHIPS - 1)
#define VESSEL_CUSTOMIZATION_LENGTH 81 /* 80 printable characters plus NUL */

#ifndef GREYHAWK_MAXSLOTS
#define GREYHAWK_MAXSLOTS 10 /* Maximum equipment slots per ship */
#endif

/* Ship Position Constants */
#define GREYHAWK_FORE 0      /* Forward position */
#define GREYHAWK_PORT 1      /* Port (left) position */
#define GREYHAWK_REAR 2      /* Rear position */
#define GREYHAWK_STARBOARD 3 /* Starboard (right) position */

/* Weapon Range Types */
#define GREYHAWK_SHRTRANGE 0 /* Short range */
#define GREYHAWK_MEDRANGE 1  /* Medium range */
#define GREYHAWK_LNGRANGE 2  /* Long range */

/* Item Type for Greyhawk Ships */
#define GREYHAWK_ITEM_SHIP 56 /* Greyhawk ship object type (moved to avoid conflict) */

/* Room flag for dockable areas */
#define DOCKABLE ROOM_DOCKABLE /* Room flag for dockable areas (41) */

/* ========================================================================= */
/* AUTOPILOT SYSTEM CONSTANTS                                                */
/* ========================================================================= */

#define MAX_WAYPOINTS_PER_ROUTE 20 /* Maximum waypoints in a single route */
#define MAX_ROUTES_PER_SHIP 5      /* Maximum routes a ship can store */
#define AUTOPILOT_TICK_INTERVAL 5  /* Ticks between autopilot updates */
#define AUTOPILOT_NAME_LENGTH 64   /* Max length for waypoint/route names */
#define VESSEL_AMBIENT_MESSAGE_COOLDOWN (120 RL_SEC)
#define VESSEL_COMBAT_MESSAGE_COOLDOWN AUTOPILOT_TICK_INTERVAL

/* Crew Role Constants (matches ship_crew_roster.crew_role ENUM) */
#define CREW_ROLE_PILOT "pilot" /* NPC vessel pilot */

/* ========================================================================= */
/* SCHEDULE SYSTEM CONSTANTS                                                 */
/* ========================================================================= */

#define SCHEDULE_INTERVAL_MIN 1          /* Minimum schedule interval (MUD hours) */
#define SCHEDULE_INTERVAL_MAX 24         /* Maximum schedule interval (MUD hours) */
#define SCHEDULE_NAME_LENGTH 64          /* Max length for schedule names */
#define VESSEL_PASSENGER_FARE_MAX 100000 /* Maximum gold charged for one boarding */

/* Schedule State Flags */
#define SCHEDULE_FLAG_ENABLED (1 << 0) /* Schedule is active */
#define SCHEDULE_FLAG_PAUSED (1 << 1)  /* Schedule temporarily paused */

/* ========================================================================= */
/* DIRECTION CONSTANTS                                                       */
/* ========================================================================= */

#ifndef OUTDIR
#define OUTDIR 6 /* Generic "out" direction for vehicles */
#endif

/* ========================================================================= */
/* DEBUG LOGGING SYSTEM                                                       */
/* ========================================================================= */
/* Comprehensive debug logging for vessel/vehicle system troubleshooting.    */
/* Set VESSEL_SYSTEM_DEBUG to 1 to enable, 0 for zero-overhead production.   */
/*                                                                           */
/* Usage:                                                                    */
/*   - Enable all: #define VESSEL_SYSTEM_DEBUG 1                             */
/*   - Disable all: #define VESSEL_SYSTEM_DEBUG 0                            */
/*   - Selective: Set individual category toggles to 0 after enabling master */
/*                                                                           */
/* Debug output prefixes (for grep filtering):                               */
/*   [VESSEL]        - General vessel operations                             */
/*   [VESSEL_MOVE]   - Position/movement updates                             */
/*   [VESSEL_AUTO]   - Autopilot navigation                                  */
/*   [VESSEL_DOCK]   - Docking operations                                    */
/*   [VESSEL_DB]     - Database persistence                                  */
/*   [VESSEL_FUNC]   - Function entry/exit                                   */
/*   [VESSEL_STATE]  - State transitions                                     */
/*   [VEHICLE]       - Vehicle operations                                    */
/*   [VEHICLE_MOVE]  - Vehicle movement                                      */
/*   [VEHICLE_XPORT] - Vehicle-vessel transport                              */
/* ========================================================================= */

/* Master debug toggle - override to 1 in an explicit development build. */
#ifndef VESSEL_SYSTEM_DEBUG
#define VESSEL_SYSTEM_DEBUG 0
#endif

#define VESSEL_DEBUG_CAT_CORE (1U << 0)
#define VESSEL_DEBUG_CAT_MOVE (1U << 1)
#define VESSEL_DEBUG_CAT_AUTO (1U << 2)
#define VESSEL_DEBUG_CAT_DOCK (1U << 3)
#define VESSEL_DEBUG_CAT_DB (1U << 4)
#define VESSEL_DEBUG_CAT_FUNC (1U << 5)
#define VESSEL_DEBUG_CAT_STATE (1U << 6)
#define VESSEL_DEBUG_CAT_VEHICLE (1U << 7)
#define VESSEL_DEBUG_CAT_VEHICLE_MOVE (1U << 8)
#define VESSEL_DEBUG_CAT_TRANSPORT (1U << 9)
#define VESSEL_DEBUG_CAT_ALL ((1U << 10) - 1U)

extern unsigned int vessel_debug_mask;
bool vessel_debug_enabled(unsigned int category);
unsigned int vessel_debug_category_from_name(const char *name);

/* Category-specific toggles (only effective when master is 1) */
#define VESSEL_DEBUG_CORE (VESSEL_SYSTEM_DEBUG && 1)   /* General vessel ops */
#define VESSEL_DEBUG_MOVE (VESSEL_SYSTEM_DEBUG && 1)   /* Movement/position */
#define VESSEL_DEBUG_AUTO (VESSEL_SYSTEM_DEBUG && 1)   /* Autopilot system */
#define VESSEL_DEBUG_DOCK (VESSEL_SYSTEM_DEBUG && 1)   /* Docking operations */
#define VESSEL_DEBUG_DB (VESSEL_SYSTEM_DEBUG && 1)     /* Database persistence */
#define VEHICLE_DEBUG_CORE (VESSEL_SYSTEM_DEBUG && 1)  /* Vehicle operations */
#define VEHICLE_DEBUG_MOVE (VESSEL_SYSTEM_DEBUG && 1)  /* Vehicle movement */
#define VEHICLE_DEBUG_XPORT (VESSEL_SYSTEM_DEBUG && 1) /* Vehicle-vessel transport */

/* ---- Vessel Debug Macros ---- */

#if VESSEL_DEBUG_CORE
#define VSSL_DEBUG(fmt, ...)                                                                       \
  do                                                                                               \
  {                                                                                                \
    if (vessel_debug_enabled(VESSEL_DEBUG_CAT_CORE))                                               \
      log("[VESSEL] " fmt, ##__VA_ARGS__);                                                         \
  } while (0)
#else
#define VSSL_DEBUG(fmt, ...)                                                                       \
  do                                                                                               \
  {                                                                                                \
  } while (0)
#endif

#if VESSEL_DEBUG_MOVE
#define VSSL_DEBUG_MOVE(fmt, ...)                                                                  \
  do                                                                                               \
  {                                                                                                \
    if (vessel_debug_enabled(VESSEL_DEBUG_CAT_MOVE))                                               \
      log("[VESSEL_MOVE] " fmt, ##__VA_ARGS__);                                                    \
  } while (0)
#else
#define VSSL_DEBUG_MOVE(fmt, ...)                                                                  \
  do                                                                                               \
  {                                                                                                \
  } while (0)
#endif

#if VESSEL_DEBUG_AUTO
#define VSSL_DEBUG_AUTO(fmt, ...)                                                                  \
  do                                                                                               \
  {                                                                                                \
    if (vessel_debug_enabled(VESSEL_DEBUG_CAT_AUTO))                                               \
      log("[VESSEL_AUTO] " fmt, ##__VA_ARGS__);                                                    \
  } while (0)
#else
#define VSSL_DEBUG_AUTO(fmt, ...)                                                                  \
  do                                                                                               \
  {                                                                                                \
  } while (0)
#endif

#if VESSEL_DEBUG_DOCK
#define VSSL_DEBUG_DOCK(fmt, ...)                                                                  \
  do                                                                                               \
  {                                                                                                \
    if (vessel_debug_enabled(VESSEL_DEBUG_CAT_DOCK))                                               \
      log("[VESSEL_DOCK] " fmt, ##__VA_ARGS__);                                                    \
  } while (0)
#else
#define VSSL_DEBUG_DOCK(fmt, ...)                                                                  \
  do                                                                                               \
  {                                                                                                \
  } while (0)
#endif

#if VESSEL_DEBUG_DB
#define VSSL_DEBUG_DB(fmt, ...)                                                                    \
  do                                                                                               \
  {                                                                                                \
    if (vessel_debug_enabled(VESSEL_DEBUG_CAT_DB))                                                 \
      log("[VESSEL_DB] " fmt, ##__VA_ARGS__);                                                      \
  } while (0)
#else
#define VSSL_DEBUG_DB(fmt, ...)                                                                    \
  do                                                                                               \
  {                                                                                                \
  } while (0)
#endif

/* ---- Vehicle Debug Macros ---- */

#if VEHICLE_DEBUG_CORE
#define VHCL_DEBUG(fmt, ...)                                                                       \
  do                                                                                               \
  {                                                                                                \
    if (vessel_debug_enabled(VESSEL_DEBUG_CAT_VEHICLE))                                            \
      log("[VEHICLE] " fmt, ##__VA_ARGS__);                                                        \
  } while (0)
#else
#define VHCL_DEBUG(fmt, ...)                                                                       \
  do                                                                                               \
  {                                                                                                \
  } while (0)
#endif

#if VEHICLE_DEBUG_MOVE
#define VHCL_DEBUG_MOVE(fmt, ...)                                                                  \
  do                                                                                               \
  {                                                                                                \
    if (vessel_debug_enabled(VESSEL_DEBUG_CAT_VEHICLE_MOVE))                                       \
      log("[VEHICLE_MOVE] " fmt, ##__VA_ARGS__);                                                   \
  } while (0)
#else
#define VHCL_DEBUG_MOVE(fmt, ...)                                                                  \
  do                                                                                               \
  {                                                                                                \
  } while (0)
#endif

#if VEHICLE_DEBUG_XPORT
#define VHCL_DEBUG_XPORT(fmt, ...)                                                                 \
  do                                                                                               \
  {                                                                                                \
    if (vessel_debug_enabled(VESSEL_DEBUG_CAT_TRANSPORT))                                          \
      log("[VEHICLE_XPORT] " fmt, ##__VA_ARGS__);                                                  \
  } while (0)
#else
#define VHCL_DEBUG_XPORT(fmt, ...)                                                                 \
  do                                                                                               \
  {                                                                                                \
  } while (0)
#endif

/* ---- Function Entry/Exit Macros ---- */

#if VESSEL_SYSTEM_DEBUG
#define VSSL_DEBUG_ENTER(func)                                                                     \
  do                                                                                               \
  {                                                                                                \
    if (vessel_debug_enabled(VESSEL_DEBUG_CAT_FUNC))                                               \
      log("[VESSEL_FUNC] ENTER: %s()", func);                                                      \
  } while (0)
#define VSSL_DEBUG_EXIT(func)                                                                      \
  do                                                                                               \
  {                                                                                                \
    if (vessel_debug_enabled(VESSEL_DEBUG_CAT_FUNC))                                               \
      log("[VESSEL_FUNC] EXIT: %s()", func);                                                       \
  } while (0)
#define VSSL_DEBUG_EXIT_VAL(func, val)                                                             \
  do                                                                                               \
  {                                                                                                \
    if (vessel_debug_enabled(VESSEL_DEBUG_CAT_FUNC))                                               \
      log("[VESSEL_FUNC] EXIT: %s() = %d", func, (int)(val));                                      \
  } while (0)
#else
#define VSSL_DEBUG_ENTER(func)                                                                     \
  do                                                                                               \
  {                                                                                                \
  } while (0)
#define VSSL_DEBUG_EXIT(func)                                                                      \
  do                                                                                               \
  {                                                                                                \
  } while (0)
#define VSSL_DEBUG_EXIT_VAL(func, val)                                                             \
  do                                                                                               \
  {                                                                                                \
  } while (0)
#endif

/* ---- State Transition Macro ---- */

#if VESSEL_SYSTEM_DEBUG
#define VSSL_DEBUG_STATE(entity, old_state, new_state)                                             \
  do                                                                                               \
  {                                                                                                \
    if (vessel_debug_enabled(VESSEL_DEBUG_CAT_STATE))                                              \
      log("[VESSEL_STATE] %s: %s -> %s", entity, old_state, new_state);                            \
  } while (0)
#else
#define VSSL_DEBUG_STATE(entity, old_state, new_state)                                             \
  do                                                                                               \
  {                                                                                                \
  } while (0)
#endif

/* ========================================================================= */
/* VESSEL CLASSIFICATIONS AND CAPABILITIES                                   */
/* ========================================================================= */

/* Vessel classification types for Phase 1 wilderness integration */
enum vessel_class
{
  VESSEL_RAFT,      /* Small, rivers/shallow water only */
  VESSEL_BOAT,      /* Medium, coastal waters */
  VESSEL_SHIP,      /* Large, ocean-capable */
  VESSEL_WARSHIP,   /* Combat vessel, heavily armed */
  VESSEL_AIRSHIP,   /* Flying vessel, ignores terrain */
  VESSEL_SUBMARINE, /* Underwater vessel, depth navigation */
  VESSEL_TRANSPORT, /* Cargo/passenger vessel */
  VESSEL_MAGICAL    /* Special magical vessels */
};

#define NUM_VESSEL_TYPES 8 /* Count of vessel_class values */

/* Autopilot state machine for automated navigation */
enum autopilot_state
{
  AUTOPILOT_OFF,       /* Autopilot disabled */
  AUTOPILOT_TRAVELING, /* Moving toward waypoint */
  AUTOPILOT_WAITING,   /* At waypoint, waiting */
  AUTOPILOT_PAUSED,    /* Temporarily suspended */
  AUTOPILOT_COMPLETE   /* Route finished */
};

/* Independent cooldowns keep a severity change or combat-state change visible
 * while suppressing repeated copies of the same high-volume message class. */
enum vessel_message_key
{
  VESSEL_MESSAGE_AMBIENT_DEPTH = 0,
  VESSEL_MESSAGE_AMBIENT_SQUALL,
  VESSEL_MESSAGE_AMBIENT_STORM,
  VESSEL_MESSAGE_AMBIENT_GALE,
  VESSEL_MESSAGE_COMBAT_DAMAGE,
  VESSEL_MESSAGE_COMBAT_RETURN_FIRE,
  VESSEL_MESSAGE_COMBAT_RETURN_FIRE_MISS,
  VESSEL_MESSAGE_COMBAT_RELOAD,
  NUM_VESSEL_MESSAGE_KEYS
};

/* Vessel terrain capabilities structure */
struct vessel_terrain_caps
{
  bool can_traverse_ocean;      /* Deep water navigation */
  bool can_traverse_shallow;    /* Shallow water/rivers */
  bool can_traverse_air;        /* Airship flight */
  bool can_traverse_underwater; /* Submarine diving */
  int min_water_depth;          /* Minimum depth required */
  int max_altitude;             /* Maximum flight altitude */
  float terrain_speed_mod[40];  /* Speed modifier by terrain type (max sector types) */
};

/* Extended vessel data for wilderness integration */
struct vessel_wilderness_data
{
  int x_coord;                             /* Wilderness X coordinate (-1024 to +1024) */
  int y_coord;                             /* Wilderness Y coordinate (-1024 to +1024) */
  int z_coord;                             /* Elevation/depth (airships/submarines) */
  float heading;                           /* Direction in degrees (0-360) */
  float speed;                             /* Current speed in coords/tick */
  enum vessel_class vessel_class;          /* Type of vessel */
  struct vessel_terrain_caps capabilities; /* Terrain capabilities */
};

/* ========================================================================= */
/* UNIFIED FACADE API                                                        */
/* ========================================================================= */

enum vessel_command
{
  VESSEL_CMD_NONE = 0,
  VESSEL_CMD_DRIVE,       /* CWG drive */
  VESSEL_CMD_SAIL_MOVE,   /* Outcast move */
  VESSEL_CMD_SAIL_SPEED,  /* Outcast speed */
  VESSEL_CMD_GH_TACTICAL, /* Greyhawk tactical */
  VESSEL_CMD_GH_STATUS,   /* Greyhawk status */
};

struct vessel_result
{
  int success;    /* boolean */
  int error_code; /* 0 success */
  char message[256];
  void *result_data;
};

/* Initialization entry point to be called at boot */
void vessel_init_all(void);

/* Unified command executor (optional facade) */
struct vessel_result vessel_execute_command(struct char_data *actor, enum vessel_command cmd,
                                            const char *argument);

/* ========================================================================= */
/* FUNCTION PROTOTYPES - WILDERNESS INTEGRATION                              */
/* ========================================================================= */
/* Functions for integrating vessels with the wilderness coordinate system    */

room_rnum get_or_allocate_wilderness_room(int x, int y);
void vessel_dynamic_room_cache_reset(void);
void vessel_dynamic_room_cache_remember(room_rnum room);
room_rnum vessel_dynamic_room_cache_lookup(int x, int y);
room_rnum vessel_dynamic_room_cache_unused(void);
bool update_ship_wilderness_position(int shipnum, int new_x, int new_y, int new_z);
int get_ship_terrain_type(int shipnum);
bool vessel_z_within_class_limits(enum vessel_class vessel_type, int z);
bool vessel_z_allows_sector(enum vessel_class vessel_type, int sector_type, int z);
bool vessel_can_occupy_coordinates(enum vessel_class vessel_type, int x, int y, int z);
bool can_vessel_traverse_terrain(enum vessel_class vessel_type, int x, int y, int z);
int get_terrain_speed_modifier(enum vessel_class vessel_type, int sector_type,
                               int weather_conditions);
bool vessel_region_feature_threshold_met(int region_type, int threshold, int z, int depth_units);
bool vessel_region_feature_at_coordinates(int region_type, int x, int y, int z,
                                          struct vessel_region_feature *feature);
int get_vessel_position_speed_modifier(enum vessel_class vessel_type, int sector_type,
                                       int weather_conditions, int x, int y, int z,
                                       struct vessel_region_feature *lane);
bool move_ship_wilderness(int shipnum, int direction, struct char_data *ch);

/* ========================================================================= */
/* NAVAL COMBAT (Phase 05, vessels_combat.c)                                 */
/* ========================================================================= */

/* Damage status bands derived from remaining internal structure */
#define VESSEL_STATUS_SOUND 0
#define VESSEL_STATUS_BATTERED 1
#define VESSEL_STATUS_CRIPPLED 2
#define VESSEL_STATUS_SINKING 3

#define VESSEL_WEAPON_RELOAD_TICKS 6

/* An owner cannot end an already-consented vessel fight by logging out. */
#define VESSEL_PVP_LOGOUT_GRACE 300

bool vessel_pvp_permitted(struct char_data *ch, struct greyhawk_ship_data *target, bool display);
bool vessel_pvp_grace_active(const struct greyhawk_ship_data *target, const char *attacker_name,
                             time_t now);
void vessel_clear_pvp_grace(struct greyhawk_ship_data *ship);
int vessel_total_internal(const struct greyhawk_ship_data *ship);
int vessel_max_internal(const struct greyhawk_ship_data *ship);
int vessel_status(const struct greyhawk_ship_data *ship);
void vessel_initialize_condition(struct greyhawk_ship_data *ship, int armor);
const char *vessel_status_name(int status);
void vessel_apply_damage(int shipnum, int amount, int arc, const char *cause);
void vessel_sink(int shipnum);
void vessel_check_grounding(int shipnum);
void vessel_combat_tick(void);
void vessel_combat_tick_one(struct greyhawk_ship_data *ship);

ACMD_DECL(do_shipfire);   /* Fire a weapon slot at another ship */
ACMD_DECL(do_shiprepair); /* Slow at-sea repairs while stationary */
ACMD_DECL(do_claimship);  /* Capture a ship from an uncontested bridge */

/* ========================================================================= */
/* SHOWCASE EVENTS AND LEADERBOARDS (Phase 16, vessels_events.c)              */
/* ========================================================================= */

enum vessel_event_type
{
  VESSEL_EVENT_NONE = 0,
  VESSEL_EVENT_REGATTA = 1,
  VESSEL_EVENT_SKIRMISH = 2,
  VESSEL_EVENT_GHOST_FLEET = 3
};

#define VESSEL_EVENT_TEAM_NONE 0
#define VESSEL_EVENT_TEAM_RED 1
#define VESSEL_EVENT_TEAM_BLUE 2

const char *vessel_event_type_name(enum vessel_event_type event_type);
enum vessel_event_type vessel_event_type_from_name(const char *name);
bool vessel_event_finish_reached(int old_x, int old_y, int new_x, int new_y, int finish_x,
                                 int finish_y);
int vessel_event_placement_points(int placement);
int vessel_event_winning_team(int red_score, int blue_score);
void vessel_event_ensure_schema(void);
void vessel_event_boot(void);
void vessel_event_tick(void);
void vessel_event_handle_move(int shipnum, int old_x, int old_y, int new_x, int new_y);
void vessel_event_record_damage(int attacker_ship_id, int target_ship_id, int amount);
void vessel_event_handle_sink(int shipnum);
ACMD_DECL(do_vevent);

/* ========================================================================= */
/* OWNERSHIP AND PERMISSIONS (Phase 06, vessels_ownership.c)                 */
/* ========================================================================= */

/* Hireable crew positions (index into greyhawk_ship_data.crew_tier) */
#define CREW_SAILMASTER 0    /* Speed handling */
#define CREW_GUNNER 1        /* Gunnery accuracy */
#define CREW_BOSUN 2         /* Repair rate */
#define CREW_QUARTERMASTER 3 /* Cargo capacity bonus */
#define NUM_CREW_POSITIONS 4

/* Crew quality tiers */
#define CREW_TIER_NONE 0
#define CREW_TIER_GREEN 1
#define CREW_TIER_ABLE 2
#define CREW_TIER_VETERAN 3

/* Wage accrual: one payday per this many combat ticks. Due payroll is spread
 * across 100 batches, bounding a full fleet to five ships per tick. */
#define CREW_WAGE_INTERVAL 600
#define CREW_WAGE_BATCH_COUNT 100

/* Installable upgrades (greyhawk_ship_data.upgrades bitfield) */
#define SHIP_UPGRADE_PLATING (1 << 0)    /* +50% max armor all sides */
#define SHIP_UPGRADE_RIGGING (1 << 1)    /* +5 max speed */
#define SHIP_UPGRADE_HOLD (1 << 2)       /* +25% cargo capacity */
#define SHIP_UPGRADE_REINFORCED (1 << 3) /* +50% max internal structure */
#define NUM_SHIP_UPGRADES 4

/* Hull wear: one wear event per this many ticks while under way */
#define SHIP_WEAR_INTERVAL 900

const char *vessel_upgrade_name(int index);
int vessel_upgrade_bit(int index);
int vessel_upgrade_cost(int index, enum vessel_class vessel_type);
void vessel_upkeep_tick(void);
void vessel_upkeep_tick_one(struct greyhawk_ship_data *ship);
void vessel_db_save_extras(struct greyhawk_ship_data *ship);
void vessel_db_load_extras(struct greyhawk_ship_data *ship);
void vessel_pay_insurance(struct greyhawk_ship_data *ship);
int vessel_deliver_pending_insurance(struct char_data *ch);

ACMD_DECL(do_shipupgrade); /* Owner: install upgrades at a dock */
ACMD_DECL(do_shipinsure);  /* Owner: buy sinking insurance */

/* ========================================================================= */
/* CARGO AND TRADE (Phase 07, vessels_trade.c)                               */
/* ========================================================================= */

/* Supply level bounds the price swing: prices move within
 * +/- TRADE_MAX_DRIFT percent of base, and never outside it. */
#define TRADE_MAX_DRIFT 60
#define TRADE_BASELINE_SUPPLY 100
#define TRADE_SUPPLY_MIN 10
#define TRADE_SUPPLY_MAX 400
#define TRADE_SELL_PERCENT 85
#define TRADE_RESTOCK_INTERVAL 1200 /* Vessel ticks between supply drift */

struct vessel_trade_simulation_result
{
  int requested_trades;
  int completed_trades;
  int minimum_supply;
  int maximum_supply;
  int profitable_routes;
  int equilibrium_source_supply;
  int equilibrium_destination_supply;
  int restocked_source_supply;
  int restocked_destination_supply;
  long long adversarial_profit;
  long long finite_route_profit;
};

#define VESSEL_BALANCE_DEFAULT_DUELS 1000
#define VESSEL_BALANCE_MAX_DUELS 5000

struct vessel_balance_duel_result
{
  int requested_duels;
  int completed_duels;
  int unresolved_duels;
  int first_wins;
  int second_wins;
  int minimum_ticks;
  int median_ticks;
  int p95_ticks;
  int maximum_ticks;
};

void vessel_trade_ensure_schema(void);
int vessel_cargo_weight(const struct greyhawk_ship_data *ship);
int vessel_commodity_price(int base_price, int supply);
int vessel_trade_adjusted_supply(int supply, int delta);
int vessel_trade_restocked_supply(int supply);
long long vessel_trade_buy_cost(int base_price, int supply, int quantity);
long long vessel_trade_sell_revenue(int base_price, int supply, int quantity);
bool vessel_trade_run_simulation(int trade_count, struct vessel_trade_simulation_result *result);
bool vessel_balance_run_duels(int duel_count, struct vessel_balance_duel_result *result);
bool vessel_balance_report(struct char_data *ch, int duel_count);
void vessel_trade_restock_tick(void);
void vessel_db_save_cargo(struct greyhawk_ship_data *ship);
void vessel_db_load_cargo(struct greyhawk_ship_data *ship);

/* ========================================================================= */
/* LIVING WORLD: WEATHER HAZARDS AND ENCOUNTERS (Phase 08)                   */
/* ========================================================================= */

/* Weather thresholds read from the wilderness 0..255 weather field
 * (get_weather(x,y)); these match the bands shown to coastal walkers. */
#define VESSEL_WEATHER_CLOUDY 128
#define VESSEL_WEATHER_FOG 178
#define VESSEL_WEATHER_SQUALL 178
#define VESSEL_WEATHER_STORM 200
#define VESSEL_WEATHER_GALE 225

/* Hazard/encounter checks run this often (vessel ticks) */
#define VESSEL_HAZARD_INTERVAL 60
#define VESSEL_ENCOUNTER_INTERVAL 180
#define VESSEL_NARRATIVE_INTERVAL 240

/* Phase 15 bounty-hunter encounter lifecycle limits. */
#define VESSEL_HUNTER_DURATION_MIN 10
#define VESSEL_HUNTER_DURATION_MAX 86400
#define VESSEL_HUNTER_GRACE_MAX 600
#define VESSEL_HUNTER_COOLDOWN_MIN 1
#define VESSEL_HUNTER_COOLDOWN_MAX 604800
#define VESSEL_HUNTER_PURSUIT_SPEED_MAX 100

struct vessel_hunter_config
{
  int encounter_id;
  int prototype_id;
  int pilot_mob_vnum;
  int min_bounty;
  int pursuit_speed;
  int hunt_duration_seconds;
  int target_grace_seconds;
  int cooldown_seconds;
  bool enabled;
};

/* Contact/spotting range shrinks in fog */
#define VESSEL_SIGHT_CLEAR 50
#define VESSEL_SIGHT_FOG 15

struct vessel_lookout_band
{
  int sector_type;
  int first_distance;
  int last_distance;
};

void vessel_hazard_ensure_schema(void);
int vessel_sight_range(const struct greyhawk_ship_data *ship);
int vessel_lookout_sample_distances(int visibility, int *distances, int capacity);
int vessel_lookout_build_bands(const int *sectors, const int *distances, int sample_count,
                               struct vessel_lookout_band *bands, int capacity);
const char *vessel_lookout_compass_direction(int bearing);
const char *vessel_weather_condition_name(int weather);
int vessel_weather_severity_from_value(int weather);
int vessel_storm_severity(const struct greyhawk_ship_data *ship);
bool vessel_build_ambient_message(enum vessel_class vessel_type, int weather, int speed,
                                  int maxspeed, int z, char *output, size_t output_size);
char *vessel_create_at_sea_description(struct char_data *ch, const struct greyhawk_ship_data *ship);
void vessel_narrative_tick(void);
void vessel_narrative_tick_one(struct greyhawk_ship_data *ship);
bool vessel_narrative_force_ship(struct greyhawk_ship_data *ship);
void vessel_weather_tick(void);
void vessel_weather_tick_one(struct greyhawk_ship_data *ship);
void vessel_encounter_tick(void);
void vessel_encounter_tick_begin(void);
void vessel_encounter_tick_one(struct greyhawk_ship_data *ship);
void vessel_encounter_force_check(void);
bool vessel_encounter_reload_config(void);
bool vessel_in_encounter_region(const struct greyhawk_ship_data *ship, int *region_vnum);
bool vessel_encounter_region_from_list(const struct region_list *regions, int *output_region_vnum);
bool vessel_encounter_region_at_coordinates(int x, int y, int *output_region_vnum);
bool vessel_encounter_chance_succeeds(int chance, int roll);
bool vessel_encounter_candidate_matches(int candidate_region_vnum, int candidate_vessel_class,
                                        int min_depth, int max_depth, int ship_region_vnum,
                                        enum vessel_class ship_class, int depth_units);
bool vessel_encounter_claim_room(room_rnum room, room_rnum *claimed_rooms, int *claimed_count,
                                 int claimed_capacity);
int vessel_encounter_cached_room_index(room_rnum room, const room_rnum *cached_rooms,
                                       int cached_count);
int vessel_lookout_bonus(const struct greyhawk_ship_data *ship);

/* Data-driven HUNTED-state warships (Phase 15, vessels_hunters.c). */
void vessel_hunter_ensure_schema(void);
void vessel_hunter_boot(void);
void vessel_hunter_tick(void);
void vessel_hunter_tick_one(struct greyhawk_ship_data *ship);
int vessel_hunter_load_config(int encounter_id, struct vessel_hunter_config *config);
bool vessel_hunter_config_is_valid(const struct vessel_hunter_config *config);
bool vessel_hunter_lifecycle_allows_spawn(const char *status, time_t next_eligible_at, time_t now);
bool vessel_hunter_target_is_eligible(const struct greyhawk_ship_data *target,
                                      const struct vessel_hunter_config *config, time_t now);
bool vessel_hunter_spawn(struct greyhawk_ship_data *target,
                         const struct vessel_hunter_config *config, const char *encounter_name);
void vessel_hunter_handle_sink(struct greyhawk_ship_data *ship);
void vessel_hunter_handle_capture(struct char_data *ch, struct greyhawk_ship_data *ship);
void vessel_hunter_handle_purge(struct greyhawk_ship_data *ship, const char *staff_name);
void vessel_hunter_handle_player_rename(const char *old_name, const char *new_name);
void vessel_hunter_handle_player_removal(const char *player_name);

ACMD_DECL(do_seastate); /* Report weather, sea state, and sight range */

/* ========================================================================= */
/* OPERATOR TOOLING AND PROTOCOL (Phase 09, vessels_admin.c)                 */
/* ========================================================================= */

void vessel_msdp_update(struct char_data *ch);
void vessel_msdp_tick(void);

ACMD_DECL(do_shiplist);    /* Staff: fleet overview + room pool health */
ACMD_DECL(do_shipgoto);    /* Staff: teleport to a ship */
ACMD_DECL(do_shipfix);     /* Staff: restore a ship to full condition */
ACMD_DECL(do_shippurge);   /* Staff: destroy one runtime ship safely */
ACMD_DECL(do_vesseldebug); /* Staff: focused runtime debug categories */

/* ========================================================================= */
/* PIRACY AND BOUNTY (Phase 07, vessels_piracy.c)                            */
/* ========================================================================= */

/* Bounty thresholds: at WANTED a player is refused docking at lawful
 * ports; at HUNTED the navy sends warships. */
#define BOUNTY_WANTED 500
#define BOUNTY_HUNTED 2000

/* Bounty earned per unit of cargo taken by force */
#define BOUNTY_PER_CARGO_UNIT 15

/* Builder-authored REGION_GEOGRAPHIC waters may refine the default piracy
 * consequence without creating a vessel-private geography model. */
#define VESSEL_WATERS_UNCLAIMED 0
#define VESSEL_WATERS_TERRITORIAL 1
#define VESSEL_WATERS_FREE 2
#define VESSEL_WATERS_PIRATE_COVE 3
#define VESSEL_PIRACY_BOUNTY_PERCENT_MAX 500

struct vessel_piracy_law
{
  bool configured;
  int region_vnum;
  int waters_type;
  int priority;
  int bounty_percent;
  char region_name[64];
  char authority[64];
};

void vessel_piracy_ensure_schema(void);
bool vessel_piracy_reload_laws(void);
void vessel_piracy_clear_laws(void);
#ifdef LUMINARI_CUTEST
size_t vessel_piracy_coordinate_cache_count(void);
#endif
int vessel_get_bounty(const char *player_name);
void vessel_add_bounty(const char *player_name, int amount);
void vessel_clear_bounty(const char *player_name);
bool vessel_has_letter_of_marque(const char *player_name);
const char *vessel_waters_type_name(int waters_type);
int vessel_piracy_bounty_for_units(int cargo_units, int bounty_percent);
bool vessel_piracy_wanted_port_is_open(const struct vessel_piracy_law *law);
bool vessel_piracy_point_in_polygon(const struct vertex *vertices, int vertex_count, int x, int y);
bool vessel_piracy_law_at_coordinates(int x, int y, struct vessel_piracy_law *law);
bool vessel_piracy_law_for_ship(const struct greyhawk_ship_data *ship,
                                struct vessel_piracy_law *law);
void vessel_piracy_track_waters(struct greyhawk_ship_data *ship, bool announce);
bool vessel_port_refuses(struct char_data *ch);
int vessel_plunder_cargo(struct char_data *ch, struct greyhawk_ship_data *prize,
                         struct greyhawk_ship_data *raider);

ACMD_DECL(do_plunder); /* Take cargo from a captured/boarded ship */
ACMD_DECL(do_bounty);  /* Check a bounty */
ACMD_DECL(do_marque);  /* Buy a letter of marque (legal privateering) */

/* Data-driven NPC merchant lifecycle (Phase 14, vessels_merchants.c) */
#define VESSEL_MERCHANT_RESPAWN_MIN 1
#define VESSEL_MERCHANT_RESPAWN_MAX 604800
#define VESSEL_MERCHANT_ATTACK_STANDING_PENALTY 25
#define VESSEL_MERCHANT_LOSS_STANDING_PENALTY 100
#define VESSEL_MERCHANT_LOSS_BOUNTY_UNITS 34
#define VESSEL_MERCHANT_RESPONSIBILITY_SECONDS 300

void vessel_merchant_ensure_schema(void);
void vessel_merchant_boot(void);
void vessel_merchant_tick(void);
void vessel_merchant_note_attacker(struct char_data *ch, struct greyhawk_ship_data *ship);
void vessel_merchant_record_plunder(struct char_data *ch, struct greyhawk_ship_data *ship,
                                    int cargo_units, int bounty_delta);
void vessel_merchant_handle_sink(struct greyhawk_ship_data *ship);
void vessel_merchant_handle_capture(struct char_data *ch, struct greyhawk_ship_data *ship);
void vessel_merchant_handle_purge(struct greyhawk_ship_data *ship, const char *staff_name);
int vessel_merchant_deliver_pending_consequences(struct char_data *ch);
bool vessel_merchant_should_spawn(bool enabled, int active_ship_id, time_t next_respawn_at,
                                  time_t now);
bool vessel_merchant_responsibility_active(time_t attacked_at, time_t now);
int vessel_merchant_faction_penalty(int cargo_units, bool total_loss);
ACMD_DECL(do_vmerchant); /* Staff: inspect, synchronize, or test merchant loss */

/* Freight contracts (Phase 07, vessels_contracts.c) */
#define CONTRACT_STATUS_OPEN 0
#define CONTRACT_STATUS_TAKEN 1
#define CONTRACT_STATUS_DONE 2
#define MAX_CONTRACT_OFFERS 6

void vessel_contracts_ensure_schema(void);
void vessel_contracts_refresh_port(int port_vnum);

ACMD_DECL(do_contracts);       /* At a dock: list the freight board */
ACMD_DECL(do_contractaccept);  /* Take a freight contract */
ACMD_DECL(do_contractdeliver); /* Deliver and collect payment */
ACMD_DECL(do_contractabandon); /* Drop a contract you cannot finish */

ACMD_DECL(do_market);        /* At a dock: list commodity prices */
ACMD_DECL(do_cargobuy);      /* At a dock: buy bulk cargo into the hold */
ACMD_DECL(do_cargosell);     /* At a dock: sell bulk cargo from the hold */
ACMD_DECL(do_cargomanifest); /* Show the ship's bulk cargo manifest */
ACMD_DECL(do_vtradecheck);   /* Staff: deterministic economy simulation */
ACMD_DECL(do_dockfees);      /* Inspect or settle the current berthing fee */

bool vessel_room_is_port(room_rnum room);
bool vessel_room_is_fee_berth(const struct greyhawk_ship_data *ship, room_rnum room);
bool vessel_ship_is_in_port(const struct greyhawk_ship_data *ship);
int vessel_dock_fee_for_class(enum vessel_class vessel_type);
int vessel_assess_dock_fee(struct greyhawk_ship_data *ship, int port_vnum, int owner_clan_vnum);
bool vessel_clear_departed_berth(struct greyhawk_ship_data *ship, room_rnum old_room,
                                 bool old_is_port);
void vessel_update_port_berth(struct greyhawk_ship_data *ship, room_rnum old_room,
                              room_rnum new_room, bool old_is_port);
int vessel_passenger_fare(const struct greyhawk_ship_data *ship);
bool vessel_collect_passenger_fare(struct char_data *ch, struct greyhawk_ship_data *ship);

const char *vessel_crew_position_name(int position);
const char *vessel_crew_tier_name(int tier);
int vessel_crew_hire_cost(int position, int tier);
int vessel_crew_wage(int position, int tier);
int vessel_crew_wage_batch_for_slot(int ship_slot);
int vessel_crew_departure_delete_query(char *query, size_t query_size, const int *ship_slots,
                                       const int *positions, int count);
void vessel_apply_crew_bonuses(struct greyhawk_ship_data *ship);
void vessel_crew_wage_tick(void);
int vessel_crew_wage_begin_tick(void);
int vessel_crew_wage_tick_one(struct greyhawk_ship_data *ship, int current_batch);
void vessel_crew_delete_departure(int ship_slot, int position);
void vessel_db_save_crew(struct greyhawk_ship_data *ship);
void vessel_db_load_crew(struct greyhawk_ship_data *ship);

bool vessel_helm_permitted(struct char_data *ch, struct greyhawk_ship_data *ship);
void vessel_ownership_ensure_schema(void);
bool vessel_db_save_owner(struct greyhawk_ship_data *ship);
void vessel_db_load_owner(struct greyhawk_ship_data *ship);
bool vessel_transfer_owner(struct greyhawk_ship_data *ship, const char *new_owner);
void vessel_db_save_permits(struct greyhawk_ship_data *ship);
void vessel_db_load_permits(struct greyhawk_ship_data *ship);
bool vessel_handle_player_removal(const char *player_name);

/* Shipyard (Phase 06, vessels_edit.c) */
int vessel_prototype_price(int vclass, int max_speed, int armor);
int vessel_spawn_from_prototype(struct char_data *ch, int id);
int vessel_spawn_public_from_prototype_at(int id, const char *instance_name, int x, int y, int z);
ACMD_DECL(do_shipbrowse);    /* Shipyard catalog with prices */
ACMD_DECL(do_shipbuy);       /* Purchase a hull at a dock */
ACMD_DECL(do_shipchristen);  /* Owner: rename the ship */
ACMD_DECL(do_shipcustomize); /* Owner: set paint and figurehead */

ACMD_DECL(do_shippermit);  /* Owner: clear a player to take the helm */
ACMD_DECL(do_shiprevoke);  /* Owner: revoke a helm permit */
ACMD_DECL(do_shipcrew);    /* List owner, permits, crew, and NPC pilot */
ACMD_DECL(do_shiphire);    /* Owner: hire crew at a dock */
ACMD_DECL(do_shipdismiss); /* Owner: dismiss hired crew */
ACMD_DECL(do_shipwages);   /* Owner: pay accrued wages */
ACMD_DECL(do_shipdeed);    /* Owner: transfer ownership */

/* Vessel type accessor functions */
const struct vessel_terrain_caps *get_vessel_terrain_caps(enum vessel_class vessel_type);
enum vessel_class get_vessel_type_from_ship(int shipnum);
const char *get_vessel_type_name(enum vessel_class vessel_type);
int get_vessel_cargo_capacity(enum vessel_class vessel_type);
int vessel_effective_cargo_capacity(const struct greyhawk_ship_data *ship);

/* Maximum cargo weight by vessel class (in pounds).
 * Covers loaded vehicles plus future cargo lots; consumed by
 * get_vessel_cargo_capacity() and check_vessel_vehicle_capacity(). */
#define VESSEL_CARGO_RAFT 300        /* Barely more than its passengers */
#define VESSEL_CARGO_BOAT 2000       /* A cart or two of coastal freight */
#define VESSEL_CARGO_SHIP 12000      /* Standard merchant hold */
#define VESSEL_CARGO_WARSHIP 6000    /* Hold space lost to armament */
#define VESSEL_CARGO_AIRSHIP 4000    /* Lift-limited */
#define VESSEL_CARGO_SUBMARINE 3000  /* Hull volume at a premium */
#define VESSEL_CARGO_TRANSPORT 40000 /* Purpose-built freighter */
#define VESSEL_CARGO_MAGICAL 12000   /* Matches SHIP unless customized */

/* ========================================================================= */
/* FUNCTION PROTOTYPES - FUTURE ADVANCED VESSEL SYSTEM                       */
/* ========================================================================= */
/* These functions are placeholders for a future advanced vessel system      */

void load_vessels(void);                              /* Load vessel data from storage */
void save_vessels(void);                              /* Save vessel data to storage */
struct vessel_data *find_vessel_by_id(int vessel_id); /* Find vessel by unique ID */

void vessel_movement_tick(void); /* Process vessel movement each tick */
void enter_vessel(struct char_data *ch, struct vessel_data *vessel);    /* Board a vessel */
void exit_vessel(struct char_data *ch);                                 /* Leave a vessel */
int can_pilot_vessel(struct char_data *ch, struct vessel_data *vessel); /* Check piloting ability */
void pilot_vessel(struct char_data *ch, int direction); /* Pilot vessel in direction */

/* ========================================================================= */
/* FUNCTION PROTOTYPES - CWG VEHICLE SYSTEM (READY TO USE)                   */
/* ========================================================================= */
#if VESSELS_ENABLE_CWG

/* Object Finding Functions */
struct obj_data *find_vehicle_by_vnum(int vnum); /* Find vehicle object by vnum */
struct obj_data *get_obj_in_list_type(int type,
                                      struct obj_data *list); /* Find object of type in list */
struct obj_data *find_control(struct char_data *ch); /* Find vehicle controls near player */

/* Vehicle Movement Functions */
void drive_into_vehicle(struct char_data *ch, struct obj_data *vehicle,
                        char *arg); /* Drive into another vehicle */
void drive_outof_vehicle(struct char_data *ch, struct obj_data *vehicle); /* Drive out of vehicle */
void drive_in_direction(struct char_data *ch, struct obj_data *vehicle,
                        int dir); /* Drive in a direction */

/* CWG System Commands (Ready to Use) */
/* ACMD_DECL(do_drive); */ /* Drive a vehicle - main command for CWG system - NOT IMPLEMENTED */

#endif /* VESSELS_ENABLE_CWG */

/* ========================================================================= */
/* OUTCAST SHIP SYSTEM CONSTANTS AND STRUCTURES                              */
/* ========================================================================= */
#if VESSELS_ENABLE_OUTCAST

#ifndef MAX_NUM_SHIPS
#define MAX_NUM_SHIPS 50 /* Maximum number of ships in game */
#endif

#ifndef MAX_NUM_ROOMS
#define MAX_NUM_ROOMS 20 /* Maximum rooms per ship */
#endif

#define SHIP_MAX_SPEED 30 /* Maximum ship speed value */

/* Note: ITEM_SHIP and DOCKABLE removed - use GREYHAWK_ITEM_SHIP (56) and DOCKABLE
 * (alias for ROOM_DOCKABLE) defined unconditionally above. */

/* Outcast Ship Data Structure */
struct outcast_ship_data
{
  int hull;     /* max hull points */
  int speed;    /* max speed */
  int capacity; /* max number of characters in ship */
  int damage;   /* amount of damage (for firing) */

  int size;     /* size of the vehicle (for ramming) */
  int velocity; /* current velocity */

  struct obj_data *obj; /* vehicle object */
  int obj_num;          /* vehicle object number */

  int timer;      /* timer for ship action other than moving */
  int move_timer; /* timer for ship movement */
  int lastdir;    /* last direction for the ship */
  int repeat;     /* autopilot */

  int in_room;                      /* room containing this ship */
  int entrance_room;                /* room to enter/exit ship */
  int num_room;                     /* number of rooms in this vehicle */
  int room_list[MAX_NUM_ROOMS + 1]; /* room numbers in this vehicle */

  int dock_vehicle; /* docked to another ship: -1 is not docked */
};

/* Navigation Data Structure */
struct outcast_navigation_data
{
  int mob; /* mob id that can control ship */
  bool sail;
  int control_room;    /* control room for this mob to become a navigator */
  char *path1, *path2; /* path for going and returning */
  char *path;
  int start1;       /* start room */
  int destination1; /* destination */
  int destination;  /* initially set to zero */
  int sail_time;    /* time ship start sailing */
  int freq;         /* ship sail once every 'freq' hours */
};

/* FUNCTION PROTOTYPES - OUTCAST SHIP SYSTEM */
void initialize_outcast_ships(void);
void outcast_ship_activity(void);
int find_outcast_ship(struct obj_data *obj);
bool is_outcast_ship_docked(int t_ship);
bool is_valid_outcast_ship(int t_ship);
int in_which_outcast_ship(struct char_data *ch);
void sink_outcast_ship(int t_ship);
bool move_outcast_ship(int t_ship, int dir, struct char_data *ch);
int outcast_navigation(struct char_data *ch, int mob, int t_ship);
int outcast_ship_proc(struct obj_data *obj, struct char_data *ch, int cmd, char *arg);
int outcast_control_panel(struct obj_data *obj, struct char_data *ch, int cmd, char *argument);
int outcast_ship_exit_room(int room, struct char_data *ch, int cmd, char *arg);
int outcast_ship_look_out_room(int room, struct char_data *ch, int cmd, char *arg);

#endif /* VESSELS_ENABLE_OUTCAST */

/* ========================================================================= */
/* GREYHAWK SHIP SYSTEM CONSTANTS AND STRUCTURES                             */
/* ========================================================================= */
#if VESSELS_ENABLE_GREYHAWK

/* Note: GREYHAWK_MAXSHIPS, GREYHAWK_MAXSLOTS, position constants (FORE/PORT/REAR/STARBOARD),
 * range constants (SHRTRANGE/MEDRANGE/LNGRANGE), and GREYHAWK_ITEM_SHIP are now defined
 * unconditionally above (lines 65-88) to avoid duplication. */

/* GREYHAWK SHIP SYSTEM MACROS (subset used by implementation) */
#define GREYHAWK_SHIPMAXFARMOR(in_room) world[(in_room)].ship->maxfarmor
#define GREYHAWK_SHIPMAXRARMOR(in_room) world[(in_room)].ship->maxrarmor
#define GREYHAWK_SHIPMAXPARMOR(in_room) world[(in_room)].ship->maxparmor
#define GREYHAWK_SHIPMAXSARMOR(in_room) world[(in_room)].ship->maxsarmor
#define GREYHAWK_SHIPFARMOR(in_room) world[(in_room)].ship->farmor
#define GREYHAWK_SHIPRARMOR(in_room) world[(in_room)].ship->rarmor
#define GREYHAWK_SHIPPARMOR(in_room) world[(in_room)].ship->parmor
#define GREYHAWK_SHIPSARMOR(in_room) world[(in_room)].ship->sarmor
#define GREYHAWK_SHIPMAINSAIL(in_room) world[(in_room)].ship->mainsail
#define GREYHAWK_SHIPMAXMAINSAIL(in_room) world[(in_room)].ship->maxmainsail

#define GREYHAWK_SHIPMAXRINTERNAL(in_room) world[(in_room)].ship->maxrinternal
#define GREYHAWK_SHIPMAXFINTERNAL(in_room) world[(in_room)].ship->maxfinternal
#define GREYHAWK_SHIPMAXPINTERNAL(in_room) world[(in_room)].ship->maxpinternal
#define GREYHAWK_SHIPMAXSINTERNAL(in_room) world[(in_room)].ship->maxsinternal
#define GREYHAWK_SHIPFINTERNAL(in_room) world[(in_room)].ship->finternal
#define GREYHAWK_SHIPRINTERNAL(in_room) world[(in_room)].ship->rinternal
#define GREYHAWK_SHIPPINTERNAL(in_room) world[(in_room)].ship->pinternal
#define GREYHAWK_SHIPSINTERNAL(in_room) world[(in_room)].ship->sinternal
#define GREYHAWK_SHIPHULLWEIGHT(in_room) world[(in_room)].ship->hullweight
#define GREYHAWK_SHIPMAXSLOTS(in_room) world[(in_room)].ship->maxslots

#define GREYHAWK_SHIPSAILNAME(in_room) world[(in_room)].ship->sailcrew.crewname
#define GREYHAWK_SHIPGUNNAME(in_room) world[(in_room)].ship->guncrew.crewname
#define GREYHAWK_SHIPSLOT(in_room) world[(in_room)].ship->slot

#define GREYHAWK_SHIPID(in_room) world[(in_room)].ship->id
#define GREYHAWK_SHIPOWNER(in_room) world[(in_room)].ship->owner
#define GREYHAWK_SHIPNAME(in_room) world[(in_room)].ship->name
#define GREYHAWK_SHIPNUM(in_room) world[(in_room)].ship->shipnum
#define GREYHAWK_SHIPOBJ(in_room) world[(in_room)].ship->shipobj

#define GREYHAWK_SHIPX(in_room) world[(in_room)].ship->x
#define GREYHAWK_SHIPY(in_room) world[(in_room)].ship->y
#define GREYHAWK_SHIPZ(in_room) world[(in_room)].ship->z
#define GREYHAWK_SHIPHEADING(in_room) world[(in_room)].ship->heading
#define GREYHAWK_SHIPSETHEADING(in_room) world[(in_room)].ship->setheading
#define GREYHAWK_SHIPSPEED(in_room) world[(in_room)].ship->speed
#define GREYHAWK_SHIPSETSPEED(in_room) world[(in_room)].ship->setspeed
#define GREYHAWK_SHIPMAXSPEED(in_room) world[(in_room)].ship->maxspeed
#define GREYHAWK_SHIPLOCATION(in_room) world[(in_room)].ship->location
#define GREYHAWK_SHIPMINSPEED(in_room) world[(in_room)].ship->minspeed

#endif /* VESSELS_ENABLE_GREYHAWK */

/* ========================================================================= */
/* COMMAND PROTOTYPES (ADVANCED PLACEHOLDERS)                                */
/* ========================================================================= */
/* Future commands - not yet implemented */
ACMD_DECL(do_board);               /* Board a vessel */
/* ACMD_DECL(do_pilot); */         /* Pilot a vessel */
/* ACMD_DECL(do_vessel_status); */ /* Show vessel status */

/* ========================================================================= */
/* GREYHAWK SHIP DATA STRUCTURES                                            */
/* ========================================================================= */

/* Greyhawk Ship Equipment Slot Structure */
struct greyhawk_ship_slot
{
  char type;                   /* Type of slot (1=weapon, 2=oarsman, 3=ammo) */
  char position;               /* Position: FORE/PORT/REAR/STARBOARD */
  unsigned char weight;        /* Weight of equipment */
  char desc[256];              /* Description of slot equipment */
  char val0, val1, val2, val3; /* Equipment values (range, damage, etc.) */
  unsigned char x, y;          /* Slot x,y position on ship room */
  short int timer;             /* Reload/action timer */
};

/* Greyhawk Ship Crew Structure */
struct greyhawk_ship_crew
{
  char crewname[256]; /* Crew description */
  char speedadjust;   /* Speed adjustment modifier */
  char gunadjust;     /* Gunnery adjustment modifier */
  char repairspeed;   /* Repair speed modifier */
};

/* Maximum ships rooms and connections for Phase 2 */
#define MAX_SHIP_ROOMS 20       /* Maximum interior rooms per ship */
#define MAX_SHIP_CONNECTIONS 40 /* Maximum connections between rooms */

/* VNUM range for dynamically generated ship interior rooms */
/* Slot 0 is reserved; active slots 1-500 use 70020-80019. */
#define SHIP_INTERIOR_VNUM_BASE 70000   /* Base VNUM for ship interiors */
#define SHIP_INTERIOR_VNUM_MAX 80019    /* Last interior VNUM for ship slot 500 */
#define VESSEL_BASE_HULL_OBJ_VNUM 70002 /* Generic boardable vessel object */

/* Ship room types for multi-room vessels */
enum ship_room_type
{
  ROOM_TYPE_BRIDGE,      /* Command center/helm */
  ROOM_TYPE_QUARTERS,    /* Crew quarters */
  ROOM_TYPE_CARGO,       /* Cargo hold */
  ROOM_TYPE_ENGINEERING, /* Engine room */
  ROOM_TYPE_WEAPONS,     /* Weapons bay */
  ROOM_TYPE_MEDICAL,     /* Medical bay */
  ROOM_TYPE_MESS_HALL,   /* Dining area */
  ROOM_TYPE_CORRIDOR,    /* Hallway/passage */
  ROOM_TYPE_AIRLOCK,     /* Entry/exit point */
  ROOM_TYPE_DECK         /* Open deck area */
};

/* Room connection structure for ship interiors */
struct room_connection
{
  int from_room;  /* Source room vnum */
  int to_room;    /* Destination room vnum */
  int direction;  /* Direction of connection */
  bool is_hatch;  /* Sealable connection */
  bool is_locked; /* Currently locked/sealed */
};

/* ========================================================================= */
/* AUTOPILOT DATA STRUCTURES                                                 */
/* ========================================================================= */

/* Forward declaration for autopilot_data */
struct autopilot_data;

/* Forward declarations for cache node structures */
struct waypoint_node;
struct route_node;

/**
 * Individual navigation waypoint.
 * Stores coordinates and metadata for a single point on a route.
 */
struct waypoint
{
  float x;                          /* Target X coordinate */
  float y;                          /* Target Y coordinate */
  float z;                          /* Target Z coordinate (altitude/depth) */
  char name[AUTOPILOT_NAME_LENGTH]; /* Waypoint name */
  float tolerance;                  /* Arrival distance threshold */
  int wait_time;                    /* Seconds to wait at waypoint */
  int flags;                        /* Waypoint flags (future use) */
};

/**
 * Ship navigation route.
 * Ordered collection of waypoints with route metadata.
 */
struct ship_route
{
  int route_id;                                       /* Unique route identifier */
  char name[AUTOPILOT_NAME_LENGTH];                   /* Route name */
  struct waypoint waypoints[MAX_WAYPOINTS_PER_ROUTE]; /* Waypoint array */
  int num_waypoints;                                  /* Actual waypoint count */
  bool loop;                                          /* Repeat route when complete */
  bool active;                                        /* Route is available */
};

/**
 * Autopilot state data.
 * Attached to greyhawk_ship_data to provide autonomous navigation.
 */
struct autopilot_data
{
  enum autopilot_state state;       /* Current autopilot state */
  struct ship_route *current_route; /* Owned assigned route (NULL if none) */
  int current_waypoint_index;       /* Index in route waypoints array */
  int tick_counter;                 /* Ticks since last update */
  int wait_remaining;               /* Seconds left at current waypoint */
  time_t last_update;               /* Timestamp of last state update */
  int pilot_mob_vnum;               /* VNUM of NPC pilot (-1 if none) */
  uint64_t movement_steps;          /* Successful autonomous position updates */
  uint64_t waypoint_arrivals;       /* Waypoints reached since initialization */
  uint64_t route_completions;       /* Complete route traversals */
};

/**
 * Vessel schedule data for timer-based route automation.
 * Enables vessels to start routes at fixed MUD hour intervals.
 */
struct vessel_schedule
{
  int schedule_id;    /* Database ID for persistence */
  int ship_id;        /* Ship index this schedule belongs to */
  int route_id;       /* Route to start when triggered */
  int interval_hours; /* MUD hours between departures */
  int next_departure; /* MUD hour for next departure */
  int passenger_fare; /* Gold charged by an unowned public vessel at boarding */
  int flags;          /* SCHEDULE_FLAG_* bits */
};

/* ========================================================================= */
/* WAYPOINT/ROUTE CACHE STRUCTURES                                           */
/* ========================================================================= */

/**
 * Waypoint cache node for in-memory linked list.
 * Stores database ID and waypoint data for fast lookups.
 */
struct waypoint_node
{
  int waypoint_id;            /* Database ID */
  struct waypoint data;       /* Waypoint data */
  struct waypoint_node *next; /* Next node in list */
};

/**
 * Route cache node for in-memory linked list.
 * Stores database ID and route data for fast lookups.
 */
struct route_node
{
  int route_id;                     /* Database ID */
  char name[AUTOPILOT_NAME_LENGTH]; /* Route name */
  bool loop;                        /* Repeat route when complete */
  bool active;                      /* Route is available */
  int num_waypoints;                /* Count of waypoints in route */
  int *waypoint_ids;                /* Array of waypoint IDs (ordered) */
  struct route_node *next;          /* Next node in list */
};

/* Global cache list heads (defined in vessels_autopilot.c) */
extern struct waypoint_node *waypoint_list;
extern struct route_node *route_list;

/* Greyhawk Ship Data Structure */
struct greyhawk_ship_data
{
  /* Armor System - different sides of ship */
  unsigned char maxfarmor, maxrarmor, maxparmor, maxsarmor;             /* Max armor values */
  unsigned char maxfinternal, maxrinternal, maxsinternal, maxpinternal; /* Max internal */
  unsigned char farmor, finternal; /* Fore armor/internal current */
  unsigned char rarmor, rinternal; /* Rear armor/internal current */
  unsigned char sarmor, sinternal; /* Starboard armor/internal current */
  unsigned char parmor, pinternal; /* Port armor/internal current */

  /* Ship Performance */
  unsigned char maxturnrate, turnrate; /* Maximum/current turn rate */
  unsigned char mainsail, maxmainsail; /* Main sail HP/condition */
  unsigned char hullweight;            /* Weight of hull (in thousands) */
  unsigned char maxslots;              /* Maximum number of equipment slots */

  /* Position and Movement */
  float x, y, z;    /* Current coordinates */
  float dx, dy, dz; /* Delta movement vectors */

  /* Crew */
  struct greyhawk_ship_crew sailcrew; /* Sailing crew */
  struct greyhawk_ship_crew guncrew;  /* Gunnery crew */

  /* Equipment */
  struct greyhawk_ship_slot slot[GREYHAWK_MAXSLOTS]; /* Equipment slots */

  /* Identification */
  char owner[64];           /* Ship owner name */
  struct obj_data *shipobj; /* Associated ship object */
  char name[128];           /* Ship name */
  char id[3];               /* Ship ID designation (AA-ZZ) */
  int prototype_id;         /* Builder prototype used to create this instance */
  int hull_object_vnum;     /* Object prototype used for the exterior hull */

  /* Location and Status */
  int dock;     /* Docked room number */
  int shiproom; /* Ship interior room vnum */
  int shipnum;  /* Canonical fleet slot and persistent identity */
  int location; /* Current world location */
  bool active;  /* Slot occupancy; independent of shipnum */

  /* Navigation */
  short int heading;            /* Current heading (0-360) */
  short int setheading;         /* Set heading (target) */
  short int minspeed, maxspeed; /* Speed range */
  short int speed, setspeed;    /* Current and target speed */

  /* Events */
  event_handle_t periodic_event_handle;
  uint64_t periodic_generation;
  bool periodic_registered;
  struct greyhawk_ship_data *periodic_prev;
  struct greyhawk_ship_data *periodic_next;

  /* Phase 2: Multi-room additions */
  enum vessel_class vessel_type;  /* Type of vessel (raft, ship, warship, etc.) */
  int num_rooms;                  /* Current room count (1-20) */
  int room_vnums[MAX_SHIP_ROOMS]; /* Interior room vnums */
  int entrance_room;              /* Primary boarding point */
  int bridge_room;                /* Control room vnum */
  int cargo_rooms[5];             /* Cargo hold vnums */
  int crew_quarters[10];          /* Crew room vnums */

  /* Room connectivity */
  struct room_connection connections[MAX_SHIP_CONNECTIONS];
  int num_connections;

  /* Docking system */
  int docked_to_ship;   /* Index of docked ship (-1 if none) */
  int docking_room;     /* Room used for docking */
  int max_docked_ships; /* How many can dock */

  /* Room discovery */
  float discovery_chance;             /* Probability of additional rooms */
  int room_templates[MAX_SHIP_ROOMS]; /* enum ship_room_type for each room */

  /* Phase 3: Autopilot system */
  struct autopilot_data *autopilot; /* Autopilot data (NULL if disabled) */
  struct vessel_schedule *schedule; /* Schedule data (NULL if none) */

  /* Canonical geographic region last observed by the vessel. This cache is
   * runtime-only and prevents duplicate named-water crossing messages. */
  int waters_region_vnum;
  bool waters_region_initialized;

  /* Phase 5: Naval combat */
  int last_attacker;           /* Fleet index of last ship to fire on us (0 = none) */
  time_t pvp_grace_until;      /* End of the bounded combat-logout window */
  char pvp_grace_attacker[64]; /* Only this already-consented player may continue */

  /* Phase 15: runtime cache for a durable bounty-hunter lifecycle. The
   * canonical row lives in vessel_bounty_hunts and is reattached at boot. */
  bool bounty_hunter;
  int hunter_encounter_id;
  int hunter_target_ship_id;
  char hunter_target_name[64];
  time_t hunter_expires_at;
  time_t hunter_target_missing_since;
  time_t hunter_last_runtime_save;
  int hunter_min_bounty;
  int hunter_pursuit_speed;
  int hunter_target_grace_seconds;
  int hunter_cooldown_seconds;
  int hunter_bounty_check_ticks;

  /* Phase 7: one berthing charge per arrival at a public or clan port */
  int dock_fee_balance; /* Gold due before the vessel may leave */
  int dock_fee_port;    /* Port room that assessed the current visit */
  int dock_fee_clan;    /* Clan that owned the port when the fee was assessed */

  /* Phase 6: Ownership and permissions */
#define MAX_HELM_PERMITS 10
  char helm_permits[MAX_HELM_PERMITS][21]; /* Player names cleared to helm */
  int num_permits;                         /* Active permit count */

  /* Phase 6: Hired crew. Tier 0 = position unfilled; 1-3 = green/able/
   * veteran. Bonuses are mirrored into sailcrew/guncrew on hire. */
  int crew_tier[4]; /* Indexed by CREW_SAILMASTER..CREW_QUARTERMASTER */
  int wages_owed;   /* Accrued unpaid wages in gold */
  int wage_ticks;   /* Ticks since last wage accrual */

  /* Phase 6: Upgrades, upkeep, and insurance */
  int upgrades;    /* SHIP_UPGRADE_* bitfield */
  int wear_ticks;  /* Ticks since last hull wear */
  int insured_for; /* Payout value if sunk (0 = uninsured) */

  /* Phase 7: Bulk cargo lots (trade goods, distinct from loaded vehicles) */
#define MAX_CARGO_LOTS 10
  struct cargo_lot
  {
    int commodity_id; /* Row id in trade_commodities (0 = empty lot) */
    int quantity;     /* Units carried */
  } cargo[MAX_CARGO_LOTS];
  int num_cargo_lots;

  /* Phase 14: data-driven NPC merchant identity. The definition table is
   * authoritative; these fields are rebuilt at boot and after respawn. */
  int merchant_id;
  unsigned int merchant_generation;
  int merchant_faction_id;

  /* Runtime-only player-message cooldowns; not written to vessel persistence. */
  uint64_t message_last_pulse[NUM_VESSEL_MESSAGE_KEYS];
  unsigned int message_seen_mask;
};

/* Greyhawk Contact Data Structure (for radar/sensors) */
struct greyhawk_contact_data
{
  int shipnum; /* Ship number being tracked */
  int x, y, z; /* Contact coordinates */
  int bearing; /* Bearing to contact */
  float range; /* Range to contact */
  char arc[3]; /* Firing arc (F/P/R/S) */
};

/* ========================================================================= */
/* FUNCTION PROTOTYPES - GREYHAWK SHIP SYSTEM                              */
/* ========================================================================= */

/* Core Ship Management Functions */
void greyhawk_initialize_ships(void);
int vessel_relink_world_objects(void);
void vessel_build_hull_keywords(char *buffer, size_t buffer_size, const char *name);
bool vessel_hull_is_managed(const struct obj_data *obj);
int greyhawk_loadship(int template, int to_room, short int x_cord, short int y_cord,
                      short int z_cord);
void greyhawk_nameship(char *name, int shipnum);
bool greyhawk_setsail(int class, int shipnum);

/* Ship Status and Information Functions */
void greyhawk_getstatus(int slot, int rnum);
void greyhawk_getposition(int slot, int rnum);
void greyhawk_dispweapon(int slot, int rnum);

/* Navigation and Movement Functions */
int greyhawk_bearing(float x1, float y1, float x2, float y2);
float greyhawk_range(float x1, float y1, float z1, float x2, float y2, float z2);
int greyhawk_weaprange(int shipnum, int slot, char range);

/* Contact and Radar Functions */
void greyhawk_dispcontact(int i);
int greyhawk_getcontacts(int shipnum);
void greyhawk_setcontact(int i, struct obj_data *obj, int shipnum, int xoffset, int yoffset);
int greyhawk_getarc(int ship1, int ship2);

/* ========================================================================= */
/* PHASE 2: MULTI-ROOM FUNCTIONS                                            */
/* ========================================================================= */

/* Room Generation and Management */
void generate_ship_interior(struct greyhawk_ship_data *ship);
bool restore_ship_interior(struct greyhawk_ship_data *ship);
void load_ship_room_templates_from_db(void); /* Boot: builder template overrides */
int create_ship_room(struct greyhawk_ship_data *ship, enum ship_room_type type);
void add_ship_room(struct greyhawk_ship_data *ship, enum ship_room_type type);
void generate_room_connections(struct greyhawk_ship_data *ship);
int get_base_rooms_for_type(enum vessel_class type);
int get_max_rooms_for_type(enum vessel_class type);
enum vessel_class derive_vessel_type_from_template(int hullweight);
bool ship_has_interior_rooms(struct greyhawk_ship_data *ship);

/* Room Navigation */
bool is_in_ship_interior(struct char_data *ch);
void do_move_ship_interior(struct char_data *ch, int dir);
struct greyhawk_ship_data *get_ship_from_room(room_rnum room);
room_rnum get_ship_exit(struct greyhawk_ship_data *ship, room_rnum current, int dir);
bool is_passage_blocked(struct greyhawk_ship_data *ship, room_rnum room, int dir);
bool room_has_outside_view(room_rnum room);

/* Coordinate Synchronization */
void update_ship_room_coordinates(struct greyhawk_ship_data *ship);
void update_room_ship_status(room_rnum room, struct greyhawk_ship_data *ship);

/* Docking Mechanics */
void initiate_docking(struct greyhawk_ship_data *ship1, struct greyhawk_ship_data *ship2);
void complete_docking(struct greyhawk_ship_data *ship1, struct greyhawk_ship_data *ship2);
bool ships_in_docking_range(struct greyhawk_ship_data *ship1, struct greyhawk_ship_data *ship2);
room_rnum find_docking_room(struct greyhawk_ship_data *ship);
void create_ship_connection(room_rnum room1, room_rnum room2, int dir);
void remove_ship_connection(room_rnum room1, room_rnum room2);
void separate_vessels(struct greyhawk_ship_data *ship1, struct greyhawk_ship_data *ship2);

/* Boarding Functions */
enum vessel_boarding_stage
{
  VESSEL_BOARDING_GRAPPLE = 0,
  VESSEL_BOARDING_CROSSING
};

struct vessel_boarding_contest
{
  int attacker_skill;
  int attacker_roll;
  int defender_skill;
  int defender_roll;
  int vessel_modifier;
  int attacker_total;
  int defender_total;
  bool attacker_wins;
  bool critical_failure;
};

bool can_attempt_boarding(struct char_data *ch, struct greyhawk_ship_data *target);
bool perform_combat_boarding(struct char_data *ch, struct greyhawk_ship_data *target);
void setup_boarding_defenses(struct greyhawk_ship_data *ship);
int vessel_boarding_defense_modifier(const struct greyhawk_ship_data *target,
                                     enum vessel_boarding_stage stage);
bool vessel_resolve_boarding_contest(int attacker_skill, int attacker_roll, int defender_skill,
                                     int defender_roll, int vessel_modifier,
                                     struct vessel_boarding_contest *result);
void vessel_abort_docking(struct greyhawk_ship_data *ship);

/* Ship Persistence */
bool save_ship_interior(struct greyhawk_ship_data *ship);
void load_ship_interior(struct greyhawk_ship_data *ship);
void serialize_ship_rooms(struct greyhawk_ship_data *ship, char *buffer);
bool vessel_db_save_runtime(struct greyhawk_ship_data *ship);
bool vessel_db_load_runtime(struct greyhawk_ship_data *ship);
bool vessel_db_save_weapons(struct greyhawk_ship_data *ship);
bool vessel_db_load_weapons(struct greyhawk_ship_data *ship);
bool vessel_place_hull_object(struct greyhawk_ship_data *ship, struct obj_data *obj);
void vessel_persistence_ensure_schema(void);
int vessel_serialize_slot_state(const struct greyhawk_ship_data *ship, char *buffer,
                                size_t buffer_size);
int vessel_deserialize_slot_state(struct greyhawk_ship_data *ship, const char *data);
bool vessel_delete_persistence(int shipnum);

/* NPC Pilot Persistence */
bool vessel_db_save_pilot(struct greyhawk_ship_data *ship);
void vessel_db_load_pilot(struct greyhawk_ship_data *ship);

/* Persistence Lifecycle Functions */
int is_valid_ship(const struct greyhawk_ship_data *ship);
void load_all_ship_interiors(void);
bool save_all_vessels(void);
int vessel_reclaim_interior_rooms(struct greyhawk_ship_data *ship, room_rnum evacuation_room);

/* Docking Record Persistence */
void save_docking_record(struct greyhawk_ship_data *ship1, struct greyhawk_ship_data *ship2,
                         const char *dock_type);
void end_docking_record(struct greyhawk_ship_data *ship1, struct greyhawk_ship_data *ship2);

/* Utility Functions */
void vessel_build_hull_description(char *buffer, size_t buffer_size,
                                   const struct greyhawk_ship_data *ship);
bool vessel_format_appearance(char *buffer, size_t buffer_size,
                              const struct greyhawk_ship_data *ship);
bool vessel_refresh_hull_strings(struct greyhawk_ship_data *ship, bool refresh_identity);
const char *vessel_figurehead(const struct greyhawk_ship_data *ship);
const char *vessel_paint_scheme(const struct greyhawk_ship_data *ship);
void vessel_set_figurehead(struct greyhawk_ship_data *ship, const char *value);
void vessel_set_paint_scheme(struct greyhawk_ship_data *ship, const char *value);
void vessel_reset_customization(struct greyhawk_ship_data *ship);
struct greyhawk_ship_data *find_ship_by_name(const char *name);
struct greyhawk_ship_data *get_ship_by_id(int id);
bool is_pilot(struct char_data *ch, struct greyhawk_ship_data *ship);
void send_to_ship(struct greyhawk_ship_data *ship, const char *format, ...);
void send_to_ship_throttled(struct greyhawk_ship_data *ship, enum vessel_message_key key,
                            uint64_t cooldown_pulses, const char *format, ...);
bool vessel_message_allowed(struct greyhawk_ship_data *ship, enum vessel_message_key key,
                            uint64_t now_pulse, uint64_t cooldown_pulses);
void show_wilderness_from_ship(struct char_data *ch, struct greyhawk_ship_data *ship);
void show_nearby_vessels(struct char_data *ch, struct greyhawk_ship_data *ship);

/* Wilderness-backed tactical chart helpers. */
bool vessel_tactical_sector_is_water(int sector_type);
char vessel_tactical_terrain_symbol(int sector_type, bool coastal);
int vessel_tactical_range_ring(int delta_x, int delta_y);
bool vessel_tactical_region_type_visible(int region_type);
char vessel_tactical_contact_symbol(int status, int contact_count);

/* ========================================================================= */
/* PHASE 3: AUTOPILOT FUNCTIONS                                              */
/* ========================================================================= */

/* Autopilot Lifecycle Functions */
struct autopilot_data *autopilot_init(struct greyhawk_ship_data *ship);
void autopilot_cleanup(struct greyhawk_ship_data *ship);
void vessel_navigation_shutdown(void);
int autopilot_start(struct greyhawk_ship_data *ship, struct ship_route *route);
int autopilot_stop(struct greyhawk_ship_data *ship);
int autopilot_pause(struct greyhawk_ship_data *ship);
int autopilot_resume(struct greyhawk_ship_data *ship);

/* Waypoint Management Functions */
int waypoint_add(struct ship_route *route, float x, float y, float z, const char *name);
int waypoint_remove(struct ship_route *route, int index);
void waypoint_clear_all(struct ship_route *route);
struct waypoint *waypoint_get_current(struct greyhawk_ship_data *ship);
struct waypoint *waypoint_get_next(struct greyhawk_ship_data *ship);

/* Route Management Functions */
struct ship_route *route_create(const char *name);
void route_destroy(struct ship_route *route);
int route_load(struct ship_route *route, int route_id);
int route_save(struct ship_route *route);
int route_activate(struct ship_route *route);
int route_deactivate(struct ship_route *route);

/* Path-Following Functions (Session 03) */
float calculate_distance_to_waypoint(const struct greyhawk_ship_data *ship,
                                     const struct waypoint *wp);
void calculate_heading_to_waypoint(struct greyhawk_ship_data *ship, struct waypoint *wp, float *dx,
                                   float *dy);
int check_waypoint_arrival(const struct greyhawk_ship_data *ship, const struct waypoint *wp);
int advance_to_next_waypoint(struct greyhawk_ship_data *ship);
void handle_waypoint_arrival(struct greyhawk_ship_data *ship);
int vessel_autopilot_grid_coordinate(float coordinate);
bool vessel_autopilot_next_position(const struct greyhawk_ship_data *ship,
                                    const struct waypoint *wp, float speed, int *target_x,
                                    int *target_y, int *target_z);
int move_vessel_toward_waypoint(struct greyhawk_ship_data *ship);
void process_waiting_vessel(struct greyhawk_ship_data *ship);
void process_traveling_vessel(struct greyhawk_ship_data *ship);
void autopilot_tick(void);
bool autopilot_tick_one(struct greyhawk_ship_data *ship);

/* ========================================================================= */
/* WAYPOINT/ROUTE DATABASE PERSISTENCE                                       */
/* ========================================================================= */

/* Waypoint Database CRUD Functions */
int waypoint_db_create(const struct waypoint *wp);
struct waypoint_node *waypoint_db_load(int waypoint_id);
int waypoint_db_update(int waypoint_id, const struct waypoint *wp);
int waypoint_db_delete(int waypoint_id);

/* Route Database CRUD Functions */
int route_db_create(const char *name, bool loop_route);
struct route_node *route_db_load(int route_id);
int route_db_update(int route_id, const char *name, bool loop_route, bool active);
int route_db_delete(int route_id);

/* Route-Waypoint Association Functions */
int route_add_waypoint_db(int route_id, int waypoint_id, int sequence_num);
int route_remove_waypoint_db(int route_id, int waypoint_id);
int route_reorder_waypoints_db(int route_id, int *waypoint_ids, int count);
int route_get_waypoint_ids(int route_id, int **waypoint_ids, int *count);

/* Boot-time Loading Functions */
void load_all_waypoints(void);
void load_all_routes(void);

/* Shutdown Saving Functions */
void save_all_waypoints(void);
void save_all_routes(void);

/* Cache Management Functions */
void waypoint_cache_clear(void);
void route_cache_clear(void);
struct waypoint_node *waypoint_cache_find(int waypoint_id);
struct route_node *route_cache_find(int route_id);

/* ========================================================================= */
/* SCHEDULE SYSTEM FUNCTIONS                                                  */
/* ========================================================================= */

/* Schedule Management Functions */
int schedule_create(struct greyhawk_ship_data *ship, int route_id, int interval,
                    int passenger_fare);
int schedule_clear(struct greyhawk_ship_data *ship);
int schedule_is_enabled(struct greyhawk_ship_data *ship);
struct vessel_schedule *schedule_get(struct greyhawk_ship_data *ship);

/* Schedule Persistence Functions */
int schedule_save(struct greyhawk_ship_data *ship);
int schedule_load(struct greyhawk_ship_data *ship);
void load_all_schedules(void);
void save_all_schedules(void);

/* Schedule Timer Functions */
void schedule_tick(void);
void schedule_tick_one(struct greyhawk_ship_data *ship);
int schedule_check_trigger(struct greyhawk_ship_data *ship);
int schedule_trigger_departure(struct greyhawk_ship_data *ship);
void schedule_calculate_next_departure(struct vessel_schedule *sched);

/* Database Table Management */
void ensure_schedule_table_exists(void);

/* ========================================================================= */
/* COMMAND PROTOTYPES                                                        */
/* ========================================================================= */

/* Greyhawk System Commands */
ACMD_DECL(do_greyhawk_tactical);  /* Display tactical map */
ACMD_DECL(do_greyhawk_contacts);  /* Show ship contacts/radar */
ACMD_DECL(do_greyhawk_status);    /* Show detailed ship status */
ACMD_DECL(do_greyhawk_speed);     /* Control ship speed */
ACMD_DECL(do_greyhawk_heading);   /* Set ship heading/direction */
ACMD_DECL(do_greyhawk_disembark); /* Leave ship */
ACMD_DECL(do_greyhawk_shipload);  /* Admin: Load a new ship */
ACMD_DECL(do_vedit);              /* Builder: ship prototype editor (vessels_edit.c) */
ACMD_DECL(do_greyhawk_setsail);   /* Move a helmed vessel in a direction */

/* Phase 2 Commands */
ACMD_DECL(do_dock);           /* Dock with another vessel */
ACMD_DECL(do_undock);         /* Undock from vessel */
ACMD_DECL(do_board_hostile);  /* Combat boarding */
ACMD_DECL(do_look_outside);   /* Look outside from ship interior */
ACMD_DECL(do_shiptalk);       /* Speak across every room aboard one vessel */
ACMD_DECL(do_transfer_cargo); /* Transfer cargo between docked ships */
ACMD_DECL(do_ship_rooms);     /* List ship interior rooms */

/* Phase 3 Autopilot Commands */
ACMD_DECL(do_autopilot);     /* Autopilot control (on/off/status) */
ACMD_DECL(do_setwaypoint);   /* Create waypoint at current position */
ACMD_DECL(do_listwaypoints); /* List all waypoints */
ACMD_DECL(do_delwaypoint);   /* Delete a waypoint */
ACMD_DECL(do_createroute);   /* Create a new route */
ACMD_DECL(do_addtoroute);    /* Add waypoint to route */
ACMD_DECL(do_delroute);      /* Delete a route */
ACMD_DECL(do_listroutes);    /* List all routes */
ACMD_DECL(do_setroute);      /* Assign route to vessel */

/* Phase 3 NPC Pilot Commands */
ACMD_DECL(do_assignpilot);   /* Assign NPC pilot to vessel */
ACMD_DECL(do_unassignpilot); /* Remove NPC pilot from vessel */

/* Phase 3 Schedule Commands */
ACMD_DECL(do_setschedule);   /* Set vessel departure schedule */
ACMD_DECL(do_clearschedule); /* Clear vessel schedule */
ACMD_DECL(do_showschedule);  /* Display vessel schedule status */

/* ========================================================================= */
/* NPC PILOT FUNCTIONS                                                        */
/* ========================================================================= */

/**
 * Validates if an NPC can serve as pilot for a vessel.
 *
 * @param ch The captain issuing the assignment
 * @param npc The NPC to validate as pilot
 * @param ship The vessel to assign pilot to
 * @return TRUE if valid pilot, FALSE otherwise (sends error to ch)
 */
int is_valid_pilot_npc(struct char_data *ch, struct char_data *npc,
                       struct greyhawk_ship_data *ship);

/**
 * Finds the pilot NPC for a ship by matching pilot_mob_vnum.
 *
 * @param ship The vessel to find pilot for
 * @return Pointer to pilot NPC, or NULL if not found
 */
struct char_data *get_pilot_from_ship(struct greyhawk_ship_data *ship);

/**
 * Checks whether an NPC is the assigned pilot at its vessel's bridge.
 *
 * @param npc The NPC to inspect
 * @return TRUE while the NPC is on active pilot duty
 */
bool vessel_npc_is_on_pilot_duty(const struct char_data *npc);

/**
 * Announces waypoint arrival to all vessel occupants.
 *
 * @param ship The vessel arriving at waypoint
 * @param wp The waypoint being arrived at
 */
void pilot_announce_waypoint(struct greyhawk_ship_data *ship, struct waypoint *wp);

/* ========================================================================= */
/* SIMPLE VEHICLE SYSTEM (Phase 02)                                          */
/* ========================================================================= */
/* Land-based transport vehicles: carts, wagons, mounts, carriages           */
/* Unlike vessels, vehicles use room-to-room movement (no wilderness coords) */
/* ========================================================================= */

/* ========================================================================= */
/* VEHICLE TYPE ENUMERATION                                                   */
/* ========================================================================= */

/**
 * Vehicle type classification for land-based transport.
 * Each type has distinct passenger capacity, weight limits, and speed.
 */
enum vehicle_type
{
  VEHICLE_NONE = 0, /* Invalid/uninitialized */
  VEHICLE_CART,     /* 1-2 passengers, low capacity, moderate speed */
  VEHICLE_WAGON,    /* 4-6 passengers, high capacity, slow speed */
  VEHICLE_MOUNT,    /* 1 rider, minimal cargo, fast movement */
  VEHICLE_CARRIAGE, /* 2-4 passengers, enclosed, moderate speed */
  NUM_VEHICLE_TYPES /* Must be last - count of vehicle types */
};

/* ========================================================================= */
/* VEHICLE STATE ENUMERATION                                                  */
/* ========================================================================= */

/**
 * Vehicle lifecycle states for state machine management.
 * Determines available actions and display behavior.
 *
 * Note: VSTATE_ON_VESSEL indicates the vehicle is loaded onto a parent vessel.
 * When a vehicle is on a vessel, its parent_vessel_id field will be > 0,
 * and its coordinates will sync with the parent vessel's location.
 */
enum vehicle_state
{
  VSTATE_IDLE = 0,   /* Stationary, not in use */
  VSTATE_MOVING,     /* Currently traveling between rooms */
  VSTATE_LOADED,     /* Carrying cargo or passengers */
  VSTATE_HITCHED,    /* Attached to another vehicle (wagon train) */
  VSTATE_DAMAGED,    /* Broken, requires repair before use */
  VSTATE_ON_VESSEL,  /* Loaded onto a water vessel (S0205) */
  NUM_VEHICLE_STATES /* Must be last - count of vehicle states */
};

/* ========================================================================= */
/* VEHICLE TERRAIN CAPABILITY FLAGS                                           */
/* ========================================================================= */

/**
 * Terrain type flags for vehicle traversal capabilities.
 * Bitfield values - multiple flags can be combined with |.
 * Example: cart_terrain = VTERRAIN_ROAD | VTERRAIN_PLAINS;
 */
#define VTERRAIN_ROAD (1 << 0)     /* Paved roads, cobblestone */
#define VTERRAIN_PLAINS (1 << 1)   /* Open grassland, fields */
#define VTERRAIN_FOREST (1 << 2)   /* Light forest, woodland paths */
#define VTERRAIN_HILLS (1 << 3)    /* Hilly terrain, gentle slopes */
#define VTERRAIN_MOUNTAIN (1 << 4) /* Mountain paths, steep terrain */
#define VTERRAIN_DESERT (1 << 5)   /* Desert, sand, arid terrain */
#define VTERRAIN_SWAMP (1 << 6)    /* Wetlands, marshes, bogs */
#define VTERRAIN_ALL 0x7F          /* All terrain flags combined */

/* Default terrain capabilities per vehicle type */
#define VTERRAIN_CART_DEFAULT (VTERRAIN_ROAD | VTERRAIN_PLAINS)
#define VTERRAIN_WAGON_DEFAULT (VTERRAIN_ROAD | VTERRAIN_PLAINS)
#define VTERRAIN_MOUNT_DEFAULT (VTERRAIN_ROAD | VTERRAIN_PLAINS | VTERRAIN_FOREST | VTERRAIN_HILLS)
#define VTERRAIN_CARRIAGE_DEFAULT (VTERRAIN_ROAD)

/* ========================================================================= */
/* VEHICLE CAPACITY CONSTANTS                                                 */
/* ========================================================================= */

/**
 * Vehicle name buffer size.
 * Sufficient for descriptive names like "Farmer John's old wooden cart".
 */
#define VEHICLE_NAME_LENGTH 64

/**
 * Maximum passengers by vehicle type.
 * These are hard limits; actual vehicles may have lower values.
 */
#define VEHICLE_PASSENGERS_CART 2     /* Cart: driver + 1 passenger */
#define VEHICLE_PASSENGERS_WAGON 6    /* Wagon: driver + 5 passengers */
#define VEHICLE_PASSENGERS_MOUNT 1    /* Mount: single rider only */
#define VEHICLE_PASSENGERS_CARRIAGE 4 /* Carriage: driver + 3 passengers */
#define VEHICLE_MAX_PASSENGERS 8      /* Absolute maximum for any vehicle */

/**
 * Maximum number of concurrent vehicles in the game.
 * 1000 vehicles x ~150 bytes = ~147KB memory usage.
 */
#define MAX_VEHICLES 1000

/**
 * Maximum weight capacity by vehicle type (in pounds).
 * Includes cargo and passengers (average 150 lbs per person).
 */
#define VEHICLE_WEIGHT_CART 500     /* Light cargo, minimal structure */
#define VEHICLE_WEIGHT_WAGON 2000   /* Heavy cargo, reinforced frame */
#define VEHICLE_WEIGHT_MOUNT 200    /* Rider + saddlebags only */
#define VEHICLE_WEIGHT_CARRIAGE 800 /* Passengers + luggage */
#define VEHICLE_MAX_WEIGHT 5000     /* Absolute maximum for any vehicle */

/* ========================================================================= */
/* VEHICLE SPEED CONSTANTS                                                    */
/* ========================================================================= */

/**
 * Base movement speed by vehicle type (rooms per movement tick).
 * Higher values = faster movement.
 * Speed may be modified by terrain, load, and condition.
 */
#define VEHICLE_SPEED_CART 2     /* Moderate speed, balanced */
#define VEHICLE_SPEED_WAGON 1    /* Slow, prioritizes capacity */
#define VEHICLE_SPEED_MOUNT 4    /* Fast, optimal for travel */
#define VEHICLE_SPEED_CARRIAGE 2 /* Moderate, prioritizes comfort */

/**
 * Speed modifiers (percentage of base speed).
 * Applied based on conditions.
 */
#define VEHICLE_SPEED_MOD_LOADED 75  /* 75% speed when fully loaded */
#define VEHICLE_SPEED_MOD_DAMAGED 50 /* 50% speed when damaged */
#define VEHICLE_SPEED_MOD_OFFROAD 50 /* 50% speed on non-road terrain */

/**
 * Terrain-based speed modifiers (percentage of base speed).
 * Applied based on sector type.
 */
#define VEHICLE_SPEED_MOD_ROAD 150    /* 150% speed on roads */
#define VEHICLE_SPEED_MOD_PLAINS 100  /* 100% speed on plains/fields */
#define VEHICLE_SPEED_MOD_FOREST 75   /* 75% speed in forests */
#define VEHICLE_SPEED_MOD_HILLS 75    /* 75% speed in hills */
#define VEHICLE_SPEED_MOD_MOUNTAIN 50 /* 50% speed in mountains */
#define VEHICLE_SPEED_MOD_SWAMP 50    /* 50% speed in swamps */
#define VEHICLE_SPEED_MOD_DESERT 75   /* 75% speed in desert */

/**
 * Wilderness coordinate bounds.
 * Vehicles use the same wilderness grid as vessels.
 */
#define VEHICLE_WILDERNESS_MIN_X (-1024)
#define VEHICLE_WILDERNESS_MAX_X (1024)
#define VEHICLE_WILDERNESS_MIN_Y (-1024)
#define VEHICLE_WILDERNESS_MAX_Y (1024)

/* ========================================================================= */
/* VEHICLE CONDITION CONSTANTS                                                */
/* ========================================================================= */

/**
 * Vehicle condition/durability values.
 * Condition degrades with use and damage, can be repaired.
 */
#define VEHICLE_CONDITION_MAX 100  /* Perfect condition */
#define VEHICLE_CONDITION_GOOD 75  /* Minor wear */
#define VEHICLE_CONDITION_FAIR 50  /* Noticeable wear */
#define VEHICLE_CONDITION_POOR 25  /* Needs repair soon */
#define VEHICLE_CONDITION_BROKEN 0 /* Cannot operate */

/* ========================================================================= */
/* VEHICLE DATA STRUCTURE                                                     */
/* ========================================================================= */

/**
 * Core vehicle data structure for land-based transport.
 * Designed to be memory-efficient (<512 bytes) for high concurrency.
 *
 * Memory layout estimate:
 *   - Identity fields: ~76 bytes (int + 2 enums + char[64])
 *   - Location fields: ~16 bytes (4 ints)
 *   - Transport fields: ~4 bytes (int for parent_vessel_id)
 *   - Capacity fields: ~16 bytes (4 ints)
 *   - Movement fields: ~12 bytes (3 ints)
 *   - Condition fields: ~8 bytes (2 ints)
 *   - Ownership: ~8 bytes (long)
 *   - Object pointer: ~8 bytes (pointer)
 *   Total: ~148 bytes
 */
struct vehicle_data
{
  /* ===== Identity Fields (T009) ===== */
  int id;                         /* Unique vehicle ID (database key) */
  enum vehicle_type type;         /* Vehicle classification */
  enum vehicle_state state;       /* Current lifecycle state */
  char name[VEHICLE_NAME_LENGTH]; /* Vehicle name/description */

  /* ===== Location Fields (T010) ===== */
  room_rnum location; /* Process-local room rnum; persistence stores its vnum */
  int direction;      /* Facing direction (0-5, matches exits) */
  int x_coord;        /* Wilderness X coordinate (-1024 to +1024) */
  int y_coord;        /* Wilderness Y coordinate (-1024 to +1024) */

  /* ===== Transport Fields (S0205) ===== */
  int parent_vessel_id; /* ID of vessel this vehicle is loaded on (0 = none) */

  /* ===== Capacity Fields (T011) ===== */
  int max_passengers;     /* Maximum passenger count */
  int current_passengers; /* Current passenger count */
  int max_weight;         /* Weight capacity in pounds */
  int current_weight;     /* Current load weight in pounds */

  /* ===== Movement Fields (T012) ===== */
  int base_speed;    /* Base movement speed (rooms/tick) */
  int current_speed; /* Modified speed after modifiers */
  int terrain_flags; /* VTERRAIN_* bitfield of capabilities */

  /* ===== Condition Fields (T013) ===== */
  int max_condition; /* Maximum durability points */
  int condition;     /* Current durability points */

  /* ===== Ownership Fields (T013) ===== */
  long owner_id; /* Player ID of owner (0 = none) */

  /* ===== Object Reference (T013) ===== */
  struct obj_data *obj; /* Associated MUD object (can be NULL) */
};

/* ========================================================================= */
/* VEHICLE FUNCTION PROTOTYPES (T014)                                         */
/* ========================================================================= */

/* Lifecycle Functions (Phase 02, Session 02) */
struct vehicle_data *vehicle_create(enum vehicle_type type, const char *name);
void vehicle_destroy(struct vehicle_data *vehicle);
void vehicle_init(struct vehicle_data *vehicle, enum vehicle_type type);

/* State Management Functions (Phase 02, Session 02) */
int vehicle_set_state(struct vehicle_data *vehicle, enum vehicle_state new_state);
enum vehicle_state vehicle_get_state(struct vehicle_data *vehicle);
const char *vehicle_state_name(enum vehicle_state state);
const char *vehicle_type_name(enum vehicle_type type);

/* Capacity Functions (Phase 02, Session 02) */
int vehicle_can_add_passenger(struct vehicle_data *vehicle);
int vehicle_add_passenger(struct vehicle_data *vehicle);
int vehicle_remove_passenger(struct vehicle_data *vehicle);
int vehicle_can_add_weight(struct vehicle_data *vehicle, int weight);
int vehicle_add_weight(struct vehicle_data *vehicle, int weight);
int vehicle_remove_weight(struct vehicle_data *vehicle, int weight);

/* Movement Functions (Phase 02, Session 03) */
int vehicle_can_move(struct vehicle_data *vehicle, int direction);
int vehicle_move(struct vehicle_data *vehicle, int direction);
int vehicle_get_speed(struct vehicle_data *vehicle);
int vehicle_can_traverse_terrain(struct vehicle_data *vehicle, int sector_type);

/* Movement Helper Functions (Phase 02, Session 03) */
void vehicle_get_direction_delta(int direction, int *dx, int *dy);
int sector_to_vterrain(int sector_type);
int get_vehicle_speed_modifier(struct vehicle_data *vehicle, int sector_type);
int move_vehicle(struct vehicle_data *vehicle, int direction);

/* Condition Functions (Phase 02, Session 02) */
int vehicle_damage(struct vehicle_data *vehicle, int amount);
int vehicle_repair(struct vehicle_data *vehicle, int amount);
int vehicle_is_operational(struct vehicle_data *vehicle);

/* Lookup Functions (Phase 02, Session 02) */
struct vehicle_data *vehicle_find_by_id(int id);
struct vehicle_data *vehicle_at_index(int index);
struct vehicle_data *vehicle_find_in_room(room_rnum room);
struct vehicle_data *vehicle_find_in_room_named(room_rnum room, const char *name);
struct vehicle_data *vehicle_find_by_obj(struct obj_data *obj);
void vehicle_reindex_room_insert(room_rnum inserted_room);
void vehicle_reindex_room_delete(room_rnum deleted_room);

/* Persistence Functions (Phase 02, Session 02) */
int vehicle_save(struct vehicle_data *vehicle);
int vehicle_load(int vehicle_id, struct vehicle_data *vehicle);
int vehicle_purge(struct vehicle_data *vehicle);
void vehicle_save_all(void);
void vehicle_load_all(void);

/* Display Functions (Phase 02, Session 04) */
void vehicle_show_status(struct char_data *ch, struct vehicle_data *vehicle);
void vehicle_show_passengers(struct char_data *ch, struct vehicle_data *vehicle);

/* ========================================================================= */
/* VEHICLE TRANSPORT PROTOTYPES (Phase 02, Session 05)                        */
/* ========================================================================= */

/* Capacity Validation (S0205) */
int check_vessel_vehicle_capacity(struct greyhawk_ship_data *vessel, struct vehicle_data *vehicle);

/* Loading/Unloading Functions (S0205) */
int load_vehicle_onto_vessel(struct char_data *ch, struct vehicle_data *vehicle,
                             struct greyhawk_ship_data *vessel);
int unload_vehicle_from_vessel(struct char_data *ch, struct vehicle_data *vehicle);

/* Vehicle List Functions (S0205) */
struct vehicle_data **get_loaded_vehicles_list(struct greyhawk_ship_data *vessel, int *count);

/* Coordinate Synchronization (S0205) */
void vehicle_sync_with_vessel(struct vehicle_data *vehicle, struct greyhawk_ship_data *vessel);
void sync_all_loaded_vehicles(struct greyhawk_ship_data *vessel);
int vehicle_release_all_from_vessel(struct greyhawk_ship_data *vessel, room_rnum exterior_room);

/* ========================================================================= */
/* VEHICLE PLAYER TRACKING (Phase 02, Session 04/06)                          */
/* ========================================================================= */

int is_player_in_vehicle(struct char_data *ch);
struct vehicle_data *get_player_vehicle(struct char_data *ch);
int register_player_mount(struct char_data *ch, struct vehicle_data *vehicle);
int unregister_player_mount(struct char_data *ch);

/* ========================================================================= */
/* VEHICLE COMMAND PROTOTYPES (Phase 02, Session 04)                          */
/* ========================================================================= */

ACMD_DECL(do_vmount);        /* Mount a vehicle (distinct from creature mount) */
ACMD_DECL(do_vdismount);     /* Dismount from vehicle (distinct from creature dismount) */
ACMD_DECL(do_hitch);         /* Hitch vehicles together */
ACMD_DECL(do_unhitch);       /* Unhitch vehicles */
ACMD_DECL(do_drive);         /* Drive a vehicle in a direction */
ACMD_DECL(do_vstatus);       /* Show vehicle status */
ACMD_DECL(do_vehiclecreate); /* Staff: create a test vehicle */
ACMD_DECL(do_vehiclepurge);  /* Staff: purge a test vehicle */
ACMD_DECL(do_loadvehicle);   /* Load vehicle onto a vessel (S0205) */
ACMD_DECL(do_unloadvehicle); /* Unload vehicle from a vessel (S0205) */

/* ========================================================================= */
/* UNIFIED TRANSPORT COMMAND PROTOTYPES (Phase 02, Session 06)                */
/* ========================================================================= */

ACMD_DECL(do_transport_enter); /* Unified entry (vehicle/vessel) */
ACMD_DECL(do_exit_transport);  /* Unified exit (vehicle/vessel) */
ACMD_DECL(do_transport_go);    /* Unified movement (vehicle/vessel) */
ACMD_DECL(do_transportstatus); /* Unified status display (vehicle/vessel) */

#endif /* _VESSELS_H_ */
