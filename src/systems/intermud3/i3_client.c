/* ************************************************************************
*  Intermud3 Client Implementation for LuminariMUD                       *
*  Core client functionality and connection management                    *
*  Based on CircleMUD/tbaMUD implementation - adapted for LuminariMUD    *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "screen.h"
#include "act.h"

#include "systems/intermud3/i3_client.h"
/* Temporarily disabled for compilation
#include "systems/intermud3/i3_security.h"
#include "systems/intermud3/i3_error.h"
*/

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <stdarg.h>
#include <pthread.h>
#include <json-c/json.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

/* Define UNUSED_VAR if not already defined */
#ifndef UNUSED_VAR
#define UNUSED_VAR(x) ((void)(x))
#endif

/* Global client instance */
i3_client_t *i3_client = NULL;

/* Forward declarations */
static int i3_socket_connect(const char *host, int port);
static int i3_authenticate(void);
void i3_queue_command(i3_command_t *cmd);
static void i3_queue_event(i3_event_t *event);
static i3_command_t *i3_pop_command(void);
static void i3_free_command(i3_command_t *cmd);
static void i3_heartbeat(void);
static void i3_reconnect(void);
static void i3_update_mudlist(json_object *result_obj);
static void i3_update_channel_list(json_object *result_obj);
static struct char_data *i3_find_online_player(const char *name);

/* Initialize the I3 client */
int i3_initialize(void)
{
  pthread_t *thread_ptr;

  /* Initialize security and error systems first - TEMPORARILY DISABLED */
  /* i3_init_security_system(I3_SECURITY_VALIDATE_JSON | I3_SECURITY_SANITIZE_INPUT | I3_SECURITY_CHECK_RATES);
    i3_init_error_system(); */

  i3_client = (i3_client_t *)calloc(1, sizeof(i3_client_t));
  if (!i3_client)
  {
    /* I3_CRITICAL(I3_ERROR_MEMORY, ENOMEM, "Failed to allocate I3 client structure");
        i3_shutdown_security_system();
        i3_shutdown_error_system(); */
    i3_log("I3: Failed to allocate client structure");
    return -1;
  }
  i3_client->event_signal_read_fd = -1;
  i3_client->event_signal_write_fd = -1;

  /* Allocate pthread structures */
  i3_client->thread_id = calloc(1, sizeof(pthread_t));
  i3_client->command_mutex = calloc(1, sizeof(pthread_mutex_t));
  i3_client->event_mutex = calloc(1, sizeof(pthread_mutex_t));
  i3_client->state_mutex = calloc(1, sizeof(pthread_mutex_t));

  /* Initialize mutexes */
  pthread_mutex_init((pthread_mutex_t *)i3_client->command_mutex, NULL);
  pthread_mutex_init((pthread_mutex_t *)i3_client->event_mutex, NULL);
  pthread_mutex_init((pthread_mutex_t *)i3_client->state_mutex, NULL);

  {
    int event_signal_fds[2];
    int flags;

    if (pipe(event_signal_fds) == 0)
    {
      i3_client->event_signal_read_fd = event_signal_fds[0];
      i3_client->event_signal_write_fd = event_signal_fds[1];

      flags = fcntl(i3_client->event_signal_read_fd, F_GETFL, 0);
      if (flags >= 0)
      {
        fcntl(i3_client->event_signal_read_fd, F_SETFL, flags | O_NONBLOCK);
      }
      flags = fcntl(i3_client->event_signal_write_fd, F_GETFL, 0);
      if (flags >= 0)
      {
        fcntl(i3_client->event_signal_write_fd, F_SETFL, flags | O_NONBLOCK);
      }
    }
    else
    {
      i3_error("Unable to create the I3 main-loop wake pipe: %s", strerror(errno));
    }
  }

  /* Set defaults */
  i3_client->state = I3_STATE_DISCONNECTED;
  i3_client->socket_fd = -1;
  i3_client->authenticated = 0;
  i3_client->next_request_id = 1;
  i3_client->max_queue_size = I3_MAX_QUEUE_SIZE;
  i3_client->reconnect_delay = I3_RECONNECT_DELAY;

  /* Default configuration */
  i3_client->enable_tell = 1;
  i3_client->enable_channels = 1;
  i3_client->enable_who = 1;
  i3_client->auto_reconnect = 1;

  /* Set default gateway */
  strlcpy(i3_client->gateway_host, "localhost", sizeof(i3_client->gateway_host));
  i3_client->gateway_port = I3_DEFAULT_PORT;
  strlcpy(i3_client->mud_name, "LuminariMUD", sizeof(i3_client->mud_name));
  strlcpy(i3_client->default_channel, "intermud", sizeof(i3_client->default_channel));

  /* Load configuration - MUD runs from lib directory */
  if (i3_load_config("i3_config") < 0)
  {
    log("Warning: Could not load I3 configuration, using defaults");
  }

  /* Create client thread */
  thread_ptr = (pthread_t *)i3_client->thread_id;
  if (pthread_create(thread_ptr, NULL, i3_client_thread, NULL) != 0)
  {
    /* I3_CRITICAL(I3_ERROR_THREADING, errno, "Failed to create I3 client thread"); */
    i3_log("I3: Failed to create client thread");
    free(i3_client->thread_id);
    free(i3_client->command_mutex);
    free(i3_client->event_mutex);
    free(i3_client->state_mutex);
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
    /* i3_shutdown_security_system();
        i3_shutdown_error_system(); */
    return -1;
  }

  i3_log("I3 client initialized successfully");
  return 0;
}

/* Shutdown the I3 client */
void i3_shutdown(void)
{
  pthread_t *thread_ptr;
  pthread_mutex_t *mutex_ptr;
  i3_command_t *cmd;
  i3_event_t *event;
  i3_mud_t *mud, *next_mud;

  if (!i3_client)
    return;

  i3_log("Shutting down I3 client");

  /* Signal shutdown */
  mutex_ptr = (pthread_mutex_t *)i3_client->state_mutex;
  pthread_mutex_lock(mutex_ptr);
  i3_client->state = I3_STATE_SHUTDOWN;
  pthread_mutex_unlock(mutex_ptr);

  /* Wait for thread to finish */
  thread_ptr = (pthread_t *)i3_client->thread_id;
  pthread_join(*thread_ptr, NULL);

  /* Clean up queues */
  while (i3_client->command_queue_head)
  {
    cmd = i3_pop_command();
    i3_free_command(cmd);
  }

  while (i3_client->event_queue_head)
  {
    event = i3_pop_event();
    i3_free_event(event);
  }

  /* Clean up MUD list */
  mud = i3_client->mud_list;
  while (mud)
  {
    next_mud = mud->next;
    free(mud);
    mud = next_mud;
  }

  /* Destroy mutexes */
  pthread_mutex_destroy((pthread_mutex_t *)i3_client->command_mutex);
  pthread_mutex_destroy((pthread_mutex_t *)i3_client->event_mutex);
  pthread_mutex_destroy((pthread_mutex_t *)i3_client->state_mutex);

  /* Free pthread structures */
  free(i3_client->thread_id);
  free(i3_client->command_mutex);
  free(i3_client->event_mutex);
  free(i3_client->state_mutex);
  free(i3_client->receive_buffer);
  if (i3_client->event_signal_read_fd >= 0)
  {
    close(i3_client->event_signal_read_fd);
  }
  if (i3_client->event_signal_write_fd >= 0)
  {
    close(i3_client->event_signal_write_fd);
  }

  /* Free client structure */
  free(i3_client);
  i3_client = NULL;

  /* Shutdown security and error systems */
  /* i3_shutdown_security_system();
    i3_shutdown_error_system(); */
}

