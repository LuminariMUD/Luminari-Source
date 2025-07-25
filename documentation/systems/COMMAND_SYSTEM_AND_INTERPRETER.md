# LuminariMUD Command System and Interpreter

## Overview

The LuminariMUD command system provides a flexible, extensible framework for processing player input and executing game commands. Built around a central interpreter that parses user input, validates permissions, and dispatches commands to appropriate handlers, the system supports complex command structures, aliases, and social interactions.

## Architecture Overview

### Core Components

#### 1. Command Interpreter (`interpreter.c`)
The main command processing engine that:
- Parses player input into commands and arguments
- Validates command permissions and prerequisites
- Dispatches commands to appropriate handler functions
- Manages command aliases and abbreviations
- Handles special cases like social commands and emotes

#### 2. Command Table (`cmd_info[]`)
A comprehensive table defining all available commands:
```c
struct command_info cmd_info[] = {
  { "RESERVED", 0, 0, 0, 0 },  // Reserved entry
  { "north", POS_STANDING, do_move, 0, SCMD_NORTH },
  { "east", POS_STANDING, do_move, 0, SCMD_EAST },
  { "south", POS_STANDING, do_move, 0, SCMD_SOUTH },
  { "west", POS_STANDING, do_move, 0, SCMD_WEST },
  { "up", POS_STANDING, do_move, 0, SCMD_UP },
  { "down", POS_STANDING, do_move, 0, SCMD_DOWN },
  
  { "look", POS_RESTING, do_look, 0, SCMD_LOOK },
  { "examine", POS_RESTING, do_look, 0, SCMD_LOOK },
  { "l", POS_RESTING, do_look, 0, SCMD_LOOK },
  
  { "say", POS_RESTING, do_say, 0, 0 },
  { "'", POS_RESTING, do_say, 0, 0 },
  
  { "tell", POS_DEAD, do_tell, 0, 0 },
  { "reply", POS_DEAD, do_reply, 0, 0 },
  
  // ... hundreds more commands
};
```

#### 3. Command Structure
```c
struct command_info {
  const char *command;     // Command name
  byte minimum_position;   // Required position (sitting, standing, etc.)
  command_func func;       // Function pointer to handler
  sh_int minimum_level;    // Minimum level required
  int subcmd;             // Subcommand identifier
};
```

## Command Processing Flow

### 1. Input Reception
```c
void game_loop(int local_port) {
  // Main game loop processes input from all connected players
  for (d = descriptor_list; d; d = d->next) {
    if (FD_ISSET(d->descriptor, &input_set)) {
      if (process_input(d) < 0) {
        close_socket(d);
      }
    }
  }
}
```

### 2. Command Parsing
```c
void command_interpreter(struct char_data *ch, char *argument) {
  int cmd, length;
  char *line;
  
  // Remove leading spaces
  skip_spaces(&argument);
  
  // Handle empty input
  if (!*argument) {
    return;
  }
  
  // Extract command word
  line = any_one_arg(argument, arg);
  
  // Find command in table
  if ((cmd = find_command(arg)) < 0) {
    // Check for social commands
    if (find_action(arg) >= 0) {
      do_action(ch, argument, cmd, 0);
      return;
    }
    
    send_to_char(ch, "Huh?!?\r\n");
    return;
  }
  
  // Execute command
  if (perform_command(ch, cmd, line)) {
    // Command executed successfully
  }
}
```

### 3. Command Validation
```c
bool perform_command(struct char_data *ch, int cmd, char *argument) {
  // Check if character can use commands
  if (IS_NPC(ch) && cmd > 0) {
    send_to_char(ch, "You are a mob, you can't use commands.\r\n");
    return FALSE;
  }
  
  // Check position requirements
  if (GET_POS(ch) < cmd_info[cmd].minimum_position) {
    switch (GET_POS(ch)) {
      case POS_DEAD:
        send_to_char(ch, "Lie still; you are DEAD!!! :-(\r\n");
        break;
      case POS_INCAP:
      case POS_MORTALLYW:
        send_to_char(ch, "You are in a pretty bad shape, unable to do anything!\r\n");
        break;
      case POS_STUNNED:
        send_to_char(ch, "All you can do right now is think about the stars!\r\n");
        break;
      case POS_SLEEPING:
        send_to_char(ch, "In your dreams, or what?\r\n");
        break;
      case POS_RESTING:
        send_to_char(ch, "Nah... You feel too relaxed to do that..\r\n");
        break;
      case POS_SITTING:
        send_to_char(ch, "Maybe you should get on your feet first?\r\n");
        break;
      case POS_FIGHTING:
        send_to_char(ch, "No way!  You're fighting for your life!\r\n");
        break;
    }
    return FALSE;
  }
  
  // Check level requirements
  if (GET_LEVEL(ch) < cmd_info[cmd].minimum_level) {
    send_to_char(ch, "You don't have enough experience to use that command.\r\n");
    return FALSE;
  }
  
  // Execute the command
  ((*cmd_info[cmd].command_pointer) (ch, argument, cmd, cmd_info[cmd].subcmd));
  return TRUE;
}
```

