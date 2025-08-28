# Changelog

## [Unreleased] - August 28, 2025

### Deployment System - Complete Overhaul (100% Complete)

#### Added
- **Simple setup script** (`scripts/simple_setup.sh`) - Zero-interaction deployment in under 2 minutes
  - Automatic configuration file setup from examples
  - Build system execution with error handling
  - Symlink creation for world/, text/, and etc/ directories
  - World file initialization with proper index naming
  - Text file creation for all required game files
  - Directory structure setup for player files

#### Fixed
- **Deploy script path navigation** - Script now correctly navigates from scripts/ to project root
- **World file copying bugs** - Fixed incorrect wildcard usage that copied wrong index files
- **Index file naming** - Files are now properly renamed from `index.xxx` to `index`
- **Symlink creation** - Automatically creates required symlinks (world, text, etc)
- **HLQ directory** - Added missing Homeland Quest directory and index
- **Text file initialization** - All required text files now created automatically

#### Changed
- **Deployment workflow** - Simplified from complex manual process to single script execution
- **Error handling** - Graceful MySQL bypass when database not configured
- **Documentation** - Updated all deployment guides with working instructions

## [Unreleased] - August 26, 2025

### Intermud3 Integration - Complete Repair and Enhancement (100% Complete)

#### Added
- **Complete Intermud3 client implementation** - Full thread-safe inter-MUD communication system
  - `src/systems/intermud3/i3_client.c` - Core threaded client with event queuing (901 lines)
  - `src/systems/intermud3/i3_client.h` - Complete API definitions and data structures (215 lines)
  - `src/systems/intermud3/i3_commands.c` - All player and admin commands implemented (602 lines)
- **Thread-safe architecture** - Producer-consumer event queuing between I3 thread and main game thread
- **Complete command set** - All inter-MUD communication features:
  - `i3tell <user>@<mud> <message>` - Send tells across MUD network
  - `i3chat [channel] <message>` - Multi-MUD channel communication
  - `i3who <mud>` - Query remote MUD player lists
  - `i3finger <user>@<mud>` - Get remote player information
  - `i3locate <user>` - Search for users across network
  - `i3mudlist` - List all connected MUDs on network
  - `i3channels list|join|leave [channel]` - Channel subscription management
  - `i3config` - Toggle I3 features on/off
  - `i3admin status|stats|reconnect|reload|save` - Administrative functions
- **JSON-RPC 2.0 protocol compliance** - Full implementation of I3 Gateway protocol
- **Configuration system** - File-based configuration in `lib/i3_config`
- **Event processing integration** - Seamless integration with main game heartbeat

#### Fixed
- **All critical security vulnerabilities** identified in CircleMUD client audit:
  - Buffer overflow vulnerabilities in message processing
  - Use-after-free and memory corruption issues  
  - Format string vulnerabilities in logging
  - Threading safety violations and race conditions
  - Resource leaks in socket and memory management
- **Complete protocol implementation** - All previously stub functions now fully implemented:
  - `i3_request_who()` - Remote player list queries
  - `i3_request_finger()` - Remote player information
  - `i3_request_locate()` - Cross-network user location
  - `i3_request_mudlist()` - Network MUD directory
  - `i3_join_channel()` / `i3_leave_channel()` - Channel management
  - `i3_send_emoteto()` / `i3_send_channel_emote()` - Emote support

#### Changed
- **Thread safety implementation**:
  - Proper mutex usage for all shared data structures
  - Event queuing prevents cross-thread character_list access
  - Safe message passing between I3 thread and main thread
- **Memory management**:
  - Proper JSON object cleanup with `json_object_put()`
  - Bounds checking on all string operations using `strncpy()`
  - Resource tracking and cleanup on shutdown
- **Error handling**:
  - Comprehensive input validation and sanitization
  - Graceful handling of connection failures with auto-reconnect
  - Proper error propagation and logging
- **Build system integration**:
  - Added to both `Makefile.am` and `CMakeLists.txt`
  - All commands registered in `interpreter.c`
  - Main loop integration in `comm.c` heartbeat function

#### Technical Details
- **Architecture**: Event-driven design with thread-safe producer-consumer queues
- **Dependencies**: json-c library for JSON-RPC protocol, pthread for threading
- **Performance**: Non-blocking I/O, efficient queue operations, minimal main thread impact
- **Security**: Input validation, bounds checking, safe string operations throughout
- **Configuration**: `lib/i3_config` with gateway host, port, API key, and feature toggles

#### Testing and Documentation
- **Integration status document** created at `docs/systems/narrative-weaver/INTERMUD3_INTEGRATION_STATUS.md`
- **Updated audit report** reflects successful remediation of all security issues
- **Production readiness**: Full compliance with I3 Gateway specifications
- **Testing instructions**: Comprehensive guide for verifying functionality

### Vessel System - Phase 2 Progress (80% Complete)

#### Added
- **Multi-room vessel interiors** - Ships now support 1-20 dynamically generated interior rooms
  - `vessels_rooms.c` - New file implementing room generation, templates, and connections (573 lines)
  - `vessels_docking.c` - New file implementing docking and boarding mechanics (412 lines)
- **Room template system** - 10 different room types with dynamic descriptions:
  - Bridge, Quarters, Cargo Hold, Engineering, Weapons, Medical, Mess Hall, Corridor, Airlock
- **Vessel-specific room generation** - Different vessel types get appropriate room layouts:
  - Warships: Multiple weapon rooms, engineering
  - Transports: Multiple cargo holds
  - Smaller vessels: Fewer, more compact layouts
- **Ship-to-ship docking mechanics**:
  - `dock` command - Dock with nearby vessels
  - `undock` command - Separate from docked vessels
  - Automatic gangway creation between docked ships
  - Safety checks for proximity and speed
- **Combat boarding system**:
  - `board_hostile` command - Attempt hostile boarding
  - Skill-based success checks
  - Consequences for failure (damage, falling into water)
- **Interior viewing commands**:
  - `look_outside` - View wilderness from ship interior
  - `ship_rooms` - List all rooms in current vessel
- **Room connection algorithm** - Smart hub-and-spoke layout with cross-connections

#### Changed
- **Build system integration**:
  - Updated Makefile.am to include vessels_rooms.c and vessels_docking.c
  - Updated CMakeLists.txt with new source files
- **C89/C90 compatibility fixes**:
  - Changed all `number()` calls to `rand_number()`
  - Fixed room coordinate fields to use `coords[0]` and `coords[1]`
  - Corrected `damage()` function calls with proper parameters
  - Fixed room_flags array handling with SET_BIT_AR
  - Moved all variable declarations to block start (no C99 loop declarations)

#### Fixed
- Compilation errors related to undefined functions
- Type mismatches for room coordinate fields
- Incorrect damage() function parameters
- C99 loop variable declarations
- Room flags assignment for array-based flags

#### Technical Details
- **Files modified**: vessels.h, Makefile.am, CMakeLists.txt
- **New dependencies**: spells.h (for TYPE_UNDEFINED)
- **Functions implemented**:
  - `generate_ship_interior()` - Creates room layout based on vessel type
  - `complete_docking()` - Establishes connections between ships
  - `do_dock()`, `do_undock()` - Docking commands
  - `do_board_hostile()` - Combat boarding command
  - `do_look_outside()` - View external wilderness

#### Remaining Work
- Database persistence for ship configurations
- Interior movement integration with ship navigation
- NPC crew management
- Cargo transfer system completion
- Performance optimization
- Unit test suite creation
- Integration testing with live gameplay

