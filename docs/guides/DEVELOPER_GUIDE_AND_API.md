# LuminariMUD Developer Guide and API Reference

## Quick Start for Developers

Get a development environment running in minutes:

```bash
# Clone and setup with development build
git clone https://github.com/LuminariMUD/Luminari-Source.git
cd Luminari-Source
./scripts/deployment/deploy.sh --auto --dev --init-world

# Start with debugging support
./scripts/debugging/debug_game.sh
```

The `--dev` flag enables:
- Debug symbols for GDB
- Verbose logging
- Development utilities
- Performance monitoring

## Overview

This guide provides comprehensive information for developers working on LuminariMUD, including coding standards, architecture patterns, API references, and contribution workflows. It covers both core engine development and content creation.

## Special-Procedure Control Plane

### Definition Registry

Special procedures registered for world data and OLC are defined in
`src/spec/spec_registry.c`. Public metadata and accessors are declared in
`src/spec/spec_registry.h`. Assignment code remains in `src/spec_assign.c`, and runtime invocation
continues to use the legacy `SPECIAL` callback ABI:

```c
int handler(struct char_data *ch, void *me, int cmd, const char *argument);
```

### Definition Contract

Each immutable `struct spec_definition` declares:

- a stable `canonical_name` used for persistence and reverse lookup;
- a separate builder-facing `display_name` and explicit aliases;
- compatible mobile, object, or room owner bits;
- one or more event contracts, including prototype and placement prerequisites;
- permitted binding sources and explicit builder visibility;
- a non-empty category and description; and
- exactly one legacy or typed handler.

The current event catalog covers command dispatch, mobile activity, mobile combat turns, object
auto-pulses, item identification, weapon hits, defense reactions, combat maneuvers, mounted charge,
and moving-room relocation. Shop and quest secondary callbacks preserve an incoming command context
and therefore remain compatibility composition around these definitions rather than separate event
values.

`spec_registry_boot_validate()` runs in `boot_db()` before `boot_world()`. Invalid programmer
metadata terminates startup with a `SYSERR` diagnostic before MySQL world initialization or any
world file is parsed. Validation rejects missing required text, case-insensitive name or alias
collisions, invalid masks, incompatible owner/event combinations, missing event prerequisites,
duplicate events, invalid visibility, and handlerless or dual-handler definitions.

### Lookup APIs

Use the canonical API for new engine code:

```c
const struct spec_definition *definition;

definition = spec_registry_find_for_owner("Bank", SPEC_OWNER_OBJECT);
if (definition != NULL &&
    spec_definition_supports_event(definition, SPEC_OWNER_OBJECT, SPEC_EVENT_COMMAND))
{
  /* The name, owner, and event contract are compatible. */
}
```

- `spec_registry_count()` and `spec_registry_get()` iterate canonical definitions only.
- `spec_registry_find_by_name()` resolves canonical names and aliases case-insensitively.
- `spec_registry_find_for_owner()` also requires exactly one compatible owner type.
- `spec_registry_find_by_handler()` returns the first canonical identity for a legacy callback.
- `spec_definition_get_event()` exposes the prerequisites for one supported event.
- `spec_definition_allows_binding()` checks one binding source at a time.

All indexed and bitmask accessors reject invalid or multi-bit query values. Returned definitions,
event contracts, aliases, and strings are process-lifetime immutable data and must not be freed or
modified.

### Compatibility Surface

The established functions in `spec_procs.h` remain available to world loaders and the current OLC
editors. They expose the historical 29-name order even though the canonical registry contains 28
definitions. `Guild` is the canonical identity, `Guildmaster` is its explicit alias, both names
resolve to `guild`, and handler reverse lookup always returns `Guild`.

When adding a registered procedure, add its handler declaration, immutable definition metadata, and
event contracts in `src/spec/spec_registry.c`; update the compatibility projection only when a new
persisted/OLC name is required. Add production-linked validation coverage and update both
`Makefile.am` and `CMakeLists.txt` if any source file is added or removed. Do not rename a canonical
identity without a separately tested content migration.