/* Main client thread */
void *i3_client_thread(void *arg)
{
  char buffer[I3_MAX_STRING_LENGTH];
  fd_set read_set;
  struct timeval timeout;
  time_t last_heartbeat = time(NULL);
  time_t now;
  int result, bytes;
  i3_command_t *cmd;
  json_object *request;

  UNUSED_VAR(arg);

  i3_log("I3 client thread started");

  /* Initial connection */
  if (i3_connect() == 0)
  {
    /* Don't authenticate immediately - wait for welcome message */
    i3_log("DEBUG: Connected/Reconnected, waiting for welcome message");
  }

  /* Main loop */
  while (i3_client->state != I3_STATE_SHUTDOWN)
  {
    /* Check for reconnection */
    if (i3_client->state == I3_STATE_DISCONNECTED && i3_client->auto_reconnect)
    {
      for (result = 0; result < i3_client->reconnect_delay && i3_client->state != I3_STATE_SHUTDOWN;
           result++)
      {
        sleep(1);
      }
      if (i3_client->state == I3_STATE_SHUTDOWN)
      {
        continue;
      }
      i3_reconnect();
      continue;
    }

    /* Process outgoing commands */
    cmd = i3_pop_command();
    if (cmd)
    {
      request = (json_object *)i3_create_request(cmd->method, cmd->params);
      cmd->params = NULL;
      if (i3_send_json(request) < 0)
      {
        i3_disconnect();
      }
      json_object_put(request);
      i3_free_command(cmd);
    }

    /* Check for incoming data */
    if (i3_client->socket_fd >= 0)
    {
      FD_ZERO(&read_set);
      FD_SET(i3_client->socket_fd, &read_set);
      timeout.tv_sec = 1;
      timeout.tv_usec = 0;

      result = select(i3_client->socket_fd + 1, &read_set, NULL, NULL, &timeout);
      if (result > 0 && FD_ISSET(i3_client->socket_fd, &read_set))
      {
        bytes = recv(i3_client->socket_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes > 0)
        {
          buffer[bytes] = '\0';
          i3_log("DEBUG: Received %d bytes: %.200s%s", bytes, buffer, (bytes > 200 ? "..." : ""));

          if (i3_process_input(buffer, (size_t)bytes) < 0)
          {
            i3_error("Invalid or oversized response from I3 gateway");
            i3_disconnect();
          }
        }
        else if (bytes == 0 ||
                 (bytes < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR))
        {
          if (bytes == 0)
          {
            i3_error("Connection closed by I3 gateway");
          }
          else
          {
            i3_error("Connection lost: %s", strerror(errno));
          }
          i3_disconnect();
        }
      }
      else if (result < 0 && errno != EINTR)
      {
        i3_error("I3 socket select failed: %s", strerror(errno));
        i3_disconnect();
      }
    }

    /* Send heartbeat */
    now = time(NULL);
    if (i3_client->state == I3_STATE_CONNECTED && now - last_heartbeat >= I3_HEARTBEAT_INTERVAL)
    {
      i3_heartbeat();
      last_heartbeat = now;
    }
  }

  i3_log("I3 client thread terminating");
  i3_disconnect();
  return NULL;
}

/* Connect to the I3 gateway */
int i3_connect(void)
{
  pthread_mutex_t *mutex_ptr;

  mutex_ptr = (pthread_mutex_t *)i3_client->state_mutex;
  pthread_mutex_lock(mutex_ptr);
  i3_client->state = I3_STATE_CONNECTING;
  pthread_mutex_unlock(mutex_ptr);

  i3_log("Connecting to I3 gateway at %s:%d", i3_client->gateway_host, i3_client->gateway_port);

  i3_client->socket_fd = i3_socket_connect(i3_client->gateway_host, i3_client->gateway_port);
  if (i3_client->socket_fd < 0)
  {
    i3_error("Failed to connect to I3 gateway");
    pthread_mutex_lock(mutex_ptr);
    i3_client->state = I3_STATE_DISCONNECTED;
    pthread_mutex_unlock(mutex_ptr);
    return -1;
  }

  i3_client->connect_time = time(NULL);
  i3_log("Connected to I3 gateway");
  return 0;
}

/* Disconnect from the I3 gateway */
void i3_disconnect(void)
{
  pthread_mutex_t *mutex_ptr;

  if (i3_client->socket_fd >= 0)
  {
    close(i3_client->socket_fd);
    i3_client->socket_fd = -1;
  }

  mutex_ptr = (pthread_mutex_t *)i3_client->state_mutex;
  pthread_mutex_lock(mutex_ptr);
  if (i3_client->state != I3_STATE_SHUTDOWN)
  {
    i3_client->state = I3_STATE_DISCONNECTED;
  }
  pthread_mutex_unlock(mutex_ptr);

  i3_client->authenticated = 0;
  i3_client->receive_length = 0;
  i3_log("Disconnected from I3 gateway");
}

/* Check if connected */
int i3_is_connected(void)
{
  return (i3_client && i3_client->state == I3_STATE_CONNECTED) ? 1 : 0;
}

/* Create TCP socket connection */
static int i3_socket_connect(const char *host, int port)
{
  struct sockaddr_in server_addr;
  struct hostent *server;
  int sock;
  int flags;

  /* Create socket */
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
  {
    i3_error("Failed to create socket: %s", strerror(errno));
    return -1;
  }

  /* Resolve hostname */
  server = gethostbyname(host);
  if (!server)
  {
    i3_error("Failed to resolve hostname: %s", host);
    close(sock);
    return -1;
  }

  /* Set up server address */
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
  server_addr.sin_port = htons(port);

  /* Connect */
  i3_log("DEBUG: Attempting TCP connection to %s:%d", host, port);
  if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
  {
    i3_error("Failed to connect: %s", strerror(errno));
    close(sock);
    return -1;
  }
  i3_log("DEBUG: TCP connection established, socket fd: %d", sock);

  /* Set non-blocking */
  flags = fcntl(sock, F_GETFL, 0);
  fcntl(sock, F_SETFL, flags | O_NONBLOCK);

  return sock;
}

/* Authenticate with the I3 gateway */
static int i3_authenticate(void)
{
  json_object *params;
  json_object *request;
  int result;
  pthread_mutex_t *mutex_ptr;

  params = json_object_new_object();
  json_object_object_add(params, "api_key", json_object_new_string(i3_client->api_key));

  request = (json_object *)i3_create_request("authenticate", params);

  result = i3_send_json(request);
  i3_log("DEBUG: Auth send result: %d", result);
  json_object_put(request);

  if (result < 0)
  {
    i3_error("Failed to send authentication request");
    return -1;
  }

  mutex_ptr = (pthread_mutex_t *)i3_client->state_mutex;
  pthread_mutex_lock(mutex_ptr);
  i3_client->state = I3_STATE_AUTHENTICATING;
  pthread_mutex_unlock(mutex_ptr);

  i3_log("Authentication request sent");
  return 0;
}

/* Send heartbeat */
static void i3_heartbeat(void)
{
  json_object *request;

  request = (json_object *)i3_create_request("heartbeat", NULL);
  i3_send_json(request);
  json_object_put(request);
  i3_client->last_heartbeat = time(NULL);
}

/* Reconnect to gateway */
static void i3_reconnect(void)
{
  i3_log("Attempting to reconnect to I3 gateway");
  i3_client->reconnects++;

  if (i3_connect() == 0)
  {
    /* Don't authenticate immediately - wait for welcome message */
    i3_log("DEBUG: Connected/Reconnected, waiting for welcome message");
  }
}

