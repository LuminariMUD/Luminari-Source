#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/db.h"
#include "../../src/systems/intermud3/i3_client.h"

#include <json-c/json.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void i3_test_setup(void)
{
  pthread_mutex_t *command_mutex;
  pthread_mutex_t *event_mutex;
  pthread_mutex_t *state_mutex;

  i3_client = (i3_client_t *)calloc(1, sizeof(i3_client_t));
  command_mutex = (pthread_mutex_t *)calloc(1, sizeof(pthread_mutex_t));
  event_mutex = (pthread_mutex_t *)calloc(1, sizeof(pthread_mutex_t));
  state_mutex = (pthread_mutex_t *)calloc(1, sizeof(pthread_mutex_t));

  pthread_mutex_init(command_mutex, NULL);
  pthread_mutex_init(event_mutex, NULL);
  pthread_mutex_init(state_mutex, NULL);

  i3_client->command_mutex = command_mutex;
  i3_client->event_mutex = event_mutex;
  i3_client->state_mutex = state_mutex;
  i3_client->socket_fd = -1;
  i3_client->next_request_id = 1;
  i3_client->max_queue_size = I3_MAX_QUEUE_SIZE;
  i3_client->enable_who = 1;
  i3_client->enable_tell = 1;
  i3_client->enable_channels = 1;
  i3_client->event_signal_read_fd = -1;
  i3_client->event_signal_write_fd = -1;

  {
    int event_signal_fds[2];
    int flags;

    if (pipe(event_signal_fds) == 0)
    {
      i3_client->event_signal_read_fd = event_signal_fds[0];
      i3_client->event_signal_write_fd = event_signal_fds[1];
      flags = fcntl(i3_client->event_signal_read_fd, F_GETFL, 0);
      fcntl(i3_client->event_signal_read_fd, F_SETFL, flags | O_NONBLOCK);
      flags = fcntl(i3_client->event_signal_write_fd, F_GETFL, 0);
      fcntl(i3_client->event_signal_write_fd, F_SETFL, flags | O_NONBLOCK);
    }
  }
}

static void i3_test_cleanup(void)
{
  i3_command_t *command;
  i3_command_t *next_command;
  i3_event_t *event;
  i3_mud_t *mud;
  i3_mud_t *next_mud;
  pthread_mutex_t *command_mutex;
  pthread_mutex_t *event_mutex;
  pthread_mutex_t *state_mutex;

  if (!i3_client)
  {
    return;
  }

  command = i3_client->command_queue_head;
  while (command)
  {
    next_command = command->next;
    if (command->params)
    {
      json_object_put((json_object *)command->params);
    }
    free(command);
    command = next_command;
  }

  while ((event = i3_pop_event()) != NULL)
  {
    i3_free_event(event);
  }

  mud = i3_client->mud_list;
  while (mud)
  {
    next_mud = mud->next;
    free(mud);
    mud = next_mud;
  }

  command_mutex = (pthread_mutex_t *)i3_client->command_mutex;
  event_mutex = (pthread_mutex_t *)i3_client->event_mutex;
  state_mutex = (pthread_mutex_t *)i3_client->state_mutex;
  pthread_mutex_destroy(command_mutex);
  pthread_mutex_destroy(event_mutex);
  pthread_mutex_destroy(state_mutex);
  free(command_mutex);
  free(event_mutex);
  free(state_mutex);
  free(i3_client->receive_buffer);
  if (i3_client->event_signal_read_fd >= 0)
  {
    close(i3_client->event_signal_read_fd);
  }
  if (i3_client->event_signal_write_fd >= 0)
  {
    close(i3_client->event_signal_write_fd);
  }
  free(i3_client);
  i3_client = NULL;
}

static json_object *i3_test_mud(const char *name, int port)
{
  json_object *mud;
  json_object *services;

  mud = json_object_new_object();
  services = json_object_new_object();
  json_object_object_add(services, "tell", json_object_new_int(1));
  json_object_object_add(mud, "name", json_object_new_string(name));
  json_object_object_add(mud, "host", json_object_new_string("192.0.2.10"));
  json_object_object_add(mud, "port", json_object_new_int(port));
  json_object_object_add(mud, "driver", json_object_new_string("TestDriver"));
  json_object_object_add(mud, "mud_type", json_object_new_string("Diku"));
  json_object_object_add(mud, "status", json_object_new_string("up"));
  json_object_object_add(mud, "admin_email", json_object_new_string("admin@example.com"));
  json_object_object_add(mud, "services", services);
  return mud;
}

