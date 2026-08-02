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
static char greyhawk_weapon[320];
#define VESSEL_DYNAMIC_ROOM_CACHE_SIZE                                                             \
  (WILD_DYNAMIC_ROOM_VNUM_END - WILD_DYNAMIC_ROOM_VNUM_START + 1)
static bool vessel_dynamic_room_configured[VESSEL_DYNAMIC_ROOM_CACHE_SIZE];
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

/**
 * Static terrain capability data for each vessel type.
 * This table provides O(1) lookup for vessel capabilities.
 *
 * Format: {can_ocean, can_shallow, can_air, can_underwater, min_depth, max_alt, speed_mods}
 * Speed modifiers indexed by sector type (0-39), values are percentages (100=normal).
 */
static const struct vessel_terrain_caps vessel_terrain_data[NUM_VESSEL_TYPES] = {
    /* VESSEL_RAFT (0): Small, rivers/shallow water only */
    {FALSE, /* can_traverse_ocean */
     TRUE,  /* can_traverse_shallow */
     FALSE, /* can_traverse_air */
     FALSE, /* can_traverse_underwater */
     0,     /* min_water_depth */
     0,     /* max_altitude */
     {      /* Speed modifiers by sector type (index 0-39) */
      /* SECT_INSIDE=0 */ 0,
      /* SECT_CITY=1 */ 0,
      /* SECT_FIELD=2 */ 0,
      /* SECT_FOREST=3 */ 0,
      /* SECT_HILLS=4 */ 0,
      /* SECT_MOUNTAIN=5 */ 0,
      /* SECT_WATER_SWIM=6 */ 100, /* Full speed in shallow water */
      /* SECT_WATER_NOSWIM=7 */ 0, /* Cannot navigate deep water */
      /* SECT_FLYING=8 */ 0,
      /* SECT_UNDERWATER=9 */ 0,
      /* SECT_ZONE_START=10 */ 0,
      /* SECT_ROAD_NS=11 */ 0,
      /* SECT_ROAD_EW=12 */ 0,
      /* SECT_ROAD_INT=13 */ 0,
      /* SECT_DESERT=14 */ 0,
      /* SECT_OCEAN=15 */ 0,      /* Cannot navigate ocean */
      /* SECT_MARSHLAND=16 */ 75, /* Slow in marshes */
      /* SECT_HIGH_MOUNTAIN=17 */ 0,
      /* SECT_PLANES=18 */ 0,
      /* SECT_UD_WILD=19 */ 0,
      /* SECT_UD_CITY=20 */ 0,
      /* SECT_UD_INSIDE=21 */ 0,
      /* SECT_UD_WATER=22 */ 80, /* Slow in UD water */
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
      /* SECT_BEACH=33 */ 50,   /* Very slow near beach */
      /* SECT_SEAPORT=34 */ 60, /* Slow in port */
      /* SECT_INSIDE_ROOM=35 */ 0,
      /* SECT_RIVER=36 */ 100, /* Full speed on rivers */
      /* unused */ 0,
      0,
      0}},

    /* VESSEL_BOAT (1): Medium, coastal waters */
    {FALSE, /* can_traverse_ocean */
     TRUE,  /* can_traverse_shallow */
     FALSE, /* can_traverse_air */
     FALSE, /* can_traverse_underwater */
     0,     /* min_water_depth */
     0,     /* max_altitude */
     {/* SECT_INSIDE=0 */ 0,
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
      /* SECT_OCEAN=15 */ 0, /* Cannot navigate ocean */
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
      /* unused */ 0,
      0,
      0}},

    /* VESSEL_SHIP (2): Large, ocean-capable */
    {TRUE,  /* can_traverse_ocean */
     TRUE,  /* can_traverse_shallow */
     FALSE, /* can_traverse_air */
     FALSE, /* can_traverse_underwater */
     2,     /* min_water_depth (needs deeper water) */
     0,     /* max_altitude */
     {/* SECT_INSIDE=0 */ 0,
      /* SECT_CITY=1 */ 0,
      /* SECT_FIELD=2 */ 0,
      /* SECT_FOREST=3 */ 0,
      /* SECT_HILLS=4 */ 0,
      /* SECT_MOUNTAIN=5 */ 0,
      /* SECT_WATER_SWIM=6 */ 75,    /* Reduced in shallow water */
      /* SECT_WATER_NOSWIM=7 */ 100, /* Full speed in deep water */
      /* SECT_FLYING=8 */ 0,
      /* SECT_UNDERWATER=9 */ 0,
      /* SECT_ZONE_START=10 */ 0,
      /* SECT_ROAD_NS=11 */ 0,
      /* SECT_ROAD_EW=12 */ 0,
      /* SECT_ROAD_INT=13 */ 0,
      /* SECT_DESERT=14 */ 0,
      /* SECT_OCEAN=15 */ 100,   /* Full speed in ocean */
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
      /* SECT_BEACH=33 */ 0,    /* Too shallow */
      /* SECT_SEAPORT=34 */ 50, /* Slow in port */
      /* SECT_INSIDE_ROOM=35 */ 0,
      /* SECT_RIVER=36 */ 50, /* Too large for most rivers */
      /* unused */ 0,
      0,
      0}},

    /* VESSEL_WARSHIP (3): Combat vessel, heavily armed - same as SHIP */
    {TRUE,  /* can_traverse_ocean */
     TRUE,  /* can_traverse_shallow */
     FALSE, /* can_traverse_air */
     FALSE, /* can_traverse_underwater */
     2,     /* min_water_depth */
     0,     /* max_altitude */
     {/* SECT_INSIDE=0 */ 0,
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
      /* unused */ 0,
      0,
      0}},

    /* VESSEL_AIRSHIP (4): Flying vessel, ignores terrain when airborne */
    {TRUE,  /* can_traverse_ocean (when landed/low) */
     TRUE,  /* can_traverse_shallow */
     TRUE,  /* can_traverse_air */
     FALSE, /* can_traverse_underwater */
     0,     /* min_water_depth */
     500,   /* max_altitude */
     {      /* At altitude, airships have 100 speed over all terrain except lava */
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
      /* unused */ 0,
      0,
      0}},

    /* VESSEL_SUBMARINE (5): Underwater vessel, depth navigation */
    {TRUE,  /* can_traverse_ocean */
     TRUE,  /* can_traverse_shallow */
     FALSE, /* can_traverse_air */
     TRUE,  /* can_traverse_underwater */
     0,     /* min_water_depth */
     0,     /* max_altitude (negative z for depth) */
     {/* SECT_INSIDE=0 */ 0,
      /* SECT_CITY=1 */ 0,
      /* SECT_FIELD=2 */ 0,
      /* SECT_FOREST=3 */ 0,
      /* SECT_HILLS=4 */ 0,
      /* SECT_MOUNTAIN=5 */ 0,
      /* SECT_WATER_SWIM=6 */ 50, /* Slow in shallow water */
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
      /* unused */ 0,
      0,
      0}},

    /* VESSEL_TRANSPORT (6): Cargo/passenger vessel - similar to SHIP */
    {TRUE,  /* can_traverse_ocean */
     TRUE,  /* can_traverse_shallow */
     FALSE, /* can_traverse_air */
     FALSE, /* can_traverse_underwater */
     2,     /* min_water_depth */
     0,     /* max_altitude */
     {/* SECT_INSIDE=0 */ 0,
      /* SECT_CITY=1 */ 0,
      /* SECT_FIELD=2 */ 0,
      /* SECT_FOREST=3 */ 0,
      /* SECT_HILLS=4 */ 0,
      /* SECT_MOUNTAIN=5 */ 0,
      /* SECT_WATER_SWIM=6 */ 60,   /* Slow in shallow - heavy */
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
      /* unused */ 0,
      0,
      0}},

    /* VESSEL_MAGICAL (7): Special magical vessels - most capable */
    {TRUE, /* can_traverse_ocean */
     TRUE, /* can_traverse_shallow */
     TRUE, /* can_traverse_air */
     TRUE, /* can_traverse_underwater */
     0,    /* min_water_depth */
     1000, /* max_altitude */
     {/* SECT_INSIDE=0 */ 0,
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
      /* unused */ 0,
      0,
      0}}};

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
    log("SYSERR: get_vessel_terrain_caps: Invalid vessel type %d, defaulting to VESSEL_SHIP",
        vessel_type);
    return &vessel_terrain_data[VESSEL_SHIP];
  }

  return &vessel_terrain_data[vessel_type];
}

/**
 * Validate the vertical coordinate against a hull class's capabilities.
 *
 * Surface hulls stay on z=0, flying hulls may rise only to their configured
 * ceiling, and submersible hulls may use negative depth. Crush depth remains
 * bathymetry-driven in vessel_weather_tick(), rather than a class constant.
 */
