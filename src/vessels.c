/* ************************************************************************
 *      File:   vessels.c                            Part of LuminariMUD  *
 *   Purpose:   Unified Vessel/Vehicle system implementation              *
 *    Author:   Zusuk                                                     *
 * ********************************************************************** */

#include "conf.h"
#include "sysdep.h"
#include <math.h>
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
#include "vessels.h"
#include "mud_event.h"
#include "dg_scripts.h"
#include "wilderness.h"

/* ========================================================================= */
/* GREYHAWK SHIP SYSTEM IMPLEMENTATION                                      */
/* ========================================================================= */
/* Integrated from Greyhawk MUD - advanced naval combat and navigation     */

/* Global variables for Greyhawk ship system */
struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];
struct greyhawk_contact_data greyhawk_contacts[30];
struct greyhawk_ship_map greyhawk_tactical[151][151];

/* Global string buffers for Greyhawk system */
static char greyhawk_status[20];
static char greyhawk_position[20];
static char greyhawk_weapon[100];
/* These will be used when full implementation is added */
/* static char greyhawk_contact[256]; */
/* static char greyhawk_arc[3]; */
/* static char greyhawk_debug[256]; */
/* static char greyhawk_arg1[80]; */
/* static char greyhawk_arg2[80]; */

/* ========================================================================= */
/* VESSEL TYPE TERRAIN CAPABILITY DATA                                       */
/* ========================================================================= */
/* Static lookup table mapping vessel_class enum to terrain capabilities      */
/* Indexed by vessel_class enum values (VESSEL_RAFT=0 through VESSEL_MAGICAL=7) */

/* Number of vessel types (must match vessel_class enum count) */
#define NUM_VESSEL_TYPES 8

/**
 * Static terrain capability data for each vessel type.
 * This table provides O(1) lookup for vessel capabilities.
 *
 * Format: {can_ocean, can_shallow, can_air, can_underwater, min_depth, max_alt, speed_mods}
 * Speed modifiers indexed by sector type (0-39), values are percentages (100=normal).
 */
