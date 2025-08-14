/**************************************************************************
 *  File: discord_bridge.c                            Part of LuminariMUD *
 *  Usage: Discord chat bridge integration - implementation               *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 *                                                                         *
 *  Copyright (C) 2025 LuminariMUD                                        *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "discord_bridge.h"
#include "act.h"
#include "screen.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

/* Define INVALID_SOCKET if not already defined */
#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

/* Define CLOSE_SOCKET if not already defined */
#ifndef CLOSE_SOCKET
#define CLOSE_SOCKET(s) close(s)
#endif

/* Global Discord bridge instance */
struct discord_bridge_data *discord_bridge = NULL;

/* Simple JSON parser/builder functions (since we're in C89) */

/* Build a JSON message for Discord */
char *build_discord_json(const char *channel, const char *name, const char *message, int emoted) {
    static char json_buf[DISCORD_JSON_BUFFER_SIZE];
    char escaped_name[256];
    char escaped_message[DISCORD_BRIDGE_MAX_MSG_LEN];
    char escaped_channel[128];
    int i, j;
    
    /* Simple JSON escaping */
    j = 0;
    for (i = 0; name[i] && i < 254 && j < 254; i++) {
        if (name[i] == '"' || name[i] == '\\') {
            if (j >= 253) break;  /* No room for escape + char */
            escaped_name[j++] = '\\';
        }
        escaped_name[j++] = name[i];
    }
    escaped_name[j] = '\0';
    
    j = 0;
    for (i = 0; message[i] && j < DISCORD_BRIDGE_MAX_MSG_LEN - 2; i++) {
        if (message[i] == '"' || message[i] == '\\') {
            escaped_message[j++] = '\\';
        }
        else if (message[i] == '\n') {
            escaped_message[j++] = '\\';
            escaped_message[j++] = 'n';
            continue;
        }
        else if (message[i] == '\r') {
            continue; /* Skip carriage returns */
        }
        escaped_message[j++] = message[i];
    }
    escaped_message[j] = '\0';
    
    j = 0;
    for (i = 0; channel[i] && i < 126 && j < 126; i++) {
        if (channel[i] == '"' || channel[i] == '\\') {
            if (j >= 125) break;  /* No room for escape + char */
            escaped_channel[j++] = '\\';
        }
        escaped_channel[j++] = channel[i];
    }
    escaped_channel[j] = '\0';
    
    /* Build JSON */
    snprintf(json_buf, DISCORD_JSON_BUFFER_SIZE,
             "{\"channel\":\"%s\",\"name\":\"%s\",\"message\":\"%s\",\"emoted\":%d}",
             escaped_channel, escaped_name, escaped_message, emoted);
    
    return json_buf;
}

/* Parse a JSON message from Discord (simple parser) */
int parse_discord_json(const char *json_str, char *channel, char *name, char *message) {
    const char *p;
    char *dest;
    int in_string = 0;
    int escape_next = 0;
    
    /* Initialize outputs */
    channel[0] = '\0';
    name[0] = '\0';
    message[0] = '\0';
    
    /* Find and extract channel */
    p = strstr(json_str, "\"channel\"");
    if (!p) return 0;
    p = strchr(p + 9, ':');
    if (!p) return 0;
    p = strchr(p, '"');
    if (!p) return 0;
    p++; /* Skip opening quote */
    
    dest = channel;
    while (*p && *p != '"' && dest - channel < 63) {
        if (*p == '\\' && *(p+1)) {
            p++;
            if (*p == 'n') *dest++ = '\n';
            else *dest++ = *p;
        } else {
            *dest++ = *p;
        }
        p++;
    }
    *dest = '\0';
    
    /* Find and extract name */
    p = strstr(json_str, "\"name\"");
    if (!p) return 0;
    p = strchr(p + 6, ':');
    if (!p) return 0;
    p = strchr(p, '"');
    if (!p) return 0;
    p++; /* Skip opening quote */
    
    dest = name;
    while (*p && *p != '"' && dest - name < 255) {
        if (*p == '\\' && *(p+1)) {
            p++;
            if (*p == 'n') *dest++ = '\n';
            else *dest++ = *p;
        } else {
            *dest++ = *p;
        }
        p++;
    }
    *dest = '\0';
    
    /* Find and extract message */
    p = strstr(json_str, "\"message\"");
    if (!p) return 0;
    p = strchr(p + 9, ':');
    if (!p) return 0;
    p = strchr(p, '"');
    if (!p) return 0;
    p++; /* Skip opening quote */
    
    dest = message;
    while (*p && dest - message < DISCORD_BRIDGE_MAX_MSG_LEN - 1) {
        if (escape_next) {
            if (*p == 'n') *dest++ = '\n';
            else if (*p == 't') *dest++ = '\t';
            else *dest++ = *p;
            escape_next = 0;
        } else if (*p == '\\') {
            escape_next = 1;
        } else if (*p == '"' && !in_string) {
            break; /* End of message */
        } else {
            *dest++ = *p;
        }
        p++;
    }
    *dest = '\0';
    
    return 1;
}

