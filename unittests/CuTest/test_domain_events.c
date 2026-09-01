#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/db.h"
#include "../../src/handler.h"
#include "../../src/active_world.h"
#include "../../src/affected_owners.h"
#include "../../src/character_periodic.h"
#include "../../src/periodic_owners.h"
#include "../../src/point_update_periodic.h"
#include "../../src/quest/hunts.h"
#include "../../src/comm.h"
#include "../../src/dgscript/dg_event.h"
#include "../../src/dgscript/dg_scripts.h"
#include "../../src/domain_event_types.h"
#include "../../src/domain_events.h"
#include "../../src/domain_event_runtime.h"
#include "../../src/domain_event_world.h"
#include "../../src/event_runtime.h"
#include "../../src/magic/spells.h"
#include "../../src/mob/mob_act.h"
#include "../../src/mob/mob_known_spells.h"
#include "../../src/mob/mob_spellslots.h"
#include "../../src/movement/movement_tracks.h"
#include "../../src/bardic_performance.h"
#include "../../src/net/protocol.h"
#include "../../src/olc/genwld.h"
#include "../../src/vessels/vessel_periodic.h"
#include "../../src/vessels/vessels.h"
#include "../../src/wilderness/wilderness.h"
#include "../../src/wilderness/spatial_events.h"

#include <pthread.h>
#include <stdint.h>
#include <string.h>

#define TEST_EVENT_OUTER UINT32_C(0x2001)
#define TEST_EVENT_INNER UINT32_C(0x2002)

extern struct greyhawk_ship_data greyhawk_ships[GREYHAWK_MAXSHIPS];

struct test_payload
{
  int value;
};

struct test_clock
{
  uint64_t usec;
};

struct handler_fixture
{
  int *order;
  size_t *order_count;
  int before;
  int after;
  domain_event_type_id_t nested_type;
  struct test_payload nested_payload;
  enum domain_event_status nested_status;
  struct test_clock *clock;
  uint64_t advance_usec;
};

struct fake_entity
{
  uint64_t runtime_id;
  uint64_t generation;
  int mutation_count;
  bool live;
};

struct resolver_fixture
{
  struct fake_entity entity;
  int resolved;
  int stale;
};

struct destroy_fixture
{
  enum domain_event_status status;
};

static uint64_t test_usec_now(void *context)
{
  struct test_clock *clock = context;

  return clock->usec;
}

static struct domain_event_bus *create_bus(CuTest *tc, uint32_t max_depth,
                                           uint32_t max_causal_events,
                                           struct test_clock *clock, uint64_t slow_usec)
{
  struct domain_event_bus_config config;
  struct domain_event_bus *bus;
  enum domain_event_status status;

  memset(&config, 0, sizeof(config));
  config.max_event_types = 16U;
  config.max_handlers = 32U;
  config.max_depth = max_depth;
  config.max_causal_events = max_causal_events;
  config.slow_handler_usec = slow_usec;
  if (clock != NULL)
  {
    config.monotonic_usec_now = test_usec_now;
    config.clock_context = clock;
  }
  bus = domain_event_bus_create(&config, &status);
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, status);
  CuAssertPtrNotNull(tc, bus);
  return bus;
}

static void register_test_type(CuTest *tc, struct domain_event_bus *bus,
                               domain_event_type_id_t type, const char *name)
{
  struct domain_event_type_config config;

  config.type = type;
  config.name = name;
  config.payload_size = sizeof(struct test_payload);
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_register_type(bus, &config));
}

static void record_handler(const struct domain_event_context *context, void *handler_context)
{
  struct handler_fixture *fixture = handler_context;

  if (fixture->order != NULL)
    fixture->order[(*fixture->order_count)++] = fixture->before;
  if (fixture->clock != NULL)
    fixture->clock->usec += fixture->advance_usec;
  if (fixture->nested_type != 0)
    fixture->nested_status = DOMAIN_EVENT_PUBLISH(context->bus, fixture->nested_type,
                                                  &fixture->nested_payload);
  if (fixture->order != NULL && fixture->after != 0)
    fixture->order[(*fixture->order_count)++] = fixture->after;
}

static void register_handler(CuTest *tc, struct domain_event_bus *bus,
                             domain_event_type_id_t type, const char *identity, int priority,
                             domain_event_handler handler, void *context)
{
  struct domain_event_handler_config config;

  config.type = type;
  config.identity = identity;
  config.priority = priority;
  config.handler = handler;
  config.handler_context = context;
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_register_handler(bus, &config));
}

static void recursive_handler(const struct domain_event_context *context, void *handler_context)
{
  enum domain_event_status *last_status = handler_context;

  *last_status = domain_event_publish(context->bus, context->type, context->payload,
                                      context->payload_size);
}

static void *fake_entity_resolver(struct domain_entity_handle handle, void *resolver_context)
{
  struct resolver_fixture *fixture = resolver_context;

  if (!fixture->entity.live || handle.runtime_id != fixture->entity.runtime_id ||
      handle.generation != fixture->entity.generation)
    return NULL;
  return &fixture->entity;
}

static void extract_entity_handler(const struct domain_event_context *context,
                                   void *handler_context)
{
  const struct domain_entity_extracted *payload = context->payload;
  struct resolver_fixture *fixture = handler_context;
  struct fake_entity *entity;

  entity = domain_event_resolve(context->bus, payload->entity, DOMAIN_ENTITY_CHARACTER);
  if (entity != NULL)
  {
    fixture->resolved++;
    entity->mutation_count++;
    entity->live = false;
  }
}

static void observe_extracted_handler(const struct domain_event_context *context,
                                      void *handler_context)
{
  const struct domain_entity_extracted *payload = context->payload;
  struct resolver_fixture *fixture = handler_context;
  struct fake_entity *entity;

  entity = domain_event_resolve(context->bus, payload->entity, DOMAIN_ENTITY_CHARACTER);
  if (entity == NULL)
    fixture->stale++;
  else
    entity->mutation_count++;
}

static void destroy_during_handler(const struct domain_event_context *context,
                                   void *handler_context)
{
  struct destroy_fixture *fixture = handler_context;

  fixture->status = domain_event_bus_destroy(context->bus);
}

struct thread_fixture
{
  struct domain_event_bus *bus;
  struct test_payload payload;
  enum domain_event_status status;
};

static void *publish_from_thread(void *context)
{
  struct thread_fixture *fixture = context;

  fixture->status = DOMAIN_EVENT_PUBLISH(fixture->bus, TEST_EVENT_OUTER, &fixture->payload);
  return NULL;
}

void TestDomainEventRegistryContracts(CuTest *tc)
{
  struct domain_event_bus *bus = create_bus(tc, 4U, 16U, NULL, 100U);
  struct domain_event_type_config duplicate = {TEST_EVENT_OUTER, "OtherName",
                                                sizeof(struct test_payload)};
  struct domain_event_handler_config invalid_handler = {TEST_EVENT_INNER, "missing", 0,
                                                         record_handler, NULL};
  struct handler_fixture handler_fixture = {0};
  struct domain_event_handler_config handler = {TEST_EVENT_OUTER, "observer", 0,
                                                 record_handler, &handler_fixture};
  struct resolver_fixture resolver = {{1U, 1U, 0, true}, 0, 0};
  struct test_payload payload = {42};
  struct domain_event_bus_stats stats;

  register_test_type(tc, bus, TEST_EVENT_OUTER, "TestOuter");
  CuAssertIntEquals(tc, DOMAIN_EVENT_DUPLICATE_TYPE,
                    domain_event_register_type(bus, &duplicate));
  CuAssertIntEquals(tc, DOMAIN_EVENT_UNKNOWN_TYPE,
                    domain_event_register_handler(bus, &invalid_handler));
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_register_handler(bus, &handler));
  CuAssertIntEquals(tc, DOMAIN_EVENT_DUPLICATE_HANDLER,
                    domain_event_register_handler(bus, &handler));
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    domain_event_register_resolver(bus, DOMAIN_ENTITY_CHARACTER,
                                                   fake_entity_resolver, &resolver));
  CuAssertIntEquals(tc, DOMAIN_EVENT_DUPLICATE_RESOLVER,
                    domain_event_register_resolver(bus, DOMAIN_ENTITY_CHARACTER,
                                                   fake_entity_resolver, &resolver));
  CuAssertIntEquals(tc, DOMAIN_EVENT_REGISTRY_NOT_SEALED,
                    DOMAIN_EVENT_PUBLISH(bus, TEST_EVENT_OUTER, &payload));
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_seal(bus));
  CuAssertIntEquals(tc, DOMAIN_EVENT_REGISTRY_SEALED,
                    domain_event_register_type(bus, &duplicate));
  CuAssertIntEquals(tc, DOMAIN_EVENT_PAYLOAD_SIZE_MISMATCH,
                    domain_event_publish(bus, TEST_EVENT_OUTER, &payload, sizeof(payload) - 1U));
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    DOMAIN_EVENT_PUBLISH(bus, TEST_EVENT_OUTER, &payload));
  domain_event_bus_get_stats(bus, &stats);
  CuAssertIntEquals(tc, 1, (int)stats.registered_type_count);
  CuAssertIntEquals(tc, 1, (int)stats.publications);
  CuAssertIntEquals(tc, 1, (int)stats.handler_calls);
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_bus_destroy(bus));
}

void TestDomainEventInspectionEnumeratesTypesAndHandlers(CuTest *tc)
{
  struct domain_event_bus *bus = create_bus(tc, 4U, 16U, NULL, 100U);
  struct domain_event_type_stats types[2];
  struct domain_event_handler_stats handlers[2];
  struct handler_fixture fixture = {0};
  size_t total;

  register_test_type(tc, bus, TEST_EVENT_OUTER, "FirstObserved");
  register_test_type(tc, bus, TEST_EVENT_INNER, "SecondObserved");
  register_handler(tc, bus, TEST_EVENT_OUTER, "late-observer", 10, record_handler, &fixture);
  register_handler(tc, bus, TEST_EVENT_OUTER, "early-observer", -10, record_handler, &fixture);

  memset(types, 0, sizeof(types));
  total = domain_event_inspect_types(bus, types, 1U);
  CuAssertIntEquals(tc, 2, (int)total);
  CuAssertTrue(tc, types[0].type == TEST_EVENT_OUTER);
  CuAssertStrEquals(tc, "FirstObserved", types[0].name);
  total = domain_event_inspect_types(bus, types, 2U);
  CuAssertIntEquals(tc, 2, (int)total);
  CuAssertTrue(tc, types[1].type == TEST_EVENT_INNER);
  CuAssertStrEquals(tc, "SecondObserved", types[1].name);

  memset(handlers, 0, sizeof(handlers));
  total = domain_event_inspect_handlers(bus, TEST_EVENT_OUTER, handlers, 1U);
  CuAssertIntEquals(tc, 2, (int)total);
  CuAssertStrEquals(tc, "early-observer", handlers[0].identity);
  CuAssertIntEquals(tc, -10, handlers[0].priority);
  total = domain_event_inspect_handlers(bus, TEST_EVENT_OUTER, handlers, 2U);
  CuAssertIntEquals(tc, 2, (int)total);
  CuAssertStrEquals(tc, "early-observer", handlers[0].identity);
  CuAssertStrEquals(tc, "late-observer", handlers[1].identity);

  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_bus_destroy(bus));
}

void TestDomainEventFoundationContracts(CuTest *tc)
{
  struct domain_event_bus *bus = create_bus(tc, 4U, 16U, NULL, 100U);
  struct domain_event_type_stats stats;
  struct domain_event_bus_stats bus_stats;
  struct domain_character_moved moved;

  memset(&moved, 0, sizeof(moved));
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_register_foundation_types(bus));
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_seal(bus));
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    DOMAIN_EVENT_PUBLISH(bus, DOMAIN_EVENT_CHARACTER_MOVED, &moved));
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    domain_event_get_type_stats(bus, DOMAIN_EVENT_ACTIVITY_TRANSITIONED, &stats));
  domain_event_bus_get_stats(bus, &bus_stats);
  CuAssertIntEquals(tc, 9, (int)bus_stats.registered_type_count);
  CuAssertStrEquals(tc, "ActivityTransitioned", stats.name);
  CuAssertIntEquals(tc, (int)sizeof(struct domain_activity_transitioned),
                    (int)stats.payload_size);
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    domain_event_get_type_stats(bus, DOMAIN_EVENT_WORLD_PHENOMENON, &stats));
  CuAssertStrEquals(tc, "WorldPhenomenon", stats.name);
  CuAssertIntEquals(tc, (int)sizeof(struct domain_world_phenomenon), (int)stats.payload_size);
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_bus_destroy(bus));
}

void TestDomainEventRegistryCapacity(CuTest *tc)
{
  struct domain_event_bus_config bus_config;
  struct domain_event_type_config second_type = {TEST_EVENT_INNER, "Second",
                                                  sizeof(struct test_payload)};
  struct domain_event_handler_config second_handler;
  struct handler_fixture fixture = {0};
  struct domain_event_bus *bus;
  enum domain_event_status status;

  memset(&bus_config, 0, sizeof(bus_config));
  bus_config.max_event_types = 1U;
  bus_config.max_handlers = 1U;
  bus = domain_event_bus_create(&bus_config, &status);
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, status);
  CuAssertPtrNotNull(tc, bus);
  register_test_type(tc, bus, TEST_EVENT_OUTER, "First");
  CuAssertIntEquals(tc, DOMAIN_EVENT_TYPE_CAPACITY_REACHED,
                    domain_event_register_type(bus, &second_type));
  register_handler(tc, bus, TEST_EVENT_OUTER, "first", 0, record_handler, &fixture);
  second_handler.type = TEST_EVENT_OUTER;
  second_handler.identity = "second";
  second_handler.priority = 0;
  second_handler.handler = record_handler;
  second_handler.handler_context = &fixture;
  CuAssertIntEquals(tc, DOMAIN_EVENT_HANDLER_CAPACITY_REACHED,
                    domain_event_register_handler(bus, &second_handler));
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_bus_destroy(bus));
}

void TestDomainEventHandlerOrderAndNestedDepthFirst(CuTest *tc)
{
  struct domain_event_bus *bus = create_bus(tc, 8U, 32U, NULL, 100U);
  int order[8] = {0};
  size_t count = 0;
  struct handler_fixture outer_first = {order, &count, 1, 4, TEST_EVENT_INNER, {0},
                                        DOMAIN_EVENT_OK, NULL, 0};
  struct handler_fixture outer_second = {order, &count, 5, 0, 0, {0}, DOMAIN_EVENT_OK, NULL, 0};
  struct handler_fixture inner_first = {order, &count, 2, 0, 0, {0}, DOMAIN_EVENT_OK, NULL, 0};
  struct handler_fixture inner_second = {order, &count, 3, 0, 0, {0}, DOMAIN_EVENT_OK, NULL, 0};
  struct test_payload payload = {0};

  register_test_type(tc, bus, TEST_EVENT_OUTER, "TestOuter");
  register_test_type(tc, bus, TEST_EVENT_INNER, "TestInner");
  register_handler(tc, bus, TEST_EVENT_OUTER, "outer-second", 20, record_handler, &outer_second);
  register_handler(tc, bus, TEST_EVENT_OUTER, "outer-first", 10, record_handler, &outer_first);
  register_handler(tc, bus, TEST_EVENT_INNER, "inner-first", 0, record_handler, &inner_first);
  register_handler(tc, bus, TEST_EVENT_INNER, "inner-second", 0, record_handler, &inner_second);
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_seal(bus));
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, DOMAIN_EVENT_PUBLISH(bus, TEST_EVENT_OUTER, &payload));
  CuAssertIntEquals(tc, 5, (int)count);
  CuAssertIntEquals(tc, 1, order[0]);
  CuAssertIntEquals(tc, 2, order[1]);
  CuAssertIntEquals(tc, 3, order[2]);
  CuAssertIntEquals(tc, 4, order[3]);
  CuAssertIntEquals(tc, 5, order[4]);
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_bus_destroy(bus));
}

void TestDomainEventNestingLimitFailsClosed(CuTest *tc)
{
  struct domain_event_bus *bus = create_bus(tc, 2U, 32U, NULL, 100U);
  struct test_payload payload = {0};
  struct domain_event_bus_stats stats;
  struct domain_event_type_stats type_stats;
  enum domain_event_status nested_status = DOMAIN_EVENT_OK;

  register_test_type(tc, bus, TEST_EVENT_OUTER, "Recursive");
  register_handler(tc, bus, TEST_EVENT_OUTER, "recursive", 0, recursive_handler,
                   &nested_status);
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_seal(bus));
  CuAssertIntEquals(tc, DOMAIN_EVENT_NESTING_LIMIT_REACHED,
                    DOMAIN_EVENT_PUBLISH(bus, TEST_EVENT_OUTER, &payload));
  CuAssertIntEquals(tc, DOMAIN_EVENT_NESTING_LIMIT_REACHED, nested_status);
  domain_event_bus_get_stats(bus, &stats);
  domain_event_get_type_stats(bus, TEST_EVENT_OUTER, &type_stats);
  CuAssertIntEquals(tc, 2, (int)stats.publications);
  CuAssertIntEquals(tc, 2, (int)stats.maximum_depth);
  CuAssertIntEquals(tc, 1, (int)stats.rejected_causal_chains);
  CuAssertIntEquals(tc, 1, (int)type_stats.rejected_publications);
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_bus_destroy(bus));
}

