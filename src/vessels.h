/* ************************************************************************
 *      File:   vessels.h                            Part of LuminariMUD  *
 *   Purpose:   Vessel/Vehicle system for transportation                  *
 * Servicing:   vessels.c                                                 *
 *    Author:   [Future Development]                                      *
 *             CWG Vehicle System integration from CircleMUD              *
 ************************************************************************ */

/*
 * This file is currently disabled for compilation.
 * Remove the '#if 0' and '#endif' lines when ready to implement.
 *
 * USAGE NOTES:
 * - Create objects with ITEM_VEHICLE type to represent vehicles
 * - Create objects with ITEM_CONTROL type that control vehicles
 * - Create objects with ITEM_HATCH type for exits from vehicles
 * - Rooms that vehicles can enter need ROOM_VEHICLE flag
 * - Use 'drive <direction>' to move vehicles
 * - Use 'drive into <vehicle>' to drive into another vehicle
 * - Use 'drive outside' to drive out of a vehicle
 *
 * OBJECT VALUE SETUP:
 * - ITEM_VEHICLE: value[0] = room vnum where vehicle interior leads
 * - ITEM_CONTROL: value[0] = vnum of ITEM_VEHICLE object this controls
 * - ITEM_HATCH: value[0] = vnum of ITEM_VEHICLE object this exits from
 *
 * QUICK NAVIGATION:
 * - Item/Room Definitions: Lines 38-87
 * - Future System Functions: Lines 89-104  
 * - CWG System Functions: Lines 106-119
 * - Commands: Lines 121-132
 */
#if 0

#ifndef _VESSELS_H_
#define _VESSELS_H_

/* ========================================================================= */
/* ITEM TYPES FOR VESSEL SYSTEM                                             */
/* ========================================================================= */

#define ITEM_VEHICLE                53  /* Vehicle object - represents the actual vehicle */
#define ITEM_CONTROL                54  /* Control mechanism - steering wheel, helm, etc. */
#define ITEM_HATCH                  55  /* Exit/entry point - doorway out of vehicle */

/* ========================================================================= */
/* ROOM FLAGS FOR VESSEL SYSTEM                                             */
/* ========================================================================= */

#define ROOM_VEHICLE                40  /* Room that vehicles can move through */

/* ========================================================================= */
/* FUTURE ADVANCED VESSEL SYSTEM CONSTANTS                                  */
/* ========================================================================= */
/* These constants are for the advanced vessel system, not the CWG system   */

/* Vessel Types */
#define VESSEL_TYPE_SAILING_SHIP    1   /* Ocean-going ships */
#define VESSEL_TYPE_SUBMARINE       2   /* Underwater vessels */
#define VESSEL_TYPE_AIRSHIP         3   /* Flying craft */
#define VESSEL_TYPE_STARSHIP        4   /* Space vessels */
#define VESSEL_TYPE_MAGICAL_CRAFT   5   /* Magically powered vehicles */

/* Vessel States */
#define VESSEL_STATE_DOCKED         0   /* Parked/anchored */
#define VESSEL_STATE_TRAVELING      1   /* Moving between locations */
#define VESSEL_STATE_COMBAT         2   /* In battle */
#define VESSEL_STATE_DAMAGED        3   /* Broken down */

/* Vessel Sizes */
#define VESSEL_SIZE_SMALL           1   /* 1-2 passengers */
#define VESSEL_SIZE_MEDIUM          2   /* 3-6 passengers */
#define VESSEL_SIZE_LARGE           3   /* 7-15 passengers */
#define VESSEL_SIZE_HUGE            4   /* 16+ passengers */

/* ========================================================================= */
/* DIRECTION CONSTANTS                                                       */
/* ========================================================================= */

#ifndef OUTDIR
#define OUTDIR                      6   /* Generic "out" direction for vehicles */
#endif

/* ========================================================================= */
/* FUNCTION PROTOTYPES - FUTURE ADVANCED VESSEL SYSTEM                     */
/* ========================================================================= */
/* These functions are placeholders for a future advanced vessel system     */

/* Data Management Functions */
void load_vessels(void);                                /* Load vessel data from storage */
void save_vessels(void);                                /* Save vessel data to storage */
struct vessel_data *find_vessel_by_id(int vessel_id);  /* Find vessel by unique ID */