static const struct vessel_terrain_caps vessel_terrain_data[NUM_VESSEL_TYPES] = {
  /* VESSEL_RAFT (0): Small, rivers/shallow water only */
  {
    FALSE,  /* can_traverse_ocean */
    TRUE,   /* can_traverse_shallow */
    FALSE,  /* can_traverse_air */
    FALSE,  /* can_traverse_underwater */
    0,      /* min_water_depth */
    0,      /* max_altitude */
    {
      /* Speed modifiers by sector type (index 0-39) */
      /* SECT_INSIDE=0 */ 0,
      /* SECT_CITY=1 */ 0,
      /* SECT_FIELD=2 */ 0,
      /* SECT_FOREST=3 */ 0,
      /* SECT_HILLS=4 */ 0,
      /* SECT_MOUNTAIN=5 */ 0,
      /* SECT_WATER_SWIM=6 */ 100,  /* Full speed in shallow water */
      /* SECT_WATER_NOSWIM=7 */ 0,  /* Cannot navigate deep water */
      /* SECT_FLYING=8 */ 0,
      /* SECT_UNDERWATER=9 */ 0,
      /* SECT_ZONE_START=10 */ 0,
      /* SECT_ROAD_NS=11 */ 0,
      /* SECT_ROAD_EW=12 */ 0,
      /* SECT_ROAD_INT=13 */ 0,
      /* SECT_DESERT=14 */ 0,
      /* SECT_OCEAN=15 */ 0,  /* Cannot navigate ocean */
      /* SECT_MARSHLAND=16 */ 75,  /* Slow in marshes */
      /* SECT_HIGH_MOUNTAIN=17 */ 0,
      /* SECT_PLANES=18 */ 0,
      /* SECT_UD_WILD=19 */ 0,
      /* SECT_UD_CITY=20 */ 0,
      /* SECT_UD_INSIDE=21 */ 0,
      /* SECT_UD_WATER=22 */ 80,  /* Slow in UD water */
      /* SECT_UD_NOSWIM=23 */ 0,
      /* SECT_UD_NOGROUND=24 */ 0,
      /* SECT_LAVA=25 */ 0,
      /* SECT_D_ROAD_NS=26 */ 0,
      /* SECT_D_ROAD_EW=27 */ 0,
      /* SECT_D_ROAD_INT=28 */ 0,
      /* SECT_CAVE=29 */ 0,
      /* SECT_JUNGLE=30 */ 0,
      /* SECT_TUNDRA=31 */ 0,
      /* SECT_TAIGA=32 */ 0,
      /* SECT_BEACH=33 */ 50,  /* Very slow near beach */
      /* SECT_SEAPORT=34 */ 60,  /* Slow in port */
      /* SECT_INSIDE_ROOM=35 */ 0,
      /* SECT_RIVER=36 */ 100,  /* Full speed on rivers */
      /* unused */ 0, 0, 0
    }
  },

  /* VESSEL_BOAT (1): Medium, coastal waters */
  {
    FALSE,  /* can_traverse_ocean */
    TRUE,   /* can_traverse_shallow */
    FALSE,  /* can_traverse_air */
    FALSE,  /* can_traverse_underwater */
    0,      /* min_water_depth */
    0,      /* max_altitude */
    {
      /* SECT_INSIDE=0 */ 0,
      /* SECT_CITY=1 */ 0,
      /* SECT_FIELD=2 */ 0,
      /* SECT_FOREST=3 */ 0,
      /* SECT_HILLS=4 */ 0,
      /* SECT_MOUNTAIN=5 */ 0,
      /* SECT_WATER_SWIM=6 */ 100,  /* Full speed in shallow water */
      /* SECT_WATER_NOSWIM=7 */ 75, /* Reduced in deeper water */
      /* SECT_FLYING=8 */ 0,
      /* SECT_UNDERWATER=9 */ 0,
      /* SECT_ZONE_START=10 */ 0,
      /* SECT_ROAD_NS=11 */ 0,
      /* SECT_ROAD_EW=12 */ 0,
      /* SECT_ROAD_INT=13 */ 0,
      /* SECT_DESERT=14 */ 0,
      /* SECT_OCEAN=15 */ 0,  /* Cannot navigate ocean */
      /* SECT_MARSHLAND=16 */ 80,
      /* SECT_HIGH_MOUNTAIN=17 */ 0,
      /* SECT_PLANES=18 */ 0,
      /* SECT_UD_WILD=19 */ 0,
      /* SECT_UD_CITY=20 */ 0,
      /* SECT_UD_INSIDE=21 */ 0,
      /* SECT_UD_WATER=22 */ 90,
      /* SECT_UD_NOSWIM=23 */ 60,
      /* SECT_UD_NOGROUND=24 */ 0,
      /* SECT_LAVA=25 */ 0,
      /* SECT_D_ROAD_NS=26 */ 0,
      /* SECT_D_ROAD_EW=27 */ 0,
      /* SECT_D_ROAD_INT=28 */ 0,
      /* SECT_CAVE=29 */ 0,
      /* SECT_JUNGLE=30 */ 0,
      /* SECT_TUNDRA=31 */ 0,
      /* SECT_TAIGA=32 */ 0,
      /* SECT_BEACH=33 */ 60,
      /* SECT_SEAPORT=34 */ 70,
      /* SECT_INSIDE_ROOM=35 */ 0,
      /* SECT_RIVER=36 */ 100,
      /* unused */ 0, 0, 0
    }
  },

  /* VESSEL_SHIP (2): Large, ocean-capable */
  {
    TRUE,   /* can_traverse_ocean */
    TRUE,   /* can_traverse_shallow */
    FALSE,  /* can_traverse_air */
    FALSE,  /* can_traverse_underwater */
    2,      /* min_water_depth (needs deeper water) */
    0,      /* max_altitude */
    {
      /* SECT_INSIDE=0 */ 0,
      /* SECT_CITY=1 */ 0,
      /* SECT_FIELD=2 */ 0,
      /* SECT_FOREST=3 */ 0,
      /* SECT_HILLS=4 */ 0,
      /* SECT_MOUNTAIN=5 */ 0,
      /* SECT_WATER_SWIM=6 */ 75,   /* Reduced in shallow water */
      /* SECT_WATER_NOSWIM=7 */ 100, /* Full speed in deep water */
      /* SECT_FLYING=8 */ 0,
      /* SECT_UNDERWATER=9 */ 0,
      /* SECT_ZONE_START=10 */ 0,
      /* SECT_ROAD_NS=11 */ 0,
      /* SECT_ROAD_EW=12 */ 0,
      /* SECT_ROAD_INT=13 */ 0,
      /* SECT_DESERT=14 */ 0,
      /* SECT_OCEAN=15 */ 100, /* Full speed in ocean */
      /* SECT_MARSHLAND=16 */ 0, /* Cannot navigate marsh */
      /* SECT_HIGH_MOUNTAIN=17 */ 0,
      /* SECT_PLANES=18 */ 0,
      /* SECT_UD_WILD=19 */ 0,
      /* SECT_UD_CITY=20 */ 0,
      /* SECT_UD_INSIDE=21 */ 0,
      /* SECT_UD_WATER=22 */ 0,
      /* SECT_UD_NOSWIM=23 */ 80,
      /* SECT_UD_NOGROUND=24 */ 0,
      /* SECT_LAVA=25 */ 0,
      /* SECT_D_ROAD_NS=26 */ 0,
      /* SECT_D_ROAD_EW=27 */ 0,
      /* SECT_D_ROAD_INT=28 */ 0,
      /* SECT_CAVE=29 */ 0,
      /* SECT_JUNGLE=30 */ 0,
      /* SECT_TUNDRA=31 */ 0,
      /* SECT_TAIGA=32 */ 0,
      /* SECT_BEACH=33 */ 0, /* Too shallow */
      /* SECT_SEAPORT=34 */ 50, /* Slow in port */
      /* SECT_INSIDE_ROOM=35 */ 0,
      /* SECT_RIVER=36 */ 50, /* Too large for most rivers */
      /* unused */ 0, 0, 0
    }
  },

  /* VESSEL_WARSHIP (3): Combat vessel, heavily armed - same as SHIP */
  {
    TRUE,   /* can_traverse_ocean */
    TRUE,   /* can_traverse_shallow */
    FALSE,  /* can_traverse_air */
    FALSE,  /* can_traverse_underwater */
    2,      /* min_water_depth */
    0,      /* max_altitude */
    {
      /* SECT_INSIDE=0 */ 0,
      /* SECT_CITY=1 */ 0,
      /* SECT_FIELD=2 */ 0,
      /* SECT_FOREST=3 */ 0,
      /* SECT_HILLS=4 */ 0,
      /* SECT_MOUNTAIN=5 */ 0,
      /* SECT_WATER_SWIM=6 */ 75,
      /* SECT_WATER_NOSWIM=7 */ 100,
      /* SECT_FLYING=8 */ 0,
      /* SECT_UNDERWATER=9 */ 0,
      /* SECT_ZONE_START=10 */ 0,
      /* SECT_ROAD_NS=11 */ 0,
      /* SECT_ROAD_EW=12 */ 0,
      /* SECT_ROAD_INT=13 */ 0,
      /* SECT_DESERT=14 */ 0,
      /* SECT_OCEAN=15 */ 100,
      /* SECT_MARSHLAND=16 */ 0,
      /* SECT_HIGH_MOUNTAIN=17 */ 0,
      /* SECT_PLANES=18 */ 0,
      /* SECT_UD_WILD=19 */ 0,
      /* SECT_UD_CITY=20 */ 0,
      /* SECT_UD_INSIDE=21 */ 0,
      /* SECT_UD_WATER=22 */ 0,
      /* SECT_UD_NOSWIM=23 */ 80,
      /* SECT_UD_NOGROUND=24 */ 0,
      /* SECT_LAVA=25 */ 0,
      /* SECT_D_ROAD_NS=26 */ 0,
      /* SECT_D_ROAD_EW=27 */ 0,
      /* SECT_D_ROAD_INT=28 */ 0,
      /* SECT_CAVE=29 */ 0,
      /* SECT_JUNGLE=30 */ 0,
      /* SECT_TUNDRA=31 */ 0,
      /* SECT_TAIGA=32 */ 0,
      /* SECT_BEACH=33 */ 0,
      /* SECT_SEAPORT=34 */ 50,
      /* SECT_INSIDE_ROOM=35 */ 0,
      /* SECT_RIVER=36 */ 50,
      /* unused */ 0, 0, 0
    }
  },

  /* VESSEL_AIRSHIP (4): Flying vessel, ignores terrain when airborne */
  {
    TRUE,   /* can_traverse_ocean (when landed/low) */
    TRUE,   /* can_traverse_shallow */
    TRUE,   /* can_traverse_air */
    FALSE,  /* can_traverse_underwater */
    0,      /* min_water_depth */
    500,    /* max_altitude */
    {
      /* At altitude, airships have 100 speed over all terrain except lava */
      /* SECT_INSIDE=0 */ 0,
      /* SECT_CITY=1 */ 80,
      /* SECT_FIELD=2 */ 100,
      /* SECT_FOREST=3 */ 100,
      /* SECT_HILLS=4 */ 100,
      /* SECT_MOUNTAIN=5 */ 100, /* Can fly over mountains at altitude */
      /* SECT_WATER_SWIM=6 */ 100,
      /* SECT_WATER_NOSWIM=7 */ 100,
      /* SECT_FLYING=8 */ 100,
      /* SECT_UNDERWATER=9 */ 0,
      /* SECT_ZONE_START=10 */ 0,
      /* SECT_ROAD_NS=11 */ 100,
      /* SECT_ROAD_EW=12 */ 100,
      /* SECT_ROAD_INT=13 */ 100,
      /* SECT_DESERT=14 */ 100,
      /* SECT_OCEAN=15 */ 100,
      /* SECT_MARSHLAND=16 */ 100,
      /* SECT_HIGH_MOUNTAIN=17 */ 100,
      /* SECT_PLANES=18 */ 100,
      /* SECT_UD_WILD=19 */ 0, /* Cannot fly underground */
      /* SECT_UD_CITY=20 */ 0,
      /* SECT_UD_INSIDE=21 */ 0,
      /* SECT_UD_WATER=22 */ 0,
      /* SECT_UD_NOSWIM=23 */ 0,
      /* SECT_UD_NOGROUND=24 */ 0,
      /* SECT_LAVA=25 */ 80, /* Heat updrafts */
      /* SECT_D_ROAD_NS=26 */ 100,
      /* SECT_D_ROAD_EW=27 */ 100,
      /* SECT_D_ROAD_INT=28 */ 100,
      /* SECT_CAVE=29 */ 0,
      /* SECT_JUNGLE=30 */ 100,
      /* SECT_TUNDRA=31 */ 100,
      /* SECT_TAIGA=32 */ 100,
      /* SECT_BEACH=33 */ 100,
      /* SECT_SEAPORT=34 */ 100,
      /* SECT_INSIDE_ROOM=35 */ 0,
      /* SECT_RIVER=36 */ 100,
      /* unused */ 0, 0, 0
    }
  },

  /* VESSEL_SUBMARINE (5): Underwater vessel, depth navigation */
  {
    TRUE,   /* can_traverse_ocean */
    TRUE,   /* can_traverse_shallow */
    FALSE,  /* can_traverse_air */
    TRUE,   /* can_traverse_underwater */
    0,      /* min_water_depth */
    0,      /* max_altitude (negative z for depth) */
    {
      /* SECT_INSIDE=0 */ 0,
      /* SECT_CITY=1 */ 0,
      /* SECT_FIELD=2 */ 0,
      /* SECT_FOREST=3 */ 0,
      /* SECT_HILLS=4 */ 0,
      /* SECT_MOUNTAIN=5 */ 0,
      /* SECT_WATER_SWIM=6 */ 50,  /* Slow in shallow water */
      /* SECT_WATER_NOSWIM=7 */ 90,
      /* SECT_FLYING=8 */ 0,
      /* SECT_UNDERWATER=9 */ 100, /* Full speed underwater */
      /* SECT_ZONE_START=10 */ 0,
      /* SECT_ROAD_NS=11 */ 0,
      /* SECT_ROAD_EW=12 */ 0,
      /* SECT_ROAD_INT=13 */ 0,
      /* SECT_DESERT=14 */ 0,
      /* SECT_OCEAN=15 */ 100, /* Full speed in ocean */
      /* SECT_MARSHLAND=16 */ 0,
      /* SECT_HIGH_MOUNTAIN=17 */ 0,
      /* SECT_PLANES=18 */ 0,
      /* SECT_UD_WILD=19 */ 0,
      /* SECT_UD_CITY=20 */ 0,
      /* SECT_UD_INSIDE=21 */ 0,
      /* SECT_UD_WATER=22 */ 90,
      /* SECT_UD_NOSWIM=23 */ 100, /* Good in UD deep water */
      /* SECT_UD_NOGROUND=24 */ 0,
      /* SECT_LAVA=25 */ 0,
      /* SECT_D_ROAD_NS=26 */ 0,
      /* SECT_D_ROAD_EW=27 */ 0,
      /* SECT_D_ROAD_INT=28 */ 0,
      /* SECT_CAVE=29 */ 0,
      /* SECT_JUNGLE=30 */ 0,
      /* SECT_TUNDRA=31 */ 0,
      /* SECT_TAIGA=32 */ 0,
      /* SECT_BEACH=33 */ 0,
      /* SECT_SEAPORT=34 */ 40, /* Very slow at surface in port */
      /* SECT_INSIDE_ROOM=35 */ 0,
      /* SECT_RIVER=36 */ 0, /* Too large for rivers */
      /* unused */ 0, 0, 0
    }
  },

  /* VESSEL_TRANSPORT (6): Cargo/passenger vessel - similar to SHIP */
  {
    TRUE,   /* can_traverse_ocean */
    TRUE,   /* can_traverse_shallow */
    FALSE,  /* can_traverse_air */
    FALSE,  /* can_traverse_underwater */
    2,      /* min_water_depth */
    0,      /* max_altitude */
    {
      /* SECT_INSIDE=0 */ 0,
      /* SECT_CITY=1 */ 0,
      /* SECT_FIELD=2 */ 0,
      /* SECT_FOREST=3 */ 0,
      /* SECT_HILLS=4 */ 0,
      /* SECT_MOUNTAIN=5 */ 0,
      /* SECT_WATER_SWIM=6 */ 60,  /* Slow in shallow - heavy */
      /* SECT_WATER_NOSWIM=7 */ 90, /* Good in deep water */
      /* SECT_FLYING=8 */ 0,
      /* SECT_UNDERWATER=9 */ 0,
      /* SECT_ZONE_START=10 */ 0,
      /* SECT_ROAD_NS=11 */ 0,
      /* SECT_ROAD_EW=12 */ 0,
      /* SECT_ROAD_INT=13 */ 0,
      /* SECT_DESERT=14 */ 0,
      /* SECT_OCEAN=15 */ 100, /* Full speed in ocean */
      /* SECT_MARSHLAND=16 */ 0,
      /* SECT_HIGH_MOUNTAIN=17 */ 0,
      /* SECT_PLANES=18 */ 0,
      /* SECT_UD_WILD=19 */ 0,
      /* SECT_UD_CITY=20 */ 0,
      /* SECT_UD_INSIDE=21 */ 0,
      /* SECT_UD_WATER=22 */ 0,
      /* SECT_UD_NOSWIM=23 */ 70,
      /* SECT_UD_NOGROUND=24 */ 0,
      /* SECT_LAVA=25 */ 0,
      /* SECT_D_ROAD_NS=26 */ 0,
      /* SECT_D_ROAD_EW=27 */ 0,
      /* SECT_D_ROAD_INT=28 */ 0,
      /* SECT_CAVE=29 */ 0,
      /* SECT_JUNGLE=30 */ 0,
      /* SECT_TUNDRA=31 */ 0,
      /* SECT_TAIGA=32 */ 0,
      /* SECT_BEACH=33 */ 0,
      /* SECT_SEAPORT=34 */ 60,
      /* SECT_INSIDE_ROOM=35 */ 0,
      /* SECT_RIVER=36 */ 40, /* Very slow on rivers */
      /* unused */ 0, 0, 0
    }
  },

  /* VESSEL_MAGICAL (7): Special magical vessels - most capable */
  {
    TRUE,   /* can_traverse_ocean */
    TRUE,   /* can_traverse_shallow */
    TRUE,   /* can_traverse_air */
    TRUE,   /* can_traverse_underwater */
    0,      /* min_water_depth */
    1000,   /* max_altitude */
    {
      /* SECT_INSIDE=0 */ 0,
      /* SECT_CITY=1 */ 80,
      /* SECT_FIELD=2 */ 100,
      /* SECT_FOREST=3 */ 100,
      /* SECT_HILLS=4 */ 100,
      /* SECT_MOUNTAIN=5 */ 100,
      /* SECT_WATER_SWIM=6 */ 100,
      /* SECT_WATER_NOSWIM=7 */ 100,
      /* SECT_FLYING=8 */ 100,
      /* SECT_UNDERWATER=9 */ 100,
      /* SECT_ZONE_START=10 */ 0,
      /* SECT_ROAD_NS=11 */ 100,
      /* SECT_ROAD_EW=12 */ 100,
      /* SECT_ROAD_INT=13 */ 100,
      /* SECT_DESERT=14 */ 100,
      /* SECT_OCEAN=15 */ 100,
      /* SECT_MARSHLAND=16 */ 100,
      /* SECT_HIGH_MOUNTAIN=17 */ 100,
      /* SECT_PLANES=18 */ 100, /* Can traverse planes */
      /* SECT_UD_WILD=19 */ 80,
      /* SECT_UD_CITY=20 */ 80,
      /* SECT_UD_INSIDE=21 */ 80,
      /* SECT_UD_WATER=22 */ 100,
      /* SECT_UD_NOSWIM=23 */ 100,
      /* SECT_UD_NOGROUND=24 */ 100,
      /* SECT_LAVA=25 */ 0, /* Even magic won't survive lava */
      /* SECT_D_ROAD_NS=26 */ 100,
      /* SECT_D_ROAD_EW=27 */ 100,
      /* SECT_D_ROAD_INT=28 */ 100,
      /* SECT_CAVE=29 */ 80,
      /* SECT_JUNGLE=30 */ 100,
      /* SECT_TUNDRA=31 */ 100,
      /* SECT_TAIGA=32 */ 100,
      /* SECT_BEACH=33 */ 100,
      /* SECT_SEAPORT=34 */ 100,
      /* SECT_INSIDE_ROOM=35 */ 0,
      /* SECT_RIVER=36 */ 100,
      /* unused */ 0, 0, 0
    }
  }
};