/* Parse one complete JSON-RPC message from the gateway. */
int i3_parse_response(const char *json_str)
{
  json_object *root;
  json_object *method_obj;
  json_object *params_obj;
  json_object *result_obj;
  json_object *status_obj;
  json_object *mud_name_obj;
  json_object *session_obj;
  json_object *error_obj;
  json_object *message_obj;
  json_object *service_obj;
  json_object *version_obj;
  json_object *from_user_obj;
  json_object *from_mud_obj;
  json_object *to_user_obj;
  json_object *channel_obj;
  json_object *muds_obj;
  json_object *channels_obj;
  const char *method;
  const char *status;
  i3_event_t *event;
  pthread_mutex_t *mutex_ptr;

  if (!i3_client || !json_str)
  {
    return -1;
  }

  root = json_tokener_parse(json_str);
  if (!root || !json_object_is_type(root, json_type_object))
  {
    i3_error("Failed to parse JSON response");
    if (root)
    {
      json_object_put(root);
    }
    return -1;
  }

  i3_client->messages_received++;
  method_obj = NULL;
  params_obj = NULL;
  result_obj = NULL;
  error_obj = NULL;

  if (!json_object_object_get_ex(root, "method", &method_obj))
  {
    if (json_object_object_get_ex(root, "error", &error_obj))
    {
      event = (i3_event_t *)calloc(1, sizeof(i3_event_t));
      if (event)
      {
        event->type = I3_MSG_ERROR;
        if (json_object_object_get_ex(error_obj, "message", &message_obj))
        {
          strlcpy(event->message, json_object_get_string(message_obj), sizeof(event->message));
        }
        else
        {
          strlcpy(event->message, "Unknown JSON-RPC error", sizeof(event->message));
        }
        event->data = json_object_get(error_obj);
        i3_queue_event(event);
      }
      json_object_put(root);
      return 0;
    }

    if (!json_object_object_get_ex(root, "result", &result_obj) ||
        !json_object_is_type(result_obj, json_type_object))
    {
      json_object_put(root);
      return 0;
    }

    status_obj = NULL;
    if (json_object_object_get_ex(result_obj, "status", &status_obj))
    {
      status = json_object_get_string(status_obj);
      if (status && strcmp(status, "authenticated") == 0)
      {
        mutex_ptr = (pthread_mutex_t *)i3_client->state_mutex;
        pthread_mutex_lock(mutex_ptr);
        i3_client->state = I3_STATE_CONNECTED;
        i3_client->authenticated = 1;
        pthread_mutex_unlock(mutex_ptr);

        mud_name_obj = NULL;
        session_obj = NULL;
        json_object_object_get_ex(result_obj, "mud_name", &mud_name_obj);
        json_object_object_get_ex(result_obj, "session_id", &session_obj);

        i3_log("Successfully authenticated with I3 gateway");
        if (mud_name_obj)
        {
          i3_log("MUD Name confirmed: %s", json_object_get_string(mud_name_obj));
        }
        if (session_obj)
        {
          strlcpy(i3_client->session_id, json_object_get_string(session_obj),
                  sizeof(i3_client->session_id));
          i3_log("Session established");
        }

        /* Prime the local caches as soon as the authenticated session is ready. */
        i3_request_mudlist();
        i3_list_channels();
      }
      else if (status && (!strcmp(status, "joined") || !strcmp(status, "left")))
      {
        channel_obj = NULL;
        if (json_object_object_get_ex(result_obj, "channel", &channel_obj))
        {
          event = (i3_event_t *)calloc(1, sizeof(i3_event_t));
          if (event)
          {
            event->type = !strcmp(status, "joined") ? I3_MSG_CHANNEL_JOIN : I3_MSG_CHANNEL_LEAVE;
            strlcpy(event->channel, json_object_get_string(channel_obj), sizeof(event->channel));
            i3_queue_event(event);
          }
        }
      }
    }

    muds_obj = NULL;
    if (json_object_object_get_ex(result_obj, "muds", &muds_obj) &&
        json_object_is_type(muds_obj, json_type_array))
    {
      event = (i3_event_t *)calloc(1, sizeof(i3_event_t));
      if (event)
      {
        event->type = I3_MSG_MUDLIST_REPLY;
        event->data = json_object_get(result_obj);
        i3_queue_event(event);
      }
    }

    channels_obj = NULL;
    if (json_object_object_get_ex(result_obj, "channels", &channels_obj) &&
        json_object_is_type(channels_obj, json_type_array))
    {
      event = (i3_event_t *)calloc(1, sizeof(i3_event_t));
      if (event)
      {
        event->type = I3_MSG_CHANNEL_LIST_REPLY;
        event->data = json_object_get(result_obj);
        i3_queue_event(event);
      }
    }

    json_object_put(root);
    return 0;
  }

  method = json_object_get_string(method_obj);
  json_object_object_get_ex(root, "params", &params_obj);

  if (!method)
  {
    json_object_put(root);
    return -1;
  }

  /* The welcome notification is the signal to authenticate. */
  if (strcmp(method, "welcome") == 0)
  {
    i3_log("DEBUG: Received welcome message from gateway");
    if (params_obj)
    {
      service_obj = NULL;
      version_obj = NULL;
      json_object_object_get_ex(params_obj, "service", &service_obj);
      json_object_object_get_ex(params_obj, "version", &version_obj);
      if (service_obj)
      {
        i3_log("Gateway service: %s", json_object_get_string(service_obj));
      }
      if (version_obj)
      {
        i3_log("Gateway version: %s", json_object_get_string(version_obj));
      }
    }
    i3_authenticate();
  }
  else if (strcmp(method, "tell_received") == 0 || strcmp(method, "emoteto_received") == 0)
  {
    from_user_obj = NULL;
    from_mud_obj = NULL;
    to_user_obj = NULL;
    message_obj = NULL;
    if (params_obj && json_object_object_get_ex(params_obj, "from_user", &from_user_obj) &&
        json_object_object_get_ex(params_obj, "from_mud", &from_mud_obj) &&
        json_object_object_get_ex(params_obj, "to_user", &to_user_obj) &&
        json_object_object_get_ex(params_obj, "message", &message_obj))
    {
      event = (i3_event_t *)calloc(1, sizeof(i3_event_t));
      if (event)
      {
        event->type = strcmp(method, "tell_received") == 0 ? I3_MSG_TELL : I3_MSG_EMOTETO;
        strlcpy(event->from_user, json_object_get_string(from_user_obj), sizeof(event->from_user));
        strlcpy(event->from_mud, json_object_get_string(from_mud_obj), sizeof(event->from_mud));
        strlcpy(event->to_user, json_object_get_string(to_user_obj), sizeof(event->to_user));
        strlcpy(event->message, json_object_get_string(message_obj), sizeof(event->message));
        i3_queue_event(event);
      }
    }
  }
  else if (strcmp(method, "channel_message") == 0 || strcmp(method, "channel_emote") == 0)
  {
    channel_obj = NULL;
    from_user_obj = NULL;
    from_mud_obj = NULL;
    message_obj = NULL;
    if (params_obj && json_object_object_get_ex(params_obj, "channel", &channel_obj) &&
        json_object_object_get_ex(params_obj, "from_user", &from_user_obj) &&
        json_object_object_get_ex(params_obj, "from_mud", &from_mud_obj) &&
        json_object_object_get_ex(params_obj, "message", &message_obj))
    {
      event = (i3_event_t *)calloc(1, sizeof(i3_event_t));
      if (event)
      {
        event->type =
            strcmp(method, "channel_message") == 0 ? I3_MSG_CHANNEL : I3_MSG_CHANNEL_EMOTE;
        strlcpy(event->channel, json_object_get_string(channel_obj), sizeof(event->channel));
        strlcpy(event->from_user, json_object_get_string(from_user_obj), sizeof(event->from_user));
        strlcpy(event->from_mud, json_object_get_string(from_mud_obj), sizeof(event->from_mud));
        strlcpy(event->message, json_object_get_string(message_obj), sizeof(event->message));
        i3_queue_event(event);
      }
    }
  }
  else if (strcmp(method, "who_reply") == 0 || strcmp(method, "finger_reply") == 0 ||
           strcmp(method, "locate_reply") == 0)
  {
    event = (i3_event_t *)calloc(1, sizeof(i3_event_t));
    if (event && params_obj)
    {
      if (strcmp(method, "who_reply") == 0)
      {
        event->type = I3_MSG_WHO_REPLY;
      }
      else if (strcmp(method, "finger_reply") == 0)
      {
        event->type = I3_MSG_FINGER_REPLY;
      }
      else
      {
        event->type = I3_MSG_LOCATE_REPLY;
      }

      from_mud_obj = NULL;
      to_user_obj = NULL;
      json_object_object_get_ex(params_obj, "from_mud", &from_mud_obj);
      json_object_object_get_ex(params_obj, "to_user", &to_user_obj);
      if (from_mud_obj)
      {
        strlcpy(event->from_mud, json_object_get_string(from_mud_obj), sizeof(event->from_mud));
      }
      if (to_user_obj)
      {
        strlcpy(event->to_user, json_object_get_string(to_user_obj), sizeof(event->to_user));
      }
      event->data = json_object_get(params_obj);
      i3_queue_event(event);
    }
    else
    {
      free(event);
    }
  }
  else if (strcmp(method, "error_occurred") == 0)
  {
    event = (i3_event_t *)calloc(1, sizeof(i3_event_t));
    if (event)
    {
      event->type = I3_MSG_ERROR;
      if (params_obj && (json_object_object_get_ex(params_obj, "error_message", &message_obj) ||
                         json_object_object_get_ex(params_obj, "message", &message_obj)))
      {
        strlcpy(event->message, json_object_get_string(message_obj), sizeof(event->message));
      }
      else
      {
        strlcpy(event->message, "Unknown I3 gateway error", sizeof(event->message));
      }
      if (params_obj)
      {
        to_user_obj = NULL;
        if (json_object_object_get_ex(params_obj, "to_user", &to_user_obj))
        {
          strlcpy(event->to_user, json_object_get_string(to_user_obj), sizeof(event->to_user));
        }
        event->data = json_object_get(params_obj);
      }
      i3_queue_event(event);
    }
  }

  json_object_put(root);
  return 0;
}

