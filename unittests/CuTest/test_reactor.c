#include "CuTest.h"

#include "../../src/reactor.h"

#include <signal.h>
#include <stdint.h>
#include <sys/socket.h>
#include <unistd.h>

static void assert_driver_readiness_and_cadence(CuTest *tc, enum luminari_io_driver driver)
{
  enum luminari_reactor_status status;
  struct luminari_reactor *reactor;
  uint64_t started;
  uint64_t elapsed;
  int sockets[2];
  char marker = 'x';

  CuAssertIntEquals(tc, 0, socketpair(AF_UNIX, SOCK_STREAM, 0, sockets));
  reactor = luminari_reactor_create(driver, &status);
  CuAssertPtrNotNull(tc, reactor);
  CuAssertIntEquals(tc, LUMINARI_REACTOR_OK, status);
  CuAssertIntEquals(tc, LUMINARI_REACTOR_OK, luminari_reactor_begin_cycle(reactor));
  CuAssertIntEquals(tc, LUMINARI_REACTOR_OK,
                    luminari_reactor_watch(reactor, sockets[0], LUMINARI_REACTOR_READ));
  CuAssertIntEquals(tc, LUMINARI_REACTOR_OK,
                    luminari_reactor_watch(reactor, sockets[0], LUMINARI_REACTOR_ERROR));
  CuAssertIntEquals(tc, 1, (int)luminari_reactor_watch_count(reactor));
  CuAssertIntEquals(tc, 1, (int)write(sockets[1], &marker, sizeof(marker)));

  started = luminari_reactor_monotonic_usec();
  CuAssertIntEquals(tc, LUMINARI_REACTOR_OK, luminari_reactor_wait(reactor, 30000));
  elapsed = luminari_reactor_monotonic_usec() - started;

  CuAssertTrue(tc, luminari_reactor_ready(reactor, sockets[0], LUMINARI_REACTOR_READ));
  CuAssertTrue(tc, elapsed >= 20000);
  CuAssertTrue(tc, elapsed < 500000);
  luminari_reactor_destroy(reactor);
  close(sockets[0]);
  close(sockets[1]);
}

void Test_reactor_drivers_have_equivalent_readiness_and_cadence(CuTest *tc)
{
  assert_driver_readiness_and_cadence(tc, LUMINARI_IO_DRIVER_SELECT);
  assert_driver_readiness_and_cadence(tc, LUMINARI_IO_DRIVER_LIBEVENT);
}

void Test_reactor_driver_selection_and_monotonic_clock(CuTest *tc)
{
  bool recognized = false;
  uint64_t before;
  uint64_t after;

  CuAssertIntEquals(tc, LUMINARI_IO_DRIVER_LIBEVENT,
                    luminari_io_driver_from_string(NULL, &recognized));
  CuAssertTrue(tc, recognized);
  CuAssertIntEquals(tc, LUMINARI_IO_DRIVER_SELECT,
                    luminari_io_driver_from_string("select", &recognized));
  CuAssertTrue(tc, recognized);
  CuAssertIntEquals(tc, LUMINARI_IO_DRIVER_LIBEVENT,
                    luminari_io_driver_from_string("invalid", &recognized));
  CuAssertTrue(tc, !recognized);
  before = luminari_reactor_monotonic_usec();
  usleep(2000);
  after = luminari_reactor_monotonic_usec();
  CuAssertTrue(tc, after > before);
  CuAssertPtrNotNull(tc, (void *)luminari_reactor_library_version());
}

static void count_signal(int signal_number, void *context)
{
  int *count = context;

  if (signal_number == SIGUSR1)
    (*count)++;
}

void Test_libevent_reactor_owns_registered_signals(CuTest *tc)
{
  enum luminari_reactor_status status;
  struct luminari_reactor *reactor;
  int signal_count = 0;
  int sockets[2];
  char marker = 's';

  CuAssertIntEquals(tc, 0, socketpair(AF_UNIX, SOCK_STREAM, 0, sockets));
  reactor = luminari_reactor_create(LUMINARI_IO_DRIVER_LIBEVENT, &status);
  CuAssertPtrNotNull(tc, reactor);
  CuAssertIntEquals(tc, LUMINARI_REACTOR_OK,
                    luminari_reactor_add_signal(reactor, SIGUSR1, count_signal, &signal_count));
  CuAssertIntEquals(tc, LUMINARI_REACTOR_OK, luminari_reactor_begin_cycle(reactor));
  CuAssertIntEquals(tc, LUMINARI_REACTOR_OK,
                    luminari_reactor_watch(reactor, sockets[0], LUMINARI_REACTOR_READ));
  CuAssertIntEquals(tc, 1, (int)write(sockets[1], &marker, sizeof(marker)));
  CuAssertIntEquals(tc, 0, raise(SIGUSR1));
  CuAssertIntEquals(tc, LUMINARI_REACTOR_OK, luminari_reactor_wait(reactor, 10000));
  CuAssertIntEquals(tc, 1, signal_count);
  CuAssertTrue(tc, luminari_reactor_ready(reactor, sockets[0], LUMINARI_REACTOR_READ));
  luminari_reactor_destroy(reactor);
  close(sockets[0]);
  close(sockets[1]);
}