/* ========================================================================= */
/* VESSEL TYPE ACCESSOR FUNCTIONS                                            */
/* ========================================================================= */

/**
 * Get terrain capabilities for a vessel type.
 *
 * Returns a const pointer to the terrain capability structure for the
 * specified vessel type. Provides O(1) lookup from the static data table.
 *
 * @param vessel_type The vessel_class enum value (VESSEL_RAFT through VESSEL_MAGICAL)
 * @return Pointer to const vessel_terrain_caps structure, or NULL if invalid type
 */
const struct vessel_terrain_caps *get_vessel_terrain_caps(enum vessel_class vessel_type)
{
  /* Bounds check - default to VESSEL_SHIP for invalid types */
  if (vessel_type < 0 || vessel_type >= NUM_VESSEL_TYPES)
  {
    log("SYSERR: get_vessel_terrain_caps: Invalid vessel type %d, defaulting to VESSEL_SHIP", vessel_type);
    return &vessel_terrain_data[VESSEL_SHIP];
  }

  return &vessel_terrain_data[vessel_type];
}

/**
 * Get vessel type from ship data.
 *
 * Extracts the vessel_type field from a greyhawk_ship_data structure
 * with proper bounds checking. Returns VESSEL_SHIP as default for
 * invalid or uninitialized values.
 *
 * @param shipnum The ship index number in greyhawk_ships array
 * @return The vessel_class enum value for this ship
 */