void TestDomainEventCausalCountLimitFailsClosed(CuTest *tc)
{
  struct domain_event_bus *bus = create_bus(tc, 8U, 3U, NULL, 100U);
  struct test_payload payload = {0};
  struct domain_event_bus_stats stats;
  enum domain_event_status nested_status = DOMAIN_EVENT_OK;

  register_test_type(tc, bus, TEST_EVENT_OUTER, "Recursive");
  register_handler(tc, bus, TEST_EVENT_OUTER, "recursive", 0, recursive_handler,
                   &nested_status);
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_seal(bus));
  CuAssertIntEquals(tc, DOMAIN_EVENT_CAUSAL_LIMIT_REACHED,
                    DOMAIN_EVENT_PUBLISH(bus, TEST_EVENT_OUTER, &payload));
  domain_event_bus_get_stats(bus, &stats);
  CuAssertIntEquals(tc, 3, (int)stats.publications);
  CuAssertIntEquals(tc, 3, (int)stats.maximum_depth);
  CuAssertIntEquals(tc, 1, (int)stats.rejected_causal_chains);
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_bus_destroy(bus));
}

void TestDomainEventExtractionMakesLaterResolutionStale(CuTest *tc)
{
  struct domain_event_bus *bus = create_bus(tc, 4U, 16U, NULL, 100U);
  struct resolver_fixture fixture = {{77U, 9U, 0, true}, 0, 0};
  struct domain_entity_extracted payload = {{DOMAIN_ENTITY_CHARACTER, 77U, 9U}, 0U};

  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_register_foundation_types(bus));
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    domain_event_register_resolver(bus, DOMAIN_ENTITY_CHARACTER,
                                                   fake_entity_resolver, &fixture));
  register_handler(tc, bus, DOMAIN_EVENT_ENTITY_EXTRACTED, "extract", 0,
                   extract_entity_handler, &fixture);
  register_handler(tc, bus, DOMAIN_EVENT_ENTITY_EXTRACTED, "observe", 10,
                   observe_extracted_handler, &fixture);
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_seal(bus));
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    DOMAIN_EVENT_PUBLISH(bus, DOMAIN_EVENT_ENTITY_EXTRACTED, &payload));
  CuAssertIntEquals(tc, 1, fixture.resolved);
  CuAssertIntEquals(tc, 1, fixture.stale);
  CuAssertIntEquals(tc, 1, fixture.entity.mutation_count);
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_bus_destroy(bus));
}

void TestDomainEventGenerationReuseRejectsStaleHandle(CuTest *tc)
{
  struct domain_event_bus *bus = create_bus(tc, 4U, 16U, NULL, 100U);
  struct resolver_fixture fixture = {{77U, 10U, 0, true}, 0, 0};
  struct domain_entity_handle stale = {DOMAIN_ENTITY_CHARACTER, 77U, 9U};
  struct domain_entity_handle current = {DOMAIN_ENTITY_CHARACTER, 77U, 10U};

  register_test_type(tc, bus, TEST_EVENT_OUTER, "TestOuter");
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    domain_event_register_resolver(bus, DOMAIN_ENTITY_CHARACTER,
                                                   fake_entity_resolver, &fixture));
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_seal(bus));
  CuAssertPtrEquals(tc, NULL,
                    domain_event_resolve(bus, stale, DOMAIN_ENTITY_CHARACTER));
  CuAssertPtrEquals(tc, &fixture.entity,
                    domain_event_resolve(bus, current, DOMAIN_ENTITY_CHARACTER));
  CuAssertPtrEquals(tc, NULL, domain_event_resolve(bus, current, DOMAIN_ENTITY_OBJECT));
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_bus_destroy(bus));
}

void TestDomainWorldHandlesResolveWithoutPopulationScans(CuTest *tc)
{
  struct domain_event_bus *bus;
  struct domain_entity_handle character_handle;
  struct domain_entity_handle object_handle;
  struct char_data character;
  struct obj_data object;
  struct char_data *saved_characters = character_list;
  struct obj_data *saved_objects = object_list;

  clear_char(&character);
  clear_object(&object);
  character_list = NULL;
  object_list = NULL;
  domain_event_world_shutdown();
  bus = create_bus(tc, 4U, 16U, NULL, 100U);
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_register_foundation_types(bus));
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_world_register_resolvers(bus));

  character_handle = domain_event_character_handle(&character);
  object_handle = domain_event_object_handle(&object);
  CuAssertPtrEquals(tc, &character,
                    domain_event_resolve(bus, character_handle, DOMAIN_ENTITY_CHARACTER));
  CuAssertPtrEquals(tc, &object,
                    domain_event_resolve(bus, object_handle, DOMAIN_ENTITY_OBJECT));

  domain_event_world_forget_character(&character);
  domain_event_world_forget_object(&object);
  CuAssertPtrEquals(tc, NULL,
                    domain_event_resolve(bus, character_handle, DOMAIN_ENTITY_CHARACTER));
  CuAssertPtrEquals(tc, NULL,
                    domain_event_resolve(bus, object_handle, DOMAIN_ENTITY_OBJECT));
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_bus_destroy(bus));
  domain_event_world_shutdown();
  character_list = saved_characters;
  object_list = saved_objects;
}

void TestDomainEventSlowHandlerDiagnostics(CuTest *tc)
{
  struct test_clock clock = {100U};
  struct domain_event_bus *bus = create_bus(tc, 4U, 16U, &clock, 20U);
  struct handler_fixture fixture = {NULL, NULL, 0, 0, 0, {0}, DOMAIN_EVENT_OK, &clock, 25U};
  struct test_payload payload = {0};
  struct domain_event_handler_stats handler_stats;
  struct domain_event_type_stats type_stats;
  struct domain_event_bus_stats bus_stats;

  register_test_type(tc, bus, TEST_EVENT_OUTER, "Timed");
  register_handler(tc, bus, TEST_EVENT_OUTER, "slow-handler", 7, record_handler, &fixture);
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_seal(bus));
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, DOMAIN_EVENT_PUBLISH(bus, TEST_EVENT_OUTER, &payload));
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    domain_event_get_handler_stats(bus, TEST_EVENT_OUTER, "slow-handler",
                                                   &handler_stats));
  domain_event_get_type_stats(bus, TEST_EVENT_OUTER, &type_stats);
  domain_event_bus_get_stats(bus, &bus_stats);
  CuAssertIntEquals(tc, 25, (int)handler_stats.total_usec);
  CuAssertIntEquals(tc, 25, (int)handler_stats.maximum_usec);
  CuAssertIntEquals(tc, 1, (int)handler_stats.slow_calls);
  CuAssertIntEquals(tc, 1, (int)type_stats.slow_handler_calls);
  CuAssertIntEquals(tc, 1, (int)bus_stats.slow_handler_calls);
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_bus_destroy(bus));
}

void TestDomainEventLifecycleAndMainThreadGuards(CuTest *tc)
{
  struct domain_event_bus *bus = create_bus(tc, 4U, 16U, NULL, 100U);
  struct destroy_fixture destroy = {DOMAIN_EVENT_OK};
  struct test_payload payload = {0};
  struct thread_fixture thread_fixture = {bus, {0}, DOMAIN_EVENT_OK};
  pthread_t thread;

  register_test_type(tc, bus, TEST_EVENT_OUTER, "TestOuter");
  register_handler(tc, bus, TEST_EVENT_OUTER, "destroy", 0, destroy_during_handler, &destroy);
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_seal(bus));
  CuAssertIntEquals(tc, 0, pthread_create(&thread, NULL, publish_from_thread, &thread_fixture));
  CuAssertIntEquals(tc, 0, pthread_join(thread, NULL));
  CuAssertIntEquals(tc, DOMAIN_EVENT_WRONG_THREAD, thread_fixture.status);
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, DOMAIN_EVENT_PUBLISH(bus, TEST_EVENT_OUTER, &payload));
  CuAssertIntEquals(tc, DOMAIN_EVENT_BUSY, destroy.status);
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_bus_destroy(bus));
}

void TestDomainEventPayloadIsBorrowedAndUnchanged(CuTest *tc)
{
  struct domain_event_bus *bus = create_bus(tc, 4U, 16U, NULL, 100U);
  struct handler_fixture fixture = {0};
  struct test_payload payload = {1234};

  register_test_type(tc, bus, TEST_EVENT_OUTER, "Borrowed");
  register_handler(tc, bus, TEST_EVENT_OUTER, "observer", 0, record_handler, &fixture);
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_seal(bus));
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, DOMAIN_EVENT_PUBLISH(bus, TEST_EVENT_OUTER, &payload));
  CuAssertIntEquals(tc, 1234, payload.value);
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_bus_destroy(bus));
}

void TestDomainEventProductionRuntimeLifecycle(CuTest *tc)
{
  struct domain_event_bus_stats stats;
  struct char_data victim;

  clear_char(&victim);
  victim.player.name = "domain event death victim";
  victim.next = NULL;
  character_list = &victim;
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_shutdown());
  event_free_all();
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER));
  event_init();
  active_world_reset_for_test();
  active_world_select_for_test(true);
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_init());
  CuAssertPtrNotNull(tc, domain_event_runtime_bus());
  domain_event_bus_get_stats(domain_event_runtime_bus(), &stats);
  CuAssertIntEquals(tc, 9, (int)stats.registered_type_count);
  CuAssertIntEquals(tc, 15, (int)stats.registered_handler_count);
  CuAssertTrue(tc, stats.sealed);
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    domain_event_runtime_character_died(&victim, NULL));
  domain_event_bus_get_stats(domain_event_runtime_bus(), &stats);
  CuAssertIntEquals(tc, 1, (int)stats.publications);
  CuAssertIntEquals(tc, DOMAIN_EVENT_BUSY, domain_event_runtime_init());
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_shutdown());
  event_free_all();
  CuAssertPtrEquals(tc, NULL, domain_event_runtime_bus());
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_shutdown());
  active_world_reset_for_test();
  character_list = NULL;
}

static void active_world_prepare_character(struct char_data *ch, bool npc, room_rnum room)
{
  clear_char(ch);
  IN_ROOM(ch) = room;
  ch->player.name = npc ? "scheduled mobile" : "observing player";
  if (npc)
  {
    SET_BIT_AR(MOB_FLAGS(ch), MOB_ISNPC);
    GET_MOB_RNUM(ch) = 0;
    GET_CLASS(ch) = CLASS_WARRIOR;
  }
}

static void process_scheduler_pulses(unsigned long count)
{
  while (count-- > 0U)
  {
    pulse++;
    event_process();
  }
}

static game_tick_t native_event_remaining(CuTest *tc, struct event_runtime_handle handle)
{
  game_tick_t remaining = 0U;

  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, event_runtime_remaining(handle, &remaining));
  return remaining;
}

static bool native_event_type_is_registered(const char *name)
{
  struct game_scheduler_stats stats;
  game_event_type_id_t event_type;

  event_runtime_get_stats(&stats);
  for (event_type = 1U; event_type <= stats.registered_type_count; event_type++)
    if (event_runtime_type_name(event_type) != NULL &&
        !strcmp(event_runtime_type_name(event_type), name))
      return true;
  return false;
}

void TestActiveWorldSchedulesOnlyConcreteAutonomousWorkWithoutPlayers(CuTest *tc)
{
  struct room_data room;
  struct char_data idle;
  struct char_data wanderer;
  struct room_data *saved_world;
  struct char_data *saved_characters;
  room_rnum saved_top_of_world;
  mob_rnum saved_top_of_mobt;
  unsigned long saved_pulse;
  uint64_t callbacks_before;
  struct event_runtime_handle cancelled_handle;
  long first_delay;

  saved_world = world;
  saved_characters = character_list;
  saved_top_of_world = top_of_world;
  saved_top_of_mobt = top_of_mobt;
  saved_pulse = pulse;
  memset(&room, 0, sizeof(room));
  room.number = 100;
  active_world_prepare_character(&idle, true, 0);
  active_world_prepare_character(&wanderer, true, 0);
  idle.player_specials = &dummy_mob;
  wanderer.player_specials = &dummy_mob;
  idle.player.short_descr = (char *)"idle sentinel";
  wanderer.player.short_descr = (char *)"offscreen wanderer";
  SET_BIT_AR(MOB_FLAGS(&idle), MOB_SENTINEL);
  idle.next = &wanderer;
  idle.next_in_room = &wanderer;
  room.people = &idle;
  world = &room;
  top_of_world = 0;
  top_of_mobt = 0;
  character_list = &idle;

  event_free_all();
  active_world_reset_for_test();
  active_world_select_for_test(true);
  character_periodic_reset_for_test();
  character_periodic_select_for_test(false);
  point_update_periodic_reset_for_test();
  point_update_periodic_select_for_test(false);
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER));
  pulse = 100U;
  event_init();
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_init());
  active_world_begin_bootstrap();
  active_world_end_bootstrap();
  CuAssertIntEquals(tc, 1,
                    (int)active_world_mobile_count(ACTIVE_WORLD_MOBILE_ACTIVE));
  CuAssertIntEquals(tc, 1,
                    (int)active_world_mobile_reason_count(MOBILE_WORK_WANDER));
  CuAssertIntEquals(tc, 1, event_queue_depth());
  CuAssertTrue(tc, native_event_type_is_registered("mobile.autonomous.agenda"));
  CuAssertTrue(tc, event_runtime_handle_is_none(idle.active_world_event_handle));
  CuAssertTrue(tc, !event_runtime_handle_is_none(wanderer.active_world_event_handle));

  cancelled_handle = wanderer.active_world_event_handle;
  CuAssertIntEquals(tc, GAME_EVENT_CANCELLED, event_runtime_cancel(cancelled_handle));
  CuAssertTrue(tc, !event_runtime_handle_is_live(cancelled_handle));
  CuAssertTrue(tc, event_runtime_handle_is_none(wanderer.active_world_event_handle));
  CuAssertIntEquals(tc, ACTIVE_WORLD_MOBILE_DORMANT, wanderer.active_world_state);
  CuAssertIntEquals(tc, 0,
                    (int)active_world_mobile_count(ACTIVE_WORLD_MOBILE_ACTIVE));
  CuAssertIntEquals(tc, 0, event_queue_depth());
  active_world_sync_mobile(&wanderer);
  CuAssertIntEquals(tc, 1,
                    (int)active_world_mobile_count(ACTIVE_WORLD_MOBILE_ACTIVE));
  CuAssertIntEquals(tc, 1, event_queue_depth());

  callbacks_before = active_world_mobile_callbacks();
  first_delay = (long)native_event_remaining(tc, wanderer.active_world_event_handle);
  CuAssertTrue(tc, first_delay > 0L);
  process_scheduler_pulses((unsigned long)first_delay);
  CuAssertIntEquals(tc, (int)(callbacks_before + 1U),
                    (int)active_world_mobile_callbacks());
  CuAssertTrue(tc, !event_runtime_handle_is_none(wanderer.active_world_event_handle));

  FIGHTING(&wanderer) = &idle;
  active_world_sync_mobile(&wanderer);
  CuAssertIntEquals(tc, 0,
                    (int)active_world_mobile_count(ACTIVE_WORLD_MOBILE_ACTIVE));
  CuAssertIntEquals(tc, 0, event_queue_depth());
  FIGHTING(&wanderer) = NULL;
  active_world_sync_mobile(&wanderer);
  CuAssertIntEquals(tc, 1,
                    (int)active_world_mobile_reason_count(MOBILE_WORK_WANDER));
  CuAssertIntEquals(tc, 1, event_queue_depth());

  SET_BIT_AR(MOB_FLAGS(&wanderer), MOB_SENTINEL);
  active_world_sync_mobile(&wanderer);
  CuAssertIntEquals(tc, 0,
                    (int)active_world_mobile_count(ACTIVE_WORLD_MOBILE_ACTIVE));
  CuAssertIntEquals(tc, 0,
                    (int)active_world_mobile_reason_count(MOBILE_WORK_WANDER));
  CuAssertIntEquals(tc, 0, event_queue_depth());

  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_shutdown());
  event_free_all();
  active_world_reset_for_test();
  character_periodic_reset_for_test();
  point_update_periodic_reset_for_test();
  pulse = saved_pulse;
  world = saved_world;
  top_of_world = saved_top_of_world;
  top_of_mobt = saved_top_of_mobt;
  character_list = saved_characters;
}

