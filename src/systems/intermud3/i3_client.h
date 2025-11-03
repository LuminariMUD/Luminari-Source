/* ************************************************************************
*  Intermud3 Client for LuminariMUD                                      *
*  Based on CircleMUD/tbaMUD implementation                              *
*  Copyright (C) 2025                                                     *
*                                                                         *
*  This integration allows LuminariMUD to connect to the global          *
*  Intermud3 network through the I3 Gateway service.                     *
************************************************************************ */

#ifndef _I3_CLIENT_H_
#define _I3_CLIENT_H_

/* Configuration constants */
#define I3_MAX_STRING_LENGTH    4096
#define I3_MAX_QUEUE_SIZE       1000
#define I3_RECONNECT_DELAY      30
#define I3_HEARTBEAT_INTERVAL   30
#define I3_DEFAULT_PORT         8081
#define I3_MAX_CHANNELS         20

/* I3 Connection states */
typedef enum {
    I3_STATE_DISCONNECTED = 0,
    I3_STATE_CONNECTING,
    I3_STATE_AUTHENTICATING,
    I3_STATE_CONNECTED,
    I3_STATE_RECONNECTING,
    I3_STATE_SHUTDOWN
} i3_state_t;

/* I3 Message types */
typedef enum {
    I3_MSG_TELL = 1,
    I3_MSG_EMOTETO,
    I3_MSG_CHANNEL,
    I3_MSG_CHANNEL_EMOTE,
    I3_MSG_WHO_REPLY,
    I3_MSG_FINGER_REPLY,
    I3_MSG_LOCATE_REPLY,
    I3_MSG_MUDLIST_REPLY,
    I3_MSG_ERROR,
    I3_MSG_CHANNEL_JOIN,
    I3_MSG_CHANNEL_LEAVE
} i3_msg_type_t;

/* Forward declarations for structs */
struct i3_command;
struct i3_event;
struct i3_mud;

/* I3 Command structure for outgoing commands */
typedef struct i3_command {
    int id;
    char method[64];
    void *params;  /* JSON object pointer */
    struct i3_command *next;
} i3_command_t;

/* I3 Event structure for incoming events */
typedef struct i3_event {
    i3_msg_type_t type;
    char from_mud[128];
    char from_user[128];
    char to_user[128];
    char channel[64];
    char message[I3_MAX_STRING_LENGTH];
    void *data;  /* JSON object pointer */
    struct i3_event *next;
} i3_event_t;

/* I3 Channel structure */
typedef struct i3_channel {
    char name[64];
    int type;  /* 0 = public, 1 = private */
    char owner[128];
    int subscribed;  /* Using int instead of bool for C89 */
    int member_count;
} i3_channel_t;

/* I3 MUD info structure */
typedef struct i3_mud {
    char name[128];
    char driver[64];
    char mud_type[64];
    char admin_email[256];
    int port;
    int online;  /* Using int instead of bool for C89 */
    char services[256];  /* Comma-separated list */
    struct i3_mud *next;
} i3_mud_t;

/* Main I3 Client structure */
typedef struct {
    /* Connection info */
    char gateway_host[256];
    int gateway_port;
    char api_key[256];
    char mud_name[128];
    char session_id[128];
    
    /* Connection state */
    i3_state_t state;
    int socket_fd;
    int authenticated;
    time_t last_heartbeat;
    time_t connect_time;
    
    /* Threading - using void pointers for pthread types */
    void *thread_id;
    void *command_mutex;
    void *event_mutex;
    void *state_mutex;
    
    /* Message queues */
    i3_command_t *command_queue_head;
    i3_command_t *command_queue_tail;
    i3_event_t *event_queue_head;
    i3_event_t *event_queue_tail;
    int command_queue_size;
    int event_queue_size;
    
    /* Channels */
    i3_channel_t channels[I3_MAX_CHANNELS];
    int channel_count;
    char default_channel[64];
    
    /* MUD list cache */
    i3_mud_t *mud_list;
    time_t mud_list_updated;
    
    /* Statistics */
    unsigned long messages_sent;
    unsigned long messages_received;
    unsigned long errors;
    unsigned long reconnects;
    
    /* Configuration */
    int enable_tell;
    int enable_channels;
    int enable_who;
    int auto_reconnect;
    int reconnect_delay;
    int max_queue_size;
    
    /* Request ID counter */
    int next_request_id;
} i3_client_t;

/* Global I3 client instance */
extern i3_client_t *i3_client;

/* Core functions */
int i3_initialize(void);
void i3_shutdown(void);
int i3_connect(void);
void i3_disconnect(void);
int i3_is_connected(void);

/* Thread functions */
void *i3_client_thread(void *arg);
void i3_process_events(void);

/* Command functions */
int i3_send_tell(const char *from_user, const char *target_mud, 
                  const char *target_user, const char *message);
int i3_send_emoteto(const char *from_user, const char *target_mud,
                    const char *target_user, const char *emote);
int i3_send_channel_message(const char *channel, const char *from_user,
                            const char *message);
int i3_send_channel_emote(const char *channel, const char *from_user,
                          const char *emote);
int i3_request_who(const char *target_mud);
int i3_request_finger(const char *target_mud, const char *target_user);
int i3_request_locate(const char *target_user);
int i3_request_mudlist(void);
int i3_join_channel(const char *channel, const char *user_name);
int i3_leave_channel(const char *channel, const char *user_name);
int i3_list_channels(void);

/* Event handling */
i3_event_t *i3_pop_event(void);
void i3_free_event(i3_event_t *event);

/* Utility functions */
const char *i3_get_state_string(void);
void i3_get_statistics(char *buf, size_t bufsize);
i3_mud_t *i3_find_mud(const char *name);
i3_channel_t *i3_find_channel(const char *name);

/* Configuration */
int i3_load_config(const char *filename);
int i3_save_config(const char *filename);

/* Logging */
void i3_log(const char *format, ...);
void i3_error(const char *format, ...);
void i3_debug(const char *format, ...);

/* JSON helpers - implementation specific */
void *i3_create_request(const char *method, void *params);
int i3_send_json(void *obj);
int i3_parse_response(const char *json_str);

/* Internal queue management */
void i3_queue_command(i3_command_t *cmd);

/* Command declarations for interpreter.c */
void do_i3tell(struct char_data *ch, const char *argument, int cmd, int subcmd);
void do_i3chat(struct char_data *ch, const char *argument, int cmd, int subcmd);
void do_i3who(struct char_data *ch, const char *argument, int cmd, int subcmd);
void do_i3finger(struct char_data *ch, const char *argument, int cmd, int subcmd);
void do_i3locate(struct char_data *ch, const char *argument, int cmd, int subcmd);
void do_i3mudlist(struct char_data *ch, const char *argument, int cmd, int subcmd);
void do_i3channels(struct char_data *ch, const char *argument, int cmd, int subcmd);
void do_i3config(struct char_data *ch, const char *argument, int cmd, int subcmd);
void do_i3admin(struct char_data *ch, const char *argument, int cmd, int subcmd);

#endif /* _I3_CLIENT_H_ */