#include "domain_events.h"

#include <limits.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct domain_event_handler_entry
{
  char *identity;
  int priority;
  uint64_t registration_sequence;
  domain_event_handler handler;
  void *handler_context;
  uint64_t calls;
  uint64_t total_usec;
  uint64_t maximum_usec;
  uint64_t slow_calls;
  struct domain_event_handler_entry *next;
};

struct domain_event_type_entry
{
  domain_event_type_id_t type;
  char *name;
  size_t payload_size;
  size_t handler_count;
  uint64_t publications;
  uint64_t handler_calls;
  uint64_t rejected_publications;
  uint64_t total_handler_usec;
  uint64_t maximum_handler_usec;
  uint64_t slow_handler_calls;
  uint32_t maximum_depth;
  struct domain_event_handler_entry *handlers;
};

struct domain_event_resolver_entry
{
  domain_entity_resolver resolver;
  void *context;
};

struct domain_event_bus
{
  size_t max_event_types;
  size_t max_handlers;
  uint32_t max_depth;
  uint32_t max_causal_events;
  uint64_t slow_handler_usec;
  domain_event_usec_source monotonic_usec_now;
  void *clock_context;
  pthread_t owner_thread;
  struct domain_event_type_entry **type_buckets;
  size_t type_bucket_count;
  struct domain_event_type_entry **types;
  size_t type_count;
  size_t handler_count;
  uint64_t next_registration_sequence;
  struct domain_event_resolver_entry resolvers[DOMAIN_ENTITY_KIND_COUNT];
  uint64_t publications;
  uint64_t handler_calls;
  uint64_t rejected_causal_chains;
  uint64_t slow_handler_calls;
  uint32_t maximum_depth;
  uint32_t dispatch_depth;
  uint32_t causal_event_count;
  enum domain_event_status causal_failure;
  bool sealed;
};

static uint64_t default_monotonic_usec(void *context)
{
  struct timespec now;

  (void)context;
  if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
    return 0;
  return (uint64_t)now.tv_sec * UINT64_C(1000000) + (uint64_t)now.tv_nsec / UINT64_C(1000);
}

static uint64_t saturating_add(uint64_t left, uint64_t right)
{
  if (UINT64_MAX - left < right)
    return UINT64_MAX;
  return left + right;
}

static size_t next_power_of_two(size_t value)
{
  size_t result;

  result = 1U;
  while (result < value && result <= SIZE_MAX / 2U)
    result *= 2U;
  return result < value ? 0U : result;
}

static size_t type_bucket(const struct domain_event_bus *bus, domain_event_type_id_t type)
{
  uint32_t hash;

  hash = type * UINT32_C(2654435761);
  return (size_t)hash & (bus->type_bucket_count - 1U);
}

static struct domain_event_type_entry *find_type(const struct domain_event_bus *bus,
                                                 domain_event_type_id_t type)
{
  size_t start;
  size_t bucket;

  if (bus == NULL || type == 0 || bus->type_bucket_count == 0)
    return NULL;
  start = type_bucket(bus, type);
  bucket = start;
  do
  {
    struct domain_event_type_entry *entry = bus->type_buckets[bucket];

    if (entry == NULL)
      return NULL;
    if (entry->type == type)
      return entry;
    bucket = (bucket + 1U) & (bus->type_bucket_count - 1U);
  } while (bucket != start);
  return NULL;
}

static bool on_owner_thread(const struct domain_event_bus *bus)
{
  return bus != NULL && pthread_equal(bus->owner_thread, pthread_self()) != 0;
}

static char *duplicate_name(const char *name)
{
  size_t length;
  char *copy;

  if (name == NULL || *name == '\0')
    return NULL;
  length = strlen(name) + 1U;
  copy = malloc(length);
  if (copy != NULL)
    memcpy(copy, name, length);
  return copy;
}

static bool type_name_exists(const struct domain_event_bus *bus, const char *name)
{
  size_t index;

  for (index = 0; index < bus->type_count; index++)
  {
    if (strcmp(bus->types[index]->name, name) == 0)
      return true;
  }
  return false;
}

static enum domain_event_status reject_causal_chain(struct domain_event_bus *bus,
                                                    struct domain_event_type_entry *type,
                                                    enum domain_event_status status)
{
  type->rejected_publications = saturating_add(type->rejected_publications, 1U);
  if (bus->causal_failure == DOMAIN_EVENT_OK)
  {
    bus->causal_failure = status;
    bus->rejected_causal_chains = saturating_add(bus->rejected_causal_chains, 1U);
  }
  return status;
}

