/**
 * @file test_vessel_persistence.c
 * @brief Unit tests for vessel persistence system
 *
 * Tests save/load cycle, data integrity, and serialization for
 * the vessel persistence system.
 *
 * Part of Phase 00, Session 09: Testing and Validation
 */

/* Enable POSIX features for snprintf in C89 mode */
#define _POSIX_C_SOURCE 200112L
#include "CuTest.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================= */
/* TYPE DEFINITIONS (self-contained for unit testing)                        */
/* ========================================================================= */

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef NOWHERE
#define NOWHERE (-1)
#endif

typedef int bool;

/* Vessel class enum */
enum vessel_class
{
  VESSEL_RAFT = 0,
  VESSEL_BOAT = 1,
  VESSEL_SHIP = 2,
  VESSEL_WARSHIP = 3,
  VESSEL_AIRSHIP = 4,
  VESSEL_SUBMARINE = 5,
  VESSEL_TRANSPORT = 6,
  VESSEL_MAGICAL = 7
};

/* Ship room types */
enum ship_room_type
{
  ROOM_TYPE_BRIDGE,
  ROOM_TYPE_QUARTERS,
  ROOM_TYPE_CARGO,
  ROOM_TYPE_ENGINEERING,
  ROOM_TYPE_WEAPONS,
  ROOM_TYPE_MEDICAL,
  ROOM_TYPE_MESS_HALL,
  ROOM_TYPE_CORRIDOR,
  ROOM_TYPE_AIRLOCK,
  ROOM_TYPE_DECK
};

/* Constants */
#define MAX_SHIP_ROOMS 20
#define MAX_SHIP_CONNECTIONS 40
#define MAX_SERIAL_BUFFER 4096

/* Room connection structure */
struct room_connection
{
  int from_room;
  int to_room;
  int direction;
  bool is_hatch;
  bool is_locked;
};

/* Mock ship data for persistence testing */
struct mock_ship_data
{
  int shipnum;
  char name[128];
  char owner[64];
  float x, y, z;
  int heading;
  int speed;
  enum vessel_class vessel_type;
  int num_rooms;
  int room_vnums[MAX_SHIP_ROOMS];
  int entrance_room;
  int bridge_room;
  struct room_connection connections[MAX_SHIP_CONNECTIONS];
  int num_connections;
  int docked_to_ship;
};

/* ========================================================================= */
/* SERIALIZATION FUNCTIONS                                                   */
/* ========================================================================= */

/**
 * Serialize ship data to string buffer
 * Returns number of bytes written, -1 on error
 */
static int serialize_ship_data(struct mock_ship_data *ship, char *buffer, int bufsize)
{
  int written;
  int i;

  if (!ship || !buffer || bufsize < 256)
  {
    return -1;
  }

  /* Basic data */
  written = snprintf(buffer, bufsize,
                     "SHIP %d\n"
                     "NAME %s\n"
                     "OWNER %s\n"
                     "POS %.2f %.2f %.2f\n"
                     "NAV %d %d\n"
                     "TYPE %d\n"
                     "ROOMS %d\n",
                     ship->shipnum, ship->name, ship->owner, ship->x, ship->y, ship->z,
                     ship->heading, ship->speed, ship->vessel_type, ship->num_rooms);

  if (written < 0 || written >= bufsize)
  {
    return -1;
  }

  /* Room VNUMs */
  for (i = 0; i < ship->num_rooms && i < MAX_SHIP_ROOMS; i++)
  {
    int len = strlen(buffer);
    int added = snprintf(buffer + len, bufsize - len, "RVNUM %d %d\n", i, ship->room_vnums[i]);
    if (added < 0 || len + added >= bufsize)
    {
      return -1;
    }
  }

  /* Connections */
  {
    int len = strlen(buffer);
    int added = snprintf(buffer + len, bufsize - len, "CONNECTIONS %d\n", ship->num_connections);
    if (added < 0 || len + added >= bufsize)
    {
      return -1;
    }
  }

  for (i = 0; i < ship->num_connections && i < MAX_SHIP_CONNECTIONS; i++)
  {
    int len = strlen(buffer);
    int added = snprintf(buffer + len, bufsize - len, "CONN %d %d %d %d %d\n",
                         ship->connections[i].from_room, ship->connections[i].to_room,
                         ship->connections[i].direction, ship->connections[i].is_hatch,
                         ship->connections[i].is_locked);
    if (added < 0 || len + added >= bufsize)
    {
      return -1;
    }
  }

  /* Special rooms */
  {
    int len = strlen(buffer);
    snprintf(buffer + len, bufsize - len,
             "ENTRANCE %d\n"
             "BRIDGE %d\n"
             "DOCKED %d\n"
             "END\n",
             ship->entrance_room, ship->bridge_room, ship->docked_to_ship);
  }

  return (int)strlen(buffer);
}