### Authored Binding API

`src/spec/spec_binding.h` defines the owned world-authoring record. A `struct spec_binding` retains
the owner type, prototype VNUM, exact requested text, resolved immutable definition when available,
one source bit, source location, and resolution status. Its four possible results are resolved,
unknown name, incompatible owner, and incompatible binding source.

Use `spec_binding_replace()` to resolve and transactionally replace a record. Unknown and
incompatible content is a valid owned result rather than an allocation failure. The callback
returned by `spec_binding_legacy_handler()` is non-null only for a fully resolved record.
`spec_binding_copy()` and `spec_binding_free()` own the prototype and OLC-copy lifetimes.

World writers call `spec_binding_persisted_name()` first. This accessor returns the exact loaded
name only for a valid world-authored record, so aliases, unknown names, and incompatible names
survive an unrelated OLC save. Reverse handler lookup is a compatibility fallback only when no
authored record exists. Explicit OLC selection replaces the record with the canonical definition
name; explicit clear frees it and removes the persisted field.

Never infer authored intent from the current callback pointer. A hard-coded assignment, shop, or
quest may replace or wrap that pointer after world loading without changing what the builder wrote.

### Effective Binding API

`src/spec/spec_effective_binding.h` defines a separate owned observation of boot-time callback
writes. `spec_effective_binding_contribute()` appends validated source, requested and installed
identities, location, handler, outcome, and optional saved secondary state. The record tracks the
current effective contribution and collision count, but it never dispatches a callback and is never
serialized as world content.

The five contribution sources are:

1. `world` for mobile `SpecProc`, object `Z`, and room `Z` authored fields;
2. `parser-hook` for moving-room `M` callback ownership;
3. `legacy-assignment` for hard-coded mobile, object, room, death-trap, and castle writes;
4. `shop` for `shop_keeper` and its actual saved `SHOP_FUNC`; and
5. `quest` for `questmaster` and its actual saved `QST_FUNC`.

Outcomes are selected, unresolved, overridden, reasserted, or wrapped. Contribution and final
formatters produce bounded `SPEC_BIND` and `SPEC_BIND_FINAL` lines; `boot_db()` surrounds them with
`SPEC_BIND_SUMMARY`. A provenance allocation or formatting failure is observable but cannot
suppress an established callback assignment.

With specials enabled, the preserved write order is world/parser loading, mobile assignments, shop
wrappers, object assignments, room assignments, and quest wrappers. With `-s`, world and parser
records still load while the guarded assignment block does not run. Reporting is outside that
guard. This is path-specific compatibility behavior, not a new global dispatch gate.

### OLC And Persistence Integration

`src/olc/spec_menu.c` exposes an owner-filtered, one-based view of builder-visible world-bindable
definitions. The menu renders description, event, prototype-flag, and placement metadata. Invalid
or out-of-range input leaves state unchanged, and selection does not mutate `MOB_SPEC`,
`ITEM_AUTOPROC`, equipment, carried state, or combat state.

Mobile, object, and room loaders attach authored and effective records to their prototype index or
room. Prototype deletion, database shutdown, OLC scratch cleanup, room insertion, and room copying
must use the matching copy/free APIs. Do not shallow-copy either owned record.

A moving room and a named room definition cannot share `room_data.func`: the moving-room caller
passes `struct moving_room_data *` through the legacy `me` parameter. The production parser rejects
`M` plus `Z` in either order, REdit blocks selection and internal save, and `save_rooms()` preflights
the entire zone before opening output or mutating mover state.

### Adding Or Changing A Registered Procedure

1. Characterize every applicable invocation path and exact legacy arguments before changing
   behavior.
2. Add or update the immutable definition and event contracts in `src/spec/spec_registry.c`.
3. Preserve a stable canonical persisted name; add an explicit alias for compatible historical
   input instead of another canonical row.
