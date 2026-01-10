# MSDP Variables Reference

This document lists all MSDP (Mud Server Data Protocol) variables that are passed between the Luminari MUD server and compatible clients.

## Overview

MSDP is a telnet protocol extension that allows real-time data exchange between the server and client. The server automatically updates these variables and sends them to clients that support MSDP.

## Variable Categories

### General Server and Character Information

| Variable | Type | Description |
|----------|------|-------------|
| `CHARACTER_NAME` | String | Player character name |
| `SERVER_ID` | String | Unique server identifier |
| `SERVER_TIME` | Number | Current server timestamp |
| `SNIPPET_VERSION` | Number | Protocol snippet version (8) |

### Character Statistics and Progression

| Variable | Type | Description |
|----------|------|-------------|
| `AFFECTS` | Array | Active spell effects and conditions |
| `INVENTORY` | Array | Character inventory items |
| `ALIGNMENT` | Number | Character alignment (-1000 to 1000) |
| `EXPERIENCE` | Number | Current experience points |
| `EXPERIENCE_MAX` | Number | Experience points at current level |
| `EXPERIENCE_TNL` | Number | Experience points to next level (To Next Level) |
| `HEALTH` | Number | Current hit points |
| `HEALTH_MAX` | Number | Maximum hit points |
| `LEVEL` | Number | Character level |
| `RACE` | String | Character race name |
| `CLASS` | String | Character class name |
| `PSP` | Number | Current psionic spell points |
| `PSP_MAX` | Number | Maximum psionic spell points |
| `WIMPY` | Number | Wimpy flee threshold |
| `PRACTICE` | Number | Practice sessions available |
| `MONEY` | Number | Character wealth in gold |
| `MOVEMENT` | Number | Current movement points |
| `MOVEMENT_MAX` | Number | Maximum movement points |

### Combat and Character Modifiers

| Variable | Type | Description |
|----------|------|-------------|
| `ATTACK_BONUS` | Number | Attack bonus modifier |
| `DAMAGE_BONUS` | Number | Damage bonus modifier |
| `AC` | Number | Armor class |

### Ability Scores (Current)

| Variable | Type | Description |
|----------|------|-------------|
| `STR` | Number | Current strength score |
| `INT` | Number | Current intelligence score |
| `WIS` | Number | Current wisdom score |
| `DEX` | Number | Current dexterity score |
| `CON` | Number | Current constitution score |
| `CHA` | Number | Current charisma score |

### Ability Scores (Permanent)

| Variable | Type | Description |
|----------|------|-------------|
| `STR_PERM` | Number | Permanent strength score |
| `INT_PERM` | Number | Permanent intelligence score |
| `WIS_PERM` | Number | Permanent wisdom score |
| `DEX_PERM` | Number | Permanent dexterity score |
| `CON_PERM` | Number | Permanent constitution score |
| `CHA_PERM` | Number | Permanent charisma score |

### Action Economy

| Variable | Type | Description |
|----------|------|-------------|
| `ACTIONS` | Table | Available actions data (structured data) |
| `STANDARD_ACTION` | Boolean | Standard action available (1 = yes, 0 = no) |
| `MOVE_ACTION` | Boolean | Move action available (1 = yes, 0 = no) |
| `SWIFT_ACTION` | Boolean | Swift action available (1 = yes, 0 = no) |

### Group and Position

| Variable | Type | Description |
|----------|------|-------------|
| `GROUP` | Array | Group members data |
| `POSITION` | String | Current position (standing, sitting, fighting, etc.) |

### Real-Time Combat Information

| Variable | Type | Description |
|----------|------|-------------|
| `OPPONENT_HEALTH` | Number | Current opponent's hit points (as percentage) |
| `OPPONENT_HEALTH_MAX` | Number | Current opponent's maximum hit points (usually 100) |
| `OPPONENT_LEVEL` | Number | Current opponent's level |
| `OPPONENT_NAME` | String | Current opponent's name |
| `TANK_NAME` | String | Group tank's name |
| `TANK_HEALTH` | Number | Group tank's current hit points (as percentage) |
| `TANK_HEALTH_MAX` | Number | Group tank's maximum hit points (usually 100) |

### World and Environment Information