/**
 * Deserialize ship data from string buffer
 * Returns TRUE on success, FALSE on error
 */
static bool deserialize_ship_data(const char *buffer, struct mock_ship_data *ship)
{
  const char *line;
  char lineBuffer[256];
  size_t len;

  if (!buffer || !ship)
  {
    return FALSE;
  }

  memset(ship, 0, sizeof(*ship));
  ship->docked_to_ship = -1;

  line = buffer;
  while (line && *line)
  {
    /* Extract line */
    const char *eol = strchr(line, '\n');
    if (eol)
    {
      len = (size_t)(eol - line);
      if (len >= sizeof(lineBuffer))
        len = sizeof(lineBuffer) - 1;
      strncpy(lineBuffer, line, len);
      lineBuffer[len] = '\0';
      line = eol + 1;
    }
    else
    {
      strncpy(lineBuffer, line, sizeof(lineBuffer) - 1);
      lineBuffer[sizeof(lineBuffer) - 1] = '\0';
      line = NULL;
    }

    /* Parse line */
    if (strncmp(lineBuffer, "SHIP ", 5) == 0)
    {
      ship->shipnum = atoi(lineBuffer + 5);
    }
    else if (strncmp(lineBuffer, "NAME ", 5) == 0)
    {
      strncpy(ship->name, lineBuffer + 5, sizeof(ship->name) - 1);
    }
    else if (strncmp(lineBuffer, "OWNER ", 6) == 0)
    {
      strncpy(ship->owner, lineBuffer + 6, sizeof(ship->owner) - 1);
    }
    else if (strncmp(lineBuffer, "POS ", 4) == 0)
    {
      sscanf(lineBuffer + 4, "%f %f %f", &ship->x, &ship->y, &ship->z);
    }
    else if (strncmp(lineBuffer, "NAV ", 4) == 0)
    {
      sscanf(lineBuffer + 4, "%d %d", &ship->heading, &ship->speed);
    }
    else if (strncmp(lineBuffer, "TYPE ", 5) == 0)
    {
      ship->vessel_type = atoi(lineBuffer + 5);
    }
    else if (strncmp(lineBuffer, "ROOMS ", 6) == 0)
    {
      ship->num_rooms = atoi(lineBuffer + 6);
    }
    else if (strncmp(lineBuffer, "RVNUM ", 6) == 0)
    {
      int idx, vnum;
      if (sscanf(lineBuffer + 6, "%d %d", &idx, &vnum) == 2)
      {
        if (idx >= 0 && idx < MAX_SHIP_ROOMS)
        {
          ship->room_vnums[idx] = vnum;
        }
      }
    }
    else if (strncmp(lineBuffer, "CONNECTIONS ", 12) == 0)
    {
      ship->num_connections = atoi(lineBuffer + 12);
    }
    else if (strncmp(lineBuffer, "CONN ", 5) == 0)
    {
      int from, to, dir, hatch, locked;
      if (sscanf(lineBuffer + 5, "%d %d %d %d %d", &from, &to, &dir, &hatch, &locked) == 5)
      {
        int idx = 0;
        while (idx < MAX_SHIP_CONNECTIONS && ship->connections[idx].from_room != 0)
        {
          idx++;
        }
        if (idx < MAX_SHIP_CONNECTIONS)
        {
          ship->connections[idx].from_room = from;
          ship->connections[idx].to_room = to;
          ship->connections[idx].direction = dir;
          ship->connections[idx].is_hatch = hatch;
          ship->connections[idx].is_locked = locked;
        }
      }
    }
    else if (strncmp(lineBuffer, "ENTRANCE ", 9) == 0)
    {
      ship->entrance_room = atoi(lineBuffer + 9);
    }
    else if (strncmp(lineBuffer, "BRIDGE ", 7) == 0)
    {
      ship->bridge_room = atoi(lineBuffer + 7);
    }
    else if (strncmp(lineBuffer, "DOCKED ", 7) == 0)
    {
      ship->docked_to_ship = atoi(lineBuffer + 7);
    }
    else if (strcmp(lineBuffer, "END") == 0)
    {
      break;
    }
  }

  return TRUE;
}

/**
 * Validate ship data integrity
 */
