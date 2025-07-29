# LuminariMUD DG Scripting System

## Overview

The DG (DikuMUD Scripting) system in LuminariMUD provides a powerful, event-driven scripting language that allows builders to create dynamic, interactive content. Scripts can be attached to mobiles (NPCs), objects, and rooms to respond to player actions, create complex behaviors, and implement sophisticated game mechanics.

## Introduction to DG Scripts

DG Scripts (DikuMUD Scripting) are the primary mechanism through which builders—the creative staff responsible for designing the game's areas, non-player characters (NPCs), and items—can create a dynamic, interactive, and responsive virtual world. This system enables the creation of complex behaviors, quests, puzzles, and environmental effects that respond to player actions.

The fundamental purpose of DG Scripts is to allow world builders to define complex behaviors for game elements without requiring them to be programmers proficient in the MUD's core language, C. This separation of concerns is critical; it empowers the individuals focused on narrative, game design, and world atmosphere to implement their ideas directly. The Builder Academy (TBA), a MUD dedicated to training builders, features extensive tutorials and help files on DG Scripts, positioning them as a key competency for anyone aspiring to create content for this family of MUDs.

### DG Scripts vs. MobProgs: An Evolutionary Leap

The development of DG Scripts represents a significant evolutionary step beyond older NPC scripting systems like MobProgs. While MobProgs were a revolutionary feature in their own right, DG Scripts expanded upon the concept in several critical ways, as detailed in historical MUD development discussions.

The key differentiators are:

- **Expanded Scope**: MobProgs were designed exclusively for "mobiles," or NPCs. DG Scripts dramatically broadened this scope, allowing scripts to be attached not only to mobiles but also to objects and rooms (often referred to as "world" or "wld" scripts). This enhancement unlocked a vast new potential for interactive design, enabling builders to create objects with unique behaviors (e.g., a magical portal that teleports a player when used) or rooms with environmental effects (e.g., a chamber that seals itself shut when a player enters).
- **Enhanced Functionality**: DG Scripts introduced a more sophisticated command set, including support for local and global variables, conditional logic (if/else), and looping (while). Perhaps the most crucial addition was the wait command, which allows a script to pause between actions. MobProgs, in contrast, executed their command lists instantaneously and in a strictly linear fashion. The ability to add delays is fundamental for creating believable interactions, timed events, and complex sequences.
- **Designed Coexistence**: The DG Script system was written with a new set of functions, allowing it to be installed and run alongside an existing MobProg system. This provided a practical migration path for established MUDs, which could adopt the more powerful DG Scripts for new content without needing to convert their entire library of existing MobProgs.

This evolution from MobProgs to DG Scripts marked a fundamental shift in MUD development philosophy. Previously, any complex behavior that fell outside the scope of MobProgs—such as a timed puzzle, an object with a unique function, or a room with special properties—required a C programmer to write a hard-coded "special procedure" (spec_proc) and recompile the MUD server. DG Scripts moved a significant amount of this power from the low-level C code into the hands of the builders. This democratization of complex world-building broke a major development bottleneck, fostering a more rapid and creative design process by empowering the content creators directly.

### The Anatomy of a DG Script

Every DG Script is composed of three fundamental components:

- **Triggers**: The specific in-game event that causes a script to execute. Each trigger is defined by its type (e.g., a player speaking, giving money to a mob), a numeric argument (NArg), and a text argument (Arg).
- **Commands**: The list of actions the script performs when triggered. This is a small program that can include standard MUD commands, script-specific commands, and flow control commands.
- **Variables**: The variable system allows scripts to store, retrieve, and manipulate data, enabling them to be stateful and intelligent.

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

## Trigger Reference: NArg and Argument Meanings

Understanding trigger mechanics is crucial. The function of Numeric Argument (NArg) and Text Argument (Arg) fields changes based on the trigger type:

