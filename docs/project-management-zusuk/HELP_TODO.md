# Help System TODO and Implementation Checklist

**Created:** January 2025  
**Last Updated:** January 2025  
**Priority Levels:** CRITICAL | HIGH | MEDIUM | LOW  
**Status:** ⬜ Not Started | 🔄 In Progress | ✅ Complete | 🔒 Blocked

---

# Database Connection & Structure

## Connection Details
- **Config File:** `lib/mysql_config`
- **Connection:** Managed through `src/mysql.c` using prepared statements

## Database Schema
```sql
help_entries (1,815 rows)
├── tag           VARCHAR(50) PRIMARY KEY  -- Unique identifier (e.g., 'score')
├── entry         TEXT                     -- The actual help content
├── min_level     INT                      -- Minimum level to view (0-60)
├── auto_generated BOOLEAN DEFAULT FALSE   -- TRUE if auto-generated, FALSE if manual
└── last_updated  TIMESTAMP               -- When entry was last modified

help_keywords (3,143 rows)
├── help_tag    VARCHAR(50) FK           -- Links to help_entries.tag
└── keyword     VARCHAR(50)              -- Search keyword (e.g., 'Score', 'character-info')

-- Indexes: help_tag, keyword, auto_generated for performance
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
- **`src/hedit.c`** (1500+ lines) - OLC help editor, database operations, auto-generation system
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
- **`help_entries`** - Main help content table (~1,622 entries after cleanup)
- **`help_keywords`** - Keyword-to-help mappings (~2,950 keywords after cleanup)
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

## AUTOMATIC HELP GENERATION SYSTEM - NEW FEATURE

### Overview
Create a system to automatically generate basic help files for commands that lack documentation, leveraging command registration data and optionally enhanced with source code metadata.

### Implementation Status - January 2025

#### Phase 1: Enhanced Auto-Generation (HIGH PRIORITY) - ✅ COMPLETE
- [x] **Create helpgen command infrastructure**
  - [x] Add ACMD(do_helpgen) to hedit.c (lines 1191-1287)
  - [x] Add declaration to hedit.h (line 29)
  - [x] Register helpgen command in interpreter.c (line 484)
  - [x] Add command options: missing, all, specific command, force flag
  
- [x] **Implement enhanced help entry generator**
  - [x] Create generate_help_entry() function with context awareness
  - [x] Extract command info (name, level, position, subcmd, action type)
  - [x] Generate contextual descriptions based on command category
  - [x] Use command names as tags directly (no CMD_ prefix)
  - [x] Handle special cases (socials, directions, aliases)
  - [x] Create rich keyword sets for better searchability
  
- [x] **Database integration**
  - [x] Add auto_generated column to help_entries table - **COMPLETED**
  - [x] Create conflict resolution for existing entries (checks & force flag)
  - [x] Implement database insert/update operations
  - [x] Add list/clean commands for managing auto-generated entries
  - [ ] Add generation logging/audit trail - **FUTURE ENHANCEMENT**
  
- [x] **Position and level formatters**
  - [x] Create get_position_name() function (lines 1012-1026)
  - [x] Create get_level_name() function (lines 1029-1050)
  - [x] Format subcommand information
  - [x] Handle special command flags

#### Phase 2: Enhanced Metadata System (MEDIUM PRIORITY)
- [ ] **Design metadata structure**
  - [ ] Add command_metadata struct to interpreter.h
  - [ ] Create metadata array parallel to cmd_info[]
  - [ ] Define metadata fields (brief_desc, syntax, category, see_also)
  
- [ ] **Manual metadata entries**
  - [ ] Add metadata for 50 most common commands
  - [ ] Create categories (combat, social, movement, info, admin)
  - [ ] Define standard syntax patterns
  - [ ] Build see_also relationships
  
- [ ] **Source code annotations (HELP_META)**
  - [ ] Define HELP_META comment format
  - [ ] Add annotations to key ACMD functions
  - [ ] Create examples for each command type
  - [ ] Document annotation guidelines

#### Phase 3: Context Analysis (LOW PRIORITY)
- [ ] **Command pattern analyzer**
  - [ ] Detect argument requirements
  - [ ] Identify subcommand patterns
  - [ ] Classify command types (movement, combat, etc.)
  - [ ] Extract affected stats/attributes
  
- [ ] **Related command discovery**
  - [ ] Find commands with similar names
  - [ ] Group commands by function pointer
  - [ ] Link subcommand variants
  - [ ] Build command families
  
- [ ] **Smart help generation**
  - [ ] Combine metadata with analysis
  - [ ] Generate contextual descriptions
  - [ ] Create relevant examples
  - [ ] Add appropriate warnings/notes

### Usage Examples
```
helpgen missing        - Generate help for all commands without entries
helpgen all           - Generate help for all commands (skip existing)
helpgen all force     - Regenerate all help entries (overwrite)
helpgen score         - Generate help for specific command
helpgen list          - List all auto-generated help entries
helpgen clean force   - Delete all auto-generated help entries
helpcheck            - List commands still missing help (existing)
```

### Generated Help Template (Enhanced)
```
COMMAND_NAME

