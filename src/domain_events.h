#ifndef DOMAIN_EVENTS_H
#define DOMAIN_EVENTS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DOMAIN_EVENT_DEFAULT_MAX_TYPES 64U
#define DOMAIN_EVENT_DEFAULT_MAX_HANDLERS 256U
#define DOMAIN_EVENT_DEFAULT_MAX_DEPTH 16U
#define DOMAIN_EVENT_DEFAULT_MAX_CAUSAL_EVENTS 1024U
#define DOMAIN_EVENT_DEFAULT_SLOW_HANDLER_USEC 5000U

typedef uint32_t domain_event_type_id_t;

struct domain_event_bus;

enum domain_event_status
{
  DOMAIN_EVENT_OK = 0,
  DOMAIN_EVENT_INVALID_ARGUMENT,
  DOMAIN_EVENT_WRONG_THREAD,
  DOMAIN_EVENT_REGISTRY_SEALED,
  DOMAIN_EVENT_REGISTRY_NOT_SEALED,
  DOMAIN_EVENT_DUPLICATE_TYPE,
  DOMAIN_EVENT_DUPLICATE_HANDLER,
  DOMAIN_EVENT_DUPLICATE_RESOLVER,
  DOMAIN_EVENT_TYPE_CAPACITY_REACHED,
  DOMAIN_EVENT_HANDLER_CAPACITY_REACHED,
  DOMAIN_EVENT_ALLOCATION_FAILED,
  DOMAIN_EVENT_UNKNOWN_TYPE,
  DOMAIN_EVENT_PAYLOAD_SIZE_MISMATCH,
  DOMAIN_EVENT_NESTING_LIMIT_REACHED,
  DOMAIN_EVENT_CAUSAL_LIMIT_REACHED,
  DOMAIN_EVENT_BUSY,
  DOMAIN_EVENT_NOT_FOUND
};

enum domain_entity_kind
{
  DOMAIN_ENTITY_NONE = 0,
  DOMAIN_ENTITY_WORLD,
  DOMAIN_ENTITY_DESCRIPTOR,
  DOMAIN_ENTITY_CHARACTER,
  DOMAIN_ENTITY_ROOM,
  DOMAIN_ENTITY_REGION,
  DOMAIN_ENTITY_OBJECT,
  DOMAIN_ENTITY_ZONE,
  DOMAIN_ENTITY_ENCOUNTER,
  DOMAIN_ENTITY_VESSEL,
  DOMAIN_ENTITY_SERVICE,
  DOMAIN_ENTITY_KIND_COUNT
};

struct domain_entity_handle
{
  enum domain_entity_kind kind;
  uint64_t runtime_id;
  uint64_t generation;
};

struct domain_event_context
{
  struct domain_event_bus *bus;
  domain_event_type_id_t type;
  const void *payload;
  size_t payload_size;
  uint32_t depth;
  uint32_t causal_sequence;
};

typedef void (*domain_event_handler)(const struct domain_event_context *context,
                                     void *handler_context);
typedef void *(*domain_entity_resolver)(struct domain_entity_handle handle,
                                       void *resolver_context);
typedef uint64_t (*domain_event_usec_source)(void *clock_context);

struct domain_event_bus_config
{
  size_t max_event_types;
  size_t max_handlers;
  uint32_t max_depth;
  uint32_t max_causal_events;
  uint64_t slow_handler_usec;
  domain_event_usec_source monotonic_usec_now;
  void *clock_context;
};

struct domain_event_type_config
{
  domain_event_type_id_t type;
  const char *name;
  size_t payload_size;
};

struct domain_event_handler_config
{
  domain_event_type_id_t type;
  const char *identity;
  int priority;
  domain_event_handler handler;
  void *handler_context;
};

struct domain_event_bus_stats
{
  size_t registered_type_count;
  size_t registered_handler_count;
  uint64_t publications;
  uint64_t handler_calls;
  uint64_t rejected_causal_chains;
  uint64_t slow_handler_calls;
  uint32_t maximum_depth;
  bool sealed;
  bool dispatching;
};

struct domain_event_type_stats
{
  domain_event_type_id_t type;
  const char *name;
  size_t payload_size;
  size_t handler_count;
  uint64_t publications;
  uint64_t handler_calls;
  uint64_t rejected_publications;
  uint64_t total_handler_usec;
  uint64_t maximum_handler_usec;
  uint64_t slow_handler_calls;
  uint32_t maximum_depth;
};

struct domain_event_handler_stats
{
  domain_event_type_id_t type;
  const char *identity;
  int priority;
  uint64_t registration_sequence;
  uint64_t calls;
  uint64_t total_usec;
  uint64_t maximum_usec;
  uint64_t slow_calls;
};

struct domain_entity_handle domain_entity_handle_none(void);
bool domain_entity_handle_is_none(struct domain_entity_handle handle);
bool domain_entity_handle_is_valid(struct domain_entity_handle handle);
bool domain_entity_handle_equal(struct domain_entity_handle left,
                                struct domain_entity_handle right);

struct domain_event_bus *domain_event_bus_create(const struct domain_event_bus_config *config,
                                                 enum domain_event_status *status);
enum domain_event_status domain_event_bus_destroy(struct domain_event_bus *bus);
enum domain_event_status domain_event_register_type(
    struct domain_event_bus *bus, const struct domain_event_type_config *config);
enum domain_event_status domain_event_register_handler(
    struct domain_event_bus *bus, const struct domain_event_handler_config *config);
enum domain_event_status domain_event_register_resolver(struct domain_event_bus *bus,
                                                        enum domain_entity_kind kind,
                                                        domain_entity_resolver resolver,
                                                        void *resolver_context);
enum domain_event_status domain_event_seal(struct domain_event_bus *bus);
enum domain_event_status domain_event_publish(struct domain_event_bus *bus,
                                              domain_event_type_id_t type,
                                              const void *payload, size_t payload_size);

void *domain_event_resolve(struct domain_event_bus *bus, struct domain_entity_handle handle,
                           enum domain_entity_kind expected_kind);
void domain_event_bus_get_stats(const struct domain_event_bus *bus,
                                struct domain_event_bus_stats *stats);
enum domain_event_status domain_event_get_type_stats(const struct domain_event_bus *bus,
                                                     domain_event_type_id_t type,
                                                     struct domain_event_type_stats *stats);
enum domain_event_status domain_event_get_handler_stats(
    const struct domain_event_bus *bus, domain_event_type_id_t type, const char *identity,
    struct domain_event_handler_stats *stats);
const char *domain_event_status_name(enum domain_event_status status);

#endif /* DOMAIN_EVENTS_H */
