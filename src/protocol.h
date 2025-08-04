/******************************************************************************
 *                                                       LuminariMUD
 *                           PROTOCOL SYSTEM HEADER
 * 
 * Protocol snippet by KaVir.  Released into the Public Domain in February 2011.
 * 
 * OVERVIEW:
 * This file defines the complete protocol system for LuminariMUD, implementing
 * KaVir's Protocol Snippet v8. The system provides advanced client-server
 * communication through multiple protocols including MSDP, GMCP, MXP, MSP,
 * MSSP, and standard telnet options.
 * 
 * SUPPORTED PROTOCOLS:
 * - MSDP (Mud Server Data Protocol): Real-time game data exchange
 * - GMCP (Generic Mud Communication Protocol): JSON-based alternative to MSDP
 * - MXP (MUD eXtension Protocol): Enhanced markup and clickable elements
 * - MSP (MUD Sound Protocol): Audio triggers and sound effects
 * - MSSP (MUD Server Status Protocol): Server advertisement and discovery
 * - TTYPE: Client identification and terminal type detection
 * - NAWS: Window size negotiation for optimal display formatting
 * - CHARSET: Character encoding negotiation (UTF-8 support)
 * - MCCP: Compression protocol framework (not fully implemented)
 * 
 * KEY FEATURES:
 * - Real-time variable updates every game pulse (0.1 seconds)
 * - 200+ predefined MSDP variables covering all game aspects
 * - Color system with RGB support and Unicode characters
 * - GUI elements (buttons and gauges) for compatible clients
 * - Campaign-specific MUD names (Dragonlance, Forgotten Realms, LuminariMUD)
 * - Memory-safe per-descriptor buffers
 * - Comprehensive error handling and debugging support
 * 
 * INTEGRATION:
 * The protocol system integrates seamlessly with:
 * - Game loop (comm.c) for pulse-based updates
 * - Character system for stats and progression data
 * - Combat system for real-time opponent information
 * - World system for room and area data
 * - Inventory system for item change notifications
 * - Group system for party member updates
 * 
 * USAGE:
 * 1. Call ProtocolCreate() when initializing a new connection
 * 2. Call ProtocolNegotiate() to start protocol negotiation
 * 3. Use MSDPSetNumber/String/Array/Table to update variables
 * 4. Call MSDPUpdate() regularly (every pulse) to send dirty variables
 * 5. Call ProtocolDestroy() when cleaning up the connection
 * 
 * NEWBIE NOTES:
 * - All protocol functions are thread-safe for single-threaded MUD architecture
 * - Memory management is handled automatically for most operations
 * - Error checking is built into all public functions
 * - Compatible with all major MUD clients (Mudlet, MUSHclient, etc.)
 * - Graceful fallback for clients that don't support advanced protocols
 * 
 ******************************************************************************/

#ifndef PROTOCOL_H
#define PROTOCOL_H

/******************************************************************************
 *                           CAMPAIGN CONFIGURATION
 * 
 * MUD_NAME: Automatically set based on campaign compilation flags:
 * - CAMPAIGN_DL: "Chronicles of Krynn" (Dragonlance campaign)
 * - CAMPAIGN_FR: "Faerun: A Forgotten Realms MUD" (Forgotten Realms)
 * - Default: "LuminariMUD" (standard fantasy campaign)
 * 
 * The descriptor_t type is defined by the MUD's communication system and
 * represents a single client connection with all associated protocol data.
 *****************************************************************************/

#if defined(CAMPAIGN_DL)
   #define MUD_NAME "Chronicles of Krynn"
#elif defined(CAMPAIGN_FR)
   #define MUD_NAME "Faerun: A Forgotten Realms MUD"
#else
   #define MUD_NAME "LuminariMUD"
#endif

/* for ssize_t and friends */
#include "conf.h"
#include "sysdep.h"

typedef struct descriptor_data descriptor_t;

/******************************************************************************
 *                           COLOR SYSTEM CONFIGURATION
 * 
 * LuminariMUD uses '\t' (tab character) as the color escape sequence instead
 * of traditional '^' or '&' characters. This prevents players from sending
 * color codes to each other while allowing easy inclusion in help files.
 * 
 * COLOR FEATURES:
 * - Basic ANSI colors (16 colors): \tr (red), \tg (green), \tb (blue), etc.
 * - Extended colors: \ta (azure), \tj (jade), \tl (lime), etc.
 * - Special formatting: \t_ (underline), \t+ (bold), \t- (blink), \t= (reverse)
 * - RGB colors: \t[F500] (red foreground), \t[B023] (dark cyan background)
 * - Unicode support: \t[U9973/B] (boat symbol with 'B' fallback)
 * - Palette colors: \t1, \t2, \t3 (customizable palette colors)
 * 
 * CONFIGURATION OPTIONS:
 ******************************************************************************/

/**
 * Alternative color character (currently disabled)
 * 
 * Traditional MUDs use '^' or '&' as color escape characters. LuminariMUD
 * uses '\t' (tab) instead to prevent player abuse and ensure compatibility
 * with help files and strings.
 * 
 * @note Uncomment and modify if you want to use a different color character
 */
/*
#define COLOUR_CHAR '^'
*/

/**
 * Default color state for new connections
 * 
 * When set to true, new players will see colors by default. When false,
 * players must manually enable colors with client commands.
 * 
 * @default true - Colors enabled by default for better user experience
 */
#define COLOUR_ON_BY_DEFAULT true

/**
 * Extended RGB color support (currently disabled)
 * 
 * Enables advanced RGB color codes like "\t[F135]" for custom colors.
 * Requires client support for 256-color or true-color terminals.
 * 
 * @note Uncomment to enable RGB color extensions
 * @warning Increases bandwidth usage and may not work with all clients
 */
/*
#define EXTENDED_COLOUR
*/

/**
 * Invalid color code display (currently disabled)
 * 
 * When enabled, invalid color codes are displayed to the user instead of
 * being silently removed. Useful for debugging color code issues.
 * 
 * @note Uncomment for debugging color code problems
 */
/*
#define DISPLAY_INVALID_COLOUR_CODES
*/

/******************************************************************************
 *                           COMPRESSION SUPPORT
 * 
 * MCCP (Mud Client Compression Protocol) v2 support framework.
 * 
 * STATUS: Framework implemented, compression functions stubbed
 * 
 * The protocol system includes full MCCP negotiation support, but the actual
 * compression functions (CompressStart/CompressEnd) are not implemented.
 * To enable compression:
 * 
 * 1. Uncomment the USING_MCCP define below
 * 2. Implement CompressStart() and CompressEnd() functions in protocol.c
 * 3. Link with zlib compression library
 * 
 * BENEFITS:
 * - Reduces bandwidth usage by up to 80% for text-heavy output
 * - Improves performance for players on slow connections
 * - Transparent to both client and server application code
 * 
 * REQUIREMENTS:
 * - zlib development libraries
 * - Client support for MCCP v2 (most modern clients support this)
 ******************************************************************************/

/**
 * Enable MCCP (Mud Client Compression Protocol) support
 * 
 * Uncomment this line after implementing compression functions to enable
 * automatic compression negotiation with supporting clients.
 * 
 * @note Requires zlib and implementation of CompressStart/CompressEnd functions
 */
/*
#define USING_MCCP
*/

/******************************************************************************
 *                           CLIENT GUI INTEGRATION
 * 
 * Automatic GUI package installation for Mudlet clients.
 * 
 * When a Mudlet client connects, the protocol system can automatically offer
 * to install a custom GUI package that provides:
 * - Real-time health/mana/movement bars
 * - Clickable command buttons
 * - Minimap display
 * - Combat information panels
 * - Inventory and equipment windows
 * 
 * The package is offered via GMCP negotiation and installed with user consent.
 * Only available for the default LuminariMUD campaign (not DL/FR variants).
 ******************************************************************************/

/**
 * Mudlet GUI package for automatic installation
 * 
 * Defines the download URL and version for the LuminariMUD GUI package.
 * Format: "version\ndownload_url"
 * 
 * @note Only defined for default LuminariMUD campaign
 * @note Dragonlance and Forgotten Realms campaigns use different/no GUIs
 */
#if !defined(CAMPAIGN_DL)
#define MUDLET_PACKAGE "4\nhttps://luminarimud.com/download/LuminariGUI-v2.0.4.015.mpackage"
#endif

/******************************************************************************
 *                           PROTOCOL CONSTANTS
 * 
 * Core constants defining protocol behavior, buffer sizes, telnet options,
 * and MSDP/MSSP data format specifications.
 * 
 * These values are based on RFC specifications and industry standards for
 * MUD client-server communication protocols.
 *****************************************************************************/

/** Protocol snippet version number for compatibility checking */
#define SNIPPET_VERSION 8 /* KaVir's Protocol Snippet v8 */

/** 
 * Protocol buffer size limits for safe memory management
 * 
 * These limits prevent buffer overflows and ensure reasonable memory usage
 * while allowing for complex protocol data exchange.
 */
#define MAX_PROTOCOL_BUFFER (12 * 1024)  /**< Main protocol buffer (matches MAX_RAW_INPUT_LENGTH) */
#define MAX_VARIABLE_LENGTH 4096         /**< Maximum length for MSDP variable values */
#define MAX_OUTPUT_BUFFER LARGE_BUFSIZE  /**< Output buffer for processed protocol data */
#define MAX_MSSP_BUFFER 4096             /**< Buffer for MSSP server status data */

/**
 * Telnet negotiation states
 * 
 * These constants represent the three possible states during telnet option
 * negotiation between client and server.
 */
#define SEND 1      /**< Send negotiation request to client */
#define ACCEPTED 2  /**< Client accepted the option */
#define REJECTED 3  /**< Client rejected the option */

/**
 * Telnet option codes for MUD protocols
 * 
 * These are the official IANA-assigned telnet option numbers for MUD-specific
 * protocols. Some are experimental (200+) but widely adopted by the MUD community.
 */
#define TELOPT_CHARSET 42   /**< Character set negotiation (RFC 2066) */
#define TELOPT_MSDP 69      /**< Mud Server Data Protocol (official) */
#define TELOPT_MSSP 70      /**< MUD Server Status Protocol (official) */
#define TELOPT_MCCP 86      /**< MUD Client Compression Protocol v2 (official) */
#define TELOPT_MSP 90       /**< MUD Sound Protocol (official) */
#define TELOPT_MXP 91       /**< MUD eXtension Protocol (official) */
#define TELOPT_GMCP 201     /**< Generic MUD Communication Protocol (experimental) */

/**
 * MSDP (Mud Server Data Protocol) data type markers
 * 
 * These byte values are used in the binary MSDP format to structure
 * complex data types like arrays and tables. They form the foundation
 * of MSDP's type-safe data exchange system.
 * 
 * Example MSDP array: VAR "AFFECTS" VAL "blind" VAL "haste" VAL "fly"
 * Example MSDP table: VAR "ROOM" VAR "NAME" VAL "Temple" VAR "EXITS" VAL "north"
 */
#define MSDP_VAR 1          /**< Variable name marker */
#define MSDP_VAL 2          /**< Variable value marker */
#define MSDP_TABLE_OPEN 3   /**< Start of MSDP table structure */
#define MSDP_TABLE_CLOSE 4  /**< End of MSDP table structure */
#define MSDP_ARRAY_OPEN 5   /**< Start of MSDP array structure */
#define MSDP_ARRAY_CLOSE 6  /**< End of MSDP array structure */
#define MAX_MSDP_SIZE 200   /**< Maximum number of MSDP variables */

/**
 * MSSP (MUD Server Status Protocol) data markers
 * 
 * Simple variable/value pair markers for server status information
 * used by MUD listing services and monitoring tools.
 */
#define MSSP_VAR 1  /**< MSSP variable name marker */
#define MSSP_VAL 2  /**< MSSP variable value marker */

/**
 * Unicode character constants for gender symbols
 * 
 * These Unicode code points provide gender symbols that can be displayed
 * by UTF-8 capable clients, with ASCII fallbacks for older clients.
 */
