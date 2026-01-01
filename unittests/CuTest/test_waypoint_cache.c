/**
 * @file test_waypoint_cache.c
 * @brief Unit tests for waypoint/route cache infrastructure
 *
 * Tests the in-memory cache linked list operations for the
 * waypoint and route management system.
 *
 * Part of Phase 01, Session 02: Waypoint/Route Management
 */

/* Enable POSIX features for snprintf in C89 mode */
#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "CuTest.h"

/* ========================================================================= */
/* TYPE DEFINITIONS (standalone to avoid server dependency)                 */
/* ========================================================================= */

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

typedef int bool;

#define AUTOPILOT_NAME_LENGTH 64

/* Waypoint structure */
struct waypoint
{
  float x;
  float y;
  float z;
  char name[AUTOPILOT_NAME_LENGTH];
  float tolerance;
  int wait_time;
  int flags;
};

/* Waypoint cache node */
struct waypoint_node
{
  int waypoint_id;
  struct waypoint data;
  struct waypoint_node *next;
};

/* Route cache node */
struct route_node
{
  int route_id;
  char name[AUTOPILOT_NAME_LENGTH];
  bool loop;
  bool active;
  int num_waypoints;
  int *waypoint_ids;
  struct route_node *next;
};

/* Local test cache heads */
static struct waypoint_node *test_waypoint_list = NULL;
static struct route_node *test_route_list = NULL;

/* ========================================================================= */
/* LOCAL CACHE OPERATIONS (mirrors vessels_autopilot.c)                      */
/* ========================================================================= */

static void test_waypoint_cache_add(struct waypoint_node *node)
{
  if (node == NULL)
    return;
  node->next = test_waypoint_list;
  test_waypoint_list = node;
}

static struct waypoint_node *test_waypoint_cache_find(int waypoint_id)
{
  struct waypoint_node *current;
  for (current = test_waypoint_list; current != NULL; current = current->next)
  {
    if (current->waypoint_id == waypoint_id)
      return current;
  }
  return NULL;
}

static void test_waypoint_cache_remove(int waypoint_id)
{
  struct waypoint_node *current, *prev;

  prev = NULL;
  current = test_waypoint_list;

  while (current != NULL)
  {
    if (current->waypoint_id == waypoint_id)
    {
      if (prev == NULL)
        test_waypoint_list = current->next;
      else
        prev->next = current->next;
      free(current);
      return;
    }
    prev = current;
    current = current->next;
  }
}

static void test_waypoint_cache_clear(void)
{
  struct waypoint_node *current, *next;
  current = test_waypoint_list;
  while (current != NULL)
  {
    next = current->next;
    free(current);
    current = next;
  }
  test_waypoint_list = NULL;
}

static void test_route_cache_add(struct route_node *node)
{
  if (node == NULL)
    return;
  node->next = test_route_list;
  test_route_list = node;
}

static struct route_node *test_route_cache_find(int route_id)
{
  struct route_node *current;
  for (current = test_route_list; current != NULL; current = current->next)
  {
    if (current->route_id == route_id)
      return current;
  }
  return NULL;
}

static void test_route_cache_remove(int route_id)
{
  struct route_node *current, *prev;

  prev = NULL;
  current = test_route_list;

  while (current != NULL)
  {
    if (current->route_id == route_id)
    {
      if (prev == NULL)
        test_route_list = current->next;
      else
        prev->next = current->next;
      if (current->waypoint_ids != NULL)
        free(current->waypoint_ids);
      free(current);
      return;
    }
    prev = current;
    current = current->next;
  }
}

static void test_route_cache_clear(void)
{
  struct route_node *current, *next;
  current = test_route_list;
  while (current != NULL)
  {
    next = current->next;
    if (current->waypoint_ids != NULL)
      free(current->waypoint_ids);
    free(current);
    current = next;
  }
  test_route_list = NULL;
}

