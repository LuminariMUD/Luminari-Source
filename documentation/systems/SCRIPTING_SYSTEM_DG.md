# LuminariMUD DG Scripting System

## Overview

The DG (DikuMUD Scripting) system in LuminariMUD provides a powerful, event-driven scripting language that allows builders to create dynamic, interactive content. Scripts can be attached to mobiles (NPCs), objects, and rooms to respond to player actions, create complex behaviors, and implement sophisticated game mechanics.

## Introduction to DG Scripts

DG Scripts (DikuMUD Scripting) allow builders to create interactive, dynamic content without programming knowledge. This system enables the creation of complex behaviors, quests, puzzles, and environmental effects that respond to player actions.

**Script Types:**
- **Mobile Scripts:** Attached to NPCs
- **Object Scripts:** Attached to items
- **Room Scripts:** Attached to rooms
- **Global Scripts:** System-wide effects

## Common Trigger Types

### Mobile Triggers
- **Global:** Check even if zone empty
- **Random:** Periodic random actions
- **Command:** When specific commands are entered
- **Speech:** When specific words are spoken
- **Act:** When specific actions occur
- **Death:** When mobile dies
- **Greet:** When player enters room (visible)
- **Greet-All:** When anything enters room
- **Entry:** When the mob enters a room
- **Receive:** When character is given object
- **Fight:** During combat (each pulse)
- **HitPrcnt:** When fighting and below certain HP percentage
- **Bribe:** When coins are given to mob
- **Load:** When the mob is loaded
- **Memory:** When mob sees someone remembered
- **Cast:** When mob is targeted by spell
- **Leave:** When someone leaves room (visible)
- **Door:** When door is manipulated in room
- **Time:** Trigger based on game hour

### Object Triggers
- **Global:** Unused
- **Random:** Periodic random actions
- **Command:** When specific command is used
- **Timer:** When item's timer expires
- **Get:** When object is picked up
- **Drop:** When character tries to drop object
- **Give:** When character tries to give object
- **Wear:** When character tries to wear object
- **Remove:** When character tries to remove object
- **Load:** When the object is loaded
- **Cast:** When object is targeted by spell
- **Leave:** When someone leaves room
- **Consume:** When character tries to eat/drink object
- **Time:** Trigger based on game hour

### Room Triggers
- **Global:** Check even if zone empty
- **Random:** Periodic random actions
- **Command:** When command is used in room
- **Speech:** When something is said in room
- **Reset:** When zone has been reset
- **Enter:** When character enters room
- **Drop:** When something is dropped in room
- **Cast:** When spell is cast in room
- **Leave:** When character leaves room
- **Door:** When door is manipulated in room
- **Login:** When character logs into MUD
- **Time:** Trigger based on game hour

## Basic Scripting Examples

### Simple Greeting Script
```
Name: 'Friendly Shopkeeper Greeting'
Trigger: Greet
Commands:
say Welcome to my shop, %actor.name%!
say I have the finest goods in the land!
```

### Quest Item Script
```
Name: 'Magic Sword Recognition'
Trigger: Get
Commands:
if %actor.level% < 20
  say This weapon is too powerful for you!
  drop %self%
else
  say The sword glows with magical power!
end
```

## Core Architecture

### Script Components

#### 1. Triggers (`struct trig_data`)
Triggers are the fundamental units of the scripting system:
```c
struct trig_data {
  IDXTYPE nr;                         // Trigger's rnum
  byte attach_type;                   // MOB_TRIGGER, OBJ_TRIGGER, WLD_TRIGGER
  byte data_type;                     // Type of game_data for trigger
  char *name;                         // Name of trigger
  long trigger_type;                  // Type of trigger (bitvector)
  struct cmdlist_element *cmdlist;    // Top of command list
  struct cmdlist_element *curr_state; // Pointer to current line of trigger
  int narg;                           // Numerical argument
  char *arglist;                      // Argument list
  struct trig_var_data *var_list;     // Local variables
  long context;                       // Context for statics
  struct event *wait_event;           // Wait event
  int depth;                          // Current depth
  struct trig_data *next;             // Next trigger in list
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
set enhanced 1
remote enhanced %actor.id%
```