struct domain_entity_handle domain_entity_handle_none(void)
{
  struct domain_entity_handle handle;

  memset(&handle, 0, sizeof(handle));
  return handle;
}

bool domain_entity_handle_is_none(struct domain_entity_handle handle)
{
  return handle.kind == DOMAIN_ENTITY_NONE && handle.runtime_id == 0 && handle.generation == 0;
}

bool domain_entity_handle_is_valid(struct domain_entity_handle handle)
{
  return handle.kind > DOMAIN_ENTITY_NONE && handle.kind < DOMAIN_ENTITY_KIND_COUNT &&
         handle.runtime_id != 0 && handle.generation != 0;
}

bool domain_entity_handle_equal(struct domain_entity_handle left,
                                struct domain_entity_handle right)
{
  return left.kind == right.kind && left.runtime_id == right.runtime_id &&
         left.generation == right.generation;
}

struct domain_event_bus *domain_event_bus_create(const struct domain_event_bus_config *config,
                                                 enum domain_event_status *status)
{
  struct domain_event_bus_config effective;
  struct domain_event_bus *bus;
  size_t bucket_target;

  memset(&effective, 0, sizeof(effective));
  if (config != NULL)
    effective = *config;
  if (effective.max_event_types == 0)
    effective.max_event_types = DOMAIN_EVENT_DEFAULT_MAX_TYPES;
  if (effective.max_handlers == 0)
    effective.max_handlers = DOMAIN_EVENT_DEFAULT_MAX_HANDLERS;
  if (effective.max_depth == 0)
    effective.max_depth = DOMAIN_EVENT_DEFAULT_MAX_DEPTH;
  if (effective.max_causal_events == 0)
    effective.max_causal_events = DOMAIN_EVENT_DEFAULT_MAX_CAUSAL_EVENTS;
  if (effective.slow_handler_usec == 0)
    effective.slow_handler_usec = DOMAIN_EVENT_DEFAULT_SLOW_HANDLER_USEC;
  if (effective.monotonic_usec_now == NULL)
    effective.monotonic_usec_now = default_monotonic_usec;
  if (effective.max_event_types > SIZE_MAX / 2U)
  {
    if (status != NULL)
      *status = DOMAIN_EVENT_INVALID_ARGUMENT;
    return NULL;
  }
  bucket_target = next_power_of_two(effective.max_event_types * 2U);
  if (bucket_target == 0)
  {
    if (status != NULL)
      *status = DOMAIN_EVENT_INVALID_ARGUMENT;
    return NULL;
  }

  bus = calloc(1, sizeof(*bus));
  if (bus == NULL)
    goto allocation_failed;
  bus->types = calloc(effective.max_event_types, sizeof(*bus->types));
  bus->type_buckets = calloc(bucket_target, sizeof(*bus->type_buckets));
  if (bus->types == NULL || bus->type_buckets == NULL)
    goto allocation_failed;
  bus->max_event_types = effective.max_event_types;
  bus->max_handlers = effective.max_handlers;
  bus->max_depth = effective.max_depth;
  bus->max_causal_events = effective.max_causal_events;
  bus->slow_handler_usec = effective.slow_handler_usec;
  bus->monotonic_usec_now = effective.monotonic_usec_now;
  bus->clock_context = effective.clock_context;
  bus->owner_thread = pthread_self();
  bus->type_bucket_count = bucket_target;
  bus->next_registration_sequence = 1U;
  bus->causal_failure = DOMAIN_EVENT_OK;
  if (status != NULL)
    *status = DOMAIN_EVENT_OK;
  return bus;

allocation_failed:
  if (bus != NULL)
  {
    free(bus->type_buckets);
    free(bus->types);
    free(bus);
  }
  if (status != NULL)
    *status = DOMAIN_EVENT_ALLOCATION_FAILED;
  return NULL;
}

enum domain_event_status domain_event_bus_destroy(struct domain_event_bus *bus)
{
  size_t index;

  if (bus == NULL)
    return DOMAIN_EVENT_INVALID_ARGUMENT;
  if (!on_owner_thread(bus))
    return DOMAIN_EVENT_WRONG_THREAD;
  if (bus->dispatch_depth != 0)
    return DOMAIN_EVENT_BUSY;
  for (index = 0; index < bus->type_count; index++)
  {
    struct domain_event_type_entry *type = bus->types[index];
    struct domain_event_handler_entry *handler = type->handlers;

    while (handler != NULL)
    {
      struct domain_event_handler_entry *next = handler->next;

      free(handler->identity);
      free(handler);
      handler = next;
    }
    free(type->name);
    free(type);
  }
  free(bus->type_buckets);
  free(bus->types);
  free(bus);
  return DOMAIN_EVENT_OK;
}

