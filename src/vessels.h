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
#include "oasis.h"
#include "screen.h"
#include "interpreter.h"
#include "modify.h"
#include "handler.h"
#include "constants.h"

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
#define GREYHAWK_MAXSHIPS 500 /* Maximum number of ships in game */
#endif

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

/* Crew Role Constants (matches ship_crew_roster.crew_role ENUM) */
#define CREW_ROLE_PILOT "pilot" /* NPC vessel pilot */

/* ========================================================================= */
/* SCHEDULE SYSTEM CONSTANTS                                                 */
/* ========================================================================= */

#define SCHEDULE_INTERVAL_MIN 1  /* Minimum schedule interval (MUD hours) */
#define SCHEDULE_INTERVAL_MAX 24 /* Maximum schedule interval (MUD hours) */
#define SCHEDULE_NAME_LENGTH 64  /* Max length for schedule names */

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

/* Autopilot state machine for automated navigation */
enum autopilot_state
{
  AUTOPILOT_OFF,       /* Autopilot disabled */
  AUTOPILOT_TRAVELING, /* Moving toward waypoint */
  AUTOPILOT_WAITING,   /* At waypoint, waiting */
  AUTOPILOT_PAUSED,    /* Temporarily suspended */
  AUTOPILOT_COMPLETE   /* Route finished */
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

bool update_ship_wilderness_position(int shipnum, int new_x, int new_y, int new_z);
int get_ship_terrain_type(int shipnum);
bool can_vessel_traverse_terrain(enum vessel_class vessel_type, int x, int y, int z);
int get_terrain_speed_modifier(enum vessel_class vessel_type, int sector_type,
                               int weather_conditions);
bool move_ship_wilderness(int shipnum, int direction, struct char_data *ch);

/* Vessel type accessor functions */
const struct vessel_terrain_caps *get_vessel_terrain_caps(enum vessel_class vessel_type);
enum vessel_class get_vessel_type_from_ship(int shipnum);
const char *get_vessel_type_name(enum vessel_class vessel_type);

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
#define ITEM_SHIP 56      /* Ship object type (avoid conflicts) */
#define DOCKABLE 41       /* Room flag for dockable areas */

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

#ifndef GREYHAWK_MAXSHIPS
#define GREYHAWK_MAXSHIPS 500 /* Maximum number of ships in game */
#endif

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
#define GREYHAWK_ITEM_SHIP 57 /* Greyhawk ship object type (avoid conflicts) */

/* Greyhawk Ship Action Event Structure (unique to this conditional block) */
struct greyhawk_ship_action_event
{
  int shipnum; /* Ship performing action */
};

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
/* Using 70000-79999 range (zones 700-799 are unused by builders) */
#define SHIP_INTERIOR_VNUM_BASE 70000 /* Base VNUM for ship interiors */
#define SHIP_INTERIOR_VNUM_MAX 79999  /* Maximum VNUM for ship interiors */

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
  struct ship_route *current_route; /* Active route (NULL if none) */
  int current_waypoint_index;       /* Index in route waypoints array */
  int tick_counter;                 /* Ticks since last update */
  int wait_remaining;               /* Seconds left at current waypoint */
  time_t last_update;               /* Timestamp of last state update */
  int pilot_mob_vnum;               /* VNUM of NPC pilot (-1 if none) */
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

  /* Location and Status */
  int dock;     /* Docked room number */
  int shiproom; /* Ship interior room vnum */
  int shipnum;  /* Ship index number */
  int location; /* Current world location */

  /* Navigation */
  short int heading;            /* Current heading (0-360) */
  short int setheading;         /* Set heading (target) */
  short int minspeed, maxspeed; /* Speed range */
  short int speed, setspeed;    /* Current and target speed */

  /* Events */
  struct event *action; /* Ship action event */

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
  int room_templates[MAX_SHIP_ROOMS]; /* Template vnums for generation */