/* ========================================================================= */
/* WAYPOINT CACHE TESTS                                                      */
/* ========================================================================= */

void test_waypoint_cache_add_single(CuTest *tc)
{
  struct waypoint_node *node;
  struct waypoint_node *found;

  /* Clear cache first */
  test_waypoint_cache_clear();

  /* Create and add a node */
  node = (struct waypoint_node *)malloc(sizeof(struct waypoint_node));
  CuAssertPtrNotNull(tc, node);

  node->waypoint_id = 1;
  node->data.x = 100.0f;
  node->data.y = 200.0f;
  node->data.z = 0.0f;
  strncpy(node->data.name, "Test Waypoint", AUTOPILOT_NAME_LENGTH - 1);
  node->data.tolerance = 5.0f;
  node->data.wait_time = 0;
  node->data.flags = 0;
  node->next = NULL;

  test_waypoint_cache_add(node);

  /* Verify node is in cache */
  found = test_waypoint_cache_find(1);
  CuAssertPtrNotNull(tc, found);
  CuAssertIntEquals(tc, 1, found->waypoint_id);
  CuAssertTrue(tc, found->data.x > 99.0f && found->data.x < 101.0f);

  /* Cleanup */
  test_waypoint_cache_clear();
}

void test_waypoint_cache_add_multiple(CuTest *tc)
{
  struct waypoint_node *found;
  int i;

  test_waypoint_cache_clear();

  /* Add 3 waypoints */
  for (i = 1; i <= 3; i++)
  {
    struct waypoint_node *node = (struct waypoint_node *)malloc(sizeof(struct waypoint_node));
    CuAssertPtrNotNull(tc, node);
    node->waypoint_id = i;
    node->data.x = (float)(i * 100);
    node->data.y = (float)(i * 200);
    node->data.z = 0.0f;
    snprintf(node->data.name, AUTOPILOT_NAME_LENGTH, "Waypoint %d", i);
    node->data.tolerance = 5.0f;
    node->data.wait_time = 0;
    node->data.flags = 0;
    node->next = NULL;
    test_waypoint_cache_add(node);
  }

  /* Verify all are findable */
  found = test_waypoint_cache_find(1);
  CuAssertPtrNotNull(tc, found);
  CuAssertIntEquals(tc, 1, found->waypoint_id);

  found = test_waypoint_cache_find(2);
  CuAssertPtrNotNull(tc, found);
  CuAssertIntEquals(tc, 2, found->waypoint_id);

  found = test_waypoint_cache_find(3);
  CuAssertPtrNotNull(tc, found);
  CuAssertIntEquals(tc, 3, found->waypoint_id);

  /* Verify non-existent returns NULL */
  found = test_waypoint_cache_find(99);
  CuAssertPtrEquals(tc, NULL, found);

  test_waypoint_cache_clear();
}

void test_waypoint_cache_remove_single(CuTest *tc)
{
  struct waypoint_node *node;
  struct waypoint_node *found;

  test_waypoint_cache_clear();

  /* Add node */
  node = (struct waypoint_node *)malloc(sizeof(struct waypoint_node));
  node->waypoint_id = 5;
  node->data.x = 500.0f;
  node->next = NULL;
  test_waypoint_cache_add(node);

  /* Verify it's there */
  found = test_waypoint_cache_find(5);
  CuAssertPtrNotNull(tc, found);

  /* Remove it */
  test_waypoint_cache_remove(5);

  /* Verify it's gone */
  found = test_waypoint_cache_find(5);
  CuAssertPtrEquals(tc, NULL, found);
}