4. Set owner, binding-source, visibility, flag, and placement metadata from traced callers. OLC does
   not establish runtime prerequisites automatically.
5. Add production-linked registry, owner, OLC, persistence, and invocation coverage as applicable.
6. When a source or CuTest file is added or removed, update both `Makefile.am` and `CMakeLists.txt`.
7. Update builder help and documentation. Database-first help changes belong in a checked-in
   migration and read-only verifier under `sql/components/`.

### Phase 00 Boundary

The delivered control plane preserves the single callback slot, `SPECIAL` ABI, world-file grammar,
command traversal, heartbeat timing, caller-specific return interpretation, activation flags, shop
and quest nesting, and boot precedence. Event gateways, typed contexts and invalidation, declarative
assignments, content extraction, shared mechanics, typed-handler conversion, and general handler
chains remain future work. Current code must not depend on those proposed interfaces.

See [OLC SpecProc Editing](OLC_SpecProcs.md) for the builder workflow and
[Phase 00 Validation](../testing/SPECIAL_PROCEDURE_PHASE_00_VALIDATION.md) for the exact evidence
matrix.

## Development Environment Setup

### Automated Setup (Recommended)

Use the deployment script with development options:

```bash
# Full development environment setup (REQUIRED - includes database)
./scripts/deployment/deploy.sh --auto --dev --init-world

Running this command builds the game, provisions MariaDB (REQUIRED), and invokes the in-engine database initializer so every subsystem (wilderness resources, PubSub, vessels, etc.) is ready without manual SQL. The generated credentials are written to `lib/mysql_config`.

**Note**: Database is REQUIRED for LuminariMUD to function. Do not use `--skip-db` unless you plan to configure the database manually.
```

### Required Tools
- **GCC 13+** or **Clang 18+** - C compiler with GNU C23 support; GCC's legacy
  `gnu2x` flag spelling is accepted only after the C23 feature probe passes
- **Git** - Version control system
- **GDB** - Debugger for troubleshooting
- **Valgrind** - Memory leak detection (Linux) - Essential for development
- **Doxygen** - Documentation generation (optional)
- **Text Editor/IDE** - VS Code, Vim, Emacs, or CLion
- **Performance Tools** - Built-in performance monitoring available

### Development Dependencies
```bash
# Ubuntu/Debian (including WSL2)
sudo apt-get update
sudo apt-get install -y build-essential gdb valgrind doxygen graphviz \
                        cppcheck clang-format git-flow \
                        libcrypt-dev libgd-dev libmariadb-dev \
                        libcurl4-openssl-dev libssl-dev mariadb-server \
                        git make cmake autoconf automake libtool pkg-config

# CentOS/RHEL/Fedora
sudo dnf install gcc gdb valgrind doxygen graphviz cppcheck \
                 clang-tools-extra git-flow \
                 mariadb-server mariadb-devel gd-devel \
                 libcrypt-devel openssl-devel libcurl-devel
```

### IDE Configuration

#### Visual Studio Code
```json
// .vscode/c_cpp_properties.json
{
    "configurations": [
        {
            "name": "Linux",
            "includePath": [
                "${workspaceFolder}/**",
                "/usr/include/mysql",
                "/usr/include/gd"
            ],
            "defines": [
                "CIRCLE_UNIX",
                "CIRCLE_MYSQL"
            ],
            "compilerPath": "/usr/bin/gcc",
            "cStandard": "c23",
            "intelliSenseMode": "linux-gcc-x64"
        }
    ]
}
```

## Coding Standards

### Code Style Guidelines

#### Naming Conventions
```c
// Functions: lowercase with underscores
void perform_action(struct char_data *ch);
int calculate_damage(int base, int modifier);

// Variables: lowercase with underscores
int player_count;
struct char_data *current_character;

// Constants: uppercase with underscores
#define MAX_PLAYERS 300
#define DEFAULT_PORT 4000

// Macros: uppercase with underscores
#define GET_NAME(ch) ((ch)->player.name)
#define IS_NPC(ch) (IS_SET(MOB_FLAGS(ch), MOB_ISNPC))

// Structures: lowercase with underscores
struct char_data {
    // members...
};

// Enumerations: uppercase with underscores
enum position_types {
    POS_DEAD,
    POS_MORTALLYW,
    POS_INCAP
};
```