enum domain_event_status domain_event_register_type(
    struct domain_event_bus *bus, const struct domain_event_type_config *config)
{
  struct domain_event_type_entry *entry;
  size_t bucket;

  if (bus == NULL || config == NULL || config->type == 0 || config->name == NULL ||
      *config->name == '\0' || config->payload_size == 0)
    return DOMAIN_EVENT_INVALID_ARGUMENT;
  if (!on_owner_thread(bus))
    return DOMAIN_EVENT_WRONG_THREAD;
  if (bus->sealed)
    return DOMAIN_EVENT_REGISTRY_SEALED;
  if (find_type(bus, config->type) != NULL || type_name_exists(bus, config->name))
    return DOMAIN_EVENT_DUPLICATE_TYPE;
  if (bus->type_count >= bus->max_event_types)
    return DOMAIN_EVENT_TYPE_CAPACITY_REACHED;
  entry = calloc(1, sizeof(*entry));
  if (entry == NULL)
    return DOMAIN_EVENT_ALLOCATION_FAILED;
  entry->name = duplicate_name(config->name);
  if (entry->name == NULL)
  {
    free(entry);
    return DOMAIN_EVENT_ALLOCATION_FAILED;
  }
  entry->type = config->type;
  entry->payload_size = config->payload_size;
  bucket = type_bucket(bus, entry->type);
  while (bus->type_buckets[bucket] != NULL)
    bucket = (bucket + 1U) & (bus->type_bucket_count - 1U);
  bus->type_buckets[bucket] = entry;
  bus->types[bus->type_count++] = entry;
  return DOMAIN_EVENT_OK;
}

enum domain_event_status domain_event_register_handler(
    struct domain_event_bus *bus, const struct domain_event_handler_config *config)
{
  struct domain_event_type_entry *type;
  struct domain_event_handler_entry **cursor;
  struct domain_event_handler_entry *handler;

  if (bus == NULL || config == NULL || config->identity == NULL || *config->identity == '\0' ||
      config->handler == NULL)
    return DOMAIN_EVENT_INVALID_ARGUMENT;
  if (!on_owner_thread(bus))
    return DOMAIN_EVENT_WRONG_THREAD;
  if (bus->sealed)
    return DOMAIN_EVENT_REGISTRY_SEALED;
  type = find_type(bus, config->type);
  if (type == NULL)
    return DOMAIN_EVENT_UNKNOWN_TYPE;
  for (handler = type->handlers; handler != NULL; handler = handler->next)
  {
    if (strcmp(handler->identity, config->identity) == 0)
      return DOMAIN_EVENT_DUPLICATE_HANDLER;
  }
  if (bus->handler_count >= bus->max_handlers)
    return DOMAIN_EVENT_HANDLER_CAPACITY_REACHED;
  handler = calloc(1, sizeof(*handler));
  if (handler == NULL)
    return DOMAIN_EVENT_ALLOCATION_FAILED;
  handler->identity = duplicate_name(config->identity);
  if (handler->identity == NULL)
  {
    free(handler);
    return DOMAIN_EVENT_ALLOCATION_FAILED;
  }
  handler->priority = config->priority;
  handler->registration_sequence = bus->next_registration_sequence++;
  handler->handler = config->handler;
  handler->handler_context = config->handler_context;
  cursor = &type->handlers;
  while (*cursor != NULL &&
         ((*cursor)->priority < handler->priority ||
          ((*cursor)->priority == handler->priority &&
           (*cursor)->registration_sequence < handler->registration_sequence)))
    cursor = &(*cursor)->next;
  handler->next = *cursor;
  *cursor = handler;
  type->handler_count++;
  bus->handler_count++;
  return DOMAIN_EVENT_OK;
}