void Test_i3_fragmented_large_mudlist_response(CuTest *tc)
{
  json_object *root;
  json_object *result;
  json_object *muds;
  const char *json_text;
  char mud_name[64];
  i3_mud_t *mud;
  size_t json_length;
  int index;
  int mud_count;

  i3_test_setup();

  root = json_object_new_object();
  result = json_object_new_object();
  muds = json_object_new_array();
  for (index = 0; index < 100; index++)
  {
    snprintf(mud_name, sizeof(mud_name), "RegressionMUD%03d", index);
    json_object_array_add(muds, i3_test_mud(mud_name, 4000 + index));
  }
  json_object_object_add(result, "status", json_object_new_string("success"));
  json_object_object_add(result, "muds", muds);
  json_object_object_add(result, "count", json_object_new_int(100));
  json_object_object_add(root, "jsonrpc", json_object_new_string("2.0"));
  json_object_object_add(root, "id", json_object_new_int(2));
  json_object_object_add(root, "result", result);

  json_text = json_object_to_json_string_ext(root, JSON_C_TO_STRING_PLAIN);
  json_length = strlen(json_text);
  CuAssertTrue(tc, json_length > I3_MAX_STRING_LENGTH);

  CuAssertIntEquals(tc, 0, i3_process_input(json_text, 37));
  CuAssertIntEquals(tc, 0, i3_process_input(json_text + 37, 4096));
  CuAssertIntEquals(tc, 0, i3_process_input(json_text + 4133, json_length - 4133));
  CuAssertIntEquals(tc, 0, i3_client->event_queue_size);
  CuAssertIntEquals(tc, 1, i3_process_input("\n", 1));
  CuAssertIntEquals(tc, 1, i3_client->event_queue_size);

  i3_process_events();

  mud_count = 0;
  for (mud = i3_client->mud_list; mud; mud = mud->next)
  {
    mud_count++;
  }
  CuAssertIntEquals(tc, 100, mud_count);
  CuAssertPtrNotNull(tc, i3_find_mud("RegressionMUD042"));
  CuAssertIntEquals(tc, 4042, i3_find_mud("RegressionMUD042")->port);
  CuAssertIntEquals(tc, 1, i3_client->messages_received);

  json_object_put(root);
  i3_test_cleanup();
}

void Test_i3_line_framing_preserves_partial_notifications(CuTest *tc)
{
  const char *first;
  const char *second;
  char combined[2048];
  size_t first_length;
  size_t second_length;
  size_t split;
  i3_event_t *event;

  i3_test_setup();
  first = "{\"jsonrpc\":\"2.0\",\"method\":\"tell_received\",\"params\":{\"from_user\":"
          "\"Alpha\",\"from_mud\":\"FirstMUD\",\"to_user\":\"Tester\",\"message\":\"one\"}}\n";
  second = "{\"jsonrpc\":\"2.0\",\"method\":\"tell_received\",\"params\":{\"from_user\":"
           "\"Beta\",\"from_mud\":\"SecondMUD\",\"to_user\":\"Tester\",\"message\":\"two\"}}\n";
  first_length = strlen(first);
  second_length = strlen(second);
  split = second_length / 2;
  memcpy(combined, first, first_length);
  memcpy(combined + first_length, second, split);

  CuAssertIntEquals(tc, 1, i3_process_input(combined, first_length + split));
  event = i3_pop_event();
  CuAssertPtrNotNull(tc, event);
  CuAssertStrEquals(tc, "Alpha", event->from_user);
  CuAssertStrEquals(tc, "one", event->message);
  i3_free_event(event);
  CuAssertIntEquals(tc, 0, i3_client->event_queue_size);

  CuAssertIntEquals(tc, 1, i3_process_input(second + split, second_length - split));
  event = i3_pop_event();
  CuAssertPtrNotNull(tc, event);
  CuAssertStrEquals(tc, "Beta", event->from_user);
  CuAssertStrEquals(tc, "two", event->message);
  i3_free_event(event);
  CuAssertIntEquals(tc, 2, i3_client->messages_received);

  i3_test_cleanup();
}