/* Initialize the Discord bridge system */
void init_discord_bridge(void) {
    log("DEBUG: init_discord_bridge() called");
    
    if (discord_bridge) {
        log("WARNING: Discord bridge already initialized (ptr=%p), reinitializing for copyover", discord_bridge);
        shutdown_discord_bridge();
    }
    
    CREATE(discord_bridge, struct discord_bridge_data, 1);
    log("DEBUG: Discord bridge structure allocated at %p", discord_bridge);
    
    discord_bridge->server_socket = INVALID_SOCKET;
    discord_bridge->client_socket = INVALID_SOCKET;
    discord_bridge->state = DISCORD_STATE_DISCONNECTED;
    discord_bridge->inbuf_len = 0;
    discord_bridge->outbuf_len = 0;
    discord_bridge->last_activity = time(NULL);
    discord_bridge->messages_sent = 0;
    discord_bridge->messages_received = 0;
    discord_bridge->messages_dropped = 0;
    discord_bridge->num_channels = 0;
    discord_bridge->authenticated = 0;
    
    /* Set a default auth token - should be loaded from config file in production */
    /* Empty token means no authentication required */
    strcpy(discord_bridge->auth_token, "");  /* Set to a secret value for security */
    log("DEBUG: Auth token set (length=%d)", (int)strlen(discord_bridge->auth_token));
    
    /* Load configuration */
    log("DEBUG: Loading Discord configuration...");
    load_discord_config();
    
    /* Start the server socket */
    log("INFO: Starting Discord bridge server on port %d...", DISCORD_BRIDGE_PORT);
    if (start_discord_server(DISCORD_BRIDGE_PORT)) {
        log("SUCCESS: Discord bridge server started on port %d", DISCORD_BRIDGE_PORT);
        log("INFO: Discord bridge ready - server_socket=%d, state=%d", 
            discord_bridge->server_socket, discord_bridge->state);
    } else {
        log("ERROR: Failed to start Discord bridge server on port %d", DISCORD_BRIDGE_PORT);
        free(discord_bridge);
        discord_bridge = NULL;
        log("DEBUG: Discord bridge structure freed, ptr set to NULL");
    }
}

/* Shutdown the Discord bridge system */
void shutdown_discord_bridge(void) {
    log("DEBUG: shutdown_discord_bridge() called");
    
    if (!discord_bridge) {
        log("DEBUG: Discord bridge already NULL, nothing to shutdown");
        return;
    }
    
    log("INFO: Shutting down Discord bridge (ptr=%p, server_socket=%d)", 
        discord_bridge, discord_bridge->server_socket);
    
    close_discord_connection();
    
    if (discord_bridge->server_socket != INVALID_SOCKET) {
        log("DEBUG: Closing server socket %d", discord_bridge->server_socket);
        if (CLOSE_SOCKET(discord_bridge->server_socket) < 0) {
            log("WARNING: Error closing server socket: %s", strerror(errno));
        } else {
            log("DEBUG: Server socket closed successfully");
        }
        discord_bridge->server_socket = INVALID_SOCKET;
    }
    
    save_discord_config();
    
    log("DEBUG: Freeing Discord bridge structure at %p", discord_bridge);
    free(discord_bridge);
    discord_bridge = NULL;
    
    log("INFO: Discord bridge shutdown complete");
}

