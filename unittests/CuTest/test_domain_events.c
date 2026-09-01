#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/db.h"
#include "../../src/domain_event_types.h"
#include "../../src/domain_events.h"
#include "../../src/domain_event_runtime.h"
#include "../../src/domain_event_world.h"
#include "../../src/net/protocol.h"
#include "../../src/wilderness/spatial_core.h"
#include "../../src/wilderness/spatial_events.h"

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

static int test_spatial_calculate_intensity(struct spatial_context *context)
{
  context->distance_attenuation = 0.5f;
  return SPATIAL_SUCCESS;
}

static int test_spatial_calculate_obstruction(struct spatial_context *context,
                                              float *obstruction_factor)
{
  (void)context;
  *obstruction_factor = 0.0f;
  return SPATIAL_SUCCESS;
}

static int test_spatial_apply_modifiers(struct spatial_context *context, float *range_modifier,
                                        float *clarity_modifier)
{
  (void)context;
  *range_modifier = 1.0f;
  *clarity_modifier = 1.0f;
  return SPATIAL_SUCCESS;
}

static int test_spatial_generate_message(struct spatial_context *context, char *output,
                                         size_t output_size)
{
  (void)context;
  snprintf(output, output_size, "test spatial message");
  return SPATIAL_SUCCESS;
}

static int test_spatial_modify_message(struct spatial_context *context, char *message,
                                       size_t message_size)
{
  (void)context;
  (void)message;
  (void)message_size;
  return SPATIAL_SUCCESS;
}

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
  CuAssertIntEquals(tc, 9, (int)stats.registered_type_count);
  CuAssertIntEquals(tc, 1, (int)stats.registered_handler_count);
  CuAssertTrue(tc, stats.sealed);
  CuAssertIntEquals(tc, DOMAIN_EVENT_BUSY, domain_event_runtime_init());
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_shutdown());
  CuAssertPtrEquals(tc, NULL, domain_event_runtime_bus());
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_shutdown());
}

void TestSpatialIntensityPreservesSourceStrengthAcrossObservers(CuTest *tc)
{
  struct stimulus_strategy stimulus;
  struct los_strategy line_of_sight;
  struct modifier_strategy modifiers;
  struct spatial_system system;
  struct spatial_context context;
  char message[SPATIAL_MAX_MESSAGE_LENGTH];

  memset(&stimulus, 0, sizeof(stimulus));
  memset(&line_of_sight, 0, sizeof(line_of_sight));
  memset(&modifiers, 0, sizeof(modifiers));
  memset(&system, 0, sizeof(system));
  memset(&context, 0, sizeof(context));
  memset(message, 0, sizeof(message));

  stimulus.base_range = 1000.0f;
  stimulus.calculate_intensity = test_spatial_calculate_intensity;
  stimulus.generate_base_message = test_spatial_generate_message;
  line_of_sight.calculate_obstruction = test_spatial_calculate_obstruction;
  modifiers.apply_environmental_modifiers = test_spatial_apply_modifiers;
  modifiers.modify_message = test_spatial_modify_message;
  system.system_name = "test spatial intensity";
  system.stimulus = &stimulus;
  system.line_of_sight = &line_of_sight;
  system.modifiers = &modifiers;
  system.enabled = true;
  system.global_range_multiplier = 1.0f;
  system.global_intensity_multiplier = 1.0f;
  context.base_intensity = 2.0f;
  context.processed_message = message;

  spatial_shutdown_system();
  CuAssertIntEquals(tc, SPATIAL_SUCCESS, spatial_init_system());
  CuAssertIntEquals(tc, SPATIAL_SUCCESS, spatial_process_stimulus(&context, &system));
  CuAssertDblEquals(tc, 2.0, context.base_intensity, 0.001);
  CuAssertDblEquals(tc, 1.0, context.final_intensity, 0.001);

  context.observer_x = 10;
  CuAssertIntEquals(tc, SPATIAL_SUCCESS, spatial_process_stimulus(&context, &system));
  CuAssertDblEquals(tc, 2.0, context.base_intensity, 0.001);
  CuAssertDblEquals(tc, 1.0, context.final_intensity, 0.001);
  spatial_shutdown_system();
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
