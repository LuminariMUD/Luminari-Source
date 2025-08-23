/* ************************************************************************
*  Intermud3 Client Implementation for LuminariMUD                       *
*  Core client functionality and connection management                    *
*  Based on CircleMUD/tbaMUD implementation - adapted for C89/ANSI C     *
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

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <stdarg.h>
#include <pthread.h>
#include <json-c/json.h>
#include <fcntl.h>
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
static void i3_handle_message(const char *json_str);
static void i3_queue_command(i3_command_t *cmd);
static void i3_queue_event(i3_event_t *event);
static i3_command_t *i3_pop_command(void);
static void i3_free_command(i3_command_t *cmd);
static void i3_heartbeat(void);
static void i3_reconnect(void);

/* Initialize the I3 client */
int i3_initialize(void)
{
    pthread_t *thread_ptr;
    
    i3_client = (i3_client_t *)calloc(1, sizeof(i3_client_t));
    if (!i3_client) {
        log("ERROR: Failed to allocate I3 client structure");
        return -1;
    }
    
    /* Allocate pthread structures */
    i3_client->thread_id = calloc(1, sizeof(pthread_t));
    i3_client->command_mutex = calloc(1, sizeof(pthread_mutex_t));
    i3_client->event_mutex = calloc(1, sizeof(pthread_mutex_t));
    i3_client->state_mutex = calloc(1, sizeof(pthread_mutex_t));
    
    /* Initialize mutexes */
    pthread_mutex_init((pthread_mutex_t *)i3_client->command_mutex, NULL);
    pthread_mutex_init((pthread_mutex_t *)i3_client->event_mutex, NULL);
    pthread_mutex_init((pthread_mutex_t *)i3_client->state_mutex, NULL);
    
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
    strcpy(i3_client->gateway_host, "localhost");
    i3_client->gateway_port = I3_DEFAULT_PORT;
    strcpy(i3_client->mud_name, "LuminariMUD");
    strcpy(i3_client->default_channel, "intermud");
    
    /* Load configuration - MUD runs from lib directory */
    if (i3_load_config("i3_config") < 0) {
        log("Warning: Could not load I3 configuration, using defaults");
    }
    
    /* Create client thread */
    thread_ptr = (pthread_t *)i3_client->thread_id;
    if (pthread_create(thread_ptr, NULL, i3_client_thread, NULL) != 0) {
        i3_error("Failed to create I3 client thread: %s", strerror(errno));
        free(i3_client->thread_id);
        free(i3_client->command_mutex);
        free(i3_client->event_mutex);
        free(i3_client->state_mutex);
        free(i3_client);
        i3_client = NULL;
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
    while (i3_client->command_queue_head) {
        cmd = i3_pop_command();
        i3_free_command(cmd);
    }
    
    while (i3_client->event_queue_head) {
        event = i3_pop_event();
        i3_free_event(event);
    }
    
    /* Clean up MUD list */
    mud = i3_client->mud_list;
    while (mud) {
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
    
    /* Free client structure */
    free(i3_client);
    i3_client = NULL;
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
    char *line;
    i3_command_t *cmd;
    json_object *request;
    
    UNUSED_VAR(arg);
    
    i3_log("I3 client thread started");
    
    /* Initial connection */
    if (i3_connect() == 0) {
        /* Don't authenticate immediately - wait for welcome message */
        i3_log("DEBUG: Connected/Reconnected, waiting for welcome message");
    }
    
    /* Main loop */
    while (i3_client->state != I3_STATE_SHUTDOWN) {
        /* Check for reconnection */
        if (i3_client->state == I3_STATE_DISCONNECTED && i3_client->auto_reconnect) {
            sleep(i3_client->reconnect_delay);
            i3_reconnect();
            continue;
        }
        
        /* Process outgoing commands */
        cmd = i3_pop_command();
        if (cmd) {
            request = (json_object *)i3_create_request(cmd->method, cmd->params);
            i3_send_json(request);
            json_object_put(request);
            i3_free_command(cmd);
        }
        
        /* Check for incoming data */
        if (i3_client->socket_fd >= 0) {
            FD_ZERO(&read_set);
            FD_SET(i3_client->socket_fd, &read_set);
            timeout.tv_sec = 1;
            timeout.tv_usec = 0;
            
            result = select(i3_client->socket_fd + 1, &read_set, NULL, NULL, &timeout);
            if (result > 0 && FD_ISSET(i3_client->socket_fd, &read_set)) {
                bytes = recv(i3_client->socket_fd, buffer, sizeof(buffer) - 1, 0);
                if (bytes > 0) {
                    buffer[bytes] = '\0';
                    i3_log("DEBUG: Received %d bytes: %.200s%s", 
                           bytes, buffer, (bytes > 200 ? "..." : ""));
                    
                    /* Handle line-delimited JSON */
                    line = strtok(buffer, "\n");
                    while (line) {
                        i3_log("DEBUG: Processing message: %.100s%s", 
                               line, (strlen(line) > 100 ? "..." : ""));
                        i3_handle_message(line);
                        line = strtok(NULL, "\n");
                    }
                } else if (bytes == 0 || (bytes < 0 && errno != EAGAIN)) {
                    i3_error("Connection lost: %s", strerror(errno));
                    i3_disconnect();
                }
            }
        }
        
        /* Send heartbeat */
        now = time(NULL);
        if (i3_client->state == I3_STATE_CONNECTED && 
            now - last_heartbeat >= I3_HEARTBEAT_INTERVAL) {
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
    
    i3_log("Connecting to I3 gateway at %s:%d", 
           i3_client->gateway_host, i3_client->gateway_port);
    i3_log("DEBUG: Using API key: %.30s...", i3_client->api_key);
    
    i3_client->socket_fd = i3_socket_connect(i3_client->gateway_host, 
                                             i3_client->gateway_port);
    if (i3_client->socket_fd < 0) {
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
    
    if (i3_client->socket_fd >= 0) {
        close(i3_client->socket_fd);
        i3_client->socket_fd = -1;
    }
    
    mutex_ptr = (pthread_mutex_t *)i3_client->state_mutex;
    pthread_mutex_lock(mutex_ptr);
    if (i3_client->state != I3_STATE_SHUTDOWN) {
        i3_client->state = I3_STATE_DISCONNECTED;
    }
    pthread_mutex_unlock(mutex_ptr);
    
    i3_client->authenticated = 0;
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
    if (sock < 0) {
        i3_error("Failed to create socket: %s", strerror(errno));
        return -1;
    }
    
    /* Resolve hostname */
    server = gethostbyname(host);
    if (!server) {
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
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
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
    const char *json_str;
    
    i3_log("DEBUG: Starting authentication with API key: %.30s...", i3_client->api_key);
    
    params = json_object_new_object();
    json_object_object_add(params, "api_key", json_object_new_string(i3_client->api_key));
    
    request = (json_object *)i3_create_request("authenticate", params);
    json_str = json_object_to_json_string(request);
    i3_log("DEBUG: Sending auth request: %s", json_str);
    
    result = i3_send_json(request);
    i3_log("DEBUG: Auth send result: %d", result);
    json_object_put(request);
    
    if (result < 0) {
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
    
    if (i3_connect() == 0) {
        /* Don't authenticate immediately - wait for welcome message */
        i3_log("DEBUG: Connected/Reconnected, waiting for welcome message");
    }
}

/* Handle incoming message */
static void i3_handle_message(const char *json_str)
{
    json_object *root, *method_obj, *params_obj;
    const char *method;
    i3_event_t *event;
    pthread_mutex_t *mutex_ptr;
    
    root = json_tokener_parse(json_str);
    if (!root) {
        i3_error("Failed to parse JSON: %s", json_str);
        return;
    }
    
    /* Check for method field */
    if (!json_object_object_get_ex(root, "method", &method_obj)) {
        /* This might be a response to our request */
        json_object *result_obj, *id_obj, *status_obj;
        
        /* Check if this is an authentication response */
        if (json_object_object_get_ex(root, "id", &id_obj) &&
            json_object_object_get_ex(root, "result", &result_obj)) {
            
            if (json_object_object_get_ex(result_obj, "status", &status_obj)) {
                const char *status = json_object_get_string(status_obj);
                if (strcmp(status, "authenticated") == 0) {
                    mutex_ptr = (pthread_mutex_t *)i3_client->state_mutex;
                    pthread_mutex_lock(mutex_ptr);
                    i3_client->state = I3_STATE_CONNECTED;
                    pthread_mutex_unlock(mutex_ptr);
                    i3_client->authenticated = 1;
                    
                    /* Get session info */
                    json_object *mud_name_obj = json_object_object_get(result_obj, "mud_name");
                    json_object *session_obj = json_object_object_get(result_obj, "session_id");
                    
                    i3_log("Successfully authenticated with I3 gateway");
                    if (mud_name_obj) {
                        i3_log("MUD Name confirmed: %s", json_object_get_string(mud_name_obj));
                    }
                    if (session_obj) {
                        i3_log("Session ID: %s", json_object_get_string(session_obj));
                    }
                }
            }
        }
        
        json_object_put(root);
        return;
    }
    
    method = json_object_get_string(method_obj);
    json_object_object_get_ex(root, "params", &params_obj);
    
    /* Handle welcome message - triggers authentication */
    if (strcmp(method, "welcome") == 0) {
        i3_log("DEBUG: Received welcome message from gateway");
        if (params_obj) {
            json_object *service_obj = json_object_object_get(params_obj, "service");
            json_object *version_obj = json_object_object_get(params_obj, "version");
            if (service_obj) {
                i3_log("Gateway service: %s", json_object_get_string(service_obj));
            }
            if (version_obj) {
                i3_log("Gateway version: %s", json_object_get_string(version_obj));
            }
        }
        
        /* Now authenticate */
        i3_authenticate();
    }
    /* Handle incoming tell */
    else if (strcmp(method, "tell") == 0) {
        /* TODO: Implement tell message parsing - DISABLED to prevent heap corruption */
        i3_log("DEBUG: Received tell message (parsing not implemented)");
    }
    /* Handle incoming channel message */
    else if (strcmp(method, "channel_message") == 0) {
        /* TODO: Implement channel message parsing - DISABLED to prevent heap corruption */
        i3_log("DEBUG: Received channel message (parsing not implemented)");
    }
    
    json_object_put(root);
}

/* Queue command for sending */
static void i3_queue_command(i3_command_t *cmd)
{
    pthread_mutex_t *mutex_ptr;
    
    mutex_ptr = (pthread_mutex_t *)i3_client->command_mutex;
    pthread_mutex_lock(mutex_ptr);
    
    if (i3_client->command_queue_size >= i3_client->max_queue_size) {
        pthread_mutex_unlock(mutex_ptr);
        if (cmd->params) {
            json_object_put((json_object *)cmd->params);
        }
        free(cmd);
        return;
    }
    
    cmd->next = NULL;
    if (i3_client->command_queue_tail) {
        i3_client->command_queue_tail->next = cmd;
    } else {
        i3_client->command_queue_head = cmd;
    }
    i3_client->command_queue_tail = cmd;
    i3_client->command_queue_size++;
    
    pthread_mutex_unlock(mutex_ptr);
}

/* Queue event for processing */
static void i3_queue_event(i3_event_t *event)
{
    pthread_mutex_t *mutex_ptr;
    
    mutex_ptr = (pthread_mutex_t *)i3_client->event_mutex;
    pthread_mutex_lock(mutex_ptr);
    
    if (i3_client->event_queue_size >= i3_client->max_queue_size) {
        pthread_mutex_unlock(mutex_ptr);
        i3_free_event(event);
        return;
    }
    
    event->next = NULL;
    if (i3_client->event_queue_tail) {
        i3_client->event_queue_tail->next = event;
    } else {
        i3_client->event_queue_head = event;
    }
    i3_client->event_queue_tail = event;
    i3_client->event_queue_size++;
    
    pthread_mutex_unlock(mutex_ptr);
}

/* Pop command from queue */
static i3_command_t *i3_pop_command(void)
{
    i3_command_t *cmd;
    pthread_mutex_t *mutex_ptr;
    
    mutex_ptr = (pthread_mutex_t *)i3_client->command_mutex;
    pthread_mutex_lock(mutex_ptr);
    
    cmd = i3_client->command_queue_head;
    if (cmd) {
        i3_client->command_queue_head = cmd->next;
        if (!i3_client->command_queue_head) {
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
    if (event) {
        i3_client->event_queue_head = event->next;
        if (!i3_client->event_queue_head) {
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
    if (cmd->params) {
        json_object_put((json_object *)cmd->params);
    }
    free(cmd);
}

/* Free event structure */
void i3_free_event(i3_event_t *event)
{
    if (event->data) {
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
    
    if (params) {
        json_object_object_add(request, "params", (json_object *)params);
    }
    
    return request;
}

/* Send JSON object over socket */
int i3_send_json(void *obj)
{
    const char *json_str;
    char buffer[I3_MAX_STRING_LENGTH];
    int len, sent;
    
    if (!obj || i3_client->socket_fd < 0) {
        return -1;
    }
    
    json_str = json_object_to_json_string((json_object *)obj);
    len = snprintf(buffer, sizeof(buffer), "%s\n", json_str);
    
    i3_log("DEBUG: Sending %d bytes: %s", len, buffer);
    
    sent = send(i3_client->socket_fd, buffer, len, 0);
    if (sent < 0) {
        i3_error("Failed to send JSON: %s", strerror(errno));
        return -1;
    } else {
        i3_log("DEBUG: Successfully sent %d bytes", sent);
    }
    
    i3_client->messages_sent++;
    return 0;
}

/* Send tell message */
int i3_send_tell(const char *from_user, const char *target_mud, 
                  const char *target_user, const char *message)
{
    json_object *params;
    i3_command_t *cmd;
    
    if (!i3_client->enable_tell) {
        return -1;
    }
    
    params = json_object_new_object();
    json_object_object_add(params, "from_user", json_object_new_string(from_user));
    json_object_object_add(params, "target_mud", json_object_new_string(target_mud));
    json_object_object_add(params, "target_user", json_object_new_string(target_user));
    json_object_object_add(params, "message", json_object_new_string(message));
    
    cmd = (i3_command_t *)calloc(1, sizeof(i3_command_t));
    cmd->id = i3_client->next_request_id++;
    strcpy(cmd->method, "tell");
    cmd->params = params;
    
    i3_queue_command(cmd);
    return 0;
}

/* Send channel message */
int i3_send_channel_message(const char *channel, const char *from_user,
                            const char *message)
{
    json_object *params;
    i3_command_t *cmd;
    
    if (!i3_client->enable_channels) {
        return -1;
    }
    
    params = json_object_new_object();
    json_object_object_add(params, "channel", json_object_new_string(channel));
    json_object_object_add(params, "from_user", json_object_new_string(from_user));
    json_object_object_add(params, "message", json_object_new_string(message));
    
    cmd = (i3_command_t *)calloc(1, sizeof(i3_command_t));
    cmd->id = i3_client->next_request_id++;
    strcpy(cmd->method, "channel_send");
    cmd->params = params;
    
    i3_queue_command(cmd);
    return 0;
}

/* Process events from the queue */
void i3_process_events(void)
{
    i3_event_t *event;
    struct char_data *ch;
    
    while ((event = i3_pop_event()) != NULL) {
        switch (event->type) {
        case I3_MSG_TELL:
            /* THREAD SAFETY FIX: Do not access character_list from I3 thread */
            /* TODO: Implement proper event queuing to main thread */
            i3_log("DEBUG: Tell event processing disabled (thread safety)");
            break;

        case I3_MSG_CHANNEL:
            /* THREAD SAFETY FIX: Do not access character_list from I3 thread */
            /* TODO: Implement proper event queuing to main thread */
            i3_log("DEBUG: Channel event processing disabled (thread safety)");
            break;
            
        case I3_MSG_ERROR:
            /* Log errors */
            i3_error("I3 Error: %s", event->message);
            break;
            
        default:
            break;
        }
        
        i3_free_event(event);
    }
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
    if (i3_client) {
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
    if (!i3_client) {
        return "Not initialized";
    }
    
    switch (i3_client->state) {
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
    char key[128], value[128];
    char *p;
    int len;
    
    fp = fopen(filename, "r");
    if (!fp) {
        i3_error("DEBUG: Failed to open config file: %s (errno: %d - %s)", 
                 filename, errno, strerror(errno));
        return -1;
    }
    i3_log("DEBUG: Successfully opened config file: %s", filename);
    
    while (fgets(line, sizeof(line), fp)) {
        /* Skip comments and empty lines */
        if (line[0] == '#' || line[0] == '\n') {
            continue;
        }
        
        /* Remove trailing newline */
        len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }
        
        /* Find the first space to split key and value */
        p = strchr(line, ' ');
        if (p) {
            *p = '\0';
            strncpy(key, line, sizeof(key) - 1);
            key[sizeof(key) - 1] = '\0';
            strncpy(value, p + 1, sizeof(value) - 1);
            value[sizeof(value) - 1] = '\0';
            
            /* Debug log for API key loading */
            if (strcmp(key, "api_key") == 0) {
                i3_log("DEBUG: Loading API key from config: %s", value);
            }
            if (strcmp(key, "gateway_host") == 0) {
                strcpy(i3_client->gateway_host, value);
            } else if (strcmp(key, "gateway_port") == 0) {
                i3_client->gateway_port = atoi(value);
            } else if (strcmp(key, "api_key") == 0) {
                strcpy(i3_client->api_key, value);
            } else if (strcmp(key, "mud_name") == 0) {
                strcpy(i3_client->mud_name, value);
            } else if (strcmp(key, "default_channel") == 0) {
                strcpy(i3_client->default_channel, value);
            } else if (strcmp(key, "enable_tell") == 0) {
                i3_client->enable_tell = atoi(value);
            } else if (strcmp(key, "enable_channels") == 0) {
                i3_client->enable_channels = atoi(value);
            } else if (strcmp(key, "enable_who") == 0) {
                i3_client->enable_who = atoi(value);
            }
        }
    }
    
    fclose(fp);
    return 0;
}

/* Save configuration to file */
int i3_save_config(const char *filename)
{
    FILE *fp;
    
    fp = fopen(filename, "w");
    if (!fp) {
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