/* Start the Discord server socket */
int start_discord_server(int port) {
    struct sockaddr_in sa;
    int opt = 1;
    int retries = 3;
    int retry_delay = 1;
    
    log("DEBUG: start_discord_server() called with port %d", port);
    
    if (!discord_bridge) {
        log("ERROR: Discord bridge structure is NULL");
        return 0;
    }
    
    /* Create socket */
    discord_bridge->server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (discord_bridge->server_socket == INVALID_SOCKET) {
        log("SYSERR: Discord bridge failed to create socket: %s (errno=%d)", 
            strerror(errno), errno);
        return 0;
    }
    log("DEBUG: Socket created successfully, fd=%d", discord_bridge->server_socket);
    
    /* Set socket options - SO_REUSEADDR and SO_REUSEPORT for Linux */
    if (setsockopt(discord_bridge->server_socket, SOL_SOCKET, SO_REUSEADDR,
                   (char *)&opt, sizeof(opt)) < 0) {
        log("WARNING: Discord bridge setsockopt SO_REUSEADDR failed: %s (errno=%d)", 
            strerror(errno), errno);
    } else {
        log("DEBUG: SO_REUSEADDR set successfully");
    }
    
#ifdef SO_REUSEPORT
    if (setsockopt(discord_bridge->server_socket, SOL_SOCKET, SO_REUSEPORT,
                   (char *)&opt, sizeof(opt)) < 0) {
        log("WARNING: Discord bridge setsockopt SO_REUSEPORT failed: %s (errno=%d)", 
            strerror(errno), errno);
    } else {
        log("DEBUG: SO_REUSEPORT set successfully");
    }
#else
    log("DEBUG: SO_REUSEPORT not defined on this system");
#endif
    
    /* Set non-blocking */
    if (fcntl(discord_bridge->server_socket, F_SETFL, O_NONBLOCK) < 0) {
        log("WARNING: Discord bridge failed to set non-blocking: %s (errno=%d)", 
            strerror(errno), errno);
    } else {
        log("DEBUG: Socket set to non-blocking mode");
    }
    
    /* Bind to port - INADDR_ANY allows connections from any interface */
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);  /* Binds to 0.0.0.0 - accepts from any interface */
    
    log("DEBUG: Attempting to bind socket %d to 0.0.0.0:%d", 
        discord_bridge->server_socket, port);
    
    /* Try binding with retries for copyover recovery */
    while (retries > 0) {
        log("DEBUG: Bind attempt %d of %d", (4 - retries), 3);
        
        if (bind(discord_bridge->server_socket, (struct sockaddr *)&sa, sizeof(sa)) == 0) {
            /* Success */
            log("SUCCESS: Socket bound to port %d on attempt %d", port, (4 - retries));
            break;
        }
        
        /* Save errno immediately as it can be changed by other calls */
        int saved_errno = errno;
        
        log("DEBUG: Bind failed with errno=%d (%s)", saved_errno, strerror(saved_errno));
        
        /* If it's not "Address already in use", fail immediately */
        if (saved_errno != EADDRINUSE) {
            log("ERROR: Discord bridge bind failed with unexpected error: %s (errno=%d)", 
                strerror(saved_errno), saved_errno);
            CLOSE_SOCKET(discord_bridge->server_socket);
            discord_bridge->server_socket = INVALID_SOCKET;
            return 0;
        }
        
        /* For EADDRINUSE, retry after a delay */
        retries--;
        if (retries > 0) {
            log("WARNING: Port %d is in use (EADDRINUSE), will retry in %d second(s)... (%d retries left)",
                port, retry_delay, retries);
            sleep(retry_delay);
        } else {
            log("ERROR: Discord bridge bind failed after all retries: %s (errno=%d)", 
                strerror(saved_errno), saved_errno);
            log("ERROR: Port %d remains in use after 3 attempts", port);
            log("HINT: Check if another process is using port %d with: netstat -tulpn | grep %d", port, port);
            CLOSE_SOCKET(discord_bridge->server_socket);
            discord_bridge->server_socket = INVALID_SOCKET;
            return 0;
        }
    }
    
    /* Listen for connections */
    log("DEBUG: Setting socket to listen mode...");
    if (listen(discord_bridge->server_socket, 1) < 0) {
        log("ERROR: Discord bridge listen failed: %s (errno=%d)", strerror(errno), errno);
        CLOSE_SOCKET(discord_bridge->server_socket);
        discord_bridge->server_socket = INVALID_SOCKET;
        return 0;
    }
    
    log("SUCCESS: Socket listening on port %d", port);
    discord_bridge->state = DISCORD_STATE_LISTENING;
    log("DEBUG: Discord bridge state set to LISTENING (%d)", discord_bridge->state);
    return 1;
}