/* Movement and Control Functions */
void vessel_movement_tick(void);                        /* Process vessel movement each tick */
void enter_vessel(struct char_data *ch, struct vessel_data *vessel);  /* Board a vessel */
void exit_vessel(struct char_data *ch);                 /* Leave a vessel */
int can_pilot_vessel(struct char_data *ch, struct vessel_data *vessel); /* Check piloting ability */
void pilot_vessel(struct char_data *ch, int direction); /* Pilot vessel in direction */

/* ========================================================================= */
/* FUNCTION PROTOTYPES - CWG VEHICLE SYSTEM (READY TO USE)                 */
/* ========================================================================= */
/* These functions implement the CWG vehicle system from CircleMUD          */

/* Object Finding Functions */
struct obj_data *find_vehicle_by_vnum(int vnum);       /* Find vehicle object by vnum */
struct obj_data *get_obj_in_list_type(int type, struct obj_data *list); /* Find object of type in list */
struct obj_data *find_control(struct char_data *ch);   /* Find vehicle controls near player */

/* Vehicle Movement Functions */
void drive_into_vehicle(struct char_data *ch, struct obj_data *vehicle, char *arg); /* Drive into another vehicle */
void drive_outof_vehicle(struct char_data *ch, struct obj_data *vehicle);          /* Drive out of vehicle */
void drive_in_direction(struct char_data *ch, struct obj_data *vehicle, int dir);  /* Drive in a direction */

/* ========================================================================= */
/* OUTCAST SHIP SYSTEM CONSTANTS AND STRUCTURES                            */
/* ========================================================================= */
/* Integrated from Outcast MUD ship system - fully functional              */

#ifndef MAX_NUM_SHIPS
#define MAX_NUM_SHIPS               50  /* Maximum number of ships in game */
#endif

#ifndef MAX_NUM_ROOMS
#define MAX_NUM_ROOMS               20  /* Maximum rooms per ship */
#endif

#define SHIP_MAX_SPEED              30  /* Maximum ship speed value */
#define ITEM_SHIP                   56  /* Ship object type (avoid conflicts) */
#define DOCKABLE                    41  /* Room flag for dockable areas */

/* Outcast Ship Data Structure */
struct outcast_ship_data {
  int hull;                             /* max hull points */
  int speed;                            /* max speed */
  int capacity;                         /* max number of characters in ship */
  int damage;                           /* amount of damage (for firing) */
  
  int size;                             /* size of the vehicle (for ramming) */
  int velocity;                         /* current velocity */
  
  struct obj_data *obj;                 /* vehicle object */
  int obj_num;                          /* vehicle object number */
  
  int timer;                            /* timer for ship action other than moving */
  int move_timer;                       /* timer for ship movement */
  int lastdir;                          /* last direction for the ship */
  int repeat;                           /* autopilot */
  
  int in_room;                          /* room containing this ship */
  int entrance_room;                    /* room to enter/exit ship */
  int num_room;                         /* number of rooms in this vehicle */
  int room_list[MAX_NUM_ROOMS + 1];     /* room numbers in this vehicle */
  
  int dock_vehicle;                     /* docked to another ship: -1 is not docked */
};

/* Navigation Data Structure */
struct outcast_navigation_data {
  int mob;                              /* mob id that can control ship */
  bool sail;
  int control_room;                     /* control room for this mob to become a navigator */
  char *path1, *path2;                  /* path for going and returning */
  char *path;
  int start1;                           /* start room */
  int destination1;                     /* destination */
  int destination;                      /* initially set to zero */
  int sail_time;                        /* time ship start sailing */
  int freq;                             /* ship sail once every 'freq' hours */
};

/* ========================================================================= */
/* FUNCTION PROTOTYPES - OUTCAST SHIP SYSTEM                               */
/* ========================================================================= */

/* Core Ship Management */
void initialize_outcast_ships(void);
void outcast_ship_activity(void);
int find_outcast_ship(struct obj_data *obj);
bool is_outcast_ship_docked(int t_ship);
bool is_valid_outcast_ship(int t_ship);
int in_which_outcast_ship(struct char_data *ch);
void sink_outcast_ship(int t_ship);

/* Ship Movement and Control */
bool move_outcast_ship(int t_ship, int dir, struct char_data *ch);
int outcast_navigation(struct char_data *ch, int mob, int t_ship);