enum vessel_class get_vessel_type_from_ship(int shipnum)
{
  enum vessel_class vtype;

  /* Validate ship number */
  if (shipnum < 0 || shipnum >= GREYHAWK_MAXSHIPS)
  {
    log("SYSERR: get_vessel_type_from_ship: Invalid ship number %d", shipnum);
    return VESSEL_SHIP;  /* Default to standard ship */
  }

  vtype = greyhawk_ships[shipnum].vessel_type;

  /* Bounds check the vessel type - default to VESSEL_SHIP for invalid values */
  if (vtype < 0 || vtype >= NUM_VESSEL_TYPES)
  {
    /* Uninitialized or invalid vessel type */
    return VESSEL_SHIP;
  }

  return vtype;
}

/**
 * Get vessel type name for display.
 *
 * Returns a human-readable string for the vessel type.
 *
 * @param vessel_type The vessel_class enum value
 * @return Static string with vessel type name
 */
const char *get_vessel_type_name(enum vessel_class vessel_type)
{
  static const char *vessel_names[NUM_VESSEL_TYPES] = {
    "Raft",
    "Boat",
    "Ship",
    "Warship",
    "Airship",
    "Submarine",
    "Transport",
    "Magical Vessel"
  };

  if (vessel_type < 0 || vessel_type >= NUM_VESSEL_TYPES)
  {
    return "Unknown Vessel";
  }

  return vessel_names[vessel_type];
}

/* Forward declarations for Greyhawk functions */
void greyhawk_getstatus(int slot, int rnum);
void greyhawk_getposition(int slot, int rnum);
void greyhawk_dispweapon(int slot, int rnum);
int greyhawk_weaprange(int shipnum, int slot, char range);
int greyhawk_bearing(float x1, float y1, float x2, float y2);
float greyhawk_range(float x1, float y1, float z1, float x2, float y2, float z2);
void greyhawk_dispcontact(int i);
int greyhawk_getcontacts(int shipnum);
void greyhawk_setcontact(int i, struct obj_data *obj, int shipnum, int xoffset, int yoffset);
int greyhawk_getarc(int ship1, int ship2);
void greyhawk_setsymbol(int x, int y, int symbol);
void greyhawk_getmap(int shipnum);
int greyhawk_loadship(int template, int to_room, short int x_cord, short int y_cord, short int z_cord);
void greyhawk_nameship(char *name, int shipnum);
bool greyhawk_setsail(int class, int shipnum);
void greyhawk_initialize_ships(void);

/* ========================================================================= */
/* WILDERNESS ROOM ALLOCATION HELPER                                        */
/* ========================================================================= */

/**
 * Get or allocate a wilderness room at the given coordinates.
 *
 * This function implements the dynamic room allocation pattern from movement.c.
 * It first checks if a room already exists at the coordinates. If not, it
 * allocates a room from the dynamic wilderness pool and configures it.
 *
 * @param x Wilderness X coordinate (-1024 to +1024)
 * @param y Wilderness Y coordinate (-1024 to +1024)
 * @return Room number (rnum) if successful, NOWHERE if allocation fails
 */
static room_rnum get_or_allocate_wilderness_room(int x, int y)
{
  room_rnum room;

  /* Validate coordinates are within wilderness bounds */
  if (x < -1024 || x > 1024 || y < -1024 || y > 1024)
  {
    log("SYSERR: get_or_allocate_wilderness_room: Coordinates out of bounds (%d, %d)", x, y);
    return NOWHERE;
  }

  /* Try to find existing room at coordinates */
  room = find_room_by_coordinates(x, y);

  if (room == NOWHERE)
  {
    /* No room exists, allocate from dynamic pool */
    room = find_available_wilderness_room();
    if (room == NOWHERE)
    {
      log("SYSERR: get_or_allocate_wilderness_room: Room pool exhausted at (%d, %d)", x, y);
      return NOWHERE;
    }

    /* Configure the room for these coordinates */
    assign_wilderness_room(room, x, y);
  }

  return room;
}

/* ========================================================================= */
/* GREYHAWK SHIP UTILITY FUNCTIONS                                         */
/* ========================================================================= */

/**
 * Get weapon status string for display
 * @param slot Weapon slot number
 * @param rnum Room number containing ship
 */
