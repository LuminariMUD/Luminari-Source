/**
 * @file terrain_bridge.c
 * @author LuminariMUD Team
 * @brief Terrain Bridge API Server - Real-time wilderness data API
 * @version 1.0
 * @date 2025-08-15
 * 
 * TCP socket-based API server that provides real-time terrain calculations
 * for external tools (Python APIs, web services, etc.) without requiring
 * database pre-caching. Based on proven Discord bridge architecture.
 */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "terrain_bridge.h"
#include "wilderness.h"
#include "modify.h"  /* For strip_colors() */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#ifdef HAVE_JSON_C
#include <json-c/json.h>
#endif

/* Define INVALID_SOCKET if not already defined */
#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

/* Define CLOSE_SOCKET if not already defined */
#ifndef CLOSE_SOCKET
#define CLOSE_SOCKET(s) close(s)
#endif

/* External declarations for sector types */
extern const char *sector_types[];
extern const char *dirs[];
extern struct zone_data *zone_table;
extern zone_rnum top_of_zone_table;

/* External declarations for wilderness functions */
extern int get_moisture(int map, int x, int y);
extern int get_temperature(int map, int x, int y);

/* Global terrain API server instance */
/* Custom socket error handling for cross-platform compatibility */
#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif

/* socket_t compatibility for different systems */
#ifndef socket_t
#define socket_t int
#endif

static struct terrain_api_server *terrain_api = NULL;

/**
 * @brief Initialize and start the terrain API server
 * @param port TCP port to listen on (default: 8182)
 * @return 1 on success, 0 on failure
 */