#### Command Triggers (Objects)
Object command triggers can be set to activate based on where the object is:
```
Trigger Type: Command
Argument: "pull lever"
Numeric Arg: 100
Command Type: EQUIP (object must be equipped)
             INVEN (object must be in inventory)
             ROOM (object must be in same room)

Script:
if %actor.level% >= 10
  %echo% The magical lever responds to your touch!
  %teleport% %actor% 1234
else
  %send% %actor% The lever doesn't respond to you.
end
```

#### Consume Triggers
Activate when object is consumed (eat/drink/quaff):
```
Trigger Type: Consume
Argument: ""
Numeric Arg: 100
Consume Type: EAT, DRINK, or QUAFF

Script:
%send% %actor% The potion tastes bitter but energizing.
%actor.hitp% += 50
if %actor.hitp% > %actor.maxhitp%
  %actor.hitp% = %actor.maxhitp%
end
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
%gecho% <message>                - Send message to entire game world
%asound% <message>               - Send message to adjacent rooms
%recho% <message>                - Send message to room (no self)
```

#### Character Manipulation
```
%teleport% <target> <room>       - Move character to room
%force% <target> <command>       - Force character to execute command
%damage% <target> <amount>       - Deal damage to character
%follow% <target>                - Make mobile follow target
%hunt% <target>                  - Make mobile hunt target
%remember% <target>              - Remember target for memory triggers
%forget% <target>                - Forget target from memory
%kill% <target>                  - Make mobile attack target
%transform% <vnum>               - Transform mobile into different mobile
```

#### Clan Commands (Mobile Scripts)
```
%clanset% <player> <clan>        - Set player's clan membership
%clanrank% <player> <rank>       - Set player's clan rank
%clangold% <clan> <amount>       - Modify clan's gold
%clanwar% <clan1> <clan2> <status> - Set war status between clans
%clanally% <clan1> <clan2> <status> - Set alliance between clans
```

#### Object Manipulation
```
%load% obj <vnum>                - Load object to room/inventory
%load% mob <vnum>                - Load mobile to room
%purge% <target>                 - Remove object/mobile
%give% <object> <target>         - Give object to character (not direct command)
%junk% <object>                  - Destroy object in mobile's inventory
%transform% <vnum>               - Transform object into another
%timer% <ticks>                  - Set object timer
```

#### Room Manipulation
```
%door% <room> <direction> <flags> - Modify door flags
%at% <room> <command>             - Execute command at different room
%move% <object> <room>            - Move object to different room
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
%time.hour%      - Current game hour (0-23)
%time.day%       - Current game day
%time.month%     - Current game month
%time.year%      - Current game year
```

#### Trigger-Specific Variables
Different triggers automatically set specific variables:

**Command Triggers:**
```
%cmd%            - The command that was entered
%arg%            - Arguments to the command
```

**Speech Triggers:**
```
%speech%         - The complete speech text
%actor%          - Who spoke
```

**Fight Triggers:**
```
%actor%          - Who the mobile is fighting
```

**Act Triggers:**
```
%actor%          - Who performed the action
%victim%         - Target of the action (if any)
%object%         - Object involved (if any)
%arg%            - Additional arguments
```

**Give/Receive Triggers:**
```
%actor%          - Who gave the object
%object%         - The object being given
```

#### Variable Operations
```
set <variable> <value>           - Set variable value
unset <variable>                 - Remove variable
eval <variable> <expression>     - Evaluate mathematical expression
remote <variable> <id>           - Set remote variable on other entity
global <variable> <value>        - Set global variable
context <id>                     - Set context for variables
extract <variable> <string> <field> - Extract field from string
makeuid <variable> <target>      - Create unique ID for target
```

#### Advanced Commands
```
wait <time>                      - Pause script execution
wait until <time>                - Wait until specific game time
halt                             - Stop script execution immediately
nop                              - No operation (comment)
dg_cast '<spell>' <target>       - Cast spell through script
dg_affect <target> <spell> <duration> - Apply spell effect
```

#### Context Variables

