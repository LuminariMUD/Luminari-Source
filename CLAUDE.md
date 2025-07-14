# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview
LuminariMUD is a text-based multiplayer online role-playing game (MUD) server based on tbaMUD/CircleMUD. It implements Pathfinder/D&D 3.5 rules and features an original world inspired by Biblical, Dragonlance, and Forgotten Realms stories.

## Build Commands
```bash
# Build the main game executable
make

# Build everything including utilities
make all

# Clean build artifacts
make clean

# Build and run unit tests
make cutest

# Generate dependencies
make depend

# Build just utilities
make utils
```

## Running and Testing
```bash
# The main binary is output to ../bin/circle
# Run the game server (from the bin directory)
./circle

# Run unit tests
../bin/cutest
```

## Deployment
The project includes deployment scripts for different environments:
```bash
# Deploy to dev environment
./copylumbinarytodev.sh

# Deploy to live environment
./copylumbinarytolive.sh

# Similar scripts exist for Dragonlance and Faerun variants
```

## Architecture Overview
The codebase follows a modular MUD architecture:

- **comm.c** - Main entry point, game loop, and network handling
- **interpreter.c** - Command parsing and execution
- **act.*.c files** - Player commands and actions (movement, combat, etc.)
- **db.c** - World database loading and management
- **handler.c** - Core object manipulation functions
- **fight.c / magic.c / spells.c** - Combat and magic systems
- **class.c / race.c / feats.c** - Character development systems
- **dg_*.c files** - DG Scripts scripting system
- ***edit.c files** - OLC (Online Creation) tools for builders
- **gen*.c files** - Generic object/mob/room creation functions

## Important Development Notes

### Missing Files
- **campaign.h** - Must be created locally. This file contains campaign-specific customizations.

### Code Style
- Follow existing indentation and brace style in each file
- Use the existing utility functions and macros defined in utils.h
- When adding new commands, register them in interpreter.c
- Use the existing logging macros (log(), mudlog()) for debugging

### Code Formatting
The project includes a `.clang-format` configuration file for consistent code formatting:
```bash
# Format a single file
clang-format -i filename.c

# Format all C files in current directory
clang-format -i *.c

# Format all C and header files recursively
find . -name "*.c" -o -name "*.h" | xargs clang-format -i

# Check formatting without modifying files
clang-format -n filename.c

# Format only modified lines (requires git)
git diff -U0 --no-color | clang-format-diff -i
```

### Key Systems to Understand
1. **DG Scripts** - The scripting system used for dynamic content
2. **OLC (Online Creation)** - The in-game building tools
3. **Event System** - mud_event.c handles timed events
4. **Action Queues** - actionqueues.c manages delayed actions
5. **Protocol Handling** - protocol.c manages telnet and MUD-specific protocols

### Database Integration
- MySQL support is integrated for persistent storage
- Connection handled through mysql.c
- Used for player data, mail, and other persistent information

### Recent Development Focus
- New crafting system (crafting_new.c)
- Epic feats and high-level progression
- Enhanced spell system with bonus slots and potency
- Background system implementation
- Role-playing tools and features