/* Accept a new Discord connection */
void accept_discord_connection(void) {
    struct sockaddr_in peer;
    socklen_t peer_len = sizeof(peer);
    socket_t new_socket;
    
    if (!discord_bridge || discord_bridge->server_socket == INVALID_SOCKET)
        return;
    
    /* Accept new connection */
    new_socket = accept(discord_bridge->server_socket, (struct sockaddr *)&peer, &peer_len);
    if (new_socket == INVALID_SOCKET) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            log("SYSERR: Discord bridge accept failed: %s", strerror(errno));
        }
        return;
    }
    
    /* Check if we already have a connection - only allow one */
    if (discord_bridge->client_socket != INVALID_SOCKET) {
        log("Discord bridge: Rejecting connection from %s:%d - already have active connection",
            inet_ntoa(peer.sin_addr), ntohs(peer.sin_port));
        CLOSE_SOCKET(new_socket);
        return;
    }
    
    /* Set non-blocking */
    if (fcntl(new_socket, F_SETFL, O_NONBLOCK) < 0) {
        log("SYSERR: Discord bridge failed to set client non-blocking: %s", strerror(errno));
    }
    
    /* Store the new connection */
    discord_bridge->client_socket = new_socket;
    discord_bridge->state = DISCORD_STATE_CONNECTED;
    discord_bridge->last_activity = time(NULL);
    discord_bridge->connection_time = time(NULL);
    discord_bridge->authenticated = 0;  /* Require authentication */
    discord_bridge->inbuf_len = 0;
    discord_bridge->outbuf_len = 0;
    
    log("Discord bridge connected from %s:%d", 
        inet_ntoa(peer.sin_addr), ntohs(peer.sin_port));
}

/* Close the Discord connection */
void close_discord_connection(void) {
    if (!discord_bridge) {
        log("DEBUG: close_discord_connection() - bridge is NULL");
        return;
    }
    
    if (discord_bridge->client_socket == INVALID_SOCKET) {
        log("DEBUG: close_discord_connection() - client_socket already INVALID");
        return;
    }
    
    log("DEBUG: Closing client socket %d", discord_bridge->client_socket);
    if (CLOSE_SOCKET(discord_bridge->client_socket) < 0) {
        log("WARNING: Error closing client socket: %s", strerror(errno));
    }
    
    discord_bridge->client_socket = INVALID_SOCKET;
    
    if (discord_bridge->server_socket != INVALID_SOCKET) {
        discord_bridge->state = DISCORD_STATE_LISTENING;
        log("DEBUG: State changed to LISTENING (server still active)");
    } else {
        discord_bridge->state = DISCORD_STATE_DISCONNECTED;
        log("DEBUG: State changed to DISCONNECTED (no server socket)");
    }
    
    discord_bridge->inbuf_len = 0;
    discord_bridge->outbuf_len = 0;
    
    log("INFO: Discord bridge connection closed");
}