enum domain_event_status domain_event_register_resolver(struct domain_event_bus *bus,
                                                        enum domain_entity_kind kind,
                                                        domain_entity_resolver resolver,
                                                        void *resolver_context)
{
  if (bus == NULL || kind <= DOMAIN_ENTITY_NONE || kind >= DOMAIN_ENTITY_KIND_COUNT ||
      resolver == NULL)
    return DOMAIN_EVENT_INVALID_ARGUMENT;
  if (!on_owner_thread(bus))
    return DOMAIN_EVENT_WRONG_THREAD;
  if (bus->sealed)
    return DOMAIN_EVENT_REGISTRY_SEALED;
  if (bus->resolvers[kind].resolver != NULL)
    return DOMAIN_EVENT_DUPLICATE_RESOLVER;
  bus->resolvers[kind].resolver = resolver;
  bus->resolvers[kind].context = resolver_context;
  return DOMAIN_EVENT_OK;
}

enum domain_event_status domain_event_seal(struct domain_event_bus *bus)
{
  if (bus == NULL)
    return DOMAIN_EVENT_INVALID_ARGUMENT;
  if (!on_owner_thread(bus))
    return DOMAIN_EVENT_WRONG_THREAD;
  if (bus->sealed)
    return DOMAIN_EVENT_REGISTRY_SEALED;
  bus->sealed = true;
  return DOMAIN_EVENT_OK;
}

enum domain_event_status domain_event_publish(struct domain_event_bus *bus,
                                              domain_event_type_id_t type_id,
                                              const void *payload, size_t payload_size)
{
  struct domain_event_type_entry *type;
  struct domain_event_handler_entry *handler;
  struct domain_event_context context;
  enum domain_event_status result;
  bool root;

  if (bus == NULL || payload == NULL)
    return DOMAIN_EVENT_INVALID_ARGUMENT;
  if (!on_owner_thread(bus))
    return DOMAIN_EVENT_WRONG_THREAD;
  if (!bus->sealed)
    return DOMAIN_EVENT_REGISTRY_NOT_SEALED;
  type = find_type(bus, type_id);
  if (type == NULL)
    return DOMAIN_EVENT_UNKNOWN_TYPE;
  if (payload_size != type->payload_size)
    return DOMAIN_EVENT_PAYLOAD_SIZE_MISMATCH;
  root = bus->dispatch_depth == 0;
  if (root)
  {
    bus->causal_event_count = 0;
    bus->causal_failure = DOMAIN_EVENT_OK;
  }
  if (bus->causal_failure != DOMAIN_EVENT_OK)
    return bus->causal_failure;
  if (bus->dispatch_depth >= bus->max_depth)
    return reject_causal_chain(bus, type, DOMAIN_EVENT_NESTING_LIMIT_REACHED);
  if (bus->causal_event_count >= bus->max_causal_events)
    return reject_causal_chain(bus, type, DOMAIN_EVENT_CAUSAL_LIMIT_REACHED);

  bus->dispatch_depth++;
  bus->causal_event_count++;
  bus->publications = saturating_add(bus->publications, 1U);
  type->publications = saturating_add(type->publications, 1U);
  if (bus->dispatch_depth > bus->maximum_depth)
    bus->maximum_depth = bus->dispatch_depth;
  if (bus->dispatch_depth > type->maximum_depth)
    type->maximum_depth = bus->dispatch_depth;
  context.bus = bus;
  context.type = type_id;
  context.payload = payload;
  context.payload_size = payload_size;
  context.depth = bus->dispatch_depth;
  context.causal_sequence = bus->causal_event_count;

  for (handler = type->handlers; handler != NULL && bus->causal_failure == DOMAIN_EVENT_OK;
       handler = handler->next)
  {
    uint64_t before;
    uint64_t after;
    uint64_t elapsed;

    before = bus->monotonic_usec_now(bus->clock_context);
    handler->handler(&context, handler->handler_context);
    after = bus->monotonic_usec_now(bus->clock_context);
    elapsed = after >= before ? after - before : 0;
    handler->calls = saturating_add(handler->calls, 1U);
    handler->total_usec = saturating_add(handler->total_usec, elapsed);
    if (elapsed > handler->maximum_usec)
      handler->maximum_usec = elapsed;
    type->handler_calls = saturating_add(type->handler_calls, 1U);
    type->total_handler_usec = saturating_add(type->total_handler_usec, elapsed);
    if (elapsed > type->maximum_handler_usec)
      type->maximum_handler_usec = elapsed;
    bus->handler_calls = saturating_add(bus->handler_calls, 1U);
    if (elapsed >= bus->slow_handler_usec)
    {
      handler->slow_calls = saturating_add(handler->slow_calls, 1U);
      type->slow_handler_calls = saturating_add(type->slow_handler_calls, 1U);
      bus->slow_handler_calls = saturating_add(bus->slow_handler_calls, 1U);
    }
  }
  bus->dispatch_depth--;
  result = bus->causal_failure;
  if (root)
  {
    bus->causal_event_count = 0;
    bus->causal_failure = DOMAIN_EVENT_OK;
  }
  return result;
}