## Command Implementation Patterns

### Standard Command Function Signature
```c
// All command functions follow this pattern
ACMD(do_command_name) {
  // ch - the character executing the command
  // argument - the remaining command line after the command word
  // cmd - the command number from the table
  // subcmd - subcommand identifier for shared functions
}

// Example: look command
ACMD(do_look) {
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  int look_type;
  
  if (!ch->desc) return;
  
  if (GET_POS(ch) < POS_SLEEPING) {
    send_to_char(ch, "You can't see anything but stars!\r\n");
    return;
  }
  
  if (AFF_FLAGGED(ch, AFF_BLIND)) {
    send_to_char(ch, "You can't see a damned thing, you're blind!\r\n");
    return;
  }
  
  // Parse arguments
  two_arguments(argument, arg, arg2);
  
  if (subcmd == SCMD_READ) {
    if (!*arg) {
      send_to_char(ch, "Read what?\r\n");
      return;
    }
    // Handle reading objects
    do_look_at_target(ch, arg, TRUE);
  } else {
    if (!*arg) {
      // Look at room
      look_at_room(ch, 0);
    } else {
      // Look at specific target
      do_look_at_target(ch, arg, FALSE);
    }
  }
}
```

### Movement Commands
```c
ACMD(do_move) {
  // subcmd contains direction (SCMD_NORTH, SCMD_EAST, etc.)
  perform_move(ch, subcmd, 1);
}

int perform_move(struct char_data *ch, int dir, int need_specials_check) {
  room_rnum was_in;
  struct follow_type *k, *next;
  
  if (ch == NULL || dir < 0 || dir >= NUM_OF_DIRS || FIGHTING(ch)) {
    return 0;
  }
  
  // Check for valid exit
  if (!EXIT(ch, dir) || EXIT(ch, dir)->to_room == NOWHERE) {
    send_to_char(ch, "Alas, you cannot go that way...\r\n");
    return 0;
  }
  
  // Check if exit is closed
  if (EXIT_FLAGGED(EXIT(ch, dir), EX_CLOSED)) {
    if (EXIT(ch, dir)->keyword) {
      send_to_char(ch, "The %s seems to be closed.\r\n", 
                   fname(EXIT(ch, dir)->keyword));
    } else {
      send_to_char(ch, "It seems to be closed.\r\n");
    }
    return 0;
  }
  
  // Perform the move
  was_in = IN_ROOM(ch);
  char_from_room(ch);
  char_to_room(ch, EXIT(ch, dir)->to_room);
  
  // Show new room
  if (!AFF_FLAGGED(ch, AFF_BLIND)) {
    look_at_room(ch, 0);
  }
  
  // Handle followers
  for (k = ch->followers; k; k = next) {
    next = k->next;
    if ((IN_ROOM(k->follower) == was_in) &&
        (GET_POS(k->follower) >= POS_STANDING)) {
      act("You follow $N.\r\n", FALSE, k->follower, 0, ch, TO_CHAR);
      perform_move(k->follower, dir, 1);
    }
  }
  
  return 1;
}
```

### Communication Commands
```c
ACMD(do_say) {
  skip_spaces(&argument);
  
  if (!*argument) {
    send_to_char(ch, "Yes, but WHAT do you want to say?\r\n");
  } else {
    char buf[MAX_STRING_LENGTH];
    
    snprintf(buf, sizeof(buf), "$n says, '%s'", argument);
    act(buf, FALSE, ch, 0, 0, TO_ROOM);
    
    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT)) {
      send_to_char(ch, "%s", CONFIG_OK);
    } else {
      send_to_char(ch, "You say, '%s'\r\n", argument);
    }
  }
}

ACMD(do_tell) {
  struct char_data *vict = NULL;
  char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
  
  half_chop(argument, buf, buf2);
  
  if (!*buf || !*buf2) {
    send_to_char(ch, "Who do you wish to tell what??\r\n");
  } else if (GET_LEVEL(ch) < LVL_IMMORT) {
    if (!(vict = get_player_vis(ch, buf, NULL, FIND_CHAR_WORLD))) {
      send_to_char(ch, "%s", CONFIG_NOPERSON);
    } else if (is_tell_ok(ch, vict)) {
      perform_tell(ch, vict, buf2);
    }
  } else {
    // Immortal tell - can reach anyone
    if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_WORLD))) {
      send_to_char(ch, "%s", CONFIG_NOPERSON);
    } else if (is_tell_ok(ch, vict)) {
      perform_tell(ch, vict, buf2);
    }
  }
}
```

