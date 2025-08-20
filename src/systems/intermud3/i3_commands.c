/* ************************************************************************
*  Intermud3 Command Implementations for LuminariMUD                     *
*  Player and immortal command handlers                                  *
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
#include <time.h>

/* Define UNUSED_VAR if not already defined */
#ifndef UNUSED_VAR
#define UNUSED_VAR(x) ((void)(x))
#endif

/* I3 Tell command */
void do_i3tell(struct char_data *ch, const char *argument, int cmd, int subcmd)
{
    char target[MAX_INPUT_LENGTH];
    char arg_copy[MAX_INPUT_LENGTH];
    const char *message;
    char target_user[128], target_mud[128];
    char *at_sign;
    i3_mud_t *mud;
    
    UNUSED_VAR(cmd);
    UNUSED_VAR(subcmd);
    
    if (!i3_client || !i3_is_connected()) {
        send_to_char(ch, "The Intermud3 network is currently unavailable.\r\n");
        return;
    }
    
    /* Make a copy to work with */
    strcpy(arg_copy, argument);
    
    /* Parse arguments */
    message = one_argument(arg_copy, target, sizeof(target));
    skip_spaces((char **)&message);
    
    if (!*target || !*message) {
        send_to_char(ch, "Usage: i3tell <user>@<mud> <message>\r\n");
        return;
    }
    
    /* Parse user@mud format */
    at_sign = strchr(target, '@');
    if (!at_sign) {
        send_to_char(ch, "You must specify both user and MUD: <user>@<mud>\r\n");
        return;
    }
    
    *at_sign = '\0';
    strcpy(target_user, target);
    strcpy(target_mud, at_sign + 1);
    
    /* Validate MUD exists */
    mud = i3_find_mud(target_mud);
    if (!mud) {
        send_to_char(ch, "Unknown MUD: %s\r\n", target_mud);
        return;
    }
    
    if (!mud->online) {
        send_to_char(ch, "That MUD is currently offline.\r\n");
        return;
    }
    
    /* Send the tell */
    if (i3_send_tell(GET_NAME(ch), target_mud, target_user, message) == 0) {
        send_to_char(ch, "%sYou tell %s@%s: %s%s\r\n", 
                     CCYEL(ch, C_NRM), target_user, target_mud, message, CCNRM(ch, C_NRM));
    } else {
        send_to_char(ch, "Failed to send tell.\r\n");
    }
}

/* I3 Chat command - send to default channel */
void do_i3chat(struct char_data *ch, const char *argument, int cmd, int subcmd)
{
    char channel[64];
    char arg_copy[MAX_INPUT_LENGTH];
    const char *message;
    char *arg_ptr;
    
    UNUSED_VAR(cmd);
    UNUSED_VAR(subcmd);
    
    if (!i3_client || !i3_is_connected()) {
        send_to_char(ch, "The Intermud3 network is currently unavailable.\r\n");
        return;
    }
    
    /* Make a copy to work with */
    strcpy(arg_copy, argument);
    arg_ptr = arg_copy;
    skip_spaces(&arg_ptr);
    
    /* Check if a channel was specified */
    message = one_argument(arg_ptr, channel, sizeof(channel));
    
    /* If no channel specified, use default */
    if (!*message) {
        message = arg_ptr;
        strcpy(channel, i3_client->default_channel);
    } else {
        skip_spaces((char **)&message);
    }
    
    if (!*message) {
        send_to_char(ch, "Usage: i3chat [channel] <message>\r\n");
        return;
    }
    
    /* Send the channel message */
    if (i3_send_channel_message(channel, GET_NAME(ch), message) == 0) {
        send_to_char(ch, "%s[%s] You: %s%s\r\n", 
                     CCYEL(ch, C_NRM), channel, message, CCNRM(ch, C_NRM));
    } else {
        send_to_char(ch, "Failed to send channel message.\r\n");
    }
}

