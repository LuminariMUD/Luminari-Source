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

struct domain_event_subscription_entry
{
  struct domain_event_subscription_handle handle;
  domain_event_type_id_t type;
  struct domain_event_topic topic;
  struct domain_entity_handle owner;
  char *identity;
  int priority;
  unsigned int flags;
  uint64_t registration_sequence;
  domain_event_handler handler;
  void *handler_context;
  domain_event_subscription_cleanup cleanup;
  uint64_t calls;
  uint64_t total_usec;
  uint64_t maximum_usec;
  uint64_t slow_calls;
  bool active;
  struct domain_event_subscription_entry *topic_next;
  struct domain_event_subscription_entry *owner_next;
  struct domain_event_subscription_entry *handle_next;
  struct domain_event_subscription_entry *all_prev;
  struct domain_event_subscription_entry *all_next;
  struct domain_event_subscription_entry *retired_next;
};

struct domain_event_bus
{
  size_t max_event_types;
  size_t max_handlers;
  size_t max_subscriptions;
  size_t max_subscriptions_per_owner;
  size_t max_subscriptions_per_topic;
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
  struct domain_event_subscription_entry **subscription_topic_buckets;
  struct domain_event_subscription_entry **subscription_owner_buckets;
  struct domain_event_subscription_entry **subscription_handle_buckets;
  size_t subscription_bucket_count;
  struct domain_event_subscription_entry *subscriptions_head;
  struct domain_event_subscription_entry *subscriptions_tail;
  struct domain_event_subscription_entry *retired_subscriptions;
  size_t subscription_count;
  size_t subscription_high_water;
  uint64_t next_subscription_id;
  uint64_t subscription_deliveries;
  uint64_t subscription_cancellations;
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

static uint64_t mix_u64(uint64_t value)
{
  value ^= value >> 30U;
  value *= UINT64_C(0xbf58476d1ce4e5b9);
  value ^= value >> 27U;
  value *= UINT64_C(0x94d049bb133111eb);
  return value ^ (value >> 31U);
}

static uint64_t entity_hash(struct domain_entity_handle entity)
{
  uint64_t value = entity.runtime_id;

  value ^= entity.generation + UINT64_C(0x9e3779b97f4a7c15) + (value << 6U) +
           (value >> 2U);
  value ^= (uint64_t)entity.kind * UINT64_C(0x517cc1b727220a95);
  return mix_u64(value);
}

static size_t subscription_topic_bucket(const struct domain_event_bus *bus,
                                        domain_event_type_id_t type,
                                        struct domain_event_topic topic)
{
  uint64_t value = entity_hash(topic.entity);