#### Indentation and Formatting
```c
// Use 2 spaces for indentation (no tabs)
if (condition) {
  if (nested_condition) {
    perform_action();
  }
}

// Brace placement: opening brace on same line
void function_name(int parameter) {
  // function body
}

// Long parameter lists: align parameters
int complex_function(struct char_data *ch,
                     struct obj_data *obj,
                     int flags,
                     const char *argument) {
  // function body
}
```

#### Comments and Documentation
```c
/**
 * Calculate combat damage based on weapon and character stats
 *
 * @param ch The attacking character
 * @param weapon The weapon being used (NULL for unarmed)
 * @param target The target being attacked
 * @return Total damage dealt (0 if miss)
 */
int calculate_combat_damage(struct char_data *ch,
                           struct obj_data *weapon,
                           struct char_data *target) {
  // Implementation details...

  /* Multi-line comment for complex logic */
  if (weapon && GET_OBJ_TYPE(weapon) == ITEM_WEAPON) {
    // Single line comment for simple explanations
    base_damage = dice(GET_OBJ_VAL(weapon, 1), GET_OBJ_VAL(weapon, 2));
  }

  return total_damage;
}
```

### Error Handling Patterns

#### Return Value Conventions
```c
// Boolean functions: return TRUE/FALSE
bool validate_input(const char *input) {
  if (!input || !*input) {
    return FALSE;
  }
  return TRUE;
}

// Pointer functions: return NULL on failure
struct char_data *find_character(const char *name) {
  if (!name || !*name) {
    return NULL;
  }
  // search logic...
  return found_character;
}

// Integer functions: return negative on error
int perform_calculation(int value) {
  if (value < 0) {
    return -1; // Error code
  }
  return result;
}
```

#### Error Logging
```c
// Use appropriate log levels
log("INFO: Player %s connected", GET_NAME(ch));
log("SYSERR: Failed to load room %d", room_vnum);
mudlog(BRF, LVL_IMMORT, TRUE, "DEATH: %s killed by %s",
       GET_NAME(victim), GET_NAME(killer));

// Include context in error messages
if (!ch) {
  log("SYSERR: %s called with NULL character", __func__);
  return;
}
```

## Core Architecture Patterns

### Memory Management

#### Character Creation and Cleanup
```c
// Always use create_char() for new characters
struct char_data *create_new_player(const char *name) {
  struct char_data *ch = create_char();
  if (!ch) {
    log("SYSERR: Failed to create character structure");
    return NULL;
  }

  // Initialize character data
  ch->player.name = strdup(name);
  ch->player_specials = create_player_specials();

  return ch;
}

// Always use free_char() for cleanup
void cleanup_character(struct char_data *ch) {
  if (!ch) return;

  // Remove from lists first
  char_from_room(ch);

  // Clean up allocated memory
  if (ch->player.name) {
    free(ch->player.name);
  }

  // Use the standard cleanup function
  free_char(ch);
}
```

#### Object Management
```c
// Create objects with proper initialization
struct obj_data *create_game_object(obj_vnum vnum) {
  struct obj_data *obj = read_object(vnum, VIRTUAL);
  if (!obj) {
    log("SYSERR: Failed to create object %d", vnum);
    return NULL;
  }

  // Set unique ID for scripting
  GET_ID(obj) = max_obj_id++;

  return obj;
}

// Proper object cleanup
void destroy_game_object(struct obj_data *obj) {
  if (!obj) return;

  // Remove from containers/rooms/characters
  obj_from_char(obj);
  obj_from_room(obj);
  obj_from_obj(obj);

  // Extract and free
  extract_obj(obj);
}
```

### Event System Integration