void TestActiveWorldDormantPopulationDoesNotCreateScheduledWork(CuTest *tc)
{
  const size_t dormant_count = 512U;
  struct room_data room;
  struct char_data *mobiles;
  struct char_data *wanderer;
  struct room_data *saved_world = world;
  struct char_data *saved_characters = character_list;
  room_rnum saved_top_of_world = top_of_world;
  mob_rnum saved_top_of_mobt = top_of_mobt;
  unsigned long saved_pulse = pulse;
  uint64_t callbacks_before;
  long first_delay;
  size_t index;

  mobiles = calloc(dormant_count + 1U, sizeof(*mobiles));
  CuAssertPtrNotNull(tc, mobiles);
  memset(&room, 0, sizeof(room));
  room.number = 100;
  for (index = 0U; index <= dormant_count; index++)
  {
    active_world_prepare_character(&mobiles[index], true, 0);
    mobiles[index].player_specials = &dummy_mob;
    mobiles[index].player.short_descr =
        index == dormant_count ? (char *)"scheduled wanderer" : (char *)"dormant sentinel";
    if (index < dormant_count)
      SET_BIT_AR(MOB_FLAGS(&mobiles[index]), MOB_SENTINEL);
    if (index < dormant_count)
    {
      mobiles[index].next = &mobiles[index + 1U];
      mobiles[index].next_in_room = &mobiles[index + 1U];
    }
  }
  wanderer = &mobiles[dormant_count];
  room.people = mobiles;
  world = &room;
  top_of_world = 0;
  top_of_mobt = 0;
  character_list = mobiles;

  event_free_all();
  active_world_reset_for_test();
  active_world_select_for_test(true);
  character_periodic_reset_for_test();
  character_periodic_select_for_test(false);
  point_update_periodic_reset_for_test();
  point_update_periodic_select_for_test(false);
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER));
  pulse = 150U;
  event_init();
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_init());
  active_world_begin_bootstrap();
  active_world_end_bootstrap();

  CuAssertIntEquals(tc, 1,
                    (int)active_world_mobile_count(ACTIVE_WORLD_MOBILE_ACTIVE));
  CuAssertIntEquals(tc, 1,
                    (int)active_world_mobile_reason_count(MOBILE_WORK_WANDER));
  CuAssertIntEquals(tc, 1, event_queue_depth());
  for (index = 0U; index < dormant_count; index++)
    CuAssertTrue(tc, event_runtime_handle_is_none(mobiles[index].active_world_event_handle));
  CuAssertTrue(tc, !event_runtime_handle_is_none(wanderer->active_world_event_handle));

  callbacks_before = active_world_mobile_callbacks();
  first_delay = (long)native_event_remaining(tc, wanderer->active_world_event_handle);
  CuAssertTrue(tc, first_delay > 0L);
  process_scheduler_pulses((unsigned long)first_delay);
  CuAssertIntEquals(tc, (int)(callbacks_before + 1U),
                    (int)active_world_mobile_callbacks());
  CuAssertIntEquals(tc, 1, event_queue_depth());

  SET_BIT_AR(MOB_FLAGS(wanderer), MOB_SENTINEL);
  active_world_sync_mobile(wanderer);
  CuAssertIntEquals(tc, 0,
                    (int)active_world_mobile_count(ACTIVE_WORLD_MOBILE_ACTIVE));
  CuAssertIntEquals(tc, 0, event_queue_depth());

  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_shutdown());
  event_free_all();
  active_world_reset_for_test();
  character_periodic_reset_for_test();
  point_update_periodic_reset_for_test();
  free(mobiles);
  pulse = saved_pulse;
  world = saved_world;
  top_of_world = saved_top_of_world;
  top_of_mobt = saved_top_of_mobt;
  character_list = saved_characters;
}

void TestActiveWorldDemandDrivenAdmissionAndLegacyGateAreExclusive(CuTest *tc)
{
  struct room_data room;
  struct char_data first;
  struct char_data second;
  struct room_data *saved_world;
  struct char_data *saved_characters;
  room_rnum saved_top_of_world;
  mob_rnum saved_top_of_mobt;
  unsigned long saved_pulse;

  saved_world = world;
  saved_characters = character_list;
  saved_top_of_world = top_of_world;
  saved_top_of_mobt = top_of_mobt;
  saved_pulse = pulse;
  memset(&room, 0, sizeof(room));
  room.number = 100;
  active_world_prepare_character(&first, true, 0);
  active_world_prepare_character(&second, true, 0);
  first.player_specials = &dummy_mob;
  second.player_specials = &dummy_mob;
  first.next = &second;
  first.next_in_room = &second;
  room.people = &first;
  world = &room;
  top_of_world = 0;
  top_of_mobt = 0;
  character_list = &first;

  event_free_all();
  active_world_reset_for_test();
  active_world_select_for_test(true);
  active_world_set_admission_limit_for_test(1U);
  character_periodic_reset_for_test();
  character_periodic_select_for_test(false);
  point_update_periodic_reset_for_test();
  point_update_periodic_select_for_test(false);
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER));
  pulse = 200U;
  event_init();
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_init());
  active_world_begin_bootstrap();
  active_world_end_bootstrap();
  CuAssertIntEquals(tc, 1,
                    (int)active_world_mobile_count(ACTIVE_WORLD_MOBILE_ACTIVE));
  CuAssertIntEquals(tc, 1, (int)active_world_mobile_admission_rejections());
  CuAssertIntEquals(tc, 1, event_queue_depth());
  CuAssertTrue(tc, !event_runtime_handle_is_none(first.active_world_event_handle));
  CuAssertTrue(tc, event_runtime_handle_is_none(second.active_world_event_handle));
  active_world_reset_telemetry();
  CuAssertIntEquals(tc, 0, (int)active_world_mobile_admission_rejections());
  CuAssertIntEquals(tc, 0, (int)active_world_mobile_callbacks());
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_shutdown());
  event_free_all();

  active_world_reset_for_test();
  active_world_select_for_test(false);
  first.active_world_state = ACTIVE_WORLD_MOBILE_DORMANT;
  second.active_world_state = ACTIVE_WORLD_MOBILE_DORMANT;
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER));
  event_init();
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_init());
  CuAssertTrue(tc, !active_world_enabled());
  CuAssertIntEquals(tc, 0, event_queue_depth());
  CuAssertIntEquals(tc, 0,
                    (int)active_world_mobile_count(ACTIVE_WORLD_MOBILE_ACTIVE));
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_shutdown());
  event_free_all();
  active_world_reset_for_test();
  character_periodic_reset_for_test();
  point_update_periodic_reset_for_test();
  pulse = saved_pulse;
  world = saved_world;
  top_of_world = saved_top_of_world;
  top_of_mobt = saved_top_of_mobt;
  character_list = saved_characters;
}

void TestActiveWorldResourceRecoveryWakesAndRetiresOneOwner(CuTest *tc)
{
  struct room_data room;
  struct char_data mobile;
  struct char_data opponent;
  struct room_data *saved_world = world;
  struct char_data *saved_characters = character_list;
  room_rnum saved_top_of_world = top_of_world;
  mob_rnum saved_top_of_mobt = top_of_mobt;
  unsigned long saved_pulse = pulse;
  const int spellnum = SPELL_MAGIC_MISSILE;
  int spell_circle;

  memset(&room, 0, sizeof(room));
  room.number = 100;
  active_world_prepare_character(&mobile, true, 0);
  active_world_prepare_character(&opponent, true, 0);
  mobile.player_specials = &dummy_mob;
  opponent.player_specials = &dummy_mob;
  SET_BIT_AR(MOB_FLAGS(&mobile), MOB_SENTINEL);
  SET_BIT_AR(MOB_FLAGS(&opponent), MOB_SENTINEL);
  MOB_KNOWS_SPELL(&mobile, spellnum) = TRUE;
  mobile.mob_specials.known_spell_slots[spellnum] = 2;
  mobile.mob_specials.last_known_slot_regen = time(0);
  mobile.next = &opponent;
  mobile.next_in_room = &opponent;
  room.people = &mobile;
  world = &room;
  top_of_world = 0;
  top_of_mobt = 0;
  character_list = &mobile;

  event_free_all();
  active_world_reset_for_test();
  active_world_select_for_test(true);
  character_periodic_reset_for_test();
  character_periodic_select_for_test(false);
  point_update_periodic_reset_for_test();
  point_update_periodic_select_for_test(false);
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER));
  pulse = 250U;
  event_init();
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_init());
  active_world_begin_bootstrap();
  active_world_end_bootstrap();
  CuAssertIntEquals(tc, 0, event_queue_depth());

  consume_known_spell_slot(&mobile, spellnum);
  CuAssertIntEquals(tc, 1, mobile.mob_specials.known_spell_slots[spellnum]);
  CuAssertIntEquals(tc, 1,
                    (int)active_world_mobile_reason_count(MOBILE_WORK_RESOURCE_RECOVERY));
  CuAssertIntEquals(tc, 1, event_queue_depth());
  CuAssertIntEquals(tc, 60 * PASSES_PER_SEC,
                    (int)native_event_remaining(tc, mobile.active_world_event_handle));

  mobile.mob_specials.last_known_slot_regen = time(0) - 60;
  FIGHTING(&mobile) = &opponent;
  process_scheduler_pulses(60U * PASSES_PER_SEC);
  CuAssertIntEquals(tc, 1, mobile.mob_specials.known_spell_slots[spellnum]);
  CuAssertIntEquals(tc, PULSE_MOBILE,
                    (int)native_event_remaining(tc, mobile.active_world_event_handle));

  FIGHTING(&mobile) = NULL;
  process_scheduler_pulses(PULSE_MOBILE);
  CuAssertIntEquals(tc, 2, mobile.mob_specials.known_spell_slots[spellnum]);
  CuAssertIntEquals(tc, 0,
                    (int)active_world_mobile_reason_count(MOBILE_WORK_RESOURCE_RECOVERY));
  CuAssertIntEquals(tc, 0, event_queue_depth());

  GET_CLASS(&mobile) = CLASS_WIZARD;
  spell_circle = get_spell_circle(spellnum, GET_CLASS(&mobile));
  CuAssertTrue(tc, spell_circle >= 0 && spell_circle <= 9);
  mobile.mob_specials.max_spell_slots[spell_circle] = 1;
  mobile.mob_specials.spell_slots[spell_circle] = 1;
  consume_spell_slot(&mobile, spellnum);
  CuAssertIntEquals(tc, 0, mobile.mob_specials.spell_slots[spell_circle]);
  CuAssertIntEquals(tc, 1,
                    (int)active_world_mobile_reason_count(MOBILE_WORK_RESOURCE_RECOVERY));
  CuAssertIntEquals(tc, 300 * PASSES_PER_SEC,
                    (int)native_event_remaining(tc, mobile.active_world_event_handle));
  mobile.mob_specials.last_slot_regen = time(0) - 300;
  process_scheduler_pulses(300U * PASSES_PER_SEC);
  CuAssertIntEquals(tc, 1, mobile.mob_specials.spell_slots[spell_circle]);
  CuAssertIntEquals(tc, 0,
                    (int)active_world_mobile_reason_count(MOBILE_WORK_RESOURCE_RECOVERY));
  CuAssertIntEquals(tc, 0, event_queue_depth());

  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_shutdown());
  event_free_all();
  active_world_reset_for_test();
  character_periodic_reset_for_test();
  point_update_periodic_reset_for_test();
  pulse = saved_pulse;
  world = saved_world;
  top_of_world = saved_top_of_world;
  top_of_mobt = saved_top_of_mobt;
  character_list = saved_characters;
}

void TestActiveWorldReactionsAndScavengingAreDemandDriven(CuTest *tc)
{
  struct room_data room;
  struct char_data player;
  struct player_special_data player_specials;
  struct char_data aggressive;
  struct char_data scavenger;
  struct obj_data object;
  struct room_data *saved_world = world;
  struct char_data *saved_characters = character_list;
  struct obj_data *saved_objects = object_list;
  room_rnum saved_top_of_world = top_of_world;
  mob_rnum saved_top_of_mobt = top_of_mobt;
  unsigned long saved_pulse = pulse;

  memset(&room, 0, sizeof(room));
  memset(&player_specials, 0, sizeof(player_specials));
  room.number = 100;
  room.sector_type = SECT_INSIDE;
  active_world_prepare_character(&player, false, 0);
  player.player_specials = &player_specials;
  player.player.name = (char *)"observer";
  SET_BIT_AR(PRF_FLAGS(&player), PRF_NOHASSLE);
  active_world_prepare_character(&aggressive, true, 0);
  aggressive.player_specials = &dummy_mob;
  aggressive.player.short_descr = (char *)"reactive sentinel";
  SET_BIT_AR(MOB_FLAGS(&aggressive), MOB_SENTINEL);
  SET_BIT_AR(MOB_FLAGS(&aggressive), MOB_AGGRESSIVE);
  active_world_prepare_character(&scavenger, true, 0);
  scavenger.player_specials = &dummy_mob;
  scavenger.player.short_descr = (char *)"reactive scavenger";
  SET_BIT_AR(MOB_FLAGS(&scavenger), MOB_SENTINEL);
  SET_BIT_AR(MOB_FLAGS(&scavenger), MOB_SCAVENGER);
  clear_object(&object);
  object.name = (char *)"test treasure";
  object.short_description = (char *)"test treasure";
  GET_OBJ_COST(&object) = 100;
  SET_BIT_AR(GET_OBJ_WEAR(&object), ITEM_WEAR_TAKE);
  SET_BIT_AR(GET_OBJ_EXTRA(&object), ITEM_GLOW);
  player.next = &aggressive;
  aggressive.next = &scavenger;
  player.next_in_room = &aggressive;
  aggressive.next_in_room = &scavenger;
  room.people = &player;
  world = &room;
  top_of_world = 0;
  top_of_mobt = 0;
  character_list = &player;
  object_list = &object;

  event_free_all();
  active_world_reset_for_test();
  active_world_select_for_test(true);
  character_periodic_reset_for_test();
  character_periodic_select_for_test(false);
  point_update_periodic_reset_for_test();
  point_update_periodic_select_for_test(false);
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER));
  pulse = 300U;
  event_init();
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_init());
  CuAssertIntEquals(tc, 0,
                    (int)active_world_mobile_count(ACTIVE_WORLD_MOBILE_ACTIVE));
  CuAssertIntEquals(tc, 0, event_queue_depth());

  active_world_begin_bootstrap();
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    domain_event_runtime_character_moved(&player, NOWHERE, 0, -1));
  CuAssertIntEquals(tc, 0,
                    (int)active_world_mobile_reason_count(MOBILE_WORK_ROOM_REACTION));
  CuAssertIntEquals(tc, 0, event_queue_depth());
  active_world_end_bootstrap();
  CuAssertIntEquals(tc, 0, event_queue_depth());

  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    domain_event_runtime_character_moved(&player, NOWHERE, 0, -1));
  CuAssertIntEquals(tc, 1,
                    (int)active_world_mobile_count(ACTIVE_WORLD_MOBILE_ACTIVE));
  CuAssertIntEquals(tc, 1,
                    (int)active_world_mobile_reason_count(MOBILE_WORK_ROOM_REACTION));
  CuAssertIntEquals(tc, 1, event_queue_depth());
  process_scheduler_pulses(1U);
  CuAssertIntEquals(tc, 0,
                    (int)active_world_mobile_count(ACTIVE_WORLD_MOBILE_ACTIVE));
  CuAssertIntEquals(tc, 0, event_queue_depth());

  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    domain_event_runtime_character_moved(&scavenger, 0, 0, -1));
  CuAssertIntEquals(tc, 0,
                    (int)active_world_mobile_reason_count(MOBILE_WORK_ROOM_REACTION));
  CuAssertIntEquals(tc, 0, event_queue_depth());

  room.contents = &object;
  IN_ROOM(&object) = 0;
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    domain_event_runtime_object_moved(&object, NOWHERE, 0));
  CuAssertIntEquals(tc, 1,
                    (int)active_world_mobile_reason_count(MOBILE_WORK_SCAVENGE));
  CuAssertIntEquals(tc, 1, event_queue_depth());
  room.contents = NULL;
  IN_ROOM(&object) = NOWHERE;
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    domain_event_runtime_object_moved(&object, 0, NOWHERE));
  CuAssertIntEquals(tc, 0,
                    (int)active_world_mobile_count(ACTIVE_WORLD_MOBILE_ACTIVE));
  CuAssertIntEquals(tc, 0,
                    (int)active_world_mobile_reason_count(MOBILE_WORK_SCAVENGE));
  CuAssertIntEquals(tc, 0, event_queue_depth());

  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_shutdown());
  event_free_all();
  active_world_reset_for_test();
  character_periodic_reset_for_test();
  point_update_periodic_reset_for_test();
  pulse = saved_pulse;
  world = saved_world;
  top_of_world = saved_top_of_world;
  top_of_mobt = saved_top_of_mobt;
  character_list = saved_characters;
  object_list = saved_objects;
}

