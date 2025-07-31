# CLAUDE.md - AI Assistant Development Guide

This file provides comprehensive guidance for AI assistants (Claude, GPT, etc.) when working with the LuminariMUD codebase. It contains essential context, patterns, and best practices for effective code assistance.

## Quick Start for Common Tasks

### Adding a New Command
1. Implement function in appropriate `act.*.c` file using `ACMD()` macro
2. Register in `interpreter.c` cmd_info[] table
3. Add help entry via `hedit` or help files
4. Test with various permission levels and edge cases

### Modifying Game Mechanics
1. Check dependencies in related systems
2. Maintain game balance considerations
3. Update help documentation
4. Test all affected systems
5. Ensure backwards compatibility

### Performance Profiling
1. Use `perfmon.cpp` for C++ profiling
2. Check `gprof` output for C functions
3. Monitor with `valgrind` for memory issues
4. Watch CPU usage during high-activity periods

## Project Overview

LuminariMUD is a sophisticated text-based multiplayer online role-playing game (MUD) server implementing authentic Pathfinder/D&D 3.5 mechanics. Built on the proven tbaMUD/CircleMUD foundation, it features:

- **Authentic D&D 3.5 Rules / D20 / and Pathfinder-Inspired**: Complete implementation of classes, races, feats, skills, and spells
- **Advanced Combat System**: Initiative-based combat with tactical positioning and combat modes
- **Dynamic Scripting**: DG Scripts for interactive content and complex behaviors
- **Online Building Tools**: Comprehensive OLC system for world creation
- **MySQL Integration**: Persistent player data, accounts, and world state
- **Performance Optimized**: Handles hundreds of concurrent players efficiently
- **Advanced Systems**: Crafting, psionics, templates, missions, hunts, encounters

### Current Version
- **LuminariMUD** (built on tbaMUD/CircleMUD/CWG [Circle with Goodies]/d20MUD [Stephen Squires])
- **Language**: C (ANSI C90/C89 - NOT C99!) with C++ performance monitoring
  - **IMPORTANT**: The codebase does NOT compile with C99 mode enabled
  - **DO NOT** use C99 features like:
    - Variable declarations inside for loops: `for (int i = 0; ...)`
    - Mixed declarations and code
    - Variable length arrays
  - **ALWAYS** declare variables at the beginning of blocks
- **Platform**: Linux/Unix systems (CentOS), WSL/Windows compatible
- **Database**: MySQL 5.0+ or MariaDB
- **Compiler**: GCC/Clang (without -std=c99 flag)

### Recent Development Focus (2025)

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

# Build with profiling support
make PROFILE=-pg

# Build C++ performance monitor tests (when enabled)
make -f test_perfmon.mk
```

## Running and Testing
```bash
# The main binary is output to ../bin/circle
# Run the game server (from the bin directory)
./circle

# Run unit tests
../bin/cutest

# Run with valgrind for memory debugging
valgrind --leak-check=full ../bin/circle

# Generate profiling data
gprof ../bin/circle gmon.out > profile.txt
```

## Deployment
The project includes deployment scripts for different environments:
```bash
# Deploy to dev environment
./copydlbinarytodev.sh    # Dragonlance variant
./copyfrbinarytodev.sh    # Faerun variant
./cpbin2dev.sh           # LuminariMUD (default) deployment

