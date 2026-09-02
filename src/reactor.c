#include "reactor.h"

#include <errno.h>
#include <event2/event.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <time.h>

struct reactor_watch
{
  int fd;
  unsigned int interests;
  unsigned int ready;
  struct event *event;
};

struct reactor_signal
{
  int signal_number;
  luminari_reactor_signal_callback callback;
  void *context;
  struct event *event;
};

struct luminari_reactor
{
  enum luminari_io_driver driver;
  struct event_base *event_base;
  struct reactor_watch *watches;
  size_t watch_count;
  size_t watch_capacity;
  struct reactor_signal *signals;
  size_t signal_count;
  size_t signal_capacity;
  bool waiting;
  bool timer_fired;
};

static bool reserve_signals(struct luminari_reactor *reactor, size_t required)
{
  struct reactor_signal *resized;
  size_t capacity;

  if (required <= reactor->signal_capacity)
    return true;
  capacity = reactor->signal_capacity == 0 ? 8U : reactor->signal_capacity;
  while (capacity < required)
  {
    if (capacity > SIZE_MAX / 2U)
      return false;
    capacity *= 2U;
  }
  resized = realloc(reactor->signals, capacity * sizeof(*resized));
  if (resized == NULL)
    return false;
  reactor->signals = resized;
  reactor->signal_capacity = capacity;
  return true;
}

static struct reactor_watch *find_watch(struct luminari_reactor *reactor, int fd)
{
  size_t index;

  for (index = 0; index < reactor->watch_count; index++)
  {
    if (reactor->watches[index].fd == fd)
      return &reactor->watches[index];
  }
  return NULL;
}

static bool reserve_watches(struct luminari_reactor *reactor, size_t required)
{
  struct reactor_watch *resized;
  size_t capacity;

  if (required <= reactor->watch_capacity)
    return true;
  capacity = reactor->watch_capacity == 0 ? 16U : reactor->watch_capacity;
  while (capacity < required)
  {
    if (capacity > SIZE_MAX / 2U)
      return false;
    capacity *= 2U;
  }
  resized = realloc(reactor->watches, capacity * sizeof(*resized));
  if (resized == NULL)
    return false;
  reactor->watches = resized;
  reactor->watch_capacity = capacity;
  return true;
}

enum luminari_io_driver luminari_io_driver_from_string(const char *value, bool *recognized)
{
  if (recognized != NULL)
    *recognized = true;
  if (value == NULL || *value == '\0' || strcmp(value, "libevent") == 0)
    return LUMINARI_IO_DRIVER_LIBEVENT;
  if (strcmp(value, "select") == 0)
    return LUMINARI_IO_DRIVER_SELECT;
  if (recognized != NULL)
    *recognized = false;
  return LUMINARI_IO_DRIVER_LIBEVENT;
}

const char *luminari_io_driver_name(enum luminari_io_driver driver)
{
  return driver == LUMINARI_IO_DRIVER_SELECT ? "select" : "libevent";
}

const char *luminari_reactor_library_version(void)
{
  return event_get_version();
}

uint64_t luminari_reactor_monotonic_usec(void)
{
  struct timespec now;

  if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
    return 0;
  return (uint64_t)now.tv_sec * UINT64_C(1000000) + (uint64_t)now.tv_nsec / UINT64_C(1000);
}

struct luminari_reactor *luminari_reactor_create(enum luminari_io_driver driver,
                                                 enum luminari_reactor_status *status)
{
  struct luminari_reactor *reactor;

  if (status != NULL)
    *status = LUMINARI_REACTOR_INVALID_ARGUMENT;
  if (driver != LUMINARI_IO_DRIVER_SELECT && driver != LUMINARI_IO_DRIVER_LIBEVENT)
    return NULL;
  reactor = calloc(1, sizeof(*reactor));
  if (reactor == NULL)
  {
    if (status != NULL)
      *status = LUMINARI_REACTOR_NO_MEMORY;
    return NULL;
  }
  reactor->driver = driver;
  if (driver == LUMINARI_IO_DRIVER_LIBEVENT)
  {
    reactor->event_base = event_base_new();
    if (reactor->event_base == NULL)
    {
      free(reactor);
      if (status != NULL)
        *status = LUMINARI_REACTOR_NO_MEMORY;
      return NULL;
    }
  }
  if (status != NULL)
    *status = LUMINARI_REACTOR_OK;
  return reactor;
}