#define UNICODE_MALE 9794    /**< ♂ Male symbol (U+2642) */
#define UNICODE_FEMALE 9792  /**< ♀ Female symbol (U+2640) */
#define UNICODE_NEUTER 9791  /**< ⚿ Neuter symbol (U+26BF) */

/**
 * Additional protocol buffer and parsing constants
 * 
 * These constants define buffer sizes for various protocol parsing operations
 * and optimization features like hash table lookups.
 */
#define MAX_MXP_TAG_LENGTH 1000        /**< Maximum length for MXP markup tags */
#define MAX_UNICODE_BUFFER 8           /**< Buffer size for UTF-8 character encoding */
#define MAX_COLOR_CODE_LENGTH 8        /**< Maximum length for color escape sequences */
#define PROTOCOL_PARSE_BUFFER_SIZE 8192 /**< Buffer for parsing protocol commands */

/**
 * MSDP variable lookup optimization
 * 
 * Hash table size for fast MSDP variable name lookups. Should be a prime number
 * larger than the number of MSDP variables for optimal distribution.
 */
#define MSDP_HASH_TABLE_SIZE 127       /**< Prime number for hash table efficiency */

/******************************************************************************
 *                           TYPE DEFINITIONS
 * 
 * Core data types and enumerations used throughout the protocol system.
 * These types provide type safety, clear interfaces, and structured data
 * for all protocol operations.
 * 
 * DESIGN PRINCIPLES:
 * - Type safety: Enums prevent invalid values
 * - Clear semantics: Descriptive names indicate purpose
 * - Memory efficiency: Packed structures where appropriate
 * - Extensibility: Room for future protocol additions
 *****************************************************************************/

/**
 * Boolean type for protocol system
 * 
 * Simple boolean enumeration used throughout the protocol system.
 * Provides type safety and clear semantics for boolean operations.
 * 
 * @note Used instead of C99 bool for C90 compatibility
 */
typedef enum
{
   bool_t_false = 0,  /**< False value (0) */
   bool_t_true = 1    /**< True value (1) */
} bool_t;

/**
 * Protocol error codes for standardized error handling
 * 
 * These error codes provide consistent error reporting across all protocol
 * functions. They follow Unix convention with 0 for success and negative
 * values for different error conditions.
 * 
 * @usage Check return values from protocol functions:
 * @code
 *   protocol_error_t result = SomeProtocolFunction(...);
 *   if (result != PROTOCOL_SUCCESS) {
 *       log("Protocol error: %d", result);
 *       return;
 *   }
 * @endcode
 */
typedef enum
{
   PROTOCOL_SUCCESS = 0,              /**< Operation completed successfully */
   PROTOCOL_ERROR_MEMORY = -1,        /**< Memory allocation failed */
   PROTOCOL_ERROR_INVALID_INPUT = -2, /**< Invalid input parameters */
   PROTOCOL_ERROR_BUFFER_FULL = -3,   /**< Buffer overflow prevented */
   PROTOCOL_ERROR_NULL_POINTER = -4   /**< Null pointer passed to function */
} protocol_error_t;

/**
 * Protocol negotiation status enumeration
 * 
 * Tracks which protocols have been successfully negotiated with the client.
 * Used as array indices in the protocol_t structure for fast lookups.
 * 
 * NEGOTIATION PROCESS:
 * 1. Server sends WILL/DO for each protocol
 * 2. Client responds with DO/WILL or DONT/WONT
 * 3. Successful negotiations are marked in the Negotiated[] array
 * 4. Protocol-specific initialization occurs after negotiation
 * 
 * @note eNEGOTIATED_MAX must always be the last entry for array sizing
 */
typedef enum
{
   eNEGOTIATED_TTYPE,    /**< Terminal type negotiation completed */
   eNEGOTIATED_ECHO,     /**< Echo control negotiation completed */
   eNEGOTIATED_NAWS,     /**< Window size negotiation completed */
   eNEGOTIATED_CHARSET,  /**< Character set negotiation completed */
   eNEGOTIATED_MSDP,     /**< MSDP protocol negotiation completed */
   eNEGOTIATED_MSSP,     /**< MSSP protocol negotiation completed */
   eNEGOTIATED_GMCP,     /**< GMCP protocol negotiation completed */
   eNEGOTIATED_MSP,      /**< MSP sound protocol negotiation completed */
   eNEGOTIATED_MXP,      /**< MXP markup protocol negotiation completed */
   eNEGOTIATED_MXP2,     /**< MXP version 2 features negotiated */
   eNEGOTIATED_MCCP,     /**< MCCP compression negotiation completed */

   eNEGOTIATED_MAX       /**< Array size marker - must always be last */
} negotiated_t;

/**
 * Feature support level enumeration
 * 
 * Represents the level of support a client has for specific features
 * like 256-color support, Unicode, etc. Allows for nuanced feature detection.
 * 
 * SUPPORT LEVELS:
 * - eUNKNOWN: Support status not yet determined
 * - eNO: Feature definitely not supported
 * - eSOMETIMES: Partial or conditional support
 * - eYES: Full feature support confirmed
 * 
 * @example Used for color support detection:
 * @code
 *   if (descriptor->pProtocol->b256Support == eYES) {
 *       // Use full 256-color palette
 *   } else if (descriptor->pProtocol->b256Support == eSOMETIMES) {
 *       // Use limited color set
 *   } else {
 *       // Fall back to basic ANSI colors
 *   }
 * @endcode
 */
typedef enum
{
   eUNKNOWN,      /**< Support status unknown/undetermined */
   eNO,           /**< Feature not supported */
   eSOMETIMES,    /**< Partial or conditional support */
   eYES           /**< Full feature support */
} support_t;

/**
 * MSDP Variable Enumeration
 * 
 * Complete enumeration of all MSDP variables supported by LuminariMUD.
 * These variables provide real-time game state information to compatible
 * clients, enabling advanced GUI features and automation.
 * 
 * VARIABLE CATEGORIES:
 * - General: Server and character identification
 * - Character: Stats, attributes, and character sheet data
 * - Combat: Real-time combat information and opponent data
 * - World: Room, area, and environment information
 * - Configuration: Client capabilities and preferences
 * - GUI: Button and gauge definitions for client interfaces
 * 
 * USAGE PATTERN:
 * @code
 *   MSDPSetNumber(descriptor, eMSDP_HEALTH, GET_HIT(ch));
 *   MSDPSetString(descriptor, eMSDP_ROOM_NAME, world[IN_ROOM(ch)].name);
 *   MSDPUpdate(descriptor); // Send all dirty variables
 * @endcode
 * 
 * @note eMSDP_NONE must be -1 for proper array indexing
 * @note eMSDP_MAX must be the last entry for array sizing
 */
typedef enum
{
   eMSDP_NONE = -1,       /**< Invalid/uninitialized variable marker */

   /* General server and character information */
   eMSDP_CHARACTER_NAME,   /**< Player character name */
   eMSDP_SERVER_ID,        /**< Unique server identifier */
   eMSDP_SERVER_TIME,      /**< Current server timestamp */
   eMSDP_SNIPPET_VERSION,  /**< Protocol snippet version (8) */

   /* Character statistics and progression */
   eMSDP_AFFECTS,          /**< Active spell effects and conditions (array) */
   eMSDP_INVENTORY,        /**< Character inventory items (array) */
   eMSDP_ALIGNMENT,        /**< Character alignment (-1000 to 1000) */
   eMSDP_EXPERIENCE,       /**< Current experience points */
   eMSDP_EXPERIENCE_MAX,   /**< Experience points at current level */
   eMSDP_EXPERIENCE_TNL,   /**< Experience points to next level */
   eMSDP_HEALTH,           /**< Current hit points */
   eMSDP_HEALTH_MAX,       /**< Maximum hit points */
   eMSDP_LEVEL,            /**< Character level */
   eMSDP_RACE,             /**< Character race name */
   eMSDP_CLASS,            /**< Character class name */
   eMSDP_PSP,              /**< Current psionic spell points */
   eMSDP_PSP_MAX,          /**< Maximum psionic spell points */
   eMSDP_WIMPY,            /**< Wimpy flee threshold */
   eMSDP_PRACTICE,         /**< Practice sessions available */
   eMSDP_MONEY,            /**< Character wealth in gold */
   eMSDP_MOVEMENT,         /**< Current movement points */
   eMSDP_MOVEMENT_MAX,     /**< Maximum movement points */
   eMSDP_ATTACK_BONUS,     /**< Attack bonus modifier */
   eMSDP_DAMAGE_BONUS,     /**< Damage bonus modifier */
   eMSDP_AC,               /**< Armor class */
   eMSDP_STR,              /**< Current strength score */
   eMSDP_INT,              /**< Current intelligence score */
   eMSDP_WIS,              /**< Current wisdom score */
   eMSDP_DEX,              /**< Current dexterity score */
   eMSDP_CON,              /**< Current constitution score */
   eMSDP_CHA,              /**< Current charisma score */
   eMSDP_STR_PERM,         /**< Permanent strength score */
   eMSDP_INT_PERM,         /**< Permanent intelligence score */
   eMSDP_WIS_PERM,         /**< Permanent wisdom score */
   eMSDP_DEX_PERM,         /**< Permanent dexterity score */
   eMSDP_CON_PERM,         /**< Permanent constitution score */
   eMSDP_CHA_PERM,         /**< Permanent charisma score */
   eMSDP_ACTIONS,          /**< Available actions data (table) */
   eMSDP_STANDARD_ACTION,  /**< Standard action available (boolean) */
   eMSDP_MOVE_ACTION,      /**< Move action available (boolean) */
   eMSDP_SWIFT_ACTION,     /**< Swift action available (boolean) */
   eMSDP_GROUP,            /**< Group members data (array) */
   eMSDP_POSITION,         /**< Current position (standing/sitting/etc) */

   /* Real-time combat information */
   eMSDP_OPPONENT_HEALTH,      /**< Current opponent's hit points */
   eMSDP_OPPONENT_HEALTH_MAX,  /**< Current opponent's maximum hit points */
   eMSDP_OPPONENT_LEVEL,       /**< Current opponent's level */
   eMSDP_OPPONENT_NAME,        /**< Current opponent's name */
   eMSDP_TANK_NAME,            /**< Group tank's name */
   eMSDP_TANK_HEALTH,          /**< Group tank's current hit points */
   eMSDP_TANK_HEALTH_MAX,      /**< Group tank's maximum hit points */

   /* World and environment information */
   eMSDP_ROOM,             /**< Complete room information (table) */
   eMSDP_AREA_NAME,        /**< Current area/zone name */
   eMSDP_ROOM_EXITS,       /**< Available exits from current room (array) */
   eMSDP_ROOM_NAME,        /**< Current room name */
   eMSDP_ROOM_VNUM,        /**< Current room virtual number */
   eMSDP_WORLD_TIME,       /**< Game world time */
   eMSDP_SECTORS,          /**< Room sector/terrain information */
   eMSDP_MINIMAP,          /**< ASCII minimap representation */

   /* Client configuration and capabilities */
   eMSDP_CLIENT_ID,        /**< Client software name (configurable) */
   eMSDP_CLIENT_VERSION,   /**< Client version string (configurable) */
   eMSDP_PLUGIN_ID,        /**< Plugin/script identification (configurable) */
   eMSDP_ANSI_COLORS,      /**< ANSI color support (boolean) */
   eMSDP_256_COLORS,       /**< 256-color support (boolean) */
   eMSDP_UTF_8,            /**< UTF-8 encoding support (boolean) */
   eMSDP_SOUND,            /**< Sound/audio support (boolean) */
   eMSDP_MXP,              /**< MXP markup support (boolean) */

   /* GUI element definitions for compatible clients */
   eMSDP_BUTTON_1,         /**< GUI button 1 definition (Help) */
   eMSDP_BUTTON_2,         /**< GUI button 2 definition (Look) */
   eMSDP_BUTTON_3,         /**< GUI button 3 definition (Score) */
   eMSDP_BUTTON_4,         /**< GUI button 4 definition (Equipment) */
   eMSDP_BUTTON_5,         /**< GUI button 5 definition (Inventory) */
   eMSDP_GAUGE_1,          /**< GUI gauge 1 definition (Health - red) */
   eMSDP_GAUGE_2,          /**< GUI gauge 2 definition (PSP - blue) */
   eMSDP_GAUGE_3,          /**< GUI gauge 3 definition (Movement - green) */
   eMSDP_GAUGE_4,          /**< GUI gauge 4 definition (Experience - yellow) */
   eMSDP_GAUGE_5,          /**< GUI gauge 5 definition (Opponent Health - dark red) */

   eMSDP_MAX               /**< Array size marker - must always be last */
} variable_t;