| Trigger Name | Attachable To | Firing Condition | NArg Meaning | Argument Meaning |
|--------------|---------------|------------------|--------------|------------------|
| Act | Mob | Sees a social message or action text in the room | 0 for phrase match, 100 for substring match | The text/phrase to match |
| Bribe | Mob | A player gives money to the mob | The minimum amount of gold to fire the trigger | Not used |
| Command | Obj, Wld | A player types a specific command | Not used | The command to trigger on |
| Death | Mob | The mob is killed | Percent chance to fire (0-100) | Not used |
| Drop | Wld | A player drops an item in the room | The VNUM of the specific item to trigger on (0 for any) | Not used |
| Enter | Wld | A player enters the room | Percent chance to fire (0-100) | Not used |
| Fight | Mob | The mob is engaged in combat (checked each round) | Percent chance to fire (0-100) | Not used |
| Get | Obj | A player picks up the object | Percent chance to fire (0-100) | Not used |
| Give | Obj | A player gives the object to a mob | Percent chance to fire (0-100) | Not used |
| Greet | Mob | A player enters the room where the mob is | Percent chance to fire (0-100) | Not used |
| Greet-All | Mob | Any character enters the room (including appearing) | Percent chance to fire (0-100) | Not used |
| HitPrcnt | Mob | The mob's health drops below a certain percentage | The hit point percentage threshold | Not used |
| Leave | Wld | A player leaves the room | The VNUM of the room the player is going to (0 for any) | Not used |
| Leave-All | Wld | Any character leaves the room (including being extracted/purged) | Similar to Leave trigger | Not used |
| Entry | Mob | The mob is loaded/enters a room | Percent chance to fire (0-100) | Not used |
| Memory | Mob | Mob sees someone it remembers | Percent chance to fire (0-100) | Not used |
| Cast | Mob, Obj, Wld | Entity is targeted by a spell | Not used | Not used |
| Load | Mob, Obj | Entity is loaded into the game | Percent chance to fire (0-100) | Not used |
| Zone Reset | Wld | Zone containing the room resets | Not used | Not used |
| Random | Mob, Wld | Fires periodically (approx. every 13-15 seconds) | Percent chance to fire (0-100) | Not used |
| Receive | Mob | A player gives an item to the mob | The VNUM of the specific item to trigger on (0 for any) | Not used |
| Speech | Mob, Wld | A player says something in the room | Not used | A list of keywords to listen for |
| Timer | Obj | Fires periodically when the object is on the ground | The number of MUD ticks between firings | Not used |
| Use | Obj | A player uses the 'use' command on the object | Not used | The name of the tool being used on the object |
| Wear | Obj | A player wears the object | Percent chance to fire (0-100) | Not used |
| Remove | Obj | A player removes the object | Percent chance to fire (0-100) | Not used |
| Give | Obj, Mob | A player gives the object/item to mob | Percent chance to fire (0-100) | Not used |
| Time | Mob, Obj, Wld | Trigger based on game hour | Not used | Not used |

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

### Gate Guard Example (Complete Tutorial)

This example demonstrates core scripting concepts through a functional gate guard:

**Scenario**: We will create a gate guard for a walled town. The town's ruler has imposed a toll. The guard's responsibilities are:
- Notify travelers of the 10-coin entry fee upon their arrival
- Collect the money from travelers
- If the correct amount is paid, open the gate to allow entry
- If an incorrect amount is paid, inform the traveler and return the money
- Close the gate after a traveler has passed through or after a set amount of time

**Trigger Selection and Configuration**

Next, we translate these desired behaviors into specific DG Script triggers and configure their arguments:

- **Behavior 1 (Notify traveler)**: This is a reaction to a character entering the room. The appropriate trigger is a Greet Trigger. We want it to fire every time, so its NArg (percent chance) will be set to 100.
- **Behavior 2, 3, 4 (Collect money)**: This involves a character giving money to the guard. The Bribe Trigger is designed for this. We will need two separate bribe triggers to handle the success and failure cases:
  - **Success Case**: To open the gate, the trigger must fire when 10 or more coins are given. The NArg (minimum amount) will be set to 10.
  - **Failure Case**: To return insufficient payment, the trigger must fire when any amount less than 10 is given. To catch all insufficient payments, its NArg will be set to 1. The script logic will ensure this only runs for amounts less than 10.
- **Behavior 5 (Close gate)**: This involves reacting to a character's actions or the passage of time:
  - To react to a character passing through the gate (e.g., "leaves south"), we will use an Act Trigger. Its Argument will be the text to match, such as "leaves south", and its NArg will be 0, which is used for phrase matching.
  - To close the gate automatically after it's been opened, we can add a timed wait command to the successful bribe trigger.

**Greet Trigger (NArg = 100)**
```
* Check if they came from outside the city
if %direction% == north
  wait 1
  emote snaps to attention as you approach.
  wait 1
  say Admittance to the city is 10 coins.
end
```