void luminari_reactor_destroy(struct luminari_reactor *reactor)
{
  size_t index;

  if (reactor == NULL)
    return;
  for (index = 0; index < reactor->watch_count; index++)
  {
    if (reactor->watches[index].event != NULL)
      event_free(reactor->watches[index].event);
  }
  for (index = 0; index < reactor->signal_count; index++)
  {
    if (reactor->signals[index].event != NULL)
      event_free(reactor->signals[index].event);
  }
  if (reactor->event_base != NULL)
    event_base_free(reactor->event_base);
  free(reactor->watches);
  free(reactor->signals);
  free(reactor);
}

static void libevent_signal_ready(evutil_socket_t signal_number, short events, void *context)
{
  struct luminari_reactor *reactor = context;
  size_t index;

  (void)events;
  for (index = 0; index < reactor->signal_count; index++)
  {
    if (reactor->signals[index].signal_number == (int)signal_number)
    {
      reactor->signals[index].callback((int)signal_number, reactor->signals[index].context);
      return;
    }
  }
}

enum luminari_reactor_status luminari_reactor_add_signal(struct luminari_reactor *reactor,
                                                         int signal_number,
                                                         luminari_reactor_signal_callback callback,
                                                         void *context)
{
  struct reactor_signal *signal_watch;
  size_t index;

  if (reactor == NULL || reactor->driver != LUMINARI_IO_DRIVER_LIBEVENT || signal_number <= 0 ||
      callback == NULL)
    return LUMINARI_REACTOR_INVALID_ARGUMENT;
  if (reactor->waiting)
    return LUMINARI_REACTOR_BUSY;
  for (index = 0; index < reactor->signal_count; index++)
  {
    if (reactor->signals[index].signal_number == signal_number)
      return LUMINARI_REACTOR_INVALID_ARGUMENT;
  }
  if (!reserve_signals(reactor, reactor->signal_count + 1U))
    return LUMINARI_REACTOR_NO_MEMORY;
  signal_watch = &reactor->signals[reactor->signal_count];
  memset(signal_watch, 0, sizeof(*signal_watch));
  signal_watch->signal_number = signal_number;
  signal_watch->callback = callback;
  signal_watch->context = context;
  signal_watch->event =
      evsignal_new(reactor->event_base, signal_number, libevent_signal_ready, reactor);
  if (signal_watch->event == NULL || event_add(signal_watch->event, NULL) != 0)
  {
    if (signal_watch->event != NULL)
      event_free(signal_watch->event);
    memset(signal_watch, 0, sizeof(*signal_watch));
    return LUMINARI_REACTOR_SYSTEM_ERROR;
  }
  reactor->signal_count++;
  return LUMINARI_REACTOR_OK;
}

enum luminari_io_driver luminari_reactor_driver(const struct luminari_reactor *reactor)
{
  return reactor != NULL ? reactor->driver : LUMINARI_IO_DRIVER_SELECT;
}

enum luminari_reactor_status luminari_reactor_begin_cycle(struct luminari_reactor *reactor)
{
  size_t index;

  if (reactor == NULL)
    return LUMINARI_REACTOR_INVALID_ARGUMENT;
  if (reactor->waiting)
    return LUMINARI_REACTOR_BUSY;
  for (index = 0; index < reactor->watch_count; index++)
  {
    if (reactor->watches[index].event != NULL)
      event_free(reactor->watches[index].event);
    reactor->watches[index].event = NULL;
  }
  reactor->watch_count = 0;
  reactor->timer_fired = false;
  return LUMINARI_REACTOR_OK;
}