/**
 * MSDP Variable Definition Structure
 * 
 * Defines the properties and constraints for each MSDP variable.
 * This structure is used in the VariableNameTable to specify
 * how each variable should be handled, validated, and transmitted.
 * 
 * VARIABLE TYPES:
 * - Read-only: Server-controlled values (health, level, etc.)
 * - Configurable: Client can set values (colors, sound preferences)
 * - Write-once: Set once by client, then read-only (client ID)
 * - GUI: Special interface configuration variables
 * 
 * @note This structure is used for both validation and initialization
 */
typedef struct
{
   variable_t Variable;    /**< Enum identifier for this variable */
   const char *pName;      /**< String name sent to client */
   bool_t bString;         /**< True if string variable, false if numeric */
   bool_t bConfigurable;   /**< True if client can modify this variable */
   bool_t bWriteOnce;      /**< True if client can only set once */
   bool_t bGUI;            /**< True if this is a GUI configuration variable */
   int Min;                /**< Minimum value/length (-1 if no limit) */
   int Max;                /**< Maximum value/length (-1 if no limit) */
   int Default;            /**< Default numeric value */
   const char *pDefault;   /**< Default string value */
} variable_name_t;

/**
 * MSDP Variable Storage Structure
 * 
 * Runtime storage for individual MSDP variables. Each client connection
 * maintains an array of these structures to track variable states and
 * manage efficient updates.
 * 
 * UPDATE MECHANISM:
 * 1. Client sends REPORT command for variables it wants
 * 2. bReport flag is set to true for requested variables
 * 3. Server calls MSDPSetNumber/String to update values
 * 4. bDirty flag is set when value changes
 * 5. MSDPUpdate() sends all dirty variables to client
 * 6. bDirty flag is cleared after successful transmission
 * 
 * MEMORY MANAGEMENT:
 * - String values are dynamically allocated and freed
 * - Numeric values are stored directly in ValueInt
 * - Arrays and tables are stored as formatted strings
 * 
 * @note Only one of ValueInt or pValueString should be used per variable
 */
typedef struct
{
   bool_t bReport;       /**< Client requested this variable via REPORT */
   bool_t bDirty;        /**< Variable has changed and needs transmission */
   int ValueInt;         /**< Numeric value (for non-string variables) */
   char *pValueString;   /**< String value (for string/array/table variables) */
} MSDP_t;

/**
 * MSSP Variable Definition Structure
 * 
 * Defines variables for the MUD Server Status Protocol, used by MUD listing
 * services and monitoring tools to gather server information.
 * 
 * MSSP VARIABLES:
 * - Static values: Set once at startup (codebase, contact info)
 * - Dynamic values: Updated regularly (player count, uptime)
 * - Computed values: Generated on-demand via function callbacks
 * 
 * VALUE SOURCES:
 * - pValue: Static string value
 * - pFunction: Callback function that returns current value
 * - Only one should be set per variable
 * 
 * @example
 * @code
 *   {"PLAYERS", NULL, GetPlayerCount},     // Dynamic via function
 *   {"CODEBASE", "LuminariMUD", NULL},     // Static string value
 * @endcode
 */
typedef struct
{
   const char *pName;          /**< MSSP variable name */
   const char *pValue;         /**< Static string value (if any) */
   const char *(*pFunction)(); /**< Function to get dynamic value (if any) */
} MSSP_t;

/**
 * Main Protocol Structure
 * 
 * Complete protocol state for a single client connection. This structure
 * tracks all negotiated protocols, client capabilities, MSDP variables,
 * and maintains per-connection buffers for safe multi-client operation.
 * 
 * LIFECYCLE:
 * 1. ProtocolCreate() allocates and initializes structure
 * 2. ProtocolNegotiate() starts telnet option negotiation
 * 3. Various protocols are negotiated and flags are set
 * 4. MSDP variables are allocated and managed
 * 5. ProtocolDestroy() cleans up when connection closes
 * 
 * NEGOTIATION FLAGS:
 * - Negotiated[]: Array indexed by negotiated_t enum
 * - b* flags: Convenience booleans for quick protocol checks
 * 
 * MEMORY SAFETY:
 * - Per-descriptor buffers prevent race conditions
 * - All strings are properly allocated/deallocated
 * - Null pointer checks throughout the system
 * 
 * CLIENT DETECTION:
 * - TTYPE cycling to identify client software
 * - Version detection for feature compatibility
 * - Capability detection for optimal feature use
 */
typedef struct
{
   /* Internal state management */
   int WriteOOB;                            /**< Out-of-band data write flag */
   bool_t Negotiated[eNEGOTIATED_MAX];     /**< Protocol negotiation status array */
   bool_t bIACMode;                        /**< IAC parsing mode for broken packets */
   bool_t bNegotiated;                     /**< Overall negotiation completion flag */
   bool_t bRenegotiate;                    /**< Force renegotiation flag */
   bool_t bNeedMXPVersion;                 /**< MXP version detection needed */
   bool_t bBlockMXP;                       /**< Block MXP based on version issues */
   
   /* Protocol support flags */
   bool_t bTTYPE;                          /**< Terminal type protocol support */
   bool_t bECHO;                           /**< Echo control protocol support */
   bool_t bNAWS;                           /**< Window size protocol support */
   bool_t bCHARSET;                        /**< Character set protocol support */
   bool_t bMSDP;                           /**< MSDP protocol support */
   bool_t bMSSP;                           /**< MSSP protocol support */
   bool_t bGMCP;                           /**< GMCP protocol support */
   bool_t bMSP;                            /**< Sound protocol support */
   bool_t bMXP;                            /**< Markup protocol support */
   bool_t bMCCP;                           /**< Compression protocol support */
   
   /* Client capabilities */
   support_t b256Support;                  /**< 256-color support level */
   int ScreenWidth;                        /**< Client screen width (from NAWS) */
   int ScreenHeight;                       /**< Client screen height (from NAWS) */
   char *pMXPVersion;                      /**< MXP version string */
   char *pLastTTYPE;                       /**< Last TTYPE for cycling detection */
   
   /* MSDP variable storage */
   MSDP_t **pVariables;                    /**< Array of MSDP variable pointers */
   
   /* Per-descriptor buffers for thread safety */
   char CmdBuf[MAX_PROTOCOL_BUFFER + 1];   /**< Command parsing buffer */
   char IacBuf[MAX_PROTOCOL_BUFFER + 1];   /**< Telnet IAC command buffer */
} protocol_t;

/******************************************************************************
 *                           PROTOCOL FUNCTIONS
 * 
 * Core protocol management functions for connection lifecycle, negotiation,
 * and data exchange. These functions provide a complete API for integrating
 * advanced MUD protocols into any MUD codebase.
 * 
 * FUNCTION CATEGORIES:
 * - Lifecycle: Create, destroy, and manage protocol structures
 * - Negotiation: Handle telnet option negotiation with clients  
 * - I/O Processing: Parse input and format output with protocol data
 * - MSDP: Manage real-time variable updates and transmission
 * - MSSP: Handle server status reporting
 * - MXP: Create and send markup tags
 * - Color: Process color codes and RGB values
 * - Unicode: Handle UTF-8 character encoding
 * - Sound: Send audio triggers to clients
 * - Copyover: Save/restore protocol state across reboots
 *****************************************************************************/

/**
 * Create and initialize a protocol structure for a new connection
 * 
 * Allocates and initializes a complete protocol_t structure for a single
 * client connection. This function sets up all necessary data structures,
 * initializes MSDP variables, and prepares the connection for protocol
 * negotiation.
 * 
 * INITIALIZATION PROCESS:
 * 1. Allocates main protocol_t structure
 * 2. Initializes all boolean flags to false
 * 3. Allocates MSDP variable array (eMSDP_MAX entries)
 * 4. Sets up default values for configurable variables
 * 5. Initializes per-descriptor buffers
 * 6. Validates variable table integrity
 * 
 * MEMORY ALLOCATION:
 * - Main structure: calloc(1, sizeof(protocol_t))
 * - MSDP variables: calloc(eMSDP_MAX, sizeof(MSDP_t*))
 * - Individual variables: calloc(1, sizeof(MSDP_t)) as needed
 * 
 * ERROR HANDLING:
 * - Returns NULL if memory allocation fails
 * - Logs critical errors if variable table is corrupted
 * - Safe to call multiple times (no global state modified)
 * 
 * @return Pointer to initialized protocol_t structure, or NULL on failure
 * 
 * @usage Called when initializing a new client connection:
 * @code
 *   descriptor->pProtocol = ProtocolCreate();
 *   if (!descriptor->pProtocol) {
 *       log("SYSERR: Failed to create protocol structure");
 *       close_socket(descriptor);
 *       return;
 *   }
 * @endcode
 * 
 * @note Must be paired with ProtocolDestroy() when connection closes
 * @note Thread-safe for single-threaded MUD architecture
 * @see ProtocolDestroy(), ProtocolNegotiate()
 */
protocol_t *ProtocolCreate(void);

/**
 * Clean up and free a protocol structure
 * 
 * Properly deallocates all memory associated with a protocol_t structure,
 * including MSDP variables, string values, and dynamically allocated buffers.
 * This function ensures no memory leaks occur when a connection closes.
 * 
 * CLEANUP PROCESS:
 * 1. Validates input pointer (safe to call with NULL)
 * 2. Frees all MSDP variable string values
 * 3. Frees individual MSDP_t structures
 * 4. Frees MSDP variable array
 * 5. Frees MXP version string
 * 6. Frees last TTYPE string
 * 7. Frees main protocol structure
 * 
 * MEMORY SAFETY:
 * - NULL pointer safe (returns immediately if apProtocol is NULL)
 * - Checks all pointers before freeing
 * - Sets freed pointers to NULL where possible
 * - No double-free issues
 * 
 * @param apProtocol Pointer to protocol structure to destroy (may be NULL)
 * 
 * @usage Called when cleaning up a client connection:
 * @code
 *   if (descriptor->pProtocol) {
 *       ProtocolDestroy(descriptor->pProtocol);
 *       descriptor->pProtocol = NULL;
 *   }
 * @endcode
 * 
 * @note Safe to call multiple times or with NULL pointer
 * @note Should be called before freeing the descriptor structure
 * @see ProtocolCreate()
 */
void ProtocolDestroy(protocol_t *apProtocol);

/**
 * Start protocol negotiation with the client
 * 
 * Initiates telnet option negotiation to determine which advanced protocols
 * the client supports. This function sends the initial negotiation sequences
 * and sets up the protocol state machine to handle responses.
 * 
 * NEGOTIATION SEQUENCE:
 * 1. Sends WILL/DO commands for each supported protocol
 * 2. Client responds with WILL/DO (accept) or WONT/DONT (reject)
 * 3. Negotiation handlers update protocol flags based on responses
 * 4. Protocol-specific initialization occurs after successful negotiation
 * 5. MSDP/GMCP variables are set up for reporting
 * 
 * PROTOCOLS NEGOTIATED:
 * - TTYPE: Terminal type identification
 * - NAWS: Window size detection
 * - CHARSET: Character encoding (UTF-8 support)
 * - MSDP: Real-time data protocol
 * - GMCP: JSON-based data protocol (fallback for MSDP)
 * - MSSP: Server status protocol
 * - MXP: Markup and hyperlink protocol
 * - MSP: Sound protocol
 * - MCCP: Compression protocol (if enabled)
 * 
 * TIMING CONSIDERATIONS:
 * - Should be called once per connection
 * - Can be called immediately after connection or after login
 * - Earlier negotiation allows better login experience
 * - Later negotiation avoids interfering with login process
 * 
 * @param apDescriptor Client connection descriptor with initialized protocol structure
 * 
 * @usage Typical negotiation timing:
 * @code
 *   // Option 1: Negotiate immediately after connection
 *   if (descriptor->pProtocol) {
 *       ProtocolNegotiate(descriptor);
 *   }
 *   
 *   // Option 2: Negotiate after character enters game
 *   if (STATE(descriptor) == CON_PLAYING && descriptor->pProtocol) {
 *       ProtocolNegotiate(descriptor);
 *   }
 * @endcode
 * 
 * @note Only call once per connection to avoid negotiation loops
 * @note Requires valid protocol structure (call ProtocolCreate() first)
 * @see ProtocolCreate(), ProtocolInput(), ProtocolOutput()
 */