void TestIdleNpcPeriodicWorkIsSeparateFromAutonomousAgenda(CuTest *tc)
{
  struct room_data room;
  struct char_data mobile;
  struct room_data *saved_world = world;
  struct char_data *saved_characters = character_list;
  room_rnum saved_top_of_world = top_of_world;
  mob_rnum saved_top_of_mobt = top_of_mobt;
  unsigned long saved_pulse = pulse;

  memset(&room, 0, sizeof(room));
  room.number = 100;
  active_world_prepare_character(&mobile, true, 0);
  mobile.player_specials = &dummy_mob;
  mobile.player.short_descr = (char *)"idle periodic mobile";
  SET_BIT_AR(MOB_FLAGS(&mobile), MOB_SENTINEL);
  GET_HIT(&mobile) = GET_REAL_MAX_HIT(&mobile) = GET_MAX_HIT(&mobile) = 100;
  GET_MOVE(&mobile) = GET_REAL_MAX_MOVE(&mobile) = GET_MAX_MOVE(&mobile) = 100;
  GET_PSP(&mobile) = GET_REAL_MAX_PSP(&mobile) = GET_MAX_PSP(&mobile) = 100;
  room.people = &mobile;
  world = &room;
  top_of_world = 0;
  top_of_mobt = 0;
  character_list = &mobile;

  event_free_all();
  active_world_reset_for_test();
  active_world_select_for_test(true);
  character_periodic_reset_for_test();
  character_periodic_select_for_test(true);
  point_update_periodic_reset_for_test();
  point_update_periodic_select_for_test(false);
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER));
  pulse = 400U;
  event_init();
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_init());
  CuAssertIntEquals(tc, 0, (int)character_periodic_owner_count());
  CuAssertIntEquals(tc, 0,
                    (int)active_world_mobile_count(ACTIVE_WORLD_MOBILE_ACTIVE));
  CuAssertIntEquals(tc, 0, event_queue_depth());

  GET_HIT(&mobile) = 50;
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    domain_event_runtime_character_damaged(&mobile, NULL, 50, 0));
  CuAssertIntEquals(tc, 1, (int)character_periodic_owner_count());
  CuAssertIntEquals(tc, 1, (int)character_periodic_scheduled_count());
  CuAssertIntEquals(tc, 0,
                    (int)active_world_mobile_count(ACTIVE_WORLD_MOBILE_ACTIVE));

  GET_HIT(&mobile) = GET_MAX_HIT(&mobile);
  character_periodic_sync(&mobile);
  CuAssertIntEquals(tc, 0, (int)character_periodic_owner_count());
  CuAssertIntEquals(tc, 0, event_queue_depth());

  REMOVE_BIT_AR(MOB_FLAGS(&mobile), MOB_SENTINEL);
  active_world_sync_mobile(&mobile);
  CuAssertIntEquals(tc, 0, (int)character_periodic_owner_count());
  CuAssertIntEquals(tc, 1,
                    (int)active_world_mobile_reason_count(MOBILE_WORK_WANDER));
  CuAssertIntEquals(tc, 1, event_queue_depth());

  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_shutdown());
  event_free_all();
  active_world_reset_for_test();
  character_periodic_reset_for_test();
  point_update_periodic_reset_for_test();
  pulse = saved_pulse;
  world = saved_world;
  top_of_world = saved_top_of_world;
  top_of_mobt = saved_top_of_mobt;
  character_list = saved_characters;
}

void TestPeriodicOwnersScheduleEveryEligibleOwnerAndCancelLifecycle(CuTest *tc)
{
  struct room_data room;
  struct room_data *saved_world = world;
  struct char_data mobile;
  struct char_data *saved_characters = character_list;
  struct obj_data *obj;
  struct obj_data *saved_objects = object_list;
  struct game_event_snapshot snapshot;
  struct game_scheduler_stats scheduler_stats;
  struct script_data mobile_script;
  struct script_data object_script;
  struct script_data room_script;
  room_rnum saved_top_of_world = top_of_world;
  unsigned long saved_pulse = pulse;

  memset(&room, 0, sizeof(room));
  memset(&mobile_script, 0, sizeof(mobile_script));
  memset(&object_script, 0, sizeof(object_script));
  memset(&room_script, 0, sizeof(room_script));
  clear_char(&mobile);
  room.number = 100;
  world = &room;
  top_of_world = 0;
  IN_ROOM(&mobile) = 0;
  SCRIPT(&mobile) = &mobile_script;
  mobile_script.types = MTRIG_RANDOM | MTRIG_GLOBAL;
  character_list = &mobile;

  event_free_all();
  periodic_owners_reset_for_test();
  autoproc_registry_reset_for_test();
  dg_random_registry_reset_for_test();
  periodic_owners_select_for_test(true, true);
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER));
  pulse = 1000U;
  event_init();
  periodic_owners_init();
  event_runtime_get_stats(&scheduler_stats);
  CuAssertIntEquals(tc, 3, (int)scheduler_stats.registered_type_count);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, event_runtime_seal_types());

  obj = create_obj();
  IN_ROOM(obj) = 0;
  GET_OBJ_TYPE(obj) = ITEM_WEAPON;
  GET_OBJ_VAL(obj, 0) = 0;
  SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_AUTOPROC);
  SCRIPT(obj) = &object_script;
  object_script.types = OTRIG_RANDOM;
  SCRIPT(&room) = &room_script;
  room_script.types = WTRIG_RANDOM | WTRIG_GLOBAL;
  autoproc_registry_sync(obj);
  dg_script_bind_owner(&mobile_script, &mobile, MOB_TRIGGER);
  dg_script_bind_owner(&object_script, obj, OBJ_TRIGGER);
  dg_script_bind_owner(&room_script, &room, WLD_TRIGGER);

  CuAssertTrue(tc, periodic_autoproc_enabled());
  CuAssertTrue(tc, periodic_dg_random_enabled());
  CuAssertIntEquals(tc, 1, (int)periodic_autoproc_scheduled_count());
  CuAssertIntEquals(tc, 1, (int)periodic_dg_random_scheduled_count(MOB_TRIGGER));
  CuAssertIntEquals(tc, 1, (int)periodic_dg_random_scheduled_count(OBJ_TRIGGER));
  CuAssertIntEquals(tc, 1, (int)periodic_dg_random_scheduled_count(WLD_TRIGGER));
  CuAssertIntEquals(tc, 4, event_queue_depth());
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    event_runtime_inspect(obj->autoproc_event_handle, &snapshot));
  CuAssertStrEquals(tc, "object.automatic_procedure",
                    event_runtime_type_name(snapshot.event_type));
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    event_runtime_inspect(mobile_script.random_event_handle, &snapshot));
  CuAssertStrEquals(tc, "dg.random_trigger", event_runtime_type_name(snapshot.event_type));
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    event_runtime_inspect(object_script.random_event_handle, &snapshot));
  CuAssertStrEquals(tc, "dg.random_trigger", event_runtime_type_name(snapshot.event_type));
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    event_runtime_inspect(room_script.random_event_handle, &snapshot));
  CuAssertStrEquals(tc, "dg.random_trigger", event_runtime_type_name(snapshot.event_type));

  pulse += PULSE_DG_SCRIPT;
  event_process();
  CuAssertIntEquals(tc, 1, (int)periodic_autoproc_callbacks());
  CuAssertIntEquals(tc, 1, (int)periodic_dg_random_callbacks(MOB_TRIGGER));
  CuAssertIntEquals(tc, 1, (int)periodic_dg_random_callbacks(OBJ_TRIGGER));
  CuAssertIntEquals(tc, 1, (int)periodic_dg_random_callbacks(WLD_TRIGGER));
  CuAssertIntEquals(tc, 1, (int)periodic_dg_random_executions(MOB_TRIGGER));
  CuAssertIntEquals(tc, 1, (int)periodic_dg_random_executions(OBJ_TRIGGER));
  CuAssertIntEquals(tc, 1, (int)periodic_dg_random_executions(WLD_TRIGGER));
  CuAssertIntEquals(tc, 4, event_queue_depth());

  REMOVE_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_AUTOPROC);
  autoproc_registry_sync(obj);
  mobile_script.owner = NULL;
  object_script.owner = NULL;
  room_script.owner = NULL;
  dg_random_registry_sync(&mobile_script);
  dg_random_registry_sync(&object_script);
  dg_random_registry_sync(&room_script);
  CuAssertIntEquals(tc, 0, event_queue_depth());
  CuAssertTrue(tc, event_runtime_handle_is_none(obj->autoproc_event_handle));
  CuAssertTrue(tc, event_runtime_handle_is_none(mobile_script.random_event_handle));
  CuAssertTrue(tc, event_runtime_handle_is_none(object_script.random_event_handle));
  CuAssertTrue(tc, event_runtime_handle_is_none(room_script.random_event_handle));

  SCRIPT(&mobile) = NULL;
  SCRIPT(obj) = NULL;
  SCRIPT(&room) = NULL;
  periodic_owners_shutdown();
  event_free_all();
  autoproc_registry_reset_for_test();
  dg_random_registry_reset_for_test();
  object_list = saved_objects;
  free(obj);
  periodic_owners_reset_for_test();
  pulse = saved_pulse;
  world = saved_world;
  top_of_world = saved_top_of_world;
  character_list = saved_characters;
}

void TestPeriodicOwnerAdmissionAndRollbackGatesAreIndependent(CuTest *tc)
{
  struct obj_data first;
  struct obj_data second;
  struct script_data first_script;
  struct script_data second_script;
  unsigned long saved_pulse = pulse;

  memset(&first, 0, sizeof(first));
  memset(&second, 0, sizeof(second));
  memset(&first_script, 0, sizeof(first_script));
  memset(&second_script, 0, sizeof(second_script));
  IN_ROOM(&first) = NOWHERE;
  IN_ROOM(&second) = NOWHERE;
  SET_BIT_AR(GET_OBJ_EXTRA(&first), ITEM_AUTOPROC);
  SET_BIT_AR(GET_OBJ_EXTRA(&second), ITEM_AUTOPROC);
  SCRIPT(&first) = &first_script;
  SCRIPT(&second) = &second_script;
  first_script.types = OTRIG_RANDOM;
  second_script.types = OTRIG_RANDOM;

  event_free_all();
  periodic_owners_reset_for_test();
  autoproc_registry_reset_for_test();
  dg_random_registry_reset_for_test();
  periodic_owners_select_for_test(true, true);
  periodic_owners_set_limits_for_test(1U, 1U);
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER));
  pulse = 2000U;
  event_init();
  periodic_owners_init();
  autoproc_registry_sync(&first);
  autoproc_registry_sync(&second);
  dg_script_bind_owner(&first_script, &first, OBJ_TRIGGER);
  dg_script_bind_owner(&second_script, &second, OBJ_TRIGGER);
  CuAssertIntEquals(tc, 1, (int)periodic_autoproc_scheduled_count());
  CuAssertIntEquals(tc, 1, (int)periodic_dg_random_scheduled_count(OBJ_TRIGGER));
  CuAssertIntEquals(tc, 1, (int)periodic_autoproc_admission_rejections());
  CuAssertIntEquals(tc, 1, (int)periodic_dg_random_admission_rejections());
  CuAssertIntEquals(tc, 2, event_queue_depth());

  autoproc_registry_remove(&first);
  CuAssertIntEquals(tc, 1, event_queue_depth());
  first_script.owner = NULL;
  pulse += PULSE_DG_SCRIPT;
  event_process();
  CuAssertTrue(tc, event_runtime_handle_is_none(first_script.random_event_handle));
  CuAssertIntEquals(tc, 0, (int)periodic_dg_random_scheduled_count(OBJ_TRIGGER));
  CuAssertIntEquals(tc, 0, event_queue_depth());

  autoproc_registry_remove(&first);
  autoproc_registry_remove(&second);
  second_script.owner = NULL;
  dg_random_registry_sync(&first_script);
  dg_random_registry_sync(&second_script);
  SCRIPT(&first) = NULL;
  SCRIPT(&second) = NULL;
  periodic_owners_shutdown();
  event_free_all();
  autoproc_registry_reset_for_test();
  dg_random_registry_reset_for_test();

  periodic_owners_reset_for_test();
  periodic_owners_select_for_test(true, true);
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_LEGACY_QUEUE));
  event_init();
  periodic_owners_init();
  SET_BIT_AR(GET_OBJ_EXTRA(&first), ITEM_AUTOPROC);
  first_script.types = OTRIG_RANDOM;
  SCRIPT(&first) = &first_script;
  autoproc_registry_sync(&first);
  dg_script_bind_owner(&first_script, &first, OBJ_TRIGGER);
  CuAssertTrue(tc, !periodic_autoproc_enabled());
  CuAssertTrue(tc, !periodic_dg_random_enabled());
  CuAssertIntEquals(tc, 0, event_queue_depth());

  autoproc_registry_remove(&first);
  first_script.owner = NULL;
  dg_random_registry_sync(&first_script);
  SCRIPT(&first) = NULL;
  periodic_owners_shutdown();
  event_free_all();
  autoproc_registry_reset_for_test();
  dg_random_registry_reset_for_test();
  periodic_owners_reset_for_test();
  pulse = saved_pulse;
}

void TestDgTimeTriggersDispatchOnlyRegisteredOwners(CuTest *tc)
{
  struct char_data mobile;
  struct char_data unrelated_mobile;
  struct char_data *saved_characters = character_list;
  struct obj_data object;
  struct obj_data unrelated_object;
  struct obj_data *saved_objects = object_list;
  struct room_data rooms[2];
  struct room_data moved_rooms[3];
  struct room_data *saved_world = world;
  struct script_data mobile_script;
  struct script_data object_script;
  struct script_data room_script;
  room_rnum saved_top_of_world = top_of_world;

  clear_char(&mobile);
  clear_char(&unrelated_mobile);
  memset(&object, 0, sizeof(object));
  memset(&unrelated_object, 0, sizeof(unrelated_object));
  memset(rooms, 0, sizeof(rooms));
  memset(moved_rooms, 0, sizeof(moved_rooms));
  memset(&mobile_script, 0, sizeof(mobile_script));
  memset(&object_script, 0, sizeof(object_script));
  memset(&room_script, 0, sizeof(room_script));
  mobile.player_specials = &dummy_mob;
  unrelated_mobile.player_specials = &dummy_mob;
  SET_BIT_AR(MOB_FLAGS(&mobile), MOB_ISNPC);
  SET_BIT_AR(MOB_FLAGS(&unrelated_mobile), MOB_ISNPC);
  IN_ROOM(&mobile) = 0;
  IN_ROOM(&unrelated_mobile) = 1;
  mobile.next = &unrelated_mobile;
  object.next = &unrelated_object;
  rooms[0].number = 100;
  rooms[1].number = 200;
  rooms[0].zone = 0;
  rooms[1].zone = 0;
  mobile_script.types = MTRIG_TIME | MTRIG_GLOBAL;
  object_script.types = OTRIG_TIME;
  room_script.types = WTRIG_TIME | WTRIG_GLOBAL;
  SCRIPT(&mobile) = &mobile_script;
  SCRIPT(&object) = &object_script;
  SCRIPT(&rooms[0]) = &room_script;
  character_list = &mobile;
  object_list = &object;
  world = rooms;
  top_of_world = 1;

  dg_time_registry_reset_for_test();
  dg_script_bind_owner(&mobile_script, &mobile, MOB_TRIGGER);
  dg_script_bind_owner(&object_script, &object, OBJ_TRIGGER);
  dg_script_bind_owner(&room_script, &rooms[0], WLD_TRIGGER);

  CuAssertIntEquals(tc, 1, (int)dg_time_registry_count(MOB_TRIGGER));
  CuAssertIntEquals(tc, 1, (int)dg_time_registry_count(OBJ_TRIGGER));
  CuAssertIntEquals(tc, 1, (int)dg_time_registry_count(WLD_TRIGGER));
  CuAssertIntEquals(tc, 0, (int)dg_time_registry_validate(MOB_TRIGGER));
  CuAssertIntEquals(tc, 0, (int)dg_time_registry_validate(OBJ_TRIGGER));
  CuAssertIntEquals(tc, 0, (int)dg_time_registry_validate(WLD_TRIGGER));

  moved_rooms[0].number = 50;
  moved_rooms[1] = rooms[0];
  moved_rooms[2] = rooms[1];
  world = moved_rooms;
  top_of_world = 2;
  IN_ROOM(&mobile) = 1;
  IN_ROOM(&unrelated_mobile) = 2;
  check_time_triggers();
  CuAssertIntEquals(tc, 1, (int)dg_time_registry_visited(MOB_TRIGGER));
  CuAssertIntEquals(tc, 1, (int)dg_time_registry_visited(OBJ_TRIGGER));
  CuAssertIntEquals(tc, 1, (int)dg_time_registry_visited(WLD_TRIGGER));
  CuAssertIntEquals(tc, 1, (int)dg_time_registry_executed(MOB_TRIGGER));
  CuAssertIntEquals(tc, 1, (int)dg_time_registry_executed(OBJ_TRIGGER));
  CuAssertIntEquals(tc, 1, (int)dg_time_registry_executed(WLD_TRIGGER));
  CuAssertIntEquals(tc, 0, (int)dg_time_registry_validate(WLD_TRIGGER));

  dg_time_registry_reset_for_test();
  SCRIPT(&mobile) = NULL;
  SCRIPT(&object) = NULL;
  SCRIPT(&moved_rooms[1]) = NULL;
  character_list = saved_characters;
  object_list = saved_objects;
  world = saved_world;
  top_of_world = saved_top_of_world;
}

