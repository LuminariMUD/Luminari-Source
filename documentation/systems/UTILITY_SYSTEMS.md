# LuminariMUD Utility Systems

## Overview

LuminariMUD includes several utility systems that provide essential infrastructure for game operations, player communication, data management, and administrative functions. These systems work together to create a comprehensive gaming environment with robust logging, communication channels, player organizations, and maintenance tools.

## Logging System

### Core Logging Architecture

#### Log Types and Levels
```c
// Log types defined in structs.h
#define SYSLOG    0    // System messages
#define ZONELOG   1    // Zone-related messages  
#define DEATHLOG  2    // Player deaths
#define MISCLOG   3    // Miscellaneous events
#define WIZLOG    4    // Wizard/immortal actions
#define IMLOG     5    // Immortal communications
#define ERRLOG    6    // Error messages
#define CONNLOG   7    // Connection events
#define RESTLOG   8    // Restoration events

// Log levels
#define BRF  0    // Brief
#define NRM  1    // Normal  
#define CMP  2    // Complete
```

#### Logging Functions
```c
// Basic logging function
void basic_mud_log(const char *format, ...) {
  va_list args;
  time_t ct = time(0);
  char *time_s = asctime(localtime(&ct));
  
  time_s[strlen(time_s) - 1] = '\0'; // Remove newline
  
  va_start(args, format);
  
  // Log to file
  if (logfile != NULL) {
    fprintf(logfile, "%-15.15s :: ", time_s + 4);
    vfprintf(logfile, format, args);
    fprintf(logfile, "\n");
    fflush(logfile);
  }
  
  // Log to stderr for debugging
  if (scheck) {
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
  }
  
  va_end(args);
}

// Advanced logging with categories
void mudlog(int type, int level, int file, const char *str, ...) {
  char buffer[MAX_STRING_LENGTH];
  struct descriptor_data *i;
  va_list args;
  
  if (str == NULL) return;
  if (file) basic_mud_log("SYSERR: %s", str);
  if (level < 0) return;
  
  va_start(args, str);
  vsnprintf(buffer, sizeof(buffer), str, args);
  va_end(args);
  
  // Send to online immortals
  for (i = descriptor_list; i; i = i->next) {
    if (STATE(i) != CON_PLAYING || IS_NPC(i->character)) continue;
    if (GET_LEVEL(i->character) < level) continue;
    if (PLR_FLAGGED(i->character, PLR_WRITING)) continue;
    
    // Check if player wants to see this log type
    if (type > 0 && !PRF_FLAGGED(i->character, PRF_LOG1 + type - 1)) continue;
    
    send_to_char(i->character, "%s[ %s ]%s\r\n", 
                 CCGRN(i->character, C_NRM), buffer, CCNRM(i->character, C_NRM));
  }
}
```

### Specialized Logging

#### Performance Logging
```c
void log_performance_data(const char *function, long execution_time) {
  static FILE *perf_log = NULL;
  
  if (!perf_log) {
    perf_log = fopen("../log/performance.log", "a");
    if (!perf_log) return;
  }
  
  fprintf(perf_log, "%ld: %s took %ld microseconds\n", 
          time(NULL), function, execution_time);
  fflush(perf_log);
}

// Macro for easy performance logging
#define LOG_PERFORMANCE(func_call) do { \
  struct timeval start, end; \
  gettimeofday(&start, NULL); \
  func_call; \
  gettimeofday(&end, NULL); \
  long diff = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec); \
  log_performance_data(#func_call, diff); \
} while(0)
```

#### Security Logging
```c
void log_security_event(struct char_data *ch, const char *event, const char *details) {
  FILE *sec_log = fopen("../log/security.log", "a");
  if (!sec_log) return;
  
  fprintf(sec_log, "[%ld] %s (%s): %s - %s\n",
          time(NULL),
          ch ? GET_NAME(ch) : "SYSTEM",
          ch && ch->desc ? ch->desc->host : "localhost",
          event,
          details);
  fclose(sec_log);
  
  // Also send to security channel
  mudlog(WIZLOG, LVL_IMMORT, TRUE, "SECURITY: %s %s - %s",
         ch ? GET_NAME(ch) : "SYSTEM", event, details);
}
```

## Communication Systems

### Channel System

