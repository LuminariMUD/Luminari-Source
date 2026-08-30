#include "CuTest.h"

#include "../../src/domain_event_types.h"
#include "../../src/domain_events.h"
#include "../../src/domain_event_runtime.h"

#include <pthread.h>
#include <stdint.h>
#include <string.h>

#define TEST_EVENT_OUTER UINT32_C(0x2001)
#define TEST_EVENT_INNER UINT32_C(0x2002)

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
  CuAssertIntEquals(tc, 8, (int)bus_stats.registered_type_count);
  CuAssertStrEquals(tc, "ActivityTransitioned", stats.name);
  CuAssertIntEquals(tc, (int)sizeof(struct domain_activity_transitioned),
                    (int)stats.payload_size);
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

  domain_event_runtime_shutdown();
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_init());
  CuAssertPtrNotNull(tc, domain_event_runtime_bus());
  domain_event_bus_get_stats(domain_event_runtime_bus(), &stats);
  CuAssertIntEquals(tc, 8, (int)stats.registered_type_count);
  CuAssertTrue(tc, stats.sealed);
  CuAssertIntEquals(tc, DOMAIN_EVENT_BUSY, domain_event_runtime_init());
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_shutdown());
  CuAssertPtrEquals(tc, NULL, domain_event_runtime_bus());
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_shutdown());
}