bool vessel_z_within_class_limits(enum vessel_class vessel_type, int z)
{
  const struct vessel_terrain_caps *caps;

  caps = get_vessel_terrain_caps(vessel_type);
  if (caps == NULL)
  {
    return FALSE;
  }

  if (z > 0)
  {
    return caps->can_traverse_air && caps->max_altitude > 0 && z <= caps->max_altitude;
  }

  if (z < 0)
  {
    return caps->can_traverse_underwater;
  }

  return TRUE;
}

/**
 * Validate that a vertical coordinate belongs to the wilderness column below.
 *
 * Negative depth is meaningful only over water. High flight cannot occupy
 * underground or interior sectors. The ordinary terrain-speed table still
 * decides whether the class can traverse the resulting sector.
 */
bool vessel_z_allows_sector(enum vessel_class vessel_type, int sector_type, int z)
{
  if (!vessel_z_within_class_limits(vessel_type, z))
  {
    return FALSE;
  }

  if (z < 0)
  {
    switch (sector_type)
    {
    case SECT_BEACH:
    case SECT_OCEAN:
    case SECT_RIVER:
    case SECT_SEAPORT:
    case SECT_UD_NOSWIM:
    case SECT_UD_WATER:
    case SECT_WATER_NOSWIM:
    case SECT_WATER_SWIM:
    case SECT_UNDERWATER:
      return TRUE;
    default:
      return FALSE;
    }
  }

  if (sector_type == SECT_UNDERWATER)
  {
    return FALSE;
  }

  if (z > 100)
  {
    if (sector_type >= SECT_UD_WILD && sector_type <= SECT_UD_NOGROUND)
    {
      return FALSE;
    }
    if (sector_type == SECT_CAVE || sector_type == SECT_INSIDE ||
        sector_type == SECT_INSIDE_ROOM)
    {
      return FALSE;
    }
  }

  return TRUE;
}

static bool vessel_can_traverse_sector(enum vessel_class vessel_type, int sector_type, int z)
{
  const struct vessel_terrain_caps *caps;

  caps = get_vessel_terrain_caps(vessel_type);
  if (caps == NULL || !vessel_z_allows_sector(vessel_type, sector_type, z) ||
      sector_type < 0 || sector_type >= 40)
  {
    return FALSE;
  }

  return caps->terrain_speed_mod[sector_type] != 0;
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
  if (shipnum < 0 || shipnum >= GREYHAWK_MAXSHIPS ||
      !is_valid_ship(&greyhawk_ships[shipnum]))
  {
    log("SYSERR: get_vessel_type_from_ship: Invalid ship number %d", shipnum);
    return VESSEL_SHIP; /* Default to standard ship */
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
      "Raft", "Boat", "Ship", "Warship", "Airship", "Submarine", "Transport", "Magical Vessel"};

  if (vessel_type < 0 || vessel_type >= NUM_VESSEL_TYPES)
  {
    return "Unknown Vessel";
  }

  return vessel_names[vessel_type];
}

/**
 * Get maximum cargo capacity for a vessel type.
 *
 * Returns the total cargo weight (in pounds) a vessel of the given class
 * can carry, covering loaded vehicles and future cargo lots. Values follow
 * the same per-class data-table pattern as vessel_terrain_data[].
 *
 * @param vessel_type The vessel_class enum value
 * @return Maximum cargo weight in pounds (0 for classes with no cargo space)
 */
int get_vessel_cargo_capacity(enum vessel_class vessel_type)
{
  static const int vessel_cargo_capacity[NUM_VESSEL_TYPES] = {
      VESSEL_CARGO_RAFT,      /* VESSEL_RAFT */
      VESSEL_CARGO_BOAT,      /* VESSEL_BOAT */
      VESSEL_CARGO_SHIP,      /* VESSEL_SHIP */
      VESSEL_CARGO_WARSHIP,   /* VESSEL_WARSHIP */
      VESSEL_CARGO_AIRSHIP,   /* VESSEL_AIRSHIP */
      VESSEL_CARGO_SUBMARINE, /* VESSEL_SUBMARINE */
      VESSEL_CARGO_TRANSPORT, /* VESSEL_TRANSPORT */
      VESSEL_CARGO_MAGICAL    /* VESSEL_MAGICAL */
  };

  if (vessel_type < 0 || vessel_type >= NUM_VESSEL_TYPES)
  {
    log("SYSERR: get_vessel_cargo_capacity: Invalid vessel type %d, defaulting to VESSEL_SHIP",
        vessel_type);
    return vessel_cargo_capacity[VESSEL_SHIP];
  }

  return vessel_cargo_capacity[vessel_type];
}

/**
 * Get a specific ship's effective cargo capacity.
 *
 * Class capacity plus the quartermaster's stowage bonus (10% per tier).
 *
 * @param ship The ship to measure
 * @return Cargo capacity in pounds
 */
int vessel_effective_cargo_capacity(const struct greyhawk_ship_data *ship)
{
  int base;

  if (ship == NULL)
  {
    return 0;
  }

  base = get_vessel_cargo_capacity(ship->vessel_type);
  if (IS_SET(ship->upgrades, SHIP_UPGRADE_HOLD))
  {
    base += base / 4;
  }
  return base + (base * ship->crew_tier[CREW_QUARTERMASTER]) / 10;
}

/**
 * Initialize every field consumed by the vessel damage model.
 *
 * A hull with armor but zero internal structure sinks on the first damaging
 * event, even when its armor absorbs the hit. Keep legacy and builder-spawned
 * vessels on the same complete condition baseline.
 */
void vessel_initialize_condition(struct greyhawk_ship_data *ship, int armor)
{
  int bounded_armor;
  int structure;

  if (ship == NULL)
  {
    return;
  }

  bounded_armor = MAX(0, MIN(255, armor));
  structure = MAX(10, bounded_armor / 2 + 10);

  ship->maxfarmor = ship->farmor = (unsigned char)bounded_armor;
  ship->maxrarmor = ship->rarmor = (unsigned char)bounded_armor;
  ship->maxparmor = ship->parmor = (unsigned char)bounded_armor;
  ship->maxsarmor = ship->sarmor = (unsigned char)bounded_armor;
  ship->maxfinternal = ship->finternal = (unsigned char)structure;
  ship->maxrinternal = ship->rinternal = (unsigned char)structure;
  ship->maxpinternal = ship->pinternal = (unsigned char)structure;
  ship->maxsinternal = ship->sinternal = (unsigned char)structure;
  ship->maxmainsail = ship->mainsail = 20;
  ship->maxturnrate = ship->turnrate = 20;
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
int greyhawk_loadship(int template, int to_room, short int x_cord, short int y_cord,
                      short int z_cord);
void greyhawk_nameship(char *name, int shipnum);
bool greyhawk_setsail(int class, int shipnum);
void greyhawk_initialize_ships(void);

/* ========================================================================= */
/* WILDERNESS ROOM ALLOCATION HELPER                                        */
/* ========================================================================= */

static int vessel_dynamic_room_cache_index(room_rnum room)
{
  room_vnum vnum;

  if (world == NULL || room == NOWHERE || room > top_of_world)
  {
    return -1;
  }

  vnum = GET_ROOM_VNUM(room);
  if (vnum < WILD_DYNAMIC_ROOM_VNUM_START || vnum > WILD_DYNAMIC_ROOM_VNUM_END)
  {
    return -1;
  }

  return vnum - WILD_DYNAMIC_ROOM_VNUM_START;
}

/**
 * Forget cached dynamic-room metadata when the vessel runtime initializes.
 */
void vessel_dynamic_room_cache_reset(void)
{
  memset(vessel_dynamic_room_configured, 0, sizeof(vessel_dynamic_room_configured));
}

/**
 * Remember that a dynamic room has valid coordinate and spatial metadata.
 */
void vessel_dynamic_room_cache_remember(room_rnum room)
{
  int index;

  index = vessel_dynamic_room_cache_index(room);
  if (index >= 0)
  {
    vessel_dynamic_room_configured[index] = TRUE;
  }
}

/**
 * Reuse a released dynamic room whose metadata already matches coordinates.
 */
room_rnum vessel_dynamic_room_cache_lookup(int x, int y)
{
  room_rnum room;
  int i;

  for (i = 0; i < VESSEL_DYNAMIC_ROOM_CACHE_SIZE; i++)
  {
    if (!vessel_dynamic_room_configured[i])
    {
      continue;
    }

    room = real_room(WILD_DYNAMIC_ROOM_VNUM_START + i);
    if (room == NOWHERE || ROOM_FLAGGED(room, ROOM_OCCUPIED))
    {
      continue;
    }

    if (world[room].coords[X_COORD] == x && world[room].coords[Y_COORD] == y)
    {
      return room;
    }
  }

  return NOWHERE;
}

/**
 * Prefer a never-configured free room before recycling cached metadata.
 */
room_rnum vessel_dynamic_room_cache_unused(void)
{
  room_rnum room;
  int i;

  for (i = 0; i < VESSEL_DYNAMIC_ROOM_CACHE_SIZE; i++)
  {
    if (vessel_dynamic_room_configured[i])
    {
      continue;
    }

    room = real_room(WILD_DYNAMIC_ROOM_VNUM_START + i);
    if (room != NOWHERE && !ROOM_FLAGGED(room, ROOM_OCCUPIED))
    {
      return room;
    }
  }

  return NOWHERE;
}

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
room_rnum get_or_allocate_wilderness_room(int x, int y)
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

  if (room != NOWHERE)
  {
    vessel_dynamic_room_cache_remember(room);
    return room;
  }

  /* A released room can retain valid metadata for a previously visited
   * coordinate. Reusing it avoids repeating region/path spatial queries. */
  room = vessel_dynamic_room_cache_lookup(x, y);

  if (room == NOWHERE)
  {
    /* Fill an unused pool entry before overwriting cached route metadata. */
    room = vessel_dynamic_room_cache_unused();
    if (room == NOWHERE)
    {
      room = find_available_wilderness_room();
    }
    if (room == NOWHERE)
    {
      log("SYSERR: get_or_allocate_wilderness_room: Room pool exhausted at (%d, %d)", x, y);
      return NOWHERE;
    }

    /* Configure the room for these coordinates */
    assign_wilderness_room(room, x, y);
    vessel_dynamic_room_cache_remember(room);
    VSSL_DEBUG_MOVE("Allocated dynamic wilderness room %d at (%d,%d)", world[room].number, x, y);
  }

  return room;
}

