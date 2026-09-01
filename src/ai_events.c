/**
 * @file ai_events.c
 * @brief Main-thread delivery and retry scheduling for asynchronous AI work.
 */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "ai_service.h"
#include "domain_event_runtime.h"
#include "domain_event_world.h"
#include "dgscript/dg_event.h"
#include "event_runtime.h"

#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>

#define AI_EVENT_INGRESS_CAPACITY 256U
#define AI_EVENT_RESPONSE_DELAY 1U
#define AI_EVENT_SERVICE_OWNER_ID UINT64_C(0x41490001)

enum ai_ingress_kind
{
  AI_INGRESS_RESPONSE = 0,
  AI_INGRESS_RETRY
};

enum ai_ingress_admission
{
  AI_INGRESS_ADMITTED = 0,
  AI_INGRESS_DROPPED,
  AI_INGRESS_FAILED
};

struct ai_response_event
{
  struct domain_entity_handle player;
  struct domain_entity_handle npc;
  char *response;
  char *backend;
  char *cache_key;
  bool from_cache;
};

struct ai_request_retry_event
{
  char *prompt;
  int request_type;
  int retry_count;
  struct domain_entity_handle player;
  struct domain_entity_handle npc;
};

struct ai_ingress_item
{
  enum ai_ingress_kind kind;
  void *payload;
  game_tick_t delay;
  struct ai_ingress_item *next;
};

static struct
{
  pthread_mutex_t mutex;
  struct ai_ingress_item *head;
  struct ai_ingress_item *tail;
  size_t depth;
  uint64_t high_water;
  uint64_t accepted;
  uint64_t processed;
  uint64_t rejected;
  uint64_t wake_failures;
  uint64_t schedule_failures;
  int read_fd;
  int write_fd;
  bool accepting;
} ai_ingress = {PTHREAD_MUTEX_INITIALIZER, NULL, NULL, 0U, 0U, 0U, 0U, 0U, 0U,
                0U, -1, -1, false};

static game_event_type_id_t ai_response_type;
static game_event_type_id_t ai_retry_type;

#if defined(LUMINARI_CUTEST)
static int ai_event_cleanup_count;

void ai_event_test_reset_cleanup_count(void)
{
  ai_event_cleanup_count = 0;
}

int ai_event_test_cleanup_count(void)
{
  return ai_event_cleanup_count;
}
#endif

static void cleanup_ai_response_event(void *event_obj)
{
  struct ai_response_event *data = event_obj;

  if (data == NULL)
    return;
  free(data->response);
  free(data->backend);
  free(data->cache_key);
  free(data);
#if defined(LUMINARI_CUTEST)
  ai_event_cleanup_count++;
#endif
}

static void cleanup_ai_request_retry_event(void *event_obj)
{
  struct ai_request_retry_event *data = event_obj;

  if (data == NULL)
    return;
  free(data->prompt);
  free(data);
#if defined(LUMINARI_CUTEST)
  ai_event_cleanup_count++;
#endif
}

static void cleanup_ai_response_rollback(event_handle_t handle, void *event_obj)
{
  (void)handle;
  cleanup_ai_response_event(event_obj);
}

static void cleanup_ai_retry_rollback(event_handle_t handle, void *event_obj)
{
  (void)handle;
  cleanup_ai_request_retry_event(event_obj);
}

static struct game_event_owner ai_service_owner(void)
{
  struct game_event_owner owner = game_event_owner_none();

  owner.kind = GAME_EVENT_OWNER_SERVICE;
  owner.runtime_id = AI_EVENT_SERVICE_OWNER_ID;
  owner.generation = 1U;
  return owner;
}

static struct game_event_owner ai_character_owner(struct domain_entity_handle handle)
{
  struct game_event_owner owner = game_event_owner_none();

  if (!domain_entity_handle_is_valid(handle) || handle.kind != DOMAIN_ENTITY_CHARACTER)
    return owner;
  owner.kind = GAME_EVENT_OWNER_CHARACTER;
  owner.runtime_id = handle.runtime_id;
  owner.generation = handle.generation;
  return owner;
}

