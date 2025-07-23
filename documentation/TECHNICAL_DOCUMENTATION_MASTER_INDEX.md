# LuminariMUD Technical Documentation Master Index

## Project Overview

LuminariMUD is a text-based multiplayer online role-playing game (MUD) server based on tbaMUD/CircleMUD. It implements Pathfinder/D&D 3.5 rules and features an original world inspired by Biblical, Dragonlance, and Forgotten Realms stories.

**Repository:** https://github.com/LuminariMUD/Luminari-Source  
**Discord:** https://discord.gg/Me3Tuu4  
**License:** GPL (see LICENSE file)

## Documentation Structure

This comprehensive technical documentation is organized into the following major sections:

### 1. Core Architecture Documentation
- **[Server Architecture](CORE_SERVER_ARCHITECTURE.md)** - Main server loop, initialization, networking
- **[Data Structures](DATA_STRUCTURES_AND_MEMORY.md)** - Core data structures and memory management
- **[Database Integration](DATABASE_INTEGRATION.md)** - MySQL integration and schema

### 2. Game Systems Documentation
- **[Player Management](PLAYER_MANAGEMENT_SYSTEM.md)** - Player creation, authentication, persistence
- **[World Simulation](WORLD_SIMULATION_SYSTEM.md)** - Rooms, zones, wilderness, resets
- **[Combat System](COMBAT_SYSTEM.md)** - Combat mechanics, initiative, damage calculations
- **[Command System](COMMAND_SYSTEM_AND_INTERPRETER.md)** - Command parsing and execution

### 3. Advanced Systems Documentation
- **[Scripting System](SCRIPTING_SYSTEM_DG.md)** - DG Scripts, triggers, variables
- **[Game Mechanics](GAME_MECHANICS_SYSTEMS.md)** - Classes, races, feats, spells, skills
- **[OLC System](OLC_ONLINE_CREATION_SYSTEM.md)** - Online level creation tools
- **[Utility Systems](UTILITY_SYSTEMS.md)** - Logging, events, mail, boards, clans

### 4. Development and Maintenance
- **[Setup and Build](SETUP_AND_BUILD_GUIDE.md)** - Compilation, installation, configuration
- **[Developer Guide](DEVELOPER_GUIDE_AND_API.md)** - Coding standards, API reference
- **[Troubleshooting](TROUBLESHOOTING_AND_MAINTENANCE.md)** - Common issues, debugging, optimization

## Key Technical Characteristics

### Architecture Pattern
- **Event-driven server architecture** with main game loop
- **Socket-based networking** for player connections
- **Modular design** with clear separation of concerns
- **Plugin-based scripting system** (DG Scripts)

### Core Technologies
- **Language:** C (C99 standard)
- **Database:** MySQL integration
- **Networking:** BSD sockets
- **Memory Management:** Custom allocation with debugging support
- **Build System:** GNU Make

### Key Data Structures
- `struct char_data` - Player and NPC character data
- `struct room_data` - World room information
- `struct obj_data` - Game object data
- `struct descriptor_data` - Network connection management
- `struct zone_data` - Zone and area management

### Performance Considerations
- **Single-threaded design** with event-driven I/O
- **Memory pooling** for frequently allocated objects
- **Efficient data structures** (hash tables, linked lists)
- **Lazy loading** of world data where appropriate

## File Organization

### Core Files
- `comm.c/h` - Main server loop and networking
- `structs.h` - All major data structure definitions
- `db.c/h` - Database operations and world loading
- `interpreter.c/h` - Command parsing and execution
- `fight.c/h` - Combat system implementation

### System Modules
- `act.*.c` - Player action implementations
- `*edit.c` - OLC (Online Level Creation) editors
- `dg_*.c` - DG scripting system
- `spec_*.c` - Special procedures and abilities

### Utility Modules
- `utils.c/h` - Common utility functions
- `handler.c/h` - Object and character manipulation
- `constants.c/h` - Game constants and tables
- `config.c/h` - Configuration management

## Documentation Standards

### Format Guidelines
- **Markdown format** for all documentation
- **Code examples** with syntax highlighting
- **Cross-references** between related modules
- **Consistent naming conventions**

### Content Requirements
- **Architectural overview** for each system
- **Function signatures** with parameter descriptions
- **Data flow diagrams** where applicable
- **Configuration requirements**
- **Error handling patterns**
- **Performance considerations**

### Code Documentation
- **Doxygen-style comments** in source code
- **Function purpose and parameters**
- **Return value descriptions**
- **Side effects and preconditions**

## Legacy Code Considerations

### Historical Context
- Based on **CircleMUD 3.0** and **tbaMUD**
- Incorporates **CWG (Copper) MUD** enhancements
- **D20/Pathfinder** rule system integration
- **15+ years** of continuous development

### Modernization Notes
- **Deprecated functions** marked for replacement
- **Memory leak fixes** and safety improvements
- **Compiler warning** resolution
- **Modern C standards** adoption where possible

### Compatibility Issues
- **Legacy MySQL** API usage
- **Platform-specific** code sections
- **Outdated dependencies** requiring updates
- **Non-standard** C extensions

## Getting Started

### For Developers
1. Read the **[Setup and Build Guide](SETUP_AND_BUILD_GUIDE.md)**
2. Review **[Developer Guide](DEVELOPER_GUIDE_AND_API.md)**
3. Study **[Core Server Architecture](CORE_SERVER_ARCHITECTURE.md)**
4. Examine **[Data Structures](DATA_STRUCTURES_AND_MEMORY.md)**

### For System Administrators
1. Follow **[Setup and Build Guide](SETUP_AND_BUILD_GUIDE.md)**
2. Configure using **[Database Integration](DATABASE_INTEGRATION.md)**
3. Reference **[Troubleshooting Guide](TROUBLESHOOTING_AND_MAINTENANCE.md)**

### For Content Creators
1. Learn **[OLC System](OLC_ONLINE_CREATION_SYSTEM.md)**
2. Study **[Scripting System](SCRIPTING_SYSTEM_DG.md)**
3. Review **[Game Mechanics](GAME_MECHANICS_SYSTEMS.md)**

## Version Information

**Documentation Version:** 1.0  
**Last Updated:** 2025-01-23  
**Covers Codebase:** Current master branch  
**Maintainer:** Development Team

## Contributing to Documentation

See **[CONTRIBUTING.md](../CONTRIBUTING.md)** for guidelines on contributing to both code and documentation.

---

*This documentation is a living document and will be updated as the codebase evolves. For the most current information, always refer to the source code and this documentation together.*