/* Special Procedures */
int outcast_ship_proc(struct obj_data *obj, struct char_data *ch, int cmd, char *arg);
int outcast_control_panel(struct obj_data *obj, struct char_data *ch, int cmd, char *argument);
int outcast_ship_exit_room(int room, struct char_data *ch, int cmd, char *arg);
int outcast_ship_look_out_room(int room, struct char_data *ch, int cmd, char *arg);

/* ========================================================================= */
/* GREYHAWK SHIP SYSTEM CONSTANTS AND STRUCTURES                           */
/* ========================================================================= */
/* Integrated from Greyhawk MUD - advanced naval combat and navigation     */

#ifndef GREYHAWK_MAXSHIPS
#define GREYHAWK_MAXSHIPS           500  /* Maximum number of ships in game */
#endif

#ifndef GREYHAWK_MAXSLOTS
#define GREYHAWK_MAXSLOTS           10   /* Maximum equipment slots per ship */
#endif

/* Ship Position Constants */
#define GREYHAWK_FORE               0    /* Forward position */
#define GREYHAWK_PORT               1    /* Port (left) position */
#define GREYHAWK_REAR               2    /* Rear position */
#define GREYHAWK_STARBOARD          3    /* Starboard (right) position */

/* Weapon Range Types */
#define GREYHAWK_SHRTRANGE          0    /* Short range */
#define GREYHAWK_MEDRANGE           1    /* Medium range */
#define GREYHAWK_LNGRANGE           2    /* Long range */

/* Item Type for Greyhawk Ships */
#define GREYHAWK_ITEM_SHIP          57   /* Greyhawk ship object type (avoid conflicts) */

/* Greyhawk Ship Equipment Slot Structure */
struct greyhawk_ship_slot {
  char type;                            /* Type of slot (1=weapon, 2=oarsman, 3=ammo) */
  char position;                        /* Position: FORE/PORT/REAR/STARBOARD */
  unsigned char weight;                 /* Weight of equipment */
  char desc[256];                       /* Description of slot equipment */
  char val0, val1, val2, val3;          /* Equipment values (range, damage, etc.) */
  unsigned char x, y;                   /* Slot x,y position on ship room */
  short int timer;                      /* Reload/action timer */
};

/* Greyhawk Ship Crew Structure */
struct greyhawk_ship_crew {
  char crewname[256];                   /* Crew description */
  char speedadjust;                     /* Speed adjustment modifier */
  char gunadjust;                       /* Gunnery adjustment modifier */
  char repairspeed;                     /* Repair speed modifier */
};

/* Greyhawk Ship Data Structure */
struct greyhawk_ship_data {
  /* Armor System - different sides of ship */
  unsigned char maxfarmor, maxrarmor, maxparmor, maxsarmor;    /* Max armor values */
  unsigned char maxfinternal, maxrinternal, maxsinternal, maxpinternal; /* Max internal */
  unsigned char farmor, finternal;      /* Fore armor/internal current */
  unsigned char rarmor, rinternal;      /* Rear armor/internal current */
  unsigned char sarmor, sinternal;      /* Starboard armor/internal current */
  unsigned char parmor, pinternal;      /* Port armor/internal current */
  
  /* Ship Performance */
  unsigned char maxturnrate, turnrate;  /* Maximum/current turn rate */
  unsigned char mainsail, maxmainsail;  /* Main sail HP/condition */
  unsigned char hullweight;             /* Weight of hull (in thousands) */
  unsigned char maxslots;               /* Maximum number of equipment slots */
  
  /* Position and Movement */
  float x, y, z;                        /* Current coordinates */
  float dx, dy, dz;                     /* Delta movement vectors */
  
  /* Crew */
  struct greyhawk_ship_crew sailcrew;   /* Sailing crew */
  struct greyhawk_ship_crew guncrew;    /* Gunnery crew */
  
  /* Equipment */
  struct greyhawk_ship_slot slot[GREYHAWK_MAXSLOTS]; /* Equipment slots */
  
  /* Identification */
  char owner[64];                       /* Ship owner name */
  struct obj_data *shipobj;             /* Associated ship object */
  char name[128];                       /* Ship name */
  char id[3];                           /* Ship ID designation (AA-ZZ) */
  
  /* Location and Status */
  int dock;                             /* Docked room number */
  int shiproom;                         /* Ship interior room vnum */
  int shipnum;                          /* Ship index number */
  int location;                         /* Current world location */
  
