# Help System TODO and Implementation Checklist

**Created:** January 2025  
**Last Updated:** December 2025  
**Priority Levels:** CRITICAL | HIGH | MEDIUM | LOW  
**Status:** ⬜ Not Started | 🔄 In Progress | ✅ Complete | 🔒 Blocked

# Database Connection & Structure

## Connection Details
- **Config File:** `lib/mysql_config`
- **Connection:** Managed through `src/mysql.c` using prepared statements

## Database Schema
```sql
help_entries (1,815 rows)
├── tag         VARCHAR(50) PRIMARY KEY  -- Unique identifier (e.g., 'score')
├── entry       TEXT                     -- The actual help content
├── min_level   INT                      -- Minimum level to view (0-60)
└── last_updated TIMESTAMP               -- When entry was last modified

help_keywords (3,143 rows)
├── help_tag    VARCHAR(50) FK           -- Links to help_entries.tag
└── keyword     VARCHAR(50)              -- Search keyword (e.g., 'Score', 'character-info')

-- Indexes: help_tag, keyword for performance
-- One help entry can have multiple keywords
```

## Current Architecture
- **Dual Mode System:** Database primary, file-based fallback
- **File Fallback:** `lib/text/help/help.hlp` (799KB, legacy support)
- **Caching:** In-memory cache with 5-minute TTL
- **Search:** Case-insensitive prefix matching with soundex fuzzy search

# Files Involved in the Help System (update if out of date)

## Core Implementation Files
- **`src/help.c`** (900+ lines) - Main help system implementation, search functions, display logic
- **`src/help.h`** (52 lines) - Help system data structures and function declarations
- **`src/hedit.c`** (1009 lines) - OLC help editor implementation, database operations
- **`src/hedit.h`** (30 lines) - Help editor declarations and command definitions

## Database Integration
- **`src/db.c`** - Contains `load_help()` function for file-based loading, help_table management
- **`src/db.h`** - Defines `help_index_element` structure, declares help_table global
- **`src/mysql.c`** - MySQL connection handling, escape functions used by help system

## Command Integration
- **`src/interpreter.c`** - Registers help commands (help, hedit, helpcheck, hindex)
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
- **`lib/text/help/index`** - Help index file
- **`lib/text/help/README`** - Documentation file

## Configuration Files
- **`lib/mysql_config`** - Database connection parameters

## Database Tables
- **`help_entries`** - Main help content table (1,815 entries)
- **`help_keywords`** - Keyword-to-help mappings (3,143 keywords)
- **`help_entries_frmud`** - Backup/migration table
- **`help_keywords_frmud`** - Backup/migration table
- **`help_topics_backup`** - Additional backup table

**Note:** No SQL schema files found in repository - tables may be created programmatically or schemas are missing

## Supporting Systems
- **`src/utils.c/h`** - Utility functions (string handling, memory management)
- **`src/structs.h`** - Core data structures
- **`src/conf.h`** - Configuration constants
- **`src/sysdep.h`** - System dependencies

# Work / ToDo / Task Lists

## HIGH PRIORITY - This Sprint (Complete within 1 week)

### Connection Pooling - ✅ COMPLETED (January 2025)
- [x] **Implement MySQL connection pooling**
  - [x] Design connection pool manager
  - [x] Implement connection reuse logic
  - [x] Add connection health checks
  - [x] Create pool size configuration
  
**Implementation Details:**
- Created robust connection pool manager with configurable min/max connections (3-10)
- Automatic connection health checks with 60-second intervals
- Connection reuse with automatic refresh of stale connections (30-second timeout)
- Dynamic pool expansion/shrinking based on demand
- Thread-safe operations with per-connection mutexes
- Backward compatibility maintained with legacy conn/conn2/conn3 pointers
- Pool statistics tracking (requests, waits, errors)
- Files modified: src/mysql.c, src/mysql.h

## MEDIUM PRIORITY - Next Release (Complete within 2-4 weeks)

### Search Enhancement
- [ ] **Full-text search implementation**
  - [ ] Implement MATCH...AGAINST queries (FULLTEXT index already added)
  - [ ] Add relevance scoring
  - [ ] Create search result ranking

- [ ] **Improve fuzzy matching**
  - [ ] Enhance soundex search
  - [ ] Add Levenshtein distance calculation
  - [ ] Implement metaphone algorithm
  - [ ] Create "did you mean?" suggestions

- [ ] **Search performance**
  - [ ] Add search result caching
  - [ ] Implement pagination for large result sets
  - [ ] Add search query logging
  - [ ] Create popular searches cache

### Category System Design
- [ ] **Database schema changes**
  - [ ] Create help_categories table
  - [ ] Add category_id to help_entries
  - [ ] Create category hierarchy support
  - [ ] Add category permissions

- [ ] **Category implementation**
  - [ ] Create category CRUD operations
  - [ ] Add category browsing commands
  - [ ] Implement category-based help listing
  - [ ] Add default categories (Commands, Spells, Skills, etc.)

- [ ] **Editor updates**
  - [ ] Add category selection to hedit
  - [ ] Create category management interface
  - [ ] Add bulk categorization tools
  - [ ] Implement category templates

## FUTURE TASK - File to Database Migration -- MUST BE LAST

### Migrate help.hlp to Database
- [ ] **Create migration script for help.hlp** (Required before disabling file loading)
  - [ ] Parse help.hlp file format (entries separated by #, keywords on first line)
  - [ ] Extract all help entries with keywords and min_level
  - [ ] Check for existing entries in database to avoid duplicates
  - [ ] Insert missing entries into help_entries table
  - [ ] Insert associated keywords into help_keywords table
  - [ ] Verify migration completeness (compare file entry count with DB)
  - [ ] Create backup of existing database entries before migration
  - [ ] Test dual-mode operation (both file and DB active)
  - [ ] Once verified, disable file loading in load_help()
  - [ ] Remove help_table allocation and sorting
  - [ ] Update all commands to use database exclusively
  - [ ] Document the migration process and rollback procedures

### System Unification -- MUST BE DONE LAST (After Database Migration)
- [ ] **Migrate from file-based to database-only system** 
  - **Status:** Previously attempted but REVERTED - system not ready for migration
  - **Prerequisites:** Complete "Migrate help.hlp to Database" task above first
  - [x] Identified all file-based help loading code (research complete)
  - [ ] Import all help.hlp entries to database (blocked by migration script)
  - [ ] Verify all help entries are in database before disabling file loading
  - [ ] Modify `load_help()` function in db.c to skip file processing
  - [ ] Mark `help_index_element` structure as deprecated (retain for compatibility)
  - [ ] Mark `help_table` global variable as deprecated (retain for compatibility)
  - [ ] Update boot sequence to skip file-based help loading
  - [ ] Disable help.hlp file processing (currently still loading from file)
  - [x] Added comprehensive comments documenting dual-mode operation