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
/* COMMAND FUNCTIONS                                                        */
/* ========================================================================= */

/* Board command - handled by special procedure on ship objects */
ACMD(do_board) {
  send_to_char(ch, "You need to be near a ship to board it.\r\n");
  /* The actual boarding is handled by the greyhawk_ship_object special procedure */
  /* This command exists just so 'board' is recognized as a valid command */
}

/* These will be implemented after the core system is working */

ACMD(do_greyhawk_tactical) {
  send_to_char(ch, "Tactical display not yet implemented.\r\n");
}

ACMD(do_greyhawk_status) {
  send_to_char(ch, "Ship status display not yet implemented.\r\n");
}

ACMD(do_greyhawk_speed) {
  send_to_char(ch, "Speed control not yet implemented.\r\n");
}

ACMD(do_greyhawk_heading) {
  send_to_char(ch, "Heading control not yet implemented.\r\n");
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