#### Channel Structure
```c
struct channel_data {
  char *name;           // Channel name
  char *color_code;     // Color code for channel
  int min_level;        // Minimum level to use
  int cost;             // Cost in movement points
  bitvector_t flags;    // Channel flags
  bool (*can_use)(struct char_data *ch); // Permission function
};

// Channel definitions
struct channel_data channels[] = {
  {"gossip", CCYEL, 1, 0, CHAN_GLOBAL, can_use_gossip},
  {"auction", CCMAG, 1, 0, CHAN_GLOBAL, can_use_auction},
  {"grats", CCGRN, 1, 0, CHAN_GLOBAL, can_use_grats},
  {"ooc", CCCYN, 1, 0, CHAN_GLOBAL, can_use_ooc},
  {"newbie", CCBLU, 1, 0, CHAN_GLOBAL | CHAN_NEWBIE, can_use_newbie},
  {"immtalk", CCRED, LVL_IMMORT, 0, CHAN_IMM, can_use_immtalk},
  {NULL, NULL, 0, 0, 0, NULL}
};
```

#### Channel Communication
```c
ACMD(do_gen_comm) {
  struct descriptor_data *i;
  char color_on[24];
  struct channel_data *chan = &channels[subcmd];
  
  // Remove leading spaces
  skip_spaces(&argument);
  
  // Check if channel is disabled
  if (!chan->can_use || !chan->can_use(ch)) {
    send_to_char(ch, "You cannot use that channel.\r\n");
    return;
  }
  
  // Check for argument
  if (!*argument) {
    send_to_char(ch, "%s what??\r\n", chan->name);
    return;
  }
  
  // Check movement cost
  if (GET_MOVE(ch) < chan->cost) {
    send_to_char(ch, "You are too exhausted to use %s.\r\n", chan->name);
    return;
  }
  
  GET_MOVE(ch) -= chan->cost;
  
  // Set up color
  strcpy(color_on, chan->color_code);
  
  // Send to all eligible players
  for (i = descriptor_list; i; i = i->next) {
    if (STATE(i) == CON_PLAYING && i != ch->desc && i->character &&
        !PLR_FLAGGED(i->character, PLR_WRITING) &&
        !ROOM_FLAGGED(IN_ROOM(i->character), ROOM_SOUNDPROOF) &&
        GET_LEVEL(i->character) >= chan->min_level &&
        can_hear_channel(i->character, subcmd)) {
      
      send_to_char(i->character, "%s[%s] %s: %s%s\r\n",
                   color_on, chan->name, GET_NAME(ch), argument, CCNRM(i->character, C_NRM));
    }
  }
  
  // Echo to sender
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT)) {
    send_to_char(ch, "%s", CONFIG_OK);
  } else {
    send_to_char(ch, "%s[%s] You: %s%s\r\n",
                 color_on, chan->name, argument, CCNRM(ch, C_NRM));
  }
}
```

### Mail System

#### Mail Structure
```c
struct mail_index_type {
  long position;        // Position in mail file
  long recipient;       // Recipient ID
  long sender;          // Sender ID  
  time_t mail_time;     // When sent
  char *subject;        // Mail subject
  bool read;            // Has been read
};

struct mail_data {
  long to;              // Recipient ID
  long from;            // Sender ID
  time_t mail_time;     // Timestamp
  char *subject;        // Subject line
  char *message;        // Message body
};
```

