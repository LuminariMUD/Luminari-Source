# LuminariMUD DG Scripting System

## Overview

The DG (DikuMUD Scripting) system in LuminariMUD provides a powerful, event-driven scripting language that allows builders to create dynamic, interactive content. Scripts can be attached to mobiles (NPCs), objects, and rooms to respond to player actions, create complex behaviors, and implement sophisticated game mechanics.

## Core Architecture

### Script Components

#### 1. Triggers (`struct trig_data`)
Triggers are the fundamental units of the scripting system:
```c
struct trig_data {
  trig_vnum vnum;           // Virtual number
  byte attach_type;         // MOB_TRIGGER, OBJ_TRIGGER, WLD_TRIGGER
  byte data_type;           // Trigger type (command, speech, etc.)
  char *name;               // Trigger name
  long trigger_type;        // Specific trigger flags
  char *arglist;            // Trigger arguments
  int narg;                 // Numeric argument
  struct cmdlist_element *cmdlist; // Command list
  int curr_state;           // Current execution state
  int wait_event;           // Wait event ID
  struct trig_var_data *var_list; // Local variables
  struct trig_data *next;   // Next trigger in list
};
```

#### 2. Variables (`struct trig_var_data`)
Script variables store data during execution:
```c
struct trig_var_data {
  char *name;               // Variable name
  char *value;              // Variable value
  long context;             // Variable context/scope
  struct trig_var_data *next; // Next variable
};
```

#### 3. Command Lists (`struct cmdlist_element`)
Script commands are stored as linked lists:
```c
struct cmdlist_element {
  char *cmd;                // Command string
  struct cmdlist_element *original; // Original command (for loops)
  struct cmdlist_element *next;     // Next command
};
```

## Trigger Types and Events

### Mobile Triggers

#### Command Triggers
Respond to specific commands entered by players:
```
Trigger Type: Command
Argument: "say hello"
Numeric Arg: 100 (percentage chance)

Script:
if %actor.name% == Gandalf
  say Greetings, %actor.name%! Welcome to my tower.
  emote bows respectfully.
else
  say Hello there, traveler.
end
```

#### Speech Triggers
Activate when specific words are spoken:
```
Trigger Type: Speech
Argument: "help quest"
Numeric Arg: 100

Script:
say You seek a quest? I have just the thing!
wait 2 sec
say Bring me the Crystal of Power from the Dark Cave.
set quest_given 1
remote quest_given %actor.id%
```

#### Act Triggers
Respond to specific actions performed in the room:
```
Trigger Type: Act
Argument: "enters"
Numeric Arg: 100

Script:
if %actor.level% < 10
  say This area is too dangerous for you, young one.
  %teleport% %actor% 3001
else
  say Welcome, brave adventurer.
end
```

#### Fight Triggers
Activate during combat situations:
```
Trigger Type: Fight
Argument: ""
Numeric Arg: 25 (25% chance per round)

Script:
switch %self.hitp%
  case 1 to 50
    say You fight well, but I have tricks yet!
    cast 'heal' self
    break
  case 51 to 100
    emote growls menacingly.
    break
  default
    break
done
```

### Object Triggers

#### Get Triggers
Activate when object is picked up:
```
Trigger Type: Get
Argument: ""
Numeric Arg: 100

Script:
%send% %actor% The sword feels warm in your hands.
%echoaround% %actor% %actor.name%'s sword begins to glow softly.
set wielder %actor.id%
remote wielder %self.id%
```

#### Drop Triggers
Activate when object is dropped:
```
Trigger Type: Drop
Argument: ""
Numeric Arg: 100

Script:
%send% %actor% The sword's glow fades as you release it.
%echoaround% %actor% The sword's glow dims and dies.
unset wielder
remote wielder %self.id%
```

#### Wear/Remove Triggers
Activate when equipment is worn or removed:
```
Trigger Type: Wear
Argument: ""
Numeric Arg: 100

Script:
%send% %actor% You feel the ring's magic coursing through you.
%actor.str% += 2
%actor.dex% += 2
set enhanced 1
remote enhanced %actor.id%
```

