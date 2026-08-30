#include "domain_event_runtime.h"

#include "domain_event_types.h"
#include "domain_event_world.h"
#include "wilderness/spatial_events.h"

static struct domain_event_bus *runtime_bus;

enum domain_event_status domain_event_runtime_init(void)
{
  enum domain_event_status status;

  if (runtime_bus != NULL)
    return DOMAIN_EVENT_BUSY;
  runtime_bus = domain_event_bus_create(NULL, &status);
  if (runtime_bus == NULL)
    return status;
  status = domain_event_register_foundation_types(runtime_bus);
  if (status == DOMAIN_EVENT_OK)
    status = domain_event_world_register_resolvers(runtime_bus);
  if (status == DOMAIN_EVENT_OK)
    status = spatial_event_register_handlers(runtime_bus);
  if (status == DOMAIN_EVENT_OK)
    status = domain_event_seal(runtime_bus);
  if (status != DOMAIN_EVENT_OK)
  {
    domain_event_bus_destroy(runtime_bus);
    runtime_bus = NULL;
  }
  return status;
}

enum domain_event_status domain_event_runtime_shutdown(void)
{
  enum domain_event_status status;

  if (runtime_bus == NULL)
    return DOMAIN_EVENT_OK;
  status = domain_event_bus_destroy(runtime_bus);
  if (status == DOMAIN_EVENT_OK)
    runtime_bus = NULL;
  return status;
}

struct domain_event_bus *domain_event_runtime_bus(void)
{
  return runtime_bus;
}