#### Mail Functions
```c
// Send mail
void send_mail(long to, long from, const char *subject, const char *message) {
  FILE *mail_file;
  struct mail_index_type mail_entry;
  long position;
  
  // Open mail file
  mail_file = fopen(MAIL_FILE, "a");
  if (!mail_file) {
    log("SYSERR: Unable to open mail file for writing");
    return;
  }
  
  // Get current position
  position = ftell(mail_file);
  
  // Write mail data
  fprintf(mail_file, "%ld %ld %ld\n%s~\n%s~\n",
          to, from, (long)time(0), subject, message);
  fclose(mail_file);
  
  // Update mail index
  mail_entry.position = position;
  mail_entry.recipient = to;
  mail_entry.sender = from;
  mail_entry.mail_time = time(0);
  mail_entry.subject = strdup(subject);
  mail_entry.read = FALSE;
  
  add_mail_index(&mail_entry);
  
  // Notify recipient if online
  struct char_data *recipient = find_char_by_id(to);
  if (recipient && recipient->desc) {
    send_to_char(recipient, "\r\n%sYou have new mail!%s\r\n",
                 CCYEL(recipient, C_NRM), CCNRM(recipient, C_NRM));
  }
}

// Read mail
ACMD(do_mail) {
  char arg[MAX_INPUT_LENGTH];
  int mail_number;
  struct mail_data *mail;
  
  one_argument(argument, arg);
  
  if (!*arg) {
    list_mail(ch);
    return;
  }
  
  if (!str_cmp(arg, "read")) {
    one_argument(argument, arg); // Get mail number
    mail_number = atoi(arg);
    
    if (mail_number < 1) {
      send_to_char(ch, "Which mail do you want to read?\r\n");
      return;
    }
    
    mail = get_mail(GET_IDNUM(ch), mail_number - 1);
    if (!mail) {
      send_to_char(ch, "You don't have that many messages.\r\n");
      return;
    }
    
    // Display mail
    send_to_char(ch, "From: %s\r\nSubject: %s\r\nDate: %s\r\n%s\r\n",
                 get_name_by_id(mail->from),
                 mail->subject,
                 ctime(&mail->mail_time),
                 mail->message);
    
    // Mark as read
    mark_mail_read(GET_IDNUM(ch), mail_number - 1);
  }
}
```

## Event System

### Event Structure
```c
struct mud_event_data {
  event_id iId;                    // Event ID
  void *pStruct;                   // Event data structure
  EVENTFUNC(*func);                // Event function
  char *sVariables;                // Event variables
  long lVariables;                 // Numeric variables
  struct mud_event_data *next;     // Next event in list
};

// Event types
typedef enum {
  eNULL = 0,
  eCOMBAT_ROUND,
  eREGEN,
  eCAST,
  eMOVEMENT,
  eAUTO_SAVE,
  eZONE_RESET,
  eMAX_EVENT
} event_id;
```

### Event Management
```c
// Create new event
struct mud_event_data *new_mud_event(event_id iId, void *pStruct, long lDelay) {
  struct mud_event_data *new_event;
  
  CREATE(new_event, struct mud_event_data, 1);
  new_event->iId = iId;
  new_event->pStruct = pStruct;
  new_event->lVariables = lDelay;
  new_event->func = get_event_func(iId);
  
  return new_event;
}

// Attach event to character
void attach_mud_event(struct mud_event_data *pMudEvent, struct char_data *ch) {
  if (!pMudEvent || !ch) return;
  
  // Add to character's event list
  pMudEvent->next = ch->events;
  ch->events = pMudEvent;
  
  // Schedule event
  schedule_event(pMudEvent);
}

// Process events
void process_events() {
  struct mud_event_data *event, *next_event;
  
  for (event = global_event_list; event; event = next_event) {
    next_event = event->next;
    
    if (--event->lVariables <= 0) {
      // Execute event
      long result = event->func(event->pStruct);
      
      if (result == 0) {
        // Event finished, remove it
        remove_event(event);
      } else {
        // Reschedule event
        event->lVariables = result;
      }
    }
  }
}
```

### Common Event Functions
```c
// Combat round event
EVENTFUNC(combat_round_event) {
  struct char_data *ch = (struct char_data *)event_obj;
  
  if (!ch || !FIGHTING(ch)) {
    return 0; // End event
  }
  
  // Perform combat round
  perform_combat_round(ch);
  
  // Continue combat
  return PULSE_COMBAT;
}

// Regeneration event
EVENTFUNC(regen_event) {
  struct char_data *ch = (struct char_data *)event_obj;
  
  if (!ch) return 0;
  
  // Regenerate hit points
  if (GET_HIT(ch) < GET_MAX_HIT(ch)) {
    GET_HIT(ch) = MIN(GET_MAX_HIT(ch), GET_HIT(ch) + hit_gain(ch));
  }
  
  // Regenerate mana
  if (GET_MANA(ch) < GET_MAX_MANA(ch)) {
    GET_MANA(ch) = MIN(GET_MAX_MANA(ch), GET_MANA(ch) + mana_gain(ch));
  }
  
  // Regenerate movement
  if (GET_MOVE(ch) < GET_MAX_MOVE(ch)) {
    GET_MOVE(ch) = MIN(GET_MAX_MOVE(ch), GET_MOVE(ch) + move_gain(ch));
  }
  
  // Continue regeneration
  return PULSE_REGEN;
}
```