static bool validate_ship_data(struct mock_ship_data *ship)
{
  if (!ship)
  {
    return FALSE;
  }

  /* Check ship number is valid */
  if (ship->shipnum < 0)
  {
    return FALSE;
  }

  /* Check vessel type is valid */
  if (ship->vessel_type < 0 || ship->vessel_type > VESSEL_MAGICAL)
  {
    return FALSE;
  }

  /* Check room count is valid */
  if (ship->num_rooms < 0 || ship->num_rooms > MAX_SHIP_ROOMS)
  {
    return FALSE;
  }

  /* Check connection count is valid */
  if (ship->num_connections < 0 || ship->num_connections > MAX_SHIP_CONNECTIONS)
  {
    return FALSE;
  }

  /* Check coordinates are valid */
  if (ship->x < -1024 || ship->x > 1024)
  {
    return FALSE;
  }
  if (ship->y < -1024 || ship->y > 1024)
  {
    return FALSE;
  }
  if (ship->z < -500 || ship->z > 500)
  {
    return FALSE;
  }

  /* Check heading is valid */
  if (ship->heading < 0 || ship->heading > 359)
  {
    return FALSE;
  }

  return TRUE;
}

/* ========================================================================= */
/* HELPER FUNCTIONS                                                          */
/* ========================================================================= */

/**
 * Initialize test ship with valid data
 */
static void init_test_ship(struct mock_ship_data *ship)
{
  memset(ship, 0, sizeof(*ship));
  ship->shipnum = 42;
  strcpy(ship->name, "Test Vessel");
  strcpy(ship->owner, "TestOwner");
  ship->x = 100.0f;
  ship->y = -50.0f;
  ship->z = 0.0f;
  ship->heading = 90;
  ship->speed = 10;
  ship->vessel_type = VESSEL_SHIP;
  ship->num_rooms = 3;
  ship->room_vnums[0] = 70840;
  ship->room_vnums[1] = 70841;
  ship->room_vnums[2] = 70842;
  ship->entrance_room = 70840;
  ship->bridge_room = 70840;
  ship->docked_to_ship = -1;

  /* Add connections */
  ship->num_connections = 2;
  ship->connections[0].from_room = 70840;
  ship->connections[0].to_room = 70841;
  ship->connections[0].direction = 0; /* North */
  ship->connections[0].is_hatch = FALSE;
  ship->connections[0].is_locked = FALSE;

  ship->connections[1].from_room = 70841;
  ship->connections[1].to_room = 70842;
  ship->connections[1].direction = 0;
  ship->connections[1].is_hatch = FALSE;
  ship->connections[1].is_locked = FALSE;
}

/* ========================================================================= */
/* SERIALIZATION TESTS                                                       */
/* ========================================================================= */

/**
 * Test basic serialization
 */
void Test_persistence_serialize_basic(CuTest *tc)
{
  struct mock_ship_data ship;
  char buffer[MAX_SERIAL_BUFFER];
  int result;

  init_test_ship(&ship);

  result = serialize_ship_data(&ship, buffer, MAX_SERIAL_BUFFER);

  CuAssertTrue(tc, result > 0);
  CuAssertTrue(tc, strstr(buffer, "SHIP 42") != NULL);
  CuAssertTrue(tc, strstr(buffer, "NAME Test Vessel") != NULL);
  CuAssertTrue(tc, strstr(buffer, "TYPE 2") != NULL);
  CuAssertTrue(tc, strstr(buffer, "ROOMS 3") != NULL);
  CuAssertTrue(tc, strstr(buffer, "END") != NULL);
}

/**
 * Test serialization with NULL ship
 */
void Test_persistence_serialize_null(CuTest *tc)
{
  char buffer[MAX_SERIAL_BUFFER];
  int result;

  result = serialize_ship_data(NULL, buffer, MAX_SERIAL_BUFFER);
  CuAssertIntEquals(tc, -1, result);
}

/**
 * Test serialization with small buffer
 */
void Test_persistence_serialize_small_buffer(CuTest *tc)
{
  struct mock_ship_data ship;
  char buffer[50]; /* Too small */
  int result;

  init_test_ship(&ship);

  result = serialize_ship_data(&ship, buffer, 50);
  CuAssertIntEquals(tc, -1, result);
}

/* ========================================================================= */
/* DESERIALIZATION TESTS                                                     */
/* ========================================================================= */

/**
 * Test basic deserialization
 */