  /* Phase 3: Autopilot system */
  struct autopilot_data *autopilot; /* Autopilot data (NULL if disabled) */
  struct vessel_schedule *schedule; /* Schedule data (NULL if none) */
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

/* Greyhawk Tactical Map Structure */
struct greyhawk_ship_map
{
  char map[10]; /* Map symbol representation */
};

/* ========================================================================= */
/* FUNCTION PROTOTYPES - GREYHAWK SHIP SYSTEM                              */
/* ========================================================================= */

/* Core Ship Management Functions */
void greyhawk_initialize_ships(void);
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

/* Tactical Map Functions */
void greyhawk_getmap(int shipnum);
void greyhawk_setsymbol(int x, int y, int symbol);

/* ========================================================================= */
/* PHASE 2: MULTI-ROOM FUNCTIONS                                            */
/* ========================================================================= */

/* Room Generation and Management */
void generate_ship_interior(struct greyhawk_ship_data *ship);
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
bool can_attempt_boarding(struct char_data *ch, struct greyhawk_ship_data *target);
void perform_combat_boarding(struct char_data *ch, struct greyhawk_ship_data *target);
void setup_boarding_defenses(struct greyhawk_ship_data *ship);
int calculate_boarding_difficulty(struct greyhawk_ship_data *target);

/* Ship Persistence */
void save_ship_interior(struct greyhawk_ship_data *ship);
void load_ship_interior(struct greyhawk_ship_data *ship);
void serialize_ship_rooms(struct greyhawk_ship_data *ship, char *buffer);

/* NPC Pilot Persistence */
void vessel_db_save_pilot(struct greyhawk_ship_data *ship);
void vessel_db_load_pilot(struct greyhawk_ship_data *ship);

/* Persistence Lifecycle Functions */
int is_valid_ship(struct greyhawk_ship_data *ship);
void load_all_ship_interiors(void);
void save_all_vessels(void);

/* Docking Record Persistence */
void save_docking_record(struct greyhawk_ship_data *ship1, struct greyhawk_ship_data *ship2,
                         const char *dock_type);
void end_docking_record(struct greyhawk_ship_data *ship1, struct greyhawk_ship_data *ship2);

/* Utility Functions */
struct greyhawk_ship_data *find_ship_by_name(const char *name);
struct greyhawk_ship_data *get_ship_by_id(int id);
bool is_pilot(struct char_data *ch, struct greyhawk_ship_data *ship);
void send_to_ship(struct greyhawk_ship_data *ship, const char *format, ...);
void show_wilderness_from_ship(struct char_data *ch, struct greyhawk_ship_data *ship);
void show_nearby_vessels(struct char_data *ch, struct greyhawk_ship_data *ship);

/* ========================================================================= */
/* PHASE 3: AUTOPILOT FUNCTIONS                                              */
/* ========================================================================= */

/* Autopilot Lifecycle Functions */
struct autopilot_data *autopilot_init(struct greyhawk_ship_data *ship);
void autopilot_cleanup(struct greyhawk_ship_data *ship);
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
float calculate_distance_to_waypoint(struct greyhawk_ship_data *ship, struct waypoint *wp);
void calculate_heading_to_waypoint(struct greyhawk_ship_data *ship, struct waypoint *wp, float *dx,
                                   float *dy);
int check_waypoint_arrival(struct greyhawk_ship_data *ship, struct waypoint *wp);
int advance_to_next_waypoint(struct greyhawk_ship_data *ship);
void handle_waypoint_arrival(struct greyhawk_ship_data *ship);
int move_vessel_toward_waypoint(struct greyhawk_ship_data *ship);
void process_waiting_vessel(struct greyhawk_ship_data *ship);
void process_traveling_vessel(struct greyhawk_ship_data *ship);
void autopilot_tick(void);

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
int schedule_create(struct greyhawk_ship_data *ship, int route_id, int interval);
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
ACMD_DECL(do_greyhawk_setsail);   /* Admin: Set ship sail configuration */

/* Phase 2 Commands */
ACMD_DECL(do_dock);           /* Dock with another vessel */
ACMD_DECL(do_undock);         /* Undock from vessel */
ACMD_DECL(do_board_hostile);  /* Combat boarding */
ACMD_DECL(do_look_outside);   /* Look outside from ship interior */
ACMD_DECL(do_transfer_cargo); /* Transfer cargo between docked ships */
ACMD_DECL(do_ship_rooms);     /* List ship interior rooms */

/* Phase 3 Autopilot Commands */
ACMD_DECL(do_autopilot);     /* Autopilot control (on/off/status) */
ACMD_DECL(do_setwaypoint);   /* Create waypoint at current position */
ACMD_DECL(do_listwaypoints); /* List all waypoints */
ACMD_DECL(do_delwaypoint);   /* Delete a waypoint */
ACMD_DECL(do_createroute);   /* Create a new route */
ACMD_DECL(do_addtoroute);    /* Add waypoint to route */
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
 */
enum vehicle_state
{
  VSTATE_IDLE = 0,   /* Stationary, not in use */
  VSTATE_MOVING,     /* Currently traveling between rooms */
  VSTATE_LOADED,     /* Carrying cargo or passengers */
  VSTATE_HITCHED,    /* Attached to another vehicle (wagon train) */
  VSTATE_DAMAGED,    /* Broken, requires repair before use */
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
 *   - Location fields: ~8 bytes (int + int)
 *   - Capacity fields: ~16 bytes (4 ints)
 *   - Movement fields: ~12 bytes (3 ints)
 *   - Condition fields: ~8 bytes (2 ints)
 *   - Ownership: ~8 bytes (long)
 *   - Object pointer: ~8 bytes (pointer)
 *   Total: ~136 bytes
 */
struct vehicle_data
{
  /* ===== Identity Fields (T009) ===== */
  int id;                         /* Unique vehicle ID (database key) */
  enum vehicle_type type;         /* Vehicle classification */
  enum vehicle_state state;       /* Current lifecycle state */
  char name[VEHICLE_NAME_LENGTH]; /* Vehicle name/description */