static struct char_data *resolve_character(struct domain_entity_handle handle)
{
  if (!domain_entity_handle_is_valid(handle) || handle.kind != DOMAIN_ENTITY_CHARACTER)
    return NULL;
  return domain_event_resolve(domain_event_runtime_bus(), handle, DOMAIN_ENTITY_CHARACTER);
}

static void deliver_ai_response(struct ai_response_event *data)
{
  struct char_data *player;
  struct char_data *npc;
  char buf[MAX_STRING_LENGTH];

  if (data == NULL || data->response == NULL)
    return;
  player = resolve_character(data->player);
  npc = resolve_character(data->npc);
  if (player == NULL || npc == NULL || IN_ROOM(player) == NOWHERE ||
      IN_ROOM(player) != IN_ROOM(npc))
  {
    event_note_stale_owner_outcome();
    return;
  }
  snprintf(buf, sizeof(buf), "$n tells you, '%s'", data->response);
  act(buf, FALSE, npc, 0, player, TO_VICT);
  log_ai_interaction(player, npc, data->response,
                     data->backend != NULL ? data->backend : "unknown",
                     data->from_cache);
}

static void run_ai_retry(struct ai_request_retry_event *data)
{
  bool player_valid;
  bool npc_valid;

  if (data == NULL || data->prompt == NULL)
    return;
  player_valid = domain_entity_handle_is_none(data->player) ||
                 resolve_character(data->player) != NULL;
  npc_valid = domain_entity_handle_is_none(data->npc) ||
              resolve_character(data->npc) != NULL;
  if (!player_valid || !npc_valid)
  {
    event_note_stale_owner_outcome();
    return;
  }
  if (!ai_retry_request_async(data->prompt, data->request_type, data->retry_count,
                              data->player, data->npc) &&
      data->retry_count < AI_MAX_RETRIES)
  {
    queue_ai_request_retry_for_entities(data->prompt, data->request_type,
                                        data->retry_count + 1, data->player,
                                        data->npc);
  }
}

static struct game_event_result ai_response_dispatch(
    const struct game_event_context *context)
{
  deliver_ai_response(context != NULL ? context->payload : NULL);
  return game_event_result_complete();
}

static struct game_event_result ai_retry_dispatch(
    const struct game_event_context *context)
{
  run_ai_retry(context != NULL ? context->payload : NULL);
  return game_event_result_complete();
}

static EVENTFUNC(ai_response_rollback)
{
  deliver_ai_response(event_obj);
  return 0;
}

static EVENTFUNC(ai_retry_rollback)
{
  run_ai_retry(event_obj);
  return 0;
}

bool ai_events_runtime_init(void)
{
  struct game_event_type_config config;
  enum game_scheduler_status status;

  if (event_backend_current() != EVENT_BACKEND_GAME_SCHEDULER)
    return true;
  if (!event_runtime_is_initialized())
    return false;
  if (ai_response_type != 0U && ai_retry_type != 0U &&
      event_runtime_type_name(ai_response_type) != NULL &&
      event_runtime_type_name(ai_retry_type) != NULL &&
      !strcmp(event_runtime_type_name(ai_response_type), "ai.response.delivery") &&
      !strcmp(event_runtime_type_name(ai_retry_type), "ai.request.retry"))
    return true;
  if (event_runtime_types_are_sealed())
    return false;

  ai_response_type = 0U;
  ai_retry_type = 0U;
  memset(&config, 0, sizeof(config));
  config.name = "ai.response.delivery";
  config.handler = ai_response_dispatch;
  config.cleanup = cleanup_ai_response_event;
  config.lateness_policy = GAME_EVENT_LATENESS_RUN_ONCE;
  config.max_events = AI_EVENT_INGRESS_CAPACITY;
  config.max_events_per_owner = 16U;
  config.requires_owner = true;
  status = event_runtime_register_type(&config, &ai_response_type);
  if (status != GAME_SCHEDULER_OK)
    return false;

  config.name = "ai.request.retry";
  config.handler = ai_retry_dispatch;
  config.cleanup = cleanup_ai_request_retry_event;
  config.max_events_per_owner = 4U;
  status = event_runtime_register_type(&config, &ai_retry_type);
  return status == GAME_SCHEDULER_OK;
}