## Board System

### Board Structure
```c
struct board_data {
  int vnum;                    // Board virtual number
  char *name;                  // Board name
  char *description;           // Board description
  int read_level;              // Level required to read
  int write_level;             // Level required to write
  int remove_level;            // Level required to remove messages
  char *filename;              // Board file name
  int max_messages;            // Maximum messages
  struct message_data *messages; // Message list
};

struct message_data {
  int number;                  // Message number
  char *author;                // Author name
  char *subject;               // Message subject
  char *message;               // Message text
  time_t timestamp;            // When posted
  int level;                   // Author level when posted
  struct message_data *next;   // Next message
};
```

### Board Operations
```c
// Read board message
ACMD(do_look_at_board) {
  struct board_data *board;
  struct message_data *msg;
  int msg_num;
  char arg[MAX_INPUT_LENGTH];
  
  one_argument(argument, arg);
  
  // Find board in room
  board = find_board_in_room(IN_ROOM(ch));
  if (!board) {
    send_to_char(ch, "There is no board here.\r\n");
    return;
  }
  
  // Check read permission
  if (GET_LEVEL(ch) < board->read_level) {
    send_to_char(ch, "You are not high enough level to read this board.\r\n");
    return;
  }
  
  if (!*arg) {
    // List all messages
    send_to_char(ch, "This is %s.\r\n%s\r\n", board->name, board->description);
    send_to_char(ch, "Usage: READ <message #>, WRITE <subject>, REMOVE <message #>\r\n\r\n");
    
    if (!board->messages) {
      send_to_char(ch, "The board is empty.\r\n");
      return;
    }
    
    send_to_char(ch, "Num  Author       Subject\r\n");
    send_to_char(ch, "---  ----------   -------\r\n");
    
    for (msg = board->messages; msg; msg = msg->next) {
      send_to_char(ch, "%3d  %-10s   %s\r\n",
                   msg->number, msg->author, msg->subject);
    }
  } else {
    // Read specific message
    msg_num = atoi(arg);
    msg = find_board_message(board, msg_num);
    
    if (!msg) {
      send_to_char(ch, "That message doesn't exist.\r\n");
      return;
    }
    
    send_to_char(ch, "Message %d: %s\r\n", msg->number, msg->subject);
    send_to_char(ch, "Author: %s, Date: %s\r\n", msg->author, ctime(&msg->timestamp));
    send_to_char(ch, "%s\r\n", msg->message);
  }
}

// Write board message
ACMD(do_write_board) {
  struct board_data *board;
  char subject[MAX_INPUT_LENGTH];
  
  skip_spaces(&argument);
  strcpy(subject, argument);
  
  board = find_board_in_room(IN_ROOM(ch));
  if (!board) {
    send_to_char(ch, "There is no board here.\r\n");
    return;
  }
  
  if (GET_LEVEL(ch) < board->write_level) {
    send_to_char(ch, "You are not high enough level to write on this board.\r\n");
    return;
  }
  
  if (!*subject) {
    send_to_char(ch, "You must specify a subject.\r\n");
    return;
  }
  
  // Start writing mode
  send_to_char(ch, "Write your message. End with @ on a new line.\r\n");
  act("$n starts writing on the board.", TRUE, ch, 0, 0, TO_ROOM);
  
  SET_BIT(PLR_FLAGS(ch), PLR_WRITING);
  ch->desc->str = &ch->desc->mail_to;
  ch->desc->max_str = MAX_MESSAGE_LENGTH;
  
  // Store board and subject for later
  ch->desc->board = board;
  ch->desc->board_subject = strdup(subject);
}
```

## Clan System

### Clan Structure
```c
struct clan_data {
  int id;                      // Clan ID
  char *name;                  // Clan name
  char *description;           // Clan description
  char *leader;                // Clan leader name
  int members;                 // Number of members
  int power;                   // Clan power/influence
  int treasury;                // Clan treasury
  room_vnum hall;              // Clan hall room
  bitvector_t flags;           // Clan flags
  struct clan_member *member_list; // Member list
};

struct clan_member {
  long player_id;              // Player ID
  char *name;                  // Player name
  int rank;                    // Clan rank
  time_t joined;               // When joined
  struct clan_member *next;    // Next member
};
```

