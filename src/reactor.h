#ifndef LUMINARI_REACTOR_H
#define LUMINARI_REACTOR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum luminari_io_driver
{
  LUMINARI_IO_DRIVER_SELECT = 0,
  LUMINARI_IO_DRIVER_LIBEVENT = 1
};

enum luminari_reactor_interest
{
  LUMINARI_REACTOR_READ = 1U << 0,
  LUMINARI_REACTOR_WRITE = 1U << 1,
  LUMINARI_REACTOR_ERROR = 1U << 2
};

enum luminari_reactor_status
{
  LUMINARI_REACTOR_OK = 0,
  LUMINARI_REACTOR_INVALID_ARGUMENT,
  LUMINARI_REACTOR_NO_MEMORY,
  LUMINARI_REACTOR_SYSTEM_ERROR,
  LUMINARI_REACTOR_BUSY
};

struct luminari_reactor;
typedef void (*luminari_reactor_signal_callback)(int signal_number, void *context);

enum luminari_io_driver luminari_io_driver_from_string(const char *value, bool *recognized);
const char *luminari_io_driver_name(enum luminari_io_driver driver);
const char *luminari_reactor_library_version(void);
uint64_t luminari_reactor_monotonic_usec(void);

struct luminari_reactor *luminari_reactor_create(enum luminari_io_driver driver,
                                                 enum luminari_reactor_status *status);
void luminari_reactor_destroy(struct luminari_reactor *reactor);
enum luminari_io_driver luminari_reactor_driver(const struct luminari_reactor *reactor);

enum luminari_reactor_status luminari_reactor_begin_cycle(struct luminari_reactor *reactor);
enum luminari_reactor_status luminari_reactor_watch(struct luminari_reactor *reactor, int fd,
                                                    unsigned int interests);
enum luminari_reactor_status luminari_reactor_add_signal(
    struct luminari_reactor *reactor, int signal_number,
    luminari_reactor_signal_callback callback, void *context);
enum luminari_reactor_status luminari_reactor_wait(struct luminari_reactor *reactor,
                                                   uint64_t timeout_usec);
bool luminari_reactor_ready(const struct luminari_reactor *reactor, int fd,
                            unsigned int interest);
size_t luminari_reactor_watch_count(const struct luminari_reactor *reactor);

#endif