# Deploy to live environment  
./copydlbinarytolive.sh   # Dragonlance variant
./copyfrbinarytolive.sh   # Faerun variant
./cpbin2live.sh          # LuminariMUD (default)  deployment
```

## Architecture Overview
The codebase follows a modular MUD architecture:

### Core Engine
- **comm.c** - Main entry point, game loop, and network handling
- **interpreter.c** - Command parsing and execution
- **db.c** - World database loading and management
- **handler.c** - Core object manipulation functions
- **utils.c** - Utility functions used throughout

### Player Actions
- **act.comm.c** - Communication commands (say, tell, channels)
- **act.informative.c** - Information commands (look, examine, who)
- **act.movement.c** - Movement commands and mechanics
- **act.offensive.c** - Combat commands and actions
- **act.other.c** - Miscellaneous player commands
- **act.wizard.c** - Administrative and immortal commands
- **act.item.c** - Item manipulation commands
- **act.social.c** - Social commands and emotes

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
- **trails.c/h** - Player tracking system

### Building & Scripting
- **dg_*.c files** - DG Scripts engine
- ***edit.c files** - OLC editors
- **gen*.c files** - Generic creation functions

### Infrastructure
- **mysql.c** - Database integration
- **mud_event.c** - Event system
- **actionqueues.c** - Action queue management
- **protocol.c** - Telnet/MUD protocols
- **perfmon.cpp** - Performance monitoring (C++)

## Important Development Notes

### Configuration Files (Not in Git)
These files must be created from examples and are `.gitignore`d:
- **campaign.h** - Copy from `campaign.example.h` for campaign settings (CAMPAIGN_DL for Dragonlance, CAMPAIGN_FR for Forgotten Realms)
- **mud_options.h** - Copy from `mud_options.example.h` for MUD-specific options
- **vnums.h** - Copy from `vnums.example.h` for zone virtual number assignments

### Code Style
- **C90/C89 COMPLIANCE REQUIRED**: 
  - Declare ALL variables at the beginning of blocks
  - NO variable declarations in for loops
  - NO mixed declarations and code
  - NO C99/C11 features
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
1. **DG Scripts** - Dynamic content scripting (triggers, variables, commands)
2. **OLC (Online Creation)** - In-game building tools (redit, oedit, medit, zedit)
3. **Event System** - Timed events (mud_event.c)
4. **Action Queues** - Delayed actions (actionqueues.c)
5. **Protocol Handling** - Telnet/MUD protocols (protocol.c)
6. **Account System** - Multi-character accounts (account.c)
7. **Crafting System** - Recipe-based crafting (crafting_new.c)
8. **Performance Monitor** - C++ profiling system (perfmon.cpp)

### Database Integration
- MySQL/MariaDB for persistent storage
- Connection managed through mysql.c
- Stores: player data, accounts, mail, houses, clans
- Recent fixes: Added missing 'idnum' columns

## AI Assistant Guidelines

### Understanding the Codebase
1. **Read Documentation First**: Key documentation files:
   - [TECHNICAL_DOCUMENTATION_MASTER_INDEX.md](docs/TECHNICAL_DOCUMENTATION_MASTER_INDEX.md) - Complete index
   - [ARCHITECTURE.md](docs/ARCHITECTURE.md) - System design
   - [DEVELOPER_GUIDE_AND_API.md](docs/DEVELOPER_GUIDE_AND_API.md) - API reference
   - [TESTING_GUIDE.md](docs/TESTING_GUIDE.md) - Testing practices
2. **Follow Existing Patterns**: Study similar functions before implementing
3. **Respect Architecture**: Maintain modular design and separation of concerns
4. **Consider Performance**: Real-time system with strict performance requirements
5. **Check Recent Changes**: Review CHANGELOG.md for latest patterns

### Code Analysis Approach
```c
// When analyzing code, consider these aspects:
// 1. Function purpose and parameters
// 2. Memory management (allocation/deallocation)
// 3. Error handling and validation
// 4. Performance implications
// 5. Thread safety (single-threaded but event-driven)
// 6. Integration with existing systems
```

### Common Patterns to Recognize

#### Character Data Access
```c
// Always validate character pointers
if (!ch) {
  log("SYSERR: %s called with NULL character", __func__);
  return;
}

// Use macros for character properties
GET_NAME(ch)     // Character name
GET_LEVEL(ch)    // Character level
GET_CLASS(ch)    // Character class
IS_NPC(ch)       // Check if NPC
```

#### Memory Management
```c
// Use CREATE macro for allocation
CREATE(new_obj, struct obj_data, 1);

// Always free allocated memory
if (obj->name) {
  free(obj->name);
  obj->name = NULL;
}

// Use appropriate cleanup functions
free_char(ch);    // For characters
extract_obj(obj); // For objects
```

#### Error Handling
```c
// Use consistent logging
log("INFO: Normal operation message");
log("SYSERR: System error occurred");
mudlog(BRF, LVL_IMMORT, TRUE, "Admin message: %s", message);