void greyhawk_getstatus(int slot, int rnum) {
  if (world[rnum].ship->slot[slot].timer > 0)
    sprintf(greyhawk_status, "&+R%-6d", world[rnum].ship->slot[slot].timer);
  else if (world[rnum].ship->slot[slot].timer == 0)
    strcpy(greyhawk_status, "Ready");
  else if (world[rnum].ship->slot[slot].timer < 0)
    strcpy(greyhawk_status, "&+L***   ");
  
  if (world[rnum].ship->slot[slot].desc[0] == '\0')
    strcpy(greyhawk_status, "");
}

/**
 * Get weapon position string for display
 * @param slot Weapon slot number  
 * @param rnum Room number containing ship
 */
void greyhawk_getposition(int slot, int rnum) {
  switch (world[rnum].ship->slot[slot].position) {
    case GREYHAWK_FORE:
      strcpy(greyhawk_position, "Forward");
      break;
    case GREYHAWK_REAR:
      strcpy(greyhawk_position, "Rear");
      break;
    case GREYHAWK_PORT:
      strcpy(greyhawk_position, "Port");
      break;
    case GREYHAWK_STARBOARD:
      strcpy(greyhawk_position, "Starboard");
      break;
    default:
      strcpy(greyhawk_position, "ERROR");
      break;
  }
  
  if (world[rnum].ship->slot[slot].desc[0] == '\0')
    strcpy(greyhawk_position, "");
}

/**
 * Format weapon display string
 * @param slot Weapon slot number
 * @param rnum Room number containing ship
 */
void greyhawk_dispweapon(int slot, int rnum) {
  if (world[rnum].ship->slot[slot].type != 1) {
    strcpy(greyhawk_weapon, " ");
  } else {
    greyhawk_getstatus(slot, rnum);
    greyhawk_getposition(slot, rnum);
    sprintf(greyhawk_weapon, "%-20s &N%-6s  &+W%-9s  %d",
            world[rnum].ship->slot[slot].desc,
            greyhawk_status,
            greyhawk_position,
            world[rnum].ship->slot[slot].val3);
  }
}

/**
 * Calculate weapon range based on type
 * @param shipnum Ship index
 * @param slot Weapon slot
 * @param range Range type (SHORT/MED/LONG)
 * @return Calculated range value
 */
int greyhawk_weaprange(int shipnum, int slot, char range) {
  if (greyhawk_ships[shipnum].slot[slot].type != 1)
    return 0;
    
  switch (range) {
    case GREYHAWK_SHRTRANGE:
      return (int)((float)(greyhawk_ships[shipnum].slot[slot].val0 -
                          greyhawk_ships[shipnum].slot[slot].val1) / 3 +
                  greyhawk_ships[shipnum].slot[slot].val1);
    case GREYHAWK_MEDRANGE:
      return (int)((float)((greyhawk_ships[shipnum].slot[slot].val0 -
                           greyhawk_ships[shipnum].slot[slot].val1) / 3) * 2 +
                  greyhawk_ships[shipnum].slot[slot].val1);
    case GREYHAWK_LNGRANGE:
      return greyhawk_ships[shipnum].slot[slot].val0;
    default:
      return 0;
  }
}

/**
 * Calculate bearing between two points
 * @param x1 Source X coordinate
 * @param y1 Source Y coordinate
 * @param x2 Target X coordinate
 * @param y2 Target Y coordinate
 * @return Bearing in degrees (0-360)
 */
int greyhawk_bearing(float x1, float y1, float x2, float y2) {
  int val;
  
  if (y1 == y2) {
    if (x1 > x2)
      return 270;
    return 90;
  }
  
  if (x1 == x2) {
    if (y1 > y2)
      return 180;
    else
      return 0;
  }
  
  val = atan((x2 - x1) / (y2 - y1)) * 180 / M_PI;
  
  if (y1 < y2) {
    if (val >= 0)
      return val;
    return (val + 360);
  } else {
    return val + 180;
  }
}

/**
 * Calculate 3D range between two points
 * @param x1 Source X coordinate
 * @param y1 Source Y coordinate
 * @param z1 Source Z coordinate
 * @param x2 Target X coordinate
 * @param y2 Target Y coordinate
 * @param z2 Target Z coordinate
 * @return 3D distance
 */
float greyhawk_range(float x1, float y1, float z1, float x2, float y2, float z2) {
  float dx = x2 - x1;
  float dy = y2 - y1;
  float dz = z2 - z1;
  
  return sqrt((dx * dx) + (dy * dy) + (dz * dz));
}

/* Placeholder for remaining Greyhawk functions - to be implemented */
/* The full implementation will be added in the next step */

/**
 * Initialize Greyhawk ship system
 * Call this during game boot sequence
 */
void greyhawk_initialize_ships(void) {
  int i, j;
  
  /* Clear ship array */
  memset(greyhawk_ships, 0, sizeof(greyhawk_ships));
  
  /* Clear contact array */
  memset(greyhawk_contacts, 0, sizeof(greyhawk_contacts));
  
  /* Initialize tactical map with default ocean pattern */
  for (i = 0; i < 151; i++) {
    for (j = 0; j < 151; j++) {
      strcpy(greyhawk_tactical[i][j].map, "  ");
    }
  }
  
  log("Greyhawk ship system initialized.");
}

/* ========================================================================= */
/* WILDERNESS INTEGRATION FUNCTIONS                                         */
/* ========================================================================= */
/* These functions integrate vessels with the wilderness coordinate system   */

/**
 * Update ship position in wilderness coordinates
 * Integrates vessel movement with the wilderness coordinate system
 * @param shipnum Ship index number
 * @param new_x New wilderness X coordinate (-1024 to +1024)
 * @param new_y New wilderness Y coordinate (-1024 to +1024)
 * @param new_z New elevation/depth for airships/submarines
 * @return TRUE if position update successful, FALSE otherwise
 */
bool update_ship_wilderness_position(int shipnum, int new_x, int new_y, int new_z)
{
  room_rnum wilderness_room;

  /* Validate ship number */
  if (shipnum < 0 || shipnum >= GREYHAWK_MAXSHIPS)
  {
    log("SYSERR: update_ship_wilderness_position: Invalid ship number %d", shipnum);
    return FALSE;
  }

  /* Validate coordinates within wilderness bounds */
  if (new_x < -1024 || new_x > 1024 || new_y < -1024 || new_y > 1024)
  {
    log("SYSERR: update_ship_wilderness_position: Coordinates out of bounds (%d, %d)", new_x, new_y);
    return FALSE;
  }

  /* Update ship coordinates */
  greyhawk_ships[shipnum].x = (float)new_x;
  greyhawk_ships[shipnum].y = (float)new_y;
  greyhawk_ships[shipnum].z = (float)new_z;

  /* Get or allocate wilderness room at these coordinates using dynamic allocation */
  wilderness_room = get_or_allocate_wilderness_room(new_x, new_y);
  if (wilderness_room == NOWHERE)
  {
    log("SYSERR: update_ship_wilderness_position: Room pool exhausted or invalid coordinates (%d, %d)", new_x, new_y);
    return FALSE;
  }

  /* Update ship's location to the wilderness room */
  greyhawk_ships[shipnum].location = world[wilderness_room].number;

  /* If ship object exists, move it to new location */
  if (greyhawk_ships[shipnum].shipobj)
  {
    obj_from_room(greyhawk_ships[shipnum].shipobj);
    obj_to_room(greyhawk_ships[shipnum].shipobj, wilderness_room);
  }

  return TRUE;
}