void ProtocolNegotiate(descriptor_t *apDescriptor);

/* MUD Primary Colours */
extern const char *RGBone;
extern const char *RGBtwo;
extern const char *RGBthree;

/**
 * Control client echo for password input
 * 
 * Sends telnet ECHO option commands to enable or disable local echo
 * on the client. This is essential for secure password input where
 * characters should not be displayed as the user types.
 * 
 * ECHO CONTROL:
 * - abOn = true: Enable echo (normal input display)
 * - abOn = false: Disable echo (hidden password input)
 * 
 * TELNET PROTOCOL:
 * - Sends IAC WILL ECHO to enable server-side echo (hide client echo)
 * - Sends IAC WONT ECHO to disable server-side echo (show client echo)
 * 
 * @param apDescriptor Client connection descriptor
 * @param abOn True to enable echo, false to disable for password input
 * 
 * @usage Password input sequence:
 * @code
 *   write_to_output(d, "Password: ");
 *   ProtocolNoEcho(d, false);  // Hide password input
 *   STATE(d) = CON_PASSWORD;
 *   
 *   // Later, after password is entered:
 *   ProtocolNoEcho(d, true);   // Restore normal echo
 * @endcode
 * 
 * @note Gracefully handles clients that don't support ECHO option
 * @see ProtocolNegotiate()
 */
void ProtocolNoEcho(descriptor_t *apDescriptor, bool_t abOn);

/**
 * Process input data and extract protocol sequences
 * 
 * Parses raw input from the client, extracts telnet negotiation sequences
 * and protocol data (MSDP, GMCP, etc.), then returns the remaining text
 * for normal MUD command processing.
 * 
 * INPUT PROCESSING:
 * 1. Scans input for IAC (telnet command) sequences
 * 2. Handles protocol negotiation responses (WILL/WONT/DO/DONT)
 * 3. Processes sub-negotiation data (MSDP variables, window size, etc.)
 * 4. Removes protocol data from input stream
 * 5. Returns clean text for command interpretation
 * 
 * SUPPORTED PROTOCOLS:
 * - Telnet option negotiation (IAC sequences)
 * - MSDP variable reports and updates
 * - GMCP JSON message processing
 * - NAWS window size updates
 * - TTYPE terminal identification
 * - CHARSET encoding negotiation
 * 
 * BUFFER MANAGEMENT:
 * - Uses per-descriptor buffers for thread safety
 * - Handles fragmented protocol sequences
 * - Prevents buffer overflows with size checking
 * - Maintains input stream integrity
 * 
 * @param apDescriptor Client connection descriptor
 * @param apData Raw input data from client
 * @param aSize Size of input data in bytes
 * @param apOut Buffer to receive processed text output
 * @return Number of bytes written to output buffer, or -1 on error
 * 
 * @usage Called in main input processing loop:
 * @code
 *   ssize_t processed = ProtocolInput(d, raw_input, input_size, clean_input);
 *   if (processed < 0) {
 *       log("Protocol input processing error");
 *       return;
 *   }
 *   
 *   // Process clean_input as normal MUD commands
 *   if (processed > 0) {
 *       interpret_command(d->character, clean_input);
 *   }
 * @endcode
 * 
 * @note Must be called before normal command interpretation
 * @note Output buffer should be at least as large as input buffer
 * @see ProtocolOutput(), ProtocolNegotiate()
 */
ssize_t ProtocolInput(descriptor_t *apDescriptor, char *apData, int aSize, char *apOut);

/* Function: ProtocolOutput
 *
 * This function takes a string, applies colour codes to it, and returns the
 * result.  It should be called just before writing to the output buffer.
 *
 * The special character used to indicate the start of a colour sequence is
 * '\t' (i.e., a tab, or ASCII character 9).  This makes it easy to include
 * in help files (as you can literally press the tab key) as well as strings
 * (where you can use \t instead).  However players can't send tabs (on most
 * muds at least), so this stops them from sending colour codes to each other.
 *
 * The predefined colours are:
 *
 *   n: no colour (switches colour off)
 *
 *   r: dark red                        R: light red
 *   g: dark green                      G: light green
 *   b: dark blue                       B: light blue
 *   y: dark yellow                     Y: light yellow
 *   m: dark magenta                    M: light magenta
 *   c: dark cyan                       C: light cyan
 *   w: dark white                      W: light white
 *
 *   a: dark azure                      A: light azure
 *   j: dark jade                       J: light jade
 *   l: dark lime                       L: light lime
 *   o: dark orange                     O: bright orange
 *   p: dark pink                       P: light pink
 *   t: dark tan                        T: light tan
 *   v: dark violet                     V: light violet
 *   d: dark grey/black                 D: light grey
 *   _: underlined (if supported)       +: bold (if supported)
 *   -: blinking (if supported)         =: reverse (if supported)
 *   *: at-sign
 *
 *   1: base palette 1                  2: base palette 2
 *   3: base palette 3
 *
 * So for example "This is \tOorange\tn." will colour the word "orange".  You
 * can add more colours yourself just by updating the switch statement.
 *
 * It's also possible to explicitly specify an RGB value, by including the four
 * character colour sequence (as used by ColourRGB) within square brackets, eg:
 *
 *    This is a \t[F010]very dark green foreground\tn.
 *
 * The square brackets can also be used to send unicode characters, like this:
 *
 *    Boat: \t[U9973/B]
 *    Rook: \t[U9814/C]
 *
 * For example you might use 'B' to represent a boat on your ASCII map, or a 'C'
 * to represent a castle - but players with UTF-8 support would actually see the
 * appropriate unicode characters for a boat or a rook (the chess playing piece).
 *
 * The exact syntax is '\t' (tab), '[', 'U' (indicating unicode), then the decimal
 * number of the unicode character (see http://www.unicode.org/charts), then '/'
 * followed by the ASCII character/s that should be used if the client doesn't
 * support UTF-8.  The ASCII sequence can be up to 7 characters in length, but in
 * most cases you'll only want it to be one or two characters (so that it has the
 * same alignment as the unicode character).
 *
 * Finally, this function also allows you to embed MXP tags.  The easiest and
 * safest way to do this is via the ( and ) bracket options:
 *
 *    From here, you can walk \t(north\t).
 *
 * However it's also possible to include more explicit MSP tags, like this:
 *
 *    The baker offers to sell you a \t<send href="buy pie">pie\t</send>.
 *
 * Note that the MXP tags will automatically be removed if the user doesn't
 * support MXP, but it's very important you remember to close the tags.
 */
/**
 * Process output data and apply color/protocol formatting
 * 
 * Transforms output text by processing color codes, Unicode characters,
 * MXP markup, and other formatting elements based on client capabilities.
 * This is the final stage before sending data to the client.
 * 
 * COLOR PROCESSING:
 * - Converts \t escape sequences to ANSI color codes
 * - Handles RGB colors (\t[F500]) based on client support
 * - Processes Unicode characters (\t[U9973/B]) with ASCII fallbacks
 * - Supports extended color palette and formatting codes
 * 
 * PROTOCOL FEATURES:
 * - MXP tag processing for markup and hyperlinks
 * - Client capability detection for optimal output
 * - Graceful fallback for unsupported features
 * - Per-client customization based on negotiated protocols
 * 
 * COLOR CODES SUPPORTED:
 * - Basic: \tr (red), \tg (green), \tb (blue), etc.
 * - Extended: \ta (azure), \tj (jade), \tl (lime), etc.
 * - Formatting: \t_ (underline), \t+ (bold), \t- (blink)
 * - RGB: \t[F500] (red fg), \t[B023] (dark cyan bg)
 * - Unicode: \t[U9973/B] (boat symbol, 'B' fallback)
 * 
 * PERFORMANCE:
 * - Caches color codes for efficiency
 * - Minimal processing for clients without color support
 * - Per-descriptor buffers prevent memory allocation overhead
 * 
 * @param apDescriptor Client connection descriptor
 * @param apData Raw output text with color codes and formatting
 * @param apLength Pointer to length variable (updated with final length)
 * @return Formatted output string ready for transmission
 * 
 * @usage Called before sending any text to client:
 * @code
 *   int length = strlen(raw_output);
 *   const char *formatted = ProtocolOutput(d, raw_output, &length);
 *   write_to_descriptor(d->descriptor, formatted, length);
 * @endcode
 * 
 * @note Returned string may be longer than input due to ANSI codes
 * @note Length parameter is updated with final output length
 * @see ProtocolInput(), ColourRGB(), UnicodeGet()
 */
const char *ProtocolOutput(descriptor_t *apDescriptor, const char *apData, int *apLength);

/******************************************************************************
 *                           COPYOVER FUNCTIONS
 * 
 * Hot-boot (copyover) support functions for maintaining protocol state
 * across server restarts. These functions allow the server to restart
 * without disconnecting players by preserving protocol negotiation state.
 * 
 * COPYOVER PROCESS:
 * 1. Server saves all connection states to temporary file
 * 2. Server executes new binary image
 * 3. New server reads connection states from temporary file
 * 4. Protocol states are restored for each connection
 * 5. MSDP/GMCP renegotiation occurs automatically
 * 
 * WHY SAVE PROTOCOL STATE:
 * - Clients refuse to renegotiate telnet options after connection
 * - Prevents negotiation loops that could disconnect clients
 * - Maintains optimal client experience across restarts
 * - Preserves color support, window size, and other capabilities
 *****************************************************************************/

/**
 * Get protocol state as string for copyover save
 * 
 * Serializes the current protocol state into a compact string format
 * suitable for storage in a copyover file. This string contains all
 * negotiated protocol flags and client capabilities.
 * 
 * SAVED INFORMATION:
 * - Protocol negotiation flags (MSDP, GMCP, MXP, etc.)
 * - Client capabilities (256-color support, UTF-8, etc.)
 * - Window dimensions (NAWS data)
 * - MXP version information
 * - Terminal type information
 * 
 * NOT SAVED:
 * - MSDP variable values (renegotiated after copyover)
 * - MSDP REPORT settings (renegotiated after copyover)
 * - Client name/version (recommend saving in player file)
 * - Dynamic buffers and temporary state
 * 
 * STRING FORMAT:
 * Compact encoded format designed for minimal storage space
 * and easy parsing by CopyoverSet().
 * 
 * @param apDescriptor Client connection descriptor
 * @return String containing serialized protocol state (static buffer)
 * 
 * @usage Save protocol state during copyover:
 * @code
 *   fprintf(copyover_file, "%s %s %s\n", 
 *           GET_NAME(ch), 
 *           CopyoverGet(ch->desc),
 *           host_info);
 * @endcode
 * 
 * @note Returns static buffer - copy if needed for later use
 * @note Safe to call with NULL descriptor (returns empty string)
 * @see CopyoverSet()
 */
const char *CopyoverGet(descriptor_t *apDescriptor);