// Validate input parameters
if (!argument || !*argument) {
  send_to_char(ch, "Usage: command <required_argument>\r\n");
  return;
}
```

### Development Workflow for AI Assistants

#### 1. Information Gathering
- **Examine Related Code**: Look at similar functions and systems
- **Check Documentation**: Consult relevant technical documentation
- **Understand Context**: Consider how changes affect other systems
- **Review Recent Changes**: Check CHANGELOG.md for recent modifications

#### 2. Code Implementation
- **Follow Coding Standards**: Use existing style and conventions
- **Add Proper Documentation**: Include function headers and comments
- **Implement Error Handling**: Validate inputs and handle edge cases
- **Consider Performance**: Optimize for the real-time environment

#### 3. Testing Considerations
- **Unit Tests**: Add tests for new functionality when possible
- **Integration Testing**: Consider how changes affect existing systems
- **Memory Testing**: Check for leaks with valgrind
- **Performance Testing**: Ensure changes don't degrade performance

### Key Files and Their Purposes

#### Core Engine Files
- **`comm.c`** - Main game loop, networking, player connections
- **`interpreter.c`** - Command parsing and execution
- **`db.c`** - World data loading, database operations
- **`handler.c`** - Core object/character manipulation
- **`utils.c`** - Utility functions used throughout codebase

#### Game Systems
- **`fight.c`** - Combat mechanics and damage calculation
- **`magic.c`** - Spell casting and magical effects
- **`spells.c`** - Individual spell implementations
- **`class.c`** - Character class definitions and abilities
- **`race.c`** - Character race definitions and bonuses
- **`feats.c`** - Feat system implementation

#### Player Actions
- **`act.comm.c`** - Communication commands (say, tell, channels)
- **`act.informative.c`** - Information commands (look, examine, who)
- **`act.movement.c`** - Movement commands and mechanics
- **`act.offensive.c`** - Combat commands and actions
- **`act.other.c`** - Miscellaneous player commands
- **`act.wizard.c`** - Administrative and immortal commands

#### Building and Scripting
- **`dg_scripts.c`** - DG scripting engine
- **`dg_triggers.c`** - Script trigger system
- **`*edit.c`** - OLC (Online Level Creation) editors
- **`genolc.c`** - Generic OLC functions

#### Specialized Systems
- **`mysql.c`** - Database integration
- **`mud_event.c`** - Event system for timed actions
- **`actionqueues.c`** - Action queue management
- **`protocol.c`** - Telnet and MUD protocol handling

### Common Debugging Scenarios

#### Memory Issues
```c
// Check for NULL pointers before use
if (!ptr) {
  log("SYSERR: Unexpected NULL pointer in %s", __func__);
  return ERROR_CODE;
}

// Use valgrind for memory leak detection
// valgrind --leak-check=full ../bin/circle
```

#### Performance Problems
```c
// Profile with gprof
// make PROFILE=-pg
// gprof ../bin/circle gmon.out > profile.txt

// Log performance-critical sections
struct timeval start, end;
gettimeofday(&start, NULL);
// ... code to profile ...
gettimeofday(&end, NULL);
long diff = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
if (diff > 1000) { // Log if > 1ms
  log("PERFORMANCE: %s took %ld microseconds", __func__, diff);
}
```

#### Database Issues
```c
// Always check MySQL connection
mysql_ping(conn);

// Use proper error handling
if (mysql_query(conn, query)) {
  log("SYSERR: MySQL query failed: %s", mysql_error(conn));
  return FALSE;
}

// Free result sets
mysql_free_result(result);
```

### Integration Points to Consider

#### When Adding New Commands
1. **Register in `interpreter.c`**: Add to cmd_info table
2. **Implement Function**: Follow ACMD() pattern
3. **Add Help Entry**: Update help database
4. **Consider Permissions**: Check level requirements
5. **Test Edge Cases**: Invalid arguments, offline targets, etc.

#### When Modifying Game Mechanics
1. **Check Dependencies**: What other systems are affected?
2. **Maintain Balance**: Consider gameplay impact
3. **Update Documentation**: Modify relevant help files
4. **Test Thoroughly**: Verify all affected systems work
5. **Consider Backwards Compatibility**: Don't break existing content

#### When Working with Data Structures
1. **Understand Relationships**: How structures connect
2. **Maintain Integrity**: Ensure all references remain valid
3. **Handle Memory Properly**: Allocate and free correctly
4. **Consider Persistence**: How changes affect saved data
5. **Test Edge Cases**: Empty lists, maximum values, etc.

### Security Considerations

#### Input Validation
```c
// Always validate user input
void sanitize_input(char *input) {
  // Remove control characters
  // Limit length
  // Escape special characters
}