/**
 * Build exterior hull keywords from both the literal and readable ship name.
 *
 * Prototype and player names may contain underscores or punctuation. Retain
 * the literal spelling for exact references and append alphanumeric words so
 * ordinary commands such as "board dinghy" remain discoverable.
 */
void vessel_build_hull_keywords(char *buffer, size_t buffer_size, const char *name)
{
  size_t length;
  size_t i;
  bool separator;

  if (buffer == NULL || buffer_size == 0)
  {
    return;
  }

  if (name == NULL)
  {
    name = "";
  }

  snprintf(buffer, buffer_size, "ship vessel %s", name);
  length = strlen(buffer);
  if (length >= buffer_size - 1 || *name == '\0')
  {
    return;
  }

  buffer[length++] = ' ';
  separator = TRUE;
  for (i = 0; name[i] != '\0' && length < buffer_size - 1; i++)
  {
    if (isalnum((unsigned char)name[i]))
    {
      buffer[length++] = name[i];
      separator = FALSE;
    }
    else if (!separator)
    {
      buffer[length++] = ' ';
      separator = TRUE;
    }
  }
  while (length > 0 && buffer[length - 1] == ' ')
  {
    length--;
  }
  buffer[length] = '\0';
}

/**
 * Return whether an exterior hull is owned by an active fleet slot.
 *
 * Zone reset remove commands must not extract these runtime-managed objects.
 * Unlinked or stale hull objects remain eligible for ordinary zone cleanup.
 */
bool vessel_hull_is_managed(const struct obj_data *obj)
{
  int shipnum;

  if (obj == NULL || GET_OBJ_TYPE(obj) != ITEM_GREYHAWK_SHIP)
  {
    return FALSE;
  }

  shipnum = GET_OBJ_VAL(obj, 1);
  if (shipnum < 0 || shipnum >= GREYHAWK_MAXSHIPS ||
      !is_valid_ship(&greyhawk_ships[shipnum]))
  {
    return FALSE;
  }

  return greyhawk_ships[shipnum].shipobj == obj;
}

/* ========================================================================= */
/* GREYHAWK SHIP UTILITY FUNCTIONS                                         */
/* ========================================================================= */

/**
 * Get weapon status string for display
 * @param slot Weapon slot number
 * @param rnum Room number containing ship
 */