/**
 * Restore protocol state from copyover string
 * 
 * Deserializes protocol state from a string created by CopyoverGet()
 * and restores the client's protocol capabilities. Also initiates
 * MSDP/GMCP renegotiation to rebuild variable reporting.
 * 
 * RESTORATION PROCESS:
 * 1. Parses protocol state string
 * 2. Sets negotiation flags based on saved state
 * 3. Restores client capabilities (colors, window size, etc.)
 * 4. Initiates MSDP/GMCP renegotiation for variable reporting
 * 5. Sends initial MSDP/GMCP setup commands
 * 
 * AUTOMATIC RENEGOTIATION:
 * - MSDP/GMCP variables are renegotiated automatically
 * - Client is asked to report desired variables again
 * - Initial variable values are sent to client
 * - Normal update cycle resumes
 * 
 * ERROR HANDLING:
 * - Invalid strings are handled gracefully
 * - Corrupted data results in default negotiation
 * - Missing data fields use safe defaults
 * 
 * @param apDescriptor Client connection descriptor with valid protocol structure
 * @param apData Protocol state string from CopyoverGet()
 * 
 * @usage Restore protocol state after copyover:
 * @code
 *   char name[MAX_NAME_LENGTH], protocol_data[256], host[256];
 *   sscanf(line, "%s %s %s", name, protocol_data, host);
 *   
 *   // Find character and descriptor...
 *   CopyoverSet(ch->desc, protocol_data);
 * @endcode
 * 
 * @note Requires valid protocol structure (call ProtocolCreate() first)
 * @note Safe to call with NULL descriptor or empty string
 * @see CopyoverGet(), ProtocolNegotiate()
 */
void CopyoverSet(descriptor_t *apDescriptor, const char *apData);

/******************************************************************************
 *                           MSDP FUNCTIONS
 * 
 * Mud Server Data Protocol (MSDP) functions for real-time game data exchange.
 * MSDP provides structured, efficient transmission of game state information
 * to compatible clients, enabling advanced GUI features and automation.
 * 
 * MSDP WORKFLOW:
 * 1. Client sends REPORT commands for desired variables
 * 2. Server calls MSDPSetNumber/String/Array/Table to update values
 * 3. Variables are marked as "dirty" when values change
 * 4. MSDPUpdate() sends all dirty variables to client
 * 5. Variables are marked clean after successful transmission
 * 
 * PROTOCOL FALLBACK:
 * All MSDP functions automatically use GMCP (Generic MUD Communication
 * Protocol) if the client doesn't support MSDP. This provides seamless
 * compatibility with different client types.
 *****************************************************************************/

/**
 * Send all dirty MSDP variables to the client
 * 
 * Transmits all MSDP variables that have been marked as dirty (changed)
 * and are being reported by the client. This is the main function for
 * regular MSDP updates and should be called frequently (every game pulse).
 * 
 * UPDATE PROCESS:
 * 1. Iterates through all MSDP variables
 * 2. Checks if variable is both reported (client wants it) and dirty (changed)
 * 3. Formats and sends variable data to client
 * 4. Marks sent variables as clean (not dirty)
 * 5. Uses GMCP if client doesn't support MSDP
 * 
 * TRANSMISSION EFFICIENCY:
 * - Only sends variables the client has requested
 * - Only sends variables that have actually changed
 * - Batches all updates into single protocol message
 * - Minimizes network traffic and client processing
 * 
 * @param apDescriptor Client connection descriptor
 * 
 * @usage Called every game pulse for real-time updates:
 * @code
 *   // In main game loop (every 0.1 seconds)
 *   for (d = descriptor_list; d; d = d->next) {
 *       if (STATE(d) == CON_PLAYING && d->pProtocol) {
 *           // Update all character data
 *           MSDPSetNumber(d, eMSDP_HEALTH, GET_HIT(d->character));
 *           MSDPSetNumber(d, eMSDP_HEALTH_MAX, GET_MAX_HIT(d->character));
 *           // ... update other variables ...
 *           
 *           // Send all dirty variables
 *           MSDPUpdate(d);
 *       }
 *   }
 * @endcode
 * 
 * @note Safe to call frequently - only sends changed data
 * @note Automatically handles MSDP/GMCP fallback
 * @see MSDPSetNumber(), MSDPSetString(), MSDPFlush()
 */
void MSDPUpdate(descriptor_t *apDescriptor);

/**
 * Send a specific MSDP variable immediately
 * 
 * Forces immediate transmission of a single MSDP variable, bypassing
 * the normal update cycle. The variable is only sent if the client
 * has requested it via REPORT and it is marked as dirty.
 * 
 * USE CASES:
 * - Critical updates that can't wait for next pulse
 * - Event-driven updates (combat damage, spell effects)
 * - Debugging and testing specific variables
 * - Time-sensitive information (opponent health in combat)
 * 
 * @param apDescriptor Client connection descriptor
 * @param aMSDP MSDP variable to flush immediately
 * 
 * @usage Send critical updates immediately:
 * @code
 *   // Immediate health update after taking damage
 *   MSDPSetNumber(ch->desc, eMSDP_HEALTH, GET_HIT(ch));
 *   MSDPFlush(ch->desc, eMSDP_HEALTH);
 *   
 *   // Immediate opponent update when combat starts
 *   MSDPSetString(ch->desc, eMSDP_OPPONENT_NAME, GET_NAME(opponent));
 *   MSDPFlush(ch->desc, eMSDP_OPPONENT_NAME);
 * @endcode
 * 
 * @note Only sends if variable is both reported and dirty
 * @note More expensive than batched updates via MSDPUpdate()
 * @see MSDPUpdate(), MSDPSetNumber(), MSDPSetString()
 */
void MSDPFlush(descriptor_t *apDescriptor, variable_t aMSDP);

/**
 * Send an MSDP variable regardless of report/dirty status
 * 
 * Forces transmission of a specific MSDP variable without checking
 * whether the client requested it or whether it has changed. This
 * function is primarily for debugging and testing.
 * 
 * NORMAL vs DEBUG SENDING:
 * - Normal: Only sends reported, dirty variables
 * - Debug: Sends any variable regardless of status
 * - Normal: Part of efficient update cycle
 * - Debug: Immediate, potentially wasteful transmission
 * 
 * @param apDescriptor Client connection descriptor
 * @param aMSDP MSDP variable to send
 * 
 * @usage Debugging and testing:
 * @code
 *   // Force send for debugging (not recommended for normal use)
 *   MSDPSend(ch->desc, eMSDP_HEALTH);
 *   
 *   // Better approach for normal updates:
 *   MSDPSetNumber(ch->desc, eMSDP_HEALTH, GET_HIT(ch));
 *   MSDPUpdate(ch->desc); // or MSDPFlush(ch->desc, eMSDP_HEALTH);
 * @endcode
 * 
 * @note Should not be used in normal operation - use MSDPUpdate() instead
 * @note Bypasses efficiency mechanisms (report checking, dirty flags)
 * @see MSDPUpdate(), MSDPFlush()
 */
void MSDPSend(descriptor_t *apDescriptor, variable_t aMSDP);

/**
 * Send a custom MSDP variable/value pair
 * 
 * Sends an arbitrary variable name and value to the client, bypassing
 * the predefined MSDP variable system. Useful for custom extensions
 * or experimental features.
 * 
 * CUSTOM VARIABLES:
 * - Not limited to predefined eMSDP_* variables
 * - Can send any variable name/value combination
 * - Client must understand custom variable meaning
 * - Useful for MUD-specific extensions
 * 
 * @param apDescriptor Client connection descriptor
 * @param apVariable Variable name to send
 * @param apValue Variable value to send
 * 
 * @usage Send custom data to client:
 * @code
 *   // Send custom variable not in standard MSDP set
 *   MSDPSendPair(ch->desc, "CUSTOM_STAT", "42");
 *   MSDPSendPair(ch->desc, "GUILD_RANK", "Initiate");
 *   
 *   // Send dynamic data
 *   sprintf(buffer, "%d", special_calculation());
 *   MSDPSendPair(ch->desc, "CALCULATED_VALUE", buffer);
 * @endcode
 * 
 * @note Automatically uses GMCP if MSDP not supported
 * @note Client must understand custom variable meanings
 * @see MSDPSendList(), MSDPSetString()
 */
void MSDPSendPair(descriptor_t *apDescriptor, const char *apVariable, const char *apValue);

/**
 * Send a custom MSDP array variable
 * 
 * Sends a custom variable as an MSDP array, where the value contains
 * multiple space-separated items. The client receives this as a
 * structured array rather than a simple string.
 * 
 * ARRAY FORMAT:
 * - Input: Space-separated values in single string
 * - Output: Properly formatted MSDP array structure
 * - Client receives structured array data
 * 
 * @param apDescriptor Client connection descriptor
 * @param apVariable Variable name to send
 * @param apValue Space-separated list of array values
 * 
 * @usage Send array data to client:
 * @code
 *   // Send list of available commands
 *   MSDPSendList(ch->desc, "COMMANDS", "north south east west look examine");
 *   
 *   // Send list of group members
 *   MSDPSendList(ch->desc, "GROUP_MEMBERS", "Alice Bob Charlie");
 *   
 *   // Send dynamic list
 *   sprintf(buffer, "%s %s %s", item1, item2, item3);
 *   MSDPSendList(ch->desc, "INVENTORY_TYPES", buffer);
 * @endcode
 * 
 * @note Values should be separated by single spaces
 * @note Automatically uses GMCP if MSDP not supported
 * @see MSDPSendPair(), MSDPSetArray()
 */
void MSDPSendList(descriptor_t *apDescriptor, const char *apVariable, const char *apValue);

/**
 * Set an MSDP numeric variable
 * 
 * Updates the value of an MSDP numeric variable and marks it as dirty
 * for transmission on the next update cycle. This is the primary function
 * for updating numeric game data like health, level, stats, etc.
 * 
 * VARIABLE HANDLING:
 * - Compares new value with current value
 * - Only marks as dirty if value actually changed
 * - Supports all integer types (int, bool, char, enum, short, etc.)
 * - Automatically handles type conversion
 * 
 * EFFICIENCY:
 * - No network traffic until MSDPUpdate() is called
 * - Duplicate values don't trigger unnecessary updates
 * - Batched with other variables for efficient transmission
 * 
 * @param apDescriptor Client connection descriptor
 * @param aMSDP MSDP variable identifier (eMSDP_* enum)
 * @param aValue New numeric value for the variable
 * 
 * @usage Update character statistics:
 * @code
 *   // Basic character stats
 *   MSDPSetNumber(ch->desc, eMSDP_HEALTH, GET_HIT(ch));
 *   MSDPSetNumber(ch->desc, eMSDP_HEALTH_MAX, GET_MAX_HIT(ch));
 *   MSDPSetNumber(ch->desc, eMSDP_LEVEL, GET_LEVEL(ch));
 *   MSDPSetNumber(ch->desc, eMSDP_EXPERIENCE, GET_EXP(ch));
 *   
 *   // Ability scores
 *   MSDPSetNumber(ch->desc, eMSDP_STR, GET_STR(ch));
 *   MSDPSetNumber(ch->desc, eMSDP_INT, GET_INT(ch));
 *   
 *   // Boolean values (0 or 1)
 *   MSDPSetNumber(ch->desc, eMSDP_STANDARD_ACTION, has_standard_action ? 1 : 0);
 *   
 *   // Enum values
 *   MSDPSetNumber(ch->desc, eMSDP_ALIGNMENT, GET_ALIGNMENT(ch));
 * @endcode
 * 
 * @note Works with any integer-compatible type (bool, char, enum, short)
 * @note Only triggers update if value actually changes
 * @see MSDPUpdate(), MSDPSetString(), MSDPFlush()
 */
void MSDPSetNumber(descriptor_t *apDescriptor, variable_t aMSDP, int aValue);