Category: [Magic/Combat/Movement/etc]
Usage: command [arguments]

[Contextual description based on command category and properties]

Requirements:
  Minimum Level: [level or role]
  Position: [standing/sitting/etc]
  Action Type: [swift/standard/move/none]

[Examples with real usage patterns]

[Notes about special behavior]

---
This help entry was automatically generated.
For more detailed information, use 'help command manual' or ask a staff member.
```

### Benefits
- Ensures 100% command documentation coverage
- Consistent help format across all commands
- Reduces manual documentation burden
- Easy regeneration when commands change
- Foundation for richer documentation

### Implementation Notes (Updated January 2025)
**Files Modified:**
- `src/hedit.c` - Enhanced helpgen implementation (500+ lines added)
  - Category detection system
  - Context-aware description generation
  - Rich keyword generation for searchability
  - List/clean commands for management
- `src/hedit.h` - Added ACMD declaration (line 29)
- `src/interpreter.c` - Added command registration (line 484)
- `src/db_init.c` - Added auto_generated column to schema
- `src/db_init_data.c` - Updated verification functions
- `src/db_startup_init.c` - Added startup checks

**Key Functions Added:**
1. `get_position_name()` - Converts position constants to readable strings
2. `get_level_name()` - Converts level constants to readable strings  
3. `get_command_category()` - Categorizes commands by type
4. `get_action_type_desc()` - Describes action requirements
5. `generate_command_description()` - Creates contextual descriptions
6. `generate_usage_examples()` - Provides command-specific examples
7. `generate_help_entry()` - Core generation with enhanced content
8. `do_helpgen()` - Main handler with list/clean/generate modes

**Database Enhancements:**
- Added `auto_generated` BOOLEAN column to help_entries table
- Tags now use command names directly (not CMD_ prefix)
- Rich keyword sets including category, related terms
- Indexed auto_generated column for fast queries
- Migration support for existing databases

**Bug Fixes Applied (January 2025):**
- Fixed `LVL_GRGOD` to `LVL_GRSTAFF` (correct constant name)
- Removed unused `MYSQL_ROW row` variable
- Updated `two_arguments()` call with proper size parameters
- Fixed database column names (`last_updated` instead of `updated_at`)
- Removed non-existent `max_level` column from INSERT
- Fixed function pointer references (`do_gen_cast` not `do_cast`, `do_write` not `do_gen_write`)
- Increased buffer size to prevent string truncation warnings
- Cleaned up 193 old CMD_ prefixed auto-generated entries from database

### Testing Checklist
- [ ] Test helpgen with no arguments (shows usage)
- [ ] Test helpgen missing (generates only missing)
- [ ] Test helpgen all (respects existing entries)
- [ ] Test helpgen all force (overwrites existing)
- [ ] Test helpgen <command> for specific command
- [ ] Verify auto_generated flag is set correctly
- [ ] Confirm help entries are searchable
- [ ] Check generated content quality
- [ ] Test with different permission levels
- [ ] Verify no memory leaks or crashes

### Next Steps to Complete
1. **COMPLETE**: ✅ Phase 1 implementation finished and compiled successfully
2. **COMPLETE**: ✅ Database schema updated with auto_generated column
3. **COMPLETE**: ✅ Cleaned up 193 old auto-generated entries from database
4. **READY FOR TESTING**: Test the enhanced helpgen system:
   - Run `helpgen` with no arguments (shows enhanced usage)
   - Run `helpgen missing` to generate contextual help for missing commands
   - Run `helpgen all` to generate for all commands (skip existing)
   - Run `helpgen all force` to regenerate all with enhanced content
   - Run `helpgen activate` to see example of context-aware generation
   - Run `helpgen list` to view all auto-generated entries  
   - Run `helpgen clean force` to remove auto-generated entries
5. **VERIFY**: Use `helpcheck` to confirm documentation coverage
6. **PHASE 2**: Begin metadata system implementation for richer content

### Database Cleanup Summary
- **Before**: 1,815 help entries with 193 poorly formatted auto-generated entries
- **After**: 1,622 help entries (all manual or ready for new generation)
- **Removed**: All CMD_ prefixed entries with generic descriptions
- **Ready**: Clean database with auto_generated tracking column