/**
 * Get terrain type at ship's current position
 * Used to determine movement restrictions and speed modifiers
 * @param shipnum Ship index number
 * @return Sector type at ship's coordinates
 */
int get_ship_terrain_type(int shipnum)
{
  room_rnum wilderness_room;
  int x;
  int y;

  /* Validate ship number */
  if (shipnum < 0 || shipnum >= GREYHAWK_MAXSHIPS)
  {
    return SECT_INSIDE;  /* Default safe sector */
  }

  /* Get ship coordinates */
  x = (int)greyhawk_ships[shipnum].x;
  y = (int)greyhawk_ships[shipnum].y;

  /* Get or allocate wilderness room at coordinates using dynamic allocation */
  wilderness_room = get_or_allocate_wilderness_room(x, y);
  if (wilderness_room == NOWHERE)
  {
    log("SYSERR: get_ship_terrain_type: Room pool exhausted at (%d, %d)", x, y);
    return SECT_INSIDE;  /* Default safe sector */
  }

  /* Return the sector type of the wilderness room */
  return world[wilderness_room].sector_type;
}

/**
 * Check if vessel can traverse terrain at given coordinates.
 *
 * Uses the static terrain capability data table for O(1) lookup.
 * Checks vessel_class capabilities against the sector type at the
 * target coordinates.
 *
 * @param vessel_type The vessel_class enum value (VESSEL_RAFT through VESSEL_MAGICAL)
 * @param x Target X coordinate
 * @param y Target Y coordinate
 * @param z Target Z coordinate (elevation/depth)
 * @return TRUE if vessel can enter terrain, FALSE otherwise
 */
bool can_vessel_traverse_terrain(enum vessel_class vessel_type, int x, int y, int z)
{
  room_rnum wilderness_room;
  int sector_type;
  const struct vessel_terrain_caps *caps;

  /* Validate coordinates within wilderness bounds first */
  if (x < -1024 || x > 1024 || y < -1024 || y > 1024)
  {
    return FALSE;
  }

  /* Get or allocate wilderness room at coordinates using dynamic allocation */
  wilderness_room = get_or_allocate_wilderness_room(x, y);
  if (wilderness_room == NOWHERE)
  {
    log("SYSERR: can_vessel_traverse_terrain: Room pool exhausted at (%d, %d)", x, y);
    return FALSE;
  }

  sector_type = world[wilderness_room].sector_type;

  /* Get terrain capabilities for this vessel type */
  caps = get_vessel_terrain_caps(vessel_type);
  if (caps == NULL)
  {
    return FALSE;
  }

  /* Airships at altitude can fly over most terrain */
  if (vessel_type == VESSEL_AIRSHIP && z > 100)
  {
    /* Flying altitude - check max altitude and underground restrictions */
    if (z > caps->max_altitude)
    {
      return FALSE;  /* Too high */
    }
    /* Cannot fly underground */
    if (sector_type >= SECT_UD_WILD && sector_type <= SECT_UD_NOGROUND)
    {
      return FALSE;
    }
    if (sector_type == SECT_CAVE || sector_type == SECT_INSIDE ||
        sector_type == SECT_INSIDE_ROOM)
    {
      return FALSE;
    }
    return TRUE;  /* Can fly over everything else at altitude */
  }

  /* Submarines must be submerged for underwater, surfaced otherwise */
  if (vessel_type == VESSEL_SUBMARINE)
  {
    if (sector_type == SECT_UNDERWATER && z >= 0)
    {
      return FALSE;  /* Must dive (negative z) for underwater */
    }
    if (sector_type != SECT_UNDERWATER && z < 0)
    {
      /* Submerged but not in underwater terrain - can travel in ocean/deep water */
      if (sector_type != SECT_OCEAN && sector_type != SECT_WATER_NOSWIM)
      {
        return FALSE;
      }
    }
  }

  /* Check speed modifier - 0 means impassable */
  if (sector_type >= 0 && sector_type < 40)
  {
    if (caps->terrain_speed_mod[sector_type] == 0)
    {
      return FALSE;
    }
    return TRUE;
  }

  /* Default fallback for unknown sector types */
  return FALSE;
}

/**
 * Calculate terrain-based speed modifier for vessel.
 *
 * Uses the static terrain capability data table for O(1) lookup.
 * Returns the speed modifier as a percentage (100 = normal speed).
 * Weather conditions apply additional penalties.
 *
 * @param vessel_type The vessel_class enum value (VESSEL_RAFT through VESSEL_MAGICAL)
 * @param sector_type Terrain sector type
 * @param weather_conditions Current weather (0=clear, higher=worse)
 * @return Speed modifier as percentage (100 = normal speed, 0 = impassable)
 */
int get_terrain_speed_modifier(enum vessel_class vessel_type, int sector_type, int weather_conditions)
{
  int base_modifier;
  const struct vessel_terrain_caps *caps;

  /* Get terrain capabilities for this vessel type */
  caps = get_vessel_terrain_caps(vessel_type);
  if (caps == NULL)
  {
    return 0;  /* Invalid vessel type */
  }

  /* Validate sector type and get base modifier from table */
  if (sector_type < 0 || sector_type >= 40)
  {
    return 0;  /* Invalid sector type */
  }

  base_modifier = (int)caps->terrain_speed_mod[sector_type];

  /* Airships are more affected by weather */
  if (vessel_type == VESSEL_AIRSHIP && weather_conditions > 0)
  {
    base_modifier -= (weather_conditions * 10);
  }

  /* Apply weather penalties (except for submarines underwater) */
  if (vessel_type != VESSEL_SUBMARINE || sector_type != SECT_UNDERWATER)
  {
    if (weather_conditions > 0)
    {
      base_modifier -= (weather_conditions * 5);
    }
  }

  /* Ensure modifier doesn't go below 0 or above 150 */
  base_modifier = MAX(0, MIN(150, base_modifier));

  return base_modifier;
}

/**
 * Move ship in given direction using wilderness coordinates
 * @param shipnum Ship index number
 * @param direction Direction to move (NORTH, SOUTH, EAST, WEST, etc.)
 * @param ch Character piloting the ship (for messages)
 * @return TRUE if movement successful, FALSE otherwise
 */