void greyhawk_getstatus(int slot, int rnum)
{
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
void greyhawk_getposition(int slot, int rnum)
{
  switch (world[rnum].ship->slot[slot].position)
  {
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
void greyhawk_dispweapon(int slot, int rnum)
{
  if (world[rnum].ship->slot[slot].type != 1)
  {
    strcpy(greyhawk_weapon, " ");
  }
  else
  {
    greyhawk_getstatus(slot, rnum);
    greyhawk_getposition(slot, rnum);
    snprintf(greyhawk_weapon, sizeof(greyhawk_weapon), "%-20s &N%-6s  &+W%-9s  %d",
             world[rnum].ship->slot[slot].desc, greyhawk_status, greyhawk_position,
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
int greyhawk_weaprange(int shipnum, int slot, char range)
{
  if (greyhawk_ships[shipnum].slot[slot].type != 1)
    return 0;

  switch (range)
  {
  case GREYHAWK_SHRTRANGE:
    return (int)((float)(greyhawk_ships[shipnum].slot[slot].val0 -
                         greyhawk_ships[shipnum].slot[slot].val1) /
                     3 +
                 greyhawk_ships[shipnum].slot[slot].val1);
  case GREYHAWK_MEDRANGE:
    return (int)((float)((greyhawk_ships[shipnum].slot[slot].val0 -
                          greyhawk_ships[shipnum].slot[slot].val1) /
                         3) *
                     2 +
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
int greyhawk_bearing(float x1, float y1, float x2, float y2)
{
  int val;

  if (y1 == y2)
  {
    if (x1 > x2)
      return 270;
    return 90;
  }

  if (x1 == x2)
  {
    if (y1 > y2)
      return 180;
    else
      return 0;
  }

  val = atan((x2 - x1) / (y2 - y1)) * 180 / M_PI;

  if (y1 < y2)
  {
    if (val >= 0)
      return val;
    return (val + 360);
  }
  else
  {
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
float greyhawk_range(float x1, float y1, float z1, float x2, float y2, float z2)
{
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
void greyhawk_initialize_ships(void)
{
  int i, j;

  vessel_dynamic_room_cache_reset();

  /* Clear ship array */
  memset(greyhawk_ships, 0, sizeof(greyhawk_ships));

  /* Clear contact array */
  memset(greyhawk_contacts, 0, sizeof(greyhawk_contacts));

  /* Initialize tactical map with default ocean pattern */
  for (i = 0; i < 151; i++)
  {
    for (j = 0; j < 151; j++)
    {
      strcpy(greyhawk_tactical[i][j].map, "  ");
    }
  }

  /* Initialize the legacy zone-700 test vessel. */
  {
    struct greyhawk_ship_data *ship = &greyhawk_ships[1];
    room_rnum interior_rnum = real_room(70003);

    if (interior_rnum != NOWHERE)
    {
      ship->active = TRUE;
      ship->shipnum = 1;
      ship->hull_object_vnum = VESSEL_BASE_HULL_OBJ_VNUM;

      /* Interior room vnum - where players go when they board */
      ship->shiproom = 70003;
      ship->entrance_room = 70003;
      ship->bridge_room = 70003;

      /* Wilderness location - where the ship object sits */
      ship->x = -66.0;
      ship->y = 92.0;
      ship->z = 0.0;
      ship->location = 0; /* Will be set when ship object loads */

      /* Navigation */
      ship->heading = 0;
      ship->speed = 0;
      ship->maxspeed = 10;
      ship->minspeed = 0;
      ship->vessel_type = VESSEL_SHIP;
      ship->docked_to_ship = -1;

      /* Hull armor, internal structure, rigging, and steering */
      vessel_initialize_condition(ship, 100);

      /* Identity */
      strlcpy(ship->name, "Test Vessel", sizeof(ship->name));
      strlcpy(ship->id, "T1", sizeof(ship->id));

      /* Interior rooms array */
      ship->room_vnums[0] = 70003;
      ship->num_rooms = 1;

      world[interior_rnum].ship = ship;

      log("Greyhawk: Test vessel initialized in slot 1 - interior room 70003 "
          "(rnum %d), location (-66, 92)",
          interior_rnum);
    }
    else
    {
      log("Greyhawk: Interior room 70003 not found, test vessel not initialized");
    }
  }

  log("Greyhawk ship system initialized.");
}

/**
 * Relink active fleet slots to hull objects loaded by zone resets.
 *
 * Fleet data is initialized before zones reset at boot. Characters may log
 * back into a static vessel interior without boarding again, so the boarding
 * special procedure cannot be the only place that establishes shipobj.
 *
 * @return Number of hull objects linked
 */
int vessel_relink_world_objects(void)
{
  struct greyhawk_ship_data *ship;
  struct obj_data *obj;
  room_rnum interior_rnum;
  int linked;
  int shipnum;

  linked = 0;
  for (obj = object_list; obj != NULL; obj = obj->next)
  {
    if (GET_OBJ_TYPE(obj) != ITEM_GREYHAWK_SHIP)
    {
      continue;
    }

    shipnum = GET_OBJ_VAL(obj, 1);
    if (shipnum < 0 || shipnum >= GREYHAWK_MAXSHIPS ||
        !is_valid_ship(&greyhawk_ships[shipnum]))
    {
      continue;
    }

    ship = &greyhawk_ships[shipnum];
    if (ship->shiproom != GET_OBJ_VAL(obj, 0))
    {
      log("SYSERR: Ship object %d entrance %d disagrees with fleet slot %d room %d",
          GET_OBJ_VNUM(obj), GET_OBJ_VAL(obj, 0), shipnum, ship->shiproom);
      continue;
    }

    interior_rnum = real_room(ship->shiproom);
    if (interior_rnum == NOWHERE)
    {
      log("SYSERR: Ship object %d cannot relink missing interior room %d",
          GET_OBJ_VNUM(obj), ship->shiproom);
      continue;
    }

    if (ship->shipobj != NULL && ship->shipobj != obj &&
        IN_ROOM(ship->shipobj) != NOWHERE)
    {
      log("SYSERR: Fleet slot %d has duplicate live ship objects %d and %d", shipnum,
          GET_OBJ_VNUM(ship->shipobj), GET_OBJ_VNUM(obj));
      continue;
    }

    if (!vessel_place_hull_object(ship, obj))
    {
      log("SYSERR: Ship object %d could not be placed for fleet slot %d", GET_OBJ_VNUM(obj),
          shipnum);
      continue;
    }
    world[interior_rnum].ship = ship;
    linked++;
  }

  log("Greyhawk: Relinked %d active hull object%s after zone reset.", linked,
      linked == 1 ? "" : "s");
  return linked;
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
  room_rnum old_room;
  bool old_is_port;
  bool position_changed;

  /* Validate ship number */
  if (shipnum < 0 || shipnum >= GREYHAWK_MAXSHIPS ||
      !is_valid_ship(&greyhawk_ships[shipnum]))
  {
    log("SYSERR: update_ship_wilderness_position: Invalid ship number %d", shipnum);
    return FALSE;
  }

  /* Validate coordinates within wilderness bounds */
  if (new_x < -1024 || new_x > 1024 || new_y < -1024 || new_y > 1024)
  {
    log("SYSERR: update_ship_wilderness_position: Coordinates out of bounds (%d, %d)", new_x,
        new_y);
    return FALSE;
  }

  if (!vessel_z_within_class_limits(greyhawk_ships[shipnum].vessel_type, new_z))
  {
    log("Info: update_ship_wilderness_position: Ship %d rejects class Z %d", shipnum, new_z);
    return FALSE;
  }

  /* Get or allocate wilderness room at these coordinates using dynamic allocation */
  wilderness_room = get_or_allocate_wilderness_room(new_x, new_y);
  if (wilderness_room == NOWHERE)
  {
    log("SYSERR: update_ship_wilderness_position: Room pool exhausted or invalid coordinates (%d, "
        "%d)",
        new_x, new_y);
    return FALSE;
  }

  if (!vessel_can_traverse_sector(greyhawk_ships[shipnum].vessel_type,
                                  world[wilderness_room].sector_type, new_z))
  {
    log("Info: update_ship_wilderness_position: Ship %d cannot occupy (%d, %d, %d)", shipnum,
        new_x, new_y, new_z);
    return FALSE;
  }

  old_room = NOWHERE;
  old_is_port = get_ship_terrain_type(shipnum) == SECT_SEAPORT;
  position_changed = new_x != (int)greyhawk_ships[shipnum].x ||
                     new_y != (int)greyhawk_ships[shipnum].y ||
                     new_z != (int)greyhawk_ships[shipnum].z;
  if (greyhawk_ships[shipnum].shipobj != NULL)
  {
    old_room = IN_ROOM(greyhawk_ships[shipnum].shipobj);
    old_is_port = old_is_port || vessel_room_is_port(old_room);
  }

  if (position_changed &&
      (old_is_port || vessel_room_is_fee_berth(&greyhawk_ships[shipnum], old_room)) &&
      greyhawk_ships[shipnum].dock_fee_balance > 0)
  {
    send_to_ship(&greyhawk_ships[shipnum],
                 "The harbor master withholds clearance: %d gold in dock fees remains due.",
                 greyhawk_ships[shipnum].dock_fee_balance);
    return FALSE;
  }

  if (position_changed && !greyhawk_ships[shipnum].waters_region_initialized)
  {
    vessel_piracy_track_waters(&greyhawk_ships[shipnum], FALSE);
  }

  /* Commit coordinates only after room allocation and departure clearance. */
  greyhawk_ships[shipnum].x = (float)new_x;
  greyhawk_ships[shipnum].y = (float)new_y;
  greyhawk_ships[shipnum].z = (float)new_z;

  /* Update ship's location to the wilderness room */
  greyhawk_ships[shipnum].location = world[wilderness_room].number;

  /* If ship object exists, move it to new location.
   * ROOM LIFECYCLE: obj_from_room() removes the ship object from the old room's
   * contents list. Once empty (no people, no objects, no effects), the
   * event_check_occupied() event will clear ROOM_OCCUPIED flag, making the
   * room available for reuse by find_available_wilderness_room().
   */
  if (greyhawk_ships[shipnum].shipobj)
  {
    /* Per-step movement is available through focused development diagnostics. */
    if (old_room != NOWHERE && old_room != wilderness_room)
    {
      VSSL_DEBUG_MOVE("Ship %d departing room %d, moving to room %d at (%d, %d)", shipnum,
                      world[old_room].number, world[wilderness_room].number, new_x, new_y);
    }

    obj_from_room(greyhawk_ships[shipnum].shipobj);
    obj_to_room(greyhawk_ships[shipnum].shipobj, wilderness_room);
    mark_wilderness_room_occupied(wilderness_room);
  }

  vessel_update_port_berth(&greyhawk_ships[shipnum], old_room, wilderness_room,
                           old_is_port);
  update_ship_room_coordinates(&greyhawk_ships[shipnum]);
  sync_all_loaded_vehicles(&greyhawk_ships[shipnum]);
  if (position_changed)
  {
    vessel_piracy_track_waters(&greyhawk_ships[shipnum], TRUE);
  }

  VSSL_DEBUG_MOVE("Ship %d position updated to (%d,%d,%d) in room %d", shipnum, new_x, new_y, new_z,
                  world[wilderness_room].number);
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
  if (shipnum < 0 || shipnum >= GREYHAWK_MAXSHIPS ||
      !is_valid_ship(&greyhawk_ships[shipnum]))
  {
    return SECT_INSIDE; /* Default safe sector */
  }

  /* Get ship coordinates */
  x = (int)greyhawk_ships[shipnum].x;
  y = (int)greyhawk_ships[shipnum].y;

  /* Get or allocate wilderness room at coordinates using dynamic allocation */
  wilderness_room = get_or_allocate_wilderness_room(x, y);
  if (wilderness_room == NOWHERE)
  {
    log("SYSERR: get_ship_terrain_type: Room pool exhausted at (%d, %d)", x, y);
    return SECT_INSIDE; /* Default safe sector */
  }

  /* Return the sector type of the wilderness room */
  VSSL_DEBUG_MOVE("Ship %d terrain at (%d,%d): sector %d", shipnum, x, y,
                  world[wilderness_room].sector_type);
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

  /* Validate coordinates within wilderness bounds first */
  if (x < -1024 || x > 1024 || y < -1024 || y > 1024)
  {
    return FALSE;
  }

  if (!vessel_z_within_class_limits(vessel_type, z))
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

  VSSL_DEBUG_MOVE("Traverse check: vessel type %d at (%d,%d,%d) sector %d", vessel_type, x, y, z,
                  sector_type);

  return vessel_can_traverse_sector(vessel_type, sector_type, z);
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
int get_terrain_speed_modifier(enum vessel_class vessel_type, int sector_type,
                               int weather_conditions)
{
  int base_modifier;
  const struct vessel_terrain_caps *caps;

  /* Get terrain capabilities for this vessel type */
  caps = get_vessel_terrain_caps(vessel_type);
  if (caps == NULL)
  {
    return 0; /* Invalid vessel type */
  }

  /* Validate sector type and get base modifier from table */
  if (sector_type < 0 || sector_type >= 40)
  {
    return 0; /* Invalid sector type */
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

  VSSL_DEBUG_MOVE("Speed modifier: vessel type %d sector %d weather %d -> %d%%", vessel_type,
                  sector_type, weather_conditions, base_modifier);
  return base_modifier;
}

/**
 * Move ship in given direction using wilderness coordinates
 * @param shipnum Ship index number
 * @param direction Direction to move (NORTH, SOUTH, EAST, WEST, etc.)
 * @param ch Character piloting the ship (for messages)
 * @return TRUE if movement successful, FALSE otherwise
 */
bool move_ship_wilderness(int shipnum, int direction, struct char_data *ch)
{
  int new_x, new_y, new_z;
  int speed_modifier;
  int terrain_type;
  int weather_conditions;
  int move_distance;
  enum vessel_class vessel_type;

  /* Validate ship number */
  if (shipnum < 0 || shipnum >= GREYHAWK_MAXSHIPS ||
      !is_valid_ship(&greyhawk_ships[shipnum]))
  {
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

  VSSL_DEBUG_MOVE("Ship %d moving dir %d from (%d,%d,%d) speed %d weather %d", shipnum, direction,
                  new_x, new_y, new_z, greyhawk_ships[shipnum].speed, weather_conditions);

  /* Calculate new position based on direction and speed */
  move_distance = MAX(1, greyhawk_ships[shipnum].speed / 10);

  /* Weather affects movement distance */
  if (weather_conditions > 50)
  {                                           /* Stormy weather */
    move_distance = MAX(1, move_distance * 75 / 100); /* 25% reduction */
    if (ch)
    {
      send_to_char(ch, "The harsh weather conditions slow your progress!\r\n");
    }
  }

  switch (direction)
  {
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
  case UP: /* For airships/submarines */
    new_z += 10;
    break;
  case DOWN: /* For airships/submarines */
    new_z -= 10;
    break;
  default:
    return FALSE;
  }

  /* Check if vessel can traverse the target terrain */
  if (!can_vessel_traverse_terrain(vessel_type, new_x, new_y, new_z))
  {
    VSSL_DEBUG_MOVE("Ship %d MOVE BLOCKED: type %d cannot enter (%d,%d,%d)", shipnum, vessel_type,
                    new_x, new_y, new_z);
    if (ch)
    {
      /* Send vessel-type-specific denial message */
      switch (vessel_type)
      {
      case VESSEL_RAFT:
        send_to_char(ch, "Your raft cannot navigate these waters! It's only suitable for rivers "
                         "and shallow water.\r\n");
        break;
      case VESSEL_BOAT:
        send_to_char(
            ch,
            "Your boat cannot handle these conditions! It's designed for coastal waters only.\r\n");
        break;
      case VESSEL_SHIP:
      case VESSEL_WARSHIP:
        send_to_char(ch,
                     "The ship cannot navigate this terrain! It requires deep water to sail.\r\n");
        break;
      case VESSEL_AIRSHIP:
        if (new_z < 100)
        {
          send_to_char(ch, "The airship cannot fly through this terrain at low altitude! Gain more "
                           "altitude.\r\n");
        }
        else
        {
          send_to_char(ch, "The airship cannot fly here - perhaps it's underground or the altitude "
                           "is too extreme.\r\n");
        }
        break;
      case VESSEL_SUBMARINE:
        if (new_z >= 0)
        {
          send_to_char(ch, "The submarine must dive to navigate underwater terrain! Use 'heading "
                           "down' to submerge.\r\n");
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
  if (!update_ship_wilderness_position(shipnum, new_x, new_y, new_z))
  {
    if (ch)
    {
      send_to_char(ch, "Movement failed - unable to update position.\r\n");
    }
    return FALSE;
  }

  /* Get terrain at new position and calculate speed modifier including weather */
  terrain_type = get_ship_terrain_type(shipnum);
  speed_modifier = get_terrain_speed_modifier(vessel_type, terrain_type, weather_conditions / 25);

  /* Adjust ship speed based on terrain and weather, then credit the
   * sailmaster's handling bonus (see vessels_crew.c) */
  greyhawk_ships[shipnum].speed = (greyhawk_ships[shipnum].setspeed * speed_modifier) / 100;
  greyhawk_ships[shipnum].speed += greyhawk_ships[shipnum].sailcrew.speedadjust;
  if (greyhawk_ships[shipnum].speed > greyhawk_ships[shipnum].maxspeed &&
      greyhawk_ships[shipnum].maxspeed > 0)
  {
    greyhawk_ships[shipnum].speed = greyhawk_ships[shipnum].maxspeed;
  }
  if (greyhawk_ships[shipnum].speed < 0)
  {
    greyhawk_ships[shipnum].speed = 0;
  }

  /* Send movement messages */
  if (ch)
  {
    send_to_char(ch, "The vessel moves %s across the wilderness.\r\n", dirs[direction]);
    send_to_char(ch, "Current position: (%d, %d, %d)\r\n", new_x, new_y, new_z);
    if (speed_modifier != 100)
    {
      send_to_char(ch, "Speed affected by terrain and weather: %d%%\r\n", speed_modifier);
    }

    /* Weather-specific messages */
    if (weather_conditions > 75)
    {
      send_to_char(ch, "The vessel struggles against the severe storm!\r\n");
      act("The ship rocks violently in the storm!", FALSE, ch, 0, 0, TO_ROOM);
    }
    else if (weather_conditions > 50)
    {
      send_to_char(ch, "Strong winds and rain buffet the vessel.\r\n");
      act("The ship sways in the rough weather.", FALSE, ch, 0, 0, TO_ROOM);
    }
    else if (weather_conditions > 25)
    {
      send_to_char(ch, "Light rain patters against the deck.\r\n");
    }
    else
    {
      send_to_char(ch, "The weather is clear for sailing.\r\n");
    }
  }

  /* Bathymetry check: deep-draft hulls ground out in the shallows */
  vessel_check_grounding(shipnum);

  return TRUE;
}

/* ========================================================================= */
/* EXTERNAL VIEW DISPLAY CONSTANTS AND HELPERS                              */
/* ========================================================================= */

/* Weather thresholds (matching wilderness weather system) */
#define VESSEL_WEATHER_CLEAR_MAX 127
#define VESSEL_WEATHER_CLOUDY_MAX 177
#define VESSEL_WEATHER_RAIN_MAX 199
#define VESSEL_WEATHER_STORM_MAX 224
/* Values 225-255 are lightning/thunderstorm */

/* Weather string lookup table */
static const char *vessel_weather_strings[] = {
    "Clear skies",                /* WEATHER_CLEAR (0-127) */
    "Overcast and cloudy",        /* WEATHER_CLOUDY (128-177) */
    "Light rain falling",         /* WEATHER_RAINY (178-199) */
    "Heavy storm conditions",     /* WEATHER_STORMY (200-224) */
    "Thunderstorm with lightning" /* WEATHER_LIGHTNING (225-255) */
};

/**
 * Convert raw weather value (0-255) to descriptive string.
 * Used by look_outside and tactical display commands.
 *
 * @param weather_val Raw weather value from get_weather()
 * @return Pointer to static weather description string
 */
static const char *get_vessel_weather_string(int weather_val)
{
  if (weather_val <= VESSEL_WEATHER_CLEAR_MAX)
    return vessel_weather_strings[0];
  else if (weather_val <= VESSEL_WEATHER_CLOUDY_MAX)
    return vessel_weather_strings[1];
  else if (weather_val <= VESSEL_WEATHER_RAIN_MAX)
    return vessel_weather_strings[2];
  else if (weather_val <= VESSEL_WEATHER_STORM_MAX)
    return vessel_weather_strings[3];
  else
    return vessel_weather_strings[4];
}

/* Tactical display constants */
#define TACTICAL_GRID_SIZE 11 /* 11x11 grid centered on ship */
#define TACTICAL_HALF_SIZE 5  /* Half the grid size for centering */
#define TACTICAL_MAX_WIDTH 40 /* Maximum display width in characters */

/* Tactical display symbols */
#define TACT_SYM_SHIP '@'    /* Current vessel position */
#define TACT_SYM_OTHER 'V'   /* Other vessels */
#define TACT_SYM_OCEAN '~'   /* Ocean/deep water */
#define TACT_SYM_SHALLOW '.' /* Shallow water */
#define TACT_SYM_LAND '#'    /* Land/impassable */
#define TACT_SYM_UNKNOWN '?' /* Unknown terrain */
#define TACT_SYM_DOCK 'D'    /* Dock/port */
#define TACT_SYM_BEACH ':'   /* Beach */

/* Contact detection constants */
#define CONTACT_DETECTION_RANGE 50 /* Default detection range in units */
#define CONTACT_MAX_DISPLAY 20     /* Maximum contacts to display */

/* Bearing direction strings (8 cardinal/ordinal directions) */
static const char *bearing_direction_str(int bearing)
{
  if (bearing >= 337 || bearing < 23)
    return "N";
  else if (bearing >= 23 && bearing < 68)
    return "NE";
  else if (bearing >= 68 && bearing < 113)
    return "E";
  else if (bearing >= 113 && bearing < 158)
    return "SE";
  else if (bearing >= 158 && bearing < 203)
    return "S";
  else if (bearing >= 203 && bearing < 248)
    return "SW";
  else if (bearing >= 248 && bearing < 293)
    return "W";
  else
    return "NW";
}

/* ========================================================================= */
/* COMMAND FUNCTIONS                                                        */
/* ========================================================================= */

/* Board command - handled by special procedure on ship objects */
ACMD(do_board_vessel)
{
  if (!CONFIG_VESSEL_SYSTEM)
  {
    send_to_char(ch, "The vessel system is currently disabled.\r\n");
    return;
  }

  send_to_char(ch, "You need to be near a ship to board it.\r\n");
  /* The actual boarding is handled by the greyhawk_ship_object special procedure */
  /* This command exists just so 'board' is recognized as a valid command */
}

/**
 * Get a simple ASCII character for terrain type display.
 * Used by tactical display for 80-column terminal compatibility.
 *
 * @param sector_type The sector type constant
 * @return Single character representing terrain
 */
static char get_tactical_terrain_char(int sector_type)
{
  switch (sector_type)
  {
  case SECT_OCEAN:
  case SECT_WATER_NOSWIM:
  case SECT_UD_NOSWIM:
    return TACT_SYM_OCEAN;

  case SECT_WATER_SWIM:
  case SECT_RIVER:
  case SECT_UD_WATER:
    return TACT_SYM_SHALLOW;

  case SECT_BEACH:
  case SECT_SEAPORT:
    return TACT_SYM_BEACH;

  case SECT_FIELD:
  case SECT_FOREST:
  case SECT_HILLS:
  case SECT_JUNGLE:
  case SECT_TAIGA:
  case SECT_TUNDRA:
  case SECT_DESERT:
  case SECT_MARSHLAND:
    return TACT_SYM_LAND;

  case SECT_MOUNTAIN:
  case SECT_HIGH_MOUNTAIN:
    return TACT_SYM_LAND;

  case SECT_CITY:
  case SECT_INSIDE:
    return TACT_SYM_DOCK;

  default:
    return TACT_SYM_UNKNOWN;
  }
}

/* Tactical display command */
ACMD(do_greyhawk_tactical)
{
  room_rnum ship_room;
  int shipnum;
  int ship_x, ship_y;
  int grid_x, grid_y;
  int world_x, world_y;
  int sector_type;
  int i;
  char grid[TACTICAL_GRID_SIZE][TACTICAL_GRID_SIZE];
  int weather_val;

  ship_room = IN_ROOM(ch);

  /* Check if character is on a ship */
  if (!world[ship_room].ship)
  {
    send_to_char(ch, "You must be aboard a vessel to view the tactical display.\r\n");
    return;
  }

  shipnum = world[ship_room].ship->shipnum;

  /* Validate ship number */
  if (shipnum < 0 || shipnum >= GREYHAWK_MAXSHIPS)
  {
    send_to_char(ch, "Error: Invalid vessel data.\r\n");
    return;
  }

  /* Get ship position */
  ship_x = (int)greyhawk_ships[shipnum].x;
  ship_y = (int)greyhawk_ships[shipnum].y;

  /* Initialize grid with terrain data */
  for (grid_y = 0; grid_y < TACTICAL_GRID_SIZE; grid_y++)
  {
    for (grid_x = 0; grid_x < TACTICAL_GRID_SIZE; grid_x++)
    {
      /* Calculate world coordinates (grid is centered on ship) */
      world_x = ship_x + (grid_x - TACTICAL_HALF_SIZE);
      world_y = ship_y + (TACTICAL_HALF_SIZE - grid_y); /* Y is inverted for display */

      /* Get terrain at this position */
      sector_type = get_modified_sector_type(0, world_x, world_y);
      grid[grid_y][grid_x] = get_tactical_terrain_char(sector_type);
    }
  }

  /* Mark ship position at center */
  grid[TACTICAL_HALF_SIZE][TACTICAL_HALF_SIZE] = TACT_SYM_SHIP;

  /* Mark other vessels in range */
  for (i = 0; i < GREYHAWK_MAXSHIPS; i++)
  {
    if (is_valid_ship(&greyhawk_ships[i]) && i != shipnum)
    {
      int other_x = (int)greyhawk_ships[i].x;
      int other_y = (int)greyhawk_ships[i].y;
      int rel_x = other_x - ship_x + TACTICAL_HALF_SIZE;
      int rel_y = TACTICAL_HALF_SIZE - (other_y - ship_y);

      /* Check if in display range */
      if (rel_x >= 0 && rel_x < TACTICAL_GRID_SIZE && rel_y >= 0 && rel_y < TACTICAL_GRID_SIZE)
      {
        grid[rel_y][rel_x] = TACT_SYM_OTHER;
      }
    }
  }

  /* Display tactical header */
  send_to_char(ch, "\r\n");
  send_to_char(ch, "       TACTICAL DISPLAY\r\n");
  send_to_char(ch, "   Position: [%d, %d]\r\n", ship_x, ship_y);
  send_to_char(ch, "   Heading: %d degrees\r\n", greyhawk_ships[shipnum].heading);

  /* Weather info */
  weather_val = get_weather(ship_x, ship_y);
  send_to_char(ch, "   Weather: %s\r\n", get_vessel_weather_string(weather_val));
  send_to_char(ch, "\r\n");

  /* Compass header */
  send_to_char(ch, "            N\r\n");
  send_to_char(ch, "       +-----------+\r\n");

  /* Render grid with borders */
  for (grid_y = 0; grid_y < TACTICAL_GRID_SIZE; grid_y++)
  {
    /* West indicator on center row */
    if (grid_y == TACTICAL_HALF_SIZE)
    {
      send_to_char(ch, "     W |");
    }
    else
    {
      send_to_char(ch, "       |");
    }

    /* Grid row */
    for (grid_x = 0; grid_x < TACTICAL_GRID_SIZE; grid_x++)
    {
      send_to_char(ch, "%c", grid[grid_y][grid_x]);
    }

    /* East indicator on center row */
    if (grid_y == TACTICAL_HALF_SIZE)
    {
      send_to_char(ch, "| E\r\n");
    }
    else
    {
      send_to_char(ch, "|\r\n");
    }
  }

  /* Compass footer */
  send_to_char(ch, "       +-----------+\r\n");
  send_to_char(ch, "            S\r\n");

  /* Legend */
  send_to_char(ch, "\r\n");
  send_to_char(ch, "   Legend: %c=You  %c=Vessel  %c=Ocean  %c=Shallow\r\n", TACT_SYM_SHIP,
               TACT_SYM_OTHER, TACT_SYM_OCEAN, TACT_SYM_SHALLOW);
  send_to_char(ch, "           %c=Land  %c=Beach  %c=Dock\r\n", TACT_SYM_LAND, TACT_SYM_BEACH,
               TACT_SYM_DOCK);
}

ACMD(do_greyhawk_status)
{
  room_rnum ship_room = IN_ROOM(ch);
  int shipnum;
  int terrain_type;
  const char *terrain_name = "Unknown";

  /* Check if character is in a ship */
  if (!world[ship_room].ship)
  {
    send_to_char(ch, "You must be aboard a ship to check its status!\r\n");
    return;
  }

  shipnum = world[ship_room].ship->shipnum;

  /* Get terrain type at current position */
  terrain_type = get_ship_terrain_type(shipnum);

  /* Convert terrain type to name */
  switch (terrain_type)
  {
  case SECT_OCEAN:
    terrain_name = "Ocean";
    break;
  case SECT_WATER_NOSWIM:
    terrain_name = "Deep Water";
    break;
  case SECT_WATER_SWIM:
    terrain_name = "Shallow Water";
    break;
  case SECT_UNDERWATER:
    terrain_name = "Underwater";
    break;
  case SECT_FIELD:
    terrain_name = "Plains";
    break;
  case SECT_FOREST:
    terrain_name = "Forest";
    break;
  case SECT_HILLS:
    terrain_name = "Hills";
    break;
  case SECT_MOUNTAIN:
    terrain_name = "Mountains";
    break;
  case SECT_BEACH:
    terrain_name = "Beach";
    break;
  case SECT_SEAPORT:
    terrain_name = "Seaport";
    break;
  default:
    terrain_name = "Unknown";
    break;
  }

  send_to_char(ch, "\r\n");
  send_to_char(ch, "=== Ship Status ===\r\n");
  send_to_char(ch, "Ship Name: %s\r\n",
               greyhawk_ships[shipnum].name[0] ? greyhawk_ships[shipnum].name : "Unnamed Vessel");
  send_to_char(ch, "Ship ID: %s\r\n", greyhawk_ships[shipnum].id);
  if (greyhawk_ships[shipnum].merchant_id > 0 &&
      greyhawk_ships[shipnum].merchant_faction_id >= FACTION_NONE &&
      greyhawk_ships[shipnum].merchant_faction_id < NUM_FACTIONS)
  {
    send_to_char(ch, "Merchant Registry: %d generation %u (%s)\r\n",
                 greyhawk_ships[shipnum].merchant_id,
                 greyhawk_ships[shipnum].merchant_generation,
                 factions[greyhawk_ships[shipnum].merchant_faction_id]);
  }
  send_to_char(ch, "\r\n");
  send_to_char(ch, "== Position ==\r\n");
  send_to_char(ch, "Coordinates: (%d, %d)\r\n", (int)greyhawk_ships[shipnum].x,
               (int)greyhawk_ships[shipnum].y);
  send_to_char(ch, "Elevation/Depth: %d\r\n", (int)greyhawk_ships[shipnum].z);
  send_to_char(ch, "Terrain: %s\r\n", terrain_name);
  send_to_char(ch, "\r\n");
  send_to_char(ch, "== Navigation ==\r\n");
  send_to_char(ch, "Heading: %d degrees\r\n", greyhawk_ships[shipnum].heading);
  send_to_char(ch, "Speed: %d / %d\r\n", greyhawk_ships[shipnum].speed,
               greyhawk_ships[shipnum].maxspeed);
  if (greyhawk_ships[shipnum].dock_fee_balance > 0)
  {
    send_to_char(ch, "Dock Fees: %d gold due at port %d\r\n",
                 greyhawk_ships[shipnum].dock_fee_balance,
                 greyhawk_ships[shipnum].dock_fee_port);
  }
  send_to_char(ch, "\r\n");
  send_to_char(ch, "== Hull Integrity ==\r\n");
  send_to_char(ch, "Forward: %d/%d\r\n", greyhawk_ships[shipnum].farmor,
               greyhawk_ships[shipnum].maxfarmor);
  send_to_char(ch, "Port: %d/%d\r\n", greyhawk_ships[shipnum].parmor,
               greyhawk_ships[shipnum].maxparmor);
  send_to_char(ch, "Starboard: %d/%d\r\n", greyhawk_ships[shipnum].sarmor,
               greyhawk_ships[shipnum].maxsarmor);
  send_to_char(ch, "Rear: %d/%d\r\n", greyhawk_ships[shipnum].rarmor,
               greyhawk_ships[shipnum].maxrarmor);
  send_to_char(ch, "\r\n");
}

ACMD(do_greyhawk_speed)
{
  char arg[MAX_INPUT_LENGTH];
  room_rnum ship_room = IN_ROOM(ch);
  struct greyhawk_ship_data *ship;
  int shipnum;
  int new_speed;

  ship = get_ship_from_room(ship_room);
  if (ship == NULL)
  {
    send_to_char(ch, "You must be in a ship's control room to adjust speed!\r\n");
    return;
  }

  if (!is_pilot(ch, ship))
  {
    send_to_char(ch, "You must be at an authorized helm to adjust speed.\r\n");
    return;
  }

  shipnum = ship->shipnum;

  one_argument(argument, arg, sizeof(arg));

  if (!*arg)
  {
    send_to_char(ch, "Current speed: %d / %d\r\n", greyhawk_ships[shipnum].speed,
                 greyhawk_ships[shipnum].maxspeed);
    send_to_char(ch, "Usage: speed <0-%d>\r\n", greyhawk_ships[shipnum].maxspeed);
    return;
  }

  new_speed = atoi(arg);

  /* Validate speed */
  if (new_speed < 0)
  {
    send_to_char(ch, "Speed cannot be negative!\r\n");
    return;
  }

  if (new_speed > greyhawk_ships[shipnum].maxspeed)
  {
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
      send_to_char(ch, "Effective speed after terrain modifiers: %d\r\n",
                   greyhawk_ships[shipnum].speed);
    }
  }
}

static int parse_vessel_direction(const char *arg)
{
  if (!str_cmp(arg, "north") || !str_cmp(arg, "n"))
    return NORTH;
  if (!str_cmp(arg, "south") || !str_cmp(arg, "s"))
    return SOUTH;
  if (!str_cmp(arg, "east") || !str_cmp(arg, "e"))
    return EAST;
  if (!str_cmp(arg, "west") || !str_cmp(arg, "w"))
    return WEST;
  if (!str_cmp(arg, "northeast") || !str_cmp(arg, "ne"))
    return NORTHEAST;
  if (!str_cmp(arg, "northwest") || !str_cmp(arg, "nw"))
    return NORTHWEST;
  if (!str_cmp(arg, "southeast") || !str_cmp(arg, "se"))
    return SOUTHEAST;
  if (!str_cmp(arg, "southwest") || !str_cmp(arg, "sw"))
    return SOUTHWEST;
  if (!str_cmp(arg, "up") || !str_cmp(arg, "u"))
    return UP;
  if (!str_cmp(arg, "down") || !str_cmp(arg, "d"))
    return DOWN;

  return -1;
}

static int vessel_direction_heading(int direction, int current_heading)
{
  switch (direction)
  {
  case NORTH:
    return 0;
  case NORTHEAST:
    return 45;
  case EAST:
    return 90;
  case SOUTHEAST:
    return 135;
  case SOUTH:
    return 180;
  case SOUTHWEST:
    return 225;
  case WEST:
    return 270;
  case NORTHWEST:
    return 315;
  default:
    return current_heading;
  }
}

ACMD(do_greyhawk_heading)
{
  char arg[MAX_INPUT_LENGTH];
  char *end;
  struct greyhawk_ship_data *ship;
  long heading;
  room_rnum ship_room = IN_ROOM(ch);

  ship = get_ship_from_room(ship_room);
  if (ship == NULL)
  {
    send_to_char(ch, "You must be in a ship's control room to set heading!\r\n");
    return;
  }

  if (!is_pilot(ch, ship))
  {
    send_to_char(ch, "You must be at an authorized helm to set heading.\r\n");
    return;
  }

  one_argument(argument, arg, sizeof(arg));

  if (!*arg)
  {
    send_to_char(ch, "Current heading: %d degrees (%s).\r\n", ship->heading,
                 bearing_direction_str(ship->heading));
    send_to_char(ch, "Usage: heading <0-360>\r\n");
    return;
  }

  heading = strtol(arg, &end, 10);
  if (*end != '\0' || heading < 0 || heading > 360)
  {
    send_to_char(ch, "Heading must be a number from 0 to 360 degrees.\r\n");
    return;
  }

  if (heading == 360)
    heading = 0;

  ship->setheading = (short int)heading;
  ship->heading = (short int)heading;
  send_to_char(ch, "Heading set to %d degrees (%s).\r\n", ship->heading,
               bearing_direction_str(ship->heading));
  act("$n adjusts the vessel's heading.", FALSE, ch, 0, 0, TO_ROOM);
}

/* Structure for sorting contacts by distance */
struct contact_entry
{
  int shipnum;
  float range;
  int bearing;
};

/* Comparison function for qsort - sort by range ascending */
static int compare_contacts(const void *a, const void *b)
{
  const struct contact_entry *ca = (const struct contact_entry *)a;
  const struct contact_entry *cb = (const struct contact_entry *)b;
  if (ca->range < cb->range)
    return -1;
  if (ca->range > cb->range)
    return 1;
  return 0;
}

/* Contacts display command */
ACMD(do_greyhawk_contacts)
{
  room_rnum ship_room;
  int shipnum;
  int ship_x, ship_y, ship_z;
  int i;
  int contact_count = 0;
  struct contact_entry contacts[CONTACT_MAX_DISPLAY];

  ship_room = IN_ROOM(ch);

  /* Check if character is on a ship */
  if (!world[ship_room].ship)
  {
    send_to_char(ch, "You must be aboard a vessel to check contacts.\r\n");
    return;
  }

  shipnum = world[ship_room].ship->shipnum;

  /* Validate ship number */
  if (shipnum < 0 || shipnum >= GREYHAWK_MAXSHIPS)
  {
    send_to_char(ch, "Error: Invalid vessel data.\r\n");
    return;
  }

  /* Get our ship position */
  ship_x = (int)greyhawk_ships[shipnum].x;
  ship_y = (int)greyhawk_ships[shipnum].y;
  ship_z = (int)greyhawk_ships[shipnum].z;

  /* Scan for other vessels in range */
  for (i = 0; i < GREYHAWK_MAXSHIPS && contact_count < CONTACT_MAX_DISPLAY; i++)
  {
    if (is_valid_ship(&greyhawk_ships[i]) && i != shipnum)
    {
      float range = greyhawk_range(ship_x, ship_y, ship_z, greyhawk_ships[i].x, greyhawk_ships[i].y,
                                   greyhawk_ships[i].z);

      if (range <= CONTACT_DETECTION_RANGE)
      {
        contacts[contact_count].shipnum = i;
        contacts[contact_count].range = range;
        contacts[contact_count].bearing =
            greyhawk_bearing(ship_x, ship_y, (int)greyhawk_ships[i].x, (int)greyhawk_ships[i].y);
        contact_count++;
      }
    }
  }

  /* Sort contacts by distance */
  if (contact_count > 1)
  {
    qsort(contacts, contact_count, sizeof(struct contact_entry), compare_contacts);
  }

  /* Display header */
  send_to_char(ch, "\r\n");
  send_to_char(ch, "       CONTACT LIST\r\n");
  send_to_char(ch, "   Our Position: [%d, %d]\r\n", ship_x, ship_y);
  send_to_char(ch, "   Detection Range: %d units\r\n", CONTACT_DETECTION_RANGE);
  send_to_char(ch, "\r\n");

  if (contact_count == 0)
  {
    send_to_char(ch, "   No contacts detected within range.\r\n");
  }
  else
  {
    send_to_char(ch, "   %-20s  %8s  %7s  %4s\r\n", "VESSEL", "RANGE", "BEARING", "DIR");
    send_to_char(ch, "   -------------------------------------------\r\n");

    for (i = 0; i < contact_count; i++)
    {
      int idx = contacts[i].shipnum;
      const char *name = greyhawk_ships[idx].name[0] ? greyhawk_ships[idx].name : "Unknown Vessel";

      send_to_char(ch, "   %-20s  %6.1f u  %5d deg  %s\r\n", name, contacts[i].range,
                   contacts[i].bearing, bearing_direction_str(contacts[i].bearing));
    }

    send_to_char(ch, "\r\n");
    send_to_char(ch, "   Total contacts: %d\r\n", contact_count);
  }
}

/* Disembark command - leave vessel */
ACMD(do_greyhawk_disembark)
{
  room_rnum ship_room;
  room_rnum exit_room = NOWHERE;
  int shipnum;
  int terrain_type;
  bool is_docked = FALSE;
  bool can_swim = FALSE;

  ship_room = IN_ROOM(ch);

  /* Check if character is on a ship */
  if (!world[ship_room].ship)
  {
    send_to_char(ch, "You're not aboard a vessel.\r\n");
    return;
  }

  shipnum = world[ship_room].ship->shipnum;

  /* Validate ship number */
  if (shipnum < 0 || shipnum >= GREYHAWK_MAXSHIPS)
  {
    send_to_char(ch, "Error: Invalid vessel data.\r\n");
    return;
  }

  /* Check if vessel is moving */
  if (greyhawk_ships[shipnum].speed > 0)
  {
    send_to_char(ch, "You can't disembark while the vessel is moving!\r\n");
    send_to_char(ch, "Bring the vessel to a stop first.\r\n");
    return;
  }

  /* Get terrain type at ship position */
  terrain_type = get_ship_terrain_type(shipnum);

  /* Check if docked (at a dock room or to another ship) */
  if (greyhawk_ships[shipnum].dock > 0 || greyhawk_ships[shipnum].docked_to_ship >= 0)
  {
    is_docked = TRUE;
  }

  /* Check for beach/seaport terrain */
  if (terrain_type == SECT_BEACH || terrain_type == SECT_SEAPORT)
  {
    is_docked = TRUE;
  }

  if (is_docked)
  {
    /* Docked - can safely disembark to dock */
    /* Use ship object's current room - reliable linkage established during boarding */
    if (greyhawk_ships[shipnum].shipobj)
    {
      exit_room = IN_ROOM(greyhawk_ships[shipnum].shipobj);
    }

    if (exit_room == NOWHERE)
    {
      send_to_char(ch, "Error: Unable to find a valid exit point.\r\n");
      return;
    }

    send_to_char(ch, "You step off the vessel onto the dock.\r\n");
    act("$n disembarks from the vessel.", TRUE, ch, 0, 0, TO_ROOM);

    /* Move character to exit room */
    char_from_room(ch);
    char_to_room(ch, exit_room);
    if (ZONE_FLAGGED(GET_ROOM_ZONE(exit_room), ZONE_WILDERNESS))
    {
      X_LOC(ch) = world[exit_room].coords[0];
      Y_LOC(ch) = world[exit_room].coords[1];
    }

    act("$n arrives from a nearby vessel.", TRUE, ch, 0, 0, TO_ROOM);
    look_at_room(ch, 0);
    return;
  }

  /* Not docked - disembarking to water */
  switch (terrain_type)
  {
  case SECT_OCEAN:
  case SECT_WATER_NOSWIM:
  case SECT_UD_NOSWIM:
    /* Deep water - cannot swim here */
    send_to_char(ch, "The water here is too deep and dangerous to enter.\r\n");
    send_to_char(ch, "You need to find a dock or shallower water.\r\n");
    return;

  case SECT_WATER_SWIM:
  case SECT_UD_WATER:
  case SECT_RIVER:
    /* Swimmable water - check if character can swim */
    break;

  default:
    send_to_char(ch, "You can't disembark here - no valid exit point.\r\n");
    return;
  }

  /* Check for swimming ability */
  if (GET_LEVEL(ch) >= LVL_IMMORT)
  {
    can_swim = TRUE;
  }
  else if (AFF_FLAGGED(ch, AFF_WATERWALK) || AFF_FLAGGED(ch, AFF_FLYING) ||
           AFF_FLAGGED(ch, AFF_LEVITATE))
  {
    can_swim = TRUE;
  }
  else if (GET_MOVE(ch) < 20)
  {
    send_to_char(ch, "You don't have enough energy to swim.\r\n");
    return;
  }
  else
  {
    can_swim = TRUE;
    /* Deduct movement cost */
    GET_MOVE(ch) -= 20;
    send_to_char(ch, "You prepare to swim...\r\n");
  }

  if (!can_swim)
  {
    send_to_char(ch, "You can't enter the water safely.\r\n");
    return;
  }

  /* Find exit room - use ship object's current room */
  if (greyhawk_ships[shipnum].shipobj)
  {
    exit_room = IN_ROOM(greyhawk_ships[shipnum].shipobj);
  }

  if (exit_room == NOWHERE)
  {
    send_to_char(ch, "Error: Unable to find a valid exit point.\r\n");
    return;
  }

  /* Perform the disembark */
  send_to_char(ch, "You leap from the vessel into the water!\r\n");
  act("$n jumps overboard into the water!", TRUE, ch, 0, 0, TO_ROOM);

  /* Move character to water room */
  char_from_room(ch);
  char_to_room(ch, exit_room);
  if (ZONE_FLAGGED(GET_ROOM_ZONE(exit_room), ZONE_WILDERNESS))
  {
    X_LOC(ch) = world[exit_room].coords[0];
    Y_LOC(ch) = world[exit_room].coords[1];
  }

  act("$n surfaces nearby, having jumped from a vessel.", TRUE, ch, 0, 0, TO_ROOM);
  look_at_room(ch, 0);
}

ACMD(do_greyhawk_shipload)
{
  send_to_char(ch, "Ship loading not yet implemented.\r\n");
}

ACMD(do_greyhawk_setsail)
{
  char arg[MAX_INPUT_LENGTH];
  struct greyhawk_ship_data *ship;
  int direction;

  ship = get_ship_from_room(IN_ROOM(ch));
  if (ship == NULL)
  {
    send_to_char(ch, "You must be aboard a vessel to set sail.\r\n");
    return;
  }

  if (!is_pilot(ch, ship))
  {
    send_to_char(ch, "You must be at an authorized helm to set sail.\r\n");
    return;
  }

  one_argument(argument, arg, sizeof(arg));
  direction = parse_vessel_direction(arg);
  if (direction < 0)
  {
    send_to_char(ch, "Usage: setsail <direction>\r\n");
    return;
  }

  if (ship->speed <= 0)
  {
    send_to_char(ch, "Set a positive speed before setting sail.\r\n");
    return;
  }

  ship->heading = (short int)vessel_direction_heading(direction, ship->heading);
  ship->setheading = ship->heading;
  if (move_ship_wilderness(ship->shipnum, direction, ch))
  {
    act("$n holds the vessel on its new course.", FALSE, ch, 0, 0, TO_ROOM);
  }
}