### Clan Operations
```c
// Join clan
ACMD(do_clan_join) {
  struct clan_data *clan;
  char clan_name[MAX_INPUT_LENGTH];
  
  one_argument(argument, clan_name);
  
  if (GET_CLAN(ch) != CLAN_NONE) {
    send_to_char(ch, "You are already in a clan.\r\n");
    return;
  }
  
  clan = find_clan_by_name(clan_name);
  if (!clan) {
    send_to_char(ch, "That clan doesn't exist.\r\n");
    return;
  }
  
  // Check if clan is accepting members
  if (IS_SET(clan->flags, CLAN_CLOSED)) {
    send_to_char(ch, "That clan is not accepting new members.\r\n");
    return;
  }
  
  // Add player to clan
  add_clan_member(clan, GET_IDNUM(ch), GET_NAME(ch), CLAN_RANK_MEMBER);
  GET_CLAN(ch) = clan->id;
  GET_CLAN_RANK(ch) = CLAN_RANK_MEMBER;
  
  send_to_char(ch, "You have joined %s!\r\n", clan->name);
  
  // Notify clan members
  clan_echo(clan, "%s has joined the clan!", GET_NAME(ch));
}
```

## Utility Commands

### System Information
```c
ACMD(do_uptime) {
  time_t uptime = time(0) - boot_time;
  int days, hours, minutes;
  
  days = uptime / 86400;
  hours = (uptime % 86400) / 3600;
  minutes = (uptime % 3600) / 60;
  
  send_to_char(ch, "Server uptime: %d day%s, %d hour%s, %d minute%s\r\n",
               days, days == 1 ? "" : "s",
               hours, hours == 1 ? "" : "s", 
               minutes, minutes == 1 ? "" : "s");
  
  send_to_char(ch, "Boot time: %s", ctime(&boot_time));
  send_to_char(ch, "Players online: %d\r\n", count_playing_chars());
  send_to_char(ch, "Maximum players today: %d\r\n", max_players_today);
}

ACMD(do_memory) {
  struct char_data *ch_iter;
  struct obj_data *obj_iter;
  int chars = 0, objs = 0, rooms = 0;
  
  // Count characters
  for (ch_iter = character_list; ch_iter; ch_iter = ch_iter->next) {
    chars++;
  }
  
  // Count objects
  for (obj_iter = object_list; obj_iter; obj_iter = obj_iter->next) {
    objs++;
  }
  
  // Count rooms
  rooms = top_of_world + 1;
  
  send_to_char(ch, "Memory usage:\r\n");
  send_to_char(ch, "Characters: %d\r\n", chars);
  send_to_char(ch, "Objects: %d\r\n", objs);
  send_to_char(ch, "Rooms: %d\r\n", rooms);
  send_to_char(ch, "Zones: %d\r\n", top_of_zone_table + 1);
  send_to_char(ch, "Mobiles: %d prototypes\r\n", top_of_mobt + 1);
  send_to_char(ch, "Objects: %d prototypes\r\n", top_of_objt + 1);
}
```

### Maintenance Tools
```c
ACMD(do_purge_players) {
  int days_inactive, purged = 0;
  char arg[MAX_INPUT_LENGTH];
  
  one_argument(argument, arg);
  days_inactive = atoi(arg);
  
  if (days_inactive < 30) {
    send_to_char(ch, "Minimum inactive period is 30 days.\r\n");
    return;
  }
  
  send_to_char(ch, "Purging players inactive for %d+ days...\r\n", days_inactive);
  
  // This would iterate through player files and remove inactive ones
  purged = purge_inactive_players(days_inactive);
  
  send_to_char(ch, "Purged %d inactive players.\r\n", purged);
  mudlog(WIZLOG, LVL_IMPL, TRUE, "%s purged %d inactive players (%d+ days)",
         GET_NAME(ch), purged, days_inactive);
}
```

---

*This documentation covers the core utility systems in LuminariMUD. For specific implementation details and configuration options, refer to the individual source files and the [Developer Guide](DEVELOPER_GUIDE_AND_API.md).*