void test_waypoint_cache_remove_middle(CuTest *tc)
{
  int i;

  test_waypoint_cache_clear();

  /* Add 5 waypoints */
  for (i = 1; i <= 5; i++)
  {
    struct waypoint_node *node = (struct waypoint_node *)malloc(sizeof(struct waypoint_node));
    node->waypoint_id = i;
    node->data.x = (float)i;
    node->next = NULL;
    test_waypoint_cache_add(node);
  }

  /* Remove middle one (id 3) */
  test_waypoint_cache_remove(3);

  /* Verify 3 is gone but others remain */
  CuAssertPtrEquals(tc, NULL, test_waypoint_cache_find(3));
  CuAssertPtrNotNull(tc, test_waypoint_cache_find(1));
  CuAssertPtrNotNull(tc, test_waypoint_cache_find(2));
  CuAssertPtrNotNull(tc, test_waypoint_cache_find(4));
  CuAssertPtrNotNull(tc, test_waypoint_cache_find(5));

  test_waypoint_cache_clear();
}

void test_waypoint_cache_clear_all(CuTest *tc)
{
  int i;

  test_waypoint_cache_clear();

  /* Add several waypoints */
  for (i = 1; i <= 10; i++)
  {
    struct waypoint_node *node = (struct waypoint_node *)malloc(sizeof(struct waypoint_node));
    node->waypoint_id = i;
    node->next = NULL;
    test_waypoint_cache_add(node);
  }

  /* Verify cache is populated */
  CuAssertPtrNotNull(tc, test_waypoint_cache_find(5));

  /* Clear cache */
  test_waypoint_cache_clear();

  /* Verify all are gone */
  CuAssertPtrEquals(tc, NULL, test_waypoint_cache_find(1));
  CuAssertPtrEquals(tc, NULL, test_waypoint_cache_find(5));
  CuAssertPtrEquals(tc, NULL, test_waypoint_cache_find(10));
}

/* ========================================================================= */
/* ROUTE CACHE TESTS                                                         */
/* ========================================================================= */

void test_route_cache_add_single(CuTest *tc)
{
  struct route_node *node;
  struct route_node *found;

  test_route_cache_clear();

  node = (struct route_node *)malloc(sizeof(struct route_node));
  CuAssertPtrNotNull(tc, node);

  node->route_id = 1;
  strncpy(node->name, "Trade Route Alpha", AUTOPILOT_NAME_LENGTH - 1);
  node->loop = TRUE;
  node->active = TRUE;
  node->num_waypoints = 5;
  node->waypoint_ids = (int *)malloc(sizeof(int) * 5);
  node->waypoint_ids[0] = 1;
  node->waypoint_ids[1] = 2;
  node->waypoint_ids[2] = 3;
  node->waypoint_ids[3] = 4;
  node->waypoint_ids[4] = 5;
  node->next = NULL;

  test_route_cache_add(node);

  found = test_route_cache_find(1);
  CuAssertPtrNotNull(tc, found);
  CuAssertIntEquals(tc, 1, found->route_id);
  CuAssertTrue(tc, strcmp(found->name, "Trade Route Alpha") == 0);
  CuAssertIntEquals(tc, TRUE, found->loop);
  CuAssertIntEquals(tc, 5, found->num_waypoints);
  CuAssertIntEquals(tc, 3, found->waypoint_ids[2]);

  test_route_cache_clear();
}

void test_route_cache_remove_with_waypoints(CuTest *tc)
{
  struct route_node *node;

  test_route_cache_clear();

  node = (struct route_node *)malloc(sizeof(struct route_node));
  node->route_id = 10;
  strncpy(node->name, "Test Route", AUTOPILOT_NAME_LENGTH - 1);
  node->loop = FALSE;
  node->active = TRUE;
  node->num_waypoints = 3;
  node->waypoint_ids = (int *)malloc(sizeof(int) * 3);
  node->waypoint_ids[0] = 100;
  node->waypoint_ids[1] = 200;
  node->waypoint_ids[2] = 300;
  node->next = NULL;

  test_route_cache_add(node);

  /* Verify it's there */
  CuAssertPtrNotNull(tc, test_route_cache_find(10));

  /* Remove it (should free waypoint_ids array too) */
  test_route_cache_remove(10);

  /* Verify it's gone */
  CuAssertPtrEquals(tc, NULL, test_route_cache_find(10));
}

