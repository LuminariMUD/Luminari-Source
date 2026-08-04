# LuminariMUD Protocol Systems Documentation

Last verified: 2026-08-04

## Overview

LuminariMUD implements **KaVir's Protocol Snippet v8**, a comprehensive suite of MUD client protocols that enhance the player experience through advanced client integration. The protocol system handles telnet negotiation, real-time data exchange, multimedia features, and graphical user interface elements.

**Current Status**: MSDP is integrated into the game loop with real-time
variable updates sent every game pulse (0.1 seconds). Source-side GMCP helper
paths exist as an MSDP fallback and Mudlet package delivery mechanism, but they
are not a stable Luminari Web module contract. MCCP is framework-only until
compression functions are implemented.

## Supported Protocols

### 1. MSDP (Mud Server Data Protocol) - TELOPT 69
**Primary data exchange protocol for real-time game state information**

- **Purpose**: Provides structured data about character stats, world state, and game information
- **Fallback**: Automatically uses GMCP if MSDP unavailable
- **Update Frequency**: Every game pulse (10 times per second)
- **Variables**: 200+ predefined variables covering all game aspects

### 2. GMCP (Generic Mud Communication Protocol) - TELOPT 201
**Source-side MSDP fallback and Mudlet package path**

- **Purpose**: Structured data exchange using JSON format
- **Compatibility**: Used by source helper code when MSDP is unavailable and
  by optional Mudlet package delivery
- **Integration**: Does not currently define source-owned module names,
  versions, schemas, or a Luminari Web proxy/client contract

### 3. MSSP (MUD Server Status Protocol) - TELOPT 70
**Server advertisement and discovery protocol**

- **Purpose**: Provides MUD listing services with server information
- **Data**: Player counts, uptime, codebase info, features
- **Updates**: Player count updated every game pulse

### 4. MXP (MUD eXtension Protocol) - TELOPT 91
**Enhanced markup and interaction protocol**

- **Purpose**: Clickable links, formatted text, GUI elements
- **Security**: Secure line mode prevents client abuse
- **Features**: Hyperlinks, colors, fonts, popup menus

### 5. MSP (MUD Sound Protocol) - TELOPT 90
**Multimedia sound support**

- **Purpose**: Trigger sound effects and music
- **Integration**: Works through MSDP/GMCP or direct MSP
- **Fallback**: Graceful degradation for non-supporting clients

### 6. TTYPE (Terminal Type) - TELOPT 24
**Client identification and capabilities**

- **Purpose**: Identify client software and version
- **Cycling**: Supports multiple TTYPE queries for detailed info
- **Detection**: Automatic client capability detection

### 7. NAWS (Negotiate About Window Size) - TELOPT 31
**Screen size negotiation**

- **Purpose**: Determine client window dimensions
- **Usage**: Optimize display formatting and layout
- **Dynamic**: Updates when client window resized

### 8. CHARSET - TELOPT 42
**Character encoding negotiation**

- **Purpose**: UTF-8 and character set support
- **Unicode**: Full Unicode character support
- **Fallback**: ASCII compatibility maintained

### 9. MCCP (Mud Client Compression Protocol) - TELOPT 86
**Data compression support** *(Framework present, not implemented)*

- **Purpose**: Reduce bandwidth usage via zlib compression
- **Status**: Negotiation supported, compression functions stubbed
- **Version**: MCCP v2 framework (TELOPT 86)
- **Implementation**: Requires CompressStart()/CompressEnd() function implementation

## Luminari Web Boundary

The first-party Luminari Web path uses uncompressed browser WebSocket
application messages to an integrated proxy, then a Telnet socket to source. The
web proxy parses MSDP and rejects MCCP today. It does not negotiate or parse
GMCP modules. Future MCCP support needs real source compression plus proxy
decompression. Future GMCP support needs module ownership, versioning, schemas,
proxy parsing, client mapping, MSDP coexistence rules, fixtures, and rollback.

Native source WebSocket support is not implemented or claimed today. A future
browser-facing source listener would need a dedicated transport design before
exposure: descriptor lifecycle, frame-to-command or application-message
contract, parser isolation, copyover behavior, origin/auth policy, message
validation, connection quotas, command throttles, privacy-safe logging,
WSS/TLS termination, health checks, observability, and rollback to the
integrated Luminari Web proxy. A raw source WebSocket endpoint must not be
treated as equivalent to the current Luminari Web `/ws` JSON application
contract unless a future compatibility spec proves parity.