**Character Variables:**
```
%actor.name%     - Actor's name
%actor.level%    - Actor's level
%actor.class%    - Actor's class
%actor.race%     - Actor's race
%actor.room%     - Actor's current room
%actor.hitp%     - Actor's current hit points
%actor.maxhitp%  - Actor's maximum hit points
%actor.mana%     - Actor's current mana
%actor.maxmana%  - Actor's maximum mana
%actor.move%     - Actor's current movement
%actor.maxmove%  - Actor's maximum movement
%actor.str%      - Actor's strength
%actor.int%      - Actor's intelligence
%actor.wis%      - Actor's wisdom
%actor.dex%      - Actor's dexterity
%actor.con%      - Actor's constitution
%actor.cha%      - Actor's charisma
%actor.exp%      - Actor's experience points
%actor.gold%     - Actor's gold
%actor.id%       - Actor's unique ID
%actor.fighting% - Who actor is fighting
%actor.eq(<pos>)% - Equipment at position
%actor.inventory(<vnum>)% - Object in inventory
```

**Object Variables:**
```
%self.name%      - Object's name
%self.shortdesc% - Object's short description
%self.id%        - Object's unique ID
%self.type%      - Object's type
%self.vnum%      - Object's virtual number
%self.val0%      - Object value 0
%self.val1%      - Object value 1
%self.val2%      - Object value 2
%self.val3%      - Object value 3
%self.timer%     - Object's timer
%self.weight%    - Object's weight
%self.cost%      - Object's cost
%self.room%      - Room containing object
%self.carried_by% - Character carrying object
%self.worn_by%   - Character wearing object
```

**Room Variables:**
```
%self.name%      - Room's name
%self.id%        - Room's unique ID
%self.vnum%      - Room's virtual number
%self.sector%    - Room's sector type
%self.north%     - Room to the north
%self.south%     - Room to the south
%self.east%      - Room to the east
%self.west%      - Room to the west
%self.up%        - Room above
%self.down%      - Room below
%self.people%    - List of people in room
%self.contents%  - List of objects in room
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
halt                            - Stop script execution immediately
nop <comment>                   - No operation (for comments)
```

**Note:** There is no `%debug%` command in the actual implementation. Use regular MUD logging or `%echo%` for debugging output.

### Error Handling
```
* Always check for valid targets
if !%actor%
  %echo% Script triggered with no actor!
  halt
end

* Validate room numbers before teleporting
if %room.vnum% < 1
  %echo% Invalid room number: %room.vnum%
  halt
end

* Check if objects/mobiles exist before manipulation
if !%target%
  %echo% Target does not exist
  halt
end
```

### Testing Procedures
```
* Use echo output for tracking
%echo% Quest script started for %actor.name%
set debug_mode 1

if %debug_mode%
  %send% %actor% DEBUG: Setting quest state to 'started'
end

remote quest_state started
```

### Script Memory System
DG Scripts support a memory system for mobiles to remember interactions:

```
* Mobile remembers player who attacked it
Trigger Type: Death
Script:
%remember% %actor%
```

```
* Mobile reacts when remembered player returns
Trigger Type: Memory
Script:
say You! You're the one who killed me before!
%force% %actor% flee
```

### Wait Command Syntax
The `wait` command supports multiple time formats:

```
wait 5                          - Wait 5 pulses
wait 10 s                       - Wait 10 seconds
wait 2 t                        - Wait 2 MUD hours
wait until 14:30                - Wait until 14:30 game time
wait until 1430                 - Wait until 14:30 game time (alternative format)
```

**Time Units:**
- No suffix: pulses (1/10 second)
- `s`: seconds
- `t`: MUD hours (75 real seconds)
- `until`: wait until specific game time (24-hour format)

### Variable Contexts and Scoping

DG Scripts support different variable scopes:

#### Local Variables
Variables set with `set` are local to the current trigger:
```
set temp_value 100
eval result %temp_value% * 2
```

#### Global Variables
Variables shared across all triggers on the same entity:
```
global persistent_state active
if %global.persistent_state% == active
  say The system is already active!
else
  global persistent_state active
  say System activated!
end
```

#### Remote Variables
Variables set on other entities using their unique ID:
```
* Set variable on another mobile/object/room
remote quest_completed %actor.id%

* Set variable on global scope (room 0)
remote world_event_active %global%
```