/* I3 Who command - list players on a MUD */
void do_i3who(struct char_data *ch, const char *argument, int cmd, int subcmd)
{
    char target_mud[128];
    
    UNUSED_VAR(cmd);
    UNUSED_VAR(subcmd);
    
    if (!ch) {
        return;
    }
    
    if (!i3_client || !i3_is_connected()) {
        send_to_char(ch, "The Intermud3 network is currently unavailable.\r\n");
        return;
    }
    
    one_argument(argument, target_mud, sizeof(target_mud));
    
    if (!*target_mud) {
        send_to_char(ch, "Usage: i3who <mud_name>\r\n");
        send_to_char(ch, "Use 'i3mudlist' to see available MUDs.\r\n");
        return;
    }
    
    /* For now, since we don't have a mud list yet */
    send_to_char(ch, "i3who: Requesting player list from %s...\r\n", target_mud);
    send_to_char(ch, "(Note: MUD list validation not yet implemented)\r\n");
    
    /* Request who list */
    if (i3_request_who(target_mud) == 0) {
        send_to_char(ch, "Request sent.\r\n");
    } else {
        send_to_char(ch, "Failed to send request.\r\n");
    }
}

/* I3 Finger command - get user info */
void do_i3finger(struct char_data *ch, const char *argument, int cmd, int subcmd)
{
    char target[MAX_INPUT_LENGTH];
    char target_user[128], target_mud[128];
    char *at_sign;
    i3_mud_t *mud;
    
    UNUSED_VAR(cmd);
    UNUSED_VAR(subcmd);
    
    if (!i3_client || !i3_is_connected()) {
        send_to_char(ch, "The Intermud3 network is currently unavailable.\r\n");
        return;
    }
    
    one_argument(argument, target, sizeof(target));
    
    if (!*target) {
        send_to_char(ch, "Usage: i3finger <user>@<mud>\r\n");
        return;
    }
    
    /* Parse user@mud format */
    at_sign = strchr(target, '@');
    if (!at_sign) {
        send_to_char(ch, "You must specify both user and MUD: <user>@<mud>\r\n");
        return;
    }
    
    *at_sign = '\0';
    strcpy(target_user, target);
    strcpy(target_mud, at_sign + 1);
    
    /* Validate MUD exists */
    mud = i3_find_mud(target_mud);
    if (!mud) {
        send_to_char(ch, "Unknown MUD: %s\r\n", target_mud);
        return;
    }
    
    if (!mud->online) {
        send_to_char(ch, "That MUD is currently offline.\r\n");
        return;
    }
    
    /* Request finger info */
    if (i3_request_finger(target_mud, target_user) == 0) {
        send_to_char(ch, "Requesting finger info for %s@%s...\r\n", target_user, target_mud);
    } else {
        send_to_char(ch, "Failed to request finger info.\r\n");
    }
}

/* I3 Locate command - find a user on the network */
void do_i3locate(struct char_data *ch, const char *argument, int cmd, int subcmd)
{
    char target_user[128];
    
    UNUSED_VAR(cmd);
    UNUSED_VAR(subcmd);
    
    if (!i3_client || !i3_is_connected()) {
        send_to_char(ch, "The Intermud3 network is currently unavailable.\r\n");
        return;
    }
    
    one_argument(argument, target_user, sizeof(target_user));
    
    if (!*target_user) {
        send_to_char(ch, "Usage: i3locate <username>\r\n");
        return;
    }
    
    /* Request locate */
    if (i3_request_locate(target_user) == 0) {
        send_to_char(ch, "Searching for %s on the Intermud3 network...\r\n", target_user);
    } else {
        send_to_char(ch, "Failed to initiate locate request.\r\n");
    }
}