### Structured Pre-Game Onboarding

The first-party gateway can negotiate a Luminari-specific structured view of
the pre-game `nanny()` states over MSDP. This is separate from ordinary
`msdp_update()`, which primarily reports playing-state HUD data.

Reserved variables are:

| Variable | Purpose |
| --- | --- |
| `LUMINARI_ONBOARDING_VERSION` | V1 capability advertisement |
| `LUMINARI_ONBOARDING_VERSIONS` | Highest-mutual-version negotiation |
| `LUMINARI_ONBOARDING` | Bounded source-to-client state |
| `LUMINARI_ONBOARDING_ACTION` | V2 client-to-source private editor transfer |
| `LUMINARI_ONBOARDING_CONTENT` | V2 source-to-client private editor transfer |

The source remains authoritative for authentication, choices, validation, and
persistence. Ordinary actions resolve to exact source-provided line values and
still pass through `nanny()`. Private multiline profile text uses a separate
bounded, chunked, digested v2 path and never becomes terminal command input.

Clients that do not negotiate a supported version, v1 clients entering a
v2-only state, and clients encountering an unsupported state continue through
the classic terminal. Protocol v2 is compile-time default-off. See
[WEB_ONBOARDING_SYSTEM.md](WEB_ONBOARDING_SYSTEM.md) for the state contract,
bounds, security model, activation, fallback, and test requirements.

## MSDP Variable Categories

### General Server Information
```
CHARACTER_NAME    - Player character name
TITLE             - Player title, empty when unavailable
SERVER_ID         - Server identification
SERVER_TIME       - Current server time
SNIPPET_VERSION   - Protocol snippet version (8)
```

### Character Statistics
```
HEALTH / HEALTH_MAX       - Hit points (current/max)
PSP / PSP_MAX            - Psionic points (current/max)  
MOVEMENT / MOVEMENT_MAX   - Movement points (current/max)
EXPERIENCE / EXPERIENCE_MAX / EXPERIENCE_TNL - Experience system
LEVEL                    - Character level
ALIGNMENT               - Character alignment text
WIMPY                   - Wimpy flee threshold
PRACTICE                - Practice sessions available
MONEY                   - Character wealth
FORTITUDE / REFLEX / WILLPOWER - Saving throw modifiers
```

### Ability Scores
```
STR / STR_PERM          - Strength (current/permanent)
INT / INT_PERM          - Intelligence (current/permanent)
WIS / WIS_PERM          - Wisdom (current/permanent)
DEX / DEX_PERM          - Dexterity (current/permanent)
CON / CON_PERM          - Constitution (current/permanent)
CHA / CHA_PERM          - Charisma (current/permanent)
```

### Combat Information
```
ATTACK_BONUS            - Attack bonus modifier
DAMAGE_BONUS            - Damage bonus modifier (reserved; live emission deferred)
AC                      - Armor class
OPPONENT_HEALTH / OPPONENT_HEALTH_MAX - Current opponent HP
OPPONENT_LEVEL          - Opponent level
OPPONENT_NAME           - Opponent name
TANK_NAME / TANK_HEALTH / TANK_HEALTH_MAX - Group tank info
```

### Character Details
```
RACE                    - Character race
CLASS                   - Character class
POSITION                - Current position (standing, sitting, etc.)
AFFECTS                 - Active spell effects (array)
INVENTORY               - Inventory items (array)
GROUP                   - Group members (array)
```

### World Information
```
ROOM_NAME               - Current room name
ROOM_VNUM               - Current room virtual number
ROOM_EXITS              - Available exits (array)
AREA_NAME               - Current area name
WORLD_TIME              - Game world time
SECTORS                 - Room sector information
MINIMAP                 - ASCII minimap data
GRAPHIC_MAP             - Structured graphical room map data
ROOM                    - Complete room information (table)
```

Current web-client-facing MSDP additions are deliberately narrow. `TITLE`,
`FORTITUDE`, `REFLEX`, `WILLPOWER`, source-backed plain-text `MINIMAP`, and
text `ALIGNMENT` are the supported contract for the current web client work.
`DAMAGE_BONUS` is still deferred because the existing live damage helper can
produce side effects, and structured quest data is not defined yet.