void TestTrailCleanupVisitsOnlyActiveRooms(CuTest *tc)
{
  struct room_data rooms[3];
  struct room_data moved_rooms[4];
  struct room_data *saved_world = world;
  const struct trail_data_list *trails;
  room_rnum saved_top_of_world = top_of_world;
  time_t old_age = time(NULL) - TRAIL_PRUNING_THRESHOLD - 1;

  memset(rooms, 0, sizeof(rooms));
  memset(moved_rooms, 0, sizeof(moved_rooms));
  rooms[0].number = 100;
  rooms[1].number = 200;
  rooms[2].number = 300;
  world = rooms;
  top_of_world = 2;

  movement_trail_registry_shutdown();
  movement_trail_record_at_room(&rooms[2], "old trail", "human", DIR_NONE, NORTH, old_age);
  CuAssertIntEquals(tc, 1, (int)movement_trail_active_location_count());
  CuAssertIntEquals(tc, 0, (int)movement_trail_registry_validate());

  moved_rooms[0].number = 50;
  moved_rooms[1] = rooms[0];
  moved_rooms[2] = rooms[1];
  moved_rooms[3] = rooms[2];
  world = moved_rooms;
  top_of_world = 3;
  trails = movement_trails_at_room(&moved_rooms[3]);
  CuAssertPtrNotNull(tc, trails);
  CuAssertPtrNotNull(tc, trails->head);
  cleanup_all_trails();
  CuAssertIntEquals(tc, 1, (int)movement_trail_cleanup_runs());
  CuAssertIntEquals(tc, 1, (int)movement_trail_last_cleanup_locations_visited());
  CuAssertIntEquals(tc, 1, (int)movement_trail_locations_visited());
  CuAssertIntEquals(tc, 1, (int)movement_trail_entries_removed());
  CuAssertIntEquals(tc, 0, (int)movement_trail_active_location_count());
  CuAssertIntEquals(tc, 0, (int)movement_trail_registry_validate());
  CuAssertPtrEquals(tc, NULL, movement_trails_at_room(&moved_rooms[3]));

  movement_trail_registry_shutdown();
  world = saved_world;
  top_of_world = saved_top_of_world;
}

void TestWildernessTrailIdentitySurvivesDynamicRoomReuse(CuTest *tc)
{
  struct room_data dynamic_slot;
  struct room_data same_place;
  const struct trail_data_list *trails;
  time_t now = time(NULL);

  memset(&dynamic_slot, 0, sizeof(dynamic_slot));
  memset(&same_place, 0, sizeof(same_place));
  dynamic_slot.number = WILD_DYNAMIC_ROOM_VNUM_START;
  dynamic_slot.zone = NOWHERE;
  dynamic_slot.coords[X_COORD] = 117;
  dynamic_slot.coords[Y_COORD] = -42;

  movement_trail_registry_shutdown();
  movement_trail_record_at_room(&dynamic_slot, "wilderness walker", "human", DIR_NONE, EAST,
                                now);
  CuAssertIntEquals(tc, 1, (int)movement_trail_active_location_count());
  trails = movement_trails_at_room(&dynamic_slot);
  CuAssertPtrNotNull(tc, trails);
  CuAssertStrEquals(tc, "wilderness walker", trails->head->name);

  dynamic_slot.coords[X_COORD] = 900;
  dynamic_slot.coords[Y_COORD] = 901;
  CuAssertPtrEquals(tc, NULL, movement_trails_at_room(&dynamic_slot));

  same_place.number = WILD_DYNAMIC_ROOM_VNUM_START + 1;
  same_place.zone = NOWHERE;
  same_place.coords[X_COORD] = 117;
  same_place.coords[Y_COORD] = -42;
  trails = movement_trails_at_room(&same_place);
  CuAssertPtrNotNull(tc, trails);
  CuAssertStrEquals(tc, "wilderness walker", trails->head->name);
  CuAssertIntEquals(tc, EAST, trails->head->to);
  CuAssertIntEquals(tc, 0, (int)movement_trail_registry_validate());

  movement_trail_registry_shutdown();
}

void TestWildernessTrailsUseRoomIdentityWithoutExplicitCoordinates(CuTest *tc)
{
  struct room_data rooms[2];
  struct zone_data wilderness_zone;
  struct zone_data *saved_zone_table = zone_table;
  zone_rnum saved_top_of_zone_table = top_of_zone_table;
  const struct trail_data_list *trails;
  time_t now = time(NULL);

  memset(rooms, 0, sizeof(rooms));
  memset(&wilderness_zone, 0, sizeof(wilderness_zone));
  wilderness_zone.number = 700;
  SET_BIT_AR(wilderness_zone.zone_flags, ZONE_WILDERNESS);
  zone_table = &wilderness_zone;
  top_of_zone_table = 0;
  rooms[0].number = 70001;
  rooms[0].zone = 0;
  rooms[1].number = 70002;
  rooms[1].zone = 0;

  movement_trail_registry_shutdown();
  movement_trail_record_at_room(&rooms[0], "first room trail", "human", DIR_NONE, NORTH, now);
  CuAssertPtrEquals(tc, NULL, movement_trails_at_room(&rooms[1]));
  movement_trail_record_at_room(&rooms[1], "second room trail", "elf", DIR_NONE, SOUTH, now);
  CuAssertIntEquals(tc, 2, (int)movement_trail_active_location_count());
  CuAssertIntEquals(tc, 0, (int)movement_trail_registry_validate());

  movement_trail_registry_shutdown();
  rooms[0].wilderness_coordinates_set = true;
  rooms[1].wilderness_coordinates_set = true;
  rooms[0].coords[X_COORD] = rooms[1].coords[X_COORD] = 12;
  rooms[0].coords[Y_COORD] = rooms[1].coords[Y_COORD] = -9;
  movement_trail_record_at_room(&rooms[0], "shared coordinate trail", "dwarf", DIR_NONE, EAST,
                                now);
  trails = movement_trails_at_room(&rooms[1]);
  CuAssertPtrNotNull(tc, trails);
  CuAssertStrEquals(tc, "shared coordinate trail", trails->head->name);
  CuAssertIntEquals(tc, 1, (int)movement_trail_active_location_count());
  CuAssertIntEquals(tc, 0, (int)movement_trail_registry_validate());

  movement_trail_registry_shutdown();
  zone_table = saved_zone_table;
  top_of_zone_table = saved_top_of_zone_table;
}

void TestAffectedOwnersExpireCharacterAndRoomStateOnRoundBoundaries(CuTest *tc)
{
  struct affected_type affect;
  struct char_data ch;
  struct game_event_owner owner;
  struct game_event_snapshot snapshot;
  struct game_scheduler_stats scheduler_stats;
  struct raff_node *raff;
  struct raff_node *saved_raff_list = raff_list;
  struct room_data room;
  struct room_data *saved_world = world;
  room_rnum saved_top_of_world = top_of_world;
  size_t event_count;
  unsigned long saved_pulse = pulse;

  memset(&room, 0, sizeof(room));
  clear_char(&ch);
  ch.player_specials = &dummy_mob;
  ch.player.short_descr = (char *)"affected owner test character";
  room.number = 100;
  world = &room;
  top_of_world = 0;
  raff_list = NULL;

  event_free_all();
  affected_owners_reset_for_test();
  affected_registry_reset_for_test();
  affected_owners_select_for_test(true);
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER));
  pulse = PULSE_VIOLENCE * 20U;
  event_init();
  affected_owners_init();
  event_runtime_get_stats(&scheduler_stats);
  CuAssertIntEquals(tc, 3, (int)scheduler_stats.registered_type_count);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, event_runtime_seal_types());

  new_affect(&affect);
  affect.spell = SPELL_ARMOR;
  affect.duration = 1;
  affect_to_char(&ch, &affect);
  affected_registry_attach(&ch);

  CREATE(raff, struct raff_node, 1);
  raff->room = 0;
  raff->timer = 2;
  raff->affection = RAFF_FOG;
  raff->spell = SPELL_ARMOR;
  raff->next = raff_list;
  raff_list = raff;
  SET_BIT(room.room_affections, RAFF_FOG);
  affected_room_owner_add(raff);

  CuAssertTrue(tc, affected_owner_events_enabled());
  CuAssertIntEquals(tc, 1, (int)affected_character_scheduled_count());
  CuAssertIntEquals(tc, 1, (int)affected_room_owner_count());
  CuAssertIntEquals(tc, 1, (int)affected_room_scheduled_count());
  CuAssertIntEquals(tc, 2, event_queue_depth());

  owner = game_event_owner_none();
  owner.kind = GAME_EVENT_OWNER_CHARACTER;
  owner.runtime_id = (uint64_t)(uintptr_t)&ch;
  owner.generation = ch.periodic_event_generation;
  event_count = 0U;
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    event_runtime_inspect_owner(owner, &snapshot, 1U, &event_count));
  CuAssertIntEquals(tc, 1, (int)event_count);
  CuAssertStrEquals(tc, "affected.character.duration",
                    event_runtime_type_name(snapshot.event_type));

  owner.kind = GAME_EVENT_OWNER_ROOM;
  owner.runtime_id = (uint64_t)(uint32_t)room.number + 1U;
  owner.generation = room.periodic_event_generation;
  event_count = 0U;
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    event_runtime_inspect_owner(owner, &snapshot, 1U, &event_count));
  CuAssertIntEquals(tc, 1, (int)event_count);
  CuAssertStrEquals(tc, "affected.room.duration",
                    event_runtime_type_name(snapshot.event_type));

  pulse += PULSE_LUMINARI;
  event_process();
  CuAssertIntEquals(tc, 1, ch.affected->duration);
  CuAssertIntEquals(tc, 2, raff->timer);
  CuAssertIntEquals(tc, 1, (int)affected_room_behavior_executions());
  CuAssertIntEquals(tc, 1, (int)affected_room_behavior_nodes_processed());
  CuAssertIntEquals(tc, 2, event_queue_depth());

  pulse += PULSE_VIOLENCE - PULSE_LUMINARI;
  event_process();
  CuAssertPtrNotNull(tc, ch.affected);
  CuAssertIntEquals(tc, 0, ch.affected->duration);
  CuAssertIntEquals(tc, 1, raff->timer);
  CuAssertIntEquals(tc, 2, event_queue_depth());

  pulse += PULSE_LUMINARI - (PULSE_VIOLENCE - PULSE_LUMINARI);
  event_process();
  CuAssertIntEquals(tc, 2, (int)affected_room_behavior_executions());
  CuAssertIntEquals(tc, 2, (int)affected_room_behavior_nodes_processed());
  CuAssertIntEquals(tc, 1, raff->timer);

  pulse += PULSE_VIOLENCE -
           (PULSE_LUMINARI - (PULSE_VIOLENCE - PULSE_LUMINARI));
  event_process();
  CuAssertPtrEquals(tc, NULL, ch.affected);
  CuAssertPtrEquals(tc, NULL, raff_list);
  CuAssertIntEquals(tc, 0, event_queue_depth());
  CuAssertIntEquals(tc, 2, (int)affected_character_callbacks());
  CuAssertIntEquals(tc, 4, (int)affected_room_callbacks());
  CuAssertIntEquals(tc, 2, (int)affected_character_nodes_processed());
  CuAssertIntEquals(tc, 2, (int)affected_room_nodes_processed());
  CuAssertIntEquals(tc, 0, (int)affected_room_registry_validate());

  affected_registry_detach(&ch);
  affected_owners_reset_for_test();
  affected_registry_reset_for_test();
  event_free_all();
  raff_list = saved_raff_list;
  world = saved_world;
  top_of_world = saved_top_of_world;
  pulse = saved_pulse;
  CuAssertIntEquals(tc, 0, (int)affected_room_owner_count());
  CuAssertIntEquals(tc, 0, (int)affected_room_registry_validate());
}

void TestAffectedRoomOwnerExpiresBeforeCoincidentBehavior(CuTest *tc)
{
  struct raff_node *raff;
  struct raff_node *saved_raff_list = raff_list;
  struct room_data room;
  struct room_data *saved_world = world;
  room_rnum saved_top_of_world = top_of_world;
  unsigned long saved_pulse = pulse;

  memset(&room, 0, sizeof(room));
  room.number = 102;
  world = &room;
  top_of_world = 0;
  raff_list = NULL;

  event_free_all();
  affected_owners_reset_for_test();
  affected_registry_reset_for_test();
  affected_owners_select_for_test(true);
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER));
  pulse = PULSE_VIOLENCE * 4U;
  event_init();
  affected_owners_init();

  CREATE(raff, struct raff_node, 1);
  raff->room = 0;
  raff->timer = 1;
  raff->affection = RAFF_FOG;
  raff->spell = SPELL_ARMOR;
  raff->next = raff_list;
  raff_list = raff;
  SET_BIT(room.room_affections, RAFF_FOG);
  affected_room_owner_add(raff);

  pulse += PULSE_LUMINARI - (pulse % PULSE_LUMINARI);
  event_process();
  CuAssertIntEquals(tc, 1, (int)affected_room_behavior_executions());
  CuAssertIntEquals(tc, 1, (int)affected_room_behavior_nodes_processed());
  CuAssertIntEquals(tc, 1, raff->timer);
  CuAssertIntEquals(tc, 1, event_queue_depth());

  pulse += PULSE_LUMINARI;
  event_process();
  CuAssertPtrEquals(tc, NULL, raff_list);
  CuAssertIntEquals(tc, 1, (int)affected_room_behavior_executions());
  CuAssertIntEquals(tc, 1, (int)affected_room_behavior_nodes_processed());
  CuAssertIntEquals(tc, 1, (int)affected_room_nodes_processed());
  CuAssertIntEquals(tc, 2, (int)affected_room_callbacks());
  CuAssertIntEquals(tc, 0, event_queue_depth());
  CuAssertIntEquals(tc, 0, (int)affected_room_registry_validate());

  affected_owners_reset_for_test();
  affected_registry_reset_for_test();
  event_free_all();
  raff_list = saved_raff_list;
  world = saved_world;
  top_of_world = saved_top_of_world;
  pulse = saved_pulse;
}

void TestAffectedOwnerAdmissionAndLegacyRollbackAreExclusive(CuTest *tc)
{
  struct affected_type affect;
  struct char_data ch;
  struct raff_node *raff;
  struct raff_node *saved_raff_list = raff_list;
  struct room_data room;
  struct room_data *saved_world = world;
  room_rnum saved_top_of_world = top_of_world;
  unsigned long saved_pulse = pulse;

  memset(&room, 0, sizeof(room));
  clear_char(&ch);
  ch.player_specials = &dummy_mob;
  ch.player.short_descr = (char *)"affected rollback test character";
  room.number = 101;
  world = &room;
  top_of_world = 0;
  raff_list = NULL;

  event_free_all();
  affected_owners_reset_for_test();
  affected_registry_reset_for_test();
  affected_owners_select_for_test(true);
  affected_owners_set_limits_for_test(0U, 0U);
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER));
  pulse = PULSE_VIOLENCE * 30U;
  event_init();
  affected_owners_init();

  new_affect(&affect);
  affect.spell = SPELL_ARMOR;
  affect.duration = 1;
  affect_to_char(&ch, &affect);
  affected_registry_attach(&ch);
  CREATE(raff, struct raff_node, 1);
  raff->room = 0;
  raff->timer = 1;
  raff->affection = RAFF_FOG;
  raff->spell = SPELL_ARMOR;
  raff->next = raff_list;
  raff_list = raff;
  affected_room_owner_add(raff);
  CuAssertIntEquals(tc, 0, event_queue_depth());
  CuAssertIntEquals(tc, 2, (int)affected_owner_admission_rejections());

  affected_registry_detach(&ch);
  while (ch.affected != NULL)
    affect_remove_no_total(&ch, ch.affected);
  rem_room_aff(raff);
  affected_owners_reset_for_test();
  affected_registry_reset_for_test();
  event_free_all();
  memset(&room, 0, sizeof(room));
  clear_char(&ch);
  ch.player_specials = &dummy_mob;
  ch.player.short_descr = (char *)"affected rollback test character";
  room.number = 101;
  raff_list = NULL;

  affected_owners_select_for_test(true);
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_LEGACY_QUEUE));
  event_init();
  affected_owners_init();
  new_affect(&affect);
  affect.spell = SPELL_ARMOR;
  affect.duration = 1;
  affect_to_char(&ch, &affect);
  affected_registry_attach(&ch);
  CREATE(raff, struct raff_node, 1);
  raff->room = 0;
  raff->timer = 2;
  raff->affection = RAFF_FOG;
  raff->spell = SPELL_ARMOR;
  raff->next = raff_list;
  raff_list = raff;
  affected_room_owner_add(raff);
  CuAssertTrue(tc, !affected_owner_events_enabled());
  CuAssertIntEquals(tc, 0, event_queue_depth());
  affect_update();
  affect_update();
  CuAssertPtrEquals(tc, NULL, ch.affected);
  CuAssertPtrEquals(tc, NULL, raff_list);

  affected_registry_detach(&ch);
  affected_owners_reset_for_test();
  affected_registry_reset_for_test();
  event_free_all();
  raff_list = saved_raff_list;
  world = saved_world;
  top_of_world = saved_top_of_world;
  pulse = saved_pulse;
  CuAssertIntEquals(tc, 0, (int)affected_room_owner_count());
  CuAssertIntEquals(tc, 0, (int)affected_room_registry_validate());
}