/* I3 Mudlist command - list all MUDs on the network */
void do_i3mudlist(struct char_data *ch, const char *argument, int cmd, int subcmd)
{
    i3_mud_t *mud;
    int count = 0;
    char buf[MAX_STRING_LENGTH];
    
    UNUSED_VAR(argument);
    UNUSED_VAR(cmd);
    UNUSED_VAR(subcmd);
    
    if (!i3_client || !i3_is_connected()) {
        send_to_char(ch, "The Intermud3 network is currently unavailable.\r\n");
        return;
    }
    
    send_to_char(ch, "%sIntermud3 MUD List:%s\r\n", CCYEL(ch, C_NRM), CCNRM(ch, C_NRM));
    send_to_char(ch, "%-20s %-10s %-15s %s\r\n", "MUD Name", "Status", "Type", "Port");
    send_to_char(ch, "--------------------------------------------------------\r\n");
    
    for (mud = i3_client->mud_list; mud; mud = mud->next) {
        snprintf(buf, sizeof(buf), "%-20s %-10s %-15s %d\r\n",
                mud->name, 
                mud->online ? "Online" : "Offline",
                mud->mud_type,
                mud->port);
        send_to_char(ch, "%s", buf);
        count++;
    }
    
    send_to_char(ch, "--------------------------------------------------------\r\n");
    send_to_char(ch, "Total MUDs: %d\r\n", count);
    
    /* Request updated list if cache is old */
    if (time(NULL) - i3_client->mud_list_updated > 3600) {
        i3_request_mudlist();
        send_to_char(ch, "Requesting updated MUD list from network...\r\n");
    }
}

/* I3 Channels command - manage channel subscriptions */
void do_i3channels(struct char_data *ch, const char *argument, int cmd, int subcmd)
{
    char cmd_arg[MAX_INPUT_LENGTH];
    char channel[64];
    char arg_copy[MAX_INPUT_LENGTH];
    const char *arg_ptr;
    int i;
    
    UNUSED_VAR(cmd);
    UNUSED_VAR(subcmd);
    
    if (!i3_client || !i3_is_connected()) {
        send_to_char(ch, "The Intermud3 network is currently unavailable.\r\n");
        return;
    }
    
    /* Make a copy to work with */
    strcpy(arg_copy, argument);
    arg_ptr = one_argument(arg_copy, cmd_arg, sizeof(cmd_arg));
    one_argument(arg_ptr, channel, sizeof(channel));
    
    if (!*cmd_arg) {
        /* List channels */
        send_to_char(ch, "%sIntermud3 Channels:%s\r\n", CCYEL(ch, C_NRM), CCNRM(ch, C_NRM));
        for (i = 0; i < i3_client->channel_count; i++) {
            send_to_char(ch, "  %-20s %s\r\n", 
                        i3_client->channels[i].name,
                        i3_client->channels[i].subscribed ? "(subscribed)" : "");
        }
        send_to_char(ch, "\r\nUsage: i3channels <list|join|leave> [channel]\r\n");
        return;
    }
    
    if (!strcasecmp(cmd_arg, "list")) {
        i3_list_channels();
        send_to_char(ch, "Requesting channel list from network...\r\n");
    } else if (!strcasecmp(cmd_arg, "join")) {
        if (!*channel) {
            send_to_char(ch, "Usage: i3channels join <channel>\r\n");
            return;
        }
        if (i3_join_channel(channel, GET_NAME(ch)) == 0) {
            send_to_char(ch, "Joining channel '%s'...\r\n", channel);
        } else {
            send_to_char(ch, "Failed to join channel.\r\n");
        }
    } else if (!strcasecmp(cmd_arg, "leave")) {
        if (!*channel) {
            send_to_char(ch, "Usage: i3channels leave <channel>\r\n");
            return;
        }
        if (i3_leave_channel(channel, GET_NAME(ch)) == 0) {
            send_to_char(ch, "Leaving channel '%s'...\r\n", channel);
        } else {
            send_to_char(ch, "Failed to leave channel.\r\n");
        }
    } else {
        send_to_char(ch, "Usage: i3channels <list|join|leave> [channel]\r\n");
    }
}

