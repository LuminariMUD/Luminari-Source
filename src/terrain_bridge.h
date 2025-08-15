#ifndef __TERRAIN_BRIDGE_H__
#define __TERRAIN_BRIDGE_H__

#include "conf.h"
#include "sysdep.h"

/* Default port for terrain API server */
#define TERRAIN_API_DEFAULT_PORT 8182

/* Maximum clients and message sizes */
#define TERRAIN_API_MAX_CLIENTS 10
#define TERRAIN_API_MAX_MSG_SIZE 4096
#define TERRAIN_API_MAX_BATCH_SIZE 1000

/* Client connection structure */
struct terrain_api_client {
    socket_t socket;
    char input_buffer[TERRAIN_API_MAX_MSG_SIZE];
    int input_pos;
    time_t connect_time;
    int requests_processed;
};

/* Main server structure */
struct terrain_api_server {
    socket_t server_socket;
    int port;
    struct terrain_api_client *clients;
    int num_clients;
    int max_clients;
    int total_requests;
    int total_connections;
    time_t start_time;
};

/* Function prototypes */
int start_terrain_api_server(int port);
void stop_terrain_api_server(void);
void terrain_api_process(void);
void terrain_api_accept_connections(void);
void terrain_api_process_clients(void);
void terrain_api_disconnect_client(int client_index);
char *process_terrain_request(const char *json_request);
void log_terrain_api_stats(void);

/* Accessor functions for terrain API server */
int terrain_api_is_running(void);
struct terrain_api_server *get_terrain_api_server(void);

/* Automatic startup/shutdown functions */
void terrain_api_start(void);
void terrain_api_stop(void);

#endif /* __TERRAIN_BRIDGE_H__ */
