# Luminari MUD Utilities and Tools Guide

# Assembled by Zusuk
# Last Updated: July 27, 2025

## Table of Contents

1. [Introduction](#introduction)
2. [Utility Programs](#utility-programs)
3. [Social Commands System](#social-commands-system)
4. [Color System](#color-system)
5. [Automation Scripts](#automation-scripts)
6. [Maintenance Tools](#maintenance-tools)
7. [Development Utilities](#development-utilities)
8. [System Integration](#system-integration)
9. [Advanced Features](#advanced-features)
10. [Troubleshooting](#troubleshooting)

---

## Introduction

Luminari MUD comes with a comprehensive set of utility programs, tools, and systems designed to make administration, development, and maintenance easier. This guide covers all the utilities, their purposes, and how to use them effectively.

### Categories of Tools

**Conversion Utilities:**
- Tools for converting data between formats
- Migration utilities for upgrading from older versions
- Format conversion for external tools

**Maintenance Utilities:**
- Player file management
- Database maintenance
- System cleanup tools

**Informational Utilities:**
- Data analysis tools
- Reporting utilities
- Statistics generators

**Automation Tools:**
- Automatic server management
- Scheduled maintenance
- Player management automation

**Development Tools:**
- Code analysis utilities
- Testing frameworks
- Documentation generators

---

## Utility Programs

### Conversion Utilities

These utilities are typically used once during setup or when migrating data between versions.

#### shopconv
**Purpose:** Convert shop files from older CircleMUD formats to current tbaMUD format.

**Usage:**
```bash
cd util
./shopconv < old_shop_file > new_shop_file
```

**Description:**
The shop format changed between CircleMUD 2.20 and CircleMUD v3. This utility converts old shop files to the new format. Only run this against files that need conversion - running it multiple times will corrupt the data.

**Example:**
```bash
# Backup original file first
cp lib/world/shp/30.shp lib/world/shp/30.shp.backup

# Convert the file
./shopconv < lib/world/shp/30.shp.backup > lib/world/shp/30.shp
```

#### split
**Purpose:** Split large world files into smaller, more manageable pieces.

**Usage:**
```bash
./split <input_file> <output_prefix> <lines_per_file>
```

**Description:**
Useful for breaking down large world files for easier editing or version control. Each output file will contain the specified number of lines.

**Example:**
```bash
# Split a large room file into 100-line chunks
./split lib/world/wld/30.wld rooms_30_ 100
```

#### wld2html
**Purpose:** Convert world files to HTML format for web viewing.

**Usage:**
```bash
./wld2html <world_file> > output.html
```

**Description:**
Creates HTML documentation of your world files, useful for sharing area designs or creating web-based area browsers.

#### webster
**Purpose:** Dictionary lookup utility for builders and developers.

**Usage:**
```bash
./webster <word>
```

**Description:**
Provides dictionary definitions for words, useful when writing room descriptions or help files.

### Maintenance Utilities

#### asciipasswd
**Purpose:** Generate encrypted passwords for player files.

**Usage:**
```bash
./asciipasswd <plaintext_password>
```

**Description:**
Creates properly encrypted passwords that can be used in ASCII player files. Useful for password recovery or creating test accounts.

**Example:**
```bash
# Generate encrypted password
./asciipasswd "mypassword"
# Output: $1$salt$encryptedpasswordhash
```

#### sign
**Purpose:** Add digital signatures to files for integrity checking.

**Usage:**
```bash
./sign <filename>
```

**Description:**
Creates checksums or signatures for important files to detect corruption or unauthorized changes.

### Informational Utilities

#### listrent
**Purpose:** Display rent information for player objects.

**Usage:**
```bash
./listrent [player_name]
```

**Description:**
Shows detailed rent information for player objects, including costs and storage details. Useful for debugging rent issues or analyzing the game economy.

**Example Output:**
```
Player: Zusuk
Object: a steel sword (rent: 50 coins/day)
Object: leather armor (rent: 25 coins/day)
Total daily rent: 75 coins
```

### Internal Utilities

#### autowiz
**Purpose:** Automatically maintain wizard lists and player rankings.

**Usage:**
```bash
# Usually called automatically by the MUD
./autowiz
```

**Description:**
Maintains the wizlist file by scanning player files and updating immortal rankings. Can be run manually or automatically at regular intervals.

**Features:**
- Automatically updates wizard lists
- Handles player level changes
- Maintains proper formatting
- Supports custom ranking criteria

**Configuration:**
Edit the autowiz configuration in your MUD's source code to customize:
- Minimum levels for inclusion
- Ranking criteria
- Output format
- Update frequency

---

## Social Commands System

The social system allows players to express emotions and interact with each other through predefined actions. Luminari MUD includes a comprehensive social editor and management system.

### Social Editor (AEDIT)

**Purpose:** Create and modify social commands online.

**Usage:**
```
aedit <social_name>
```

**Access:** Requires OLC privileges (usually level 33+)

**Features:**
- Real-time social creation
- Built-in validation
- Automatic formatting
- Immediate testing capability

### Social Message Codes

Social messages use special codes to insert character names and pronouns:

**Character Codes (lowercase = actor, uppercase = target):**
- `$n` / `$N` - Character name
- `$e` / `$E` - Subject pronoun (he/she/it)
- `$m` / `$M` - Object pronoun (him/her/it)
- `$s` / `$S` - Possessive pronoun (his/her/its)

**Special Codes:**
- `$t` - Body part of target
- `$p` - Object being used

### Social File Format

Socials are stored in a specific format:

```
<command> <sort_name> <hide_flag> <min_pos_victim> <min_pos_char> <min_level>
<message to character if no argument>
<message to others if no argument>
<message to char if victim not found>
<message to char if victim is character>
<message to others if victim is character>
<message to char if victim found>
<message to others if victim found>
<message to victim>
```

### Example Social

```
smile smile 0 0 0 0
You smile happily.
$n smiles happily.
Smile at who?
You smile at yourself.
$n smiles at $mself.
You smile at $N.
$n smiles at $N.
$n smiles at you.
```

### Creating Custom Socials

**Step 1: Plan the Social**
- Choose a unique command name
- Decide on required positions
- Write clear, engaging messages
- Consider all possible scenarios

**Step 2: Use AEDIT**
```
aedit newsocial
```

**Step 3: Fill in Messages**
- Test each message type
- Use appropriate pronoun codes
- Ensure messages make sense in context

**Step 4: Test and Refine**
- Test with different character types
- Check for grammatical errors
- Verify all message variations work

### Social Management Commands

**ASTAT:** View existing social statistics
```
astat <social_name>
```

**ALIST:** List all available socials
```
alist
```

**Social Removal:** Delete socials (implementor only)
```
aedit <social_name>
# Set all messages to empty and save
```

---

## Color System

Luminari MUD supports ANSI color codes to enhance the visual experience for players. The color system is flexible and allows players to choose their preferred color level.

### Color Levels

Players can set their color preference using the `color` command:

**Color Levels:**
- **Off:** No color codes displayed
- **Brief:** Minimal color for important information
- **Normal:** Standard color scheme
- **Complete:** Full color experience

### Programming with Colors

**Include Required Header:**
```c
#include "screen.h"
```

**Available Colors:**
- Normal (NRM)
- Red (RED)
- Green (GRN)
- Yellow (YEL)
- Blue (BLU)
- Magenta (MAG)
- Cyan (CYN)
- White (WHT)

### Color Macro Series

**K Series (Direct):**
```c
sprintf(buf, "%sThis is red text%s\n", KRED, KNRM);
```

**CC Series (Conditional):**
```c
sprintf(buf, "%sThis is red text%s\n", CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
```

The CC series automatically checks the player's color preference and only displays color if appropriate.

### Color Usage Examples

**Basic Coloring:**
```c
send_to_char(ch, "%sWelcome to the game!%s\n", CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));
```

**Conditional Coloring:**
```c
if (GET_LEVEL(ch) >= LVL_IMMORT) {
    send_to_char(ch, "%s[IMMORTAL]%s %s\n",
                 CCYEL(ch, C_NRM), CCNRM(ch, C_NRM), message);
} else {
    send_to_char(ch, "%s\n", message);
}
```

**Combat Messages:**
```c
sprintf(buf, "%sYou hit %s for %d damage!%s\n",
        CCRED(ch, C_NRM), GET_NAME(victim), damage, CCNRM(ch, C_NRM));
send_to_char(ch, buf);
```

### Color Configuration

**Player Color Preferences:**
Players can customize their color experience:
```
color off     # Disable all colors
color brief   # Minimal colors
color normal  # Standard colors
color complete # Full colors
```

**Administrative Color Control:**
Administrators can set default color levels and create custom color schemes for different message types.

### Best Practices

**Color Usage Guidelines:**
- Use colors to enhance readability, not overwhelm
- Maintain consistency in color meanings
- Provide options for colorblind players
- Test colors on different terminal types
- Use the CC series for player-facing messages

**Performance Considerations:**
- Color codes add to message length
- Consider bandwidth for players with slow connections
- Cache color strings when possible
- Use color sparingly in high-frequency messages

---

## Automation Scripts

Luminari MUD includes several automation scripts to help manage server operations and reduce administrative overhead.

### Autorun Script

**Purpose:** Automatically restart the MUD server if it crashes or shuts down.

**Location:** `autorun` (in the main MUD directory)

**Usage:**
```bash
./autorun &
```

**Features:**
- Automatic crash detection and restart
- Log rotation and management
- Memory usage monitoring
- Email notifications (if configured)
- Customizable restart conditions

**Configuration Options:**
Edit the autorun script to customize:
```bash
# Port number
PORT=4000

# Maximum number of crashes before giving up
MAXCRASH=10

# Email address for notifications
EMAIL="admin@yourmud.com"

# Log file locations
LOGDIR="log"
SYSLOG="$LOGDIR/syslog"

# Pause between restarts (seconds)
PAUSE=10
```

**Advanced Features:**
- **Hot Reboot Support:** Maintains player connections during restarts
- **Memory Monitoring:** Restarts if memory usage exceeds limits
- **Time-based Restarts:** Scheduled restarts for maintenance
- **Backup Integration:** Automatic backups before restarts

### Platform-Specific Autorun Scripts

**Unix/Linux (autorun.sh):**
```bash
#!/bin/bash
# Standard Unix autorun script with full features
```

**Windows (autorun.cmd):**
```cmd
@echo off
REM Windows batch file version
```

**Perl Version (autorun.pl):**
```perl
#!/usr/bin/perl
# Cross-platform Perl implementation
```

**macOS (macrun.pl):**
```perl
#!/usr/bin/perl
# macOS-specific optimizations
```

### Build Automation

**VMS Build Script (build_circlemud.com):**
For VMS systems, provides automated compilation:
```
$ @build_circlemud
```

**Features:**
- Automatic dependency checking
- Optimized compilation flags
- Error reporting and logging
- Integration with VMS development tools

---

## Maintenance Tools

### Player File Management

#### ASCII Player Files
**Purpose:** Modern player file format that's human-readable and easier to debug.

**Features:**
- Text-based format for easy editing
- Better error recovery
- Simplified backup and restoration
- Cross-platform compatibility

**Conversion Tools:**
```bash
# Convert from binary to ASCII (if needed)
./plr2ascii < binary_file > ascii_file

# Validate ASCII player files
./validate_plr ascii_file
```

#### Player File Utilities

**purgeplay:**
```bash
./purgeplay <days_inactive>
```
Removes player files that haven't been accessed for the specified number of days.

**showplay:**
```bash
./showplay <player_name>
```
Displays detailed information about a player file without loading the MUD.

### Object Management

#### Object Stacking System
**Purpose:** Automatically stack identical objects to reduce memory usage and improve performance.

**Features:**
- Automatic stacking of identical items
- Configurable stacking rules
- Performance optimization
- Memory usage reduction

**Configuration:**
```c
// In your configuration
#define STACK_OBJECTS 1
#define MAX_STACK_SIZE 100
```

#### Container Systems

**Enterable Containers:**
Allow players to enter large containers like buildings or vehicles.

**Features:**
- Room-like containers
- Multiple occupant support
- Custom enter/exit messages
- Integration with movement system

### Database Maintenance

#### Index Rebuilding
**Purpose:** Rebuild database indexes for optimal performance.

**Usage:**
```bash
# Rebuild room indexes
./rebuild_index rooms

# Rebuild mobile indexes
./rebuild_index mobs

# Rebuild object indexes
./rebuild_index objects
```

#### Data Validation
**Purpose:** Check world files for consistency and errors.

**Usage:**
```bash
# Validate all world files
./validate_world

# Check specific file types
./validate_world rooms
./validate_world mobs
./validate_world objects
```

**Common Issues Detected:**
- Missing references
- Invalid room connections
- Orphaned objects
- Circular references
- Invalid data ranges

---

## Development Utilities

### Code Analysis Tools

#### Dependency Checker
**Purpose:** Analyze code dependencies and detect circular includes.

**Usage:**
```bash
./check_deps src/*.c
```

**Output:**
- Dependency graph
- Circular dependency warnings
- Unused include detection
- Optimization suggestions

#### Memory Leak Detection
**Purpose:** Detect memory leaks in custom code.

**Integration with Valgrind:**
```bash
# Run MUD under Valgrind
valgrind --leak-check=full --log-file=valgrind.log ./circle

# Analyze results
./analyze_leaks valgrind.log
```

### Testing Framework

#### Unit Testing
**Purpose:** Automated testing of individual functions and systems.

**Test Structure:**
```c
// test_combat.c
#include "test_framework.h"

TEST(combat_damage_calculation) {
    struct char_data *attacker = create_test_char();
    struct char_data *victim = create_test_char();

    int damage = calculate_damage(attacker, victim);

    ASSERT_GT(damage, 0);
    ASSERT_LT(damage, 1000);

    cleanup_test_chars();
}
```

**Running Tests:**
```bash
# Compile test suite
make tests

# Run all tests
./run_tests

# Run specific test category
./run_tests combat
```

#### Integration Testing
**Purpose:** Test system interactions and complex scenarios.

**Features:**
- Automated player simulation
- Multi-user scenario testing
- Performance benchmarking
- Regression testing

### Documentation Tools

#### Help File Generator
**Purpose:** Automatically generate help files from code comments.

**Usage:**
```bash
# Generate help files from source
./gen_help src/*.c > lib/text/help/auto_generated.hlp
```

**Comment Format:**
```c
/**
 * @help COMMAND_NAME
 * @syntax command_name <argument>
 * @description This command does something useful.
 * @example command_name test
 * @see_also RELATED_COMMAND
 */
ACMD(do_command_name) {
    // Implementation
}
```

---

## System Integration

### External Tool Integration

#### Web Interface Integration
**Purpose:** Connect MUD data with web-based tools and interfaces.

**Features:**
- Player statistics export
- Real-time status monitoring
- Web-based administration
- Player forums integration

**Example Integration:**
```bash
# Export player data for web interface
./export_players --format=json > web/data/players.json

# Update web statistics
./update_web_stats
```

#### Database Integration
**Purpose:** Connect with external databases for enhanced features.

**Supported Databases:**
- MySQL/MariaDB
- PostgreSQL
- SQLite
- MongoDB (with adapters)

**Configuration:**
```c
// Database connection settings
#define DB_HOST "localhost"
#define DB_USER "muduser"
#define DB_PASS "password"
#define DB_NAME "luminari"
```

### Monitoring Integration

#### System Monitoring
**Purpose:** Integration with system monitoring tools.

**Nagios Integration:**
```bash
# Check MUD status for Nagios
./check_mud_status --nagios
```

**Zabbix Integration:**
```bash
# Export metrics for Zabbix
./mud_metrics --zabbix
```

#### Log Analysis
**Purpose:** Integration with log analysis tools.

**ELK Stack Integration:**
```bash
# Format logs for Elasticsearch
./format_logs --elasticsearch

# Send logs to Logstash
./send_logs --logstash
```

---

## Advanced Features

### Protocol Systems

#### MUD Client Protocol (MCP)
**Purpose:** Enhanced communication with advanced MUD clients.

**Features:**
- Rich text formatting
- Client-side scripting support
- Enhanced user interfaces
- Multimedia content delivery

**Implementation:**
```c
// Send MCP message
send_mcp_message(ch, "client-info", "version", "1.0");
```

#### GMCP (Generic MUD Communication Protocol)
**Purpose:** Structured data exchange with clients.

**Features:**
- JSON-based data exchange
- Real-time status updates
- Client automation support
- Enhanced user experience

### Language Systems

#### Multi-Language Support
**Purpose:** Support for multiple languages and character sets.

**Features:**
- Unicode support
- Language-specific help files
- Automatic language detection
- Translation management

**Configuration:**
```c
// Language settings
#define DEFAULT_LANGUAGE "english"
#define SUPPORT_UNICODE 1
```

### Vehicle Systems

#### Transportation
**Purpose:** Advanced vehicle and transportation systems.

**Features:**
- Multi-room vehicles
- Vehicle combat
- Passenger management
- Dynamic movement

**Example Vehicle:**
```
Vehicle: Airship
Rooms: Bridge, Passenger Deck, Cargo Hold
Capacity: 20 passengers
Speed: Fast
Special: Flying vehicle
```

---

## Troubleshooting

### Common Utility Issues

**"Permission denied" when running utilities:**
```bash
# Fix permissions
chmod +x util/*
```

**Utilities not found:**
```bash
# Compile utilities
cd util
make
```

**Conversion utilities producing errors:**
```bash
# Check input file format
file input_file

# Verify file isn't already converted
head -n 10 input_file
```

### Social System Issues

**Socials not working:**
- Check social file format
- Verify AEDIT permissions
- Test with simple social first
- Check for syntax errors in messages

**Color codes not displaying:**
- Verify player color settings
- Check terminal compatibility
- Test with simple color codes
- Verify include statements in code

### Automation Script Issues

**Autorun not restarting MUD:**
- Check script permissions
- Verify crash detection logic
- Test manual restart
- Check system resources

**Scripts failing on different platforms:**
- Verify shell compatibility
- Check path separators
- Test with platform-specific versions
- Verify required tools are installed

### Performance Issues

**Utilities running slowly:**
- Check available system resources
- Optimize file I/O operations
- Consider batch processing
- Monitor memory usage

**High memory usage:**
- Check for memory leaks
- Optimize data structures
- Consider lazy loading
- Monitor object creation

---

## Conclusion

The utilities and tools provided with Luminari MUD form a comprehensive ecosystem for managing, maintaining, and enhancing your MUD server. From basic file conversion to advanced automation systems, these tools are designed to reduce administrative overhead and improve the overall experience for both administrators and players.

### Key Takeaways

**Utility Management:**
- Understand the purpose of each utility before using it
- Always backup data before running conversion utilities
- Test utilities in a development environment first
- Keep utilities updated with your MUD version

**Automation Benefits:**
- Reduces manual administrative tasks
- Improves server reliability and uptime
- Provides consistent maintenance procedures
- Enables advanced monitoring and alerting

**System Integration:**
- Leverage external tools for enhanced functionality
- Maintain compatibility with standard protocols
- Document custom integrations thoroughly
- Plan for scalability and growth

### Best Practices

**Development:**
- Use the testing framework for quality assurance
- Integrate code analysis tools into your workflow
- Maintain comprehensive documentation
- Follow established coding standards

**Administration:**
- Implement automated backup procedures
- Monitor system performance regularly
- Keep detailed logs of administrative actions
- Plan for disaster recovery scenarios

**Community:**
- Share useful utilities with the MUD community
- Contribute improvements and bug fixes
- Document custom tools and procedures
- Help others learn and implement best practices

---

## Additional Resources

### Documentation References
- **Administrator's Guide:** Server management and configuration
- **Builder's Manual:** World creation and area building
- **Developer's Guide:** Code modification and programming
- **Installation Guide:** Platform-specific setup instructions

### Community Resources
- **Utility Collections:** Shared tools and scripts
- **Forums:** Discussion and support
- **Code Repositories:** Open source utilities
- **Tutorial Collections:** Step-by-step guides

### Technical References
- **ANSI Color Codes:** Terminal color reference
- **MUD Protocols:** MCP, GMCP, and other standards
- **Database Documentation:** Integration guides
- **System Administration:** Unix/Linux administration guides

---

*This utilities guide consolidates information from utils.txt, socials.txt, color.txt, and numerous utility README files, updated and expanded for modern MUD administration and development.*

*Last updated: July 27, 2025*