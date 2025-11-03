# CLAUDE.md - AI Assistant Development Guide

# **NOTE** Rename this file CLAUDE.md and make sure it is in the root folder of your project -- adjust as needed

## MySQL and OpenAI Credentials
You can access these in files:
lib/mysql_config
lib/.env

## IMPORTANT: Build and Compilation Policy
**DO NOT run make, build, or compilation commands unless explicitly asked by the user.** Focus on code analysis, fixes, and recommendations. Only execute build commands when the user specifically requests compilation or building.

**When you desire a build for any reason, simply pause, ask User to run it for you and report the results**

## Configuration Files (Not in Git)
**CRITICAL: NEVER EVER touch, overwrite, copy over, or modify ANY *.h configuration files. These contain custom configurations and MUST NOT be touched.**
src/campaign.h
src/mud_options.h
src/vnums.h
**Examples files yes could be editted if necessary:**
src/campaign.example.h
src/mud_options.example.h
src/vnums.example.h

## Sensitive Configuration Files (not in Git)
**CRITICAL: You can read, but otherwise:  NEVER EVER touch, overwrite, copy over, or modify ANY of these files. These contain passwords, keys, and other sensitive information.**
lib/mysql_config
lib/.env
**Examples files yes could be editted if necessary:**
lib/mysql_config_example
lib/.env.example

## Build System
  **We support both autotools and CMake, but prefer autotools for faster builds.**
  - **Autotools (preferred)**: Edit `Makefile.am`, then run `autoreconf -fiv && ./configure && make -j20`
  - **CMake**: Edit `CMakeLists.txt`, then run `cmake --build build/ -j20`
  - When adding new source files, update BOTH `Makefile.am` and `CMakeLists.txt` to maintain compatibility

## Quick Start for Common Tasks

### Adding a New Command
1. Implement function in appropriate `act.*.c` file using `ACMD()` macro
2. Register in `interpreter.c` cmd_info[] table
3. Add help entry via `hedit` or help files
4. Test with various permission levels and edge cases
5. Always trace code, never assume naming conventions

### Modifying Game Mechanics
1. Check dependencies in related systems
2. Maintain game balance considerations
3. Update help documentation
4. Test all affected systems
5. Ensure backwards compatibility

### Debugging / Performance Profiling
- Debug with GDB (debug_game.sh)
- Memory issues with Valgrind (vgrind.sh)
- Uses `perfmon.c` for performance profiling in game

## Project Overview

LuminariMUD is a sophisticated text-based multiplayer online role-playing game (MUD) server implementing authentic Pathfinder/D20/D&D 3.5 mechanics. Built on the proven tbaMUD/CircleMUD foundation, it features:

- **Authentic D&D 3.5 Rules / D20 / and Pathfinder-Inspired**: Complete implementation of classes, races, feats, skills, and spells
- **MySQL Integration**: Persistent player data, accounts, and world state
- **Performance Optimized**: Handles hundreds of concurrent players efficiently

### Current Version & Standards
- **LuminariMUD** (built on tbaMUD/CircleMUD/CWG [Circle with Goodies]/d20MUD [Stephen Squires])
- **Language**: C (ANSI C90/C89 - NOT C99!)
  - **IMPORTANT**: The codebase does NOT compile with C99 mode enabled
  - **DO NOT** use C99 features like:
    - Variable declarations inside for loops: `for (int i = 0; ...)`
    - Mixed declarations and code
    - Variable length arrays
  - **ALWAYS** declare variables at the beginning of blocks
- **Platform**: Linux/Unix systems (Ubuntu)
- **Database**: MariaDB
- **Compiler**: GCC/Clang (without -std=c99 flag)
- **Binary Output**: Main executable is output to `/bin/circle`
- Use the existing utility functions and macros defined in utils.h
- When adding new commands, register them in interpreter.c
- Use the existing logging macros (log(), mudlog()) for debugging

### Campaign System
The codebase supports multiple campaign settings:
- **Default LuminariMUD**: Standard fantasy setting (no CAMPAIGN_* defined)
- **CAMPAIGN_DL**: DragonLance (Chronicles of Krynn)
- **CAMPAIGN_FR**: Forgotten Realms (Faerun)