void TestAffectedOwnerCapacityRefillsAfterLifecycleCancellation(CuTest *tc)
{
  struct affected_type first_affect;
  struct affected_type second_affect;
  struct char_data first;
  struct char_data second;
  struct raff_node *first_raff;
  struct raff_node *second_raff;
  struct raff_node *saved_raff_list = raff_list;
  struct room_data rooms[2];
  struct room_data *saved_world = world;
  room_rnum saved_top_of_world = top_of_world;
  unsigned long saved_pulse = pulse;

  memset(rooms, 0, sizeof(rooms));
  clear_char(&first);
  clear_char(&second);
  first.player_specials = &dummy_mob;
  second.player_specials = &dummy_mob;
  first.player.short_descr = (char *)"first refill test character";
  second.player.short_descr = (char *)"second refill test character";
  rooms[0].number = 102;
  rooms[1].number = 103;
  world = rooms;
  top_of_world = 1;
  raff_list = NULL;

  event_free_all();
  affected_owners_reset_for_test();
  affected_registry_reset_for_test();
  affected_owners_select_for_test(true);
  affected_owners_set_limits_for_test(1U, 1U);
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER));
  pulse = PULSE_VIOLENCE * 40U;
  event_init();
  affected_owners_init();

  new_affect(&first_affect);
  first_affect.spell = SPELL_ARMOR;
  first_affect.duration = 1;
  affect_to_char(&first, &first_affect);
  affected_registry_attach(&first);
  new_affect(&second_affect);
  second_affect.spell = SPELL_ARMOR;
  second_affect.duration = 1;
  affect_to_char(&second, &second_affect);
  affected_registry_attach(&second);

  CREATE(first_raff, struct raff_node, 1);
  first_raff->room = 0;
  first_raff->timer = 2;
  first_raff->affection = RAFF_FOG;
  first_raff->spell = SPELL_ARMOR;
  first_raff->next = raff_list;
  raff_list = first_raff;
  affected_room_owner_add(first_raff);
  affected_room_owner_add(first_raff);
  CREATE(second_raff, struct raff_node, 1);
  second_raff->room = 1;
  second_raff->timer = 2;
  second_raff->affection = RAFF_FOG;
  second_raff->spell = SPELL_ARMOR;
  second_raff->next = raff_list;
  raff_list = second_raff;
  affected_room_owner_add(second_raff);

  CuAssertIntEquals(tc, 2, event_queue_depth());
  CuAssertIntEquals(tc, 2, (int)affected_owner_admission_rejections());
  CuAssertIntEquals(tc, 1, (int)rooms[0].affected_count);
  CuAssertTrue(tc, event_runtime_handle_is_none(second.affected_event_handle));
  CuAssertTrue(tc, event_runtime_handle_is_none(rooms[1].affected_event_handle));

  CuAssertPtrNotNull(tc, affected_registry_iteration_begin());
  affected_registry_detach(&first);
  CuAssertTrue(tc, event_runtime_handle_is_none(second.affected_event_handle));
  affected_registry_iteration_end();
  rem_room_aff(first_raff);
  CuAssertIntEquals(tc, 2, event_queue_depth());
  CuAssertTrue(tc, !event_runtime_handle_is_none(second.affected_event_handle));
  CuAssertTrue(tc, !event_runtime_handle_is_none(rooms[1].affected_event_handle));
  CuAssertIntEquals(tc, 0, (int)affected_room_registry_validate());

  while (first.affected != NULL)
    affect_remove_no_total(&first, first.affected);
  affected_registry_detach(&second);
  while (second.affected != NULL)
    affect_remove_no_total(&second, second.affected);
  rem_room_aff(second_raff);
  affected_owners_reset_for_test();
  affected_registry_reset_for_test();
  event_free_all();
  raff_list = saved_raff_list;
  world = saved_world;
  top_of_world = saved_top_of_world;
  pulse = saved_pulse;
  CuAssertIntEquals(tc, 0, (int)affected_room_owner_count());
  CuAssertIntEquals(tc, 0, (int)affected_room_registry_validate());
}

void TestAffectedRoomOwnersSurviveRoomOLCAndWorldReindex(CuTest *tc)
{
  struct event_runtime_handle saved_event_handle;
  struct raff_node *raff;
  struct raff_node *saved_raff_list = raff_list;
  struct room_data original[2];
  struct room_data moved[3];
  struct room_data edited;
  struct room_data *saved_world = world;
  room_rnum saved_top_of_world = top_of_world;
  unsigned long saved_pulse = pulse;

  memset(original, 0, sizeof(original));
  memset(moved, 0, sizeof(moved));
  memset(&edited, 0, sizeof(edited));
  original[0].number = 100;
  original[1].number = 200;
  world = original;
  top_of_world = 1;
  raff_list = NULL;

  event_free_all();
  affected_owners_reset_for_test();
  affected_owners_select_for_test(true);
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER));
  pulse = PULSE_VIOLENCE * 50U;
  event_init();
  affected_owners_init();

  CREATE(raff, struct raff_node, 1);
  raff->room = 1;
  raff->timer = 2;
  raff->affection = RAFF_FOG;
  raff->spell = SPELL_ARMOR;
  raff->next = raff_list;
  raff_list = raff;
  SET_BIT(original[1].room_affections, RAFF_FOG);
  affected_room_owner_add(raff);
  saved_event_handle = original[1].affected_event_handle;
  CuAssertTrue(tc, !event_runtime_handle_is_none(saved_event_handle));

  edited.number = original[1].number;
  edited.room_affections = 0L;
  CuAssertIntEquals(tc, TRUE, copy_room(&original[1], &edited));
  CuAssertPtrEquals(tc, raff, original[1].affected_head);
  CuAssertTrue(tc, event_runtime_handles_equal(saved_event_handle,
                                              original[1].affected_event_handle));
  CuAssertTrue(tc, ROOM_AFFECTED(1, RAFF_FOG));

  affected_room_owners_prepare_world_reindex();
  CuAssertIntEquals(tc, 0, event_queue_depth());
  moved[0] = original[0];
  moved[1].number = 150;
  moved[2] = original[1];
  world = moved;
  top_of_world = 2;
  affected_room_owners_finish_world_reindex(1U, true);

  CuAssertIntEquals(tc, 2, (int)raff->room);
  CuAssertPtrEquals(tc, raff, moved[2].affected_head);
  CuAssertTrue(tc, !event_runtime_handle_is_none(moved[2].affected_event_handle));
  CuAssertIntEquals(tc, 1, event_queue_depth());
  CuAssertIntEquals(tc, 0, (int)affected_room_registry_validate());

  pulse += PULSE_VIOLENCE;
  event_process();
  CuAssertIntEquals(tc, 1, raff->timer);

  affected_room_owners_prepare_world_reindex();
  CuAssertIntEquals(tc, 0, event_queue_depth());
  original[0] = moved[0];
  original[1] = moved[2];
  world = original;
  top_of_world = 1;
  affected_room_owners_finish_world_reindex(1U, false);

  CuAssertIntEquals(tc, 1, (int)raff->room);
  CuAssertPtrEquals(tc, raff, original[1].affected_head);
  CuAssertTrue(tc, !event_runtime_handle_is_none(original[1].affected_event_handle));
  CuAssertIntEquals(tc, 1, event_queue_depth());
  CuAssertIntEquals(tc, 0, (int)affected_room_registry_validate());

  pulse += PULSE_VIOLENCE;
  event_process();
  CuAssertPtrEquals(tc, NULL, raff_list);
  CuAssertIntEquals(tc, 0, event_queue_depth());
  CuAssertIntEquals(tc, 0, (int)affected_room_registry_validate());

  affected_owners_reset_for_test();
  event_free_all();
  free_room_strings(&original[1]);
  raff_list = saved_raff_list;
  world = saved_world;
  top_of_world = saved_top_of_world;
  pulse = saved_pulse;
}

void TestDeviceRecoverySkipsCharacterWithoutPlayerSpecials(CuTest *tc)
{
  struct char_data ch;

  memset(&ch, 0, sizeof(ch));
  CuAssertPtrEquals(tc, NULL, ch.player_specials);
  check_device_one(&ch);
  CuAssertPtrEquals(tc, NULL, ch.player_specials);
}

void TestCharacterPeriodicOwnerUsesNearestGameplayDeadlines(CuTest *tc)
{
  struct char_data ch;
  struct descriptor_data descriptor;
  struct game_event_owner owner;
  struct game_event_snapshot snapshot;
  struct game_scheduler_stats scheduler_stats;
  struct player_special_data specials;
  struct char_data *saved_character_list = character_list;
  size_t event_count;
  unsigned long saved_pulse = pulse;

  memset(&descriptor, 0, sizeof(descriptor));
  memset(&specials, 0, sizeof(specials));
  clear_char(&ch);
  ch.player_specials = &specials;
  ch.desc = &descriptor;
  descriptor.character = &ch;
  STATE(&descriptor) = CON_PLAYING;
  SET_BIT_AR(PRF_FLAGS(&ch), PRF_NOHINT);
  IN_ROOM(&ch) = NOWHERE;
  specials.walkto_location = 103009;
  character_list = NULL;

  event_free_all();
  character_periodic_reset_for_test();
  character_periodic_select_for_test(true);
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER));
  pulse = 700U;
  event_init();
  character_periodic_init();
  event_runtime_get_stats(&scheduler_stats);
  CuAssertIntEquals(tc, 2, (int)scheduler_stats.registered_type_count);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, event_runtime_seal_types());
  character_periodic_sync(&ch);

  CuAssertTrue(tc, character_periodic_events_enabled());
  CuAssertIntEquals(tc, 1, (int)character_periodic_owner_count());
  CuAssertIntEquals(tc, 1, (int)character_periodic_scheduled_count());
  CuAssertIntEquals(tc, 7, (int)native_event_remaining(tc, ch.character_periodic_event_handle));
  CuAssertIntEquals(tc, 1, event_queue_depth());
  owner = game_event_owner_none();
  owner.kind = GAME_EVENT_OWNER_CHARACTER;
  owner.runtime_id = (uint64_t)(uintptr_t)&ch;
  owner.generation = ch.periodic_event_generation;
  event_count = 0U;
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    event_runtime_inspect_owner(owner, &snapshot, 1U, &event_count));
  CuAssertIntEquals(tc, 1, (int)event_count);
  CuAssertStrEquals(tc, "character.maintenance",
                    event_runtime_type_name(snapshot.event_type));

  CuAssertIntEquals(tc, GAME_EVENT_CANCELLED,
                    event_runtime_cancel(ch.character_periodic_event_handle));
  CuAssertTrue(tc, event_runtime_handle_is_none(ch.character_periodic_event_handle));
  CuAssertIntEquals(tc, 0, (int)character_periodic_scheduled_count());
  character_periodic_sync(&ch);
  CuAssertIntEquals(tc, 1, (int)character_periodic_scheduled_count());
  CuAssertIntEquals(tc, 7,
                    (int)native_event_remaining(tc, ch.character_periodic_event_handle));

  process_scheduler_pulses(7U);
  CuAssertIntEquals(tc, 0, specials.walkto_location);
  CuAssertIntEquals(tc, 1, (int)character_periodic_walk_executions());
  CuAssertIntEquals(tc, 43,
                    (int)native_event_remaining(tc, ch.character_periodic_event_handle));

  process_scheduler_pulses(43U);
  CuAssertIntEquals(tc, 1, (int)character_periodic_psp_executions());

  IS_PERFORMING(&ch) = TRUE;
  GET_PERFORMING(&ch) = PERFORMANCE_NONE;
  GET_SECONDARY_PERFORMING(&ch) = PERFORMANCE_NONE;
  character_periodic_sync(&ch);
  CuAssertIntEquals(tc, 20,
                    (int)native_event_remaining(tc, ch.character_periodic_event_handle));

  process_scheduler_pulses(20U);
  CuAssertIntEquals(tc, 1, (int)character_periodic_bardic_executions());
  CuAssertTrue(tc, !IS_PERFORMING(&ch));

  process_scheduler_pulses(2230U);
  CuAssertIntEquals(tc, 1, (int)character_periodic_hint_executions());
  CuAssertIntEquals(tc, 0, (int)character_periodic_registry_validate());

  character_periodic_forget(&ch);
  CuAssertIntEquals(tc, 0, event_queue_depth());
  character_periodic_reset_for_test();
  event_free_all();
  character_list = saved_character_list;
  pulse = saved_pulse;
}

void TestCharacterPeriodicLateCallbackRunsPassedDeadline(CuTest *tc)
{
  struct char_data ch;
  struct descriptor_data descriptor;
  struct player_special_data specials;
  struct char_data *saved_character_list = character_list;
  unsigned long saved_pulse = pulse;

  memset(&descriptor, 0, sizeof(descriptor));
  memset(&specials, 0, sizeof(specials));
  clear_char(&ch);
  ch.periodic_event_generation = 9001U;
  ch.player_specials = &specials;
  ch.desc = &descriptor;
  descriptor.character = &ch;
  STATE(&descriptor) = CON_PLAYING;
  SET_BIT_AR(PRF_FLAGS(&ch), PRF_NOHINT);
  IN_ROOM(&ch) = NOWHERE;
  specials.walkto_location = 103009;
  character_list = NULL;

  event_free_all();
  character_periodic_reset_for_test();
  character_periodic_select_for_test(true);
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER));
  pulse = 700U;
  event_init();
  character_periodic_init();
  character_periodic_sync(&ch);
  CuAssertIntEquals(tc, 707, (int)ch.character_periodic_due_pulse);

  pulse = 709U;
  event_process();
  CuAssertIntEquals(tc, 0, specials.walkto_location);
  CuAssertIntEquals(tc, 1, (int)character_periodic_walk_executions());
  CuAssertIntEquals(tc, 39,
                    (int)native_event_remaining(tc, ch.character_periodic_event_handle));
  CuAssertIntEquals(tc, 750, (int)ch.character_periodic_due_pulse);

  character_periodic_forget(&ch);
  character_periodic_reset_for_test();
  event_free_all();
  character_list = saved_character_list;
  pulse = saved_pulse;
}

void TestCharacterPeriodicSchedulesPerformingNpcWithoutOtherWork(CuTest *tc)
{
  struct char_data npc;
  struct room_data room;
  struct char_data *saved_character_list = character_list;
  struct room_data *saved_world = world;
  room_rnum saved_top_of_world = top_of_world;
  unsigned long saved_pulse = pulse;
  long delay;

  memset(&room, 0, sizeof(room));
  clear_char(&npc);
  npc.periodic_event_generation = 9002U;
  room.number = 1401;
  world = &room;
  top_of_world = 0;
  npc.player_specials = &dummy_mob;
  npc.player.short_descr = (char *)"performing periodic test mobile";
  SET_BIT_AR(MOB_FLAGS(&npc), MOB_ISNPC);
  IN_ROOM(&npc) = 0;
  GET_HIT(&npc) = GET_REAL_MAX_HIT(&npc) = GET_MAX_HIT(&npc) = 100;
  GET_MOVE(&npc) = GET_REAL_MAX_MOVE(&npc) = GET_MAX_MOVE(&npc) = 100;
  GET_PSP(&npc) = GET_REAL_MAX_PSP(&npc) = GET_MAX_PSP(&npc) = 100;
  IS_PERFORMING(&npc) = TRUE;
  GET_PERFORMING(&npc) = PERFORMANCE_NONE;
  GET_SECONDARY_PERFORMING(&npc) = PERFORMANCE_NONE;
  room.people = &npc;
  character_list = &npc;

  event_free_all();
  character_periodic_reset_for_test();
  character_periodic_select_for_test(true);
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER));
  pulse = PULSE_VIOLENCE * 20U;
  event_init();
  character_periodic_init();
  CuAssertIntEquals(tc, 1, (int)character_periodic_owner_count());
  CuAssertIntEquals(tc, 1, (int)character_periodic_scheduled_count());
  delay = (long)native_event_remaining(tc, npc.character_periodic_event_handle);
  CuAssertTrue(tc, delay > 0L && delay <= PULSE_VERSE_INTERVAL);

  process_scheduler_pulses(PULSE_VERSE_INTERVAL);
  CuAssertIntEquals(tc, 1, (int)character_periodic_bardic_executions());
  CuAssertTrue(tc, !IS_PERFORMING(&npc));
  CuAssertIntEquals(tc, 0, (int)character_periodic_owner_count());
  CuAssertIntEquals(tc, 0, (int)character_periodic_scheduled_count());

  character_periodic_reset_for_test();
  event_free_all();
  character_list = saved_character_list;
  world = saved_world;
  top_of_world = saved_top_of_world;
  pulse = saved_pulse;
}

