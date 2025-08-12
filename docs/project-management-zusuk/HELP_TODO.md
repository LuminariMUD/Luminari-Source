# Help System TODO and Implementation Checklist

**Created:** August 12, 2025  
**Priority Levels:** CRITICAL | HIGH | MEDIUM | LOW  
**Status:**  Not Started | = In Progress |  Complete | L Blocked

## ARCHITECTURAL REFACTORING - High Priority (Added January 2025)

### Problem Statement
The current `do_help()` function in help.c has severe architectural issues:
- **Deep Nesting Hell**: 7+ levels of nested if statements (lines 468-549) creating a "pyramid of doom"
- **Mixed Responsibilities**: Single function handling deities, regions, backgrounds, database searches, dynamic generation, and fuzzy matching
- **Inconsistent Logic Order**: Some checks occur before main search, others after
- **Memory Management Risk**: `raw_argument` must be freed in multiple places (error-prone)
- **Poor Extensibility**: Adding new help sources requires modifying core function

### Proposed Solution: Chain of Responsibility Pattern

#### Design Goals
- [ ] **Implement handler registration system**
  - [ ] Create `help_handler` structure with name and check function
  - [ ] Add `register_help_handler()` function for modules to register
  - [ ] Implement `init_help_handlers()` to set up chain during boot
  - [ ] Convert do_help() to iterate through handler chain linearly

#### Refactoring Tasks
- [ ] **Phase 1: Create handler infrastructure** (1-2 days)
  - [ ] Define help_handler structure in help.h
  - [ ] Implement handler registration and management functions
  - [ ] Create handler chain initialization function
  - [ ] Add handler cleanup for shutdown

- [ ] **Phase 2: Extract existing checks to handlers** (2-3 days)
  - [ ] Create `check_database_help()` handler (primary)
  - [ ] Create `check_deity_help()` handler
  - [ ] Create `check_region_help()` handler with capitalization logic
  - [ ] Create `check_background_help()` handler
  - [ ] Create `check_discovery_help()` handler (all 4 alchemy types)
  - [ ] Create `check_feat_help()` handler
  - [ ] Create `check_evolution_help()` handler
  - [ ] Create `check_weapon_help()` handler
  - [ ] Create `check_armor_help()` handler
  - [ ] Create `check_class_help()` handler
  - [ ] Create `check_race_help()` handler
  - [ ] Create `show_soundex_suggestions()` as final fallback

- [ ] **Phase 3: Optimize handler order** (1 day)
  - [ ] Reorder handlers for optimal performance:
    1. Database/File first (most specific, authored content)
    2. Game mechanics (feats, classes, races - generated)
    3. Special cases (deities, regions, backgrounds)
    4. Fuzzy matching last
  - [ ] Document rationale for ordering
  - [ ] Make order configurable if needed

- [ ] **Phase 4: Module integration** (2-3 days)
  - [ ] Move feat handler to feats.c with registration call
  - [ ] Move class handler to class.c with registration call
  - [ ] Move race handler to race.c with registration call
  - [ ] Move alchemy handlers to alchemy.c with registration calls
  - [ ] Move evolution handler to evolutions.c with registration call
  - [ ] Update each module's initialization to register handlers

- [ ] **Phase 5: Cleanup and optimization** (1 day)
  - [ ] Simplify do_help() to ~50 lines max
  - [ ] Centralize memory management (single allocation/free point)
  - [ ] Remove redundant raw_argument duplication
  - [ ] Optimize region capitalization logic
  - [ ] Add performance metrics/logging

#### Expected Benefits
- **Maintainability**: Each handler is self-contained and testable
- **Extensibility**: New systems add handlers without touching core
- **Performance**: Linear search, early exit, optimized ordering
- **Memory Safety**: Centralized allocation/deallocation
- **Code Clarity**: Clear separation of concerns
- **Debugging**: Easy to trace which handler processes a request

#### Success Metrics
- Reduce do_help() from 230+ lines to <50 lines
- Eliminate nested if statements (max 2 levels)
- Zero memory leaks in Valgrind testing
- Sub-millisecond response time for common queries
- Easy addition of new help sources without modifying help.c

### Implementation Notes
- Maintain backward compatibility during refactoring
- Use feature flags to toggle between old/new implementation
- Comprehensive testing of all help paths before switching
- Document the new architecture for future developers

# Files Involved in the Help System (update if out of date)

## Core Implementation Files
- **`src/help.c`** (463 lines) - Main help system implementation, search functions, display logic
- **`src/help.h`** (53 lines) - Help system data structures and function declarations
- **`src/hedit.c`** (662 lines) - OLC help editor implementation, database operations
- **`src/hedit.h`** (31 lines) - Help editor declarations and command definitions

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

### Database Query Security
- [x] **SQL Injection Prevention** (Completed August 12, 2025)
  - [x] Fixed SQL injection vulnerability in help.c soundex_search_help_keywords function
  - [x] Verified hedit.c already has comprehensive input validation and escaping
  - [x] All user inputs now properly escaped using mysql_real_escape_string or mysql_escape_string_alloc
- [x] **Implement prepared statements** (Completed December 12, 2024)
  - [x] Create prepared statement wrapper functions in mysql.c
  - [x] Convert all INSERT queries in hedit.c to prepared statements
  - [x] Convert all UPDATE queries (UPSERT) in hedit.c to prepared statements
  - [x] Convert all DELETE queries in hedit.c to prepared statements
  - [x] Convert all SELECT queries in help.c to prepared statements
  - [x] Add parameter binding for all user inputs - completely SQL injection safe

### System Unification  
- [ ] **Migrate from file-based to database-only system** (RESTORED - Not ready for migration)
  - [x] Identified all file-based help loading code
  - [ ] Import all help.hlp entries to database (PENDING - need migration script)
  - [ ] Verify all help entries are in database before disabling file loading
  - [ ] Modified `load_help()` function in db.c to skip file processing (REVERTED)
  - [ ] Mark `help_index_element` structure as deprecated (retain for compatibility)
  - [ ] Mark `help_table` global variable as deprecated (retain for compatibility)
  - [ ] Update boot sequence to skip file-based help loading (REVERTED)
  - [ ] Help.hlp file processing bypass (REVERTED - still loading from file)
  - [x] Added comprehensive comments documenting dual-mode operation

### Query Optimization
- [x] **Query result caching** (Completed August 12, 2025)
  - [x] Implemented in-memory cache for help search results
  - [x] Cache stores up to 50 entries with 5-minute TTL
  - [x] Automatic cache expiration and LRU eviction
  - [x] Deep copy mechanism to prevent memory corruption
  - [ ] Implement connection pooling (future enhancement)

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

## FUTURE TASK - File to Database Migration

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