/* I3 Config command - configure I3 settings */
void do_i3config(struct char_data *ch, const char *argument, int cmd, int subcmd)
{
    char arg[MAX_INPUT_LENGTH];
    
    UNUSED_VAR(cmd);
    UNUSED_VAR(subcmd);
    
    if (!i3_client) {
        send_to_char(ch, "The Intermud3 system is not initialized.\r\n");
        return;
    }
    
    one_argument(argument, arg, sizeof(arg));
    
    if (!*arg) {
        send_to_char(ch, "%sIntermud3 Configuration:%s\r\n", CCYEL(ch, C_NRM), CCNRM(ch, C_NRM));
        send_to_char(ch, "  Tells:    %s\r\n", i3_client->enable_tell ? "Enabled" : "Disabled");
        send_to_char(ch, "  Channels: %s\r\n", i3_client->enable_channels ? "Enabled" : "Disabled");
        send_to_char(ch, "  Who:      %s\r\n", i3_client->enable_who ? "Enabled" : "Disabled");
        send_to_char(ch, "\r\nUsage: i3config <tells|channels|who> <on|off>\r\n");
        return;
    }
    
    /* Toggle settings */
    if (!strcasecmp(arg, "tells")) {
        i3_client->enable_tell = !i3_client->enable_tell;
        send_to_char(ch, "I3 tells %s.\r\n", i3_client->enable_tell ? "enabled" : "disabled");
    } else if (!strcasecmp(arg, "channels")) {
        i3_client->enable_channels = !i3_client->enable_channels;
        send_to_char(ch, "I3 channels %s.\r\n", i3_client->enable_channels ? "enabled" : "disabled");
    } else if (!strcasecmp(arg, "who")) {
        i3_client->enable_who = !i3_client->enable_who;
        send_to_char(ch, "I3 who %s.\r\n", i3_client->enable_who ? "enabled" : "disabled");
    } else {
        send_to_char(ch, "Usage: i3config <tells|channels|who> <on|off>\r\n");
    }
}

/* I3 Admin command - admin functions */
void do_i3admin(struct char_data *ch, const char *argument, int cmd, int subcmd)
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    
    UNUSED_VAR(cmd);
    UNUSED_VAR(subcmd);
    
    if (!i3_client) {
        send_to_char(ch, "The Intermud3 system is not initialized.\r\n");
        return;
    }
    
    one_argument(argument, arg, sizeof(arg));
    
    if (!*arg) {
        send_to_char(ch, "I3 Admin Commands:\r\n");
        send_to_char(ch, "  i3admin status     - Show connection status\r\n");
        send_to_char(ch, "  i3admin stats      - Show statistics\r\n");
        send_to_char(ch, "  i3admin reconnect  - Force reconnection\r\n");
        send_to_char(ch, "  i3admin reload     - Reload configuration\r\n");
        send_to_char(ch, "  i3admin save       - Save configuration\r\n");
        return;
    }
    
    if (!strcasecmp(arg, "status")) {
        snprintf(buf, sizeof(buf), 
                "%sI3 Status:%s %s\r\n"
                "MUD Name: %s\r\n"
                "Gateway: %s:%d\r\n"
                "Session: %s\r\n"
                "Authenticated: %s\r\n"
                "Uptime: %ld seconds\r\n",
                CCYEL(ch, C_NRM), CCNRM(ch, C_NRM),
                i3_get_state_string(),
                i3_client->mud_name,
                i3_client->gateway_host, i3_client->gateway_port,
                i3_client->session_id,
                i3_client->authenticated ? "Yes" : "No",
                i3_client->connect_time ? (time(NULL) - i3_client->connect_time) : 0);
        send_to_char(ch, "%s", buf);
    } else if (!strcasecmp(arg, "stats")) {
        i3_get_statistics(buf, sizeof(buf));
        send_to_char(ch, "%s", buf);
    } else if (!strcasecmp(arg, "reconnect")) {
        i3_disconnect();
        if (i3_connect() == 0) {
            send_to_char(ch, "Reconnecting to I3 gateway...\r\n");
        } else {
            send_to_char(ch, "Failed to reconnect.\r\n");
        }
    } else if (!strcasecmp(arg, "reload")) {
        if (i3_load_config("lib/i3_config") == 0) {
            send_to_char(ch, "Configuration reloaded.\r\n");
        } else {
            send_to_char(ch, "Failed to reload configuration.\r\n");
        }
    } else if (!strcasecmp(arg, "save")) {
        if (i3_save_config("lib/i3_config") == 0) {
            send_to_char(ch, "Configuration saved.\r\n");
        } else {
            send_to_char(ch, "Failed to save configuration.\r\n");
        }
    } else {
        send_to_char(ch, "Unknown admin command. Type 'i3admin' for help.\r\n");
    }
}

