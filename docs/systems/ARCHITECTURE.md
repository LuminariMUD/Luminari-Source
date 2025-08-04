# LuminariMUD System Architecture

Last Updated: 2025-08-01  
Version: 1.0

## Overview

LuminariMUD is a sophisticated text-based multiplayer online role-playing game (MUD) server built on the proven tbaMUD/CircleMUD foundation. It implements authentic Pathfinder/D&D 3.5 mechanics with advanced features including dynamic scripting, comprehensive online building tools, and MySQL integration.

## Core Architecture Principles

### Language and Standards
- **Primary Language**: ANSI C90/C89 (NOT C99!)
- **Performance Monitoring**: C (perfmon.c)
- **Compiler Compliance**: GCC/Clang without -std=c99 flag
- **Platform Support**: Linux/Unix systems (CentOS), WSL/Windows compatible

### Design Philosophy
- **Modular Architecture**: Clear separation of concerns
- **Event-Driven Design**: Single-threaded with event queue
- **Real-Time Performance**: Optimized for hundreds of concurrent players
- **Data Persistence**: MySQL for player data, flat files for world data
- **Extensibility**: DG Scripts for dynamic content

## System Layers

### 1. Network Layer (comm.c)
- **Main Game Loop**: Handles all I/O operations
- **Connection Management**: Player connections and socket handling
- **Protocol Support**: Telnet, MSDP, GMCP via protocol.c
- **Buffer Management**: Input/output queuing
- **Descriptor Handling**: Player connection state management

### 2. Command Processing Layer
- **interpreter.c**: Command parsing and dispatch
- **Command Table**: Registration and permission checking
- **Alias System**: Player-defined command shortcuts
- **Abbreviation Matching**: Flexible command input

### 3. Game World Layer
- **World Database (db.c)**: 
  - Zone loading and management
  - Mobile (NPC) templates
  - Object templates
  - Room data structures
- **Zone System**:
  - Zone reset mechanics
  - Mobile and object spawning
  - Door state management
- **Wilderness System**: Dynamic overworld generation

### 4. Character Management Layer
- **Character Data Structure**: Comprehensive player/NPC representation
- **Account System (account.c)**: Multi-character account management
- **Player Files**: Binary format for fast loading
- **Descriptor Binding**: Link between network and character

### 5. Game Mechanics Layer
- **Combat System (fight.c)**:
  - Initiative-based combat
  - D&D 3.5 attack/damage calculations
  - Combat modes and stances
- **Magic System (magic.c, spells.c)**:
  - Spell preparation and memorization
  - Component-based casting
  - Spell effects and duration
- **Skills and Abilities**:
  - Skill checks and progression
  - Feat system implementation
  - Class abilities and features

### 6. Scripting Engine (DG Scripts)
- **Trigger System**: Event-based script execution
- **Script Types**:
  - Mobile scripts (NPCs)
  - Object scripts
  - Room scripts
- **Variable System**: Persistent script variables
- **Script Commands**: Extensive command set

### 7. Building System (OLC)
- **Online Creation**:
  - Room editor (redit)
  - Object editor (oedit)
  - Mobile editor (medit)
  - Zone editor (zedit)
- **Generic Functions**: Shared OLC utilities
- **Save Mechanisms**: Write to disk functionality

### 8. Database Integration Layer
- **MySQL Connection (mysql.c)**:
  - Connection pooling
  - Query execution
  - Result handling
- **Persistent Data**:
  - Player accounts
  - Character data
  - Mail system
  - Houses and clans
  - Crafting data

## Module Organization

### Core Engine Modules
```
comm.c          - Main loop and networking
interpreter.c   - Command processing
db.c            - World database
handler.c       - Core object manipulation
utils.c         - Utility functions
```

### Player Action Modules
```
act.comm.c      - Communication commands
act.informative.c - Information commands
act.movement.c  - Movement commands
act.offensive.c - Combat commands
act.other.c     - Miscellaneous commands
act.wizard.c    - Admin commands
act.item.c      - Item manipulation
act.social.c    - Social commands
```