/**
 * Set an MSDP string variable
 * 
 * Updates the value of an MSDP string variable and marks it as dirty
 * for transmission on the next update cycle. This is the primary function
 * for updating text-based game data like names, descriptions, etc.
 * 
 * STRING HANDLING:
 * - Safely handles NULL pointers (converts to empty string)
 * - Performs string comparison to avoid unnecessary updates
 * - Dynamically allocates/deallocates string memory
 * - Properly manages memory to prevent leaks
 * 
 * MEMORY MANAGEMENT:
 * - Frees previous string value if present
 * - Allocates new memory for changed strings
 * - Safe to call with same string multiple times
 * - Handles empty strings and NULL pointers gracefully
 * 
 * @param apDescriptor Client connection descriptor
 * @param aMSDP MSDP variable identifier (eMSDP_* enum)
 * @param apValue New string value for the variable (may be NULL)
 * 
 * @usage Update character information:
 * @code
 *   // Basic character info
 *   MSDPSetString(ch->desc, eMSDP_CHARACTER_NAME, GET_NAME(ch));
 *   MSDPSetString(ch->desc, eMSDP_RACE, race_list[GET_RACE(ch)].name);
 *   MSDPSetString(ch->desc, eMSDP_CLASS, class_list[GET_CLASS(ch)].name);
 *   
 *   // World information
 *   MSDPSetString(ch->desc, eMSDP_ROOM_NAME, world[IN_ROOM(ch)].name);
 *   MSDPSetString(ch->desc, eMSDP_AREA_NAME, zone_table[world[IN_ROOM(ch)].zone].name);
 *   
 *   // Combat information
 *   if (FIGHTING(ch)) {
 *       MSDPSetString(ch->desc, eMSDP_OPPONENT_NAME, GET_NAME(FIGHTING(ch)));
 *   } else {
 *       MSDPSetString(ch->desc, eMSDP_OPPONENT_NAME, "");
 *   }
 *   
 *   // Handle NULL safely
 *   MSDPSetString(ch->desc, eMSDP_POSITION, position_types[GET_POS(ch)]); // Even if NULL
 * @endcode
 * 
 * @note NULL pointers are converted to empty strings
 * @note Only triggers update if string content actually changes
 * @note Memory is managed automatically
 * @see MSDPUpdate(), MSDPSetNumber(), MSDPSetArray()
 */
void MSDPSetString(descriptor_t *apDescriptor, variable_t aMSDP, const char *apValue);

/**
 * Set an MSDP table variable
 * 
 * Updates an MSDP variable with structured table data containing
 * multiple name/value pairs. Tables are used for complex data
 * structures like room information with multiple attributes.
 * 
 * TABLE FORMAT:
 * Tables use MSDP_VAR and MSDP_VAL markers to structure data:
 * - MSDP_VAR: Indicates a variable name
 * - MSDP_VAL: Indicates a variable value
 * - Pattern: VAR name1 VAL value1 VAR name2 VAL value2...
 * 
 * BINARY FORMAT:
 * The value string must contain binary MSDP markers (bytes 1 and 2)
 * mixed with text data for proper table structure.
 * 
 * @param apDescriptor Client connection descriptor
 * @param aMSDP MSDP variable identifier (eMSDP_* enum)
 * @param apValue Pre-formatted table data with MSDP markers
 * 
 * @usage Create structured room data:
 * @code
 *   char room_buffer[MAX_STRING_LENGTH];
 *   
 *   // Build room table: VNUM, NAME, EXITS
 *   sprintf(room_buffer, "%c%s%c%d%c%s%c%s%c%s%c%s",
 *           MSDP_VAR, "VNUM", MSDP_VAL, world[room].number,
 *           MSDP_VAR, "NAME", MSDP_VAL, world[room].name,
 *           MSDP_VAR, "EXITS", MSDP_VAL, get_exits_string(room));
 *   
 *   MSDPSetTable(ch->desc, eMSDP_ROOM, room_buffer);
 *   
 *   // Character equipment table
 *   sprintf(equip_buffer, "%c%s%c%s%c%s%c%s%c%s%c%s",
 *           MSDP_VAR, "WEAPON", MSDP_VAL, weapon_name,
 *           MSDP_VAR, "ARMOR", MSDP_VAL, armor_name,
 *           MSDP_VAR, "SHIELD", MSDP_VAL, shield_name);
 *   
 *   MSDPSetTable(ch->desc, eMSDP_ACTIONS, equip_buffer);
 * @endcode
 * 
 * @note Value must be pre-formatted with MSDP_VAR/MSDP_VAL markers
 * @note Each variable name and value should be null-terminated strings
 * @see MSDPSetString(), MSDPSetArray(), MSDP_VAR, MSDP_VAL
 */
void MSDPSetTable(descriptor_t *apDescriptor, variable_t aMSDP, const char *apValue);

/**
 * Set an MSDP array variable
 * 
 * Updates an MSDP variable with structured array data containing
 * multiple values. Arrays are used for lists like inventory items,
 * group members, available exits, spell effects, etc.
 * 
 * ARRAY FORMAT:
 * Arrays use MSDP_VAL markers to separate array elements:
 * - MSDP_VAL: Indicates an array element value
 * - Pattern: VAL element1 VAL element2 VAL element3...
 * 
 * BINARY FORMAT:
 * The value string must contain binary MSDP_VAL markers (byte 2)
 * before each array element for proper array structure.
 * 
 * @param apDescriptor Client connection descriptor
 * @param aMSDP MSDP variable identifier (eMSDP_* enum)
 * @param apValue Pre-formatted array data with MSDP_VAL markers
 * 
 * @usage Create structured array data:
 * @code
 *   char affects_buffer[MAX_STRING_LENGTH];
 *   char inventory_buffer[MAX_STRING_LENGTH];
 *   char exits_buffer[MAX_STRING_LENGTH];
 *   
 *   // Build affects array
 *   sprintf(affects_buffer, "%c%s%c%s%c%s",
 *           MSDP_VAL, "blind",
 *           MSDP_VAL, "haste", 
 *           MSDP_VAL, "fly");
 *   
 *   MSDPSetArray(ch->desc, eMSDP_AFFECTS, affects_buffer);
 *   
 *   // Build inventory array
 *   sprintf(inventory_buffer, "%c%s%c%s%c%s%c%s",
 *           MSDP_VAL, "a sword",
 *           MSDP_VAL, "a shield",
 *           MSDP_VAL, "a potion",
 *           MSDP_VAL, "some gold");
 *   
 *   MSDPSetArray(ch->desc, eMSDP_INVENTORY, inventory_buffer);
 *   
 *   // Build exits array
 *   sprintf(exits_buffer, "%c%s%c%s%c%s%c%s",
 *           MSDP_VAL, "north",
 *           MSDP_VAL, "south",
 *           MSDP_VAL, "east",
 *           MSDP_VAL, "up");
 *   
 *   MSDPSetArray(ch->desc, eMSDP_ROOM_EXITS, exits_buffer);
 * @endcode
 * 
 * @note Value must be pre-formatted with MSDP_VAL markers
 * @note Each array element should be a null-terminated string
 * @see MSDPSetString(), MSDPSetTable(), MSDP_VAL
 */
void MSDPSetArray(descriptor_t *apDescriptor, variable_t aMSDP, const char *apValue);

/******************************************************************************
 *                           MSSP FUNCTIONS
 * 
 * MUD Server Status Protocol (MSSP) functions for advertising server
 * information to MUD listing services and monitoring tools.
 * 
 * MSSP provides standardized server information including:
 * - Player count and capacity
 * - Server uptime and version
 * - Codebase and contact information  
 * - Game features and settings
 * - Connection and website details
 *****************************************************************************/

/**
 * Update current player count for MSSP reporting
 * 
 * Sets the current number of connected players for MSSP advertisement.
 * This information is used by MUD listing services to display accurate
 * player counts and help players find active games.
 * 
 * INITIALIZATION:
 * - First call also records server uptime (startup time)
 * - Subsequent calls only update player count
 * - Uptime remains constant until server restart
 * 
 * USAGE FREQUENCY:
 * - Should be called whenever player count changes
 * - Typically called every game pulse (0.1 seconds)
 * - Used by MUD listing services for real-time data
 * 
 * @param aPlayers Current number of connected players
 * 
 * @usage Update player count regularly:
 * @code
 *   // In main game loop
 *   int player_count = 0;
 *   for (d = descriptor_list; d; d = d->next) {
 *       if (STATE(d) == CON_PLAYING)
 *           player_count++;
 *   }
 *   MSSPSetPlayers(player_count);
 *   
 *   // Or use existing counting function
 *   MSSPSetPlayers(get_player_count());
 * @endcode
 * 
 * @note First call records uptime - call early in server startup
 * @note Safe to call frequently - no significant overhead
 * @see MSSP protocol documentation for full variable list
 */
void MSSPSetPlayers(int aPlayers);

/******************************************************************************
 *                           MXP FUNCTIONS
 * 
 * MUD eXtension Protocol (MXP) functions for enhanced markup, hyperlinks,
 * and interactive elements. MXP allows rich formatting and clickable
 * elements in compatible clients.
 * 
 * MXP FEATURES:
 * - Clickable hyperlinks and commands
 * - Rich text formatting (colors, fonts, styles)
 * - Interactive forms and input elements
 * - Secure mode prevents client-side command injection
 * - Graceful fallback for non-supporting clients
 *****************************************************************************/

/**
 * Create an MXP tag with secure line protection
 * 
 * Wraps the provided MXP tag in secure line markers to prevent client
 * command injection while enabling rich formatting and interactivity.
 * Returns the formatted tag if client supports MXP, or the original
 * tag if MXP is not supported.
 * 
 * SECURITY:
 * - Uses secure line mode to prevent command injection
 * - Client cannot execute arbitrary commands via MXP
 * - Safe to use with user-generated content
 * - Tags are validated before transmission
 * 
 * CLIENT COMPATIBILITY:
 * - Returns formatted tag for MXP-supporting clients
 * - Returns original tag for non-MXP clients (visible as text)
 * - Recommend checking MXP support before using
 * 
 * @param apDescriptor Client connection descriptor
 * @param apTag MXP tag to format (e.g., "<send href='look'>examine</send>")
 * @return Formatted MXP tag or original tag (static buffer)
 * 
 * @usage Create interactive elements:
 * @code
 *   // Check MXP support first
 *   if (descriptor->pProtocol && descriptor->pProtocol->bMXP) {
 *       const char *tag = MXPCreateTag(descriptor, "<send href='north'>Go North</send>");
 *       write_to_output(descriptor, "%s\r\n", tag);
 *   } else {
 *       // Fallback for non-MXP clients
 *       write_to_output(descriptor, "[north]\r\n");
 *   }
 *   
 *   // Color and formatting
 *   const char *colored = MXPCreateTag(descriptor, "<color fore='red'>Warning!</color>");
 *   
 *   // Clickable commands
 *   const char *clickable = MXPCreateTag(descriptor, "<send href='score'>Check Stats</send>");
 * @endcode
 * 
 * @note Returns static buffer - copy if needed for later use
 * @note Better to use ProtocolOutput() for embedded MXP tags
 * @see MXPSendTag(), ProtocolOutput()
 */
const char *MXPCreateTag(descriptor_t *apDescriptor, const char *apTag);

/**
 * Send an MXP tag directly to the client
 * 
 * Creates and immediately transmits an MXP tag to the client, bypassing
 * the normal output processing. This is primarily used for protocol
 * initialization tags like VERSION queries.
 * 
 * IMMEDIATE TRANSMISSION:
 * - Sends tag directly without buffering
 * - Does not go through normal output processing
 * - Useful for protocol setup and initialization
 * - Bypasses color processing and other output filters
 * 
 * COMMON USES:
 * - VERSION tag to query client MXP version
 * - Protocol setup and initialization
 * - Out-of-band protocol commands
 * - Debug and testing commands
 * 
 * @param apDescriptor Client connection descriptor
 * @param apTag MXP tag to send immediately
 * 
 * @usage Protocol initialization:
 * @code
 *   // Query client MXP version during negotiation
 *   if (descriptor->pProtocol && descriptor->pProtocol->bMXP) {
 *       MXPSendTag(descriptor, "<VERSION>");
 *   }
 *   
 *   // Send setup commands
 *   MXPSendTag(descriptor, "<SUPPORT>+channels +ignore +music");
 *   
 *   // Enable specific MXP features
 *   MXPSendTag(descriptor, "<RECOMMEND_OPTION USE_MXP=1>");
 * @endcode
 * 
 * @note Sends immediately - does not wait for output flush
 * @note Mainly for protocol setup, not regular content
 * @see MXPCreateTag(), ProtocolOutput()
 */
void MXPSendTag(descriptor_t *apDescriptor, const char *apTag);