#### Creating Events
```c
// Define event function
EVENTFUNC(combat_round_event) {
  struct char_data *ch = (struct char_data *)event_obj;

  if (!ch || !FIGHTING(ch)) {
    return 0; // Event ends
  }

  // Perform combat round
  perform_combat_round(ch);

  // Return pulse delay for next event (or 0 to end)
  return PULSE_COMBAT;
}

// Attach event to character
void start_combat_events(struct char_data *ch) {
  if (char_has_mud_event(ch, eCOMBAT_ROUND)) {
    return; // Already has combat event
  }

  attach_mud_event(new_mud_event(eCOMBAT_ROUND, ch, PULSE_COMBAT), ch);
}
```

#### Event Cleanup
```c
void stop_character_events(struct char_data *ch) {
  // Remove specific events
  if (char_has_mud_event(ch, eCOMBAT_ROUND)) {
    remove_mud_event(ch, eCOMBAT_ROUND);
  }

  // Or remove all events for character
  clear_char_event_list(ch);
}
```

## API Reference

### Core Character Functions

#### Character Information
```c
// Basic character properties
char *GET_NAME(struct char_data *ch);
int GET_LEVEL(struct char_data *ch);
int GET_CLASS(struct char_data *ch);
int GET_RACE(struct char_data *ch);

// Character state
bool IS_NPC(struct char_data *ch);
bool IS_PLAYING(struct char_data *ch);
int GET_POS(struct char_data *ch);

// Character location
room_rnum IN_ROOM(struct char_data *ch);
struct room_data *GET_ROOM(struct char_data *ch);
```

#### Character Manipulation
```c
// Movement and positioning
void char_from_room(struct char_data *ch);
void char_to_room(struct char_data *ch, room_rnum room);
int perform_move(struct char_data *ch, int dir, int need_specials_check);

// Equipment management
void equip_char(struct char_data *ch, struct obj_data *obj, int pos);
struct obj_data *unequip_char(struct char_data *ch, int pos);
struct obj_data *GET_EQ(struct char_data *ch, int pos);

// Inventory management
void obj_to_char(struct obj_data *obj, struct char_data *ch);
void obj_from_char(struct obj_data *obj);
struct obj_data *get_obj_in_list_vis(struct char_data *ch, char *name,
                                     int *number, struct obj_data *list);
```

### Communication Functions

#### Player Communication
```c
// Direct messaging
void send_to_char(struct char_data *ch, const char *messg, ...);
void act(const char *str, int hide_invisible, struct char_data *ch,
         struct obj_data *obj, const void *vict_obj, int type);

// Room communication
void send_to_room(room_rnum room, const char *messg, ...);
void send_to_outdoor(const char *messg, ...);

// Channel communication
void perform_tell(struct char_data *ch, struct char_data *vict, char *arg);
void perform_say(struct char_data *ch, const char *argument);
```

#### Logging and Debugging
```c
// System logging
void log(const char *format, ...);
void mudlog(int type, int level, int file, const char *str, ...);

// Debug output
void debug_log(const char *format, ...);
void core_dump_real(const char *who, int line);
#define core_dump() core_dump_real(__FILE__, __LINE__)
```

### Database Integration

#### Player Data Operations
```c
// Player loading and saving
int load_char(const char *name, struct char_data *ch);
void save_char(struct char_data *ch);
void save_char_vars(struct char_data *ch);

// Database queries
MYSQL_RES *mysql_query_wrapper(const char *query);
void mysql_safe_query(const char *format, ...);
```

### Scripting Integration

#### DG Script Functions
```c
// Script execution
void script_trigger_check(void);
int script_driver(struct char_data *ch, struct trig_data *trig,
                  int type, int mode);

// Variable management
void add_var(struct trig_var_data **var_list, char *name, char *value,
             long id);
struct trig_var_data *find_var(struct trig_var_data *var_list, char *name);
void remove_var(struct trig_var_data **var_list, char *name);
```

## Development Workflow

### Git Workflow