/* Accumulate newline-delimited JSON across arbitrary TCP recv() boundaries. */
int i3_process_input(const char *data, size_t length)
{
  char *new_buffer;
  char *newline;
  size_t needed;
  size_t capacity;
  size_t consumed;
  size_t line_length;
  int processed;

  if (!i3_client || (!data && length > 0))
  {
    return -1;
  }

  if (length == 0)
  {
    return 0;
  }

  if (length > I3_MAX_RECEIVE_LENGTH - i3_client->receive_length)
  {
    i3_error("Gateway response exceeded %d bytes", I3_MAX_RECEIVE_LENGTH);
    i3_client->receive_length = 0;
    return -1;
  }

  needed = i3_client->receive_length + length;
  if (needed > i3_client->receive_capacity)
  {
    capacity = i3_client->receive_capacity ? i3_client->receive_capacity : 8192;
    while (capacity < needed)
    {
      if (capacity >= I3_MAX_RECEIVE_LENGTH / 2)
      {
        capacity = I3_MAX_RECEIVE_LENGTH;
        break;
      }
      capacity *= 2;
    }

    new_buffer = (char *)realloc(i3_client->receive_buffer, capacity + 1);
    if (!new_buffer)
    {
      i3_error("Unable to allocate I3 receive buffer");
      return -1;
    }
    i3_client->receive_buffer = new_buffer;
    i3_client->receive_capacity = capacity;
  }

  memcpy(i3_client->receive_buffer + i3_client->receive_length, data, length);
  i3_client->receive_length += length;
  i3_client->receive_buffer[i3_client->receive_length] = '\0';

  consumed = 0;
  processed = 0;
  while ((newline = memchr(i3_client->receive_buffer + consumed, '\n',
                           i3_client->receive_length - consumed)) != NULL)
  {
    line_length = (size_t)(newline - (i3_client->receive_buffer + consumed));
    while (line_length > 0 && i3_client->receive_buffer[consumed + line_length - 1] == '\r')
    {
      line_length--;
    }
    i3_client->receive_buffer[consumed + line_length] = '\0';

    if (line_length > 0)
    {
      i3_parse_response(i3_client->receive_buffer + consumed);
      processed++;
    }
    consumed = (size_t)(newline - i3_client->receive_buffer) + 1;
  }

  if (consumed > 0)
  {
    i3_client->receive_length -= consumed;
    memmove(i3_client->receive_buffer, i3_client->receive_buffer + consumed,
            i3_client->receive_length);
    i3_client->receive_buffer[i3_client->receive_length] = '\0';
  }

  if (i3_client->receive_length == I3_MAX_RECEIVE_LENGTH)
  {
    i3_error("Gateway sent an unterminated oversized response");
    i3_client->receive_length = 0;
    return -1;
  }

  return processed;
}

/* Queue command for sending */
void i3_queue_command(i3_command_t *cmd)
{
  pthread_mutex_t *mutex_ptr;

  mutex_ptr = (pthread_mutex_t *)i3_client->command_mutex;
  pthread_mutex_lock(mutex_ptr);

  if (i3_client->command_queue_size >= i3_client->max_queue_size)
  {
    pthread_mutex_unlock(mutex_ptr);
    if (cmd->params)
    {
      json_object_put((json_object *)cmd->params);
    }
    free(cmd);
    return;
  }

  cmd->next = NULL;
  if (i3_client->command_queue_tail)
  {
    i3_client->command_queue_tail->next = cmd;
  }
  else
  {
    i3_client->command_queue_head = cmd;
  }
  i3_client->command_queue_tail = cmd;
  i3_client->command_queue_size++;

  pthread_mutex_unlock(mutex_ptr);
}