/* Process input from Discord */
void process_discord_input(void) {
    ssize_t bytes_read;
    char *newline;
    char line_buf[DISCORD_JSON_BUFFER_SIZE];
    
    if (!discord_bridge || discord_bridge->client_socket == INVALID_SOCKET)
        return;
    
    /* Read data from socket */
    bytes_read = recv(discord_bridge->client_socket, 
                      discord_bridge->inbuf + discord_bridge->inbuf_len,
                      DISCORD_BRIDGE_BUFFER_SIZE - discord_bridge->inbuf_len - 1, 0);
    
    if (bytes_read < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            log("SYSERR: Discord bridge recv error: %s", strerror(errno));
            close_discord_connection();
        }
        return;
    } else if (bytes_read == 0) {
        /* Connection closed */
        close_discord_connection();
        return;
    }
    
    discord_bridge->inbuf_len += bytes_read;
    discord_bridge->inbuf[discord_bridge->inbuf_len] = '\0';
    discord_bridge->last_activity = time(NULL);  /* Update activity timestamp */
    
    /* Process complete lines */
    while ((newline = strchr(discord_bridge->inbuf, '\n')) != NULL) {
        *newline = '\0';
        strncpy(line_buf, discord_bridge->inbuf, DISCORD_JSON_BUFFER_SIZE - 1);
        line_buf[DISCORD_JSON_BUFFER_SIZE - 1] = '\0';
        
        /* Process the JSON message */
        receive_from_discord(line_buf);
        
        /* Move remaining data to beginning of buffer */
        memmove(discord_bridge->inbuf, newline + 1, 
                discord_bridge->inbuf_len - (newline - discord_bridge->inbuf + 1));
        discord_bridge->inbuf_len -= (newline - discord_bridge->inbuf + 1);
    }
    
    /* Check for buffer overflow */
    if (discord_bridge->inbuf_len >= DISCORD_BRIDGE_BUFFER_SIZE - 1) {
        log("SYSERR: Discord bridge input buffer overflow, clearing");
        discord_bridge->inbuf_len = 0;
    }
}

/* Process output to Discord */
void process_discord_output(void) {
    ssize_t bytes_sent;
    
    if (!discord_bridge || discord_bridge->client_socket == INVALID_SOCKET)
        return;
    
    if (discord_bridge->outbuf_len == 0)
        return;
    
    /* Send data */
    bytes_sent = send(discord_bridge->client_socket, 
                      discord_bridge->outbuf, 
                      discord_bridge->outbuf_len, 0);
    
    if (bytes_sent < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            log("SYSERR: Discord bridge send error: %s", strerror(errno));
            close_discord_connection();
        }
        return;
    }
    
    /* Remove sent data from buffer */
    if (bytes_sent > 0) {
        memmove(discord_bridge->outbuf, 
                discord_bridge->outbuf + bytes_sent,
                discord_bridge->outbuf_len - bytes_sent);
        discord_bridge->outbuf_len -= bytes_sent;
    }
}

/* Send a message to Discord */
void send_to_discord(const char *channel, const char *name, const char *message, int emoted) {
    char *json;
    int json_len;
    
    if (!discord_bridge || discord_bridge->client_socket == INVALID_SOCKET)
        return;
    
    /* Build JSON message */
    json = build_discord_json(channel, name, message, emoted);
    json_len = strlen(json);
    
    /* Check buffer space */
    if (discord_bridge->outbuf_len + json_len + 2 >= DISCORD_BRIDGE_BUFFER_SIZE) {
        log("SYSERR: Discord bridge output buffer full, dropping message");
        return;
    }
    
    /* Add to output buffer */
    strcpy(discord_bridge->outbuf + discord_bridge->outbuf_len, json);
    discord_bridge->outbuf_len += json_len;
    discord_bridge->outbuf[discord_bridge->outbuf_len++] = '\n';
    discord_bridge->outbuf[discord_bridge->outbuf_len] = '\0';
    
    discord_bridge->messages_sent++;
    
    /* Try to send immediately */
    process_discord_output();
}