  value ^= (uint64_t)type * UINT64_C(0x9e3779b185ebca87);
  value ^= (uint64_t)topic.role * UINT64_C(0xc2b2ae3d27d4eb4f);
  return (size_t)mix_u64(value) & (bus->subscription_bucket_count - 1U);
}

static size_t subscription_owner_bucket(const struct domain_event_bus *bus,
                                        struct domain_entity_handle owner)
{
  return (size_t)entity_hash(owner) & (bus->subscription_bucket_count - 1U);
}

static size_t subscription_handle_bucket(const struct domain_event_bus *bus, uint64_t id)
{
  return (size_t)mix_u64(id) & (bus->subscription_bucket_count - 1U);
}

static bool topic_equal(struct domain_event_topic left, struct domain_event_topic right)
{
  return left.role == right.role && domain_entity_handle_equal(left.entity, right.entity);
}

static bool topic_is_valid(struct domain_event_topic topic)
{
  if (topic.role == DOMAIN_EVENT_TOPIC_ANY)
    return domain_entity_handle_is_none(topic.entity);
  return topic.role >= DOMAIN_EVENT_TOPIC_SUBJECT && topic.role <= DOMAIN_EVENT_TOPIC_OWNER &&
         domain_entity_handle_is_valid(topic.entity);
}

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

static void cleanup_subscription(struct domain_event_subscription_entry *subscription)
{
  if (subscription->cleanup != NULL)
    subscription->cleanup(subscription->handler_context);
  free(subscription->identity);
  free(subscription);
}

static void reclaim_retired_subscriptions(struct domain_event_bus *bus)
{
  struct domain_event_subscription_entry *subscription;

  if (bus->dispatch_depth != 0)
    return;
  subscription = bus->retired_subscriptions;
  bus->retired_subscriptions = NULL;
  while (subscription != NULL)
  {
    struct domain_event_subscription_entry *next = subscription->retired_next;

    cleanup_subscription(subscription);
    subscription = next;
  }
}

static struct domain_event_subscription_entry *find_subscription(
    const struct domain_event_bus *bus, struct domain_event_subscription_handle handle)
{
  struct domain_event_subscription_entry *subscription;
  size_t bucket;

  if (bus == NULL || handle.id == 0 || handle.generation == 0)
    return NULL;
  bucket = subscription_handle_bucket(bus, handle.id);
  for (subscription = bus->subscription_handle_buckets[bucket]; subscription != NULL;
       subscription = subscription->handle_next)
    if (subscription->active && subscription->handle.id == handle.id &&
        subscription->handle.generation == handle.generation)
      return subscription;
  return NULL;
}

static void unlink_subscription(struct domain_event_bus *bus,
                                struct domain_event_subscription_entry *subscription)
{
  struct domain_event_subscription_entry **cursor;
  size_t bucket;

  if (!subscription->active)
    return;
  bucket = subscription_topic_bucket(bus, subscription->type, subscription->topic);
  cursor = &bus->subscription_topic_buckets[bucket];
  while (*cursor != NULL && *cursor != subscription)
    cursor = &(*cursor)->topic_next;
  if (*cursor == subscription)
    *cursor = subscription->topic_next;

  bucket = subscription_owner_bucket(bus, subscription->owner);
  cursor = &bus->subscription_owner_buckets[bucket];
  while (*cursor != NULL && *cursor != subscription)
    cursor = &(*cursor)->owner_next;
  if (*cursor == subscription)
    *cursor = subscription->owner_next;

  bucket = subscription_handle_bucket(bus, subscription->handle.id);
  cursor = &bus->subscription_handle_buckets[bucket];
  while (*cursor != NULL && *cursor != subscription)
    cursor = &(*cursor)->handle_next;
  if (*cursor == subscription)
    *cursor = subscription->handle_next;

  if (subscription->all_prev != NULL)
    subscription->all_prev->all_next = subscription->all_next;
  else
    bus->subscriptions_head = subscription->all_next;
  if (subscription->all_next != NULL)
    subscription->all_next->all_prev = subscription->all_prev;
  else
    bus->subscriptions_tail = subscription->all_prev;
  subscription->active = false;
  bus->subscription_count--;
  bus->subscription_cancellations =
      saturating_add(bus->subscription_cancellations, 1U);