void Test_persistence_deserialize_basic(CuTest *tc)
{
  struct mock_ship_data ship;
  bool result;
  const char *data = "SHIP 42\n"
                     "NAME Test Ship\n"
                     "OWNER TestOwner\n"
                     "POS 100.00 -50.00 0.00\n"
                     "NAV 90 10\n"
                     "TYPE 2\n"
                     "ROOMS 2\n"
                     "RVNUM 0 70000\n"
                     "RVNUM 1 70001\n"
                     "CONNECTIONS 1\n"
                     "CONN 70000 70001 0 0 0\n"
                     "ENTRANCE 70000\n"
                     "BRIDGE 70000\n"
                     "DOCKED -1\n"
                     "END\n";

  result = deserialize_ship_data(data, &ship);

  CuAssertIntEquals(tc, TRUE, result);
  CuAssertIntEquals(tc, 42, ship.shipnum);
  CuAssertStrEquals(tc, "Test Ship", ship.name);
  CuAssertStrEquals(tc, "TestOwner", ship.owner);
  CuAssertDblEquals(tc, 100.0, ship.x, 0.1);
  CuAssertDblEquals(tc, -50.0, ship.y, 0.1);
  CuAssertIntEquals(tc, 90, ship.heading);
  CuAssertIntEquals(tc, 10, ship.speed);
  CuAssertIntEquals(tc, VESSEL_SHIP, ship.vessel_type);
  CuAssertIntEquals(tc, 2, ship.num_rooms);
  CuAssertIntEquals(tc, 70000, ship.room_vnums[0]);
  CuAssertIntEquals(tc, 70001, ship.room_vnums[1]);
}

/**
 * Test deserialization with NULL buffer
 */
void Test_persistence_deserialize_null_buffer(CuTest *tc)
{
  struct mock_ship_data ship;
  bool result;

  result = deserialize_ship_data(NULL, &ship);
  CuAssertIntEquals(tc, FALSE, result);
}

/**
 * Test deserialization with NULL ship
 */
void Test_persistence_deserialize_null_ship(CuTest *tc)
{
  bool result;
  const char *data = "SHIP 1\nEND\n";

  result = deserialize_ship_data(data, NULL);
  CuAssertIntEquals(tc, FALSE, result);
}

/* ========================================================================= */
/* ROUND-TRIP TESTS                                                          */
/* ========================================================================= */

/**
 * Test save/load round-trip preserves data
 */
void Test_persistence_roundtrip(CuTest *tc)
{
  struct mock_ship_data original, loaded;
  char buffer[MAX_SERIAL_BUFFER];
  int written;
  bool parsed;

  init_test_ship(&original);

  /* Serialize */
  written = serialize_ship_data(&original, buffer, MAX_SERIAL_BUFFER);
  CuAssertTrue(tc, written > 0);

  /* Deserialize */
  parsed = deserialize_ship_data(buffer, &loaded);
  CuAssertIntEquals(tc, TRUE, parsed);

  /* Compare */
  CuAssertIntEquals(tc, original.shipnum, loaded.shipnum);
  CuAssertStrEquals(tc, original.name, loaded.name);
  CuAssertStrEquals(tc, original.owner, loaded.owner);
  CuAssertDblEquals(tc, original.x, loaded.x, 0.01);
  CuAssertDblEquals(tc, original.y, loaded.y, 0.01);
  CuAssertDblEquals(tc, original.z, loaded.z, 0.01);
  CuAssertIntEquals(tc, original.heading, loaded.heading);
  CuAssertIntEquals(tc, original.speed, loaded.speed);
  CuAssertIntEquals(tc, original.vessel_type, loaded.vessel_type);
  CuAssertIntEquals(tc, original.num_rooms, loaded.num_rooms);
  CuAssertIntEquals(tc, original.entrance_room, loaded.entrance_room);
  CuAssertIntEquals(tc, original.bridge_room, loaded.bridge_room);
}

/**
 * Test round-trip with complex ship
 */