/* Receive a message from Discord */
void receive_from_discord(const char *json_data) {
    char channel[64];
    char name[256];
    char message[DISCORD_BRIDGE_MAX_MSG_LEN];
    
    if (!discord_bridge)
        return;
    
    /* Parse JSON */
    if (!parse_discord_json(json_data, channel, name, message)) {
        log("SYSERR: Discord bridge failed to parse JSON: %s", json_data);
        return;
    }
    
    /* Check for authentication message if not authenticated and token is set */
    if (strlen(discord_bridge->auth_token) > 0 && !discord_bridge->authenticated) {
        /* First message should be auth token */
        if (strcmp(channel, "auth") == 0 && strcmp(message, discord_bridge->auth_token) == 0) {
            discord_bridge->authenticated = 1;
            discord_bridge->state = DISCORD_STATE_AUTHENTICATED;
            log("Discord bridge: Client authenticated successfully");
            return;
        } else {
            log("SYSERR: Discord bridge authentication failed");
            close_discord_connection();
            return;
        }
    }
    
    /* If auth is required but not authenticated, reject message */
    if (strlen(discord_bridge->auth_token) > 0 && !discord_bridge->authenticated) {
        log("SYSERR: Discord bridge received message from unauthenticated client");
        return;
    }
    
    /* Validate input */
    if (strlen(channel) == 0 || strlen(name) == 0 || strlen(message) == 0) {
        log("SYSERR: Discord bridge received empty field in message");
        return;
    }
    
    /* Sanitize Discord username to prevent injection */
    /* Remove special characters that could be interpreted as MUD commands */
    if (strchr(name, '@') || strchr(name, '!') || strchr(name, '#')) {
        log("SYSERR: Discord bridge received invalid username: %s", name);
        return;
    }
    
    /* Truncate message if too long - use Discord's max, not MUD's */
    if (strlen(message) > DISCORD_BRIDGE_MAX_MSG_LEN) {
        message[DISCORD_BRIDGE_MAX_MSG_LEN - 1] = '\0';
    }
    
    discord_bridge->messages_received++;
    
    /* Route to MUD */
    route_discord_to_mud(channel, name, message);
}

/* Route a Discord message to the MUD */
void route_discord_to_mud(const char *channel, const char *name, const char *message) {
    struct discord_channel_config *config;
    struct descriptor_data *d;
    char buf[MAX_STRING_LENGTH];
    char formatted_name[256];
    int channel_flag = 0;
    
    /* Find channel configuration */
    config = find_discord_channel_by_discord(channel);
    if (!config || !config->enabled) {
        return;
    }
    
    /* Check rate limiting */
    if (!check_discord_rate_limit(config)) {
        log("SYSERR: Discord bridge rate limit exceeded for channel %s", channel);
        discord_bridge->messages_dropped++;
        return;
    }
    
    /* Format the Discord user's name to prevent loops */
    snprintf(formatted_name, sizeof(formatted_name), "[Discord] %s", name);
    
    /* Map SCMD to channel PRF flag */
    switch (config->scmd) {
        case SCMD_GOSSIP:
            channel_flag = PRF_NOGOSS;
            break;
        case SCMD_AUCTION:
            channel_flag = PRF_NOAUCT;
            break;
        case SCMD_GRATZ:
            channel_flag = PRF_NOGRATZ;
            break;
        case SCMD_SHOUT:
            channel_flag = PRF_NOSHOUT;
            break;
        default:
            channel_flag = 0;
            break;
    }
    
    /* Send to all connected players who have the channel enabled */
    for (d = descriptor_list; d; d = d->next) {
        if (STATE(d) != CON_PLAYING || !d->character)
            continue;
        
        /* Check if player has this channel disabled */
        if (channel_flag && PRF_FLAGGED(d->character, channel_flag))
            continue;
            
        /* Format message with color codes for this player */
        snprintf(buf, sizeof(buf), "%s%s: %s%s\r\n", 
                 CCYEL(d->character, C_NRM), formatted_name, message, CCNRM(d->character, C_NRM));
        
        /* Send the message */
        send_to_char(d->character, "%s", buf);
    }
}

/* Route a MUD message to Discord */
void route_mud_to_discord(int subcmd, struct char_data *ch, const char *message, int emoted) {
    struct discord_channel_config *config;
    char *cleaned_message;
    
    if (!discord_bridge || !is_discord_bridge_active())
        return;
    
    /* Find channel configuration */
    config = find_discord_channel_by_scmd(subcmd);
    if (!config || !config->enabled)
        return;
    
    /* Skip if it's a Discord-originated message */
    if (strstr(GET_NAME(ch), "[Discord]"))
        return;
    
    /* Strip MUD color codes */
    cleaned_message = strip_mud_colors(message);
    
    /* Send to Discord */
    send_to_discord(config->discord_name, GET_NAME(ch), cleaned_message, emoted);
}

/* Find Discord channel by MUD channel name */
struct discord_channel_config *find_discord_channel_by_mud(const char *mud_channel) {
    int i;
    