### Action Economy (D&D/Pathfinder)
```
ACTIONS                 - Available actions (table)
STANDARD_ACTION         - Standard action available (boolean)
MOVE_ACTION            - Move action available (boolean)
SWIFT_ACTION           - Swift action available (boolean)
```

### Client Configuration
```
CLIENT_ID              - Client software identification
CLIENT_VERSION         - Client version number
PLUGIN_ID              - Plugin identification
ANSI_COLORS           - ANSI color support (boolean)
256_COLORS            - 256-color support (boolean)
UTF_8                 - UTF-8 encoding support (boolean)
SOUND                 - Sound support (boolean)
MXP                   - MXP support (boolean)
```

### GUI Elements
```
BUTTON_1 through BUTTON_5   - GUI button definitions
GAUGE_1 through GAUGE_5     - GUI gauge/progress bar definitions
```

## Implementation Architecture

#### Protocol Structure (protocol.h)
```c
typedef struct {
    int WriteOOB;                        // Used internally to indicate OOB data
    bool_t Negotiated[eNEGOTIATED_MAX]; // Protocol negotiation status
    bool_t bIACMode;                     // Current mode - deals with broken packets
    bool_t bNegotiated;                  // Indicates client successfully negotiated
    bool_t bRenegotiate;                 // Workaround for clients that autoconnect
    bool_t bNeedMXPVersion;              // Workaround for clients that autoconnect
    bool_t bBlockMXP;                    // Used internally based on MXP version

    // Protocol support flags
    bool_t bTTYPE, bECHO, bNAWS, bCHARSET;
    bool_t bMSDP, bMSSP, bGMCP;
    bool_t bMSP, bMXP, bMCCP;

    support_t b256Support;               // Color support level (eUNKNOWN/eNO/eSOMETIMES/eYES)
    int ScreenWidth, ScreenHeight;       // Client dimensions from NAWS
    char *pMXPVersion;                   // MXP version string
    char *pLastTTYPE;                    // Used for the cyclic TTYPE check
    MSDP_t **pVariables;                 // MSDP variable storage array

    // Per-descriptor buffers to replace global static buffers
    char CmdBuf[MAX_PROTOCOL_BUFFER + 1];  // Command buffer for input processing
    char IacBuf[MAX_PROTOCOL_BUFFER + 1];  // IAC buffer for telnet commands
} protocol_t;
```

#### Variable Management
```c
typedef struct {
    bool_t bReport;           // Client wants this variable reported
    bool_t bDirty;           // Variable needs to be sent
    int ValueInt;            // Numeric value
    char *pValueString;      // String value
} MSDP_t;

// Variable definition structure
typedef struct {
    variable_t Variable;      // The enum type of this variable
    const char *pName;        // The string name of this variable
    bool_t bString;          // Is this variable a string or a number?
    bool_t bConfigurable;    // Can it be configured by the client?
    bool_t bWriteOnce;       // Can only set this variable once
    bool_t bGUI;             // It's a special GUI configuration variable
    int Min, Max;            // Valid value/length ranges
    int Default;             // Default value for numbers
    const char *pDefault;    // Default value for strings
} variable_name_t;
```

### Integration Points

#### Game Loop Integration (comm.c)
The protocol system is integrated into the main game loop via the `msdp_update()` function, called every pulse:

```c
static void msdp_update(void) {
    struct descriptor_data *d;
    struct char_data *ch;

    for (d = descriptor_list; d; d = d->next) {
        if (STATE(d) != CON_PLAYING || !(ch = d->character))
            continue;

        if (IS_NPC(ch))
            continue;

        // Update all MSDP variables for this character
        MSDPSetString(d, eMSDP_CHARACTER_NAME, GET_NAME(ch));
        MSDPSetNumber(d, eMSDP_HEALTH, GET_HIT(ch));
        MSDPSetNumber(d, eMSDP_HEALTH_MAX, GET_MAX_HIT(ch));
        // ... (all other variables)

        // Send dirty variables to client
        MSDPUpdate(d);
    }

    // Update player count for MSSP
    MSSPSetPlayers(PlayerCount);
}
```

