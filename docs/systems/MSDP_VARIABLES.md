# MSDP Variables Reference

This document lists the MSDP (Mud Server Data Protocol) variables that LuminariMUD exposes to
compatible clients over native MSDP or the standard MSDP-over-GMCP fallback.

Last verified: 2026-08-05

## Overview

MSDP is a Telnet protocol extension for real-time structured data exchange. The server updates
reported variables for clients that negotiate native MSDP. A client that negotiates GMCP but
not native MSDP receives the same logical values in the case-sensitive `MSDP` GMCP package.

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
| `AFFECTS` | Table | `AFFECTED_BY` and `SPELL_LIKE_AFFECTS` arrays |
| `INVENTORY` | Array | Character inventory items |
| `ALIGNMENT` | String | Character alignment text, such as Lawful Good or Neutral Evil |
| `TITLE` | String | Player title, or an empty string when no title is available |
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
| `FORTITUDE` | Number | Current Fortitude saving throw modifier |
| `REFLEX` | Number | Current Reflex saving throw modifier |
| `WILLPOWER` | Number | Current Will saving throw modifier |

### Combat and Character Modifiers

| Variable | Type | Description |
|----------|------|-------------|
| `ATTACK_BONUS` | Number | Attack bonus modifier |
| `DAMAGE_BONUS` | Number | Damage bonus modifier. Reserved in the table, but live emission is deferred until a side-effect-free calculation is available |
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
| `WORLD_TIME` | Number | Game world time value |
| `SECTORS` | Table | Room sector/terrain information |
| `MINIMAP` | String | Plain ASCII minimap representation with source color codes stripped, or an empty string when the map is unavailable |
| `GRAPHIC_MAP` | Table | Structured 21x21 room map for graphical clients |

`GRAPHIC_MAP` uses a compact MSDP table with this shape:

- `ver`: payload version, currently `2`
- `radius`: map radius, currently `10`
- `rooms`: array of room tables

Each room table contains:

- `x`: relative X offset from the current room
- `y`: relative Y offset from the current room
- `v`: room vnum
- `s`: numeric sector type
- `i`: `1` if `ROOM_INDOORS` is set, otherwise `0`
- `c`: optional directional connector bitmask, ordered N/E/S/W/NW/NE/SE/SW
- `sp`: optional special-exit markers string using `u`, `d`, `i`, `o`

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
Client-facing text without LuminariMUD's internal tab-color directives. Examples: character
name, room name, and class name. Scalar content cannot contain NUL, Telnet IAC, or the six
reserved MSDP marker bytes.

### Number
Integer values. Examples: hit points, experience, ability scores.

### Boolean
Integer values representing true (1) or false (0).

### Array
A list of values. Example format:
```
GROUP "Alice" "Bob" "Charlie"
```

### Table
Structured key-value pairs. Example format:
```
ROOM "NAME" "Temple Square" "EXITS" "N" "E" "S" "W" "VNUM" "3001"
```

`AFFECTS` is a table whose `AFFECTED_BY` and `SPELL_LIKE_AFFECTS` members are arrays of
tables. `ROOM_EXITS` is a table keyed by direction name.

## Wire Encodings

- Native clients receive MSDP marker framing inside `IAC SB MSDP ... IAC SE`.
- GMCP-only clients receive one strict UTF-8 JSON object in the exact package form
  `MSDP {"VARIABLE":value}`. Top-level number variables become JSON numbers, scalar variables
  become JSON strings, and MSDP tables and arrays become JSON objects and arrays.
- Strings are JSON-escaped and the frame bound is checked after escaping. Invalid UTF-8,
  malformed structures, unsupported marker bytes in scalar values, and oversized output are
  rejected without queuing a partial frame.
- Inbound fallback commands use the same case-sensitive package, for example
  `MSDP {"REPORT":["HEALTH","TITLE"]}`. Command values may be strings, integers, Booleans,
  or arrays of those scalar types. Malformed JSON, embedded NUL, invalid UTF-8, and unsupported
  nesting are rejected before any command in the object is applied.

## Update Frequency

Most MSDP variables are evaluated once per second by `msdp_update()` in the game loop. Only
reported dirty values are transmitted. Some variables trigger immediate updates when they
change:

- **Combat variables** - Update when combat starts/ends
- **Room variables** - Update on movement
- **Group variables** - Update when group composition changes
- **Inventory variables** - Update when items are gained/lost
- **Affects variables** - Update when spell effects change

## Current Contract Notes

- `ALIGNMENT`, `AREA_NAME`, and `ROOM_NAME` are copied and stripped of internal color markup at
  the protocol boundary. Their canonical colored sources are not modified.
- `AFFECTS` is stored as table content through `MSDPSetTable()` and is available over either
  native MSDP or the GMCP fallback.
- `TITLE` is emitted as a plain string from player title data, with an empty string for missing
  titles.
- `FORTITUDE`, `REFLEX`, and `WILLPOWER` are signed integer saving throw modifiers from
  `compute_mag_saves()`.
- `MINIMAP` is emitted from automap data after source color stripping; unavailable maps emit an
  empty string.
- `DAMAGE_BONUS` remains table-reserved but is not emitted from the game pulse until the damage
  calculation path is side-effect-free.
- Structured quest data is not part of the current MSDP contract.

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

- [PROTOCOL_SYSTEMS.md](PROTOCOL_SYSTEMS.md) - Detailed protocol implementation
- [protocol.h](../../src/net/protocol.h) - MSDP implementation in source code
- [msdp_json.c](../../src/net/msdp_json.c) - MSDP and JSON conversion
- [comm.c](../../src/comm.c) - Main MSDP update loop (msdp_update function)

## References

- [MSDP specification](https://tintin.mudhalla.net/protocols/msdp/)
- [MSDP over GMCP](https://mudstandards.org/gmcp/msdp/)
- Telnet Protocol: RFC 854
- Telnet Option Negotiation: RFC 855