/******************************************************************************
 *                           SOUND FUNCTIONS
 * 
 * MUD Sound Protocol (MSP) and MSDP/GMCP sound support functions for
 * audio triggers and sound effects. Provides immersive audio experience
 * for compatible clients.
 * 
 * SOUND FEATURES:
 * - Background music and ambient sounds
 * - Action-triggered sound effects (combat, spells, etc.)
 * - Location-based audio (different sounds per area/room)
 * - Volume and loop control
 * - Multiple protocol support (MSP, MSDP, GMCP)
 *****************************************************************************/

/**
 * Send sound trigger to client
 * 
 * Transmits a sound trigger to the client using the best available
 * protocol. Automatically chooses MSDP/GMCP if supported, otherwise
 * falls back to traditional MSP protocol.
 * 
 * PROTOCOL SELECTION:
 * 1. MSDP: If client supports MSDP, send via MSDP SOUND variable
 * 2. GMCP: If client supports GMCP, send via GMCP sound module
 * 3. MSP: Fallback to direct MSP protocol commands
 * 4. None: Silent failure if no sound protocols supported
 * 
 * SOUND TRIGGER FORMAT:
 * - Relative path and filename (e.g., "combat/sword_hit.wav")
 * - Client downloads from MUD's sound directory
 * - Supports common audio formats (WAV, MP3, OGG)
 * - Can include volume and loop parameters
 * 
 * CLIENT BEHAVIOR:
 * - Client downloads sound files as needed
 * - Files are cached locally for performance
 * - Volume controlled by client settings
 * - Graceful handling if file not found
 * 
 * @param apDescriptor Client connection descriptor
 * @param apTrigger Sound file path relative to MUD sound directory
 * 
 * @usage Trigger sounds for game events:
 * @code
 *   // Combat sounds
 *   SoundSend(ch->desc, "combat/sword_hit.wav");
 *   SoundSend(victim->desc, "combat/take_damage.wav");
 *   
 *   // Spell casting
 *   SoundSend(ch->desc, "magic/fireball.wav");
 *   
 *   // Environmental sounds
 *   if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_FOREST)) {
 *       SoundSend(ch->desc, "ambient/forest_birds.wav");
 *   }
 *   
 *   // Movement sounds
 *   SoundSend(ch->desc, "movement/footsteps_stone.wav");
 *   
 *   // Item interactions
 *   SoundSend(ch->desc, "items/door_open.wav");
 *   SoundSend(ch->desc, "items/chest_close.wav");
 * @endcode
 * 
 * @note Sound files should be in MUD's public sound directory
 * @note Graceful fallback if client doesn't support sound
 * @note No error if sound file doesn't exist - client handles silently
 * @see MSDP SOUND variable, MSP protocol documentation
 */
void SoundSend(descriptor_t *apDescriptor, const char *apTrigger);

/******************************************************************************
 *                           COLOR FUNCTIONS
 * 
 * Color processing functions for advanced color support including RGB colors,
 * ANSI fallbacks, and client capability detection. Provides rich color
 * experience while maintaining compatibility with older terminals.
 * 
 * COLOR FEATURES:
 * - RGB color support for 256-color and true-color terminals
 * - Automatic fallback to ANSI colors for older clients
 * - Client capability detection and adaptation
 * - Foreground and background color support
 * - Integration with LuminariMUD's \t color system
 *****************************************************************************/

/**
 * Convert RGB values to appropriate color escape codes
 * 
 * Generates color escape sequences based on RGB values, automatically
 * adapting to client color capabilities. Returns 256-color escape codes
 * for supporting clients, or best-fit ANSI colors for older terminals.
 * 
 * RGB FORMAT:
 * - 4-character string: [F/B][R][G][B]
 * - F/B: 'F' or 'f' for foreground, 'B' or 'b' for background
 * - R/G/B: Digits 0-5 representing red, green, blue intensity
 * - Examples: "F500" (red), "B035" (cyan bg), "f555" (white)
 * 
 * COLOR ADAPTATION:
 * - 256-color clients: Full RGB color mapping
 * - ANSI clients: Best-fit from 16-color palette
 * - Monochrome clients: No color codes
 * - Invalid colors: Reset to default
 * 
 * PERFORMANCE:
 * - Color codes are cached for efficiency
 * - Client capabilities detected once per connection
 * - Minimal processing for unsupported clients
 * 
 * @param apDescriptor Client connection descriptor
 * @param apRGB 4-character RGB color specification
 * @return Color escape sequence appropriate for client
 * 
 * @usage Generate dynamic colors:
 * @code
 *   // Direct RGB colors
 *   const char *red_fg = ColourRGB(d, "F500");      // Bright red foreground
 *   const char *blue_bg = ColourRGB(d, "B005");     // Bright blue background
 *   const char *orange = ColourRGB(d, "F530");      // Orange foreground
 *   
 *   write_to_output(d, "%sThis is red text%s\r\n", red_fg, ColourRGB(d, "F999"));
 *   
 *   // Health bar colors based on percentage
 *   int health_pct = (GET_HIT(ch) * 100) / GET_MAX_HIT(ch);
 *   char color_code[5];
 *   if (health_pct > 75) {
 *       strcpy(color_code, "F050");      // Green for healthy
 *   } else if (health_pct > 25) {
 *       strcpy(color_code, "F550");      // Yellow for wounded
 *   } else {
 *       strcpy(color_code, "F500");      // Red for critical
 *   }
 *   
 *   write_to_output(d, "%sHealth: %d/%d%s\r\n", 
 *                   ColourRGB(d, color_code), 
 *                   GET_HIT(ch), GET_MAX_HIT(ch),
 *                   ColourRGB(d, "F999")); // Reset
 * @endcode
 * 
 * @note RGB values range from 0 (darkest) to 5 (brightest)
 * @note Invalid color codes return reset sequence
 * @note Prefer ProtocolOutput() for embedded colors in strings
 * @see ProtocolOutput(), color system documentation
 */
const char *ColourRGB(descriptor_t *apDescriptor, const char *apRGB);

/******************************************************************************
 *                           UNICODE FUNCTIONS
 * 
 * UTF-8 encoding functions for Unicode character support. Enables display
 * of special characters, symbols, and international text on supporting
 * clients while providing ASCII fallbacks for older terminals.
 * 
 * UNICODE FEATURES:
 * - Full UTF-8 encoding support
 * - Unicode code point to UTF-8 conversion
 * - Dynamic string building with Unicode characters
 * - ASCII fallback system for compatibility
 * - Support for symbols, arrows, boxes, and international characters
 *****************************************************************************/

/**
 * Convert Unicode code point to UTF-8 sequence
 * 
 * Converts a Unicode code point (decimal value) to its corresponding
 * UTF-8 byte sequence. This allows display of special characters and
 * symbols on UTF-8 capable clients.
 * 
 * UTF-8 ENCODING:
 * - Handles Unicode code points from 0 to 0x10FFFF
 * - Returns 1-4 byte UTF-8 sequences as appropriate
 * - Properly encodes multi-byte characters
 * - Returns ASCII for code points 0-127
 * 
 * COMMON UNICODE RANGES:
 * - 0-127: ASCII characters
 * - 128-255: Latin-1 supplement
 * - 9728-9983: Miscellaneous symbols (✓, ✗, ☀, ☁)
 * - 9984-10175: Dingbats (✈, ✉, ✎, ✏)
 * - 8592-8703: Arrows (←, →, ↑, ↓)
 * - 9472-9599: Box drawing characters
 * 
 * @param aValue Unicode code point (decimal)
 * @return UTF-8 encoded string (static buffer)
 * 
 * @usage Convert common symbols:
 * @code
 *   // Common symbols
 *   char *checkmark = UnicodeGet(10003);     // ✓ Check mark
 *   char *crossmark = UnicodeGet(10007);     // ✗ Cross mark
 *   char *heart = UnicodeGet(9829);          // ♥ Heart
 *   char *diamond = UnicodeGet(9830);        // ♦ Diamond
 *   
 *   // Directional arrows
 *   char *north_arrow = UnicodeGet(8593);    // ↑ Up arrow
 *   char *south_arrow = UnicodeGet(8595);    // ↓ Down arrow
 *   char *east_arrow = UnicodeGet(8594);     // → Right arrow
 *   char *west_arrow = UnicodeGet(8592);     // ← Left arrow
 *   
 *   // Gender symbols (defined as constants)
 *   char *male = UnicodeGet(UNICODE_MALE);     // ♂ Male symbol
 *   char *female = UnicodeGet(UNICODE_FEMALE); // ♀ Female symbol
 *   
 *   // Usage in output
 *   write_to_output(d, "Status: %s Complete\r\n", checkmark);
 *   write_to_output(d, "Go %s to continue\r\n", north_arrow);
 * @endcode
 * 
 * @note Returns static buffer - copy if needed for later use
 * @note Invalid code points return empty string
 * @see UnicodeAdd(), ProtocolOutput() for embedded Unicode
 */
char *UnicodeGet(int aValue);

/**
 * Append UTF-8 sequence to existing string
 * 
 * Appends the UTF-8 encoding of a Unicode code point to an existing
 * string buffer, extending the string length appropriately. This is
 * useful for building strings with multiple Unicode characters.
 * 
 * STRING BUILDING:
 * - Appends to existing string without null terminator
 * - Extends string length by 1-4 bytes as needed
 * - Does not add null terminator (caller must add)
 * - Modifies string pointer if reallocation needed
 * 
 * MEMORY MANAGEMENT:
 * - May reallocate string buffer if needed
 * - Updates string pointer if buffer moves
 * - Caller responsible for final null termination
 * - Caller responsible for freeing allocated memory
 * 
 * @param apString Pointer to string pointer (may be modified)
 * @param aValue Unicode code point to append
 * 
 * @usage Build Unicode strings:
 * @code
 *   char *symbol_string = malloc(100);
 *   *symbol_string = '\0';  // Start with empty string
 *   
 *   // Build a string with multiple Unicode symbols
 *   UnicodeAdd(&symbol_string, 9733);    // ★ Star
 *   UnicodeAdd(&symbol_string, 32);      // Space
 *   UnicodeAdd(&symbol_string, 9829);    // ♥ Heart
 *   UnicodeAdd(&symbol_string, 32);      // Space
 *   UnicodeAdd(&symbol_string, 9830);    // ♦ Diamond
 *   
 *   // Don't forget null terminator
 *   strcat(symbol_string, "\0");
 *   
 *   write_to_output(d, "Symbols: %s\r\n", symbol_string);
 *   free(symbol_string);
 *   
 *   // Building directional indicators
 *   char *direction_string = strdup("Go ");
 *   UnicodeAdd(&direction_string, 8594);  // → Right arrow
 *   strcat(direction_string, " to exit");
 *   
 *   write_to_output(d, "%s\r\n", direction_string);
 *   free(direction_string);
 * @endcode
 * 
 * @note String pointer may change due to reallocation
 * @note Caller must add null terminator when finished
 * @note Caller responsible for memory management
 * @see UnicodeGet(), ProtocolOutput()
 */
void UnicodeAdd(char **apString, int aValue);

/******************************************************************************
 *                           USAGE EXAMPLES
 * 
 * Complete examples showing how to integrate the protocol system into
 * a MUD codebase, from basic setup to advanced features.
 *****************************************************************************/

/*
 * BASIC SETUP EXAMPLE:
 * 
 * // 1. In descriptor initialization (comm.c or similar)
 * void init_descriptor(descriptor_t *d) {
 *     d->pProtocol = ProtocolCreate();
 *     if (!d->pProtocol) {
 *         log("SYSERR: Failed to create protocol structure");
 *         return;
 *     }
 * }
 * 
 * // 2. Start negotiation after connection or login
 * void start_protocol_negotiation(descriptor_t *d) {
 *     if (d->pProtocol) {
 *         ProtocolNegotiate(d);
 *     }
 * }
 * 
 * // 3. Process input in main input loop
 * void process_input(descriptor_t *d, char *input, int length) {
 *     char clean_input[MAX_INPUT_LENGTH];
 *     ssize_t processed = ProtocolInput(d, input, length, clean_input);
 *     
 *     if (processed > 0) {
 *         command_interpreter(d->character, clean_input);
 *     }
 * }
 * 
 * // 4. Process output before sending
 * void send_to_char(struct char_data *ch, const char *txt) {
 *     if (ch->desc && ch->desc->pProtocol) {
 *         int length = strlen(txt);
 *         const char *output = ProtocolOutput(ch->desc, txt, &length);
 *         write_to_descriptor(ch->desc, output, length);
 *     }
 * }
 * 
 * // 5. Clean up on disconnect
 * void free_descriptor(descriptor_t *d) {
 *     if (d->pProtocol) {
 *         ProtocolDestroy(d->pProtocol);
 *         d->pProtocol = NULL;
 *     }
 * }
 */