**Explanation**: The script uses the built-in %direction% variable, which is automatically set by a Greet trigger. The wait 1 commands are crucial; Greet triggers are checked before the character's arrival is displayed to them. These slight pauses ensure the guard's emote and speech appear after the character's "arrival" text, creating a logical sequence of events.

**Bribe Trigger Success (NArg = 10)**
```
wait 1
unlock gate
open gate
wait 20 s
close gate
lock gate
```

**Explanation**: The script unlocks and opens the gate. The command wait 20 s causes the script to pause for 20 real-world seconds before proceeding to close and lock the gate. The s suffix denotes real time, as opposed to game "pulses". This creates a self-resetting mechanism.

**Bribe Trigger Failure (NArg = 1)**
```
if %amount% < 10
  wait 1
  say This is not enough!
  give %amount% coins %actor%
end
```

**Explanation**: The if %amount% < 10 check is essential. Without it, this trigger would fire even when 10+ coins are given. The script uses the %amount% and %actor% variables, which are automatically populated by a Bribe trigger with the amount of gold and the character who gave it, respectively. The guard then returns the exact amount of coins to the character.

**Act Trigger (Argument = "leaves south", NArg = 0)**
```
wait 1
close gate
lock gate
```

**Explanation**: This script reacts to seeing the text "leaves south" in the room. The wait 1 provides a brief delay to avoid the visual of the gate slamming shut on the character as they are leaving.

Key points demonstrated:
- The %direction% variable is automatically set by Greet triggers
- The wait commands ensure proper timing of messages
- Bribe triggers automatically populate %amount% and %actor%
- Multiple triggers can work together for complex behaviors
- The NArg for Act triggers (0 for phrase match, 100 for substring match) determines matching behavior

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

The DG Script command set provides powerful tools for world interaction. Commands can be categorized by their function and the type of script they're used in.

### Script Flow and Logic Commands

These commands control the execution path of the script itself:

- **if/else/end**: Standard conditional logic. The script evaluates an expression, and if true, executes the following block of commands until an else or end is reached.
- **while/done**: Creates a loop that continues as long as the specified condition is true.
- **switch/case/break/default/done**: A multi-way branching structure that compares a variable against multiple case values.
- **wait**: Pauses the script's execution. wait <num> pauses for <num> game pulses (approx. 0.1 seconds each). wait <num> s pauses for <num> real-world seconds.
- **eval**: Evaluates a mathematical or logical expression and assigns the result to a local script variable. Example: eval foobar 15 - 5.
- **set/global/context**: set assigns a value to a local variable. global makes that variable accessible to other scripts. context assigns a numerical context to a global variable, allowing a single script to manage states for multiple entities simultaneously.
- **remote**: Writes the value of a local variable to a global variable stored on a specific player character, identified by their ID. This is the modern method for managing persistent player-specific data.
- **break**: Immediately exits the current while or switch block.

### World and Game State Commands

These commands directly alter the state of the MUD world:

- **%load% <obj|mob> <vnum> [target][location]**: Loads an instance of an object or mobile into the game. It can be loaded into the room, a player's inventory, or even equipped directly.
- **%purge% [target]**: Permanently removes a mob or object from the game. If no target is specified, it purges the entity the script is attached to. It is critical to use this command at the end of a script, as purging an entity makes its variables inaccessible.
- **%teleport% <target> <location_vnum>**: Instantly moves a character or all characters in the room ('all') to a new room.
- **%damage% <target> <amount>**: Inflicts damage on a target. A negative amount will heal the target.
- **%door% <room_vnum> <dir> <field> [value]**: A powerful command to manipulate a room's exits. It can create or purge exits, change their flags (e.g., closed, locked), set a key, or change the destination room.
- **%at% <location_vnum> <command>**: Executes a command in a different room from where the script is running.

### Mobile-Specific Commands

These commands are typically used in scripts attached to mobiles:

- **%force% <target> <command>**: Forces a character to perform a command as if they had typed it themselves.
- **%mfollow% <target>**: Causes the mob to begin following the target character without the standard "starts following you" message.
- **%mtransform% <vnum>**: Permanently transforms the mob into a different mob specified by <vnum>. The new mob retains the hit points and script of the original.

### Object-Specific Commands

These commands are used in scripts attached to objects:

- **%otransform% <vnum>**: Permanently transforms the object into a different one specified by <vnum>, retaining the original script.
- **%opurge%**: A specific alias for purging the object the script is attached to.