  /* Navigation */
  short int heading;                    /* Current heading (0-360) */
  short int setheading;                 /* Set heading (target) */
  short int minspeed, maxspeed;         /* Speed range */
  short int speed, setspeed;            /* Current and target speed */
  
  /* Events */
  struct event *action;                 /* Ship action event */
};

/* Greyhawk Contact Data Structure (for radar/sensors) */
struct greyhawk_contact_data {
  int shipnum;                          /* Ship number being tracked */
  int x, y, z;                          /* Contact coordinates */
  int bearing;                          /* Bearing to contact */
  float range;                          /* Range to contact */
  char arc[3];                          /* Firing arc (F/P/R/S) */
};

/* Greyhawk Ship Action Event Structure */
struct greyhawk_ship_action_event {
  int shipnum;                          /* Ship performing action */
};

/* Greyhawk Tactical Map Structure */
struct greyhawk_ship_map {
  char map[10];                         /* Map symbol representation */
};

/* ========================================================================= */
/* GREYHAWK SHIP SYSTEM MACROS                                             */
/* ========================================================================= */
/* Convenience macros for accessing ship data via room number              */

/* Armor Access Macros */
#define GREYHAWK_SHIPMAXFARMOR(in_room)    world[(in_room)].ship->maxfarmor
#define GREYHAWK_SHIPMAXRARMOR(in_room)    world[(in_room)].ship->maxrarmor
#define GREYHAWK_SHIPMAXPARMOR(in_room)    world[(in_room)].ship->maxparmor
#define GREYHAWK_SHIPMAXSARMOR(in_room)    world[(in_room)].ship->maxsarmor
#define GREYHAWK_SHIPFARMOR(in_room)       world[(in_room)].ship->farmor
#define GREYHAWK_SHIPRARMOR(in_room)       world[(in_room)].ship->rarmor
#define GREYHAWK_SHIPPARMOR(in_room)       world[(in_room)].ship->parmor
#define GREYHAWK_SHIPSARMOR(in_room)       world[(in_room)].ship->sarmor
#define GREYHAWK_SHIPMAINSAIL(in_room)     world[(in_room)].ship->mainsail
#define GREYHAWK_SHIPMAXMAINSAIL(in_room)  world[(in_room)].ship->maxmainsail

/* Internal Structure Access Macros */
#define GREYHAWK_SHIPMAXRINTERNAL(in_room) world[(in_room)].ship->maxrinternal
#define GREYHAWK_SHIPMAXFINTERNAL(in_room) world[(in_room)].ship->maxfinternal
#define GREYHAWK_SHIPMAXPINTERNAL(in_room) world[(in_room)].ship->maxpinternal
#define GREYHAWK_SHIPMAXSINTERNAL(in_room) world[(in_room)].ship->maxsinternal
#define GREYHAWK_SHIPFINTERNAL(in_room)    world[(in_room)].ship->finternal
#define GREYHAWK_SHIPRINTERNAL(in_room)    world[(in_room)].ship->rinternal
#define GREYHAWK_SHIPPINTERNAL(in_room)    world[(in_room)].ship->pinternal
#define GREYHAWK_SHIPSINTERNAL(in_room)    world[(in_room)].ship->sinternal
#define GREYHAWK_SHIPHULLWEIGHT(in_room)   world[(in_room)].ship->hullweight
#define GREYHAWK_SHIPMAXSLOTS(in_room)     world[(in_room)].ship->maxslots

/* Crew Access Macros */
#define GREYHAWK_SHIPSAILNAME(in_room)     world[(in_room)].ship->sailcrew.crewname
#define GREYHAWK_SHIPSAILSPEED(in_room)    world[(in_room)].ship->sailcrew.speedadjust
#define GREYHAWK_SHIPSAILGUN(in_room)      world[(in_room)].ship->sailcrew.gunadjust
#define GREYHAWK_SHIPSAILREPAIR(in_room)   world[(in_room)].ship->sailcrew.repairspeed
#define GREYHAWK_SHIPGUNNAME(in_room)      world[(in_room)].ship->guncrew.crewname
#define GREYHAWK_SHIPGUNSPEED(in_room)     world[(in_room)].ship->guncrew.speedadjust
#define GREYHAWK_SHIPGUNGUN(in_room)       world[(in_room)].ship->guncrew.gunadjust
#define GREYHAWK_SHIPGUNREPAIR(in_room)    world[(in_room)].ship->guncrew.repairspeed
#define GREYHAWK_SHIPSLOT(in_room)         world[(in_room)].ship->slot