// Check permissions before actions
if (GET_LEVEL(ch) < LVL_IMMORT) {
  send_to_char(ch, "You don't have permission for that.\r\n");
  return;
}
```

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
```c
// Use object pools for frequently allocated structures
// Minimize string operations in tight loops
// Cache expensive calculations
```

#### Database Optimization
```c
// Batch database operations when possible
// Use indexes on frequently queried columns
// Limit result sets with appropriate WHERE clauses
```

### Testing and Quality Assurance

#### Unit Testing
```c
// Use CuTest framework for unit tests
void test_combat_damage_calculation(CuTest *tc) {
  struct char_data *attacker = create_test_character();
  struct char_data *victim = create_test_character();

  int damage = calculate_damage(attacker, victim, NULL);

  CuAssertTrue(tc, damage >= 1); // Minimum damage
  CuAssertTrue(tc, damage <= 100); // Maximum reasonable damage

  cleanup_test_character(attacker);
  cleanup_test_character(victim);
}
```

#### Integration Testing
- Test with existing world data
- Verify compatibility with saved player files
- Check interaction with other systems
- Test under load conditions

### Documentation Standards

#### Function Documentation
```c
/**
 * Calculate initiative for combat ordering
 *
 * @param ch The character rolling initiative
 * @return Initiative value (1d20 + modifiers)
 */
int calculate_initiative(struct char_data *ch) {
  // Implementation...
}
```

#### Code Comments
```c
// Single line for simple explanations
int damage = roll_damage(weapon);

/* Multi-line for complex logic
 * This section handles the interaction between
 * multiple combat modifiers and special cases
 */
```

### Useful Macros and Functions

#### Character Macros
```c
GET_NAME(ch)          // Character name
GET_LEVEL(ch)         // Character level
GET_CLASS(ch)         // Character class
GET_RACE(ch)          // Character race
GET_STR(ch)           // Strength score
GET_HIT(ch)           // Current hit points
GET_MAX_HIT(ch)       // Maximum hit points
IS_NPC(ch)            // Is character an NPC?
IN_ROOM(ch)           // Character's current room
```

#### Object Macros
```c
GET_OBJ_TYPE(obj)     // Object type
GET_OBJ_WEIGHT(obj)   // Object weight
GET_OBJ_COST(obj)     // Object cost
GET_OBJ_VAL(obj, n)   // Object value n
```

#### Utility Functions
```c
one_argument(input, output)     // Parse one argument
two_arguments(input, arg1, arg2) // Parse two arguments
skip_spaces(&ptr)               // Skip whitespace
str_cmp(str1, str2)            // Case-insensitive compare
is_abbrev(abbr, full)          // Check abbreviation
```

### Common Pitfalls and Critical Patterns

#### NPC vs Player Data Access (CRITICAL)
```c
// ALWAYS check IS_NPC() before accessing player-only data
if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOEXIT)) {
  // Safe to access PRF flags
}

// NEVER do this:
// if (PRF_FLAGGED(ch, PRF_SOMETHING)) // Can crash with NPCs!
```

#### Memory Management Patterns
```c
// String allocation pattern
char *new_str = strdup(old_str);
if (target->str) free(target->str);
target->str = new_str;