    if (!discord_bridge)
        return NULL;
    
    for (i = 0; i < discord_bridge->num_channels; i++) {
        if (!str_cmp(discord_bridge->channels[i].mud_channel, mud_channel))
            return &discord_bridge->channels[i];
    }
    
    return NULL;
}

/* Find Discord channel by Discord name */
struct discord_channel_config *find_discord_channel_by_discord(const char *discord_name) {
    int i;
    
    if (!discord_bridge)
        return NULL;
    
    for (i = 0; i < discord_bridge->num_channels; i++) {
        if (!str_cmp(discord_bridge->channels[i].discord_name, discord_name))
            return &discord_bridge->channels[i];
    }
    
    return NULL;
}

/* Find Discord channel by subcmd */
struct discord_channel_config *find_discord_channel_by_scmd(int scmd) {
    int i;
    
    if (!discord_bridge)
        return NULL;
    
    for (i = 0; i < discord_bridge->num_channels; i++) {
        if (discord_bridge->channels[i].scmd == scmd)
            return &discord_bridge->channels[i];
    }
    
    return NULL;
}

/* Add a Discord channel configuration */
void add_discord_channel(const char *mud_channel, const char *discord_name, int scmd, int enabled) {
    struct discord_channel_config *config;
    
    if (!discord_bridge)
        return;
    
    if (discord_bridge->num_channels >= DISCORD_MAX_CHANNELS) {
        log("SYSERR: Discord bridge max channels reached");
        return;
    }
    
    config = &discord_bridge->channels[discord_bridge->num_channels];
    strncpy(config->mud_channel, mud_channel, 63);
    config->mud_channel[63] = '\0';
    strncpy(config->discord_name, discord_name, 63);
    config->discord_name[63] = '\0';
    config->scmd = scmd;
    config->enabled = enabled;
    config->filter_emotes = 0;
    
    discord_bridge->num_channels++;
}

/* Strip MUD color codes from text */
char *strip_mud_colors(const char *text) {
    static char buf[MAX_STRING_LENGTH];
    const char *ptr = text;
    char *dest = buf;
    
    while (*ptr && dest - buf < MAX_STRING_LENGTH - 1) {
        if (*ptr == '@') {
            /* Skip color code */
            ptr++;
            if (*ptr) ptr++;
        } else if (*ptr == '\x1B' && *(ptr+1) == '[') {
            /* Skip ANSI codes */
            ptr += 2;
            while (*ptr && *ptr != 'm')
                ptr++;
            if (*ptr == 'm')
                ptr++;
        } else {
            *dest++ = *ptr++;
        }
    }
    
    *dest = '\0';
    return buf;
}

/* Check if Discord bridge is active */
int is_discord_bridge_active(void) {
    if (discord_bridge && 
        discord_bridge->client_socket != INVALID_SOCKET &&
        (discord_bridge->state == DISCORD_STATE_CONNECTED ||
         discord_bridge->state == DISCORD_STATE_AUTHENTICATED)) {
        return 1;
    }
    return 0;
}

/* Display Discord bridge status */
void discord_bridge_status(struct char_data *ch) {
    int i;
    
    if (!discord_bridge) {
        send_to_char(ch, "Discord bridge is not initialized.\r\n");
        return;
    }
    
    send_to_char(ch, "Discord Bridge Status:\r\n");
    send_to_char(ch, "----------------------\r\n");
    send_to_char(ch, "Server Socket: %s\r\n", 
                 discord_bridge->server_socket != INVALID_SOCKET ? "Active" : "Inactive");
    send_to_char(ch, "Client Connection: %s\r\n",
                 discord_bridge->client_socket != INVALID_SOCKET ? "Connected" : "Disconnected");
    send_to_char(ch, "Authentication: %s\r\n",
                 strlen(discord_bridge->auth_token) == 0 ? "Disabled" : 
                 (discord_bridge->authenticated ? "Authenticated" : "Not Authenticated"));
    send_to_char(ch, "Messages Sent: %d\r\n", discord_bridge->messages_sent);
    send_to_char(ch, "Messages Received: %d\r\n", discord_bridge->messages_received);
    send_to_char(ch, "Messages Dropped (Rate Limit): %d\r\n", discord_bridge->messages_dropped);
    if (discord_bridge->client_socket != INVALID_SOCKET) {
        time_t now = time(NULL);
        send_to_char(ch, "Connection Time: %ld seconds\r\n", now - discord_bridge->connection_time);
        send_to_char(ch, "Last Activity: %ld seconds ago\r\n", now - discord_bridge->last_activity);
    }
    send_to_char(ch, "\r\nConfigured Channels:\r\n");
    
    for (i = 0; i < discord_bridge->num_channels; i++) {
        send_to_char(ch, "  %s -> %s [%s]\r\n",
                     discord_bridge->channels[i].mud_channel,
                     discord_bridge->channels[i].discord_name,
                     discord_bridge->channels[i].enabled ? "Enabled" : "Disabled");
    }
}