#### Branch Management
```bash
# Create feature branch
git checkout -b feature/new-combat-system

# Regular commits with descriptive messages
git commit -m "Add initiative-based combat ordering

- Implement initiative calculation based on DEX and level
- Sort combat list by initiative values
- Add initiative display in combat status"

# Push feature branch
git push origin feature/new-combat-system
```

#### Code Review Process
1. **Create Pull Request** with detailed description
2. **Automated Testing** - ensure all tests pass
3. **Code Review** - at least one reviewer approval required
4. **Integration Testing** - test with full codebase
5. **Merge** - squash commits for clean history

### Testing Procedures

#### Unit Testing
```c
// Example unit test using CuTest framework
void test_combat_initiative_calculation(CuTest *tc) {
    struct char_data *ch = create_char();

    // Set test values
    GET_DEX(ch) = 16; // +3 modifier
    GET_LEVEL(ch) = 5;

    // Test initiative calculation
    int initiative = calculate_initiative(ch);

    // Verify result (base roll + DEX mod + level/4)
    CuAssertTrue(tc, initiative >= 4 && initiative <= 24); // 1d20+3+1

    free_char(ch);
}
```

#### Integration Testing
```bash
# Build and run tests
make clean
make cutest

# Run specific test suites
./test_combat_system
./test_database_integration
```

### Performance Optimization

#### Profiling
```bash
# Compile with profiling
make PROFILE=-pg

# Run server with test load
./run_performance_test.sh

# Analyze results
gprof ../bin/circle gmon.out > profile_analysis.txt
```

#### Memory Leak Detection
```bash
# Run with Valgrind
valgrind --leak-check=full --show-leak-kinds=all \
         --track-origins=yes ../bin/circle

# Check for memory issues
valgrind --tool=memcheck ../bin/circle
```

## Common Development Tasks

### Adding New Commands

#### Command Implementation
```c
// In interpreter.c - add to cmd_info table
{"newcommand", POS_STANDING, do_newcommand, 0, 0},

// Command function implementation
ACMD(do_newcommand) {
  struct char_data *victim;
  char arg[MAX_INPUT_LENGTH];

  // Parse arguments
  one_argument(argument, arg);

  // Validate input
  if (!*arg) {
    send_to_char(ch, "Usage: newcommand <target>\r\n");
    return;
  }

  // Find target
  if (!(victim = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "They aren't here.\r\n");
    return;
  }

  // Perform command action
  perform_newcommand(ch, victim);
}
```

### Adding New Game Mechanics

#### Skill System Extension
```c
// Add to constants.c
const char *skill_names[] = {
  "reserved",
  "backstab",
  "bash",
  "hide",
  "kick",
  "pick lock",
  "new skill",  // Add new skill here
  "\n"
};

// Update skill numbers in structs.h
#define SKILL_NEW_SKILL    6

// Implement skill function
void perform_new_skill(struct char_data *ch, struct char_data *victim) {
  int skill_level = GET_SKILL(ch, SKILL_NEW_SKILL);

  if (skill_level < 1) {
    send_to_char(ch, "You don't know how to do that.\r\n");
    return;
  }

  // Skill implementation...
  improve_skill(ch, SKILL_NEW_SKILL);
}
```

## Debugging Techniques

### Common Debugging Scenarios

#### Crash Investigation
```c
// Add debug output before potential crash points
log("DEBUG: Entering function %s with ch=%p", __func__, ch);

// Validate pointers before use
if (!ch) {
  log("SYSERR: %s called with NULL character", __func__);
  core_dump();
  return;
}

// Check data integrity
if (IN_ROOM(ch) == NOWHERE) {
  log("SYSERR: Character %s in NOWHERE", GET_NAME(ch));
  char_to_room(ch, real_room(3001)); // Emergency room
}
```