enum luminari_reactor_status luminari_reactor_watch(struct luminari_reactor *reactor, int fd,
                                                    unsigned int interests)
{
  struct reactor_watch *watch;
  const unsigned int allowed =
      LUMINARI_REACTOR_READ | LUMINARI_REACTOR_WRITE | LUMINARI_REACTOR_ERROR;

  if (reactor == NULL || fd < 0 || interests == 0 || (interests & ~allowed) != 0)
    return LUMINARI_REACTOR_INVALID_ARGUMENT;
  if (reactor->waiting)
    return LUMINARI_REACTOR_BUSY;
  watch = find_watch(reactor, fd);
  if (watch != NULL)
  {
    watch->interests |= interests;
    return LUMINARI_REACTOR_OK;
  }
  if (!reserve_watches(reactor, reactor->watch_count + 1U))
    return LUMINARI_REACTOR_NO_MEMORY;
  watch = &reactor->watches[reactor->watch_count++];
  memset(watch, 0, sizeof(*watch));
  watch->fd = fd;
  watch->interests = interests;
  return LUMINARI_REACTOR_OK;
}

static void libevent_watch_ready(evutil_socket_t fd, short events, void *context)
{
  struct reactor_watch *watch = context;

  (void)fd;
  if ((events & EV_READ) != 0)
    watch->ready |= LUMINARI_REACTOR_READ;
  if ((events & EV_WRITE) != 0)
    watch->ready |= LUMINARI_REACTOR_WRITE;
#ifdef EV_CLOSED
  if ((events & EV_CLOSED) != 0)
    watch->ready |= LUMINARI_REACTOR_ERROR;
#endif
}

static void libevent_timer_ready(evutil_socket_t fd, short events, void *context)
{
  struct luminari_reactor *reactor = context;

  (void)fd;
  (void)events;
  reactor->timer_fired = true;
}

static enum luminari_reactor_status wait_with_select(struct luminari_reactor *reactor,
                                                     uint64_t timeout_usec)
{
  fd_set read_set;
  fd_set write_set;
  fd_set error_set;
  struct timeval timeout;
  struct reactor_watch *watch;
  uint64_t deadline;
  uint64_t now;
  uint64_t remaining;
  size_t index;
  int max_fd;
  int result;

  for (index = 0; index < reactor->watch_count; index++)
  {
    watch = &reactor->watches[index];
    watch->ready = 0;
    if (watch->fd >= FD_SETSIZE)
    {
      errno = EINVAL;
      return LUMINARI_REACTOR_SYSTEM_ERROR;
    }
  }
  now = luminari_reactor_monotonic_usec();
  deadline = UINT64_MAX - now < timeout_usec ? UINT64_MAX : now + timeout_usec;
  do
  {
    FD_ZERO(&read_set);
    FD_ZERO(&write_set);
    FD_ZERO(&error_set);
    max_fd = -1;
    for (index = 0; index < reactor->watch_count; index++)
    {
      watch = &reactor->watches[index];
      if ((watch->interests & LUMINARI_REACTOR_READ) != 0 &&
          (watch->ready & LUMINARI_REACTOR_READ) == 0)
        FD_SET(watch->fd, &read_set);
      if ((watch->interests & LUMINARI_REACTOR_WRITE) != 0 &&
          (watch->ready & LUMINARI_REACTOR_WRITE) == 0)
        FD_SET(watch->fd, &write_set);
      if ((watch->interests & LUMINARI_REACTOR_ERROR) != 0 &&
          (watch->ready & LUMINARI_REACTOR_ERROR) == 0)
        FD_SET(watch->fd, &error_set);
      if (watch->fd > max_fd)
        max_fd = watch->fd;
    }
    now = luminari_reactor_monotonic_usec();
    remaining = now >= deadline ? 0 : deadline - now;
    timeout.tv_sec = (time_t)(remaining / UINT64_C(1000000));
    timeout.tv_usec = (suseconds_t)(remaining % UINT64_C(1000000));
    result = select(max_fd + 1, &read_set, &write_set, &error_set, &timeout);
    if (result < 0 && errno != EINTR)
      return LUMINARI_REACTOR_SYSTEM_ERROR;
    if (result > 0)
    {
      for (index = 0; index < reactor->watch_count; index++)
      {
        watch = &reactor->watches[index];
        if (FD_ISSET(watch->fd, &read_set))
          watch->ready |= LUMINARI_REACTOR_READ;
        if (FD_ISSET(watch->fd, &write_set))
          watch->ready |= LUMINARI_REACTOR_WRITE;
        if (FD_ISSET(watch->fd, &error_set))
          watch->ready |= LUMINARI_REACTOR_ERROR;
      }
    }
  } while (luminari_reactor_monotonic_usec() < deadline);
  return LUMINARI_REACTOR_OK;
}