bool move_ship_wilderness(int shipnum, int direction, struct char_data *ch) {
  int new_x, new_y, new_z;
  int speed_modifier;
  int terrain_type;
  int weather_conditions;
  enum vessel_class vessel_type;

  /* Validate ship number */
  if (shipnum < 0 || shipnum >= GREYHAWK_MAXSHIPS) {
    return FALSE;
  }

  /* Get actual vessel type from ship data */
  vessel_type = get_vessel_type_from_ship(shipnum);
  
  /* Get current position */
  new_x = (int)greyhawk_ships[shipnum].x;
  new_y = (int)greyhawk_ships[shipnum].y;
  new_z = (int)greyhawk_ships[shipnum].z;
  
  /* Get weather conditions at current position */
  weather_conditions = get_weather(new_x, new_y);
  
  /* Calculate new position based on direction and speed */
  int move_distance = MAX(1, greyhawk_ships[shipnum].speed / 10);
  
  /* Weather affects movement distance */
  if (weather_conditions > 50) {  /* Stormy weather */
    move_distance = move_distance * 75 / 100;  /* 25% reduction */
    if (ch) {
      send_to_char(ch, "The harsh weather conditions slow your progress!\r\n");
    }
  }
  
  switch (direction) {
    case NORTH:
      new_y += move_distance;
      break;
    case SOUTH:
      new_y -= move_distance;
      break;
    case EAST:
      new_x += move_distance;
      break;
    case WEST:
      new_x -= move_distance;
      break;
    case NORTHEAST:
      new_x += move_distance;
      new_y += move_distance;
      break;
    case NORTHWEST:
      new_x -= move_distance;
      new_y += move_distance;
      break;
    case SOUTHEAST:
      new_x += move_distance;
      new_y -= move_distance;
      break;
    case SOUTHWEST:
      new_x -= move_distance;
      new_y -= move_distance;
      break;
    case UP:  /* For airships/submarines */
      new_z += 10;
      break;
    case DOWN:  /* For airships/submarines */
      new_z -= 10;
      break;
    default:
      return FALSE;
  }
  
  /* Check if vessel can traverse the target terrain */
  if (!can_vessel_traverse_terrain(vessel_type, new_x, new_y, new_z)) {
    if (ch) {
      /* Send vessel-type-specific denial message */
      switch (vessel_type)
      {
        case VESSEL_RAFT:
          send_to_char(ch, "Your raft cannot navigate these waters! It's only suitable for rivers and shallow water.\r\n");
          break;
        case VESSEL_BOAT:
          send_to_char(ch, "Your boat cannot handle these conditions! It's designed for coastal waters only.\r\n");
          break;
        case VESSEL_SHIP:
        case VESSEL_WARSHIP:
          send_to_char(ch, "The ship cannot navigate this terrain! It requires deep water to sail.\r\n");
          break;
        case VESSEL_AIRSHIP:
          if (new_z < 100)
          {
            send_to_char(ch, "The airship cannot fly through this terrain at low altitude! Gain more altitude.\r\n");
          }
          else
          {
            send_to_char(ch, "The airship cannot fly here - perhaps it's underground or the altitude is too extreme.\r\n");
          }
          break;
        case VESSEL_SUBMARINE:
          if (new_z >= 0)
          {
            send_to_char(ch, "The submarine must dive to navigate underwater terrain! Use 'heading down' to submerge.\r\n");
          }
          else
          {
            send_to_char(ch, "The submarine cannot traverse this area while submerged!\r\n");
          }
          break;
        case VESSEL_TRANSPORT:
          send_to_char(ch, "The transport vessel draws too much water for this area!\r\n");
          break;
        case VESSEL_MAGICAL:
          send_to_char(ch, "Even magical forces cannot penetrate this barrier!\r\n");
          break;
        default:
          send_to_char(ch, "The vessel cannot navigate that terrain!\r\n");
          break;
      }
    }
    return FALSE;
  }
  
  /* Update ship position */
  if (!update_ship_wilderness_position(shipnum, new_x, new_y, new_z)) {
    if (ch) {
      send_to_char(ch, "Movement failed - unable to update position.\r\n");
    }
    return FALSE;
  }
  
  /* Get terrain at new position and calculate speed modifier including weather */
  terrain_type = get_ship_terrain_type(shipnum);
  speed_modifier = get_terrain_speed_modifier(vessel_type, terrain_type, weather_conditions / 25);
  
  /* Adjust ship speed based on terrain and weather */
  greyhawk_ships[shipnum].speed = (greyhawk_ships[shipnum].setspeed * speed_modifier) / 100;
  
  /* Send movement messages */
  if (ch) {
    send_to_char(ch, "The vessel moves %s across the wilderness.\r\n", dirs[direction]);
    send_to_char(ch, "Current position: (%d, %d, %d)\r\n", new_x, new_y, new_z);
    if (speed_modifier != 100) {
      send_to_char(ch, "Speed affected by terrain and weather: %d%%\r\n", speed_modifier);
    }
    
    /* Weather-specific messages */
    if (weather_conditions > 75) {
      send_to_char(ch, "The vessel struggles against the severe storm!\r\n");
      act("The ship rocks violently in the storm!", FALSE, ch, 0, 0, TO_ROOM);
    } else if (weather_conditions > 50) {
      send_to_char(ch, "Strong winds and rain buffet the vessel.\r\n");
      act("The ship sways in the rough weather.", FALSE, ch, 0, 0, TO_ROOM);
    } else if (weather_conditions > 25) {
      send_to_char(ch, "Light rain patters against the deck.\r\n");
    } else {
      send_to_char(ch, "The weather is clear for sailing.\r\n");
    }
  }
  
  return TRUE;
}

/* ========================================================================= */
/* COMMAND FUNCTIONS                                                        */
/* ========================================================================= */

/* Board command - handled by special procedure on ship objects */
ACMD(do_board_vessel) {
  send_to_char(ch, "You need to be near a ship to board it.\r\n");
  /* The actual boarding is handled by the greyhawk_ship_object special procedure */
  /* This command exists just so 'board' is recognized as a valid command */
}

/* These will be implemented after the core system is working */

ACMD(do_greyhawk_tactical) {
  send_to_char(ch, "Tactical display not yet implemented.\r\n");
}

ACMD(do_greyhawk_status) {
  room_rnum ship_room = IN_ROOM(ch);
  int shipnum;
  int terrain_type;
  const char *terrain_name = "Unknown";
  
  /* Check if character is in a ship */
  if (!world[ship_room].ship) {
    send_to_char(ch, "You must be aboard a ship to check its status!\r\n");
    return;
  }
  
  shipnum = world[ship_room].ship->shipnum;
  
  /* Get terrain type at current position */
  terrain_type = get_ship_terrain_type(shipnum);
  
  /* Convert terrain type to name */
  switch (terrain_type) {
    case SECT_OCEAN: terrain_name = "Ocean"; break;
    case SECT_WATER_NOSWIM: terrain_name = "Deep Water"; break;
    case SECT_WATER_SWIM: terrain_name = "Shallow Water"; break;
    case SECT_UNDERWATER: terrain_name = "Underwater"; break;
    case SECT_FIELD: terrain_name = "Plains"; break;
    case SECT_FOREST: terrain_name = "Forest"; break;
    case SECT_HILLS: terrain_name = "Hills"; break;
    case SECT_MOUNTAIN: terrain_name = "Mountains"; break;
    case SECT_BEACH: terrain_name = "Beach"; break;
    default: terrain_name = "Unknown"; break;
  }
  
  send_to_char(ch, "\r\n");
  send_to_char(ch, "=== Ship Status ===\r\n");
  send_to_char(ch, "Ship Name: %s\r\n", greyhawk_ships[shipnum].name[0] ? greyhawk_ships[shipnum].name : "Unnamed Vessel");
  send_to_char(ch, "Ship ID: %s\r\n", greyhawk_ships[shipnum].id);
  send_to_char(ch, "\r\n");
  send_to_char(ch, "== Position ==\r\n");
  send_to_char(ch, "Coordinates: (%d, %d)\r\n", (int)greyhawk_ships[shipnum].x, (int)greyhawk_ships[shipnum].y);
  send_to_char(ch, "Elevation/Depth: %d\r\n", (int)greyhawk_ships[shipnum].z);
  send_to_char(ch, "Terrain: %s\r\n", terrain_name);
  send_to_char(ch, "\r\n");
  send_to_char(ch, "== Navigation ==\r\n");
  send_to_char(ch, "Heading: %d degrees\r\n", greyhawk_ships[shipnum].heading);
  send_to_char(ch, "Speed: %d / %d\r\n", greyhawk_ships[shipnum].speed, greyhawk_ships[shipnum].maxspeed);
  send_to_char(ch, "\r\n");
  send_to_char(ch, "== Hull Integrity ==\r\n");
  send_to_char(ch, "Forward: %d/%d\r\n", greyhawk_ships[shipnum].farmor, greyhawk_ships[shipnum].maxfarmor);
  send_to_char(ch, "Port: %d/%d\r\n", greyhawk_ships[shipnum].parmor, greyhawk_ships[shipnum].maxparmor);
  send_to_char(ch, "Starboard: %d/%d\r\n", greyhawk_ships[shipnum].sarmor, greyhawk_ships[shipnum].maxsarmor);
  send_to_char(ch, "Rear: %d/%d\r\n", greyhawk_ships[shipnum].rarmor, greyhawk_ships[shipnum].maxrarmor);
  send_to_char(ch, "\r\n");
}