void Test_i3_event_queue_signals_idle_main_loop(CuTest *tc)
{
  const char *notification;
  unsigned char signal_byte;
  i3_event_t *event;
  ssize_t bytes_read;

  i3_test_setup();
  notification = "{\"jsonrpc\":\"2.0\",\"method\":\"tell_received\",\"params\":{\"from_user\":"
                 "\"WakeTester\",\"from_mud\":\"RemoteMUD\",\"to_user\":\"Tester\","
                 "\"message\":\"wake\"}}\n";

  CuAssertIntEquals(tc, 1, i3_process_input(notification, strlen(notification)));
  CuAssertTrue(tc, i3_get_event_fd() >= 0);
  bytes_read = read(i3_get_event_fd(), &signal_byte, sizeof(signal_byte));
  CuAssertIntEquals(tc, 1, (int)bytes_read);

  event = i3_pop_event();
  CuAssertPtrNotNull(tc, event);
  CuAssertStrEquals(tc, "WakeTester", event->from_user);
  i3_free_event(event);
  i3_test_cleanup();
}

void Test_i3_gateway_method_contract(CuTest *tc)
{
  i3_command_t *command;
  json_object *params;
  json_object *value;

  i3_test_setup();

  CuAssertIntEquals(tc, 0, i3_request_who("RemoteMUD", "Tester"));
  CuAssertIntEquals(tc, 0, i3_request_mudlist());

  command = i3_client->command_queue_head;
  CuAssertPtrNotNull(tc, command);
  CuAssertStrEquals(tc, "who", command->method);
  params = (json_object *)command->params;
  value = NULL;
  CuAssertTrue(tc, json_object_object_get_ex(params, "from_user", &value));
  CuAssertStrEquals(tc, "Tester", json_object_get_string(value));

  command = command->next;
  CuAssertPtrNotNull(tc, command);
  CuAssertStrEquals(tc, "mudlist", command->method);

  i3_test_cleanup();
}

void Test_i3_authentication_primes_network_caches(CuTest *tc)
{
  i3_command_t *command;
  const char *response;

  i3_test_setup();
  response = "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{\"status\":\"authenticated\","
             "\"mud_name\":\"LuminariMUD\",\"session_id\":\"regression-session\"}}";

  CuAssertIntEquals(tc, 0, i3_parse_response(response));
  CuAssertIntEquals(tc, I3_STATE_CONNECTED, i3_client->state);
  CuAssertIntEquals(tc, 1, i3_client->authenticated);
  CuAssertStrEquals(tc, "regression-session", i3_client->session_id);

  command = i3_client->command_queue_head;
  CuAssertPtrNotNull(tc, command);
  CuAssertStrEquals(tc, "mudlist", command->method);
  command = command->next;
  CuAssertPtrNotNull(tc, command);
  CuAssertStrEquals(tc, "channel_list", command->method);

  i3_test_cleanup();
}

void Test_i3_direct_message_lookup_does_not_require_a_viewer(CuTest *tc)
{
  const char *notification;
  struct char_data player;
  struct player_special_data player_specials;
  struct char_data *saved_character_list;

  memset(&player, 0, sizeof(player));
  memset(&player_specials, 0, sizeof(player_specials));
  player.player.name = "TargetPlayer";
  player.player_specials = &player_specials;

  saved_character_list = character_list;
  character_list = &player;
  i3_test_setup();

  CuAssertPtrEquals(tc, &player, i3_find_online_player_for_test("targetplayer"));
  CuAssertPtrEquals(tc, NULL, i3_find_online_player_for_test("MissingPlayer"));
  CuAssertPtrEquals(tc, NULL, i3_find_online_player_for_test(NULL));

  notification = "{\"jsonrpc\":\"2.0\",\"method\":\"tell_received\",\"params\":{\"from_user\":"
                 "\"RemotePlayer\",\"from_mud\":\"RemoteMUD\",\"to_user\":\"TargetPlayer\","
                 "\"message\":\"safe delivery\"}}\n";
  CuAssertIntEquals(tc, 1, i3_process_input(notification, strlen(notification)));
  CuAssertIntEquals(tc, 1, i3_client->event_queue_size);

  i3_process_events();

  CuAssertIntEquals(tc, 0, i3_client->event_queue_size);
  CuAssertIntEquals(tc, 1, i3_client->messages_received);

  i3_test_cleanup();
  character_list = saved_character_list;
}