static enum luminari_reactor_status wait_with_libevent(struct luminari_reactor *reactor,
                                                       uint64_t timeout_usec)
{
  struct event *timer;
  struct timeval timeout;
  struct reactor_watch *watch;
  size_t index;
  short flags;
  int result;

  for (index = 0; index < reactor->watch_count; index++)
  {
    watch = &reactor->watches[index];
    watch->ready = 0;
    flags = 0;
    if ((watch->interests & LUMINARI_REACTOR_READ) != 0)
      flags |= EV_READ;
    if ((watch->interests & LUMINARI_REACTOR_WRITE) != 0)
      flags |= EV_WRITE;
#ifdef EV_CLOSED
    if ((watch->interests & LUMINARI_REACTOR_ERROR) != 0)
      flags |= EV_CLOSED;
#endif
    watch->event = event_new(reactor->event_base, watch->fd, flags, libevent_watch_ready, watch);
    if (watch->event == NULL || event_add(watch->event, NULL) != 0)
      return LUMINARI_REACTOR_SYSTEM_ERROR;
  }
  timeout.tv_sec = (time_t)(timeout_usec / UINT64_C(1000000));
  timeout.tv_usec = (suseconds_t)(timeout_usec % UINT64_C(1000000));
  timer = evtimer_new(reactor->event_base, libevent_timer_ready, reactor);
  if (timer == NULL || evtimer_add(timer, &timeout) != 0)
  {
    if (timer != NULL)
      event_free(timer);
    return LUMINARI_REACTOR_SYSTEM_ERROR;
  }
  do
  {
    result = event_base_loop(reactor->event_base, EVLOOP_ONCE);
    if (result < 0)
      break;
  } while (!reactor->timer_fired);
  event_free(timer);
  if (result < 0)
    return LUMINARI_REACTOR_SYSTEM_ERROR;
  return LUMINARI_REACTOR_OK;
}

enum luminari_reactor_status luminari_reactor_wait(struct luminari_reactor *reactor,
                                                   uint64_t timeout_usec)
{
  enum luminari_reactor_status status;

  if (reactor == NULL)
    return LUMINARI_REACTOR_INVALID_ARGUMENT;
  if (reactor->waiting)
    return LUMINARI_REACTOR_BUSY;
  reactor->waiting = true;
  if (reactor->driver == LUMINARI_IO_DRIVER_LIBEVENT)
    status = wait_with_libevent(reactor, timeout_usec);
  else
    status = wait_with_select(reactor, timeout_usec);
  reactor->waiting = false;
  return status;
}

bool luminari_reactor_ready(const struct luminari_reactor *reactor, int fd, unsigned int interest)
{
  size_t index;

  if (reactor == NULL || fd < 0)
    return false;
  for (index = 0; index < reactor->watch_count; index++)
  {
    if (reactor->watches[index].fd == fd)
      return (reactor->watches[index].ready & interest) != 0;
  }
  return false;
}

size_t luminari_reactor_watch_count(const struct luminari_reactor *reactor)
{
  return reactor != NULL ? reactor->watch_count : 0;
}