/* Load Discord configuration */
void load_discord_config(void) {
    /* Default configuration - add channels as needed */
    /* These subcmds need to match your MUD's channel system */
    log("DEBUG: Adding default Discord channels...");
    add_discord_channel("gossip", "gossip", SCMD_GOSSIP, 1);
    add_discord_channel("auction", "auction", SCMD_AUCTION, 1);
    add_discord_channel("gratz", "gratz", SCMD_GRATZ, 1);
    
    log("INFO: Discord bridge configuration loaded with %d channels", discord_bridge->num_channels);
    if (discord_bridge->num_channels > 0) {
        int i;
        for (i = 0; i < discord_bridge->num_channels; i++) {
            log("  Channel %d: MUD='%s' Discord='%s' SCMD=%d Enabled=%d",
                i+1, discord_bridge->channels[i].mud_channel,
                discord_bridge->channels[i].discord_name,
                discord_bridge->channels[i].scmd,
                discord_bridge->channels[i].enabled);
        }
    }
}

/* Save Discord configuration */
void save_discord_config(void) {
    /* TODO: Implement saving to file */
    log("Discord bridge configuration saved");
}

/* Check rate limit for a channel */
int check_discord_rate_limit(struct discord_channel_config *channel) {
    time_t now = time(NULL);
    struct discord_rate_limit *rl = &channel->rate_limit;
    
    /* Reset window if expired */
    if (now - rl->window_start >= DISCORD_RATE_LIMIT_WINDOW) {
        rl->message_count = 0;
        rl->window_start = now;
    }
    
    /* Check if limit exceeded */
    if (rl->message_count >= DISCORD_RATE_LIMIT_MESSAGES) {
        return 0; /* Rate limit exceeded */
    }
    
    rl->message_count++;
    return 1; /* OK to send */
}

/* Check for connection timeout */
void check_discord_timeout(void) {
    time_t now;
    
    if (!discord_bridge || discord_bridge->client_socket == INVALID_SOCKET) {
        return;
    }
    
    now = time(NULL);
    
    /* Check if connection has been idle too long */
    if (now - discord_bridge->last_activity > DISCORD_CONNECTION_TIMEOUT) {
        log("SYSERR: Discord bridge connection timed out (idle for %ld seconds)",
            now - discord_bridge->last_activity);
        close_discord_connection();
    }
}

/* Discord command handler */
ACMD(do_discord)
{
    char arg[MAX_INPUT_LENGTH];
    
    one_argument(argument, arg, sizeof(arg));
    
    if (!*arg) {
        discord_bridge_status(ch);
        return;
    }
    
    if (!str_cmp(arg, "start")) {
        if (!discord_bridge) {
            init_discord_bridge();
            send_to_char(ch, "Discord bridge started.\r\n");
        } else {
            send_to_char(ch, "Discord bridge is already running.\r\n");
        }
    } else if (!str_cmp(arg, "stop")) {
        if (discord_bridge) {
            shutdown_discord_bridge();
            send_to_char(ch, "Discord bridge stopped.\r\n");
        } else {
            send_to_char(ch, "Discord bridge is not running.\r\n");
        }
    } else if (!str_cmp(arg, "status")) {
        discord_bridge_status(ch);
    } else {
        send_to_char(ch, "Usage: discord [start|stop|status]\r\n");
    }
}