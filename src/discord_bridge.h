/**************************************************************************
 *  File: discord_bridge.h                            Part of LuminariMUD *
 *  Usage: Discord chat bridge integration - header file                  *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 *                                                                         *
 *  Copyright (C) 2025 LuminariMUD                                        *
 **************************************************************************/

#ifndef _DISCORD_BRIDGE_H_
#define _DISCORD_BRIDGE_H_

#include "structs.h"
#include "comm.h"

/* Configuration constants */
/* Campaign-dependent Discord bridge port */
#ifdef CAMPAIGN_DL
  #define DISCORD_BRIDGE_PORT 8201  /* DragonLance campaign port */
#elif defined(CAMPAIGN_FR)
  #define DISCORD_BRIDGE_PORT 8191  /* Forgotten Realms campaign port */
#else
  #define DISCORD_BRIDGE_PORT 8181  /* Default Luminari port */
#endif
#define DISCORD_BRIDGE_MAX_MSG_LEN 65535
#define DISCORD_BRIDGE_BUFFER_SIZE 4096
#define DISCORD_JSON_BUFFER_SIZE 66000  /* Large enough for max message + JSON overhead */
#define DISCORD_MAX_CHANNELS 20
/* #define DISCORD_CONNECTION_TIMEOUT 300 */  /* DISABLED - Discord bridge should never timeout */
#define DISCORD_RATE_LIMIT_MESSAGES 10  /* Max messages per window */
#define DISCORD_RATE_LIMIT_WINDOW 1     /* Window size in seconds */
#define DISCORD_AUTH_TOKEN_SIZE 64      /* Size of auth token */

/* Discord bridge state */
#define DISCORD_STATE_DISCONNECTED 0
#define DISCORD_STATE_LISTENING 1
#define DISCORD_STATE_CONNECTED 2
#define DISCORD_STATE_AUTHENTICATED 3

/* Message types */
#define DISCORD_MSG_NORMAL 0
#define DISCORD_MSG_EMOTE 1

/* Rate limiting structure */
struct discord_rate_limit {
    int message_count;         /* Messages in current window */
    time_t window_start;       /* Start of current window */
};

/* Channel configuration structure */
struct discord_channel_config {
    char mud_channel[64];      /* MUD channel name */
    char discord_name[64];     /* Discord channel name */ 
    int enabled;               /* Is this channel bridge enabled? */
    int filter_emotes;         /* Filter emote messages? */
    int scmd;                  /* MUD subcmd for this channel */
    struct discord_rate_limit rate_limit; /* Per-channel rate limiting */
};

/* Discord bridge structure */
struct discord_bridge_data {
    socket_t server_socket;    /* Server socket for accepting connections */
    socket_t client_socket;    /* Connected Discord bot client */
    int state;                 /* Connection state */
    char inbuf[DISCORD_BRIDGE_BUFFER_SIZE];  /* Input buffer */
    int inbuf_len;            /* Current input buffer length */
    char outbuf[DISCORD_BRIDGE_BUFFER_SIZE]; /* Output buffer */
    int outbuf_len;           /* Current output buffer length */
    time_t last_activity;     /* Last activity timestamp */
    time_t connection_time;   /* When connection was established */
    int messages_sent;        /* Statistics: messages sent to Discord */
    int messages_received;    /* Statistics: messages received from Discord */
    int messages_dropped;     /* Statistics: messages dropped due to rate limiting */
    char auth_token[DISCORD_AUTH_TOKEN_SIZE]; /* Authentication token */
    int authenticated;        /* Is connection authenticated? */
    struct discord_channel_config channels[DISCORD_MAX_CHANNELS];
    int num_channels;         /* Number of configured channels */
};

/* Global Discord bridge instance */
extern struct discord_bridge_data *discord_bridge;

/* Function prototypes */

/* Initialization and shutdown */
void init_discord_bridge(void);
void shutdown_discord_bridge(void);
void load_discord_config(void);
void save_discord_config(void);

/* Connection management */
int start_discord_server(int port);
void accept_discord_connection(void);
void close_discord_connection(void);
void process_discord_input(void);
void process_discord_output(void);

/* Message handling */
void send_to_discord(const char *channel, const char *name, const char *message, int emoted);
void receive_from_discord(const char *json_data);
void route_discord_to_mud(const char *channel, const char *name, const char *message);
void route_mud_to_discord(int subcmd, struct char_data *ch, const char *message, int emoted);

/* JSON processing */
char *build_discord_json(const char *channel, const char *name, const char *message, int emoted);
int parse_discord_json(const char *json_str, char *channel, char *name, char *message);

/* Channel management */
struct discord_channel_config *find_discord_channel_by_mud(const char *mud_channel);
struct discord_channel_config *find_discord_channel_by_discord(const char *discord_name);
struct discord_channel_config *find_discord_channel_by_scmd(int scmd);
void add_discord_channel(const char *mud_channel, const char *discord_name, int scmd, int enabled);
void remove_discord_channel(const char *mud_channel);
void toggle_discord_channel(const char *mud_channel, int enabled);

/* Utility functions */
char *strip_mud_colors(const char *text);
int is_discord_bridge_active(void);
void discord_bridge_status(struct char_data *ch);
void discord_bridge_statistics(struct char_data *ch);
int check_discord_rate_limit(struct discord_channel_config *channel);
void check_discord_timeout(void);

/* Command handlers */
void do_discord(struct char_data *ch, const char *argument, int cmd, int subcmd);

#endif /* _DISCORD_BRIDGE_H_ */