#### Context Variables
Control variable context for advanced scripting:
```
context %actor.id%
set player_specific_data some_value
context 0
set global_data shared_value
```

### Special Script Commands

#### Extract Command
Extract specific fields from strings:
```
* Extract word from sentence
extract word "hello world test" 2
* %word% now contains "world"

* Extract character name from UID
extract name %actor% name
```

#### MakeUID Command
Create unique identifiers for script references:
```
makeuid target_id %actor%
remote last_visitor %target_id%
```

#### DG Cast Command
Cast spells through scripts:
```
dg_cast 'heal' %actor%
dg_cast 'fireball' %target%
dg_cast 'bless' self
```

#### DG Affect Command
Apply spell effects without casting:
```
dg_affect %actor% 'bless' 10
dg_affect %target% 'curse' 5
```

## Complete Command Reference by Entity Type

### Mobile Commands (prefix: m)
```
masound <message>                - Send message to adjacent rooms
mat <room> <command>             - Execute command at different room
mdamage <target> <amount>        - Deal damage to target
mdoor <room> <dir> <flags>       - Modify door flags
mecho <message>                  - Send message to room (including self)
mgecho <message>                 - Send message to entire game world
mrecho <start> <end> <message>   - Send message to room range
mechoaround <target> <message>   - Send message to room except target
msend <target> <message>         - Send message to specific character
mload <type> <vnum>              - Load mobile or object
mpurge <target>                  - Remove mobile or object
mgoto <room>                     - Move mobile to room
mteleport <target> <room>        - Move target to room
mforce <target> <command>        - Force target to execute command
mhunt <target>                   - Make mobile hunt target
mremember <target>               - Remember target for memory triggers
mforget <target>                 - Forget target from memory
mtransform <vnum>                - Transform into different mobile
mzoneecho <message>              - Send message to entire zone
mfollow <target>                 - Make mobile follow target
mkill <target>                   - Attack target
mjunk <object>                   - Destroy object in inventory
```

### Object Commands (prefix: o)
```
oasound <message>                - Send message to adjacent rooms
oat <room> <command>             - Execute command at different room
odamage <target> <amount>        - Deal damage to target
odoor <room> <dir> <flags>       - Modify door flags
oecho <message>                  - Send message to room
ogecho <message>                 - Send message to entire game world
orecho <start> <end> <message>   - Send message to room range
oechoaround <target> <message>   - Send message to room except target
osend <target> <message>         - Send message to specific character
oload <type> <vnum>              - Load mobile or object
opurge <target>                  - Remove mobile or object
oteleport <target> <room>        - Move target to room
oforce <target> <command>        - Force target to execute command
otimer <ticks>                   - Set object timer
otransform <vnum>                - Transform into different object
ozoneecho <message>              - Send message to entire zone
osetval <value0> <value1> <value2> <value3> - Set object values
omove <target> <room>            - Move object to room
```

### Room Commands (prefix: w)
```
wasound <message>                - Send message to adjacent rooms
wat <room> <command>             - Execute command at different room
wdamage <target> <amount>        - Deal damage to target
wdoor <room> <dir> <flags>       - Modify door flags
wecho <message>                  - Send message to room
wgecho <message>                 - Send message to entire game world
wrecho <start> <end> <message>   - Send message to room range
wechoaround <target> <message>   - Send message to room except target
wsend <target> <message>         - Send message to specific character
wload <type> <vnum>              - Load mobile or object
wpurge <target>                  - Remove mobile or object
wteleport <target> <room>        - Move target to room
wforce <target> <command>        - Force target to execute command
wzoneecho <message>              - Send message to entire zone
wmove <target> <room>            - Move object to room
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

## Script Limitations and Considerations

### Maximum Script Depth
Scripts have a maximum recursion depth of 10 levels to prevent infinite loops and stack overflow.

### Charmed Mobiles
Charmed mobiles (those under player control) cannot execute most script commands for security reasons.

### Player vs NPC Targeting
Some commands have restrictions on targeting players vs NPCs. Use `valid_dg_target()` checks in the code.

### Memory Management
- Variables are automatically cleaned up when triggers complete
- Global variables persist until explicitly unset
- Remote variables remain until the target entity is destroyed

### Performance Impact
- Random triggers fire based on percentage chance
- Global triggers check even in empty zones (use sparingly)
- Complex scripts can impact server performance

## Best Practices

### Script Organization
```
* Use descriptive trigger names
Name: 'Shopkeeper Greeting and Quest Giver'