/* Identification Access Macros */
#define GREYHAWK_SHIPID(in_room)           world[(in_room)].ship->id
#define GREYHAWK_SHIPOWNER(in_room)        world[(in_room)].ship->owner
#define GREYHAWK_SHIPNAME(in_room)         world[(in_room)].ship->name
#define GREYHAWK_SHIPNUM(in_room)          world[(in_room)].ship->shipnum
#define GREYHAWK_SHIPOBJ(in_room)          world[(in_room)].ship->shipobj

/* Movement and Navigation Access Macros */
#define GREYHAWK_SHIPX(in_room)            world[(in_room)].ship->x
#define GREYHAWK_SHIPY(in_room)            world[(in_room)].ship->y
#define GREYHAWK_SHIPZ(in_room)            world[(in_room)].ship->z
#define GREYHAWK_SHIPDX(in_room)           world[(in_room)].ship->dx
#define GREYHAWK_SHIPDY(in_room)           world[(in_room)].ship->dy
#define GREYHAWK_SHIPDZ(in_room)           world[(in_room)].ship->dz
#define GREYHAWK_SHIPHEADING(in_room)      world[(in_room)].ship->heading
#define GREYHAWK_SHIPSETHEADING(in_room)   world[(in_room)].ship->setheading
#define GREYHAWK_SHIPSPEED(in_room)        world[(in_room)].ship->speed
#define GREYHAWK_SHIPSETSPEED(in_room)     world[(in_room)].ship->setspeed
#define GREYHAWK_SHIPMAXSPEED(in_room)     world[(in_room)].ship->maxspeed
#define GREYHAWK_SHIPMAXTURNRATE(in_room)  world[(in_room)].ship->maxturnrate
#define GREYHAWK_SHIPTURNRATE(in_room)     world[(in_room)].ship->turnrate
#define GREYHAWK_SHIPDOCK(in_room)         world[(in_room)].ship->dock
#define GREYHAWK_SHIPROOM(in_room)         world[(in_room)].ship->shiproom
#define GREYHAWK_SHIPLOCATION(in_room)     world[(in_room)].ship->location
#define GREYHAWK_SHIPMINSPEED(in_room)     world[(in_room)].ship->minspeed

/* Event Access Macros */
#define GREYHAWK_GET_SHIP_ACTION(shipnum)  greyhawk_ships[(shipnum)].action

/* ========================================================================= */
/* FUNCTION PROTOTYPES - GREYHAWK SHIP SYSTEM                              */
/* ========================================================================= */

/* Core Ship Management Functions */
void greyhawk_initialize_ships(void);
int greyhawk_loadship(int template, int to_room, short int x_cord, short int y_cord, short int z_cord);
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

/* Special Procedures */
int greyhawk_ship_commands(struct obj_data *obj, struct char_data *ch, int cmd, char *argument);
int greyhawk_ship_object(struct obj_data *obj, struct char_data *ch, int cmd, char *argument);
int greyhawk_ship_loader(struct obj_data *obj, struct char_data *ch, int cmd, char *argument);

/* ========================================================================= */
/* COMMAND PROTOTYPES                                                        */
/* ========================================================================= */

/* Future Advanced System Commands */
ACMD(do_board);         /* Board a vessel */
ACMD(do_disembark);     /* Leave a vessel */
ACMD(do_pilot);         /* Pilot a vessel */
ACMD(do_vessel_status); /* Show vessel status */

/* CWG System Commands (Ready to Use) */
ACMD(do_drive);         /* Drive a vehicle - main command for CWG system */

/* Outcast System Commands (Ready to Use) */
ACMD(do_enter_ship);    /* Enter a ship (handled by ship special procedure) */
ACMD(do_order_ship);    /* Order ship commands (handled by control panel) */

/* Greyhawk System Commands (Ready to Use) */
ACMD(do_greyhawk_tactical);    /* Display tactical map */
ACMD(do_greyhawk_contacts);    /* Show ship contacts/radar */
ACMD(do_greyhawk_weaponspec);  /* Show weapon specifications */
ACMD(do_greyhawk_status);      /* Show detailed ship status */
ACMD(do_greyhawk_speed);       /* Control ship speed */
ACMD(do_greyhawk_heading);     /* Set ship heading/direction */
ACMD(do_greyhawk_disembark);   /* Leave ship */
ACMD(do_greyhawk_shipload);    /* Admin: Load a new ship */
ACMD(do_greyhawk_setsail);     /* Admin: Set ship sail configuration */