  if (bus->dispatch_depth != 0)
  {
    subscription->retired_next = bus->retired_subscriptions;
    bus->retired_subscriptions = subscription;
  }
  else
    cleanup_subscription(subscription);
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
  if (effective.max_subscriptions == 0)
    effective.max_subscriptions = DOMAIN_EVENT_DEFAULT_MAX_SUBSCRIPTIONS;
  if (effective.max_subscriptions_per_owner == 0)
    effective.max_subscriptions_per_owner =
        effective.max_subscriptions < DOMAIN_EVENT_DEFAULT_MAX_SUBSCRIPTIONS_PER_OWNER
            ? effective.max_subscriptions
            : DOMAIN_EVENT_DEFAULT_MAX_SUBSCRIPTIONS_PER_OWNER;
  if (effective.max_subscriptions_per_topic == 0)
    effective.max_subscriptions_per_topic =
        effective.max_subscriptions < DOMAIN_EVENT_DEFAULT_MAX_SUBSCRIPTIONS_PER_TOPIC
            ? effective.max_subscriptions
            : DOMAIN_EVENT_DEFAULT_MAX_SUBSCRIPTIONS_PER_TOPIC;
  if (effective.max_depth == 0)
    effective.max_depth = DOMAIN_EVENT_DEFAULT_MAX_DEPTH;
  if (effective.max_causal_events == 0)
    effective.max_causal_events = DOMAIN_EVENT_DEFAULT_MAX_CAUSAL_EVENTS;
  if (effective.slow_handler_usec == 0)
    effective.slow_handler_usec = DOMAIN_EVENT_DEFAULT_SLOW_HANDLER_USEC;
  if (effective.monotonic_usec_now == NULL)
    effective.monotonic_usec_now = default_monotonic_usec;
  if (effective.max_event_types > SIZE_MAX / 2U ||
      effective.max_subscriptions > SIZE_MAX / 2U ||
      effective.max_subscriptions_per_owner > effective.max_subscriptions ||
      effective.max_subscriptions_per_topic > effective.max_subscriptions)
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
  bus->subscription_bucket_count =
      next_power_of_two(effective.max_subscriptions * 2U);
  if (bus->subscription_bucket_count == 0)
    goto allocation_failed;
  bus->subscription_topic_buckets =
      calloc(bus->subscription_bucket_count, sizeof(*bus->subscription_topic_buckets));
  bus->subscription_owner_buckets =
      calloc(bus->subscription_bucket_count, sizeof(*bus->subscription_owner_buckets));
  bus->subscription_handle_buckets =
      calloc(bus->subscription_bucket_count, sizeof(*bus->subscription_handle_buckets));
  if (bus->types == NULL || bus->type_buckets == NULL ||
      bus->subscription_topic_buckets == NULL ||
      bus->subscription_owner_buckets == NULL ||
      bus->subscription_handle_buckets == NULL)
    goto allocation_failed;
  bus->max_event_types = effective.max_event_types;
  bus->max_handlers = effective.max_handlers;
  bus->max_subscriptions = effective.max_subscriptions;
  bus->max_subscriptions_per_owner = effective.max_subscriptions_per_owner;
  bus->max_subscriptions_per_topic = effective.max_subscriptions_per_topic;
  bus->max_depth = effective.max_depth;
  bus->max_causal_events = effective.max_causal_events;
  bus->slow_handler_usec = effective.slow_handler_usec;
  bus->monotonic_usec_now = effective.monotonic_usec_now;
  bus->clock_context = effective.clock_context;
  bus->owner_thread = pthread_self();
  bus->type_bucket_count = bucket_target;
  bus->next_registration_sequence = 1U;
  bus->next_subscription_id = 1U;
  bus->causal_failure = DOMAIN_EVENT_OK;
  if (status != NULL)
    *status = DOMAIN_EVENT_OK;
  return bus;

allocation_failed:
  if (bus != NULL)
  {
    free(bus->type_buckets);
    free(bus->types);
    free(bus->subscription_topic_buckets);
    free(bus->subscription_owner_buckets);
    free(bus->subscription_handle_buckets);
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
  while (bus->subscriptions_head != NULL)
    unlink_subscription(bus, bus->subscriptions_head);
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
  free(bus->subscription_topic_buckets);
  free(bus->subscription_owner_buckets);
  free(bus->subscription_handle_buckets);
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

enum domain_event_status domain_event_subscribe(
    struct domain_event_bus *bus, const struct domain_event_subscription_config *config,
    struct domain_event_subscription_handle *handle)
{
  struct domain_event_subscription_entry *subscription;
  struct domain_event_subscription_entry **cursor;
  size_t owner_count = 0U;
  size_t topic_count = 0U;
  size_t bucket;

  if (handle != NULL)
    *handle = domain_event_subscription_handle_none();
  if (bus == NULL || config == NULL || handle == NULL || config->type == 0 ||
      config->identity == NULL || *config->identity == '\0' || config->handler == NULL ||
      !topic_is_valid(config->topic) || !domain_entity_handle_is_valid(config->owner) ||
      (config->flags & ~DOMAIN_EVENT_SUBSCRIPTION_ONCE) != 0U)
    return DOMAIN_EVENT_INVALID_ARGUMENT;
  if (!on_owner_thread(bus))
    return DOMAIN_EVENT_WRONG_THREAD;
  if (!bus->sealed)
    return DOMAIN_EVENT_REGISTRY_NOT_SEALED;
  if (bus->dispatch_depth != 0)
    return DOMAIN_EVENT_BUSY;
  if (find_type(bus, config->type) == NULL)
    return DOMAIN_EVENT_UNKNOWN_TYPE;
  if (bus->subscription_count >= bus->max_subscriptions ||
      bus->next_subscription_id == UINT64_MAX)
    return DOMAIN_EVENT_SUBSCRIPTION_CAPACITY_REACHED;

  bucket = subscription_owner_bucket(bus, config->owner);
  for (subscription = bus->subscription_owner_buckets[bucket]; subscription != NULL;
       subscription = subscription->owner_next)
    if (subscription->active && domain_entity_handle_equal(subscription->owner, config->owner))
      owner_count++;
  if (owner_count >= bus->max_subscriptions_per_owner)
    return DOMAIN_EVENT_OWNER_CAPACITY_REACHED;

  bucket = subscription_topic_bucket(bus, config->type, config->topic);
  for (subscription = bus->subscription_topic_buckets[bucket]; subscription != NULL;
       subscription = subscription->topic_next)
    if (subscription->active && subscription->type == config->type &&
        topic_equal(subscription->topic, config->topic))
      topic_count++;
  if (topic_count >= bus->max_subscriptions_per_topic)
    return DOMAIN_EVENT_TOPIC_CAPACITY_REACHED;

  subscription = calloc(1, sizeof(*subscription));
  if (subscription == NULL)
    return DOMAIN_EVENT_ALLOCATION_FAILED;
  subscription->identity = duplicate_name(config->identity);
  if (subscription->identity == NULL)
  {
    free(subscription);
    return DOMAIN_EVENT_ALLOCATION_FAILED;
  }
  subscription->handle.id = bus->next_subscription_id++;
  subscription->handle.generation = subscription->handle.id;
  subscription->type = config->type;
  subscription->topic = config->topic;
  subscription->owner = config->owner;
  subscription->priority = config->priority;
  subscription->flags = config->flags;
  subscription->registration_sequence = bus->next_registration_sequence++;
  subscription->handler = config->handler;
  subscription->handler_context = config->handler_context;
  subscription->cleanup = config->cleanup;
  subscription->active = true;

  cursor = &bus->subscription_topic_buckets[bucket];
  while (*cursor != NULL &&
         ((*cursor)->priority < subscription->priority ||
          ((*cursor)->priority == subscription->priority &&
           (*cursor)->registration_sequence < subscription->registration_sequence)))
    cursor = &(*cursor)->topic_next;
  subscription->topic_next = *cursor;
  *cursor = subscription;

  bucket = subscription_owner_bucket(bus, subscription->owner);
  subscription->owner_next = bus->subscription_owner_buckets[bucket];
  bus->subscription_owner_buckets[bucket] = subscription;
  bucket = subscription_handle_bucket(bus, subscription->handle.id);
  subscription->handle_next = bus->subscription_handle_buckets[bucket];
  bus->subscription_handle_buckets[bucket] = subscription;
  subscription->all_prev = bus->subscriptions_tail;
  if (bus->subscriptions_tail != NULL)
    bus->subscriptions_tail->all_next = subscription;
  else
    bus->subscriptions_head = subscription;
  bus->subscriptions_tail = subscription;
  bus->subscription_count++;
  if (bus->subscription_count > bus->subscription_high_water)
    bus->subscription_high_water = bus->subscription_count;
  *handle = subscription->handle;
  return DOMAIN_EVENT_OK;
}

enum domain_event_status domain_event_unsubscribe(
    struct domain_event_bus *bus, struct domain_event_subscription_handle handle)
{
  struct domain_event_subscription_entry *subscription;

  if (bus == NULL || domain_event_subscription_handle_is_none(handle))
    return DOMAIN_EVENT_INVALID_ARGUMENT;
  if (!on_owner_thread(bus))
    return DOMAIN_EVENT_WRONG_THREAD;
  subscription = find_subscription(bus, handle);
  if (subscription == NULL)
    return DOMAIN_EVENT_NOT_FOUND;
  unlink_subscription(bus, subscription);
  reclaim_retired_subscriptions(bus);
  return DOMAIN_EVENT_OK;
}

enum domain_event_status domain_event_unsubscribe_owner(
    struct domain_event_bus *bus, struct domain_entity_handle owner, size_t *cancelled_count)
{
  struct domain_event_subscription_entry *subscription;
  size_t cancelled = 0U;
  size_t bucket;

  if (cancelled_count != NULL)
    *cancelled_count = 0U;
  if (bus == NULL || !domain_entity_handle_is_valid(owner))
    return DOMAIN_EVENT_INVALID_ARGUMENT;
  if (!on_owner_thread(bus))
    return DOMAIN_EVENT_WRONG_THREAD;
  bucket = subscription_owner_bucket(bus, owner);
  subscription = bus->subscription_owner_buckets[bucket];
  while (subscription != NULL)
  {
    struct domain_event_subscription_entry *next = subscription->owner_next;

    if (subscription->active && domain_entity_handle_equal(subscription->owner, owner))
    {
      unlink_subscription(bus, subscription);
      cancelled++;
    }
    subscription = next;
  }
  reclaim_retired_subscriptions(bus);
  if (cancelled_count != NULL)
    *cancelled_count = cancelled;
  return DOMAIN_EVENT_OK;
}

static void deliver_subscriptions(struct domain_event_bus *bus,
                                  struct domain_event_type_entry *type,
                                  const struct domain_event_context *context,
                                  struct domain_event_topic topic)
{
  struct domain_event_subscription_entry *subscription;
  size_t bucket = subscription_topic_bucket(bus, type->type, topic);

  subscription = bus->subscription_topic_buckets[bucket];
  while (subscription != NULL && bus->causal_failure == DOMAIN_EVENT_OK)
  {
    struct domain_event_subscription_entry *next = subscription->topic_next;

    if (subscription->active && subscription->type == type->type &&
        topic_equal(subscription->topic, topic))
    {
      uint64_t before;
      uint64_t after;
      uint64_t elapsed;

      if ((subscription->flags & DOMAIN_EVENT_SUBSCRIPTION_ONCE) != 0U)
        unlink_subscription(bus, subscription);
      before = bus->monotonic_usec_now(bus->clock_context);
      subscription->handler(context, subscription->handler_context);
      after = bus->monotonic_usec_now(bus->clock_context);
      elapsed = after >= before ? after - before : 0U;
      subscription->calls = saturating_add(subscription->calls, 1U);
      subscription->total_usec = saturating_add(subscription->total_usec, elapsed);
      if (elapsed > subscription->maximum_usec)
        subscription->maximum_usec = elapsed;
      type->handler_calls = saturating_add(type->handler_calls, 1U);
      type->total_handler_usec = saturating_add(type->total_handler_usec, elapsed);
      if (elapsed > type->maximum_handler_usec)
        type->maximum_handler_usec = elapsed;
      bus->handler_calls = saturating_add(bus->handler_calls, 1U);
      bus->subscription_deliveries = saturating_add(bus->subscription_deliveries, 1U);
      if (elapsed >= bus->slow_handler_usec)
      {
        subscription->slow_calls = saturating_add(subscription->slow_calls, 1U);
        type->slow_handler_calls = saturating_add(type->slow_handler_calls, 1U);
        bus->slow_handler_calls = saturating_add(bus->slow_handler_calls, 1U);
      }
    }
    subscription = next;
  }
}

enum domain_event_status domain_event_publish(struct domain_event_bus *bus,
                                              domain_event_type_id_t type_id,
                                              const void *payload, size_t payload_size)
{
  return domain_event_publish_routed(bus, type_id, NULL, 0U, payload, payload_size);
}

enum domain_event_status domain_event_publish_routed(
    struct domain_event_bus *bus, domain_event_type_id_t type_id,
    const struct domain_event_topic *topics, size_t topic_count,
    const void *payload, size_t payload_size)
{
  struct domain_event_type_entry *type;
  struct domain_event_handler_entry *handler;
  struct domain_event_context context;
  enum domain_event_status result;
  bool root;

  size_t topic_index;

  if (bus == NULL || payload == NULL || topic_count > DOMAIN_EVENT_MAX_PUBLICATION_TOPICS ||
      (topic_count != 0U && topics == NULL))
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
  for (topic_index = 0U; topic_index < topic_count; topic_index++)
  {
    size_t previous;

    if (topics[topic_index].role == DOMAIN_EVENT_TOPIC_ANY ||
        !topic_is_valid(topics[topic_index]))
      return DOMAIN_EVENT_INVALID_ARGUMENT;
    for (previous = 0U; previous < topic_index; previous++)
      if (topic_equal(topics[topic_index], topics[previous]))
        return DOMAIN_EVENT_INVALID_ARGUMENT;
  }
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
  context.topics = topics;
  context.topic_count = topic_count;

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
  if (bus->causal_failure == DOMAIN_EVENT_OK)
  {
    struct domain_event_topic wildcard;

    wildcard.role = DOMAIN_EVENT_TOPIC_ANY;
    wildcard.entity = domain_entity_handle_none();
    deliver_subscriptions(bus, type, &context, wildcard);
  }
  for (topic_index = 0U;
       topic_index < topic_count && bus->causal_failure == DOMAIN_EVENT_OK;
       topic_index++)
    deliver_subscriptions(bus, type, &context, topics[topic_index]);
  bus->dispatch_depth--;
  result = bus->causal_failure;
  if (root)
  {
    reclaim_retired_subscriptions(bus);
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
  stats->live_subscription_count = bus->subscription_count;
  stats->subscription_high_water = bus->subscription_high_water;
  stats->publications = bus->publications;
  stats->handler_calls = bus->handler_calls;
  stats->subscription_deliveries = bus->subscription_deliveries;
  stats->subscription_cancellations = bus->subscription_cancellations;
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
  stats->live_subscription_count = 0U;
  {
    struct domain_event_subscription_entry *subscription;

    for (subscription = bus->subscriptions_head; subscription != NULL;
         subscription = subscription->all_next)
      if (subscription->type == type_id)
        stats->live_subscription_count++;
  }
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

size_t domain_event_inspect_types(const struct domain_event_bus *bus,
                                  struct domain_event_type_stats *snapshots,
                                  size_t snapshot_capacity)
{
  size_t index;

  if (bus == NULL)
    return 0;
  for (index = 0; index < bus->type_count && index < snapshot_capacity; index++)
  {
    if (snapshots != NULL)
      (void)domain_event_get_type_stats(bus, bus->types[index]->type, &snapshots[index]);
  }
  return bus->type_count;
}

size_t domain_event_inspect_handlers(const struct domain_event_bus *bus,
                                     domain_event_type_id_t type_id,
                                     struct domain_event_handler_stats *snapshots,
                                     size_t snapshot_capacity)
{
  struct domain_event_type_entry *type;
  struct domain_event_handler_entry *handler;
  size_t index;

  if (bus == NULL)
    return 0;
  type = find_type(bus, type_id);
  if (type == NULL)
    return 0;
  index = 0;
  for (handler = type->handlers; handler != NULL; handler = handler->next)
  {
    if (snapshots != NULL && index < snapshot_capacity)
    {
      snapshots[index].type = type_id;
      snapshots[index].identity = handler->identity;
      snapshots[index].priority = handler->priority;
      snapshots[index].registration_sequence = handler->registration_sequence;
      snapshots[index].calls = handler->calls;
      snapshots[index].total_usec = handler->total_usec;
      snapshots[index].maximum_usec = handler->maximum_usec;
      snapshots[index].slow_calls = handler->slow_calls;
    }
    index++;
  }
  return index;
}

size_t domain_event_inspect_subscriptions(
    const struct domain_event_bus *bus,
    const struct domain_event_subscription_stats *filter,
    struct domain_event_subscription_stats *snapshots, size_t snapshot_capacity)
{
  struct domain_event_subscription_entry *subscription;
  size_t count = 0U;

  if (bus == NULL)
    return 0U;
  for (subscription = bus->subscriptions_head; subscription != NULL;
       subscription = subscription->all_next)
  {
    if (filter != NULL)
    {
      if (filter->handle.id != 0U &&
          (filter->handle.id != subscription->handle.id ||
           (filter->handle.generation != 0U &&
            filter->handle.generation != subscription->handle.generation)))
        continue;
      if (filter->type != 0U && filter->type != subscription->type)
        continue;
      if ((filter->topic.role != DOMAIN_EVENT_TOPIC_ANY ||
           !domain_entity_handle_is_none(filter->topic.entity)) &&
          !topic_equal(filter->topic, subscription->topic))
        continue;
      if (domain_entity_handle_is_valid(filter->owner) &&
          !domain_entity_handle_equal(filter->owner, subscription->owner))
        continue;
      if (filter->identity != NULL && strstr(subscription->identity, filter->identity) == NULL)
        continue;
    }
    if (snapshots != NULL && count < snapshot_capacity)
    {
      snapshots[count].handle = subscription->handle;
      snapshots[count].type = subscription->type;
      snapshots[count].topic = subscription->topic;
      snapshots[count].owner = subscription->owner;
      snapshots[count].identity = subscription->identity;
      snapshots[count].priority = subscription->priority;
      snapshots[count].flags = subscription->flags;
      snapshots[count].registration_sequence = subscription->registration_sequence;
      snapshots[count].calls = subscription->calls;
      snapshots[count].total_usec = subscription->total_usec;
      snapshots[count].maximum_usec = subscription->maximum_usec;
      snapshots[count].slow_calls = subscription->slow_calls;
    }
    count++;
  }
  return count;
}

size_t domain_event_inspect_entity_subscriptions(
    const struct domain_event_bus *bus, struct domain_entity_handle entity,
    struct domain_event_subscription_stats *snapshots, size_t snapshot_capacity)
{
  struct domain_event_subscription_entry *subscription;
  size_t count = 0U;

  if (bus == NULL || !domain_entity_handle_is_valid(entity))
    return 0U;
  for (subscription = bus->subscriptions_head; subscription != NULL;
       subscription = subscription->all_next)
  {
    if (!domain_entity_handle_equal(subscription->owner, entity) &&
        !domain_entity_handle_equal(subscription->topic.entity, entity))
      continue;
    if (snapshots != NULL && count < snapshot_capacity)
    {
      snapshots[count].handle = subscription->handle;
      snapshots[count].type = subscription->type;
      snapshots[count].topic = subscription->topic;
      snapshots[count].owner = subscription->owner;
      snapshots[count].identity = subscription->identity;
      snapshots[count].priority = subscription->priority;
      snapshots[count].flags = subscription->flags;
      snapshots[count].registration_sequence = subscription->registration_sequence;
      snapshots[count].calls = subscription->calls;
      snapshots[count].total_usec = subscription->total_usec;
      snapshots[count].maximum_usec = subscription->maximum_usec;
      snapshots[count].slow_calls = subscription->slow_calls;
    }
    count++;
  }
  return count;
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
      "subscription capacity reached",
      "owner subscription capacity reached",
      "topic subscription capacity reached",
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