/* Helper function implementations */
i3_mud_t *i3_find_mud(const char *name)
{
    i3_mud_t *mud;
    
    if (!i3_client) {
        return NULL;
    }
    
    for (mud = i3_client->mud_list; mud; mud = mud->next) {
        if (!strcasecmp(mud->name, name)) {
            return mud;
        }
    }
    
    return NULL;
}

i3_channel_t *i3_find_channel(const char *name)
{
    int i;
    
    if (!i3_client) {
        return NULL;
    }
    
    for (i = 0; i < i3_client->channel_count; i++) {
        if (!strcasecmp(i3_client->channels[i].name, name)) {
            return &i3_client->channels[i];
        }
    }
    
    return NULL;
}

/* Get statistics string */
void i3_get_statistics(char *buf, size_t bufsize)
{
    if (!i3_client) {
        snprintf(buf, bufsize, "I3 client not initialized.\r\n");
        return;
    }
    
    snprintf(buf, bufsize,
            "Intermud3 Statistics:\r\n"
            "Messages Sent:     %lu\r\n"
            "Messages Received: %lu\r\n"
            "Errors:            %lu\r\n"
            "Reconnects:        %lu\r\n"
            "Command Queue:     %d/%d\r\n"
            "Event Queue:       %d/%d\r\n"
            "Channels:          %d\r\n"
            "MUDs in List:      %d\r\n",
            i3_client->messages_sent,
            i3_client->messages_received,
            i3_client->errors,
            i3_client->reconnects,
            i3_client->command_queue_size, i3_client->max_queue_size,
            i3_client->event_queue_size, i3_client->max_queue_size,
            i3_client->channel_count,
            0); /* TODO: Count MUDs in list */
}

/* Stub implementations for remaining protocol functions */
int i3_request_who(const char *target_mud)
{
    /* TODO: Implement */
    UNUSED_VAR(target_mud);
    return 0;
}

int i3_request_finger(const char *target_mud, const char *target_user)
{
    /* TODO: Implement */
    UNUSED_VAR(target_mud);
    UNUSED_VAR(target_user);
    return 0;
}

int i3_request_locate(const char *target_user)
{
    /* TODO: Implement */
    UNUSED_VAR(target_user);
    return 0;
}

int i3_request_mudlist(void)
{
    /* TODO: Implement */
    return 0;
}

int i3_join_channel(const char *channel, const char *user_name)
{
    /* TODO: Implement */
    UNUSED_VAR(channel);
    UNUSED_VAR(user_name);
    return 0;
}

int i3_leave_channel(const char *channel, const char *user_name)
{
    /* TODO: Implement */
    UNUSED_VAR(channel);
    UNUSED_VAR(user_name);
    return 0;
}

int i3_list_channels(void)
{
    /* TODO: Implement */
    return 0;
}

int i3_send_emoteto(const char *from_user, const char *target_mud,
                    const char *target_user, const char *emote)
{
    /* TODO: Implement */
    UNUSED_VAR(from_user);
    UNUSED_VAR(target_mud);
    UNUSED_VAR(target_user);
    UNUSED_VAR(emote);
    return 0;
}

int i3_send_channel_emote(const char *channel, const char *from_user,
                          const char *emote)
{
    /* TODO: Implement */
    UNUSED_VAR(channel);
    UNUSED_VAR(from_user);
    UNUSED_VAR(emote);
    return 0;
}

int i3_parse_response(const char *json_str)
{
    /* TODO: Implement */
    UNUSED_VAR(json_str);
    return 0;
}