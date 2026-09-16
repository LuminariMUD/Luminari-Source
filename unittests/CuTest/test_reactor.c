#include "CuTest.h"

#include "conf.h"
#include "../../src/core/sysdep.h"
#include "../../src/core/structs.h"
#include "../../src/core/utils.h"
#include "../../src/core/comm.h"
#include "../../src/core/db.h"
#include "../../src/net/reactor.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <stdint.h>
#include <string.h>
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
  CuAssertIntEquals(tc, LUMINARI_REACTOR_OK, luminari_reactor_wait(reactor, 1000000));
  elapsed = luminari_reactor_monotonic_usec() - started;

  CuAssertTrue(tc, luminari_reactor_ready(reactor, sockets[0], LUMINARI_REACTOR_READ));
  CuAssertTrue(tc, elapsed < 500000);
  CuAssertIntEquals(tc, 1, (int)read(sockets[0], &marker, sizeof(marker)));
  CuAssertIntEquals(tc, LUMINARI_REACTOR_OK, luminari_reactor_begin_cycle(reactor));
  CuAssertIntEquals(tc, LUMINARI_REACTOR_OK,
                    luminari_reactor_watch(reactor, sockets[0], LUMINARI_REACTOR_READ));
  started = luminari_reactor_monotonic_usec();
  CuAssertIntEquals(tc, LUMINARI_REACTOR_OK, luminari_reactor_wait(reactor, 30000));
  elapsed = luminari_reactor_monotonic_usec() - started;
  CuAssertTrue(tc, elapsed >= 20000);
  CuAssertTrue(tc, !luminari_reactor_ready(reactor, sockets[0], LUMINARI_REACTOR_READ));
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
  CuAssertPtrNotNull(tc, luminari_reactor_library_version());
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

void Test_reactor_shutdown_preserves_terminating_signal_handlers(CuTest *tc)
{
#ifdef CIRCLE_UNIX
  CuAssertTrue(tc, comm_test_preserve_shutdown_signal_handlers());
#else
  CuAssertTrue(tc, true);
#endif
}

/* True once fd is readable within timeout_ms. */
static bool lifecycle_readable(int fd, int timeout_ms)
{
  struct pollfd waiting;

  waiting.fd = fd;
  waiting.events = POLLIN;
  waiting.revents = 0;
  return poll(&waiting, 1, timeout_ms) == 1;
}

/* The server's descriptor lifecycle on a real loopback connection: accepting
 * lists a new descriptor with its greeting queued, output reaches the client,
 * client input lands in the command queue, and close_socket unlists and frees
 * the descriptor and closes the connection. */
void Test_descriptor_lifecycle_accepts_serves_and_closes_a_connection(CuTest *tc)
{
  struct descriptor_data *saved_descriptors = descriptor_list;
  struct descriptor_data *descriptor = NULL;
  struct sockaddr_in address;
  socklen_t address_length = sizeof(address);
  char *saved_greetings = GREETINGS;
  int saved_ns_is_slow = CONFIG_NS_IS_SLOW;
  int saved_negotiation = CONFIG_PROTOCOL_NEGOTIATION;
  int saved_max_playing = CONFIG_MAX_PLAYING;
  char received[512];
  char host[HOST_LENGTH + 1];
  ssize_t length = -1;
  int listener;
  int client = -1;
  int accepted = -1;
  int written = -1;
  int read_result = -1;
  int state = -1;
  bool connected = false;
  bool listed = false;
  bool greeting_queued = false;
  bool greeting_received = false;
  bool input_queued = false;
  bool unlisted = false;
  bool closed_for_client = false;

  host[0] = '\0';
  memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  listener = socket(AF_INET, SOCK_STREAM, 0);
  if (listener >= 0 && bind(listener, (struct sockaddr *)&address, sizeof(address)) == 0 &&
      listen(listener, 1) == 0 &&
      getsockname(listener, (struct sockaddr *)&address, &address_length) == 0)
  {
    client = socket(AF_INET, SOCK_STREAM, 0);
    connected = client >= 0 && connect(client, (struct sockaddr *)&address, sizeof(address)) == 0;
  }

  if (connected)
  {
    GREETINGS = CuMutableString("Lifecycle greeting\r\n");
    CONFIG_NS_IS_SLOW = YES; /* no reverse lookup */
    CONFIG_PROTOCOL_NEGOTIATION = NO;
    CONFIG_MAX_PLAYING = 8;
    descriptor_list = NULL;

    accepted = new_descriptor_for_test(listener);
    descriptor = descriptor_list;
    listed = descriptor != NULL && descriptor->next == NULL;
  }
  if (listed)
  {
    strlcpy(host, descriptor->host, sizeof(host));
    state = STATE(descriptor);
    greeting_queued = strstr(descriptor->output, "Lifecycle greeting") != NULL;
    written = process_output_for_test(descriptor);
    if (lifecycle_readable(client, 5000))
    {
      length = recv(client, received, sizeof(received) - 1, 0);
      received[length > 0 ? length : 0] = '\0';
      greeting_received = strstr(received, "Lifecycle greeting") != NULL;
    }

    if (send(client, "look around\r\n", 13, 0) == 13 &&
        lifecycle_readable(descriptor->descriptor, 5000))
    {
      read_result = process_input_for_test(descriptor);
      input_queued = descriptor->input.head != NULL &&
                     strcmp(descriptor->input.head->text, "look around") == 0;
    }

    close_socket(descriptor);
    unlisted = descriptor_list == NULL;
    closed_for_client =
        lifecycle_readable(client, 5000) && recv(client, received, sizeof(received), 0) == 0;
  }

  descriptor_list = saved_descriptors;
  GREETINGS = saved_greetings;
  CONFIG_NS_IS_SLOW = saved_ns_is_slow;
  CONFIG_PROTOCOL_NEGOTIATION = saved_negotiation;
  CONFIG_MAX_PLAYING = saved_max_playing;
  if (client >= 0)
    close(client);
  if (listener >= 0)
    close(listener);

  CuAssertTrue(tc, connected);
  CuAssertIntEquals(tc, 0, accepted);
  CuAssertTrue(tc, listed);
  CuAssertStrEquals(tc, "127.0.0.1", host);
  CuAssertIntEquals(tc, CON_ACCOUNT_NAME, state);
  CuAssertTrue(tc, greeting_queued);
  CuAssertTrue(tc, written > 0);
  CuAssertTrue(tc, greeting_received);
  CuAssertTrue(tc, read_result >= 0);
  CuAssertTrue(tc, input_queued);
  CuAssertTrue(tc, unlisted);
  CuAssertTrue(tc, closed_for_client);
}