void TestCharacterPeriodicSchedulesInWorldMixedWorkByOwner(CuTest *tc)
{
  struct char_data npc;
  struct char_data player;
  struct descriptor_data descriptor;
  struct player_special_data specials;
  struct room_data room;
  struct char_data *saved_character_list = character_list;
  struct room_data *saved_world = world;
  room_rnum saved_top_of_world = top_of_world;
  unsigned long saved_pulse = pulse;

  memset(&descriptor, 0, sizeof(descriptor));
  memset(&specials, 0, sizeof(specials));
  memset(&room, 0, sizeof(room));
  clear_char(&npc);
  clear_char(&player);

  room.number = 1400;
  room.sector_type = SECT_INSIDE;
  world = &room;
  top_of_world = 0;

  npc.player_specials = &dummy_mob;
  npc.player.short_descr = (char *)"periodic test mobile";
  SET_BIT_AR(MOB_FLAGS(&npc), MOB_ISNPC);
  IN_ROOM(&npc) = 0;
  GET_LEVEL(&npc) = 1;
  GET_HIT(&npc) = GET_REAL_MAX_HIT(&npc) = GET_MAX_HIT(&npc) = 100;
  GET_MOVE(&npc) = GET_REAL_MAX_MOVE(&npc) = GET_MAX_MOVE(&npc) = 100;
  GET_PSP(&npc) = GET_REAL_MAX_PSP(&npc) = GET_MAX_PSP(&npc) = 100;
  npc.char_specials.daze_cooldown = 2;
  npc.char_specials.terror_cooldown = 2;

  player.player_specials = &specials;
  player.player.name = (char *)"Periodic test player";
  player.desc = &descriptor;
  descriptor.character = &player;
  descriptor.output = descriptor.small_outbuf;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  descriptor.pProtocol = ProtocolCreate();
  STATE(&descriptor) = CON_PLAYING;
  IN_ROOM(&player) = 0;
  GET_LEVEL(&player) = 1;
  GET_HIT(&player) = GET_REAL_MAX_HIT(&player) = GET_MAX_HIT(&player) = 100;
  GET_MOVE(&player) = GET_REAL_MAX_MOVE(&player) = GET_MAX_MOVE(&player) = 100;
  GET_PSP(&player) = GET_REAL_MAX_PSP(&player) = GET_MAX_PSP(&player) = 100;
  player.char_specials.daze_cooldown = 2;
  specials.saved.mission_cooldown = 2;
  GET_QUEST(&player, 0) = 1000;
  GET_QUEST_TIME(&player, 0) = 2;

  player.next = &npc;
  player.next_in_room = &npc;
  room.people = &player;
  character_list = &player;

  event_free_all();
  character_periodic_reset_for_test();
  character_periodic_select_for_test(true);
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER));
  pulse = PULSE_VIOLENCE * 20U;
  event_init();
  character_periodic_init();

  CuAssertIntEquals(tc, 2, (int)character_periodic_owner_count());
  CuAssertIntEquals(tc, 2, (int)character_periodic_scheduled_count());
  CuAssertIntEquals(tc, 2, event_queue_depth());

  process_scheduler_pulses(PULSE_LUMINARI);
  CuAssertIntEquals(tc, 2, (int)character_periodic_luminari_executions());
  CuAssertIntEquals(tc, 1, (int)character_periodic_damage_effect_executions());
  CuAssertIntEquals(tc, 0, (int)character_periodic_player_misc_executions());
  CuAssertIntEquals(tc, 1, npc.char_specials.daze_cooldown);
  CuAssertIntEquals(tc, 2, specials.saved.mission_cooldown);

  process_scheduler_pulses(PULSE_VIOLENCE - PULSE_LUMINARI);
  CuAssertIntEquals(tc, 2, (int)character_periodic_damage_effect_executions());
  CuAssertIntEquals(tc, 1, (int)character_periodic_player_misc_executions());
  CuAssertIntEquals(tc, 1, npc.char_specials.daze_cooldown);
  CuAssertIntEquals(tc, 1, npc.char_specials.terror_cooldown);
  CuAssertIntEquals(tc, 1, player.char_specials.daze_cooldown);
  CuAssertIntEquals(tc, 1, specials.saved.mission_cooldown);
  CuAssertIntEquals(tc, 2, (int)character_periodic_d20_round_executions());
  CuAssertIntEquals(tc, 5, (int)character_periodic_callbacks());
  CuAssertIntEquals(tc, 0, (int)character_periodic_registry_validate());
  CuAssertIntEquals(tc, 2, event_queue_depth());

  process_scheduler_pulses(SECS_PER_MUD_HOUR * PASSES_PER_SEC * 2U - pulse);
  /* The NPC retires when its explicit cooldown work ends; only the player
   * remains eligible for the later generic device check. */
  CuAssertIntEquals(tc, 1, (int)character_periodic_device_executions());
  CuAssertIntEquals(tc, 1, (int)character_periodic_timed_quest_executions());
  CuAssertIntEquals(tc, 1, GET_QUEST_TIME(&player, 0));

  character_periodic_reset_for_test();
  event_free_all();
  ProtocolDestroy(descriptor.pProtocol);
  character_list = saved_character_list;
  world = saved_world;
  top_of_world = saved_top_of_world;
  pulse = saved_pulse;
}

void TestHuntTargetsObserveResetThroughOwnerGeneration(CuTest *tc)
{
  struct char_data hunt_target;

  clear_char(&hunt_target);
  hunt_target.player_specials = &dummy_mob;
  SET_BIT_AR(MOB_FLAGS(&hunt_target), MOB_ISNPC);
  SET_BIT_AR(MOB_FLAGS(&hunt_target), MOB_HUNTS_TARGET);
  hunt_target.mob_specials.hunt_generation = 40U;
  hunt_target.mob_specials.hunt_cooldown = -1;
  hunt_target_set_generation_for_test(41U);

  hunt_target_periodic_one(&hunt_target);
  CuAssertIntEquals(tc, 41, (int)hunt_target.mob_specials.hunt_generation);
  CuAssertIntEquals(tc, 49, hunt_target.mob_specials.hunt_cooldown);

  hunt_target_periodic_one(&hunt_target);
  CuAssertIntEquals(tc, 48, hunt_target.mob_specials.hunt_cooldown);
  CuAssertIntEquals(tc, 41, (int)hunt_target_generation_for_test());

  hunt_target_set_generation_for_test(1U);
}

void TestCharacterPeriodicTypedMovementAdmitsInWorldOwner(CuTest *tc)
{
  struct char_data ch;
  struct domain_character_moved moved;
  struct domain_event_bus *bus;
  struct room_data room;
  struct char_data *saved_character_list = character_list;
  struct room_data *saved_world = world;
  room_rnum saved_top_of_world = top_of_world;
  unsigned long saved_pulse = pulse;

  memset(&room, 0, sizeof(room));
  memset(&moved, 0, sizeof(moved));
  clear_char(&ch);
  ch.player_specials = &dummy_mob;
  ch.player.short_descr = (char *)"movement-admitted mobile";
  SET_BIT_AR(MOB_FLAGS(&ch), MOB_ISNPC);
  ch.char_specials.daze_cooldown = 2;
  room.number = 1401;
  room.sector_type = SECT_INSIDE;
  world = &room;
  top_of_world = 0;
  character_list = NULL;

  event_free_all();
  character_periodic_reset_for_test();
  character_periodic_select_for_test(true);
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER));
  pulse = 1400U;
  event_init();
  character_periodic_init();
  CuAssertIntEquals(tc, 0, event_queue_depth());

  IN_ROOM(&ch) = 0;
  room.people = &ch;
  character_list = &ch;
  bus = create_bus(tc, 4U, 16U, NULL, 100U);
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_register_foundation_types(bus));
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_world_register_resolvers(bus));
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, character_periodic_register_handlers(bus));
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_seal(bus));
  moved.character = domain_event_character_handle(&ch);
  moved.from_room = domain_entity_handle_none();
  moved.to_room = domain_event_room_handle(0);
  moved.direction = -1;

  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    DOMAIN_EVENT_PUBLISH(bus, DOMAIN_EVENT_CHARACTER_MOVED, &moved));
  CuAssertIntEquals(tc, 1, (int)character_periodic_owner_count());
  CuAssertIntEquals(tc, 1, (int)character_periodic_scheduled_count());
  CuAssertTrue(tc, !event_runtime_handle_is_none(ch.character_periodic_event_handle));
  CuAssertIntEquals(tc, 0, (int)character_periodic_registry_validate());

  character_periodic_forget(&ch);
  CuAssertIntEquals(tc, 0, event_queue_depth());
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_bus_destroy(bus));
  character_periodic_reset_for_test();
  event_free_all();
  character_list = saved_character_list;
  world = saved_world;
  top_of_world = saved_top_of_world;
  pulse = saved_pulse;
}

void TestCharacterPeriodicCapacityRefillsAndLegacyIsExclusive(CuTest *tc)
{
  struct char_data first;
  struct char_data second;
  struct descriptor_data first_descriptor;
  struct descriptor_data second_descriptor;
  struct char_data *saved_character_list = character_list;
  unsigned long saved_pulse = pulse;

  memset(&first_descriptor, 0, sizeof(first_descriptor));
  memset(&second_descriptor, 0, sizeof(second_descriptor));
  clear_char(&first);
  clear_char(&second);
  first.desc = &first_descriptor;
  second.desc = &second_descriptor;
  first_descriptor.character = &first;
  second_descriptor.character = &second;
  STATE(&first_descriptor) = CON_PLAYING;
  STATE(&second_descriptor) = CON_PLAYING;
  character_list = NULL;

  event_free_all();
  character_periodic_reset_for_test();
  character_periodic_select_for_test(true);
  CuAssertIntEquals(tc, 32768, (int)character_periodic_admission_limit());
  character_periodic_set_limit_for_test(1U);
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER));
  pulse = 1000U;
  event_init();
  character_periodic_init();
  character_periodic_sync(&first);
  character_periodic_sync(&second);

  CuAssertIntEquals(tc, 2, (int)character_periodic_owner_count());
  CuAssertIntEquals(tc, 1, (int)character_periodic_scheduled_count());
  CuAssertIntEquals(tc, 1, (int)character_periodic_admission_rejections());
  CuAssertTrue(tc, !event_runtime_handle_is_none(first.character_periodic_event_handle));
  CuAssertTrue(tc, event_runtime_handle_is_none(second.character_periodic_event_handle));

  character_periodic_forget(&first);
  CuAssertTrue(tc, !event_runtime_handle_is_none(second.character_periodic_event_handle));
  CuAssertIntEquals(tc, 1, event_queue_depth());
  second.desc = NULL;
  character_periodic_sync(&second);
  CuAssertIntEquals(tc, 0, (int)character_periodic_owner_count());
  CuAssertIntEquals(tc, 0, event_queue_depth());
  character_periodic_reset_for_test();
  event_free_all();

  character_periodic_select_for_test(true);
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_LEGACY_QUEUE));
  event_init();
  character_periodic_init();
  first.desc = &first_descriptor;
  character_periodic_sync(&first);
  CuAssertTrue(tc, !character_periodic_events_enabled());
  CuAssertIntEquals(tc, 0, event_queue_depth());

  character_periodic_reset_for_test();
  event_free_all();
  character_list = saved_character_list;
  pulse = saved_pulse;
}

void TestPointUpdateSchedulesOnlyDuePlayersAndObjects(CuTest *tc)
{
  struct char_data player;
  struct char_data npc;
  struct player_special_data specials;
  struct obj_data timed;
  struct obj_data dormant;
  struct char_data *saved_character_list = character_list;
  struct obj_data *saved_object_list = object_list;
  struct happyhour saved_happy = happy_data;
  int saved_idle_max_level = CONFIG_IDLE_MAX_LEVEL;
  unsigned long saved_pulse = pulse;

  memset(&specials, 0, sizeof(specials));
  clear_char(&player);
  clear_char(&npc);
  clear_object(&timed);
  clear_object(&dormant);
  player.player_specials = &specials;
  GET_LEVEL(&player) = 1;
  GET_COND(&player, HUNGER) = 10;
  GET_COND(&player, DRUNK) = 10;
  GET_COND(&player, THIRST) = 10;
  npc.player_specials = &dummy_mob;
  SET_BIT_AR(MOB_FLAGS(&npc), MOB_ISNPC);
  player.next = &npc;
  character_list = &player;
  timed.object_list_member = true;
  dormant.object_list_member = true;
  timed.next = &dormant;
  object_list = &timed;
  GET_OBJ_TIMER(&timed) = 2;
  GET_OBJ_SPECTIMER(&timed, 0) = 2;
  CONFIG_IDLE_MAX_LEVEL = 0;
  HAPPY_TIME = 3;

  event_free_all();
  point_update_periodic_reset_for_test();
  point_update_periodic_select_for_test(true);
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER));
  pulse = 100U;
  event_init();
  point_update_periodic_init();

  CuAssertTrue(tc, point_update_events_enabled());
  CuAssertTrue(tc, native_event_type_is_registered("world.mud_hour_update"));
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, event_runtime_seal_types());
  CuAssertIntEquals(tc, 1, event_queue_depth());
  CuAssertIntEquals(tc, 1, (int)point_update_character_count());
  CuAssertIntEquals(tc, 1, (int)point_update_object_count());
  CuAssertIntEquals(tc, 0, (int)point_update_character_registry_validate());
  CuAssertIntEquals(tc, 0, (int)point_update_object_registry_validate());

  pulse = SECS_PER_MUD_HOUR * PASSES_PER_SEC;
  event_process();
  CuAssertIntEquals(tc, 1, (int)point_update_service_callbacks());
  CuAssertIntEquals(tc, 3, HAPPY_TIME);
  CuAssertIntEquals(tc, 10, GET_COND(&player, HUNGER));
  CuAssertIntEquals(tc, 2, GET_OBJ_TIMER(&timed));

  CuAssertTrue(tc, point_update_periodic_dispatch_due());
  CuAssertIntEquals(tc, 2, HAPPY_TIME);
  CuAssertIntEquals(tc, 9, GET_COND(&player, HUNGER));
  CuAssertIntEquals(tc, 9, GET_COND(&player, DRUNK));
  CuAssertIntEquals(tc, 9, GET_COND(&player, THIRST));
  CuAssertIntEquals(tc, 1, player.char_specials.timer);
  CuAssertIntEquals(tc, 1, GET_OBJ_TIMER(&timed));
  CuAssertIntEquals(tc, 1, GET_OBJ_SPECTIMER(&timed, 0));
  CuAssertIntEquals(tc, 1, (int)point_update_character_executions());
  CuAssertIntEquals(tc, 1, (int)point_update_object_executions());
  CuAssertTrue(tc, !point_update_periodic_dispatch_due());

  pulse += SECS_PER_MUD_HOUR * PASSES_PER_SEC;
  event_process();
  CuAssertTrue(tc, point_update_periodic_dispatch_due());
  CuAssertIntEquals(tc, 0, GET_OBJ_TIMER(&timed));
  CuAssertIntEquals(tc, 0, GET_OBJ_SPECTIMER(&timed, 0));
  CuAssertIntEquals(tc, 0, (int)point_update_object_count());
  CuAssertIntEquals(tc, 2, (int)point_update_dispatches());

  point_update_periodic_reset_for_test();
  event_free_all();
  character_list = saved_character_list;
  object_list = saved_object_list;
  happy_data = saved_happy;
  CONFIG_IDLE_MAX_LEVEL = saved_idle_max_level;
  pulse = saved_pulse;
}