#### Character Data Updates
Real-time updates are triggered by specific game events:

- **Health/Stats**: Updated every pulse via `msdp_update()`
- **Combat**: Updated during combat rounds with opponent information
- **Movement**: Updated via `update_msdp_room()` when changing rooms
- **Experience**: Updated every pulse, includes TNL calculations
- **Inventory**: Updated via `update_msdp_inventory()` on item changes
- **Groups**: Updated via `update_msdp_group()` on group formation/changes
- **Affects**: Updated via `update_msdp_affects()` when spells applied/removed
- **Actions**: Updated via `update_msdp_actions()` when action economy changes

#### Specialized Update Functions
```c
void update_msdp_room(struct char_data *ch);        // Room/world data
void update_msdp_affects(struct char_data *ch);     // Active spell effects
void update_msdp_actions(struct char_data *ch);     // D&D action economy
void update_msdp_group(struct char_data *ch);       // Group member data
void update_msdp_inventory(struct char_data *ch);   // Inventory items
```

## Color System

### Standard Color Codes
LuminariMUD uses tab (`\t`) as the color escape character:

#### Basic Colors
```
\tn - Normal (no color)
\tr - Dark red       \tR - Light red
\tg - Dark green     \tG - Light green  
\tb - Dark blue      \tB - Light blue
\ty - Dark yellow    \tY - Light yellow
\tm - Dark magenta   \tM - Light magenta
\tc - Dark cyan      \tC - Light cyan
\tw - Dark white     \tW - Light white
\td - Dark grey      \tD - Light grey
```

#### Extended Colors
```
\ta - Dark azure     \tA - Light azure
\tj - Dark jade      \tJ - Light jade
\tl - Dark lime      \tL - Light lime
\to - Dark orange    \tO - Bright orange
\tp - Dark pink      \tP - Light pink
\tt - Dark tan       \tT - Light tan
\tv - Dark violet    \tV - Light violet
```

#### Special Formatting
```
\t_ - Underlined     \t+ - Bold
\t- - Blinking       \t= - Reverse
\t* - Literal @      \t1-3 - Palette colors
```

#### RGB Colors
```
\t[F500] - Red foreground (RGB: 5,0,0)
\t[B023] - Dark cyan background (RGB: 0,2,3)
```

#### Unicode Support
```
\t[U9973/B] - Boat symbol (Unicode 9973, fallback 'B')
\t[U9814/C] - Castle symbol (Unicode 9814, fallback 'C')
```

## Client Integration Examples

### Basic MSDP Usage
```
MSDP REPORT HEALTH
MSDP REPORT HEALTH_MAX
MSDP REPORT MOVEMENT
```

### GMCP Equivalent
```json
Core.Hello { "client": "MudletClient", "version": "4.15" }
Char.Items.List { "location": "inv" }
Char.StatusVars { "hp": 150, "maxhp": 200 }
```

### GUI Configuration
```
BUTTON_1: [SEND]Help[/SEND]help[END]
GAUGE_1: [NAME]Health[COLOR]red[VAR]HEALTH[MAX]HEALTH_MAX[END]
```

## Performance Considerations

### Update Frequency
- **MSDP Variables**: Updated every 0.1 seconds (game pulse)
- **Dirty Flag System**: Only changed variables are transmitted
- **Batch Updates**: All variables sent in single MSDP packet

### Memory Management
- **Variable Storage**: Pre-allocated arrays for all variables
- **String Caching**: Duplicate strings avoided
- **Protocol Cleanup**: Proper memory deallocation on disconnect

### Network Optimization
- **Compression Ready**: MCCP framework for future implementation
- **Efficient Encoding**: Binary MSDP format reduces bandwidth
- **Smart Updates**: Only dirty variables transmitted

## Configuration Options

#### Campaign-Specific MUD Names (protocol.h)
```c
#if defined(CAMPAIGN_DL)
   #define MUD_NAME "Chronicles of Krynn"
#elif defined(CAMPAIGN_FR)
   #define MUD_NAME "Faerun: A Forgotten Realms MUD"
#else
   #define MUD_NAME "LuminariMUD"
#endif
```