/* ========================================================================= */
/* INTEGRATION SUMMARY AND USAGE NOTES                                      */
/* ========================================================================= */

/*
 * VESSEL SYSTEMS INTEGRATION COMPLETE
 * ====================================
 * 
 * Three independent ship/vessel systems have been integrated into vessels.c/h:
 * 1. OUTCAST SHIP SYSTEM - Multi-room ships with combat and autopilot
 * 2. CWG VEHICLE SYSTEM - Object-based vehicles with simple drive mechanics
 * 3. GREYHAWK SHIP SYSTEM - Advanced naval combat with tactical displays
 * 
 * All systems maintain strict backward compatibility and use prefixed naming.
 * 
 * OUTCAST SHIP SYSTEM FEATURES:
 * - Multi-room ships with automatic room discovery
 * - Ship-to-ship docking and boarding
 * - Automated navigation with NPC pilots
 * - Ship combat (firing cannons, ramming)
 * - Speed control and autopilot functionality
 * 
 * CWG VEHICLE SYSTEM FEATURES:
 * - Object-based vehicles using ITEM_VEHICLE type
 * - Simple drive commands in compass directions
 * - Vehicle-in-vehicle support (cars on ferries)
 * - Requires ROOM_VEHICLE flag for movement
 * 
 * GREYHAWK SHIP SYSTEM FEATURES:
 * - Advanced naval combat with detailed ship status
 * - Tactical map display with contact tracking
 * - Weapon systems with multiple firing arcs
 * - Detailed armor and internal damage modeling
 * - Crew management (sail crew, gunnery crew)
 * - Equipment slot system for weapons and upgrades
 * - Coordinate-based movement with heading/speed control
 * - Real-time tactical display with ASCII ship diagrams
 * 
 * GREYHAWK SETUP REQUIREMENTS:
 * 1. Create GREYHAWK_ITEM_SHIP objects (type 57) with values:
 *    value[0] = ship interior room vnum
 *    value[1] = ship index number
 * 
 * 2. Add ship structure to world data:
 *    - Rooms need world[room].ship pointer to greyhawk_ship_data
 *    - Ships need coordinate system integration
 * 
 * 3. Assign special procedures:
 *    - Ship objects: greyhawk_ship_object (for entering)
 *    - Control rooms: greyhawk_ship_commands (for ship commands)
 *    - Admin rooms: greyhawk_ship_loader (for ship creation)
 * 
 * 4. Initialize ships:
 *    - Call greyhawk_initialize_ships() during boot
 *    - Use greyhawk_loadship() to create new ships
 *    - Configure tactical map system
 * 
 * GREYHAWK PLAYER COMMANDS:
 * - "tactical [x] [y]" - Display tactical map
 * - "contacts" - Show detected ships
 * - "weaponspec" - Show weapon specifications
 * - "status" - Show detailed ship status with ASCII diagram
 * - "speed [value]" - Set ship speed
 * - "heading [degrees]" - Set ship heading (0-360)
 * - "disembark" - Leave ship
 * 
 * GREYHAWK ADMIN COMMANDS:
 * - "shipload <template>" - Create new ship from template
 * - "setsail <class>" - Configure ship sail system
 * - "mapload" - Initialize tactical map
 * 
 * NAMING CONFLICTS RESOLVED:
 * - Outcast functions: "outcast_" prefix
 * - CWG functions: no prefix (standard CircleMUD)
 * - Greyhawk functions: "greyhawk_" prefix
 * - Item types: ITEM_SHIP(56), GREYHAWK_ITEM_SHIP(57), ITEM_VEHICLE(53)
 * - All constants and macros appropriately prefixed
 * 
 * INTEGRATION NOTES:
 * - All systems disabled by #if 0 blocks for safety
 * - Compatible with existing Luminari codebase patterns
 * - Memory allocation uses appropriate Luminari patterns
 * - API calls updated for Luminari compatibility
 * - Follows Luminari naming conventions and K&R style
 * 
 * TO ENABLE ANY SYSTEM:
 * Remove the #if 0 and #endif lines from both vessels.c and vessels.h
 */

#endif /* _VESSELS_H_ */

#endif /* Disabled compilation block */