void Test_persistence_roundtrip_complex(CuTest *tc)
{
  struct mock_ship_data original, loaded;
  char buffer[MAX_SERIAL_BUFFER];
  int i;

  memset(&original, 0, sizeof(original));
  original.shipnum = 99;
  strcpy(original.name, "Complex Warship");
  strcpy(original.owner, "Admiral");
  original.x = -512.5f;
  original.y = 256.75f;
  original.z = 100.0f;
  original.heading = 270;
  original.speed = 25;
  original.vessel_type = VESSEL_WARSHIP;
  original.num_rooms = 10;

  for (i = 0; i < 10; i++)
  {
    original.room_vnums[i] = 71980 + i;
  }

  original.num_connections = 9;
  for (i = 0; i < 9; i++)
  {
    original.connections[i].from_room = 71980 + i;
    original.connections[i].to_room = 71981 + i;
    original.connections[i].direction = 0;
  }

  original.entrance_room = 71980;
  original.bridge_room = 71980;
  original.docked_to_ship = 5;

  /* Round-trip */
  serialize_ship_data(&original, buffer, MAX_SERIAL_BUFFER);
  deserialize_ship_data(buffer, &loaded);

  /* Verify */
  CuAssertIntEquals(tc, original.shipnum, loaded.shipnum);
  CuAssertStrEquals(tc, original.name, loaded.name);
  CuAssertIntEquals(tc, original.num_rooms, loaded.num_rooms);
  CuAssertIntEquals(tc, original.docked_to_ship, loaded.docked_to_ship);
}

/* ========================================================================= */
/* VALIDATION TESTS                                                          */
/* ========================================================================= */

/**
 * Test validation with valid ship
 */
void Test_persistence_validate_valid(CuTest *tc)
{
  struct mock_ship_data ship;

  init_test_ship(&ship);

  CuAssertIntEquals(tc, TRUE, validate_ship_data(&ship));
}

/**
 * Test validation with NULL ship
 */
void Test_persistence_validate_null(CuTest *tc)
{
  CuAssertIntEquals(tc, FALSE, validate_ship_data(NULL));
}

/**
 * Test validation with invalid coordinates
 */
void Test_persistence_validate_bad_coords(CuTest *tc)
{
  struct mock_ship_data ship;

  init_test_ship(&ship);

  /* X too large */
  ship.x = 2000.0f;
  CuAssertIntEquals(tc, FALSE, validate_ship_data(&ship));

  init_test_ship(&ship);

  /* Y too large */
  ship.y = 2000.0f;
  CuAssertIntEquals(tc, FALSE, validate_ship_data(&ship));

  init_test_ship(&ship);

  /* Z too large */
  ship.z = 600.0f;
  CuAssertIntEquals(tc, FALSE, validate_ship_data(&ship));
}

/**
 * Test validation with invalid vessel type
 */
void Test_persistence_validate_bad_type(CuTest *tc)
{
  struct mock_ship_data ship;

  init_test_ship(&ship);
  ship.vessel_type = 100;

  CuAssertIntEquals(tc, FALSE, validate_ship_data(&ship));
}

/**
 * Test validation with invalid room count
 */
void Test_persistence_validate_bad_rooms(CuTest *tc)
{
  struct mock_ship_data ship;

  init_test_ship(&ship);
  ship.num_rooms = 100;

  CuAssertIntEquals(tc, FALSE, validate_ship_data(&ship));
}

/**
 * Test validation with invalid heading
 */
void Test_persistence_validate_bad_heading(CuTest *tc)
{
  struct mock_ship_data ship;

  init_test_ship(&ship);
  ship.heading = 400;

  CuAssertIntEquals(tc, FALSE, validate_ship_data(&ship));
}

/* ========================================================================= */
/* TEST SUITE REGISTRATION                                                   */
/* ========================================================================= */

/**
 * Get the vessel persistence test suite
 */
CuSuite *VesselPersistenceGetSuite(void)
{
  CuSuite *suite = CuSuiteNew();

  /* Serialization tests */
  SUITE_ADD_TEST(suite, Test_persistence_serialize_basic);
  SUITE_ADD_TEST(suite, Test_persistence_serialize_null);
  SUITE_ADD_TEST(suite, Test_persistence_serialize_small_buffer);

  /* Deserialization tests */
  SUITE_ADD_TEST(suite, Test_persistence_deserialize_basic);
  SUITE_ADD_TEST(suite, Test_persistence_deserialize_null_buffer);
  SUITE_ADD_TEST(suite, Test_persistence_deserialize_null_ship);

  /* Round-trip tests */
  SUITE_ADD_TEST(suite, Test_persistence_roundtrip);
  SUITE_ADD_TEST(suite, Test_persistence_roundtrip_complex);

  /* Validation tests */
  SUITE_ADD_TEST(suite, Test_persistence_validate_valid);
  SUITE_ADD_TEST(suite, Test_persistence_validate_null);
  SUITE_ADD_TEST(suite, Test_persistence_validate_bad_coords);
  SUITE_ADD_TEST(suite, Test_persistence_validate_bad_type);
  SUITE_ADD_TEST(suite, Test_persistence_validate_bad_rooms);
  SUITE_ADD_TEST(suite, Test_persistence_validate_bad_heading);

  return suite;
}