#### Memory Corruption Detection
```c
// Use magic numbers to detect corruption
#define CHAR_MAGIC 0xDEADBEEF
struct char_data {
  unsigned int magic;  // Add at start of structure
  // ... other members
};

// Check magic number before operations
bool validate_char(struct char_data *ch) {
  if (!ch || ch->magic != CHAR_MAGIC) {
    log("SYSERR: Invalid character structure detected");
    return FALSE;
  }
  return TRUE;
}
```

## Contributing Guidelines

### Code Submission Process

1. **Fork Repository** and create feature branch
2. **Implement Changes** following coding standards
3. **Add Tests** for new functionality
4. **Update Documentation** as needed
5. **Submit Pull Request** with detailed description
6. **Address Review Comments** promptly
7. **Merge** after approval and testing

### Documentation Requirements

- **Function Documentation** - All public functions must have Doxygen comments
- **API Changes** - Update relevant API documentation
- **User-Facing Changes** - Update help files and user guides
- **Architecture Changes** - Update technical documentation

### Quality Assurance

- **Code Review** - All changes require peer review
- **Testing** - Unit tests for new functionality
- **Performance** - Profile performance-critical changes
- **Compatibility** - Ensure backward compatibility where possible

## PHP Tools Development and Security

### Modern PHP Development Standards

The LuminariMUD PHP tools have been modernized to follow current security and development best practices:

#### Security Framework
```php
// Authentication & Authorization
if (!SecurityConfig::isAuthenticated(['developer', 'admin'])) {
    ErrorHandler::authenticationError('tool_name');
}

// Input Validation
$input = InputValidator::validate($_POST['field'], 'identifier', 50);
if ($input === false) {
    ErrorHandler::validationError("Invalid input");
}

// CSRF Protection
SecurityConfig::validateCSRF();
echo HTMLHelper::csrfTokenField();

// XSS Prevention
echo HTMLHelper::escape($user_input);
```

#### Code Organization
- **Shared Configuration**: `config.php` with reusable security functions
- **PSR-4 Autoloading**: `autoload.php` with modern PHP standards
- **Environment Configuration**: `.env` file for secure credential management
- **Caching System**: Performance optimization for database queries

#### Deployment Requirements
- **PHP 7.4+** (PHP 8.1+ recommended)
- **MySQL 5.7+** with dedicated user account
- **HTTPS enabled** web server
- **Proper file permissions** (`.env` = 600, PHP files = 644)

#### Security Configuration
```bash
# Environment setup
cp .env.example .env
# Edit .env with secure values

# File permissions
chmod 600 .env
chmod 644 *.php
chmod 755 cache/ logs/

# Database user (minimal privileges)
CREATE USER 'luminari_tools'@'localhost' IDENTIFIED BY 'secure_password';
GRANT SELECT ON luminari_db.* TO 'luminari_tools'@'localhost';
```

#### Monitoring and Maintenance
- **Error Logging**: Comprehensive logging without information disclosure
- **Access Monitoring**: Track authentication attempts and suspicious activity
- **Regular Updates**: Keep PHP and dependencies current
- **Security Reviews**: Quarterly assessments and annual audits

### PHP Development Guidelines

#### Security Best Practices
1. **Always validate input** using the InputValidator class
2. **Use parameterized queries** for all database operations
3. **Implement CSRF protection** for all forms
4. **Escape output** using HTMLHelper::escape()
5. **Require authentication** for sensitive tools
6. **Log security events** for monitoring

#### Code Quality Standards
1. **Follow PSR-12** coding style guidelines
2. **Use type declarations** where appropriate
3. **Implement proper error handling** with try-catch blocks
4. **Document all functions** with PHPDoc comments
5. **Avoid code duplication** by using shared utilities

#### Performance Considerations
1. **Use caching** for expensive database queries
2. **Optimize database queries** with proper indexing
3. **Implement pagination** for large result sets
4. **Monitor memory usage** in data processing
5. **Profile performance** of critical operations

---

*This guide covers the essential development practices for LuminariMUD. For specific system documentation, refer to the [Technical Documentation Master Index](../TECHNICAL_DOCUMENTATION_MASTER_INDEX.md).*