void test_route_without_waypoints(CuTest *tc)
{
  struct route_node *node;
  struct route_node *found;

  test_route_cache_clear();

  /* Create route with no waypoints */
  node = (struct route_node *)malloc(sizeof(struct route_node));
  node->route_id = 50;
  strncpy(node->name, "Empty Route", AUTOPILOT_NAME_LENGTH - 1);
  node->loop = FALSE;
  node->active = FALSE;
  node->num_waypoints = 0;
  node->waypoint_ids = NULL;
  node->next = NULL;

  test_route_cache_add(node);

  found = test_route_cache_find(50);
  CuAssertPtrNotNull(tc, found);
  CuAssertIntEquals(tc, 0, found->num_waypoints);
  CuAssertPtrEquals(tc, NULL, found->waypoint_ids);

  test_route_cache_clear();
}

/* ========================================================================= */
/* WAYPOINT NODE STRUCTURE TESTS                                             */
/* ========================================================================= */

void test_waypoint_node_struct_size(CuTest *tc)
{
  size_t size = sizeof(struct waypoint_node);
  CuAssertTrue(tc, size > 0);
  CuAssertTrue(tc, size < 256); /* Should be reasonable size */
}

void test_route_node_struct_size(CuTest *tc)
{
  size_t size = sizeof(struct route_node);
  CuAssertTrue(tc, size > 0);
  CuAssertTrue(tc, size < 256); /* Should be reasonable size */
}

void test_memory_print_sizes(CuTest *tc)
{
  printf("\n  Cache structure sizes:\n");
  printf("    struct waypoint_node: %lu bytes\n", (unsigned long)sizeof(struct waypoint_node));
  printf("    struct route_node: %lu bytes\n", (unsigned long)sizeof(struct route_node));
  CuAssertTrue(tc, 1); /* Always pass, just printing info */
}

/* ========================================================================= */
/* TEST SUITE REGISTRATION                                                   */
/* ========================================================================= */

CuSuite *get_waypoint_cache_suite(void)
{
  CuSuite *suite = CuSuiteNew();

  /* Waypoint cache tests */
  SUITE_ADD_TEST(suite, test_waypoint_cache_add_single);
  SUITE_ADD_TEST(suite, test_waypoint_cache_add_multiple);
  SUITE_ADD_TEST(suite, test_waypoint_cache_remove_single);
  SUITE_ADD_TEST(suite, test_waypoint_cache_remove_middle);
  SUITE_ADD_TEST(suite, test_waypoint_cache_clear_all);

  /* Route cache tests */
  SUITE_ADD_TEST(suite, test_route_cache_add_single);
  SUITE_ADD_TEST(suite, test_route_cache_remove_with_waypoints);
  SUITE_ADD_TEST(suite, test_route_without_waypoints);

  /* Structure tests */
  SUITE_ADD_TEST(suite, test_waypoint_node_struct_size);
  SUITE_ADD_TEST(suite, test_route_node_struct_size);
  SUITE_ADD_TEST(suite, test_memory_print_sizes);

  return suite;
}

/* ========================================================================= */
/* STANDALONE TEST RUNNER                                                    */
/* ========================================================================= */

int main(void)
{
  CuString *output = CuStringNew();
  CuSuite *suite = CuSuiteNew();

  printf("LuminariMUD Waypoint Cache Tests\n");
  printf("================================\n");
  printf("Phase 01, Session 02: Waypoint/Route Management\n\n");

  CuSuiteAddSuite(suite, get_waypoint_cache_suite());

  CuSuiteRun(suite);
  CuSuiteSummary(suite, output);
  CuSuiteDetails(suite, output);

  printf("%s\n", output->buffer);

  return suite->failCount;
}