#### Protocol Settings (protocol.h)
```c
#define SNIPPET_VERSION 8                           // Protocol version
#define COLOUR_ON_BY_DEFAULT true                   // Default color state
// Mudlet keys packages by URL basename; keep LuminariGUI.mpackage stable.
#define MUDLET_PACKAGE \
  "{\"version\":\"5\",\"url\":\"https://luminarimud.com/download/LuminariGUI.mpackage\"}"
```

> Full wiring of the Mudlet auto-download (config toggle, cedit option, and the
> two GMCP/TTYPE send paths) is documented in
> [MUDLET_GUI_PACKAGE.md](MUDLET_GUI_PACKAGE.md).

#### Compile-Time Options (commented out by default)
```c
/* #define USING_MCCP */         // Enable compression support
/* #define EXTENDED_COLOUR */    // Enable RGB color codes  
/* #define COLOUR_CHAR '^' */    // Alternative color character
/* #define DISPLAY_INVALID_COLOUR_CODES */ // Show invalid codes
```

#### Buffer Sizes
```c
#define MAX_PROTOCOL_BUFFER (12 * 1024)  // Same as MAX_RAW_INPUT_LENGTH
#define MAX_VARIABLE_LENGTH 16384
#define MAX_OUTPUT_BUFFER LARGE_BUFSIZE
#define MAX_MSSP_BUFFER 4096
#define MAX_MSSP_PAIR 128
```

## Security Features

### MXP Security
- **Secure Line Mode**: Prevents client command injection
- **Bounded Tags**: Outbound tags longer than `MAX_MXP_TAG_LENGTH` are rejected
- **Escape Sequences**: Proper escape sequence handling

### Input Validation
- **Persistent Telnet State**: Split IAC negotiation and subnegotiation frames
  are retained across socket reads
- **Buffer Limits**: Oversized command text is truncated; oversized Telnet,
  MXP, MSDP, MSSP, and output frames are rejected or discarded without
  disconnecting the session
- **Short Payloads**: NAWS and CHARSET payload sizes are checked before access
- **Command Validation**: Client commands validated before execution
- **Character Filtering**: Control character filtering
- **Public API Validation**: Fallible protocol actions reject null and invalid
  inputs with `protocol_error_t`

## Debugging and Troubleshooting

### Common Issues

#### Protocol Negotiation Failures
```c
// Check negotiation status
if (!descriptor->pProtocol->bMSDP) {
    log("MSDP negotiation failed for %s", GET_NAME(ch));
}
```

#### Variable Update Problems
```c
// Force variable update
MSDPSetNumber(descriptor, eMSDP_HEALTH, GET_HIT(ch));
MSDPFlush(descriptor, eMSDP_HEALTH);  // Immediate send
```

#### Client Compatibility
```c
// Detect client capabilities
if (descriptor->pProtocol->b256Support == eYES) {
    // Use 256-color mode
} else {
    // Fallback to 16-color ANSI
}
```

### Logging
```c
// Protocol debugging
log("Protocol: MSDP %s, GMCP %s, MXP %s",
    protocol->bMSDP ? "YES" : "NO",
    protocol->bGMCP ? "YES" : "NO",
    protocol->bMXP ? "YES" : "NO");
```

## API Reference

### Core Functions

#### Protocol Lifecycle
```c
protocol_t *ProtocolCreate(void);                    // Create protocol structure
void ProtocolDestroy(protocol_t *protocol);         // Clean up protocol data
protocol_error_t ProtocolNegotiate(descriptor_t *descriptor); // Start negotiation
protocol_error_t ProtocolNoEcho(descriptor_t *descriptor, bool_t on);
```

#### Input/Output Processing
```c
ssize_t ProtocolInput(descriptor_t *d, char *data, int size, char *out);
const char *ProtocolOutput(descriptor_t *d, const char *data, int *length);
```

#### MSDP Variable Management
```c
protocol_error_t MSDPUpdate(descriptor_t *descriptor);
protocol_error_t MSDPFlush(descriptor_t *descriptor, variable_t var);
protocol_error_t MSDPSetNumber(descriptor_t *d, variable_t var, int val);
protocol_error_t MSDPSetString(descriptor_t *d, variable_t var, const char *val);
protocol_error_t MSDPSetArray(descriptor_t *d, variable_t var, const char *val);
protocol_error_t MSDPSetTable(descriptor_t *d, variable_t var, const char *val);
```