* Comment your scripts
nop This trigger handles the dragon quest
nop Check if player has completed prerequisite
```

### Error Prevention
```
* Always validate targets exist
if !%actor%
  halt
end

* Check for required conditions
if %actor.level% < 10
  %send% %actor% You are not experienced enough.
  halt
end

* Use appropriate trigger types
* Command triggers for specific commands
* Speech triggers for keywords
* Act triggers for general actions
```

### Variable Naming
```
* Use descriptive names
set quest_dragon_completed 1
set player_last_visit_time %time.hour%

* Avoid conflicts with system variables
* Don't use names like 'actor', 'self', 'random'
```

### Testing Scripts
```
* Test with different player levels
* Test edge cases (no arguments, invalid targets)
* Test script interactions with other systems
* Use temporary echo statements for debugging
```

## Common Pitfalls

### Variable Scope Confusion
```
* Wrong: Expecting local variables to persist
set temp_value 100
wait 10 s
* temp_value may not exist after wait

* Right: Use global variables for persistence
global temp_value 100
wait 10 s
* %global.temp_value% will still exist
```

### Infinite Loops
```
* Wrong: No exit condition
while 1
  %echo% This will run forever!
done

* Right: Always have exit conditions
set counter 0
while %counter% < 10
  %echo% Loop iteration %counter%
  eval counter %counter% + 1
done
```

### Resource Leaks
```
* Wrong: Loading without cleanup
%load% mob 1234
%load% mob 1234
* Creates multiple copies

* Right: Check before loading
if !%room.mob(1234)%
  %load% mob 1234
end
```

## Script File Format and OLC Integration

### Script File Structure
DG Scripts are stored in `.trg` files with the following format:

```
#<vnum>
Name: Script Name~
Attach: <MOB|OBJ|WLD>
Triggers: <trigger_types>
Numeric Arg: <percentage>
Arguments: <trigger_arguments>~
Commands:
<script_commands>
~
```

**Example Script File:**
```
#1234
Name: Friendly Shopkeeper~
Attach: MOB
Triggers: c
Numeric Arg: 100
Arguments: greet hello~
Commands:
if %actor.level% < 5
  say Welcome, young adventurer!
  say I have special items for beginners.
else
  say Greetings, experienced warrior!
  say Browse my finest wares.
end
~
```

### OLC (Online Creation) Integration

#### Creating Scripts with TRIGEDIT
```
trigedit <vnum>                  - Create/edit trigger
trigedit <vnum> copy <source>    - Copy existing trigger
trigedit <vnum> delete           - Delete trigger
```

#### Attaching Scripts to Entities
```
* Mobile Scripts
medit <vnum>
script <trigger_vnum>            - Attach trigger to mobile

* Object Scripts
oedit <vnum>
script <trigger_vnum>            - Attach trigger to object

* Room Scripts
redit <vnum>
script <trigger_vnum>            - Attach trigger to room
```

#### Script Testing Commands
```
vstat trig <vnum>                - View trigger details
tstat <entity>                   - View entity's attached scripts
```

### Integration with Game Systems

#### Zone Reset Integration
Scripts can be triggered during zone resets:
```
* Zone Reset Trigger (Room)
Trigger Type: Reset
Script:
%echo% The magical energies in this room have been restored.
%load% obj 1234
```

#### Combat System Integration
Scripts integrate with the combat system:
```
* Fight Trigger activates each combat round
* Death Trigger activates when mobile dies
* HitPrcnt Trigger activates at specific HP thresholds
```

#### Quest System Integration
Scripts can manage quest states and progression:
```
* Use remote variables to track quest progress
* Coordinate between multiple NPCs and objects
* Implement complex quest chains
```

---

*This documentation covers the complete DG scripting system implementation in LuminariMUD. For builder-specific guides and world creation examples, refer to the building documentation in the `documentation/building_game-data/` directory.*