int start_terrain_api_server(int port) {
    socket_t s;
    struct sockaddr_in sa;
    int opt = 1;
    
    if (terrain_api) {
        log("TERRAIN-API: Server already running on port %d", terrain_api->port);
        return 1;
    }
    
    log("TERRAIN-API: Starting terrain bridge API server...");
    
    /* Allocate server structure */
    CREATE(terrain_api, struct terrain_api_server, 1);
    terrain_api->server_socket = INVALID_SOCKET;
    terrain_api->clients = NULL;
    terrain_api->num_clients = 0;
    terrain_api->max_clients = TERRAIN_API_MAX_CLIENTS;
    terrain_api->total_requests = 0;
    terrain_api->total_connections = 0;
    terrain_api->start_time = time(NULL);
    
    /* Allocate client array */
    CREATE(terrain_api->clients, struct terrain_api_client, terrain_api->max_clients);
    
    /* Initialize all client sockets to invalid */
    {
        int i;
        for (i = 0; i < terrain_api->max_clients; i++) {
            terrain_api->clients[i].socket = INVALID_SOCKET;
            terrain_api->clients[i].input_pos = 0;
            terrain_api->clients[i].connect_time = 0;
            terrain_api->clients[i].requests_processed = 0;
        }
    }
    
    /* Create socket */
    if ((s = socket(PF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        log("TERRAIN-API: ERROR - Socket creation failed: %s", strerror(errno));
        free(terrain_api->clients);
        free(terrain_api);
        terrain_api = NULL;
        return 0;
    }
    
    /* Set socket options for reuse */
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0) {
        log("TERRAIN-API: WARNING - setsockopt SO_REUSEADDR failed: %s", strerror(errno));
    }
    
    /* Set non-blocking mode */
    if (fcntl(s, F_SETFL, O_NONBLOCK) < 0) {
        log("TERRAIN-API: WARNING - Failed to set non-blocking mode: %s", strerror(errno));
    }
    
    /* Bind to localhost only for security */
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);  /* localhost only */
    
    if (bind(s, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
        log("TERRAIN-API: ERROR - Bind to port %d failed: %s", port, strerror(errno));
        CLOSE_SOCKET(s);
        free(terrain_api->clients);
        free(terrain_api);
        terrain_api = NULL;
        return 0;
    }
    
    /* Listen for connections */
    if (listen(s, 5) < 0) {
        log("TERRAIN-API: ERROR - Listen failed: %s", strerror(errno));
        CLOSE_SOCKET(s);
        free(terrain_api->clients);
        free(terrain_api);
        terrain_api = NULL;
        return 0;
    }
    
    terrain_api->server_socket = s;
    terrain_api->port = port;
    
    log("TERRAIN-API: Server successfully started on localhost:%d", port);
    log("TERRAIN-API: Maximum clients: %d, Max message size: %d bytes", 
        TERRAIN_API_MAX_CLIENTS, TERRAIN_API_MAX_MSG_SIZE);
    
    return 1;
}

/**
 * @brief Stop the terrain API server and cleanup resources
 */
void stop_terrain_api_server(void) {
    if (!terrain_api) {
        return;
    }
    
    log("TERRAIN-API: Shutting down server...");
    
    /* Disconnect all clients */
    {
        int i;
        for (i = 0; i < terrain_api->max_clients; i++) {
            if (terrain_api->clients[i].socket != INVALID_SOCKET) {
                terrain_api_disconnect_client(i);
            }
        }
    }
    
    /* Close server socket */
    if (terrain_api->server_socket != INVALID_SOCKET) {
        CLOSE_SOCKET(terrain_api->server_socket);
    }
    
    /* Log final statistics */
    time_t uptime = time(NULL) - terrain_api->start_time;
    log("TERRAIN-API: Server stopped. Uptime: %ld seconds, Total requests: %d, Total connections: %d",
        uptime, terrain_api->total_requests, terrain_api->total_connections);
    
    /* Free memory */
    free(terrain_api->clients);
    free(terrain_api);
    terrain_api = NULL;
}

/**
 * @brief Accept new client connections (non-blocking)
 */
void terrain_api_accept_connections(void) {
    socket_t new_socket;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_slot = -1;
    
    /* Find available client slot */
    {
        int i;
        for (i = 0; i < terrain_api->max_clients; i++) {
            if (terrain_api->clients[i].socket == INVALID_SOCKET) {
                client_slot = i;
                break;
            }
        }
    }
    
    if (client_slot == -1) {
        /* No available slots - accept and immediately close */
        new_socket = accept(terrain_api->server_socket, (struct sockaddr *)&client_addr, &addr_len);
        if (new_socket != INVALID_SOCKET) {
            log("TERRAIN-API: Connection rejected - server full");
            CLOSE_SOCKET(new_socket);
        }
        return;
    }
    
    /* Accept new connection */
    new_socket = accept(terrain_api->server_socket, (struct sockaddr *)&client_addr, &addr_len);
    if (new_socket == INVALID_SOCKET) {
        /* No pending connections (normal for non-blocking) */
        return;
    }
    
    /* Set client socket to non-blocking */
    if (fcntl(new_socket, F_SETFL, O_NONBLOCK) < 0) {
        log("TERRAIN-API: WARNING - Failed to set client socket non-blocking: %s", strerror(errno));
    }
    
    /* Initialize client data */
    terrain_api->clients[client_slot].socket = new_socket;
    terrain_api->clients[client_slot].input_pos = 0;
    terrain_api->clients[client_slot].connect_time = time(NULL);
    terrain_api->clients[client_slot].requests_processed = 0;
    memset(terrain_api->clients[client_slot].input_buffer, 0, TERRAIN_API_MAX_MSG_SIZE);
    
    terrain_api->num_clients++;
    terrain_api->total_connections++;
    
    log("TERRAIN-API: Client connected from %s (slot %d, total clients: %d)",
        inet_ntoa(client_addr.sin_addr), client_slot, terrain_api->num_clients);
}

/**
 * @brief Disconnect a client and cleanup resources
 * @param client_index Index in the clients array
 */
void terrain_api_disconnect_client(int client_index) {
    if (client_index < 0 || client_index >= terrain_api->max_clients) {
        return;
    }
    
    struct terrain_api_client *client = &terrain_api->clients[client_index];
    
    if (client->socket != INVALID_SOCKET) {
        time_t session_time = time(NULL) - client->connect_time;
        log("TERRAIN-API: Client disconnected (slot %d, session: %ld seconds, requests: %d)",
            client_index, session_time, client->requests_processed);
        
        CLOSE_SOCKET(client->socket);
        client->socket = INVALID_SOCKET;
        client->input_pos = 0;
        client->connect_time = 0;
        client->requests_processed = 0;
        
        terrain_api->num_clients--;
    }
}

/**
 * @brief Process terrain calculation request and return JSON response
 * @param json_request JSON request string
 * @return JSON response string (caller must free)
 */
char *process_terrain_request(const char *json_request) {
#ifdef HAVE_JSON_C
    json_object *root, *cmd_obj, *x_obj, *y_obj, *params_obj;
    json_object *response, *result_obj;
    const char *command;
    int x, y;
    char *response_str;
    
    /* Parse JSON request */
    root = json_tokener_parse(json_request);
    if (!root) {
        return strdup("{\"error\":\"Invalid JSON\",\"success\":false}");
    }
    
    /* Get command */
    if (!json_object_object_get_ex(root, "command", &cmd_obj)) {
        json_object_put(root);
        return strdup("{\"error\":\"Missing command field\",\"success\":false}");
    }
    command = json_object_get_string(cmd_obj);
    
    /* Create response object */
    response = json_object_new_object();
    
    if (strcmp(command, "get_terrain") == 0) {
        /* Single coordinate terrain request */
        if (!json_object_object_get_ex(root, "x", &x_obj) ||
            !json_object_object_get_ex(root, "y", &y_obj)) {
            json_object_object_add(response, "error", json_object_new_string("Missing x or y coordinates"));
            json_object_object_add(response, "success", json_object_new_boolean(FALSE));
        } else {
            x = json_object_get_int(x_obj);
            y = json_object_get_int(y_obj);
            
            /* Validate coordinates against wilderness bounds */
            if (x < -1024 || x > 1024 || y < -1024 || y > 1024) {
                json_object_object_add(response, "error", 
                    json_object_new_string("Coordinates out of bounds (-1024 to 1024)"));
                json_object_object_add(response, "success", json_object_new_boolean(FALSE));
            } else {
                /* Calculate terrain values using existing wilderness functions */
                int elevation = get_elevation(NOISE_MATERIAL_PLANE_ELEV, x, y);
                int moisture = get_moisture(NOISE_MATERIAL_PLANE_MOISTURE, x, y);
                int temperature = get_temperature(NOISE_MATERIAL_PLANE_ELEV, x, y);
                int sector = get_sector_type(elevation, temperature, moisture);
                int weather = get_weather(x, y);
                
                /* Build result object */
                result_obj = json_object_new_object();
                json_object_object_add(result_obj, "x", json_object_new_int(x));
                json_object_object_add(result_obj, "y", json_object_new_int(y));
                json_object_object_add(result_obj, "elevation", json_object_new_int(elevation));
                json_object_object_add(result_obj, "moisture", json_object_new_int(moisture));
                json_object_object_add(result_obj, "temperature", json_object_new_int(temperature));
                json_object_object_add(result_obj, "sector_type", json_object_new_int(sector));
                
                /* Add sector name if available */
                if (sector >= 0 && sector < NUM_ROOM_SECTORS) {
                    json_object_object_add(result_obj, "sector_name", 
                        json_object_new_string(sector_types[sector]));
                } else {
                    json_object_object_add(result_obj, "sector_name", 
                        json_object_new_string("unknown"));
                }
                
                json_object_object_add(result_obj, "weather", json_object_new_int(weather));
                
                json_object_object_add(response, "success", json_object_new_boolean(TRUE));
                json_object_object_add(response, "data", result_obj);
            }
        }
    }
    else if (strcmp(command, "get_terrain_batch") == 0) {
        /* Batch terrain request */
        if (!json_object_object_get_ex(root, "params", &params_obj)) {
            json_object_object_add(response, "error", json_object_new_string("Missing batch parameters"));
            json_object_object_add(response, "success", json_object_new_boolean(FALSE));
        } else {
            json_object *x_min_obj, *x_max_obj, *y_min_obj, *y_max_obj;
            int x_min, x_max, y_min, y_max;
            
            if (json_object_object_get_ex(params_obj, "x_min", &x_min_obj) &&
                json_object_object_get_ex(params_obj, "x_max", &x_max_obj) &&
                json_object_object_get_ex(params_obj, "y_min", &y_min_obj) &&
                json_object_object_get_ex(params_obj, "y_max", &y_max_obj)) {
                
                x_min = json_object_get_int(x_min_obj);
                x_max = json_object_get_int(x_max_obj);
                y_min = json_object_get_int(y_min_obj);
                y_max = json_object_get_int(y_max_obj);
                
                /* Validate batch parameters */
                int total_coords = (x_max - x_min + 1) * (y_max - y_min + 1);
                if (total_coords > TERRAIN_API_MAX_BATCH_SIZE) {
                    json_object_object_add(response, "error", 
                        json_object_new_string("Batch too large (max 1000 coordinates)"));
                    json_object_object_add(response, "success", json_object_new_boolean(FALSE));
                } else if (x_min > x_max || y_min > y_max) {
                    json_object_object_add(response, "error", 
                        json_object_new_string("Invalid coordinate range"));
                    json_object_object_add(response, "success", json_object_new_boolean(FALSE));
                } else {
                    /* Process batch request */
                    json_object *data_array = json_object_new_array();
                    int bx, by;
                    
                    for (bx = x_min; bx <= x_max; bx++) {
                        for (by = y_min; by <= y_max; by++) {
                            json_object *coord_obj = json_object_new_object();
                            
                            int elevation = get_elevation(NOISE_MATERIAL_PLANE_ELEV, bx, by);
                            int moisture = get_moisture(NOISE_MATERIAL_PLANE_MOISTURE, bx, by);
                            int temperature = get_temperature(NOISE_MATERIAL_PLANE_ELEV, bx, by);
                            int sector = get_sector_type(elevation, temperature, moisture);
                            
                            json_object_object_add(coord_obj, "x", json_object_new_int(bx));
                            json_object_object_add(coord_obj, "y", json_object_new_int(by));
                            json_object_object_add(coord_obj, "elevation", json_object_new_int(elevation));
                            json_object_object_add(coord_obj, "moisture", json_object_new_int(moisture));
                            json_object_object_add(coord_obj, "temperature", json_object_new_int(temperature));
                            json_object_object_add(coord_obj, "sector_type", json_object_new_int(sector));
                            
                            json_object_array_add(data_array, coord_obj);
                        }
                    }
                    
                    json_object_object_add(response, "success", json_object_new_boolean(TRUE));
                    json_object_object_add(response, "data", data_array);
                    json_object_object_add(response, "count", json_object_new_int(total_coords));
                }
            } else {
                json_object_object_add(response, "error", 
                    json_object_new_string("Invalid batch parameters (need x_min, x_max, y_min, y_max)"));
                json_object_object_add(response, "success", json_object_new_boolean(FALSE));
            }
        }
    }
    else if (strcmp(command, "get_static_rooms_list") == 0) {
        /* Static room list with basic info only */
        json_object *data_array, *room_obj;
        json_object *limit_obj = NULL;
        room_rnum room_rnum_val;
        room_vnum room_vnum_val;
        struct room_data *room;
        zone_rnum zone_rnum_val;
        int count = 0;
        int max_rooms = 1000;  /* Higher limit for basic data */
        
        /* Check for optional limit parameter */
        if (json_object_object_get_ex(root, "limit", &limit_obj)) {
            int requested_limit = json_object_get_int(limit_obj);
            if (requested_limit > 0 && requested_limit <= 5000) {
                max_rooms = requested_limit;
            }
        }
        
        data_array = json_object_new_array();
        
        /* Iterate through all rooms */
        for (room_rnum_val = 0; room_rnum_val <= top_of_world; room_rnum_val++) {
            room = &world[room_rnum_val];
            room_vnum_val = GET_ROOM_VNUM(room_rnum_val);
            
            if (room_vnum_val == NOWHERE) continue;
            
            /* Only include wilderness static rooms in the proper VNUM range */
            if (room_vnum_val < WILD_ROOM_VNUM_START || room_vnum_val > WILD_ROOM_VNUM_END) {
                continue;  /* Skip non-wilderness rooms */
            }
            
            /* Get zone information for this room */
            zone_rnum_val = world[room_rnum_val].zone;
            
            /* Check if we've hit the limit */
            if (count >= max_rooms) {
                break;  /* Stop processing to prevent timeouts */
            }
            
            /* Create basic room object */
            room_obj = json_object_new_object();
            json_object_object_add(room_obj, "vnum", json_object_new_int64(room_vnum_val));
            json_object_object_add(room_obj, "name", json_object_new_string(room->name ? room->name : ""));
            json_object_object_add(room_obj, "x", json_object_new_int(room->coords[0]));
            json_object_object_add(room_obj, "y", json_object_new_int(room->coords[1]));
            json_object_object_add(room_obj, "sector_type", json_object_new_string(
                (room->sector_type >= 0 && room->sector_type < NUM_ROOM_SECTORS) ? 
                sector_types[room->sector_type] : "unknown"));
            
            /* Zone name only */
            if (zone_rnum_val >= 0 && zone_rnum_val <= top_of_zone_table) {
                char clean_zone_name[256];
                const char *zone_name = zone_table[zone_rnum_val].name ? zone_table[zone_rnum_val].name : "";
                
                /* Copy and strip color codes from zone name */
                strncpy(clean_zone_name, zone_name, sizeof(clean_zone_name) - 1);
                clean_zone_name[sizeof(clean_zone_name) - 1] = '\0';
                strip_colors(clean_zone_name);
                
                json_object_object_add(room_obj, "zone_name", json_object_new_string(clean_zone_name));
                json_object_object_add(room_obj, "zone_vnum", json_object_new_int(zone_table[zone_rnum_val].number));
            } else {
                json_object_object_add(room_obj, "zone_name", json_object_new_string(""));
                json_object_object_add(room_obj, "zone_vnum", json_object_new_int(-1));
            }
            
            json_object_array_add(data_array, room_obj);
            count++;
        }
        
        json_object_object_add(response, "success", json_object_new_boolean(TRUE));
        json_object_object_add(response, "total_rooms", json_object_new_int(count));
        json_object_object_add(response, "data", data_array);
    }
    else if (strcmp(command, "get_room_details") == 0) {
        /* Detailed room info for a specific room */
        json_object *vnum_obj;
        room_vnum target_vnum;
        room_rnum target_rnum;
        
        if (!json_object_object_get_ex(root, "vnum", &vnum_obj)) {
            json_object_object_add(response, "error", json_object_new_string("Missing vnum parameter"));
            json_object_object_add(response, "success", json_object_new_boolean(FALSE));
        } else {
            target_vnum = json_object_get_int64(vnum_obj);
            target_rnum = real_room(target_vnum);
            
            if (target_rnum == NOWHERE) {
                json_object_object_add(response, "error", json_object_new_string("Room not found"));
                json_object_object_add(response, "success", json_object_new_boolean(FALSE));
            } else {
                json_object *room_obj, *exits_array, *exit_obj, *zone_obj;
                struct room_data *room = &world[target_rnum];
                zone_rnum zone_rnum_val;
                int dir;
                
                /* Create detailed room object */
                room_obj = json_object_new_object();
                
                /* Basic room data */
                json_object_object_add(room_obj, "vnum", json_object_new_int64(target_vnum));
                json_object_object_add(room_obj, "name", json_object_new_string(room->name ? room->name : ""));
                json_object_object_add(room_obj, "description", json_object_new_string(room->description ? room->description : ""));
                json_object_object_add(room_obj, "sector_type", json_object_new_string(
                    (room->sector_type >= 0 && room->sector_type < NUM_ROOM_SECTORS) ? 
                    sector_types[room->sector_type] : "unknown"));
                json_object_object_add(room_obj, "sector_type_num", json_object_new_int(room->sector_type));
                
                /* Coordinates */
                json_object_object_add(room_obj, "x", json_object_new_int(room->coords[0]));
                json_object_object_add(room_obj, "y", json_object_new_int(room->coords[1]));
                
                /* Zone information */
                zone_rnum_val = world[target_rnum].zone;
                if (zone_rnum_val >= 0 && zone_rnum_val <= top_of_zone_table) {
                    char clean_zone_name[256];
                    const char *zone_name = zone_table[zone_rnum_val].name ? zone_table[zone_rnum_val].name : "";
                    
                    /* Copy and strip color codes from zone name */
                    strncpy(clean_zone_name, zone_name, sizeof(clean_zone_name) - 1);
                    clean_zone_name[sizeof(clean_zone_name) - 1] = '\0';
                    strip_colors(clean_zone_name);
                    
                    zone_obj = json_object_new_object();
                    json_object_object_add(zone_obj, "vnum", json_object_new_int(zone_table[zone_rnum_val].number));
                    json_object_object_add(zone_obj, "name", json_object_new_string(clean_zone_name));
                    json_object_object_add(zone_obj, "builders", json_object_new_string(
                        zone_table[zone_rnum_val].builders ? zone_table[zone_rnum_val].builders : ""));
                    json_object_object_add(zone_obj, "min_level", json_object_new_int(zone_table[zone_rnum_val].min_level));
                    json_object_object_add(zone_obj, "max_level", json_object_new_int(zone_table[zone_rnum_val].max_level));
                    json_object_object_add(room_obj, "zone", zone_obj);
                } else {
                    json_object_object_add(room_obj, "zone", NULL);
                }
                
                /* Exit information */
                exits_array = json_object_new_array();
                for (dir = 0; dir < NUM_OF_DIRS; dir++) {
                    if (room->dir_option[dir] && room->dir_option[dir]->to_room != NOWHERE) {
                        struct room_data *exit_room;
                        room_rnum exit_room_rnum;
                        room_vnum exit_room_vnum;
                        
                        exit_room_rnum = room->dir_option[dir]->to_room;
                        exit_room_vnum = (exit_room_rnum >= 0 && exit_room_rnum <= top_of_world) ? 
                                        GET_ROOM_VNUM(exit_room_rnum) : NOWHERE;
                        
                        if (exit_room_vnum != NOWHERE) {
                            exit_room = &world[exit_room_rnum];
                            
                            exit_obj = json_object_new_object();
                            json_object_object_add(exit_obj, "direction", json_object_new_string(dirs[dir]));
                            json_object_object_add(exit_obj, "direction_num", json_object_new_int(dir));
                            json_object_object_add(exit_obj, "to_room_vnum", json_object_new_int64(exit_room_vnum));
                            json_object_object_add(exit_obj, "to_room_name", json_object_new_string(
                                exit_room->name ? exit_room->name : ""));
                            json_object_object_add(exit_obj, "to_room_sector_type", json_object_new_string(
                                (exit_room->sector_type >= 0 && exit_room->sector_type < NUM_ROOM_SECTORS) ? 
                                sector_types[exit_room->sector_type] : "unknown"));
                            json_object_object_add(exit_obj, "to_room_sector_type_num", json_object_new_int(exit_room->sector_type));
                            
                            /* Zone info for exit room */
                            zone_rnum_val = world[exit_room_rnum].zone;
                            if (zone_rnum_val >= 0 && zone_rnum_val <= top_of_zone_table) {
                                json_object_object_add(exit_obj, "to_zone_vnum", json_object_new_int(zone_table[zone_rnum_val].number));
                                json_object_object_add(exit_obj, "to_zone_name", json_object_new_string(
                                    zone_table[zone_rnum_val].name ? zone_table[zone_rnum_val].name : ""));
                            } else {
                                json_object_object_add(exit_obj, "to_zone_vnum", json_object_new_int(-1));
                                json_object_object_add(exit_obj, "to_zone_name", json_object_new_string(""));
                            }
                            
                            /* Exit flags/info */
                            json_object_object_add(exit_obj, "exit_info", json_object_new_int(room->dir_option[dir]->exit_info));
                            json_object_object_add(exit_obj, "keyword", json_object_new_string(
                                room->dir_option[dir]->keyword ? room->dir_option[dir]->keyword : ""));
                            json_object_object_add(exit_obj, "description", json_object_new_string(
                                room->dir_option[dir]->general_description ? room->dir_option[dir]->general_description : ""));
                            
                            json_object_array_add(exits_array, exit_obj);
                        }
                    }
                }
                json_object_object_add(room_obj, "exits", exits_array);
                
                /* Room flags and light level */
                json_object_object_add(room_obj, "room_flags_0", json_object_new_int(room->room_flags[0]));
                json_object_object_add(room_obj, "room_flags_1", json_object_new_int(room->room_flags[1]));
                json_object_object_add(room_obj, "room_flags_2", json_object_new_int(room->room_flags[2]));
                json_object_object_add(room_obj, "room_flags_3", json_object_new_int(room->room_flags[3]));
                json_object_object_add(room_obj, "light", json_object_new_int(room->light));
                
                json_object_object_add(response, "success", json_object_new_boolean(TRUE));
                json_object_object_add(response, "data", room_obj);
            }
        }
    }
    else if (strcmp(command, "get_wilderness_exits") == 0) {
        /* Find wilderness rooms that have exits to non-wilderness zones */
        json_object *data_array = json_object_new_array();
        room_rnum room;
        int dir;
        int count = 0;
        
        /* Iterate through all wilderness rooms */
        for (room = 0; room <= top_of_world; room++) {
            /* Check if this is a wilderness room */
            if (IS_WILDERNESS_VNUM(world[room].number) && 
                world[room].coords[0] != 0 && world[room].coords[1] != 0) {
                
                /* Check all exits from this room */
                for (dir = 0; dir < NUM_OF_DIRS; dir++) {
                    if (world[room].dir_option[dir] && 
                        world[room].dir_option[dir]->to_room != NOWHERE) {
                        
                        room_rnum target_room = world[room].dir_option[dir]->to_room;
                        
                        /* If target room is not wilderness, this is an exit point */
                        if (!IS_WILDERNESS_VNUM(world[target_room].number)) {
                            json_object *exit_obj = json_object_new_object();
                            
                            /* Add wilderness room info */
                            json_object_object_add(exit_obj, "wilderness_vnum", 
                                json_object_new_int(world[room].number));
                            json_object_object_add(exit_obj, "wilderness_x", 
                                json_object_new_int(world[room].coords[0]));
                            json_object_object_add(exit_obj, "wilderness_y", 
                                json_object_new_int(world[room].coords[1]));
                            
                            /* Add wilderness room name (strip color codes) */
                            if (world[room].name) {
                                char *clean_name = strdup(world[room].name);
                                if (clean_name) {
                                    strip_colors(clean_name);
                                    json_object_object_add(exit_obj, "wilderness_name", 
                                        json_object_new_string(clean_name));
                                    free(clean_name);
                                } else {
                                    json_object_object_add(exit_obj, "wilderness_name", 
                                        json_object_new_string("Wilderness"));
                                }
                            } else {
                                json_object_object_add(exit_obj, "wilderness_name", 
                                    json_object_new_string("Wilderness"));
                            }
                            
                            /* Add exit direction */
                            json_object_object_add(exit_obj, "exit_direction", 
                                json_object_new_string(dirs[dir]));
                            
                            /* Add target zone info */
                            json_object_object_add(exit_obj, "target_vnum", 
                                json_object_new_int(world[target_room].number));
                            json_object_object_add(exit_obj, "target_zone", 
                                json_object_new_int(world[target_room].zone));
                            
                            /* Add target room name (strip color codes) */
                            if (world[target_room].name) {
                                char *clean_target_name = strdup(world[target_room].name);
                                if (clean_target_name) {
                                    strip_colors(clean_target_name);
                                    json_object_object_add(exit_obj, "target_name", 
                                        json_object_new_string(clean_target_name));
                                    free(clean_target_name);
                                } else {
                                    json_object_object_add(exit_obj, "target_name", 
                                        json_object_new_string("Unknown"));
                                }
                            } else {
                                json_object_object_add(exit_obj, "target_name", 
                                    json_object_new_string("Unknown"));
                            }
                            
                            /* Add zone name if available */
                            if (world[target_room].zone >= 0 && world[target_room].zone <= top_of_zone_table &&
                                zone_table[world[target_room].zone].name) {
                                char *clean_zone_name = strdup(zone_table[world[target_room].zone].name);
                                if (clean_zone_name) {
                                    strip_colors(clean_zone_name);
                                    json_object_object_add(exit_obj, "target_zone_name", 
                                        json_object_new_string(clean_zone_name));
                                    free(clean_zone_name);
                                } else {
                                    json_object_object_add(exit_obj, "target_zone_name", 
                                        json_object_new_string("Unknown Zone"));
                                }
                            } else {
                                json_object_object_add(exit_obj, "target_zone_name", 
                                    json_object_new_string("Unknown Zone"));
                            }
                            
                            json_object_array_add(data_array, exit_obj);
                            count++;
                            break; /* Only need to find one exit per room */
                        }
                    }
                }
            }
        }
        
        json_object_object_add(response, "success", json_object_new_boolean(TRUE));
        json_object_object_add(response, "count", json_object_new_int(count));
        json_object_object_add(response, "data", data_array);
    }
    else if (strcmp(command, "ping") == 0) {
        /* Simple connectivity test */
        json_object_object_add(response, "success", json_object_new_boolean(TRUE));
        json_object_object_add(response, "message", json_object_new_string("pong"));
        json_object_object_add(response, "server_time", json_object_new_int(time(NULL)));
        json_object_object_add(response, "uptime", 
            json_object_new_int(time(NULL) - terrain_api->start_time));
    }
    else {
        /* Unknown command */
        json_object_object_add(response, "error", 
            json_object_new_string("Unknown command (supported: get_terrain, get_terrain_batch, get_static_rooms_list, get_room_details, get_wilderness_exits, ping)"));
        json_object_object_add(response, "success", json_object_new_boolean(FALSE));
    }
    
    /* Convert response to string */
    response_str = strdup(json_object_to_json_string(response));
    
    /* Cleanup JSON objects */
    json_object_put(root);
    json_object_put(response);
    
    return response_str;
    
#else
    /* JSON-C not available */
    return strdup("{\"error\":\"JSON support not compiled in\",\"success\":false}");
#endif
}

/**
 * @brief Process incoming data from clients
 */
void terrain_api_process_clients(void) {
    int i;
    for (i = 0; i < terrain_api->max_clients; i++) {
        struct terrain_api_client *client = &terrain_api->clients[i];
        
        if (client->socket == INVALID_SOCKET) {
            continue;
        }
        
        /* Read data from client */
        char temp_buffer[1024];
        ssize_t bytes_read = recv(client->socket, temp_buffer, sizeof(temp_buffer) - 1, 0);
        
        if (bytes_read > 0) {
            temp_buffer[bytes_read] = '\0';
            
            /* Append to input buffer */
            int remaining_space = TERRAIN_API_MAX_MSG_SIZE - client->input_pos - 1;
            if (bytes_read <= remaining_space) {
                strncpy(client->input_buffer + client->input_pos, temp_buffer, bytes_read);
                client->input_pos += bytes_read;
                client->input_buffer[client->input_pos] = '\0';
                
                /* Check for complete message (newline terminated) */
                char *newline = strchr(client->input_buffer, '\n');
                if (newline) {
                    *newline = '\0';  /* Null-terminate the message */
                    
                    /* Process the request */
                    char *response = process_terrain_request(client->input_buffer);
                    
                    /* Send response */
                    if (response) {
                        size_t response_len = strlen(response);
                        char *response_with_newline = malloc(response_len + 2);
                        if (response_with_newline) {
                            snprintf(response_with_newline, response_len + 2, "%s\n", response);
                            
                            ssize_t sent = send(client->socket, response_with_newline, strlen(response_with_newline), 0);
                            if (sent < 0) {
                                log("TERRAIN-API: Send failed for client %d: %s", i, strerror(errno));
                                terrain_api_disconnect_client(i);
                            } else {
                                client->requests_processed++;
                                terrain_api->total_requests++;
                            }
                            
                            free(response_with_newline);
                        } else {
                            log("TERRAIN-API: Failed to allocate memory for response");
                            terrain_api_disconnect_client(i);
                        }
                        
                        free(response);
                    }
                    
                    /* Reset input buffer */
                    client->input_pos = 0;
                    memset(client->input_buffer, 0, TERRAIN_API_MAX_MSG_SIZE);
                }
            } else {
                /* Buffer overflow - disconnect client */
                log("TERRAIN-API: Input buffer overflow for client %d, disconnecting", i);
                terrain_api_disconnect_client(i);
            }
        }
        else if (bytes_read == 0) {
            /* Client disconnected cleanly */
            terrain_api_disconnect_client(i);
        }
        else if (errno != EAGAIN && errno != EWOULDBLOCK) {
            /* Real error occurred */
            log("TERRAIN-API: Receive error for client %d: %s", i, strerror(errno));
            terrain_api_disconnect_client(i);
        }
    }
}

/**
 * @brief Main processing function called from game loop
 */
void terrain_api_process(void) {
    if (!terrain_api || terrain_api->server_socket == INVALID_SOCKET) {
        return;
    }
    
    /* Accept new connections */
    terrain_api_accept_connections();
    
    /* Process existing client requests */
    terrain_api_process_clients();
}

/**
 * @brief Log current server statistics
 */
void log_terrain_api_stats(void) {
    if (!terrain_api) {
        log("TERRAIN-API: Server not running");
        return;
    }
    
    time_t uptime = time(NULL) - terrain_api->start_time;
    log("TERRAIN-API: Statistics - Port: %d, Uptime: %ld seconds", terrain_api->port, uptime);
    log("TERRAIN-API: Clients: %d/%d, Total connections: %d, Total requests: %d",
        terrain_api->num_clients, terrain_api->max_clients, 
        terrain_api->total_connections, terrain_api->total_requests);
}

/* Accessor functions for external code */
int terrain_api_is_running(void) {
  return (terrain_api && terrain_api->server_socket != INVALID_SOCKET);
}

struct terrain_api_server *get_terrain_api_server(void) {
  return terrain_api;
}

/* Wrapper functions for automatic startup/shutdown */
void terrain_api_start(void) {
    /* Debug logging for automatic startup */
    log("TERRAIN-API: terrain_api_start() called during initialization");
    
    /* Check if server is already running */
    if (terrain_api_is_running()) {
        log("TERRAIN-API: Server is already running, skipping startup");
        return;
    }
    
    /* Directly call server start like the working manual command */
    log("TERRAIN-API: Calling start_terrain_api_server(port=%d)", TERRAIN_API_DEFAULT_PORT);
    if (start_terrain_api_server(TERRAIN_API_DEFAULT_PORT)) {
        log("TERRAIN-API: Automatic startup successful on port %d", TERRAIN_API_DEFAULT_PORT);
    } else {
        log("TERRAIN-API: ERROR - Automatic startup failed on port %d", TERRAIN_API_DEFAULT_PORT);
    }
}

void terrain_api_stop(void) {
    if (terrain_api_is_running()) {
        stop_terrain_api_server();
    }
}