/* Queue event for processing - currently unused but kept for future implementation */
static void i3_queue_event(i3_event_t *event)
{
  pthread_mutex_t *mutex_ptr;
  unsigned char signal_byte;

  mutex_ptr = (pthread_mutex_t *)i3_client->event_mutex;
  pthread_mutex_lock(mutex_ptr);

  if (i3_client->event_queue_size >= i3_client->max_queue_size)
  {
    pthread_mutex_unlock(mutex_ptr);
    i3_free_event(event);
    return;
  }

  event->next = NULL;
  if (i3_client->event_queue_tail)
  {
    i3_client->event_queue_tail->next = event;
  }
  else
  {
    i3_client->event_queue_head = event;
  }
  i3_client->event_queue_tail = event;
  i3_client->event_queue_size++;

  pthread_mutex_unlock(mutex_ptr);

  if (i3_client->event_signal_write_fd >= 0)
  {
    signal_byte = 1;
    if (write(i3_client->event_signal_write_fd, &signal_byte, sizeof(signal_byte)) < 0 &&
        errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
    {
      i3_error("Unable to signal the main loop for an I3 event: %s", strerror(errno));
    }
  }
}

/* Pop command from queue */
static i3_command_t *i3_pop_command(void)
{
  i3_command_t *cmd;
  pthread_mutex_t *mutex_ptr;

  mutex_ptr = (pthread_mutex_t *)i3_client->command_mutex;
  pthread_mutex_lock(mutex_ptr);

  cmd = i3_client->command_queue_head;
  if (cmd)
  {
    i3_client->command_queue_head = cmd->next;
    if (!i3_client->command_queue_head)
    {
      i3_client->command_queue_tail = NULL;
    }
    i3_client->command_queue_size--;
  }

  pthread_mutex_unlock(mutex_ptr);
  return cmd;
}

/* Pop event from queue */
i3_event_t *i3_pop_event(void)
{
  i3_event_t *event;
  pthread_mutex_t *mutex_ptr;

  mutex_ptr = (pthread_mutex_t *)i3_client->event_mutex;
  pthread_mutex_lock(mutex_ptr);

  event = i3_client->event_queue_head;
  if (event)
  {
    i3_client->event_queue_head = event->next;
    if (!i3_client->event_queue_head)
    {
      i3_client->event_queue_tail = NULL;
    }
    i3_client->event_queue_size--;
  }

  pthread_mutex_unlock(mutex_ptr);
  return event;
}

/* Free command structure */
static void i3_free_command(i3_command_t *cmd)
{
  if (cmd->params)
  {
    json_object_put((json_object *)cmd->params);
  }
  free(cmd);
}

/* Free event structure */
void i3_free_event(i3_event_t *event)
{
  if (event->data)
  {
    json_object_put((json_object *)event->data);
  }
  free(event);
}

/* Create JSON-RPC request */
void *i3_create_request(const char *method, void *params)
{
  json_object *request;

  request = json_object_new_object();
  json_object_object_add(request, "jsonrpc", json_object_new_string("2.0"));
  json_object_object_add(request, "method", json_object_new_string(method));
  json_object_object_add(request, "id", json_object_new_int(i3_client->next_request_id++));

  if (params)
  {
    json_object_object_add(request, "params", (json_object *)params);
  }

  return request;
}

/* Send JSON object over socket */
int i3_send_json(void *obj)
{
  const char *json_str;
  const char *method;
  char *buffer;
  json_object *method_obj;
  fd_set write_set;
  struct timeval timeout;
  size_t json_length;
  size_t total_length;
  size_t offset;
  ssize_t sent;
  int result;
  int send_flags;

  if (!obj || !i3_client || i3_client->socket_fd < 0)
  {
    return -1;
  }

  json_str = json_object_to_json_string_ext((json_object *)obj, JSON_C_TO_STRING_PLAIN);
  json_length = strlen(json_str);
  if (json_length >= I3_MAX_RECEIVE_LENGTH)
  {
    i3_error("Refusing to send oversized JSON request");
    return -1;
  }

  total_length = json_length + 1;
  buffer = (char *)malloc(total_length);
  if (!buffer)
  {
    i3_error("Unable to allocate I3 send buffer");
    return -1;
  }
  memcpy(buffer, json_str, json_length);
  buffer[json_length] = '\n';

  method = "response";
  method_obj = NULL;
  if (json_object_object_get_ex((json_object *)obj, "method", &method_obj))
  {
    method = json_object_get_string(method_obj);
  }
  i3_log("DEBUG: Sending %zu-byte JSON-RPC %s", total_length, method ? method : "request");

  send_flags = 0;
#ifdef MSG_NOSIGNAL
  send_flags = MSG_NOSIGNAL;
#endif

  offset = 0;
  while (offset < total_length)
  {
    sent = send(i3_client->socket_fd, buffer + offset, total_length - offset, send_flags);
    if (sent > 0)
    {
      offset += (size_t)sent;
      continue;
    }
    if (sent < 0 && errno == EINTR)
    {
      continue;
    }
    if (sent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
    {
      FD_ZERO(&write_set);
      FD_SET(i3_client->socket_fd, &write_set);
      timeout.tv_sec = 5;
      timeout.tv_usec = 0;
      result = select(i3_client->socket_fd + 1, NULL, &write_set, NULL, &timeout);
      if (result > 0)
      {
        continue;
      }
      if (result < 0 && errno == EINTR)
      {
        continue;
      }
      i3_error("Timed out sending JSON to I3 gateway");
    }
    else
    {
      i3_error("Failed to send JSON: %s", sent == 0 ? "connection closed" : strerror(errno));
    }
    free(buffer);
    return -1;
  }

  free(buffer);
  i3_log("DEBUG: Successfully sent %zu bytes", total_length);
  i3_client->messages_sent++;
  return 0;
}

/* Send tell message */
int i3_send_tell(const char *from_user, const char *target_mud, const char *target_user,
                 const char *message)
{
  json_object *params;
  i3_command_t *cmd;
  /* i3_validation_result_t validation; */
  /* i3_security_context_t *sec_ctx; */
  void *sec_ctx;
  /* char sanitized_message[I3_MAX_MESSAGE_LENGTH]; */

  if (!i3_client)
  {
    return -1;
  }

  if (!from_user || !target_mud || !target_user || !message)
  {
    i3_log("I3: Invalid tell parameters");
    return -1;
  }

  if (!i3_client->enable_tell)
  {
    /* I3_WARNING(I3_ERROR_PERMISSION, -1, "Tell functionality disabled"); */
    i3_log("I3: Tell functionality disabled");
    return -1;
  }

  /* Validate inputs */
  /* validation = i3_validate_username(from_user); */
  /* if (!validation.valid) {
        I3_ERROR_WITH_INFO(I3_ERROR_VALIDATION, validation.error_code,
                          "Invalid from_user in tell", validation.error_message);
        return -1;
    } */

  /* validation = i3_validate_mudname(target_mud); */
  /* if (!validation.valid) {
        I3_ERROR_WITH_INFO(I3_ERROR_VALIDATION, validation.error_code,
                          "Invalid target_mud in tell", validation.error_message);
        return -1;
    } */

  /* validation = i3_validate_username(target_user); */
  /* if (!validation.valid) {
        I3_ERROR_WITH_INFO(I3_ERROR_VALIDATION, validation.error_code,
                          "Invalid target_user in tell", validation.error_message);
        return -1;
    } */

  /* validation = i3_validate_message(message); */
  /* if (!validation.valid) {
        I3_ERROR_WITH_INFO(I3_ERROR_VALIDATION, validation.error_code,
                          "Invalid message in tell", validation.error_message);
        return -1;
    } */

  /* Check rate limits */
  /* sec_ctx = i3_create_security_context(from_user, "127.0.0.1"); */ /* TODO: Get real IP */
  sec_ctx = NULL;
  /* if (sec_ctx && !i3_check_rate_limit(sec_ctx, "tell")) {
        i3_log_rate_limit_violation(sec_ctx, "tell");
        i3_destroy_security_context(sec_ctx);
        return -1;
    } */

  /* Sanitize message */
  /* i3_sanitize_message(message, sanitized_message, sizeof(sanitized_message)); */

  params = json_object_new_object();
  json_object_object_add(params, "from_user", json_object_new_string(from_user));
  json_object_object_add(params, "target_mud", json_object_new_string(target_mud));
  json_object_object_add(params, "target_user", json_object_new_string(target_user));
  json_object_object_add(params, "message", json_object_new_string(message));

  cmd = (i3_command_t *)calloc(1, sizeof(i3_command_t));
  if (!cmd)
  {
    /* I3_ERROR(I3_ERROR_MEMORY, ENOMEM, "Failed to allocate tell command"); */
    i3_log("I3: Failed to allocate tell command");
    json_object_put(params);
    /* if (sec_ctx) i3_destroy_security_context(sec_ctx); */
    return -1;
  }

  cmd->id = 0;
  strncpy(cmd->method, "tell", sizeof(cmd->method) - 1);
  cmd->method[sizeof(cmd->method) - 1] = '\0';
  cmd->params = params;

  /* Update rate limits */
  if (sec_ctx)
  {
    /* i3_update_rate_limit(sec_ctx, "tell");
        i3_destroy_security_context(sec_ctx); */
  }

  i3_queue_command(cmd);
  return 0;
}

/* Send channel message */
int i3_send_channel_message(const char *channel, const char *from_user, const char *message)
{
  json_object *params;
  i3_command_t *cmd;
  /* i3_validation_result_t validation; */
  /* i3_security_context_t *sec_ctx; */
  void *sec_ctx;
  /* char sanitized_message[I3_MAX_MESSAGE_LENGTH]; */

  if (!i3_client)
  {
    return -1;
  }

  if (!channel || !from_user || !message)
  {
    i3_log("I3: Invalid channel message parameters");
    return -1;
  }

  if (!i3_client->enable_channels)
  {
    /* I3_WARNING(I3_ERROR_PERMISSION, -1, "Channel functionality disabled"); */
    i3_log("I3: Channel functionality disabled");
    return -1;
  }

  /* Validate inputs */
  /* validation = i3_validate_channel(channel); */
  /* if (!validation.valid) {
        I3_ERROR_WITH_INFO(I3_ERROR_VALIDATION, validation.error_code,
                          "Invalid channel in channel message", validation.error_message);
        return -1;
    } */

  /* validation = i3_validate_username(from_user); */
  /* if (!validation.valid) {
        I3_ERROR_WITH_INFO(I3_ERROR_VALIDATION, validation.error_code,
                          "Invalid from_user in channel message", validation.error_message);
        return -1;
    } */

  /* validation = i3_validate_message(message); */
  /* if (!validation.valid) {
        I3_ERROR_WITH_INFO(I3_ERROR_VALIDATION, validation.error_code,
                           "Invalid message in channel message", validation.error_message);
        i3_log("I3: Invalid message in channel message");
        return -1;
    } */

  /* Check rate limits */
  /* sec_ctx = i3_create_security_context(from_user, "127.0.0.1"); */ /* TODO: Get real IP */
  sec_ctx = NULL;
  /* if (sec_ctx && !i3_check_rate_limit(sec_ctx, "channel")) {
        i3_log_rate_limit_violation(sec_ctx, "channel");
        i3_destroy_security_context(sec_ctx);
        return -1;
    } */

  /* Sanitize message */
  /* i3_sanitize_message(message, sanitized_message, sizeof(sanitized_message)); */

  params = json_object_new_object();
  json_object_object_add(params, "channel", json_object_new_string(channel));
  json_object_object_add(params, "from_user", json_object_new_string(from_user));
  json_object_object_add(params, "message", json_object_new_string(message));

  cmd = (i3_command_t *)calloc(1, sizeof(i3_command_t));
  if (!cmd)
  {
    /* I3_ERROR(I3_ERROR_MEMORY, ENOMEM, "Failed to allocate channel command"); */
    i3_log("I3: Failed to allocate channel command");
    json_object_put(params);
    /* if (sec_ctx) i3_destroy_security_context(sec_ctx); */
    return -1;
  }

  cmd->id = 0;
  strncpy(cmd->method, "channel_send", sizeof(cmd->method) - 1);
  cmd->method[sizeof(cmd->method) - 1] = '\0';
  cmd->params = params;

  /* Update rate limits */
  if (sec_ctx)
  {
    /* i3_update_rate_limit(sec_ctx, "channel");
        i3_destroy_security_context(sec_ctx); */
  }

  i3_queue_command(cmd);
  return 0;
}

static void i3_copy_json_string(json_object *object, const char *key, char *destination,
                                size_t destination_size)
{
  json_object *value;
  const char *text;

  value = NULL;
  if (!object || !json_object_object_get_ex(object, key, &value))
  {
    return;
  }

  text = json_object_get_string(value);
  if (text)
  {
    strlcpy(destination, text, destination_size);
  }
}

/* Replace the main-thread MUD cache with a complete gateway snapshot. */
static void i3_update_mudlist(json_object *result_obj)
{
  json_object *muds_obj;
  json_object *mud_obj;
  json_object *value_obj;
  json_object *services_obj;
  i3_mud_t *new_head;
  i3_mud_t *new_tail;
  i3_mud_t *mud;
  i3_mud_t *old_mud;
  i3_mud_t *next_mud;
  const char *status;
  size_t mud_count;
  size_t index;
  size_t used;
  int allocation_failed;

  muds_obj = NULL;
  if (!result_obj || !json_object_object_get_ex(result_obj, "muds", &muds_obj) ||
      !json_object_is_type(muds_obj, json_type_array))
  {
    i3_error("MUD list response did not contain a valid array");
    return;
  }

  new_head = NULL;
  new_tail = NULL;
  allocation_failed = 0;
  mud_count = json_object_array_length(muds_obj);

  for (index = 0; index < mud_count; index++)
  {
    mud_obj = json_object_array_get_idx(muds_obj, index);
    if (!mud_obj || !json_object_is_type(mud_obj, json_type_object))
    {
      continue;
    }

    value_obj = NULL;
    if (!json_object_object_get_ex(mud_obj, "name", &value_obj) ||
        !json_object_get_string(value_obj) || !*json_object_get_string(value_obj))
    {
      continue;
    }

    mud = (i3_mud_t *)calloc(1, sizeof(i3_mud_t));
    if (!mud)
    {
      allocation_failed = 1;
      break;
    }

    i3_copy_json_string(mud_obj, "name", mud->name, sizeof(mud->name));
    i3_copy_json_string(mud_obj, "driver", mud->driver, sizeof(mud->driver));
    i3_copy_json_string(mud_obj, "mud_type", mud->mud_type, sizeof(mud->mud_type));
    i3_copy_json_string(mud_obj, "admin_email", mud->admin_email, sizeof(mud->admin_email));

    value_obj = NULL;
    if (json_object_object_get_ex(mud_obj, "port", &value_obj))
    {
      mud->port = json_object_get_int(value_obj);
    }

    status = NULL;
    value_obj = NULL;
    if (json_object_object_get_ex(mud_obj, "status", &value_obj))
    {
      status = json_object_get_string(value_obj);
    }
    mud->online = status && (!strcasecmp(status, "up") || !strcasecmp(status, "online"));

    services_obj = NULL;
    if (json_object_object_get_ex(mud_obj, "services", &services_obj) &&
        json_object_is_type(services_obj, json_type_object))
    {
      json_object_object_foreach(services_obj, service_name, service_value)
      {
        UNUSED_VAR(service_value);
        used = strlen(mud->services);
        if (used > 0 && used < sizeof(mud->services) - 1)
        {
          strlcat(mud->services, ",", sizeof(mud->services));
        }
        strlcat(mud->services, service_name, sizeof(mud->services));
      }
    }

    if (new_tail)
    {
      new_tail->next = mud;
    }
    else
    {
      new_head = mud;
    }
    new_tail = mud;
  }

  if (allocation_failed)
  {
    while (new_head)
    {
      next_mud = new_head->next;
      free(new_head);
      new_head = next_mud;
    }
    i3_error("Unable to allocate the I3 MUD list");
    return;
  }

  old_mud = i3_client->mud_list;
  i3_client->mud_list = new_head;
  i3_client->mud_list_updated = time(NULL);
  while (old_mud)
  {
    next_mud = old_mud->next;
    free(old_mud);
    old_mud = next_mud;
  }

  i3_log("Updated local MUD list with %zu entries", mud_count);
}

/* Replace the main-thread channel cache with a complete gateway snapshot. */
static void i3_update_channel_list(json_object *result_obj)
{
  json_object *channels_obj;
  json_object *subscribed_obj;
  json_object *channel_obj;
  json_object *value_obj;
  json_object *subscribed_value;
  i3_channel_t *new_channels;
  const char *channel_name;
  const char *subscribed_name;
  size_t channel_total;
  size_t channel_index;
  size_t subscribed_total;
  size_t subscribed_index;
  int channel_count;

  channels_obj = NULL;
  if (!result_obj || !json_object_object_get_ex(result_obj, "channels", &channels_obj) ||
      !json_object_is_type(channels_obj, json_type_array))
  {
    i3_error("Channel list response did not contain a valid array");
    return;
  }

  new_channels = (i3_channel_t *)calloc(I3_MAX_CHANNELS, sizeof(i3_channel_t));
  if (!new_channels)
  {
    i3_error("Unable to allocate the I3 channel list");
    return;
  }

  subscribed_obj = NULL;
  json_object_object_get_ex(result_obj, "subscribed_channels", &subscribed_obj);
  subscribed_total = subscribed_obj && json_object_is_type(subscribed_obj, json_type_array)
                         ? json_object_array_length(subscribed_obj)
                         : 0;

  channel_count = 0;
  channel_total = json_object_array_length(channels_obj);
  for (channel_index = 0; channel_index < channel_total && channel_count < I3_MAX_CHANNELS;
       channel_index++)
  {
    channel_obj = json_object_array_get_idx(channels_obj, channel_index);
    if (!channel_obj || !json_object_is_type(channel_obj, json_type_object))
    {
      continue;
    }

    value_obj = NULL;
    if (!json_object_object_get_ex(channel_obj, "name", &value_obj))
    {
      continue;
    }
    channel_name = json_object_get_string(value_obj);
    if (!channel_name || !*channel_name)
    {
      continue;
    }

    strlcpy(new_channels[channel_count].name, channel_name,
            sizeof(new_channels[channel_count].name));
    i3_copy_json_string(channel_obj, "owner", new_channels[channel_count].owner,
                        sizeof(new_channels[channel_count].owner));

    value_obj = NULL;
    if (json_object_object_get_ex(channel_obj, "type", &value_obj))
    {
      new_channels[channel_count].type = json_object_get_int(value_obj);
    }
    value_obj = NULL;
    if (json_object_object_get_ex(channel_obj, "member_count", &value_obj))
    {
      new_channels[channel_count].member_count = json_object_get_int(value_obj);
    }

    for (subscribed_index = 0; subscribed_index < subscribed_total; subscribed_index++)
    {
      subscribed_value = json_object_array_get_idx(subscribed_obj, subscribed_index);
      subscribed_name = json_object_get_string(subscribed_value);
      if (subscribed_name && !strcasecmp(channel_name, subscribed_name))
      {
        new_channels[channel_count].subscribed = 1;
        break;
      }
    }
    channel_count++;
  }

  memcpy(i3_client->channels, new_channels, sizeof(i3_client->channels));
  i3_client->channel_count = channel_count;
  free(new_channels);

  i3_log("Updated local channel list with %d entries", channel_count);
}

static struct char_data *i3_reply_recipient(i3_event_t *event)
{
  if (!event->to_user[0] || !strcmp(event->to_user, "*"))
  {
    return NULL;
  }
  return i3_find_online_player(event->to_user);
}

static struct char_data *i3_find_online_player(const char *name)
{
  struct char_data *candidate;

  if (!name || !*name)
  {
    return NULL;
  }

  for (candidate = character_list; candidate; candidate = candidate->next)
  {
    if (!IS_NPC(candidate) && GET_NAME(candidate) && !str_cmp(GET_NAME(candidate), name))
    {
      return candidate;
    }
  }

  return NULL;
}

#ifdef LUMINARI_CUTEST
struct char_data *i3_find_online_player_for_test(const char *name)
{
  return i3_find_online_player(name);
}
#endif

static void i3_deliver_who_reply(i3_event_t *event)
{
  struct char_data *victim;
  json_object *params_obj;
  json_object *users_obj;
  json_object *user_obj;
  json_object *value_obj;
  const char *name;
  const char *extra;
  size_t user_count;
  size_t index;
  int idle;
  int level;

  victim = i3_reply_recipient(event);
  params_obj = (json_object *)event->data;
  users_obj = NULL;
  if (!victim || !params_obj || !json_object_object_get_ex(params_obj, "users", &users_obj) ||
      !json_object_is_type(users_obj, json_type_array))
  {
    i3_log("Who reply from %s had no online recipient", event->from_mud);
    return;
  }

  user_count = json_object_array_length(users_obj);
  send_to_char(victim, "%sI3 who list for %s (%zu player%s):%s\r\n", CCYEL(victim, C_NRM),
               event->from_mud, user_count, user_count == 1 ? "" : "s", CCNRM(victim, C_NRM));
  for (index = 0; index < user_count; index++)
  {
    user_obj = json_object_array_get_idx(users_obj, index);
    if (!user_obj || !json_object_is_type(user_obj, json_type_object))
    {
      continue;
    }

    name = "Unknown";
    extra = "";
    idle = 0;
    level = 0;
    value_obj = NULL;
    if (json_object_object_get_ex(user_obj, "name", &value_obj))
    {
      name = json_object_get_string(value_obj);
    }
    value_obj = NULL;
    if (json_object_object_get_ex(user_obj, "extra", &value_obj))
    {
      extra = json_object_get_string(value_obj);
    }
    value_obj = NULL;
    if (json_object_object_get_ex(user_obj, "idle", &value_obj))
    {
      idle = json_object_get_int(value_obj);
    }
    value_obj = NULL;
    if (json_object_object_get_ex(user_obj, "level", &value_obj))
    {
      level = json_object_get_int(value_obj);
    }
    send_to_char(victim, "  %-20s level %-4d idle %-6d %s\r\n", name ? name : "Unknown", level,
                 idle, extra ? extra : "");
  }
}

static void i3_deliver_finger_reply(i3_event_t *event)
{
  struct char_data *victim;
  json_object *params_obj;
  json_object *info_obj;
  json_object *value_obj;
  const char *name;
  const char *title;
  const char *real_name;
  const char *email;
  int level;
  int idle;

  victim = i3_reply_recipient(event);
  params_obj = (json_object *)event->data;
  info_obj = NULL;
  if (!victim || !params_obj || !json_object_object_get_ex(params_obj, "user_info", &info_obj) ||
      !json_object_is_type(info_obj, json_type_object))
  {
    i3_log("Finger reply from %s had no online recipient", event->from_mud);
    return;
  }

  if (json_object_object_length(info_obj) == 0)
  {
    send_to_char(victim, "No finger information was returned by %s.\r\n", event->from_mud);
    return;
  }

  name = "Unknown";
  title = "";
  real_name = "";
  email = "";
  level = 0;
  idle = 0;
  value_obj = NULL;
  if (json_object_object_get_ex(info_obj, "name", &value_obj))
  {
    name = json_object_get_string(value_obj);
  }
  value_obj = NULL;
  if (json_object_object_get_ex(info_obj, "title", &value_obj))
  {
    title = json_object_get_string(value_obj);
  }
  value_obj = NULL;
  if (json_object_object_get_ex(info_obj, "real_name", &value_obj))
  {
    real_name = json_object_get_string(value_obj);
  }
  value_obj = NULL;
  if (json_object_object_get_ex(info_obj, "email", &value_obj))
  {
    email = json_object_get_string(value_obj);
  }
  value_obj = NULL;
  if (json_object_object_get_ex(info_obj, "level", &value_obj))
  {
    level = json_object_get_int(value_obj);
  }
  value_obj = NULL;
  if (json_object_object_get_ex(info_obj, "idle_time", &value_obj))
  {
    idle = json_object_get_int(value_obj);
  }

  send_to_char(victim, "%sI3 finger for %s@%s:%s\r\n", CCYEL(victim, C_NRM),
               name ? name : "Unknown", event->from_mud, CCNRM(victim, C_NRM));
  send_to_char(victim, "  Title: %s\r\n", title ? title : "");
  send_to_char(victim, "  Real name: %s\r\n", real_name ? real_name : "");
  send_to_char(victim, "  Email: %s\r\n", email ? email : "");
  send_to_char(victim, "  Level: %d  Idle: %d seconds\r\n", level, idle);
}

static void i3_deliver_locate_reply(i3_event_t *event)
{
  struct char_data *victim;
  json_object *params_obj;
  json_object *value_obj;
  const char *located_mud;
  const char *located_user;
  const char *status;
  int idle;

  victim = i3_reply_recipient(event);
  params_obj = (json_object *)event->data;
  if (!victim || !params_obj)
  {
    i3_log("Locate reply from %s had no online recipient", event->from_mud);
    return;
  }

  located_mud = "";
  located_user = "";
  status = "";
  idle = 0;
  value_obj = NULL;
  if (json_object_object_get_ex(params_obj, "located_mud", &value_obj))
  {
    located_mud = json_object_get_string(value_obj);
  }
  value_obj = NULL;
  if (json_object_object_get_ex(params_obj, "located_user", &value_obj))
  {
    located_user = json_object_get_string(value_obj);
  }
  value_obj = NULL;
  if (json_object_object_get_ex(params_obj, "status", &value_obj))
  {
    status = json_object_get_string(value_obj);
  }
  value_obj = NULL;
  if (json_object_object_get_ex(params_obj, "idle", &value_obj))
  {
    idle = json_object_get_int(value_obj);
  }

  if (!located_mud || !*located_mud)
  {
    send_to_char(victim, "No matching I3 user was reported by %s.\r\n", event->from_mud);
    return;
  }

  send_to_char(victim, "%sI3 locate:%s %s is on %s (idle %d seconds)%s%s\r\n", CCYEL(victim, C_NRM),
               CCNRM(victim, C_NRM), located_user && *located_user ? located_user : "User",
               located_mud, idle, status && *status ? " - " : "", status && *status ? status : "");
}

/* Process events from the queue - called from main thread */
void i3_process_events(void)
{
  i3_event_t *event;
  struct char_data *victim;
  unsigned char signal_buffer[64];

  if (!i3_client)
  {
    return;
  }

  if (i3_client->event_signal_read_fd >= 0)
  {
    while (read(i3_client->event_signal_read_fd, signal_buffer, sizeof(signal_buffer)) > 0)
    {
      /* Drain all pending wake bytes before processing the event queue. */
    }
  }

  while ((event = i3_pop_event()) != NULL)
  {
    switch (event->type)
    {
    case I3_MSG_TELL:
    case I3_MSG_EMOTETO:
      /* Find the target player and deliver the tell */
      victim = i3_find_online_player(event->to_user);
      if (victim)
      {
        if (event->type == I3_MSG_TELL)
        {
          send_to_char(victim, "%s[I3 Tell] %s@%s tells you: %s%s\r\n", CCYEL(victim, C_NRM),
                       event->from_user, event->from_mud, event->message, CCNRM(victim, C_NRM));
        }
        else
        {
          send_to_char(victim, "%s[I3 Emote] %s@%s %s%s\r\n", CCYEL(victim, C_NRM),
                       event->from_user, event->from_mud, event->message, CCNRM(victim, C_NRM));
        }
        i3_log("Delivered direct message from %s@%s to %s", event->from_user, event->from_mud,
               event->to_user);
      }
      else
      {
        i3_log("Direct message from %s@%s to %s: player not found", event->from_user,
               event->from_mud, event->to_user);
      }
      break;

    case I3_MSG_CHANNEL:
    case I3_MSG_CHANNEL_EMOTE:
      /* Broadcast channel message to all online players */
      /* TODO: Implement channel subscription system */
      {
        struct descriptor_data *d;
        for (d = descriptor_list; d; d = d->next)
        {
          if (STATE(d) == CON_PLAYING && d->character)
          {
            if (event->type == I3_MSG_CHANNEL)
            {
              send_to_char(d->character, "%s[I3:%s] %s@%s: %s%s\r\n", CCYEL(d->character, C_NRM),
                           event->channel, event->from_user, event->from_mud, event->message,
                           CCNRM(d->character, C_NRM));
            }
            else
            {
              send_to_char(d->character, "%s[I3:%s] * %s@%s %s%s\r\n", CCYEL(d->character, C_NRM),
                           event->channel, event->from_user, event->from_mud, event->message,
                           CCNRM(d->character, C_NRM));
            }
          }
        }
        i3_log("Broadcast channel message from %s@%s on %s", event->from_user, event->from_mud,
               event->channel);
      }
      break;

    case I3_MSG_WHO_REPLY:
      i3_deliver_who_reply(event);
      break;

    case I3_MSG_FINGER_REPLY:
      i3_deliver_finger_reply(event);
      break;

    case I3_MSG_LOCATE_REPLY:
      i3_deliver_locate_reply(event);
      break;

    case I3_MSG_MUDLIST_REPLY:
      i3_update_mudlist((json_object *)event->data);
      break;

    case I3_MSG_CHANNEL_LIST_REPLY:
      i3_update_channel_list((json_object *)event->data);
      break;

    case I3_MSG_CHANNEL_JOIN:
    case I3_MSG_CHANNEL_LEAVE:
    {
      i3_channel_t *channel;

      channel = i3_find_channel(event->channel);
      if (channel)
      {
        channel->subscribed = event->type == I3_MSG_CHANNEL_JOIN;
      }
      i3_log("%s channel %s", event->type == I3_MSG_CHANNEL_JOIN ? "Joined" : "Left",
             event->channel);
    }
    break;

    case I3_MSG_ERROR:
      /* Log errors */
      i3_error("I3 Error: %s", event->message);
      victim = i3_reply_recipient(event);
      if (victim)
      {
        send_to_char(victim, "Intermud3 error: %s\r\n", event->message);
      }
      break;

    default:
      i3_log("DEBUG: Unknown event type: %d", event->type);
      break;
    }

    i3_free_event(event);
  }
}

int i3_get_event_fd(void)
{
  if (!i3_client)
  {
    return -1;
  }

  return i3_client->event_signal_read_fd;
}

/* Logging functions */
void i3_log(const char *format, ...)
{
  va_list args;
  char buf[2048];

  va_start(args, format);
  vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);

  /* Use MUD's logging system */
  log("I3: %s", buf);
}

void i3_error(const char *format, ...)
{
  va_list args;
  char buf[2048];

  va_start(args, format);
  vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);

  i3_log("ERROR: %s", buf);
  if (i3_client)
  {
    i3_client->errors++;
  }
}