### Communication Commands

These commands allow scripts to communicate with players:

- **%echo% <message>**: Sends a message to all characters in the room.
- **%send% <target> <message>**: Sends a private message to a single target character.
- **%echoaround% <target> <message>**: Sends a message to everyone in the room except the specified target.
- **%asound% <message>**: Sends a message to all adjacent rooms, typically as an ambient sound (e.g., "You hear a scream from the north.").
- **%zoneecho% <room_vnum> <message>**: Sends a message to every room in the zone that contains the specified room vnum.

A critical and often frustrating issue for new scripters involves pronoun substitution. In echo and send commands, the tilde character (~) is used as a placeholder for pronouns (e.g., ~actor% might expand to "he", "she", or "it" depending on the actor's gender). However, historical versions of the script loader and various text editors could not properly handle the ~ character, leading to script errors or garbage output. Some codebases have modified this to use a different character, such as #. Builders should be aware of this potential pitfall and verify which character their specific MUD version uses for pronoun substitution.

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

**Note on Pronoun Substitution**: In echo and send commands, the tilde character (~) is used as a placeholder for pronouns (e.g., ~actor% might expand to "he", "she", or "it" depending on the actor's gender). However, some codebases have modified this to use a different character, such as #. Builders should verify which character their specific MUD version uses.

#### Character Manipulation
```
%teleport% <target> <room>       - Move character to room
%force% <target> <command>       - Force character to execute command
%damage% <target> <amount>       - Deal damage to character (negative heals)
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
%load% obj <vnum> [target][location] - Load object to room/inventory/equipment
%load% mob <vnum>                - Load mobile to room
%purge% [target]                 - Remove object/mobile (if no target, purges self)
%give% <object> <target>         - Give object to character (not direct command)
%junk% <object>                  - Destroy object in mobile's inventory
%transform% <vnum>               - Transform object into another
%timer% <ticks>                  - Set object timer
```

**Critical Note on %purge%**: Always place %purge% commands at the very end of a script. Purging an entity immediately removes it and all its associated variables from the game, which can cause subsequent commands in the same script to fail if they try to reference the purged entity.

#### Room Manipulation
```
%door% <room> <direction> <field> [value] - Manipulate room exits
%at% <location> <command>        - Execute command at different room
%move% <object> <room>           - Move object to different room
```

The %door% command is particularly powerful, allowing scripts to:
- Create or purge exits
- Change exit flags (e.g., closed, locked)
- Set key requirements
- Change destination rooms

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

#### Flow Control Commands
```
wait <time>                      - Pause script execution
wait until <time>                - Wait until specific game time
halt                             - Stop script execution immediately
break                            - Exit current loop or switch block
nop                              - No operation (comment)
return                           - Exit from current script execution
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

#### Expression Operators

Expressions are used in if and eval statements to make decisions and perform calculations:

- **Logical**: || (or), && (and), ! (not)
- **Comparison**: == (equal, case-insensitive for strings), != (not equal), <, >, <=, >=
- **Substring**: /= (returns true if the right operand is a substring of the left)
- **Arithmetic**: + (add), - (subtract), * (multiply), / (divide)

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

The variable system allows scripts to read game data, make decisions, and manipulate the world state. All variables in DG Scripts are referenced by enclosing their name in percent signs (e.g., %actor%). To access specific pieces of information about a variable, a field is appended using a dot (.). 

Fields can be read-only (e.g., %actor.name%) or assignable. Assignable fields are used to change a value and are typically denoted with parentheses, which may contain an argument.

**Character Variables (%actor%, %self%, %victim%, %random%, etc.):**

| Field | Description | Assignable? | Example |
|-------|-------------|-------------|---------|
| .name | The character's name | No | %actor.name% |
| .vnum | The character's virtual number (-1 for players) | No | if %self.vnum(1234)% |
| .level | The character's level | Yes | %actor.level(10)% |
| .class | The character's class as a string | Yes | if %actor.class% == Warrior |
| .race | The character's race | Yes | %actor.race% |
| .room | Character's current room VNUM | No | %actor.room% |
| .hitp() | The character's current hit points | Yes | %self.hitp(50)% (heals 50) |
| .maxhitp() | The character's maximum hit points | Yes | eval newmax %self.maxhitp% |
| .mana() | Current mana points | Yes | %actor.mana(-10)% |
| .maxmana() | Maximum mana points | Yes | %actor.maxmana% |
| .move() | Current movement points | Yes | %actor.move% |
| .maxmove() | Maximum movement points | Yes | %actor.maxmove% |
| .str | Strength score | No | %actor.str% |
| .int | Intelligence score | No | %actor.int% |
| .wis | Wisdom score | No | %actor.wis% |
| .dex | Dexterity score | No | %actor.dex% |
| .con | Constitution score | No | %actor.con% |
| .cha | Charisma score | No | %actor.cha% |
| .exp() | Experience points | Yes | %actor.exp% += 5000 |
| .gold() | Gold amount | Yes | %actor.gold(-100)% (takes 100) |
| .id | Unique character ID | No | %actor.id% |
| .fighting | ID of character this character is fighting | No | if !%self.fighting% |
| .affect() | Checks if character has a specific spell effect | No | if %actor.affect(bless)% |
| .inventory() | Gets first item, or specific item by vnum, from inventory | No | %purge% %actor.inventory(3010)% |
| .eq() | Gets item equipped in a specific slot | No | if %actor.eq(wield)% |
| .skillset() | Sets a character's proficiency in a skill | Yes | %actor.skillset("bash" 95)% |
| .is_pc | Returns true if the character is a player | No | if %actor.is_pc% |
| .varexists() | Checks if a global variable exists on the character | No | if %actor.varexists(questvar)% |

**Advanced Variable Chaining**: A powerful technique is the chaining of variables to traverse the MUD's data structure. For instance, %object.worn_by.skillset("magic missile" 95)% first identifies an object, then finds the character wearing that object, and finally modifies that character's skill.

**Object Variables (%obj%, %container%, etc.):**

| Field | Description | Assignable? | Example |
|-------|-------------|-------------|---------|
| .name | Object's name | No | %obj.name% |
| .vnum | Object's virtual number | No | %obj.vnum% |
| .cost() | Object's cost | Yes | %obj.cost(100)% |
| .weight() | Object's weight | Yes | %obj.weight(5)% |
| .worn_by | Character variable of person wearing object | No | %obj.worn_by% |
| .carried_by | Character carrying object | No | %obj.carried_by% |
| .contains | First object inside this object (if container) | No | %container.contains% |
| .id | Object's unique ID | No | %obj.id% |
| .type | Object's type | No | %obj.type% |
| .shortdesc | Object's short description | No | %obj.shortdesc% |
| .val0 to .val3 | Object values 0-3 | No | %obj.val0% |
| .timer | Object's timer | No | %obj.timer% |
| .room | Room containing object | No | %obj.room% |

**Room Variables (%room%):**

| Field | Description | Assignable? | Example |
|-------|-------------|-------------|---------|
| .name | Room's name | No | %room.name% |
| .vnum | Room's virtual number | No | %room.vnum% |
| .people | First character in the room | No | %room.people% |
| .contents | First object in the room | No | %room.contents% |
| .<direction>(field) | Inspect exits | No | %room.north(vnum)% |

The direction fields are particularly powerful - %room.north(vnum)% returns the VNUM of the room to the north, while %room.south(bits)% returns the flag state (e.g., "DOOR CLOSED LOCKED") of the southern exit.

**Special and Text Variables:**
```
%random.<N>%     - Random integer between 1 and N
%time.hour%      - Current in-game hour (0-23)
%time.day%       - Current game day
%time.month%     - Current game month
%time.year%      - Current game year
%text.car%       - First word of a string (LISP-style)
%text.cdr%       - Rest of string after first word

```

### Using Expressions: Logical and Arithmetic Operators

Expressions are used in if and eval statements to make decisions and perform calculations. The following operators are supported:

- **Logical**: || (or), && (and), ! (not)
- **Comparison**: == (equal, case-insensitive for strings), != (not equal), <, >, <=, >=
- **Substring**: /= (returns true if the right operand is a substring of the left)
- **Arithmetic**: + (add), - (subtract), * (multiply), / (divide)

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

### Complex State-Aware NPCs

#### Self-Healing Cleric
This mob uses combat state to decide which heal to cast:
```
Trigger: HitPrcnt, NArg = 80
Commands:
* Check if the mob is NOT fighting anyone
if !%self.fighting%
  * If not in combat, cast a slow, powerful heal
  dg_cast 'heal' %self%
else
  * If in combat, cast a faster, less powerful heal
  dg_cast 'cure critical' %self.fighting%
end
```

#### Time-Based Event
Using Random trigger and global variable for regular intervals:
```
Trigger: Random, NArg = 100
Commands:
* Increment a counter variable each time the trigger fires
eval counter %counter% + 1
global counter

* Assuming random triggers fire every 15 seconds, and a MUD hour is 75 seconds.
* The event should fire every 5 triggers (75 / 15 = 5).
if %counter% >= 5
  zoneecho 1200 The town crier yells, 'It is now %time.hour% o'clock!'
  * Reset the counter
  eval counter 0
  global counter
end
```

### Dynamic Objects and Environments

#### One-Way Portal
An object that teleports the user and is consumed:
```
Object: A glowing rune
Trigger: Command, Argument = 'touch rune'
Commands:
%send% %actor% The rune flares with brilliant light!
%teleport% %actor% 3001
%purge% %self%
```

#### Room Trap
A room that seals itself and spawns a monster:
```
Trigger: Enter, NArg = 100
Commands:
%echo% The stone door slams shut behind you!
%door% %room.vnum% north flags b
%load% mob 1234
```

### Interacting with Players: Remote Variables

The modern solution for tracking player progress uses remote variables stored directly on the player character object. This is the key to creating any non-trivial quest system.

#### Quest Giver Interaction
```
* Script on a quest mob, triggered by speech 'I will help'

* Check if the player has already accepted this quest
if %actor.varexists(quest_accepted)%
  say You are already on this quest!
else
  say Thank you, brave adventurer!
  * Set the variable on the player.
  * First, set a local variable with the value 1 (true).
  set quest_accepted 1
  * Then, write that local variable to the remote variable on the player.
  remote quest_accepted %actor.id%
end
```

## Debugging and Testing

### Debug Commands
```
halt                            - Stop script execution immediately
nop <comment>                   - No operation (for comments)
%log% <message>                 - Write to immortal-visible system log
```

The %log% command is an indispensable debugging tool. It allows a builder to write custom messages to the immortal-visible system log file. This can be used to trace a script's execution path, check the values of variables at different points in the script, and confirm whether specific conditions are being met. For example:
```
%log% Bribe trigger fired for %actor.name% with %amount% coins.
```

### Testing and Debugging Strategies

Thorough testing is vital to prevent faulty scripts from causing game crashes or unintended behavior.

#### Live Testing with attach/detach

Before permanently adding a script to a mob or object via its OLC editor, builders should use the attach command. This command applies a trigger to a single instance of an entity temporarily. It allows for safe, live testing in a controlled environment. If the script is flawed, it can be corrected without affecting the permanent world files. The corresponding detach command removes the temporary script.

```
attach mob 1234 5678    - Attach trigger 5678 to mob instance 1234
detach mob 1234 5678    - Remove trigger 5678 from mob instance 1234
```

#### Debugging with %log%

The %log% command is an indispensable debugging tool. It allows a builder to write custom messages to the immortal-visible system log file. This can be used to trace a script's execution path, check the values of variables at different points in the script, and confirm whether specific conditions are being met. For example, one could add %log% Bribe trigger fired for %actor.name% with %amount% coins. to the bribe script to monitor its activation.

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

### Debugging Best Practices

1. **Comment your code extensively** using lines that start with *. This is invaluable for future maintenance by yourself or other builders.

2. **Use the %log% command liberally** during development to trace execution and variable states.

3. **Test with different player levels** and character types to ensure scripts work correctly for all situations.

4. **Test edge cases** such as no arguments, invalid targets, or unexpected player actions.

5. **Verify script interactions** with other systems (combat, spells, etc.).

6. **Use temporary echo statements** for debugging but remember to remove them before finalizing.

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

### Common Script Errors to Avoid

1. **Pronoun Substitution Issues**: Historical versions of the script loader and various text editors could not properly handle the ~ character used for pronoun substitution in echo and send commands. Some codebases have modified this to use #. Verify which character your MUD uses.

2. **Purge Command Placement**: Always place %purge% commands at the very end of a script. Purging an entity immediately removes it and all its associated variables from the game.

3. **Context Sensitivity**: Remember that NArg and Arg fields have different meanings for different trigger types. Always verify the correct usage for your specific trigger.

4. **Script Depth Limits**: Scripts have a maximum recursion depth of 10 levels to prevent infinite loops and stack overflow.

5. **Charmed Mobile Restrictions**: Charmed mobiles (those under player control) cannot execute most script commands for security reasons.

### Understanding Trigger Mechanics: Type, NArg, and Argument

A frequent source of confusion for new scripters is the generic nature of the "Numeric Arg" (NArg) and "Arguments" (Arg) fields in the trigedit interface. The function of these arguments is not fixed; it is entirely dependent on the Trigger Type selected.

- **Trigger Type**: This is a bitvector that defines the specific in-game event(s) that will activate the script.
- **Numeric Argument (NArg)**: A number whose meaning changes based on the trigger type. For a Greet trigger, it is a percentage chance (0-100) that the trigger will fire. For a Bribe trigger, it is the minimum amount of gold required to activate it. For a HitPrcnt trigger, it is the health percentage threshold below which the mob's script will fire.
- **Text Argument (Arg)**: A string whose meaning also changes. For a Speech trigger, it is a list of keywords the script should listen for. For an Act trigger, it is a social message or action text to react to. For most other triggers, it is not used.

Understanding this context-sensitivity is fundamental to correctly configuring triggers.

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

Scripts are created and managed within the game itself using OasisOLC (On-Line Creation), an integrated suite of editors that is a hallmark of the tbaMUD codebase.

#### Creating Scripts with TRIGEDIT

The primary tool for script creation is trigedit, the dedicated trigger editor. A builder uses the command trigedit <vnum> to create a new script or edit an existing one.

```
trigedit <vnum>                  - Create/edit trigger
trigedit <vnum> copy <source>    - Copy existing trigger
trigedit <vnum> delete           - Delete trigger
```

The trigedit interface presents several fields:
- **Name**: A descriptive name for the trigger (e.g., 'guard shout for help')
- **Intended for**: The type of entity the script is designed for: Mobiles, Objects, or Rooms
- **Trigger types**: A bitvector specifying which events will activate the script (e.g., Greet, Bribe, Command). This is the most critical setting.
- **Numeric Arg**: A numerical value whose meaning depends on the selected trigger type
- **Arguments**: A text string whose meaning also depends on the trigger type
- **Commands**: The list of commands to be executed

#### Attaching Scripts: An Overview of trigedit and In-Game Editors

Scripts are created and managed within the game itself using OasisOLC (On-Line Creation), an integrated suite of editors that is a hallmark of the tbaMUD codebase.

The primary tool for script creation is trigedit, the dedicated trigger editor. A builder uses the command trigedit <vnum> to create a new script or edit an existing one. The trigedit interface presents several fields:

- **Name**: A descriptive name for the trigger (e.g., 'guard shout for help'). Intended for builders.
- **Intended for**: The type of entity the script is designed for: Mobiles, Objects, or Rooms
- **Trigger types**: A bitvector specifying which events will activate the script (e.g., Greet, Bribe, Command). This is the most critical setting.
- **Numeric Arg**: A numerical value whose meaning depends on the selected trigger type
- **Arguments**: A text string whose meaning also depends on the trigger type
- **Commands**: The list of commands to be executed

Associated utility commands include:
- **tlist** - for listing triggers within a zone
- **tstat** - for viewing the detailed configuration of a specific trigger

### Attaching Scripts to Entities

Once a trigger is created and saved with trigedit, it is attached to a specific mobile, object, or room using their respective OLC editors:

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
tlist                            - List triggers in current zone
tlist <zone>                     - List triggers in specific zone
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

## Debugging and Best Practices

### Adhering to Best Practices

Adhering to best practices ensures stable and maintainable scripts:

1. **Use the attach/detach workflow** for safely testing new scripts before making them permanent.
2. **Use the %log% command liberally** during development to trace execution and variable states.
3. **Comment your code extensively** using lines that start with *. This is invaluable for future maintenance by yourself or other builders.
4. **Always place %purge% commands at the very end of a script**. Purging an entity immediately removes it and all its associated variables from the game, which can cause subsequent commands in the same script to fail if they try to reference the purged entity.
5. **Be aware of common pitfalls**, such as the tilde (~) character issue in communication commands, and verify the correct syntax for your MUD's specific codebase.

---

*This documentation covers the complete DG scripting system implementation in LuminariMUD. For builder-specific guides and world creation examples, refer to the building documentation in the `documentation/building_game-data/` directory.*