void *domain_event_resolve(struct domain_event_bus *bus, struct domain_entity_handle handle,
                           enum domain_entity_kind expected_kind)
{
  struct domain_event_resolver_entry *resolver;

  if (bus == NULL || !on_owner_thread(bus) || !domain_entity_handle_is_valid(handle) ||
      expected_kind <= DOMAIN_ENTITY_NONE || expected_kind >= DOMAIN_ENTITY_KIND_COUNT ||
      handle.kind != expected_kind)
    return NULL;
  resolver = &bus->resolvers[expected_kind];
  if (resolver->resolver == NULL)
    return NULL;
  return resolver->resolver(handle, resolver->context);
}

void domain_event_bus_get_stats(const struct domain_event_bus *bus,
                                struct domain_event_bus_stats *stats)
{
  if (stats == NULL)
    return;
  memset(stats, 0, sizeof(*stats));
  if (bus == NULL)
    return;
  stats->registered_type_count = bus->type_count;
  stats->registered_handler_count = bus->handler_count;
  stats->publications = bus->publications;
  stats->handler_calls = bus->handler_calls;
  stats->rejected_causal_chains = bus->rejected_causal_chains;
  stats->slow_handler_calls = bus->slow_handler_calls;
  stats->maximum_depth = bus->maximum_depth;
  stats->sealed = bus->sealed;
  stats->dispatching = bus->dispatch_depth != 0;
}

enum domain_event_status domain_event_get_type_stats(const struct domain_event_bus *bus,
                                                     domain_event_type_id_t type_id,
                                                     struct domain_event_type_stats *stats)
{
  struct domain_event_type_entry *type;

  if (bus == NULL || stats == NULL)
    return DOMAIN_EVENT_INVALID_ARGUMENT;
  type = find_type(bus, type_id);
  if (type == NULL)
    return DOMAIN_EVENT_UNKNOWN_TYPE;
  stats->type = type->type;
  stats->name = type->name;
  stats->payload_size = type->payload_size;
  stats->handler_count = type->handler_count;
  stats->publications = type->publications;
  stats->handler_calls = type->handler_calls;
  stats->rejected_publications = type->rejected_publications;
  stats->total_handler_usec = type->total_handler_usec;
  stats->maximum_handler_usec = type->maximum_handler_usec;
  stats->slow_handler_calls = type->slow_handler_calls;
  stats->maximum_depth = type->maximum_depth;
  return DOMAIN_EVENT_OK;
}

enum domain_event_status domain_event_get_handler_stats(
    const struct domain_event_bus *bus, domain_event_type_id_t type_id, const char *identity,
    struct domain_event_handler_stats *stats)
{
  struct domain_event_type_entry *type;
  struct domain_event_handler_entry *handler;

  if (bus == NULL || identity == NULL || stats == NULL)
    return DOMAIN_EVENT_INVALID_ARGUMENT;
  type = find_type(bus, type_id);
  if (type == NULL)
    return DOMAIN_EVENT_UNKNOWN_TYPE;
  for (handler = type->handlers; handler != NULL; handler = handler->next)
  {
    if (strcmp(handler->identity, identity) != 0)
      continue;
    stats->type = type_id;
    stats->identity = handler->identity;
    stats->priority = handler->priority;
    stats->registration_sequence = handler->registration_sequence;
    stats->calls = handler->calls;
    stats->total_usec = handler->total_usec;
    stats->maximum_usec = handler->maximum_usec;
    stats->slow_calls = handler->slow_calls;
    return DOMAIN_EVENT_OK;
  }
  return DOMAIN_EVENT_NOT_FOUND;
}

const char *domain_event_status_name(enum domain_event_status status)
{
  static const char *const names[] = {
      "ok",
      "invalid argument",
      "wrong thread",
      "registry sealed",
      "registry not sealed",
      "duplicate type",
      "duplicate handler",
      "duplicate resolver",
      "type capacity reached",
      "handler capacity reached",
      "allocation failed",
      "unknown type",
      "payload size mismatch",
      "nesting limit reached",
      "causal limit reached",
      "busy",
      "not found",
  };

  if (status < DOMAIN_EVENT_OK || (size_t)status >= sizeof(names) / sizeof(names[0]))
    return "unknown status";
  return names[status];
}