### Room Triggers

#### Enter Triggers
Activate when someone enters the room:
```
Trigger Type: Enter
Argument: ""
Numeric Arg: 100

Script:
if %actor.level% >= 20
  %echo% The ancient chamber recognizes a worthy hero.
  %load% obj 1234
  %echo% A treasure chest materializes in the center of the room.
else
  %echo% The chamber remains silent and empty.
end
```

#### Command Triggers
Respond to commands entered in the room:
```
Trigger Type: Command
Argument: "pull lever"
Numeric Arg: 100

Script:
%echo% You hear the grinding of ancient gears.
wait 3 sec
%echo% A secret door opens in the north wall!
%door% 1234 north flags a
wait 30 sec
%echo% The secret door slowly closes.
%door% 1234 north flags abc
```

## Script Commands

### Basic Commands

#### Output Commands
```
%send% <target> <message>        - Send message to specific character
%echo% <message>                 - Send message to entire room
%echoaround% <target> <message>  - Send message to room except target
%zoneecho% <message>             - Send message to entire zone
```

#### Character Manipulation
```
%teleport% <target> <room>       - Move character to room
%force% <target> <command>       - Force character to execute command
%damage% <target> <amount>       - Deal damage to character
%heal% <target> <amount>         - Heal character
```

#### Object Manipulation
```
%load% obj <vnum>                - Load object to room
%load% mob <vnum>                - Load mobile to room
%purge% <target>                 - Remove object/mobile
%give% <object> <target>         - Give object to character
```

#### Room Manipulation
```
%door% <room> <direction> <flags> - Modify door flags
%teleport% <target> <room>        - Transport character
%at% <room> <command>             - Execute command at different room
```

### Control Flow

#### Conditional Statements
```
if <condition>
  <commands>
elseif <condition>
  <commands>
else
  <commands>
end
```

#### Loops
```
while <condition>
  <commands>
done

foreach <variable> <list>
  <commands>
done
```

#### Switch Statements
```
switch <variable>
  case <value1>
    <commands>
    break
  case <value2> to <value3>
    <commands>
    break
  default
    <commands>
    break
done
```

### Variables and Context

#### Variable Types
```
%actor%          - The character who triggered the script
%self%           - The mobile/object/room running the script
%random.X%       - Random number from 1 to X
%time.hour%      - Current game hour
%time.day%       - Current game day
```

#### Variable Operations
```
set <variable> <value>           - Set variable value
unset <variable>                 - Remove variable
eval <variable> <expression>     - Evaluate mathematical expression
remote <variable> <id>           - Set remote variable on other entity
```

#### Context Variables
```
%actor.name%     - Actor's name
%actor.level%    - Actor's level
%actor.class%    - Actor's class
%actor.race%     - Actor's race
%actor.room%     - Actor's current room
%actor.hitp%     - Actor's current hit points
%actor.maxhitp%  - Actor's maximum hit points
```

## Advanced Scripting Techniques

### State Management
```
* Trigger: Command "start quest"
if %actor.var(quest_state)% == completed
  say You have already completed this quest.
  halt
elseif %actor.var(quest_state)% == started
  say You are already on this quest.
  halt
else
  say Very well, I will give you the quest.
  remote quest_state started
  remote quest_giver %self.id%
end
```

### Complex Interactions
```
* Trigger: Speech "password"
if %speech% /= secret123
  say That is not the correct password.
  halt
end

say Welcome, agent. Here are your orders.
wait 2 sec
%load% obj 5678
give orders %actor%
say Destroy this message after reading.
```

### Timer-Based Events
```
* Trigger: Enter
%echo% You hear ominous rumbling from deep within the cave.
set timer 0
while %timer% < 10
  wait 6 sec
  eval timer %timer% + 1
  switch %timer%
    case 3
      %echo% The rumbling grows louder.
      break
    case 6
      %echo% Rocks begin to fall from the ceiling!
      break
    case 9
      %echo% The cave is about to collapse!
      break
    case 10
      %echo% The cave collapses! Everyone must flee!
      %teleport% %actor% 3001
      break
  done
done
```