Different campaigns may have different features enabled/disabled (e.g., trail system is disabled for DL/FR campaigns).

## Architecture Overview

**This is NOT a comprehensive list**

### Core Engine
- **comm.c** - Main entry point, game loop, and network handling
- **interpreter.c** - Command parsing and execution
- **db.c** - World database loading and management
- **handler.c** - Core object manipulation functions
- **utils.c** - Utility functions used throughout

### Player Actions
- **act.comm.c** - Communication commands (say, tell, channels)
- **act.informative.c** - Information commands (look, examine, who)
- **act.offensive.c** - Combat commands and actions
- **act.other.c** - Miscellaneous player commands
- **act.wizard.c** - Administrative and immortal commands
- **act.item.c** - Item manipulation commands
- **act.social.c** - Social commands and emotes
- **movement.c** - Movement commands and mechanics

### Game Systems
- **fight.c** - Combat mechanics and damage calculation
- **magic.c / spells.c** - Spell casting and magical effects
- **psionics.c** - Psionic power system
- **class.c / race.c / feats.c** - Character development
- **combat_modes.c** - Combat stance system
- **grapple.c** - Grappling mechanics

### Advanced Features
- **account.c** - Multi-character account management
- **crafting_new.c/h** - Modern crafting system
- **crafting_recipes.c** - Recipe definitions
- **treasure.c** - Loot generation system
- **missions.c / quest.c / hlquest.c** - Quest systems
- **encounters.c / hunts.c** - Dynamic content
- **templates.c** - Character templates
- **trails.c/h** - Player tracking system (disabled in some campaigns)

### Building & Scripting
- **dg_*.c files** - DG Scripts engine
- ***edit.c files** - OLC editors
- **gen*.c files** - Generic creation functions

### Infrastructure
- **mysql.c** - Database integration
- **mud_event.c** - Event system
- **actionqueues.c** - Action queue management
- **protocol.c** - Telnet/MUD protocols (handles GMCP, MSDP, etc.)
- **perfmon.c** - Performance monitoring

### WORLD lib/
Basically everything that makes up the world or that is saved can be found here including:
World, Mobs, Objects, Shops, Zones, Triggers, Quests, Help Files, etc.

### Database Integration
- MariaDB for persistent storage
- Connection managed through mysql.c
- Stores: player data, accounts, mail, houses, clans
- Always check bounds when loading data from database
- Free MySQL results properly to avoid memory leaks

## AI Assistant Guidelines

### Code Analysis Approach

**ONE RULE TO RULE THEM ALL: DO NOT BREAK THE GAME**
**Fully understand the code flow BEFORE making changes.  Trace through several examples to verify your fix is safe before applying.  Add comments in the file that would enable new engineers to understand what is happening in the code quickly.**

When analyzing code, consider:
1. Function purpose and parameters
2. Memory management (allocation/deallocation)
3. Error handling and validation
4. Performance implications
5. Thread safety (single-threaded but event-driven)
6. Integration with existing systems
7. Campaign-specific differences

### Security Considerations

#### Input Validation
- Always validate user input
- Remove control characters
- Limit input length
- Escape special characters
- Check permissions before actions

#### SQL Injection Prevention
```c
// Use mysql_real_escape_string()
char escaped_name[MAX_NAME_LENGTH * 2 + 1];
mysql_real_escape_string(conn, escaped_name, player_name, strlen(player_name));

// Or use prepared statements for complex queries
```

### Performance Best Practices

#### Efficient Loops
```c
// Cache frequently accessed values
struct char_data *next_ch;
for (ch = character_list; ch; ch = next_ch) {
  next_ch = ch->next; // Cache before potential removal
  // ... process character ...
}
```

#### Memory Optimization
- Use object pools for frequently allocated structures
- Minimize string operations in tight loops
- Cache expensive calculations

#### Database Optimization
- Batch database operations when possible
- Use indexes on frequently queried columns
- Limit result sets with appropriate WHERE clauses
- Always free MySQL results

---
