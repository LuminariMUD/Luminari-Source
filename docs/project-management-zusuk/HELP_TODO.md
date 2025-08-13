# Help System TODO

## Status: All Tasks Completed
**Last Updated:** 2025-08-13

All help system improvements have been implemented and documented in CHANGELOG.md

---

# Helpful Database Connection & Structure Information

## Connection Details
- **Config File:** `lib/mysql_config`
- **Connection:** Managed through `src/mysql.c` using prepared statements
- **Status:** ✅ FUNCTIONAL - Database connection verified and operational

## Database Schema
```sql
help_entries (1,815+ rows)
├── tag         VARCHAR(50) PRIMARY KEY  -- Unique identifier (e.g., 'score')
├── entry       TEXT                     -- The actual help content
├── min_level   INT                      -- Minimum level to view (0-60)
└── last_updated TIMESTAMP               -- When entry was last modified

help_keywords (3,143+ rows)
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
- **`src/help.c`** (1296 lines) - Main help system implementation, search functions, display logic
- **`src/help.h`** (89 lines) - Help system data structures and function declarations
- **`src/hedit.c`** (2000+ lines) - OLC help editor implementation, database operations, import/export
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

**Note:** No SQL schema files found in repository - tables are created programmatically in db_init.c

## Supporting Systems
- **`src/utils.c/h`** - Utility functions (string handling, memory management)
- **`src/structs.h`** - Core data structures
- **`src/conf.h`** - Configuration constants
- **`src/sysdep.h`** - System dependencies

---

## Overview