#### Direct Protocol Communication
```c
protocol_error_t MSDPSend(descriptor_t *descriptor, variable_t var);
protocol_error_t MSDPSendPair(descriptor_t *d, const char *var, const char *val);
protocol_error_t MSDPSendList(descriptor_t *d, const char *var, const char *val);
```

#### MSSP Functions
```c
void MSSPSetPlayers(int player_count);  // Update player count for MSSP
```

#### MXP Functions
```c
const char *MXPCreateTag(descriptor_t *d, const char *tag);  // Create MXP tag
protocol_error_t MXPSendTag(descriptor_t *descriptor, const char *tag);
```

#### Sound Functions
```c
protocol_error_t SoundSend(descriptor_t *descriptor, const char *trigger);
```

#### Color Functions
```c
const char *ColourRGB(descriptor_t *d, const char *rgb);  // Convert RGB to escape codes
```

#### Unicode Functions
```c
char *UnicodeGet(int unicode_value);                          // Get UTF-8 sequence
protocol_error_t UnicodeAdd(char **string, int unicode_value);
```

#### Copyover Support
```c
const char *CopyoverGet(descriptor_t *descriptor);           // Get protocol state
protocol_error_t CopyoverSet(descriptor_t *descriptor, const char *data);
```

Fallible actions return `PROTOCOL_SUCCESS` or a negative
`protocol_error_t`. `ProtocolInput()` returns a nonnegative command-byte count
on success or a negative protocol error for invalid arguments. Oversized
network data is handled in place and does not use an error return to request a
disconnect.

### Usage Patterns

#### Basic Variable Updates
```c
// In character update functions
MSDPSetNumber(ch->desc, eMSDP_HEALTH, GET_HIT(ch));
MSDPSetNumber(ch->desc, eMSDP_HEALTH_MAX, GET_MAX_HIT(ch));
MSDPSetString(ch->desc, eMSDP_ROOM_NAME, world[IN_ROOM(ch)].name);

// Bulk update at end of game pulse
MSDPUpdate(ch->desc);
```

#### Combat Information
```c
// During combat rounds
if (FIGHTING(ch)) {
    MSDPSetString(ch->desc, eMSDP_OPPONENT_NAME, GET_NAME(FIGHTING(ch)));
    MSDPSetNumber(ch->desc, eMSDP_OPPONENT_HEALTH, GET_HIT(FIGHTING(ch)));
    MSDPSetNumber(ch->desc, eMSDP_OPPONENT_HEALTH_MAX, GET_MAX_HIT(FIGHTING(ch)));
}
```

#### Group Information
```c
// Group member list as MSDP array
snprintf(msdp_buffer, sizeof(msdp_buffer), "%c%s%c%s%c%s",
    MSDP_VAL, "PlayerOne",
    MSDP_VAL, "PlayerTwo",
    MSDP_VAL, "PlayerThree");
MSDPSetArray(ch->desc, eMSDP_GROUP, msdp_buffer);
```

#### Room Information Table
```c
// Room data as MSDP table
snprintf(msdp_buffer, sizeof(msdp_buffer), "%cVNUM%c%d%cNAME%c%s%cEXITS%c%s",
    MSDP_VAR, MSDP_VAL, world[IN_ROOM(ch)].number,
    MSDP_VAR, MSDP_VAL, world[IN_ROOM(ch)].name,
    MSDP_VAR, MSDP_VAL, exit_string);
MSDPSetTable(ch->desc, eMSDP_ROOM, msdp_buffer);
```

## Integration with Game Systems

### Character Sheet Integration
The protocol system automatically provides character sheet data to compatible clients:

- **Statistics**: All ability scores (STR, INT, WIS, DEX, CON, CHA)
- **Resources**: Health, PSP, Movement points
- **Progression**: Level, experience, experience to next level
- **Combat**: Attack bonus, damage bonus, armor class
- **Status**: Current position, active affects

### Combat System Integration
Real-time combat information is transmitted during battles:

- **Opponent Data**: Health, level, name of current opponent
- **Tank Information**: Group tank status and health
- **Action Economy**: Available actions (standard, move, swift)
- **Combat Status**: Current combat position and state

### World System Integration
Location and environment data is continuously updated:

- **Room Information**: Name, description, virtual number
- **Area Data**: Current area name and zone information
- **Exit Information**: Available movement directions
- **Minimap**: ASCII representation of surrounding area
- **Sector Data**: Terrain and environment type

### Inventory System Integration
Item and equipment information is transmitted on changes:

- **Inventory Lists**: Complete inventory as MSDP array
- **Equipment Status**: Worn/wielded items
- **Item Updates**: Real-time inventory changes
- **Container Contents**: Contents of bags and containers

## Current Implementation Status

### Fully Implemented Features
- **MSDP**: Complete variable system with real-time updates
- **MSSP**: Server status reporting with player count updates
- **MXP**: Basic tag support with secure line mode
- **MSP**: Sound trigger support via MSDP/GMCP
- **TTYPE/NAWS/CHARSET**: Full telnet option negotiation
- **Color System**: Tab-based color codes with RGB support framework

### Framework Present (Not Implemented)
- **MCCP**: Compression negotiation works, but CompressStart()/CompressEnd() are stubbed
- **GMCP web modules**: Source helper paths exist, but Luminari Web module
  schemas, proxy parsing, client mapping, and fixtures are not defined
- **Extended RGB Colors**: Code present but commented out by default
- **Advanced MXP**: Basic support only, rich formatting not implemented

## Performance Characteristics

### Update Frequency
- **MSDP Variables**: Updated every 0.1 seconds (game pulse)
- **MSSP Player Count**: Updated every pulse
- **Event-Driven Updates**: Immediate updates for inventory, affects, groups, room changes
- **Dirty Flag System**: Only changed variables transmitted

### Memory Management
- **Per-Descriptor Buffers**: Each connection has separate protocol buffers
- **Variable Storage**: Zero-initialized MSDP structures and bounded strings
- **String Handling**: Proper allocation/deallocation with NULL checks
- **Cleanup**: Protocol structures properly freed on disconnect

### Network Optimization
- **Batch Updates**: All dirty variables sent in single MSDP packet
- **Binary Format**: Efficient MSDP encoding reduces bandwidth
- **Smart Updates**: Only reports variables client has requested
- **OOB Handling**: Out-of-band data properly flagged and transmitted

## Implementation Notes

### Key Differences from Standard Snippet
1. **Campaign Support**: MUD name changes based on compile-time campaign flags
2. **Specialized Updates**: Dedicated functions for different data types (affects, actions, inventory)
3. **D&D Integration**: Action economy variables (STANDARD_ACTION, MOVE_ACTION, SWIFT_ACTION)
4. **Pathfinder Stats**: Complete ability score tracking with permanent values
5. **Memory Safety**: Enhanced error checking and per-descriptor buffers

### Integration with Game Systems
- **Combat System**: Real-time opponent and tank information
- **Spell System**: Automatic affect updates when spells applied/removed  
- **Action Economy**: D&D-style action tracking with GUI integration
- **Group System**: Dynamic group member updates
- **Inventory System**: Real-time item change notifications
- **World System**: Room information with minimap support

### GUI Elements
The system includes predefined GUI configurations for compatible clients:
- **5 Buttons**: Help, Look, Score, Equipment, Inventory
- **5 Gauges**: Health (red), PSP (blue), Movement (green), Experience (yellow), Opponent Health (dark red)

## Troubleshooting

### Common Issues
1. **No Protocol Response**: Check if client supports MSDP/GMCP
2. **Missing Variables**: Verify client has sent REPORT commands
3. **Memory Leaks**: Ensure proper cleanup in update functions
4. **Color Problems**: Check tab character handling in client

### Debug Commands
```c
// Log protocol negotiation status
log("Protocol: MSDP %s, GMCP %s, MXP %s",
    d->pProtocol->bMSDP ? "YES" : "NO",
    d->pProtocol->bGMCP ? "YES" : "NO",
    d->pProtocol->bMXP ? "YES" : "NO");

// Force variable flush
MSDPFlush(d, eMSDP_HEALTH);  // Send specific variable immediately
```

---

*This documentation reflects the actual implementation of KaVir's Protocol Snippet v8 in LuminariMUD as of 2025. For implementation details, refer to `protocol.h` and `protocol.c`. For integration points, see `comm.c`, `handler.c`, `actions.c`, and `act.other.c`.*