### Game System Modules
```
fight.c         - Combat mechanics
magic.c         - Spell system
spells.c        - Spell implementations
psionics.c      - Psionic powers
class.c         - Character classes
race.c          - Character races
feats.c         - Feat system
```

### Advanced Feature Modules
```
crafting_new.c  - Crafting system
missions.c      - Mission/quest system
encounters.c    - Random encounters
hunts.c         - Hunt system
templates.c     - Character templates
trails.c        - Tracking system
```

### Infrastructure Modules
```
mysql.c         - Database integration
mud_event.c     - Event system
actionqueues.c  - Action queuing
protocol.c      - Client protocols
perfmon.c       - Performance monitoring
```

## Data Flow

### Command Execution Flow
1. Player input received in comm.c
2. Input parsed in interpreter.c
3. Command function called from cmd_info table
4. Command modifies game state
5. Output sent back through comm.c

### Combat Round Flow
1. Initiative calculated for all participants
2. Actions processed in initiative order
3. Attack rolls and damage calculated
4. Effects and conditions applied
5. Death/victory conditions checked

### Event Processing Flow
1. Events scheduled via mud_event system
2. Game loop checks event queue each pulse
3. Due events executed in order
4. Event handlers modify game state
5. Completed events removed from queue

## Memory Management

### Allocation Strategy
- **CREATE() Macro**: Standard allocation with clearing
- **Object Pools**: For frequently allocated structures
- **String Management**: Careful free/strdup patterns
- **Reference Counting**: For shared resources

### Critical Patterns
```c
// String allocation pattern
char *new_str = strdup(old_str);
if (target->str) free(target->str);
target->str = new_str;

// Object allocation pattern
CREATE(obj, struct obj_data, 1);
clear_object(obj);
```

## Performance Considerations

### Optimization Areas
- **Zone Reset**: O(n) algorithm (optimized from O(n²))
- **Mobile Activity**: Cached calculations and early exits
- **String Operations**: Minimized in hot paths
- **Database Queries**: Batched where possible

### Profiling Tools
- **gprof**: Function-level profiling
- **perfmon.c**: Custom performance profiler
- **valgrind**: Memory leak detection
- **System monitoring**: CPU and memory usage

## Security Architecture

### Input Validation
- All user input sanitized
- Buffer overflow protection
- SQL injection prevention
- Command permission checking

### Access Control
- Level-based permissions
- Immortal command restrictions
- Building access controls
- Database query limitations

## Extension Points

### Adding New Commands
1. Implement in appropriate act.*.c file
2. Register in interpreter.c cmd_info[]
3. Add help entry
4. Test permissions and edge cases

### Adding New Systems
1. Create new module files
2. Add initialization in db.c
3. Hook into event system if needed
4. Update save/load routines
5. Document in technical guides

### Scripting Extensions
1. Add new trigger types
2. Implement script commands
3. Update variable system
4. Test with existing scripts

## File Organization

### Source Layout
```
/               - Core source files
/unittests/     - Unit test framework
/util/          - Utility programs
/mysql/         - MySQL headers
/docs/          - Documentation
/lib/           - Game data files
/bin/           - Compiled binaries
```

### Header Files
- **structs.h**: Core data structures
- **utils.h**: Utility macros and functions
- **comm.h**: Communication definitions
- **db.h**: Database structures
- **spells.h**: Spell definitions
- **interpreter.h**: Command structures

## Build System

### Makefile Structure
- Main executable target
- Utility programs
- Dependency tracking
- Debug/profile builds
- Clean targets

### Configuration
- **campaign.h**: Campaign settings
- **mud_options.h**: Runtime options
- **vnums.h**: Virtual number assignments
- **conf.h**: System configuration

## Future Architecture Goals

### Planned Improvements
- Enhanced threading model
- Improved database abstraction
- Plugin system for modules
- Better script debugging tools
- Performance monitoring dashboard

### Technical Debt
- Legacy code modernization
- Consistent error handling
- Comprehensive unit tests
- Documentation updates
- Code style standardization

---

*This document provides a high-level overview of LuminariMUD's architecture. For detailed information about specific systems, consult the relevant documentation in the docs/systems/ directory.*