/*
 * MSDP UPDATE EXAMPLE:
 * 
 * // Update function called every game pulse (0.1 seconds)
 * void update_msdp_data(void) {
 *     descriptor_t *d;
 *     struct char_data *ch;
 *     
 *     for (d = descriptor_list; d; d = d->next) {
 *         if (STATE(d) != CON_PLAYING || !(ch = d->character))
 *             continue;
 *             
 *         if (IS_NPC(ch) || !d->pProtocol)
 *             continue;
 *             
 *         // Character stats
 *         MSDPSetNumber(d, eMSDP_HEALTH, GET_HIT(ch));
 *         MSDPSetNumber(d, eMSDP_HEALTH_MAX, GET_MAX_HIT(ch));
 *         MSDPSetNumber(d, eMSDP_PSP, GET_PSP(ch));
 *         MSDPSetNumber(d, eMSDP_PSP_MAX, GET_MAX_PSP(ch));
 *         MSDPSetNumber(d, eMSDP_MOVEMENT, GET_MOVE(ch));
 *         MSDPSetNumber(d, eMSDP_MOVEMENT_MAX, GET_MAX_MOVE(ch));
 *         
 *         // Character information
 *         MSDPSetString(d, eMSDP_CHARACTER_NAME, GET_NAME(ch));
 *         MSDPSetString(d, eMSDP_RACE, race_list[GET_RACE(ch)].name);
 *         MSDPSetString(d, eMSDP_CLASS, class_list[GET_CLASS(ch)].name);
 *         MSDPSetNumber(d, eMSDP_LEVEL, GET_LEVEL(ch));
 *         
 *         // Ability scores
 *         MSDPSetNumber(d, eMSDP_STR, GET_STR(ch));
 *         MSDPSetNumber(d, eMSDP_INT, GET_INT(ch));
 *         MSDPSetNumber(d, eMSDP_WIS, GET_WIS(ch));
 *         MSDPSetNumber(d, eMSDP_DEX, GET_DEX(ch));
 *         MSDPSetNumber(d, eMSDP_CON, GET_CON(ch));
 *         MSDPSetNumber(d, eMSDP_CHA, GET_CHA(ch));
 *         
 *         // World information
 *         MSDPSetString(d, eMSDP_ROOM_NAME, world[IN_ROOM(ch)].name);
 *         MSDPSetNumber(d, eMSDP_ROOM_VNUM, world[IN_ROOM(ch)].number);
 *         MSDPSetString(d, eMSDP_AREA_NAME, zone_table[world[IN_ROOM(ch)].zone].name);
 *         
 *         // Combat information
 *         if (FIGHTING(ch)) {
 *             MSDPSetString(d, eMSDP_OPPONENT_NAME, GET_NAME(FIGHTING(ch)));
 *             MSDPSetNumber(d, eMSDP_OPPONENT_HEALTH, GET_HIT(FIGHTING(ch)));
 *             MSDPSetNumber(d, eMSDP_OPPONENT_HEALTH_MAX, GET_MAX_HIT(FIGHTING(ch)));
 *             MSDPSetNumber(d, eMSDP_OPPONENT_LEVEL, GET_LEVEL(FIGHTING(ch)));
 *         } else {
 *             MSDPSetString(d, eMSDP_OPPONENT_NAME, "");
 *             MSDPSetNumber(d, eMSDP_OPPONENT_HEALTH, 0);
 *             MSDPSetNumber(d, eMSDP_OPPONENT_HEALTH_MAX, 0);
 *             MSDPSetNumber(d, eMSDP_OPPONENT_LEVEL, 0);
 *         }
 *         
 *         // Send all dirty variables
 *         MSDPUpdate(d);
 *     }
 *     
 *     // Update MSSP player count
 *     MSSPSetPlayers(count_playing_players());
 * }
 */

/*
 * SPECIALIZED UPDATE FUNCTIONS:
 * 
 * // Update room information when character moves
 * void update_msdp_room(struct char_data *ch) {
 *     descriptor_t *d = ch->desc;
 *     if (!d || !d->pProtocol) return;
 *     
 *     // Update room data
 *     MSDPSetString(d, eMSDP_ROOM_NAME, world[IN_ROOM(ch)].name);
 *     MSDPSetNumber(d, eMSDP_ROOM_VNUM, world[IN_ROOM(ch)].number);
 *     MSDPSetString(d, eMSDP_AREA_NAME, zone_table[world[IN_ROOM(ch)].zone].name);
 *     
 *     // Build exits array
 *     char exits_buf[MAX_STRING_LENGTH];
 *     build_exits_array(ch, exits_buf);
 *     MSDPSetArray(d, eMSDP_ROOM_EXITS, exits_buf);
 *     
 *     // Send immediately for responsive movement
 *     MSDPFlush(d, eMSDP_ROOM_NAME);
 *     MSDPFlush(d, eMSDP_ROOM_VNUM);
 *     MSDPFlush(d, eMSDP_AREA_NAME);
 *     MSDPFlush(d, eMSDP_ROOM_EXITS);
 * }
 * 
 * // Update spell effects when spells are cast or expire
 * void update_msdp_affects(struct char_data *ch) {
 *     descriptor_t *d = ch->desc;
 *     if (!d || !d->pProtocol) return;
 *     
 *     char affects_buf[MAX_STRING_LENGTH];
 *     build_affects_array(ch, affects_buf);
 *     MSDPSetArray(d, eMSDP_AFFECTS, affects_buf);
 *     MSDPFlush(d, eMSDP_AFFECTS);
 * }
 * 
 * // Update inventory when items are gained/lost
 * void update_msdp_inventory(struct char_data *ch) {
 *     descriptor_t *d = ch->desc;
 *     if (!d || !d->pProtocol) return;
 *     
 *     char inventory_buf[MAX_STRING_LENGTH];
 *     build_inventory_array(ch, inventory_buf);
 *     MSDPSetArray(d, eMSDP_INVENTORY, inventory_buf);
 *     MSDPFlush(d, eMSDP_INVENTORY);
 * }
 */

/*
 * COLOR AND FORMATTING EXAMPLES:
 * 
 * // Basic color usage with \t codes
 * void send_colored_message(struct char_data *ch, const char *message) {
 *     // Colors are processed by ProtocolOutput()
 *     send_to_char(ch, "\trThis is red text.\tn\r\n");
 *     send_to_char(ch, "\tGThis is bright green.\tn\r\n");
 *     send_to_char(ch, "\t[F500]This is RGB red.\tn\r\n");
 *     send_to_char(ch, "\t[U9733/\*]This is a star symbol.\tn\r\n");
 * }
 * 
 * // Dynamic RGB colors based on game state
 * void send_health_bar(struct char_data *ch) {
 *     int health_pct = (GET_HIT(ch) * 100) / GET_MAX_HIT(ch);
 *     const char *color;
 *     
 *     if (health_pct > 75) {
 *         color = ColourRGB(ch->desc, "F050");  // Green
 *     } else if (health_pct > 50) {
 *         color = ColourRGB(ch->desc, "F550");  // Yellow
 *     } else if (health_pct > 25) {
 *         color = ColourRGB(ch->desc, "F520");  // Orange
 *     } else {
 *         color = ColourRGB(ch->desc, "F500");  // Red
 *     }
 *     
 *     send_to_char(ch, "%sHealth: %d/%d\tn\r\n", 
 *                  color, GET_HIT(ch), GET_MAX_HIT(ch));
 * }
 * 
 * // MXP clickable elements
 * void send_mxp_menu(struct char_data *ch) {
 *     if (ch->desc->pProtocol && ch->desc->pProtocol->bMXP) {
 *         send_to_char(ch, "\t<send href='north'>Go North\t</send>\r\n");
 *         send_to_char(ch, "\t<send href='look'>Look Around\t</send>\r\n");
 *         send_to_char(ch, "\t<send href='inventory'>Check Inventory\t</send>\r\n");
 *     } else {
 *         send_to_char(ch, "[North] [Look] [Inventory]\r\n");
 *     }
 * }
 */

/*
 * ERROR HANDLING AND DEBUGGING:
 * 
 * // Safe protocol function calls
 * void safe_msdp_update(struct char_data *ch) {
 *     if (!ch || !ch->desc || !ch->desc->pProtocol) {
 *         return; // Silently handle missing protocol
 *     }
 *     
 *     // Check if client supports MSDP or GMCP
 *     if (!ch->desc->pProtocol->bMSDP && !ch->desc->pProtocol->bGMCP) {
 *         return; // No point updating if client can't receive
 *     }
 *     
 *     // Proceed with updates
 *     MSDPSetNumber(ch->desc, eMSDP_HEALTH, GET_HIT(ch));
 *     MSDPUpdate(ch->desc);
 * }
 * 
 * // Debug protocol status
 * void debug_protocol_status(struct char_data *ch) {
 *     protocol_t *p = ch->desc->pProtocol;
 *     if (!p) {
 *         send_to_char(ch, "No protocol structure.\r\n");
 *         return;
 *     }
 *     
 *     send_to_char(ch, "Protocol Status:\r\n");
 *     send_to_char(ch, "  MSDP: %s\r\n", p->bMSDP ? "YES" : "NO");
 *     send_to_char(ch, "  GMCP: %s\r\n", p->bGMCP ? "YES" : "NO");
 *     send_to_char(ch, "  MXP:  %s\r\n", p->bMXP ? "YES" : "NO");
 *     send_to_char(ch, "  MSP:  %s\r\n", p->bMSP ? "YES" : "NO");
 *     send_to_char(ch, "  NAWS: %dx%d\r\n", p->ScreenWidth, p->ScreenHeight);
 *     send_to_char(ch, "  256 Colors: %s\r\n", 
 *                  p->b256Support == eYES ? "YES" : 
 *                  p->b256Support == eSOMETIMES ? "SOMETIMES" : "NO");
 * }
 */

/******************************************************************************
 *                           INTEGRATION NOTES
 * 
 * PERFORMANCE CONSIDERATIONS:
 * - Call MSDPUpdate() every game pulse (0.1s) for real-time data
 * - Use MSDPFlush() sparingly for immediate critical updates
 * - Dirty flag system prevents unnecessary network traffic
 * - Per-descriptor buffers prevent memory allocation overhead
 * 
 * MEMORY MANAGEMENT:
 * - Protocol structures are automatically managed
 * - String values are properly allocated/deallocated
 * - Always call ProtocolDestroy() when closing connections
 * - No memory leaks when used correctly
 * 
 * CLIENT COMPATIBILITY:
 * - Automatic fallback from MSDP to GMCP
 * - Graceful degradation for unsupported features
 * - Color system adapts to client capabilities
 * - Safe to use with any client (including telnet)
 * 
 * DEBUGGING:
 * - Use ReportBug() function for internal errors
 * - Check return values from ProtocolInput()
 * - Validate pointers before using protocol functions
 * - Test with multiple client types
 * 
 * SECURITY:
 * - MXP secure line mode prevents command injection
 * - Input validation prevents buffer overflows  
 * - Per-descriptor buffers prevent race conditions
 * - Protocol data is properly escaped
 * 
 * For more information, see:
 * - docs/systems/PROTOCOL_SYSTEMS.md
 * - KaVir's Protocol Snippet documentation
 * - MSDP, GMCP, and MXP protocol specifications
 *****************************************************************************/

#endif /* PROTOCOL_H */