static void set_nonblocking(int fd)
{
  int flags;

  flags = fcntl(fd, F_GETFL, 0);
  if (flags >= 0)
    (void)fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

bool ai_events_ingress_init(void)
{
  int signal_fds[2];

  pthread_mutex_lock(&ai_ingress.mutex);
  if (ai_ingress.accepting)
  {
    pthread_mutex_unlock(&ai_ingress.mutex);
    return true;
  }
  ai_ingress.accepting = true;
  if (ai_ingress.read_fd < 0 && pipe(signal_fds) == 0)
  {
    ai_ingress.read_fd = signal_fds[0];
    ai_ingress.write_fd = signal_fds[1];
    set_nonblocking(ai_ingress.read_fd);
    set_nonblocking(ai_ingress.write_fd);
  }
  else if (ai_ingress.read_fd < 0)
  {
    ai_ingress.wake_failures++;
    log("SYSERR: Unable to create the AI main-loop wake pipe: %s", strerror(errno));
  }
  pthread_mutex_unlock(&ai_ingress.mutex);
  return true;
}

static void cleanup_ingress_item(struct ai_ingress_item *item)
{
  if (item == NULL)
    return;
  if (item->payload != NULL)
  {
    if (item->kind == AI_INGRESS_RESPONSE)
      cleanup_ai_response_event(item->payload);
    else
      cleanup_ai_request_retry_event(item->payload);
  }
  free(item);
}

void ai_events_ingress_shutdown(void)
{
  struct ai_ingress_item *item;
  struct ai_ingress_item *next;
  int read_fd;
  int write_fd;

  pthread_mutex_lock(&ai_ingress.mutex);
  ai_ingress.accepting = false;
  item = ai_ingress.head;
  ai_ingress.head = NULL;
  ai_ingress.tail = NULL;
  ai_ingress.depth = 0U;
  read_fd = ai_ingress.read_fd;
  write_fd = ai_ingress.write_fd;
  ai_ingress.read_fd = -1;
  ai_ingress.write_fd = -1;
  pthread_mutex_unlock(&ai_ingress.mutex);

  while (item != NULL)
  {
    next = item->next;
    cleanup_ingress_item(item);
    item = next;
  }
  if (read_fd >= 0)
    close(read_fd);
  if (write_fd >= 0)
    close(write_fd);
}

int ai_events_ingress_fd(void)
{
  int fd;

  pthread_mutex_lock(&ai_ingress.mutex);
  fd = ai_ingress.read_fd;
  pthread_mutex_unlock(&ai_ingress.mutex);
  return fd;
}

static bool enqueue_ingress(enum ai_ingress_kind kind, void *payload,
                            game_tick_t delay)
{
  struct ai_ingress_item *item;
  unsigned char signal_byte = 1U;

  item = calloc(1U, sizeof(*item));
  if (item == NULL)
    return false;
  item->kind = kind;
  item->payload = payload;
  item->delay = MAX(delay, 1U);

  pthread_mutex_lock(&ai_ingress.mutex);
  if (!ai_ingress.accepting || ai_ingress.depth >= AI_EVENT_INGRESS_CAPACITY)
  {
    ai_ingress.rejected++;
    pthread_mutex_unlock(&ai_ingress.mutex);
    free(item);
    return false;
  }
  if (ai_ingress.tail != NULL)
    ai_ingress.tail->next = item;
  else
    ai_ingress.head = item;
  ai_ingress.tail = item;
  ai_ingress.depth++;
  ai_ingress.accepted++;
  if (ai_ingress.depth > ai_ingress.high_water)
    ai_ingress.high_water = ai_ingress.depth;
  if (ai_ingress.write_fd >= 0 &&
      write(ai_ingress.write_fd, &signal_byte, sizeof(signal_byte)) < 0 &&
      errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
    ai_ingress.wake_failures++;
  pthread_mutex_unlock(&ai_ingress.mutex);
  return true;
}

static struct ai_ingress_item *pop_ingress(void)
{
  struct ai_ingress_item *item;

  pthread_mutex_lock(&ai_ingress.mutex);
  item = ai_ingress.head;
  if (item != NULL)
  {
    ai_ingress.head = item->next;
    if (ai_ingress.head == NULL)
      ai_ingress.tail = NULL;
    item->next = NULL;
    ai_ingress.depth--;
    ai_ingress.processed++;
  }
  pthread_mutex_unlock(&ai_ingress.mutex);
  return item;
}

static enum ai_ingress_admission schedule_ingress_item(struct ai_ingress_item *item)
{
  struct event_runtime_handle runtime_handle = EVENT_RUNTIME_HANDLE_NONE;
  struct game_event_owner owner;
  enum game_scheduler_status status;
  event_handle_t rollback_handle;

  if (item == NULL)
    return AI_INGRESS_FAILED;
  if (item->kind == AI_INGRESS_RESPONSE)
  {
    struct ai_response_event *data = item->payload;

    owner = ai_character_owner(data->player);
    if (!game_event_owner_is_valid(owner))
      return AI_INGRESS_FAILED;
    if (data->cache_key != NULL && data->response != NULL)
      ai_cache_response(data->cache_key, data->response);
    if (domain_event_runtime_bus() != NULL &&
        (resolve_character(data->player) == NULL ||
         resolve_character(data->npc) == NULL))
    {
      event_note_stale_owner_outcome();
      return AI_INGRESS_DROPPED;
    }
    if (event_backend_current() == EVENT_BACKEND_GAME_SCHEDULER)
    {
      if (!ai_events_runtime_init())
        return AI_INGRESS_FAILED;
      status = event_runtime_schedule_owned_after(ai_response_type, owner, item->delay,
                                                  data, &runtime_handle);
      return status == GAME_SCHEDULER_OK ? AI_INGRESS_ADMITTED : AI_INGRESS_FAILED;
    }
    rollback_handle = event_schedule_owned_named_with_terminal_cleanup(
        ai_response_rollback, data, (long)item->delay, "ai.response.delivery",
        cleanup_ai_response_rollback, owner);
    return rollback_handle != EVENT_HANDLE_NONE ? AI_INGRESS_ADMITTED : AI_INGRESS_FAILED;
  }
  else
  {
    struct ai_request_retry_event *data = item->payload;

    owner = ai_character_owner(data->player);
    if (!game_event_owner_is_valid(owner))
      owner = ai_service_owner();
    if (domain_event_runtime_bus() != NULL &&
        ((!domain_entity_handle_is_none(data->player) &&
          resolve_character(data->player) == NULL) ||
         (!domain_entity_handle_is_none(data->npc) &&
          resolve_character(data->npc) == NULL)))
    {
      event_note_stale_owner_outcome();
      return AI_INGRESS_DROPPED;
    }
    if (event_backend_current() == EVENT_BACKEND_GAME_SCHEDULER)
    {
      if (!ai_events_runtime_init())
        return AI_INGRESS_FAILED;
      status = event_runtime_schedule_owned_after(ai_retry_type, owner, item->delay,
                                                  data, &runtime_handle);
      return status == GAME_SCHEDULER_OK ? AI_INGRESS_ADMITTED : AI_INGRESS_FAILED;
    }
    rollback_handle = event_schedule_owned_named_with_terminal_cleanup(
        ai_retry_rollback, data, (long)item->delay, "ai.request.retry",
        cleanup_ai_retry_rollback, owner);
    return rollback_handle != EVENT_HANDLE_NONE ? AI_INGRESS_ADMITTED : AI_INGRESS_FAILED;
  }
}

void ai_events_process_ingress(void)
{
  struct ai_ingress_item *item;
  unsigned char signal_buffer[64];
  int read_fd;

  read_fd = ai_events_ingress_fd();
  if (read_fd >= 0)
    while (read(read_fd, signal_buffer, sizeof(signal_buffer)) > 0)
    {
    }
  while ((item = pop_ingress()) != NULL)
  {
    enum ai_ingress_admission admission = schedule_ingress_item(item);

    if (admission == AI_INGRESS_ADMITTED)
      item->payload = NULL;
    else if (admission == AI_INGRESS_FAILED)
    {
      pthread_mutex_lock(&ai_ingress.mutex);
      ai_ingress.schedule_failures++;
      pthread_mutex_unlock(&ai_ingress.mutex);
    }
    cleanup_ingress_item(item);
  }
}

void ai_events_get_ingress_stats(struct ai_event_ingress_stats *stats)
{
  if (stats == NULL)
    return;
  memset(stats, 0, sizeof(*stats));
  pthread_mutex_lock(&ai_ingress.mutex);
  stats->available = ai_ingress.accepting;
  stats->depth = ai_ingress.depth;
  stats->capacity = AI_EVENT_INGRESS_CAPACITY;
  stats->high_water = ai_ingress.high_water;
  stats->accepted = ai_ingress.accepted;
  stats->processed = ai_ingress.processed;
  stats->rejected = ai_ingress.rejected;
  stats->wake_failures = ai_ingress.wake_failures;
  stats->schedule_failures = ai_ingress.schedule_failures;
  pthread_mutex_unlock(&ai_ingress.mutex);
}

void queue_ai_response_for_entities(struct domain_entity_handle player,
                                    struct domain_entity_handle npc,
                                    const char *response, const char *backend,
                                    const char *cache_key, bool from_cache)
{
  struct ai_response_event *data;

  if (!domain_entity_handle_is_valid(player) ||
      !domain_entity_handle_is_valid(npc) || response == NULL)
    return;
  data = calloc(1U, sizeof(*data));
  if (data == NULL)
    return;
  data->player = player;
  data->npc = npc;
  data->response = strdup(response);
  data->backend = strdup(backend != NULL ? backend : "unknown");
  data->cache_key = cache_key != NULL ? strdup(cache_key) : NULL;
  data->from_cache = from_cache;
  if (data->response == NULL || data->backend == NULL ||
      (cache_key != NULL && data->cache_key == NULL) ||
      !enqueue_ingress(AI_INGRESS_RESPONSE, data, AI_EVENT_RESPONSE_DELAY))
    cleanup_ai_response_event(data);
}

void queue_ai_response(struct char_data *ch, struct char_data *npc,
                       const char *response, const char *backend,
                       bool from_cache)
{
  queue_ai_response_for_entities(domain_event_character_handle(ch),
                                 domain_event_character_handle(npc), response,
                                 backend, NULL, from_cache);
}

void queue_ai_request_retry_for_entities(const char *prompt, int request_type,
                                         int retry_count,
                                         struct domain_entity_handle player,
                                         struct domain_entity_handle npc)
{
  struct ai_request_retry_event *data;
  game_tick_t delay;

  if (prompt == NULL)
    return;
  data = calloc(1U, sizeof(*data));
  if (data == NULL)
    return;
  data->prompt = strdup(prompt);
  data->request_type = request_type;
  data->retry_count = retry_count;
  data->player = player;
  data->npc = npc;
  delay = retry_count <= 0 ? PASSES_PER_SEC :
          (game_tick_t)MIN(1 << MIN(retry_count, 4), 16) * PASSES_PER_SEC;
  if (data->prompt == NULL || !enqueue_ingress(AI_INGRESS_RETRY, data, delay))
    cleanup_ai_request_retry_event(data);
}

void queue_ai_request_retry(const char *prompt, int request_type, int retry_count,
                            struct char_data *ch, struct char_data *npc)
{
  struct domain_entity_handle player = domain_entity_handle_none();
  struct domain_entity_handle mobile = domain_entity_handle_none();

  if (ch != NULL)
    player = domain_event_character_handle(ch);
  if (npc != NULL)
    mobile = domain_event_character_handle(npc);
  queue_ai_request_retry_for_entities(prompt, request_type, retry_count, player,
                                      mobile);
}
