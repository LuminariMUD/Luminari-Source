# LuminariMUD Help System Documentation

## Table of Contents
1. [System Overview](#system-overview)
2. [Database Architecture](#database-architecture)
3. [Core Components](#core-components)
4. [User Commands](#user-commands)
5. [Search Algorithms](#search-algorithms)
6. [Caching System](#caching-system)
7. [OLC Help Editor (hedit)](#olc-help-editor-hedit)
8. [Chain of Responsibility Pattern](#chain-of-responsibility-pattern)
9. [Help Content Generation](#help-content-generation)
10. [Migration & Backup](#migration--backup)
11. [Troubleshooting](#troubleshooting)
12. [Best Practices](#best-practices)

---

## System Overview

The LuminariMUD help system is a sophisticated, multi-layered documentation framework that provides:
- **Database-driven content** with MySQL/MariaDB backend
- **Intelligent search** with fuzzy matching and soundex algorithms
- **In-memory caching** for performance optimization
- **OLC integration** for real-time content editing
- **Chain of Responsibility pattern** for extensible help handlers
- **Auto-generation** of help content from game data
- **Database-only mode** - No help.hlp file required (default)
- **Optional file fallback** - Can use help.hlp if present

### Key Features
- **3,143+ searchable keywords** mapping to 1,815+ help entries
- **Level-based access control** (min_level 0-60)
- **Case-insensitive prefix matching** with partial match support
- **Soundex fuzzy search** for typo tolerance
- **5-minute cache TTL** with 50-entry cache size
- **Real-time editing** via OLC hedit interface
- **Automatic content generation** for game elements

---

## Database Architecture

## Connection Details
- **Config File:** `lib/mysql_config`
- **Connection:** Managed through `src/mysql.c` using prepared statements
- **Status:** ✅ FUNCTIONAL - Database connection verified and operational

## Database Schema

### Primary Tables

#### help_entries (1,815+ rows)
```sql
CREATE TABLE help_entries (
    id INT AUTO_INCREMENT PRIMARY KEY,
    tag VARCHAR(50) UNIQUE NOT NULL,           -- Unique identifier (e.g., 'score')
    entry LONGTEXT NOT NULL,                   -- The actual help content
    min_level INT DEFAULT 0,                   -- Minimum level to view
    max_level INT DEFAULT 1000,                -- Maximum level (future use)
    auto_generated BOOLEAN DEFAULT FALSE,      -- TRUE if auto-generated
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_help_tag (tag),
    INDEX idx_min_level (min_level),
    INDEX idx_auto_generated (auto_generated)
);
```

#### help_keywords (3,143+ rows)
```sql
CREATE TABLE help_keywords (
    id INT AUTO_INCREMENT PRIMARY KEY,
    help_tag VARCHAR(50) NOT NULL,             -- Links to help_entries.tag
    keyword VARCHAR(100) NOT NULL,             -- Search keyword
    FOREIGN KEY (help_tag) REFERENCES help_entries(tag) ON DELETE CASCADE,
    INDEX idx_keyword (keyword),
    INDEX idx_help_tag (help_tag),
    UNIQUE KEY unique_tag_keyword (help_tag, keyword)
);
```

### Supporting Tables

#### help_versions (Audit Trail)
```sql
CREATE TABLE help_versions (
    id INT AUTO_INCREMENT PRIMARY KEY,
    help_tag VARCHAR(50) NOT NULL,
    old_entry LONGTEXT,
    new_entry LONGTEXT,
    changed_by VARCHAR(50),
    change_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    change_type ENUM('CREATE', 'UPDATE', 'DELETE'),
    INDEX idx_help_tag (help_tag),
    INDEX idx_change_date (change_date)
);
```

#### help_search_history (Analytics)
```sql
CREATE TABLE help_search_history (
    id INT AUTO_INCREMENT PRIMARY KEY,
    search_term VARCHAR(100),
    found BOOLEAN,
    player_name VARCHAR(50),
    search_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_search_term (search_term),
    INDEX idx_search_date (search_date)
);
```

#### help_related_topics (Cross-references)
```sql
CREATE TABLE help_related_topics (
    id INT AUTO_INCREMENT PRIMARY KEY,
    help_tag VARCHAR(50) NOT NULL,
    related_tag VARCHAR(50) NOT NULL,
    FOREIGN KEY (help_tag) REFERENCES help_entries(tag) ON DELETE CASCADE,
    FOREIGN KEY (related_tag) REFERENCES help_entries(tag) ON DELETE CASCADE,
    UNIQUE KEY unique_relation (help_tag, related_tag)
);
```

### Migration/Backup Tables
- **help_entries_frmud** - Backup/migration from file-based system
- **help_keywords_frmud** - Keyword backup/migration
- **help_topics_backup** - Additional backup table

---

## Core Components

### Architecture Overview
- **Primary Storage:** MySQL/MariaDB database (required)
- **Optional File Support:** File-based (`lib/text/help/help.hlp`, if present)
- **Cache Layer:** In-memory LRU cache (50 entries, 5-minute TTL)
- **Search Engine:** Multi-algorithm search with fuzzy matching
- **Content Handlers:** Chain of Responsibility pattern for extensibility

#### Operating Modes
1. **Database-Only Mode (Default)**
   - No help.hlp file required
   - All help content stored in MySQL
   - System logs informational message if help.hlp missing
   - Full functionality without file system dependency

2. **Dual Mode (Optional)**
   - Uses database as primary source
   - Falls back to help.hlp if database query fails
   - Provides redundancy for critical systems

### Key Data Structures

#### help_entry_list (src/help.h)
```c
struct help_entry_list {
    char *tag;           // Unique identifier
    char *keywords;      // Comma-separated keyword list
    char *entry;         // Help content text
    int min_level;       // Minimum access level
    char *last_updated;  // Timestamp
    struct help_keyword_list *keyword_list;
    struct help_entry_list *next;
};
```

#### help_cache_entry (src/help.c)
```c
struct help_cache_entry {
    char *argument;      // Search term
    int level;           // User level when cached
    struct help_entry_list *result;  // Cached result
    time_t timestamp;    // Cache timestamp
    struct help_cache_entry *next;
};
```

---

## User Commands

### help - Main Help Command
**Usage:** `help [topic]`
- **Location:** src/help.c:1233 (do_help)
- **Access:** All players (POS_DEAD)
- **Features:**
  - No arguments: Shows default help screen
  - With topic: Searches for matching help entry
  - Partial matches: Shows list of possibilities
  - Fuzzy search: Suggests similar topics via soundex
  - Level filtering: Only shows appropriate content

### helpsearch - Full-text Search
**Usage:** `helpsearch <search term>`
- **Location:** src/help.c:1647 (do_helpsearch)
- **Access:** All players (POS_DEAD)
- **Features:**
  - Searches within help content (not just keywords)
  - Returns all matching entries
  - Useful for finding related topics

### hedit - OLC Help Editor
**Usage:** `hedit <tag | new>`
- **Location:** src/hedit.c
- **Access:** Builders+ (LVL_BUILDER)
- **Features:**
  - Create new help entries
  - Edit existing entries
  - Manage keywords
  - Set access levels
  - Real-time database updates

### helpcheck - Validate Help System
**Usage:** `helpcheck`
- **Location:** src/act.wizard.c
- **Access:** Immortals (LVL_IMMORT)
- **Features:**
  - Checks database connectivity
  - Validates help entries
  - Reports missing or broken links
  - Identifies orphaned keywords

### hindex - Rebuild Help Index
**Usage:** `hindex`
- **Location:** src/act.wizard.c
- **Access:** Implementor (LVL_IMPL)
- **Features:**
  - Rebuilds file-based help index
  - Synchronizes database with files
  - Clears cache

### helpgen - Generate, Import, and Export Help Content
**Usage:** `helpgen <type> [arguments]`
- **Location:** src/hedit.c:1707
- **Access:** Implementor (LVL_IMPL)
- **Features:**
  - Auto-generates help for game elements
  - Types: classes, races, feats, spells, skills
  - Marks entries as auto_generated
  - Updates existing auto-generated content
  - **Import functionality** (see Import System section below)
  - **Export functionality** (see Export System section below)

---

## Search Algorithms

### Search Priority Order
1. **Exact Match** - Direct tag or keyword match
2. **Prefix Match** - Keywords starting with search term
3. **Partial Match** - Keywords containing search term
4. **Soundex Match** - Phonetically similar keywords
5. **Full-text Search** - Content containing search term

### Algorithm Details

#### Primary Search (search_help)
**Location:** src/help.c:161
```c
1. Check cache for recent queries
2. Normalize search term (lowercase, trim)
3. Query database with prepared statements
4. Apply level restrictions
5. Fall back to file-based search if needed
6. Cache successful results
7. Return help_entry_list structure
```

#### Soundex Fuzzy Search
**Location:** src/help.c:550
- Uses MySQL SOUNDEX() function
- Finds phonetically similar keywords
- Suggests alternatives for typos
- Example: "comand" suggests "command"

#### Full-text Search
**Location:** src/help.c:search_help_fulltext
- Searches within help content
- Case-insensitive LIKE queries
- Returns all matching entries
- Used by helpsearch command

---

## Caching System

### Cache Configuration
- **Size:** 50 entries (HELP_CACHE_SIZE)
- **TTL:** 300 seconds (HELP_CACHE_TIMEOUT)
- **Strategy:** LRU with timestamp-based expiration

### Cache Operations

#### Cache Lookup (get_cached_help)
**Location:** src/help.c
1. Iterate through cache entries
2. Match on argument AND level
3. Check timestamp validity
4. Return deep copy if valid
5. Purge expired entries periodically

#### Cache Addition (add_to_help_cache)
1. Check cache size limit
2. Evict oldest entry if full
3. Deep copy result data
4. Store with current timestamp
5. Increment cache counter

#### Cache Purging
- Automatic during lookups
- Manual via hindex command
- On server reboot
- When cache is full

---

## OLC Help Editor (hedit)

### Editor Modes
```c
HEDIT_MAIN_MENU      - Main editing menu
HEDIT_ENTRY          - Edit help text
HEDIT_KEYWORDS       - Manage keywords
HEDIT_KEYWORD_MENU   - Keyword submenu
HEDIT_MIN_LEVEL      - Set minimum level
HEDIT_CONFIRM_SAVE   - Save confirmation
HEDIT_CONFIRM_DELETE - Delete confirmation
```

### Workflow
1. **Create/Load Entry**
   - `hedit new` - Create new entry
   - `hedit <tag>` - Edit existing entry

2. **Edit Content**
   - Main text editing with string editor
   - Keyword management (add/remove)
   - Access level configuration

3. **Save Process**
   - Validate data integrity
   - Update database tables
   - Update version history
   - Clear relevant cache entries
   - Broadcast changes to connected users

### Database Operations
**Location:** src/hedit.c:331 (hedit_save_to_db)
1. Begin transaction
2. Delete existing keywords
3. Insert/update help_entries
4. Insert new keywords
5. Update version history
6. Commit transaction
7. Clear cache

---

## Chain of Responsibility Pattern

### Design Overview
The help system uses a Chain of Responsibility pattern to handle different types of help content. Each handler is responsible for one type of content and can pass requests to the next handler.

### Handler Structure
```c
struct help_handler {
    const char *name;           // Handler identifier
    help_handler_func handler;  // Processing function
    struct help_handler *next;  // Next in chain
};
```

### Registered Handlers
1. **Database Handler** - Primary database searches
2. **Deity Handler** - Deity-specific help
3. **Region Handler** - Zone/area information
4. **Background Handler** - Character backgrounds
5. **Discovery Handler** - Alchemist discoveries
6. **Feat Handler** - Feat descriptions
7. **Evolution Handler** - Summoner evolutions
8. **Weapon Handler** - Weapon information
9. **Armor Handler** - Armor information
10. **Class Handler** - Class descriptions
11. **Race Handler** - Race information
12. **Soundex Handler** - Fuzzy search suggestions

### Handler Registration
**Location:** src/help.c (init_help_handlers)
```c
void init_help_handlers(void) {
    register_help_handler("database", handle_database_help);
    register_help_handler("deity", handle_deity_help);
    // ... additional handlers
    register_help_handler("soundex", handle_soundex_suggestions);
}
```

---

## Help Content Generation

### Auto-Generation System
The helpgen command automatically creates help entries for game elements:

#### Supported Types
- **Classes** - Generate from class definitions
- **Races** - Generate from race data
- **Feats** - Generate from feat tables
- **Spells** - Generate from spell definitions
- **Skills** - Generate from skill data
- **Weapons** - Generate from weapon tables
- **Armor** - Generate from armor types

#### Generation Process
1. Query game data structures
2. Format into help text
3. Set auto_generated flag
4. Create appropriate keywords
5. Store in database
6. Update existing if auto_generated=TRUE

### Example Usage
```
helpgen class warrior
helpgen race elf
helpgen feat power-attack
helpgen spell fireball
```

---

## Migration & Backup

### Legacy Help File Import System

#### Overview
The `helpgen import` command provides a comprehensive solution for importing the legacy help.hlp file (1,265 entries) into the MySQL database while intelligently handling duplicates and preserving existing content.

#### Command Syntax
```
helpgen import <mode>
```
**Required modes (no default - must explicitly choose):**
- **preview** - Dry run showing what would be imported without making changes
- **skip** - Only import new entries, skip any that already exist
- **merge** - Intelligently merge duplicate keywords with suffixes (_2, _3)
- **force** - Overwrite existing entries completely

#### Import Modes

##### Preview Mode
- Shows what entries would be imported, skipped, or merged
- No database changes are made
- Limited to first 100 entries for readability
- Safe way to test before actual import

##### Force Mode
- Deletes existing entries that share keywords
- Replaces with new content from help.hlp
- Maintains referential integrity
- Use with caution - data loss possible

##### Skip Mode (Safest)
- Only imports entries that don't exist in database
- Skips all entries with matching keywords
- No data loss or duplication
- Fastest import method
- Ideal for initial imports to populated database

##### Merge Mode (Recommended for duplicates)
- Detects entries with duplicate keywords
- Creates new entries with numeric suffixes (_2, _3, etc.)
- Preserves both original and imported content
- Example: "help" becomes "help_2" if "help" already exists
- Maintains all keywords pointing to new suffixed tags

#### Implementation Details

##### File Parser
- Reads from `text/help/help.hlp` (relative to lib directory)
- Parses format: keywords line, content body, #level marker
- Handles multi-line content and special characters
- Generates unique tags from first keyword

##### Duplicate Detection
- Checks each keyword against existing database entries
- Identifies conflicts before any changes are made
- Reports exact duplicate counts and affected entries

##### Database Operations
- Uses transactions for atomicity (all-or-nothing)
- Dynamic memory allocation prevents buffer overflows
- Proper SQL escaping prevents injection attacks
- Automatic cache clearing after successful import

##### Progress Reporting
- Shows progress every 50 entries during processing
- Uses pagination system to prevent output overflow
- Detailed summary with import/skip/error counts
- Logs all operations to system log

#### Example Import Session
```
> helpgen import preview
Reading help.hlp file from: text/help/help.hlp
  [NEW] abbreviations - would import
  [EXISTS] help - would skip/merge
  [MERGED] score - would create as score_2
...
=== Import Summary ===
Total entries processed: 100
Would import: 45
Would skip: 55

> helpgen import merge
Reading help.hlp file from: text/help/help.hlp
Processing entry 50...
  [IMPORTED] abbreviations
  [MERGED] help (as help_2)
  [SKIPPED] admin - already exists
...
Transaction committed successfully.
=== Import Summary ===
Total entries processed: 1265
Successfully imported: 623
Skipped (duplicates): 642
Errors: 0
```

#### Post-Import Verification
After import, the database typically contains:
- Original entries preserved with original tags
- Imported entries with suffixed tags where conflicts existed
- All keywords properly mapped to appropriate entries
- Example: 3,271 total entries after importing 1,265 file entries

### Database to File Export System

#### Overview
The `helpgen export` command provides a comprehensive solution for exporting the MySQL database help entries back to the help.hlp file format. This is useful for backups, migrations, or sharing content.

#### Command Syntax
```
helpgen export <mode> [filters]
```

**Required modes (no default - must explicitly choose):**
- **preview** - Dry run showing what would be exported without writing files
- **backup** - Creates timestamped backup before exporting (safest)
- **force** - Overwrites help.hlp without creating backup

**Optional filters (add after mode):**
- **noauto** - Exclude auto-generated entries
- **level <num>** - Only export entries accessible at specified level or below

#### Export Modes

##### Preview Mode
- Shows summary of what would be exported
- Displays first 20 entries as examples
- No files are written or modified
- Safe way to test export parameters

##### Backup Mode (Recommended)
- Creates timestamped backup (help.hlp.YYYYMMDD_HHMMSS)
- Then exports current database to help.hlp
- Preserves previous file version
- Safe for regular exports

##### Force Mode
- Directly overwrites help.hlp file
- No backup created
- Use with caution
- Faster for development environments

#### Export Features
- **Proper file format** with keywords, content, and #level markers
- **File termination** with required $~ marker
- **Keyword conversion** to uppercase space-separated format
- **Progress reporting** every 100 entries for large exports
- **File size reporting** upon completion
- **SQL injection prevention** with proper query escaping
- **Memory efficient** processing for thousands of entries

#### Example Export Session
```
> helpgen export preview
PREVIEW MODE - No files will be written
Processing entry 100...
  [  1] abbreviations                  (Level 0, 3 keywords)
  [  2] commands                        (Level 0, 5 keywords)
  ...
=== Export Summary ===
Would export: 3271 entries

> helpgen export backup noauto
BACKUP MODE - Will create backup before exporting
Excluding auto-generated entries
Created backup: text/help/help.hlp.20250813_163045
Processing entry 100...
Processing entry 200...
...
=== Export Summary ===
Exported: 2847 entries
Errors: 0
Output file: text/help/help.hlp
File size: 799709 bytes
```

### File to Database Migration (Legacy Process)
1. Parse help.hlp file format
2. Extract entries and keywords
3. Insert into database tables
4. Verify data integrity
5. Mark as migrated

### Backup Procedures

#### Database Backup
```bash
# Export help tables
mysqldump -u user -p database \
  help_entries help_keywords \
  help_versions help_search_history \
  help_related_topics > help_backup.sql
```

#### File Backup
```bash
# Backup help files
tar -czf help_files.tar.gz lib/text/help/
```

### Restoration
```bash
# Restore database
mysql -u user -p database < help_backup.sql

# Restore files
tar -xzf help_files.tar.gz -C /
```

---

## Troubleshooting

### Common Issues

#### Database Connection Failed
**Symptoms:** Help returns "No help available"
**Solution:**
1. Check `lib/mysql_config` settings
2. Verify MySQL service is running
3. Test connection with `helpcheck`
4. Check error logs for details

#### Cache Not Updating
**Symptoms:** Old help content displayed
**Solution:**
1. Run `hindex` to clear cache
2. Wait for 5-minute TTL expiration
3. Check cache size settings
4. Verify timestamps are correct

#### Missing Help Entries
**Symptoms:** Topics not found
**Solution:**
1. Check keyword mappings
2. Verify access levels
3. Run `helpgen` for auto-content
4. Check database integrity

#### Slow Search Performance
**Symptoms:** Help commands lag
**Solution:**
1. Check database indexes
2. Optimize query performance
3. Increase cache size
4. Review search algorithms

### Debug Mode
Enable help system debugging:
```c
#define HELP_DEBUG 1  // In src/help.c
```

Debug output includes:
- Cache hit/miss statistics
- Database query details
- Search algorithm paths
- Performance metrics

---

## Best Practices

### Content Guidelines
1. **Clear Titles** - Use descriptive tags
2. **Multiple Keywords** - Include variations and aliases
3. **Proper Levels** - Set appropriate access restrictions
4. **Cross-references** - Link related topics
5. **Consistent Format** - Follow established style

### Performance Optimization
1. **Use Cache** - Leverage caching for common queries
2. **Index Keywords** - Maintain database indexes
3. **Batch Updates** - Group database operations
4. **Limit Results** - Use pagination for large sets
5. **Profile Queries** - Monitor slow queries

### Maintenance Tasks
1. **Regular Backups** - Daily database exports
2. **Cache Monitoring** - Check hit rates
3. **Keyword Audit** - Remove duplicates
4. **Content Review** - Update outdated entries
5. **Performance Analysis** - Review search metrics

### Security Considerations
1. **SQL Injection** - Use prepared statements
2. **Access Control** - Enforce level restrictions
3. **Input Validation** - Sanitize search terms
4. **Audit Trail** - Track content changes
5. **Backup Security** - Encrypt sensitive data

---

# File System Reference

## Core Implementation Files
- **`src/help.c`** (1296 lines) - Main help system implementation, search functions, display logic
- **`src/help.h`** (89 lines) - Help system data structures and function declarations
- **`src/hedit.c`** (1847 lines) - OLC help editor implementation, database operations
- **`src/hedit.h`** (31 lines) - Help editor declarations and command definitions

## Database Integration
- **`src/db.c`** - Contains `load_help()` function for file-based loading, help_table management
- **`src/db.h`** - Defines `help_index_element` structure, declares help_table global
- **`src/mysql.c`** - MySQL connection handling, escape functions used by help system
- **`src/db_init.c`** - Database table creation and initialization

## Command Integration
- **`src/interpreter.c`** - Registers help commands (help, hedit, helpcheck, hindex, helpgen)
- **`src/act.h`** - Command declarations
- **`src/comm.c`** - Uses help_table for initial help display

## OLC Framework
- **`src/oasis.h`** - OLC data structures used by hedit
- **`src/oasis.c`** - OLC framework functions
- **`src/genolc.c`** - Generic OLC functions
- **`src/modify.c`** - String editing functions for help text

## Data Files
- **`lib/text/help/help.hlp`** (799,709 bytes) - Legacy help file database
- **`lib/text/help/help`** (1,498 bytes) - Default help screen
- **`lib/text/help/ihelp`** (895 bytes) - Immortal help screen
- **`lib/text/help/index`** (11 bytes) - Help index file
- **`lib/text/help/README`** (101 bytes) - Documentation file

## Configuration Files
- **`lib/mysql_config`** - Database connection parameters

## Database Tables
- **`help_entries`** - Main help content table (3,271+ entries after import)
- **`help_keywords`** - Keyword-to-help mappings (7,206+ keywords after import)
- **`help_entries_frmud`** - Backup/migration table
- **`help_keywords_frmud`** - Backup/migration table
- **`help_topics_backup`** - Additional backup table

---

## Appendix A: Configuration Constants

### Cache Settings (src/help.c)
```c
#define HELP_CACHE_SIZE     50    // Maximum cached entries
#define HELP_CACHE_TIMEOUT  300   // Cache TTL in seconds
#define HELP_DEBUG          0     // Debug output flag
```

### Access Levels
```c
#define LVL_IMPL     60  // Implementor
#define LVL_GRGOD    59  // Greater God
#define LVL_GOD      58  // God
#define LVL_IMMORT   51  // Immortal
#define LVL_BUILDER  55  // Builder
```

### Editor Permissions
```c
#define HEDIT_PERMISSION  LVL_BUILDER  // Minimum level to use hedit
```

---

## Appendix B: Database Queries

### Common Query Examples

#### Find all help entries for a level
```sql
SELECT h.tag, h.entry, GROUP_CONCAT(k.keyword) as keywords
FROM help_entries h
LEFT JOIN help_keywords k ON h.tag = k.help_tag
WHERE h.min_level <= 20
GROUP BY h.tag;
```

#### Find orphaned keywords
```sql
SELECT k.keyword 
FROM help_keywords k
LEFT JOIN help_entries h ON k.help_tag = h.tag
WHERE h.tag IS NULL;
```

#### Search history analysis
```sql
SELECT search_term, COUNT(*) as search_count,
       SUM(found) as found_count
FROM help_search_history
WHERE search_date > DATE_SUB(NOW(), INTERVAL 7 DAY)
GROUP BY search_term
ORDER BY search_count DESC
LIMIT 20;
```

#### Auto-generated content audit
```sql
SELECT tag, updated_at
FROM help_entries
WHERE auto_generated = TRUE
  AND updated_at < DATE_SUB(NOW(), INTERVAL 30 DAY);
```

---

## Appendix C: File Format Specifications

### help.hlp Format
```
#<level>
<keyword1> <keyword2> <keyword3>~
<help text>
<multiple lines>
~
```

Example:
```
#0
SCORE STATS CHARACTER-INFO~
The SCORE command displays your character's current statistics,
including attributes, hit points, experience, and equipment.

Usage: score
~
```

**Note:** Tables are created programmatically in db_init.c

## Supporting Systems
- **`src/utils.c/h`** - Utility functions (string handling, memory management)
- **`src/structs.h`** - Core data structures
- **`src/conf.h`** - Configuration constants
- **`src/sysdep.h`** - System dependencies

---