ACMD(do_greyhawk_speed) {
  char arg[MAX_INPUT_LENGTH];
  room_rnum ship_room = IN_ROOM(ch);
  int shipnum;
  int new_speed;
  
  /* Check if character is in a ship control room */
  if (!world[ship_room].ship) {
    send_to_char(ch, "You must be in a ship's control room to adjust speed!\r\n");
    return;
  }
  
  shipnum = world[ship_room].ship->shipnum;
  
  one_argument(argument, arg, sizeof(arg));
  
  if (!*arg) {
    send_to_char(ch, "Current speed: %d / %d\r\n", 
                 greyhawk_ships[shipnum].speed, greyhawk_ships[shipnum].maxspeed);
    send_to_char(ch, "Usage: speed <0-%d>\r\n", greyhawk_ships[shipnum].maxspeed);
    return;
  }
  
  new_speed = atoi(arg);
  
  /* Validate speed */
  if (new_speed < 0) {
    send_to_char(ch, "Speed cannot be negative!\r\n");
    return;
  }
  
  if (new_speed > greyhawk_ships[shipnum].maxspeed) {
    send_to_char(ch, "Maximum speed is %d!\r\n", greyhawk_ships[shipnum].maxspeed);
    return;
  }
  
  /* Set the new speed */
  greyhawk_ships[shipnum].setspeed = new_speed;
  greyhawk_ships[shipnum].speed = new_speed;

  /* Apply terrain modifiers using actual vessel type */
  {
    enum vessel_class vtype = get_vessel_type_from_ship(shipnum);
    int terrain_type = get_ship_terrain_type(shipnum);
    int speed_modifier = get_terrain_speed_modifier(vtype, terrain_type, 0);
    greyhawk_ships[shipnum].speed = (new_speed * speed_modifier) / 100;

    /* Send feedback */
    if (new_speed == 0)
    {
      send_to_char(ch, "All stop! The vessel comes to a halt.\r\n");
      act("$n brings the vessel to a stop.", FALSE, ch, 0, 0, TO_ROOM);
    }
    else if (new_speed < greyhawk_ships[shipnum].maxspeed / 3)
    {
      send_to_char(ch, "Slow ahead. Speed set to %d.\r\n", new_speed);
      act("$n reduces the vessel's speed.", FALSE, ch, 0, 0, TO_ROOM);
    }
    else if (new_speed < (greyhawk_ships[shipnum].maxspeed * 2) / 3)
    {
      send_to_char(ch, "Half speed. Speed set to %d.\r\n", new_speed);
      act("$n sets the vessel to half speed.", FALSE, ch, 0, 0, TO_ROOM);
    }
    else
    {
      send_to_char(ch, "Full speed ahead! Speed set to %d.\r\n", new_speed);
      act("$n sets the vessel to full speed!", FALSE, ch, 0, 0, TO_ROOM);
    }

    if (speed_modifier != 100)
    {
      send_to_char(ch, "Effective speed after terrain modifiers: %d\r\n", greyhawk_ships[shipnum].speed);
    }
  }
}

ACMD(do_greyhawk_heading) {
  char arg[MAX_INPUT_LENGTH];
  int direction = -1;
  int shipnum = -1;
  room_rnum ship_room = IN_ROOM(ch);
  
  /* Check if character is in a ship control room */
  if (!world[ship_room].ship) {
    send_to_char(ch, "You must be in a ship's control room to set heading!\r\n");
    return;
  }
  
  shipnum = world[ship_room].ship->shipnum;
  
  one_argument(argument, arg, sizeof(arg));
  
  if (!*arg) {
    send_to_char(ch, "Set heading to which direction?\r\n");
    send_to_char(ch, "Valid directions: north, south, east, west, northeast, northwest, southeast, southwest\r\n");
    return;
  }
  
  /* Parse direction */
  if (!str_cmp(arg, "north") || !str_cmp(arg, "n"))
    direction = NORTH;
  else if (!str_cmp(arg, "south") || !str_cmp(arg, "s"))
    direction = SOUTH;
  else if (!str_cmp(arg, "east") || !str_cmp(arg, "e"))
    direction = EAST;
  else if (!str_cmp(arg, "west") || !str_cmp(arg, "w"))
    direction = WEST;
  else if (!str_cmp(arg, "northeast") || !str_cmp(arg, "ne"))
    direction = NORTHEAST;
  else if (!str_cmp(arg, "northwest") || !str_cmp(arg, "nw"))
    direction = NORTHWEST;
  else if (!str_cmp(arg, "southeast") || !str_cmp(arg, "se"))
    direction = SOUTHEAST;
  else if (!str_cmp(arg, "southwest") || !str_cmp(arg, "sw"))
    direction = SOUTHWEST;
  else if (!str_cmp(arg, "up") || !str_cmp(arg, "u"))
    direction = UP;
  else if (!str_cmp(arg, "down") || !str_cmp(arg, "d"))
    direction = DOWN;
  else {
    send_to_char(ch, "Invalid direction!\r\n");
    return;
  }
  
  /* Move the ship using wilderness coordinates */
  if (move_ship_wilderness(shipnum, direction, ch)) {
    act("$n adjusts the ship's heading and the vessel begins to move.", FALSE, ch, 0, 0, TO_ROOM);
  }
}

ACMD(do_greyhawk_contacts) {
  send_to_char(ch, "Contact list not yet implemented.\r\n");
}

ACMD(do_greyhawk_disembark) {
  send_to_char(ch, "Disembark function not yet implemented.\r\n");
}

ACMD(do_greyhawk_shipload) {
  send_to_char(ch, "Ship loading not yet implemented.\r\n");
}

ACMD(do_greyhawk_setsail) {
  send_to_char(ch, "Set sail function not yet implemented.\r\n");
}