void TestPointUpdateMutationHooksAndLegacyGateAreExclusive(CuTest *tc)
{
  struct obj_data live;
  struct obj_data editor_draft;
  struct obj_data *saved_object_list = object_list;
  unsigned long saved_pulse = pulse;

  clear_object(&live);
  clear_object(&editor_draft);
  live.object_list_member = true;
  object_list = &live;

  event_free_all();
  point_update_periodic_reset_for_test();
  point_update_periodic_select_for_test(true);
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER));
  pulse = 200U;
  event_init();
  point_update_periodic_init();
  CuAssertIntEquals(tc, 0, (int)point_update_object_count());

  point_update_object_spec_timer_set(&live, 0, 4);
  CuAssertIntEquals(tc, 1, (int)point_update_object_count());
  point_update_object_spec_timer_set(&live, 0, 0);
  CuAssertIntEquals(tc, 0, (int)point_update_object_count());

  GET_OBJ_TIMER(&editor_draft) = 5;
  point_update_object_sync(&editor_draft);
  CuAssertIntEquals(tc, 0, (int)point_update_object_count());
  GET_OBJ_TIMER(&live) = 5;
  point_update_object_sync(&live);
  CuAssertIntEquals(tc, 1, (int)point_update_object_count());
  point_update_object_forget(&live);
  CuAssertIntEquals(tc, 0, (int)point_update_object_count());

  point_update_periodic_reset_for_test();
  event_free_all();
  point_update_periodic_select_for_test(false);
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER));
  event_init();
  point_update_periodic_init();
  point_update_object_sync(&live);
  CuAssertTrue(tc, !point_update_events_enabled());
  CuAssertIntEquals(tc, 0, event_queue_depth());
  CuAssertIntEquals(tc, 0, (int)point_update_object_count());

  point_update_periodic_reset_for_test();
  event_free_all();
  object_list = saved_object_list;
  pulse = saved_pulse;
}

void TestPointUpdateObjectDecayRemainsExtractionSafe(CuTest *tc)
{
  struct obj_data *first;
  struct obj_data *second;
  struct char_data *saved_character_list = character_list;
  struct obj_data *saved_object_list = object_list;
  unsigned long saved_pulse = pulse;

  character_list = NULL;
  object_list = NULL;
  event_free_all();
  point_update_periodic_reset_for_test();
  point_update_periodic_select_for_test(true);
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER));
  pulse = 100U;
  event_init();
  point_update_periodic_init();

  first = create_obj();
  second = create_obj();
  SET_BIT_AR(GET_OBJ_EXTRA(first), ITEM_DECAY);
  SET_BIT_AR(GET_OBJ_EXTRA(second), ITEM_DECAY);
  GET_OBJ_TIMER(first) = 1;
  GET_OBJ_TIMER(second) = 1;
  point_update_object_sync(first);
  point_update_object_sync(second);
  CuAssertIntEquals(tc, 2, (int)point_update_object_count());

  pulse = SECS_PER_MUD_HOUR * PASSES_PER_SEC;
  event_process();
  CuAssertTrue(tc, point_update_periodic_dispatch_due());
  CuAssertPtrEquals(tc, NULL, object_list);
  CuAssertIntEquals(tc, 0, (int)point_update_object_count());
  CuAssertIntEquals(tc, 2, (int)point_update_object_executions());
  CuAssertIntEquals(tc, 0, (int)point_update_object_registry_validate());

  point_update_periodic_reset_for_test();
  event_free_all();
  character_list = saved_character_list;
  object_list = saved_object_list;
  pulse = saved_pulse;
}

void TestPointUpdateFallsBackWhenServiceCannotStart(CuTest *tc)
{
  event_free_all();
  point_update_periodic_reset_for_test();
  point_update_periodic_select_for_test(true);
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_LEGACY_QUEUE));
  event_init();
  point_update_periodic_init();

  CuAssertTrue(tc, !point_update_events_enabled());
  CuAssertIntEquals(tc, 0, event_queue_depth());

  point_update_periodic_reset_for_test();
  event_free_all();
}

void TestVesselPeriodicSchedulesLoadedOwnersAndKeepsLegacyExclusive(CuTest *tc)
{
  const int slot = GREYHAWK_MAXSHIPS - 1;
  struct greyhawk_ship_data saved_ship = greyhawk_ships[slot];
  struct greyhawk_ship_data *ship = &greyhawk_ships[slot];
  struct game_event_snapshot snapshot;
  unsigned long saved_pulse = pulse;
  int saved_vessel_system = CONFIG_VESSEL_SYSTEM;
  uint64_t first_generation;

  event_free_all();
  vessel_periodic_reset_for_test();
  memset(ship, 0, sizeof(*ship));
  ship->active = true;
  ship->shipnum = slot;
  ship->slot[0].type = 1;
  ship->slot[0].timer = 2;
  CONFIG_VESSEL_SYSTEM = 1;
  vessel_periodic_select_for_test(true);
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER));
  pulse = 100U;
  event_init();
  vessel_periodic_init();
  CuAssertTrue(tc, native_event_type_is_registered("vessel.greyhawk.agenda"));
  CuAssertTrue(tc, native_event_type_is_registered("vessel.shared.agenda"));
  CuAssertTrue(tc, native_event_type_is_registered("vessel.rol.agenda"));
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK, event_runtime_seal_types());
  vessel_periodic_sync(ship);

  CuAssertTrue(tc, vessel_periodic_events_enabled());
  CuAssertIntEquals(tc, 1, (int)vessel_periodic_owner_count());
  CuAssertIntEquals(tc, 1, (int)vessel_periodic_scheduled_count());
  CuAssertIntEquals(tc, 0, (int)vessel_periodic_registry_validate());
  CuAssertTrue(tc, event_queue_depth() >= 2);
  CuAssertIntEquals(tc, GAME_SCHEDULER_OK,
                    event_runtime_inspect(ship->periodic_event_handle, &snapshot));
  CuAssertStrEquals(tc, "vessel.greyhawk.agenda",
                    event_runtime_type_name(snapshot.event_type));
  first_generation = ship->periodic_generation;

  pulse += AUTOPILOT_TICK_INTERVAL;
  event_process();
  CuAssertIntEquals(tc, 1, ship->slot[0].timer);
  CuAssertIntEquals(tc, 1, (int)vessel_periodic_callbacks());
  CuAssertIntEquals(tc, 1, (int)vessel_periodic_service_callbacks());
  CuAssertIntEquals(tc, 1, (int)vessel_periodic_fast_executions());
  CuAssertIntEquals(tc, 0, (int)vessel_periodic_registry_validate());

  CONFIG_VESSEL_SYSTEM = 0;
  vessel_periodic_feature_changed();
  CuAssertIntEquals(tc, 0, (int)vessel_periodic_owner_count());
  CuAssertIntEquals(tc, 0, (int)vessel_periodic_scheduled_count());
  CuAssertTrue(tc, event_runtime_handle_is_none(ship->periodic_event_handle));
  CuAssertIntEquals(tc, 0, event_queue_depth());
  CONFIG_VESSEL_SYSTEM = 1;
  vessel_periodic_feature_changed();
  CuAssertIntEquals(tc, 1, (int)vessel_periodic_owner_count());
  CuAssertIntEquals(tc, 1, (int)vessel_periodic_scheduled_count());
  CuAssertTrue(tc, !event_runtime_handle_is_none(ship->periodic_event_handle));
  CuAssertTrue(tc, event_queue_depth() >= 2);
  CuAssertIntEquals(tc, 0, (int)vessel_periodic_registry_validate());

  vessel_periodic_forget(ship);
  memset(ship, 0, sizeof(*ship));
  ship->active = true;
  ship->shipnum = slot;
  vessel_periodic_sync(ship);
  CuAssertTrue(tc, ship->periodic_generation != 0U);
  CuAssertTrue(tc, ship->periodic_generation != first_generation);
  CuAssertIntEquals(tc, 1, (int)vessel_periodic_owner_count());
  CuAssertIntEquals(tc, 0, (int)vessel_periodic_registry_validate());

  vessel_periodic_reset_for_test();
  event_free_all();
  vessel_periodic_select_for_test(false);
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER));
  event_init();
  vessel_periodic_init();
  vessel_periodic_sync(ship);
  CuAssertTrue(tc, !vessel_periodic_events_enabled());
  CuAssertIntEquals(tc, 0, event_queue_depth());
  ship->slot[0].type = 1;
  ship->slot[0].timer = 2;
  vessel_combat_tick();
  CuAssertIntEquals(tc, 1, ship->slot[0].timer);

  vessel_periodic_reset_for_test();
  event_free_all();
  greyhawk_ships[slot] = saved_ship;
  CONFIG_VESSEL_SYSTEM = saved_vessel_system;
  pulse = saved_pulse;
}

void TestVesselPeriodicCapacityRefillsAfterOwnerCancellation(CuTest *tc)
{
  const int first_slot = GREYHAWK_MAXSHIPS - 2;
  const int second_slot = GREYHAWK_MAXSHIPS - 1;
  struct greyhawk_ship_data saved_first = greyhawk_ships[first_slot];
  struct greyhawk_ship_data saved_second = greyhawk_ships[second_slot];
  struct greyhawk_ship_data *first = &greyhawk_ships[first_slot];
  struct greyhawk_ship_data *second = &greyhawk_ships[second_slot];
  unsigned long saved_pulse = pulse;
  int saved_vessel_system = CONFIG_VESSEL_SYSTEM;

  event_free_all();
  vessel_periodic_reset_for_test();
  memset(first, 0, sizeof(*first));
  memset(second, 0, sizeof(*second));
  first->active = true;
  first->shipnum = first_slot;
  second->active = true;
  second->shipnum = second_slot;
  CONFIG_VESSEL_SYSTEM = 1;
  vessel_periodic_select_for_test(true);
  vessel_periodic_set_admission_limit_for_test(1U);
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER));
  pulse = 200U;
  event_init();
  vessel_periodic_init();
  vessel_periodic_sync(first);
  vessel_periodic_sync(second);

  CuAssertIntEquals(tc, 2, (int)vessel_periodic_owner_count());
  CuAssertIntEquals(tc, 1, (int)vessel_periodic_scheduled_count());
  CuAssertIntEquals(tc, 1, (int)vessel_periodic_admission_rejections());
  CuAssertTrue(tc, !event_runtime_handle_is_none(first->periodic_event_handle));
  CuAssertTrue(tc, event_runtime_handle_is_none(second->periodic_event_handle));
  CuAssertIntEquals(tc, 0, (int)vessel_periodic_registry_validate());

  vessel_periodic_forget(first);
  CuAssertIntEquals(tc, 1, (int)vessel_periodic_owner_count());
  CuAssertIntEquals(tc, 1, (int)vessel_periodic_scheduled_count());
  CuAssertTrue(tc, !event_runtime_handle_is_none(second->periodic_event_handle));
  CuAssertIntEquals(tc, 0, (int)vessel_periodic_registry_validate());

  vessel_periodic_reset_for_test();
  event_free_all();
  greyhawk_ships[first_slot] = saved_first;
  greyhawk_ships[second_slot] = saved_second;
  CONFIG_VESSEL_SYSTEM = saved_vessel_system;
  pulse = saved_pulse;
}

void TestVesselPeriodicFallsBackWhenServiceCannotStart(CuTest *tc)
{
  int saved_vessel_system = CONFIG_VESSEL_SYSTEM;

  event_free_all();
  vessel_periodic_reset_for_test();
  CONFIG_VESSEL_SYSTEM = 1;
  vessel_periodic_select_for_test(true);
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_LEGACY_QUEUE));
  event_init();
  vessel_periodic_init();

  CuAssertTrue(tc, !vessel_periodic_events_enabled());
  CuAssertIntEquals(tc, 0, event_queue_depth());

  vessel_periodic_reset_for_test();
  event_free_all();
  CONFIG_VESSEL_SYSTEM = saved_vessel_system;
}

void TestWorldPhenomenonRoomPropagation(CuTest *tc)
{
  struct room_data rooms[3];
  struct room_direction_data first_exit;
  struct room_direction_data second_exit;
  struct char_data adjacent;
  struct char_data distant;
  struct player_special_data adjacent_specials;
  struct player_special_data distant_specials;
  struct descriptor_data adjacent_desc;
  struct descriptor_data distant_desc;
  struct room_data *saved_world = world;
  room_rnum saved_top_of_world = top_of_world;
  struct domain_event_bus *bus = create_bus(tc, 4U, 16U, NULL, 100U);
  struct domain_world_phenomenon phenomenon;

  memset(rooms, 0, sizeof(rooms));
  memset(&first_exit, 0, sizeof(first_exit));
  memset(&second_exit, 0, sizeof(second_exit));
  memset(&adjacent_specials, 0, sizeof(adjacent_specials));
  memset(&distant_specials, 0, sizeof(distant_specials));
  memset(&adjacent_desc, 0, sizeof(adjacent_desc));
  memset(&distant_desc, 0, sizeof(distant_desc));
  memset(&phenomenon, 0, sizeof(phenomenon));
  clear_char(&adjacent);
  clear_char(&distant);

  rooms[0].number = 100;
  rooms[1].number = 200;
  rooms[2].number = 300;
  first_exit.to_room = 1;
  SET_BIT(first_exit.exit_info, EX_CLOSED);
  second_exit.to_room = 2;
  rooms[0].dir_option[EAST] = &first_exit;
  rooms[1].dir_option[EAST] = &second_exit;

  adjacent.player_specials = &adjacent_specials;
  adjacent.player.name = "Adjacent observer";
  adjacent.desc = &adjacent_desc;
  IN_ROOM(&adjacent) = 1;
  adjacent_desc.character = &adjacent;
  adjacent_desc.output = adjacent_desc.small_outbuf;
  adjacent_desc.bufspace = SMALL_BUFSIZE - 1;
  adjacent_desc.pProtocol = ProtocolCreate();
  STATE(&adjacent_desc) = CON_PLAYING;
  rooms[1].people = &adjacent;

  distant.player_specials = &distant_specials;
  distant.player.name = "Distant observer";
  distant.desc = &distant_desc;
  IN_ROOM(&distant) = 2;
  distant_desc.character = &distant;
  distant_desc.output = distant_desc.small_outbuf;
  distant_desc.bufspace = SMALL_BUFSIZE - 1;
  distant_desc.pProtocol = ProtocolCreate();
  STATE(&distant_desc) = CON_PLAYING;
  rooms[2].people = &distant;

  world = rooms;
  top_of_world = 2;
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_register_foundation_types(bus));
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_world_register_resolvers(bus));
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, spatial_event_register_handlers(bus));
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_seal(bus));

  phenomenon.source_room = domain_event_room_handle(0);
  phenomenon.visual_range = 2;
  phenomenon.audio_range = 2;
  phenomenon.minimum_range = 1;
  phenomenon.channels = DOMAIN_WORLD_PHENOMENON_VISUAL | DOMAIN_WORLD_PHENOMENON_AUDIBLE;
  phenomenon.propagation = DOMAIN_WORLD_PROPAGATE_ROOMS;
  phenomenon.visual_description = "A flash lights the next room.";
  phenomenon.audio_description = "A fireball explodes nearby.";
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    DOMAIN_EVENT_PUBLISH(bus, DOMAIN_EVENT_WORLD_PHENOMENON, &phenomenon));
  CuAssertPtrEquals(tc, NULL, strstr(adjacent_desc.output, phenomenon.visual_description));
  CuAssertPtrNotNull(tc, strstr(adjacent_desc.output, phenomenon.audio_description));
  CuAssertPtrNotNull(tc, strstr(distant_desc.output, phenomenon.audio_description));

  adjacent_desc.output[0] = '\0';
  adjacent_desc.bufptr = 0;
  adjacent_desc.bufspace = SMALL_BUFSIZE - 1;
  distant_desc.output[0] = '\0';
  distant_desc.bufptr = 0;
  distant_desc.bufspace = SMALL_BUFSIZE - 1;
  REMOVE_BIT(first_exit.exit_info, EX_CLOSED);
  phenomenon.channels = DOMAIN_WORLD_PHENOMENON_VISUAL;
  phenomenon.visual_range = 1;
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    DOMAIN_EVENT_PUBLISH(bus, DOMAIN_EVENT_WORLD_PHENOMENON, &phenomenon));
  CuAssertPtrNotNull(tc, strstr(adjacent_desc.output, phenomenon.visual_description));
  CuAssertPtrEquals(tc, NULL, strstr(distant_desc.output, phenomenon.visual_description));

  adjacent_desc.output[0] = '\0';
  adjacent_desc.bufptr = 0;
  adjacent_desc.bufspace = SMALL_BUFSIZE - 1;
  rooms[0].event_owner_generation++;
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    DOMAIN_EVENT_PUBLISH(bus, DOMAIN_EVENT_WORLD_PHENOMENON, &phenomenon));
  CuAssertPtrEquals(tc, NULL, strstr(adjacent_desc.output, phenomenon.visual_description));

  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_bus_destroy(bus));
  ProtocolDestroy(adjacent_desc.pProtocol);
  ProtocolDestroy(distant_desc.pProtocol);
  world = saved_world;
  top_of_world = saved_top_of_world;
}