| Variable | Type | Description |
|----------|------|-------------|
| `ROOM` | Table | Complete room information (structured data) |
| `AREA_NAME` | String | Current area/zone name |
| `ROOM_EXITS` | Table | Available exits from current room |
| `ROOM_NAME` | String | Current room name |
| `ROOM_VNUM` | Number | Current room virtual number |
| `WORLD_TIME` | String | Game world time |
| `SECTORS` | Table | Room sector/terrain information |
| `MINIMAP` | String | ASCII minimap representation |

### Client Configuration and Capabilities

| Variable | Type | Description |
|----------|------|-------------|
| `CLIENT_ID` | String | Client software name (configurable by client) |
| `CLIENT_VERSION` | String | Client version string (configurable by client) |
| `PLUGIN_ID` | String | Plugin/script identification (configurable by client) |
| `ANSI_COLORS` | Boolean | ANSI color support (1 = yes, 0 = no) |
| `256_COLORS` | Boolean | 256-color support (1 = yes, 0 = no) |
| `UTF_8` | Boolean | UTF-8 encoding support (1 = yes, 0 = no) |
| `SOUND` | Boolean | Sound/audio support (1 = yes, 0 = no) |
| `MXP` | Boolean | MXP markup support (1 = yes, 0 = no) |

### GUI Element Definitions

These variables are used by compatible GUI clients to display buttons and gauges.

#### Buttons

| Variable | Description |
|----------|-------------|
| `BUTTON_1` | GUI button 1 definition (typically Help) |
| `BUTTON_2` | GUI button 2 definition (typically Look) |
| `BUTTON_3` | GUI button 3 definition (typically Score) |
| `BUTTON_4` | GUI button 4 definition (typically Equipment) |
| `BUTTON_5` | GUI button 5 definition (typically Inventory) |

#### Gauges

| Variable | Description |
|----------|-------------|
| `GAUGE_1` | GUI gauge 1 definition (Health - red) |
| `GAUGE_2` | GUI gauge 2 definition (PSP - blue) |
| `GAUGE_3` | GUI gauge 3 definition (Movement - green) |
| `GAUGE_4` | GUI gauge 4 definition (Experience - yellow) |
| `GAUGE_5` | GUI gauge 5 definition (Opponent Health - dark red) |

## Data Types

### String
Plain text values. Examples: character name, room name, class name.

### Number
Integer values. Examples: hit points, experience, ability scores.

### Boolean
Integer values representing true (1) or false (0).

### Array
A list of values. Example format:
```
AFFECTS "Bless" "Shield" "Detect Magic"
```

### Table
Structured key-value pairs. Example format:
```
ROOM "NAME" "Temple Square" "EXITS" "N" "E" "S" "W" "VNUM" "3001"
```

## Update Frequency

Most MSDP variables are updated automatically once per second through the `msdp_update()` function in the game loop. Some variables trigger immediate updates when they change:

- **Combat variables** - Update when combat starts/ends
- **Room variables** - Update on movement
- **Group variables** - Update when group composition changes
- **Inventory variables** - Update when items are gained/lost
- **Affects variables** - Update when spell effects change

## Usage in Code

### Setting Variables

Variables are set using these functions:

```c
MSDPSetNumber(descriptor, eMSDP_HEALTH, GET_HIT(ch));
MSDPSetString(descriptor, eMSDP_CHARACTER_NAME, GET_NAME(ch));
MSDPSetArray(descriptor, eMSDP_GROUP, group_buffer);
MSDPSetTable(descriptor, eMSDP_ROOM, room_buffer);
```

### Forcing Immediate Updates

To send updates immediately instead of waiting for the next tick:

```c
MSDPFlush(descriptor, eMSDP_HEALTH);  // Flush specific variable
MSDPUpdate(descriptor);                // Flush all changed variables
```

## Client Support

MSDP is supported by many MUD clients including:

- MUSHclient (with MSDP plugin)
- Mudlet
- Beip
- BlowTorch
- And other clients supporting the MSDP protocol

## See Also

- [PROTOCOL_SYSTEMS.md](systems/PROTOCOL_SYSTEMS.md) - Detailed protocol implementation
- [protocol.h](../src/protocol.h) - MSDP implementation in source code
- [comm.c](../src/comm.c) - Main MSDP update loop (msdp_update function)

## References

- MSDP Specification: http://tintin.sourceforge.net/msdp/
- Telnet Protocol: RFC 854
- Telnet Option Negotiation: RFC 855