// Object allocation pattern
CREATE(obj, struct obj_data, 1);
clear_object(obj);
// ... initialize object ...
```

#### Performance Critical Sections
```c
// Cache frequently accessed values
struct room_data *room = &world[IN_ROOM(ch)];
struct char_data *next_ch;
for (vict = room->people; vict; vict = next_ch) {
  next_ch = vict->next_in_room; // Cache before potential removal
  // Process character
}
```

### Complete Command Implementation Example

#### 1. In act.other.c (or appropriate file):
```c
ACMD(do_meditate) {
  struct char_data *vict;
  int heal_amount;
  
  // Input validation
  if (IS_NPC(ch)) {
    send_to_char(ch, "NPCs cannot meditate.\r\n");
    return;
  }
  
  // Check conditions
  if (FIGHTING(ch)) {
    send_to_char(ch, "You can't meditate while fighting!\r\n");
    return;
  }
  
  // Implementation
  heal_amount = dice(1, 8) + GET_LEVEL(ch);
  GET_HIT(ch) = MIN(GET_HIT(ch) + heal_amount, GET_MAX_HIT(ch));
  
  // Messages
  send_to_char(ch, "You meditate peacefully and recover %d hit points.\r\n", heal_amount);
  act("$n meditates peacefully.", TRUE, ch, 0, 0, TO_ROOM);
}
```

#### 2. In interpreter.c:
```c
// In cmd_info[] array, maintaining alphabetical order:
{ "meditate", "med", POS_RESTING, do_meditate, 0, 0, 0, 0 },
```

#### 3. Help Entry (via hedit or help files):
```
MEDITATE

Usage: meditate

This command allows you to enter a meditative state and recover
hit points through peaceful contemplation. You must be resting
and not in combat to meditate.

Healing: 1d8 + character level hit points

See also: REST, SLEEP, HEAL
```

### Testing Checklist for New Code
- [ ] Check IS_NPC() before accessing player data
- [ ] Validate all pointers before use
- [ ] Free allocated memory appropriately
- [ ] Test with both PC and NPC characters
- [ ] Verify no memory leaks with valgrind
- [ ] Check performance impact with perfmon
- [ ] Test edge cases (NULL inputs, max values)
- [ ] Ensure backwards compatibility
- [ ] Update relevant documentation

### Recent Changes and Patterns (2025)

#### Performance Optimizations
- Zone reset: O(n²) to O(n) algorithm improvement
- Mobile activity: Caching and early exit optimization
- NPC casting: Direct function calls vs command parsing
- NPC buffing: Reduced frequency with saturation checks

#### Critical Bug Patterns Fixed
- NPCs accessing PRF_FLAGGED player data
- Missing database columns (idnum)
- Memory leaks in string operations
- Buffer overflows in user input

#### Code Quality Standards
- Always use IS_NPC() checks
- Cache array lookups in loops
- Validate user input lengths
- Use proper error logging
- Follow existing code style

---

*This guide is updated regularly to reflect current development practices and codebase changes. Always refer to the latest version when working with the code.*

## Additional Resources

### Documentation Files
- **Setup**: [SETUP_AND_BUILD_GUIDE.md](docs/SETUP_AND_BUILD_GUIDE.md)
- **Architecture**: [ARCHITECTURE.md](docs/ARCHITECTURE.md), [CORE_SERVER_ARCHITECTURE.md](docs/CORE_SERVER_ARCHITECTURE.md)
- **Systems**: [GAME_MECHANICS_SYSTEMS.md](docs/GAME_MECHANICS_SYSTEMS.md), [COMBAT_SYSTEM.md](docs/COMBAT_SYSTEM.md)
- **Scripting**: [SCRIPTING_SYSTEM_DG.md](docs/SCRIPTING_SYSTEM_DG.md)
- **OLC**: [OLC_ONLINE_CREATION_SYSTEM.md](docs/OLC_ONLINE_CREATION_SYSTEM.md)
- **Database**: [DATABASE_INTEGRATION.md](docs/DATABASE_INTEGRATION.md)
- **Testing**: [TESTING_GUIDE.md](docs/TESTING_GUIDE.md)
- **Troubleshooting**: [TROUBLESHOOTING_AND_MAINTENANCE.md](docs/TROUBLESHOOTING_AND_MAINTENANCE.md)

### Support Files
- **CHANGELOG**: [docs/CHANGELOG.md](docs/CHANGELOG.md) - Recent changes
- **TODO**: [TASK_LIST.md](TASK_LIST.md) - Development roadmap
- **Errors**: [SYSLOG_ERRORS.md](SYSLOG_ERRORS.md) - Common error patterns