# Luminari MUD Changelog

Assembled by Zusuk

## 2025-08-13 - Help System Critical Issues Fixed - Part 3

### Memory Management Consolidation
- **Created centralized memory management functions** to eliminate code duplication:
  - `alloc_help_entry()` - Consistent initialization of help entries
  - `alloc_help_keyword()` - Consistent initialization of keywords
  - `free_help_entry_single()` - Single entry cleanup
  - `free_help_keyword_list()` - Keyword list cleanup
  - `free_help_entry_list()` - Complete list cleanup
- **Eliminated duplicate cleanup code** throughout help.c
- All memory operations now go through centralized functions
- Ensures consistent NULL checks and proper cleanup on all paths

### Database Migration System
- **Added schema_migrations table** to track database version changes
- Implemented `apply_migration()` function for applying database updates
- Created `run_database_migrations()` to automatically apply pending migrations
- **Initial migrations include**:
  - Migration 1: help_versions table for version history
  - Migration 2: help_search_history table for analytics
  - Migration 3: category support for help entries
  - Migration 4: help_related_topics table for linking
- System automatically detects and applies only unapplied migrations
- Prevents duplicate migrations and maintains database consistency

### Search Analytics Implementation
- **Added automatic search tracking** to all help search types:
  - Keyword searches logged with results count
  - Full-text searches logged with results count
  - Failed searches tracked for improvement insights
- Data stored in help_search_history table with:
  - Search term, searcher info, result count
  - Search type (keyword/fulltext/soundex)
  - Timestamp for temporal analysis
- Non-critical logging - failures don't affect search functionality

### Category & Related Topics Support
- **Added category field** to help_entries table
- Categories enable browsing help by topic areas
- **Created help_related_topics table** for linking related help entries
- Relevance scoring for intelligent suggestions
- Indexed for fast retrieval of related content

### Technical Details
- All changes maintain ANSI C90 compatibility
- Memory management improvements reduce leak potential
- Migration system ensures smooth database updates
- Analytics provide insights for help system improvements
- Affected files: `src/help.c`, `src/help.h`, `src/db_init.c`

## 2025-08-13 - Help System Major Improvements

### Architecture Improvements
- **Eliminated global state variables** by introducing a context structure for handler communication
- Replaced `g_partial_help_displayed` global with proper context passing between handlers
- Improved handler chain architecture for better maintainability

### Full-Text Search Capability
- **Added new HELPSEARCH command** for searching within help content, not just keywords
- Searches both help entry content and keywords using database queries
- Returns up to 20 most relevant results with previews
- Clickable results for easy navigation to full help text
- Usage: `helpsearch <search term>` (e.g., `helpsearch damage reduction`)

### Content Versioning
- **Added automatic version history** tracking for all help entry changes
- Previous versions are saved to `help_versions` table before updates
- Tracks who made changes and when for audit purposes
- Preserves content history for rollback capabilities

### Code Quality Improvements
- **Replaced magic numbers with named constants**:
  - `MAX_HELP_SEARCH_LENGTH` (200)
  - `MAX_HELP_ENTRY_BUFFER` (4096)
  - `MIN_SEARCH_LENGTH_FOR_STRICT` (2)
  - `MAX_KEYWORD_LENGTH_DIFF` (3)
- **Improved short search handling** with configurable thresholds
- Better search relevance for 1-2 character searches

### Technical Details
- All changes maintain ANSI C90 compatibility
- No breaking changes to existing APIs
- Affected files: `src/help.c`, `src/help.h`, `src/hedit.c`, `src/interpreter.c`

## 2025-08-13 - Help System Critical Issues Fixed - Part 2

### Database Maintenance
- **Added helpgen repair command** to fix database integrity issues
- Automatically detects and removes orphaned keywords (keywords pointing to non-existent help entries)
- Identifies help entries without any keywords and adds the tag as a keyword
- Safe two-phase operation: first shows issues, then fixes with 'force' flag
- Prevents data loss by requiring explicit confirmation

### Enhanced Error Reporting
- **Improved all error messages** to include specific context and MySQL error details
- Database connection failures now clearly indicate the operation that failed
- Query preparation and execution errors include the search term for easier debugging
- Users receive more informative messages when help lookups fail
- All SYSERR logs now include relevant parameters for troubleshooting

### Memory Management Fixes
- **Added NULL checks** for all strdup() calls in help.c
- Proper cleanup on all error paths to prevent memory leaks
- Fixed memory leak risks in search_help_table() function
- Added allocation failure handling in get_help_keywords()
- Fixed memory leak in soundex_search_help_keywords()
- Proper error handling in do_help() for argument allocation

### User Experience Improvements
- Help not found messages now show the exact term searched: "There is no help on 'xyz'"
- Soundex suggestions provide better feedback for partial matches
- Database errors are properly logged while gracefully falling back to file-based help

### Technical Details
- All fixes maintain ANSI C90 compatibility
- No changes to existing APIs or data structures
- Affected files: `src/help.c`, `src/hedit.c`

## 2025-08-13 - Help Editor Critical Issues Fixed

### Transaction Support
- **Added full database transaction support** with START TRANSACTION, COMMIT, and ROLLBACK
- All help entry saves are now atomic - either all operations succeed or all are rolled back
- Prevents partial updates and data corruption during save failures

### Error Handling & User Feedback
- **Improved error reporting** throughout the help editor
- Users now receive clear feedback when operations fail
- Added specific error messages for each type of failure
- Database errors are properly logged and reported to users

### Performance Optimization
- **Optimized keyword updates** to use differential approach
- Only adds new keywords and removes deleted ones instead of recreating all
- Reduces database operations and improves save performance
- Fixed SQL injection vulnerability in keyword fetch using prepared statements

### Code Quality Improvements
- **Reduced code duplication** through validation function consolidation
- All validation logic now in dedicated functions
- Added proper constants for magic numbers (MAX_HELP_TAG_LENGTH, etc.)
- Improved code maintainability and readability

### Technical Notes
- Connection pooling not implemented due to transaction scope requirements
- Future enhancement: Modify pool to support transaction-scoped connections
- All fixes maintain ANSI C90 compatibility
- Affected files: `src/hedit.c`

## 2025-01-13 - Help Editor Security and Stability Fixes

### Security Fixes
- **Fixed SQL injection vulnerabilities** in help editor functions:
  - `hedit_delete_entry()` now uses prepared statements instead of manual SQL escaping
  - `do_hindex()` converted all three queries to use prepared statements
  - Eliminates potential SQL injection attack vectors in help system

### Memory Management Improvements
- **Added NULL checks** for all `strdup()` and `malloc()` calls to prevent crashes under memory pressure
- **Fixed memory leaks** in `generate_help_entry()` by ensuring proper cleanup on all error paths
- **Added keyword limit** (MAX_KEYWORDS_PER_ENTRY = 20) to prevent memory exhaustion attacks

### Functionality Fixes
- **Fixed race condition** in single-editor lock mechanism by atomically acquiring CON_HEDIT state
- Prevents multiple editors from simultaneously editing help files due to race condition window

### Technical Details
- All fixes use existing prepared statement infrastructure
- Maintains backward compatibility
- Follows ANSI C90 standards
- Affected file: `src/hedit.c`