## Advanced Command Features

### Command Aliases and Abbreviations
```c
int find_command(const char *command) {
  int cmd, length;
  
  length = strlen(command);
  
  // Exact match first
  for (cmd = 0; *cmd_info[cmd].command != '\n'; cmd++) {
    if (!strcmp(cmd_info[cmd].command, command)) {
      return cmd;
    }
  }
  
  // Partial match (abbreviation)
  for (cmd = 0; *cmd_info[cmd].command != '\n'; cmd++) {
    if (!strncmp(cmd_info[cmd].command, command, length)) {
      return cmd;
    }
  }
  
  return -1; // Command not found
}
```

### Social Commands System
```c
// Social commands are defined in a separate table
struct social_messg {
  int act_nr;
  int hide;
  int min_victim_position;
  
  // Messages for different scenarios
  char *char_no_arg;      // To character when no argument
  char *others_no_arg;    // To others when no argument
  char *char_found;       // To character when target found
  char *others_found;     // To others when target found
  char *vict_found;       // To victim when targeted
  char *char_body_found;  // To character when targeting body part
  char *others_body_found; // To others when targeting body part
  char *vict_body_found;  // To victim when targeting body part
  char *char_obj_found;   // To character when targeting object
  char *others_obj_found; // To others when targeting object
  char *char_auto;        // To character when targeting self
  char *others_auto;      // To others when targeting self
};

ACMD(do_action) {
  int act_nr;
  struct social_messg *action;
  struct char_data *vict = NULL;
  struct obj_data *targ_obj = NULL;
  char *bodypart;
  
  if ((act_nr = find_action(argument)) < 0) {
    send_to_char(ch, "That action is not supported.\r\n");
    return;
  }
  
  action = &soc_mess_list[act_nr];
  
  if (!argument || !*argument) {
    // No argument - general social
    send_to_char(ch, "%s\r\n", action->char_no_arg);
    act(action->others_no_arg, action->hide, ch, 0, 0, TO_ROOM);
    return;
  }
  
  // Parse target and optional body part
  two_arguments(argument, buf, buf2);
  
  if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM))) {
    // Try to find object
    if (!(targ_obj = get_obj_in_list_vis(ch, buf, NULL, ch->carrying))) {
      if (!(targ_obj = get_obj_in_list_vis(ch, buf, NULL, world[IN_ROOM(ch)].contents))) {
        send_to_char(ch, "They aren't here.\r\n");
        return;
      }
    }
  }
  
  if (vict == ch) {
    // Targeting self
    send_to_char(ch, "%s\r\n", action->char_auto);
    act(action->others_auto, action->hide, ch, 0, 0, TO_ROOM);
  } else if (vict) {
    // Targeting another character
    if (*buf2) {
      // With body part
      bodypart = buf2;
      act(action->char_body_found, 0, ch, (struct obj_data *)bodypart, vict, TO_CHAR);
      act(action->others_body_found, action->hide, ch, (struct obj_data *)bodypart, vict, TO_NOTVICT);
      act(action->vict_body_found, action->hide, ch, (struct obj_data *)bodypart, vict, TO_VICT);
    } else {
      // Without body part
      act(action->char_found, 0, ch, 0, vict, TO_CHAR);
      act(action->others_found, action->hide, ch, 0, vict, TO_NOTVICT);
      act(action->vict_found, action->hide, ch, 0, vict, TO_VICT);
    }
  } else {
    // Targeting object
    act(action->char_obj_found, 0, ch, targ_obj, 0, TO_CHAR);
    act(action->others_obj_found, action->hide, ch, targ_obj, 0, TO_ROOM);
  }
}
```