### Multi-Object Coordination
```
* Lever Object Trigger: Command "pull"
%echo% You pull the lever down.
set lever_state pulled
remote lever1_pulled 12345

* Door Room Trigger: Global (checks every 10 seconds)
if %global.lever1_pulled% && %global.lever2_pulled% && %global.lever3_pulled%
  %echo% All three levers have been pulled! The door opens!
  %door% %self.vnum% north flags a
  unset lever1_pulled
  unset lever2_pulled  
  unset lever3_pulled
end
```

## Debugging and Testing

### Debug Commands
```
%debug% <message>                - Output debug message to system log
%halt%                          - Stop script execution immediately
%return% <value>                - Return value from script function
```

### Error Handling
```
* Always check for valid targets
if !%actor%
  %debug% Script triggered with no actor!
  halt
end

* Validate room numbers before teleporting
if %room.vnum% < 1
  %debug% Invalid room number: %room.vnum%
  halt
end
```

### Testing Procedures
```
* Use debug output for tracking
%debug% Quest script started for %actor.name%
set debug_mode 1

if %debug_mode%
  %send% %actor% DEBUG: Setting quest state to 'started'
end

remote quest_state started
```

## Performance Considerations

### Optimization Tips

#### Efficient Variable Usage
```
* Cache frequently used values
set actor_level %actor.level%
set actor_name %actor.name%

* Use local variables for calculations
eval damage_bonus %actor.level% / 5
eval total_damage %base_damage% + %damage_bonus%
```

#### Minimize Remote Operations
```
* Batch remote variable sets
remote quest_state started
remote quest_giver %self.id%
remote quest_time %time.hour%

* Instead of multiple remote calls in loops
```

#### Smart Trigger Usage
```
* Use appropriate trigger types
* Command triggers for specific commands
* Speech triggers for keywords
* Act triggers for general actions
* Timer triggers for periodic events
```

### Memory Management
```
* Clean up variables when done
unset temporary_var
unset calculation_result

* Use context-appropriate variable scopes
* Global variables for persistent data
* Local variables for temporary calculations
```

## Common Patterns and Examples

### Quest System Implementation
```
* Quest Giver Mobile
Trigger: Command "quest"
if %actor.var(hero_quest)% == completed
  say You have proven yourself a true hero!
  %load% obj 9999
  give hero_medal %actor%
elseif %actor.var(hero_quest)% == started
  if %actor.var(dragon_killed)%
    say Excellent! You have slain the dragon!
    remote hero_quest completed
    %actor.exp% += 5000
  else
    say The dragon still terrorizes our village.
  end
else
  say Will you help us slay the dragon?
  remote hero_quest started
  remote quest_giver %self.id%
end
```

### Dynamic Environment
```
* Weather Room Trigger: Random (every 30 seconds)
eval weather_roll %random.100%
switch %weather_roll%
  case 1 to 20
    %echo% Dark clouds gather overhead.
    set weather stormy
    break
  case 21 to 40
    %echo% A gentle rain begins to fall.
    set weather rainy
    break
  case 41 to 80
    %echo% The sun breaks through the clouds.
    set weather sunny
    break
  default
    %echo% A cool breeze stirs the air.
    set weather clear
    break
done
```

### Interactive Puzzle
```
* Puzzle Room: Command "push button"
eval button_count %self.var(buttons_pushed)% + 1
set buttons_pushed %button_count%

switch %button_count%
  case 1
    %echo% The first button glows softly.
    break
  case 2
    %echo% The second button activates.
    break
  case 3
    %echo% All buttons are now active! A secret passage opens!
    %door% %self.vnum% down flags a
    set buttons_pushed 0
    break
  default
    %echo% Nothing happens.
    break
done
```

---

*This documentation covers the core DG scripting system. For advanced scripting techniques and specific trigger examples, refer to the builder documentation and existing script files in the world data.*