  /* ===== Location Fields (T010) ===== */
  room_rnum location; /* Current room vnum */
  int direction;      /* Facing direction (0-5, matches exits) */
  int x_coord;        /* Wilderness X coordinate (-1024 to +1024) */
  int y_coord;        /* Wilderness Y coordinate (-1024 to +1024) */

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
struct vehicle_data *vehicle_find_in_room(room_rnum room);
struct vehicle_data *vehicle_find_by_obj(struct obj_data *obj);

/* Persistence Functions (Phase 02, Session 02) */
int vehicle_save(struct vehicle_data *vehicle);
int vehicle_load(int vehicle_id, struct vehicle_data *vehicle);
void vehicle_save_all(void);
void vehicle_load_all(void);

/* Display Functions (Phase 02, Session 04) */
void vehicle_show_status(struct char_data *ch, struct vehicle_data *vehicle);
void vehicle_show_passengers(struct char_data *ch, struct vehicle_data *vehicle);

/* ========================================================================= */
/* VEHICLE COMMAND PROTOTYPES (Phase 02, Session 04)                          */
/* ========================================================================= */

ACMD_DECL(do_vmount);    /* Mount a vehicle (distinct from creature mount) */
ACMD_DECL(do_vdismount); /* Dismount from vehicle (distinct from creature dismount) */
ACMD_DECL(do_hitch);     /* Hitch vehicles together */
ACMD_DECL(do_unhitch);   /* Unhitch vehicles */
ACMD_DECL(do_drive);     /* Drive a vehicle in a direction */
ACMD_DECL(do_vstatus);   /* Show vehicle status */

#endif /* _VESSELS_H_ */