### Command Queuing and Delays
```c
// Some commands have delays or queues
ACMD(do_cast) {
  struct char_data *tch = NULL;
  struct obj_data *tobj = NULL;
  char *s, *t;
  int mana, spellnum, i, target = 0;
  
  // Check if already casting
  if (CASTING(ch)) {
    send_to_char(ch, "You are already casting a spell!\r\n");
    return;
  }
  
  // Parse spell name and target
  s = strtok(argument, "'");
  if (s == NULL) {
    send_to_char(ch, "Cast what where?\r\n");
    return;
  }
  
  s = strtok(NULL, "'");
  if (s == NULL) {
    send_to_char(ch, "Spell names must be enclosed in the Holy Magic Symbols: '\r\n");
    return;
  }
  
  t = strtok(NULL, "\0");
  
  // Find spell
  spellnum = find_skill_num(s);
  if (spellnum < 1 || spellnum > MAX_SPELLS) {
    send_to_char(ch, "Cast what?!?\r\n");
    return;
  }
  
  // Check if character knows the spell
  if (GET_SKILL(ch, spellnum) == 0) {
    send_to_char(ch, "You are unfamiliar with that spell.\r\n");
    return;
  }
  
  // Check mana cost
  mana = mag_manacost(ch, spellnum);
  if (GET_MANA(ch) < mana) {
    send_to_char(ch, "You haven't the energy to cast that spell!\r\n");
    return;
  }
  
  // Start casting process
  CASTING_SPELLNUM(ch) = spellnum;
  CASTING_TIME(ch) = spell_info[spellnum].cast_time;
  
  if (t != NULL) {
    one_argument(t, arg);
    if (spell_info[spellnum].targets & TAR_CHAR_ROOM) {
      if ((tch = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)) != NULL) {
        CASTING_TARGET(ch) = (void *)tch;
        CASTING_TTYPE(ch) = TAR_CHAR_ROOM;
      }
    }
  }
  
  // Begin casting
  send_to_char(ch, "You begin casting %s.\r\n", spell_info[spellnum].name);
  act("$n begins casting a spell.", TRUE, ch, 0, 0, TO_ROOM);
  
  // Set up casting event
  attach_mud_event(new_mud_event(eCAST, ch, CASTING_TIME(ch) * PULSE_CAST), ch);
}
```

## Command Security and Validation

### Permission Checking
```c
bool has_command_permission(struct char_data *ch, int cmd) {
  // Check basic level requirement
  if (GET_LEVEL(ch) < cmd_info[cmd].minimum_level) {
    return FALSE;
  }
  
  // Check special restrictions
  if (PLR_FLAGGED(ch, PLR_FROZEN) && cmd_info[cmd].minimum_level < LVL_IMPL) {
    return FALSE;
  }
  
  // Check if command is disabled
  if (cmd_info[cmd].minimum_level == LVL_IMPL + 1) {
    return FALSE; // Command disabled
  }
  
  return TRUE;
}
```

### Input Sanitization
```c
void sanitize_input(char *input) {
  char *src, *dest;
  
  // Remove control characters and excessive whitespace
  for (src = dest = input; *src; src++) {
    if (*src >= ' ' && *src <= '~') {
      *dest++ = *src;
    } else if (*src == '\t') {
      *dest++ = ' '; // Convert tabs to spaces
    }
    // Skip other control characters
  }
  *dest = '\0';
  
  // Trim trailing whitespace
  while (dest > input && *(dest - 1) == ' ') {
    *--dest = '\0';
  }
}
```

## Performance Considerations

### Command Lookup Optimization
```c
// Hash table for faster command lookup
#define COMMAND_HASH_SIZE 256
static int command_hash[COMMAND_HASH_SIZE];

void init_command_hash() {
  int cmd;
  
  // Initialize hash table
  for (cmd = 0; cmd < COMMAND_HASH_SIZE; cmd++) {
    command_hash[cmd] = -1;
  }
  
  // Build hash table
  for (cmd = 0; *cmd_info[cmd].command != '\n'; cmd++) {
    int hash = hash_string(cmd_info[cmd].command) % COMMAND_HASH_SIZE;
    // Handle collisions with chaining
    // ... implementation details
  }
}

int find_command_fast(const char *command) {
  int hash = hash_string(command) % COMMAND_HASH_SIZE;
  int cmd = command_hash[hash];
  
  while (cmd != -1) {
    if (!strcmp(cmd_info[cmd].command, command)) {
      return cmd;
    }
    // Follow collision chain
    cmd = cmd_info[cmd].next_hash;
  }
  
  return -1; // Not found
}
```

### Command Frequency Analysis
```c
// Track command usage for optimization
struct command_stats {
  int usage_count;
  long total_time;
  long max_time;
};

static struct command_stats cmd_stats[MAX_COMMANDS];

void log_command_performance(int cmd, long execution_time) {
  cmd_stats[cmd].usage_count++;
  cmd_stats[cmd].total_time += execution_time;
  if (execution_time > cmd_stats[cmd].max_time) {
    cmd_stats[cmd].max_time = execution_time;
  }
}
```

---

*This documentation covers the core command system architecture. For specific command implementations and advanced features, refer to the individual source files and the [Developer Guide](DEVELOPER_GUIDE_AND_API.md).*
