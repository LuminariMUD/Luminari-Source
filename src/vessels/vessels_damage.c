/* ************************************************************************
 *      File:   vessels_damage.c                      Part of LuminariMUD  *
 *   Purpose:   Vessel damage model: class condition profiles, arcs,      *
 *              hull and sail damage, breaches, sinking, and salvage      *
 *              (vessels-ships study S3, 3.3.1 and 3.3.3)                 *
 * ********************************************************************** */

/*
 * Each class takes its DurisMUD analog's per-arc armor and internal
 * structure, stated at the class beam armor; a prototype's armor (its beam
 * armor) scales all eight numbers in proportion. The beams are the big
 * targets and the stern is the weak spot.
 *
 * Arcs are relative to the heading, as in DurisMUD: fore 320-40 degrees,
 * starboard 40-140, rear 140-220, port 220-320.
 */

#include "conf.h"
#include "core/sysdep.h"
#include <math.h>
#include "core/structs.h"
#include "core/utils.h"
#include "core/comm.h"
#include "core/db.h"
#include "core/handler.h"
#include "vessels.h"

/* Class order follows enum vessel_class; arcs follow GREYHAWK_FORE (0),
 * GREYHAWK_PORT (1), GREYHAWK_REAR (2), GREYHAWK_STARBOARD (3). */
static const struct vessel_class_condition class_condition[NUM_VESSEL_TYPES] = {
    /* beam  armor F/P/R/S         internal F/P/R/S   sail  price */
    {3, {2, 3, 1, 3}, {1, 1, 1, 1}, 20, 200},                 /* RAFT: Duris sloop */
    {8, {6, 8, 4, 8}, {3, 4, 2, 4}, 40, 600},                 /* BOAT: yacht */
    {66, {53, 66, 33, 66}, {26, 33, 16, 33}, 110, 8000},      /* SHIP: caravel */
    {109, {87, 109, 65, 109}, {38, 47, 23, 47}, 140, 44000},  /* WARSHIP: frigate */
    {63, {50, 63, 37, 63}, {22, 27, 13, 27}, 120, 72000},     /* AIRSHIP: corvette */
    {84, {67, 84, 50, 84}, {29, 36, 18, 36}, 130, 60000},     /* SUBMARINE: destroyer */
    {110, {88, 110, 55, 110}, {44, 55, 27, 55}, 130, 24000},  /* TRANSPORT: galleon */
    {153, {122, 153, 91, 153}, {53, 66, 33, 66}, 160, 144000} /* MAGICAL: cruiser */
};

const struct vessel_class_condition *vessel_class_condition(enum vessel_class vessel_type)
{
  if (vessel_type < 0 || vessel_type >= NUM_VESSEL_TYPES)
  {
    vessel_type = VESSEL_SHIP;
  }
  return &class_condition[vessel_type];
}

/** A class profile value scaled from the class beam armor to `armor`. */
static int vessel_scale_profile(int value, int armor, int beam)
{
  if (beam <= 0)
  {
    return value;
  }
  return MIN(255, (value * armor + beam / 2) / beam);
}

/**
 * Set a hull's condition from its class profile at the prototype's armor:
 * per-arc armor and internal structure (at least 1 per arc), the class
 * sail, and a whole rudder.
 */
void vessel_initialize_condition(struct greyhawk_ship_data *ship, int armor)
{
  const struct vessel_class_condition *profile;
  int arc;

  if (ship == NULL)
  {
    return;
  }

  profile = vessel_class_condition(ship->vessel_type);
  armor = MAX(0, MIN(VESSEL_MAX_PROTOTYPE_ARMOR, armor));
  for (arc = 0; arc < VESSEL_NUM_ARCS; arc++)
  {
    *vessel_arc_max_armor(ship, arc) = *vessel_arc_armor(ship, arc) =
        (unsigned char)vessel_scale_profile(profile->armor[arc], armor, profile->beam_armor);
    *vessel_arc_max_internal(ship, arc) = *vessel_arc_internal(ship, arc) = (unsigned char)MAX(
        1, vessel_scale_profile(profile->internal[arc], armor, profile->beam_armor));
  }
  ship->maxmainsail = ship->mainsail = (unsigned char)profile->sail;
  ship->maxturnrate = ship->turnrate = VESSEL_RUDDER_MAX;
}

/* Arc field accessors, indexed GREYHAWK_FORE..GREYHAWK_STARBOARD. */
unsigned char *vessel_arc_armor(struct greyhawk_ship_data *ship, int arc)
{
  switch (arc)
  {
  case GREYHAWK_PORT:
    return &ship->parmor;
  case GREYHAWK_REAR:
    return &ship->rarmor;
  case GREYHAWK_STARBOARD:
    return &ship->sarmor;
  default:
    return &ship->farmor;
  }
}

unsigned char *vessel_arc_max_armor(struct greyhawk_ship_data *ship, int arc)
{
  switch (arc)
  {
  case GREYHAWK_PORT:
    return &ship->maxparmor;
  case GREYHAWK_REAR:
    return &ship->maxrarmor;
  case GREYHAWK_STARBOARD:
    return &ship->maxsarmor;
  default:
    return &ship->maxfarmor;
  }
}

unsigned char *vessel_arc_internal(struct greyhawk_ship_data *ship, int arc)
{
  switch (arc)
  {
  case GREYHAWK_PORT:
    return &ship->pinternal;
  case GREYHAWK_REAR:
    return &ship->rinternal;
  case GREYHAWK_STARBOARD:
    return &ship->sinternal;
  default:
    return &ship->finternal;
  }
}

unsigned char *vessel_arc_max_internal(struct greyhawk_ship_data *ship, int arc)
{
  switch (arc)
  {
  case GREYHAWK_PORT:
    return &ship->maxpinternal;
  case GREYHAWK_REAR:
    return &ship->maxrinternal;
  case GREYHAWK_STARBOARD:
    return &ship->maxsinternal;
  default:
    return &ship->maxfinternal;
  }
}

/** The arc a relative bearing (degrees off the bow) falls in. */
int vessel_arc_for_relative_bearing(int relative)
{
  relative %= 360;
  if (relative < 0)
  {
    relative += 360;
  }

  if (relative >= 320 || relative < 40)
  {
    return GREYHAWK_FORE;
  }
  if (relative < 140)
  {
    return GREYHAWK_STARBOARD;
  }
  if (relative < 220)
  {
    return GREYHAWK_REAR;
  }
  return GREYHAWK_PORT;
}