void i3_debug(const char *format, ...)
{
  va_list args;
  char buf[2048];

  va_start(args, format);
  vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);

  i3_log("DEBUG: %s", buf);
}

/* Get state string */
const char *i3_get_state_string(void)
{
  if (!i3_client)
  {
    return "Not initialized";
  }

  switch (i3_client->state)
  {
  case I3_STATE_DISCONNECTED:
    return "Disconnected";
  case I3_STATE_CONNECTING:
    return "Connecting";
  case I3_STATE_AUTHENTICATING:
    return "Authenticating";
  case I3_STATE_CONNECTED:
    return "Connected";
  case I3_STATE_RECONNECTING:
    return "Reconnecting";
  case I3_STATE_SHUTDOWN:
    return "Shutting down";
  default:
    return "Unknown";
  }
}

/* Load configuration from file */
int i3_load_config(const char *filename)
{
  FILE *fp;
  char line[256];
  char key[128], value[256];
  char *p;
  char *url_host;
  int len;

  fp = fopen(filename, "r");
  if (!fp)
  {
    i3_error("DEBUG: Failed to open config file: %s (errno: %d - %s)", filename, errno,
             strerror(errno));
    return -1;
  }
  i3_log("DEBUG: Successfully opened config file: %s", filename);

  while (fgets(line, sizeof(line), fp))
  {
    /* Skip comments and empty lines */
    if (line[0] == '#' || line[0] == '\n')
    {
      continue;
    }

    /* Remove trailing newline and carriage return */
    len = strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
    {
      line[--len] = '\0';
    }

    /* Try to find '=' first (new format), then space (old format) */
    p = strchr(line, '=');
    if (!p)
    {
      p = strchr(line, ' ');
    }

    if (p)
    {
      *p = '\0';
      strlcpy(key, line, sizeof(key));
      strlcpy(value, p + 1, sizeof(value));

      /* Handle new I3_GATEWAY_URL format (extract host from URL) */
      if (strcmp(key, "I3_GATEWAY_URL") == 0)
      {
        i3_log("DEBUG: Loading gateway URL from config: %s", value);
        /* Extract host from URL like wss://host or ws://host */
        url_host = value;
        if (strncmp(url_host, "wss://", 6) == 0)
        {
          url_host += 6;
        }
        else if (strncmp(url_host, "ws://", 5) == 0)
        {
          url_host += 5;
        }
        strlcpy(i3_client->gateway_host, url_host, sizeof(i3_client->gateway_host));
        i3_log("DEBUG: Extracted gateway host: %s", i3_client->gateway_host);
      }
      /* Handle new I3_API_KEY format */
      else if (strcmp(key, "I3_API_KEY") == 0)
      {
        strlcpy(i3_client->api_key, value, sizeof(i3_client->api_key));
      }
      /* Handle new I3_MUD_NAME format */
      else if (strcmp(key, "I3_MUD_NAME") == 0)
      {
        i3_log("DEBUG: Loading MUD name from config (I3_MUD_NAME): %s", value);
        strlcpy(i3_client->mud_name, value, sizeof(i3_client->mud_name));
      }
      /* Handle old gateway_host format */
      else if (strcmp(key, "gateway_host") == 0)
      {
        strlcpy(i3_client->gateway_host, value, sizeof(i3_client->gateway_host));
      }
      else if (strcmp(key, "gateway_port") == 0)
      {
        i3_client->gateway_port = atoi(value);
      }
      /* Handle old api_key format */
      else if (strcmp(key, "api_key") == 0)
      {
        strlcpy(i3_client->api_key, value, sizeof(i3_client->api_key));
      }
      /* Handle old mud_name format */
      else if (strcmp(key, "mud_name") == 0)
      {
        strlcpy(i3_client->mud_name, value, sizeof(i3_client->mud_name));
      }
      else if (strcmp(key, "default_channel") == 0)
      {
        strlcpy(i3_client->default_channel, value, sizeof(i3_client->default_channel));
      }
      else if (strcmp(key, "enable_tell") == 0)
      {
        i3_client->enable_tell = atoi(value);
      }
      else if (strcmp(key, "enable_channels") == 0)
      {
        i3_client->enable_channels = atoi(value);
      }
      else if (strcmp(key, "enable_who") == 0)
      {
        i3_client->enable_who = atoi(value);
      }
    }
  }

  fclose(fp);
  i3_log("DEBUG: Config loaded - Host: %s, Port: %d, MUD: %s", i3_client->gateway_host,
         i3_client->gateway_port, i3_client->mud_name);
  return 0;
}

/* Save configuration to file */
int i3_save_config(const char *filename)
{
  FILE *fp;
  int fd;

  fd = open(filename, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
  if (fd < 0)
  {
    return -1;
  }

  if (fchmod(fd, S_IRUSR | S_IWUSR) < 0)
  {
    close(fd);
    return -1;
  }

  if (ftruncate(fd, 0) < 0)
  {
    close(fd);
    return -1;
  }

  fp = fdopen(fd, "w");
  if (!fp)
  {
    close(fd);
    return -1;
  }

  fprintf(fp, "# Intermud3 Configuration\n");
  fprintf(fp, "gateway_host %s\n", i3_client->gateway_host);
  fprintf(fp, "gateway_port %d\n", i3_client->gateway_port);
  fprintf(fp, "api_key %s\n", i3_client->api_key);
  fprintf(fp, "mud_name %s\n", i3_client->mud_name);
  fprintf(fp, "default_channel %s\n", i3_client->default_channel);
  fprintf(fp, "enable_tell %d\n", i3_client->enable_tell);
  fprintf(fp, "enable_channels %d\n", i3_client->enable_channels);
  fprintf(fp, "enable_who %d\n", i3_client->enable_who);

  fclose(fp);
  return 0;
}
