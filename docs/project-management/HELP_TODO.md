# Help System TODO and Implementation Checklist

**Created:** August 12, 2025  
**Priority Levels:** CRITICAL | HIGH | MEDIUM | LOW  
**Status:**  Not Started | = In Progress |  Complete | L Blocked

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
- [ ] **Implement prepared statements** (Future enhancement)
  - [ ] Create prepared statement wrapper functions
  - [ ] Convert all INSERT queries
  - [ ] Convert all UPDATE queries
  - [ ] Convert all DELETE queries
  - [ ] Convert all SELECT queries
  - [ ] Add parameter binding for all user inputs

### System Unification
- [x] **Remove legacy file system** (Completed August 12, 2025)
  - [x] Identified all file-based help loading code
  - [x] Modified `load_help()` function in db.c to skip file processing
  - [x] Marked `help_index_element` structure as deprecated (retained for compatibility)
  - [x] Marked `help_table` global variable as deprecated (retained for compatibility)
  - [x] Updated boot sequence to skip file-based help loading
  - [x] Help.hlp file processing now bypassed entirely
  - [x] Added comprehensive comments documenting the transition to database-driven help

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

### Versioning System
- [ ] **Database changes**
  - [ ] Create help_history table
  - [ ] Add version tracking columns
  - [ ] Create audit triggers
  - [ ] Add change tracking

- [ ] **Version control features**
  - [ ] Implement revision history viewing
  - [ ] Add rollback functionality
  - [ ] Create diff viewer
  - [ ] Add change approval workflow

## LOW PRIORITY - Future Enhancements (Nice to have)

### Interface Modernization
- [ ] **Markdown support**
  - [ ] Add markdown parser
  - [ ] Convert existing help to markdown
  - [ ] Update editor for markdown
  - [ ] Add preview functionality

- [ ] **Rich formatting**
  - [ ] Enhanced color support
  - [ ] Table formatting
  - [ ] List formatting
  - [ ] Code block support

- [ ] **Web interface**
  - [ ] Create REST API for help
  - [ ] Build web-based help browser
  - [ ] Add search interface
  - [ ] Implement help editor web UI

### Performance Optimization
- [ ] **Caching layer**
  - [ ] Implement Redis/Memcached support
  - [ ] Add query result caching
  - [ ] Create help entry cache
  - [ ] Add cache invalidation

- [ ] **String optimization**
  - [ ] Implement string interning
  - [ ] Add compression for help text
  - [ ] Optimize memory allocation
  - [ ] Reduce string duplication

### Advanced Features
- [ ] **Multi-language support**
  - [ ] Add language column to tables
  - [ ] Implement translation interface
  - [ ] Create language switching
  - [ ] Add automatic translation

- [ ] **Help analytics**
  - [ ] Track help usage statistics
  - [ ] Log search queries
  - [ ] Identify missing help topics
  - [ ] Generate usage reports

- [ ] **Interactive tutorials**
  - [ ] Create tutorial framework
  - [ ] Add step-by-step guides
  - [ ] Implement progress tracking
  - [ ] Add achievement system

## Testing Checklist

### Security Testing
- [ ] **SQL Injection Tests**
  - [ ] Test with single quotes in all inputs
  - [ ] Test with SQL keywords (DROP, DELETE, etc.)
  - [ ] Test with escape sequences
  - [ ] Test with Unicode characters
  - [ ] Use sqlmap tool for comprehensive testing

- [ ] **Input Validation Tests**
  - [ ] Test maximum length boundaries
  - [ ] Test minimum length requirements
  - [ ] Test invalid characters
  - [ ] Test null/empty inputs
  - [ ] Test numeric range boundaries

- [ ] **Memory Testing**
  - [ ] Run Valgrind memcheck
  - [ ] Test with AddressSanitizer
  - [ ] Check for memory leaks
  - [ ] Test buffer overflow conditions
  - [ ] Stress test with large datasets

### Performance Testing
- [ ] **Load Testing**
  - [ ] Test with 10,000+ help entries
  - [ ] Concurrent user simulation
  - [ ] Search performance benchmarking
  - [ ] Database query profiling
  - [ ] Memory usage monitoring

- [ ] **Query Performance**
  - [ ] EXPLAIN all queries
  - [ ] Identify slow queries
  - [ ] Test with large result sets
  - [ ] Monitor query cache hit rates
  - [ ] Check index usage

### Functional Testing
- [ ] **Editor Testing**
  - [ ] Create new help entries
  - [ ] Edit existing entries
  - [ ] Delete entries
  - [ ] Keyword management
  - [ ] Permission testing

- [ ] **Search Testing**
  - [ ] Exact match searches
  - [ ] Partial match searches
  - [ ] Fuzzy searches
  - [ ] Case sensitivity
  - [ ] Special character handling

- [ ] **Command Testing**
  - [ ] help command variations
  - [ ] hindex command
  - [ ] helpcheck command
  - [ ] hedit command
  - [ ] Permission levels

## Implementation Schedule

### Week 1: System Unification
- Days 1-3: Migration planning and backup
- Days 4-5: Legacy system removal
- Days 6-7: Integration testing

### Week 2: Query Result Caching
- Days 1-2: Implement caching layer
- Days 3-4: Add connection pooling
- Days 5-7: Performance testing

### Weeks 5-8: Enhancement Features
- Category system implementation
- Search improvements
- Versioning system
- Documentation updates

## Risk Mitigation

### Before Any Changes
1. [ ] Create full system backup
2. [ ] Document current functionality
3. [ ] Create rollback procedures
4. [ ] Test in development environment
5. [ ] Get approval for production changes

### During Implementation
1. [ ] Use version control for all changes
2. [ ] Create feature branches
3. [ ] Perform code reviews
4. [ ] Write unit tests
5. [ ] Document changes

### After Implementation
1. [ ] Monitor system performance
2. [ ] Check error logs
3. [ ] Gather user feedback
4. [ ] Create post-mortem report
5. [ ] Update documentation

## Success Metrics

### Security Metrics
- [ ] Zero SQL injection vulnerabilities
- [ ] All inputs validated
- [ ] No memory leaks detected
- [ ] Pass security audit

### Performance Metrics
- [ ] Search response time < 100ms
- [ ] Editor operations < 50ms
- [ ] Memory usage stable
- [ ] Database queries optimized

### Quality Metrics
- [ ] 100% help coverage for commands
- [ ] Zero duplicate entries
- [ ] All entries categorized
- [ ] Documentation complete

## Additional Notes from Code Review

1. **Database Schema**: No SQL schema files found in repository. Verify if tables are created programmatically or if schema files are missing.

2. **Dual System Confirmed**: File-based help loaded at boot (db.c:1601 via `load_help()`), while runtime uses MySQL queries.

3. **help_table Usage**: Previously used in `do_helpcheck` and `do_hindex` commands - hindex now converted to MySQL (August 12, 2025)

## Notes and Considerations

1. **Backward Compatibility**: Ensure all changes maintain compatibility with existing game commands and scripts.

2. **Data Migration**: The file-to-database migration must be thoroughly tested to prevent data loss.

3. **Performance Impact**: Monitor system performance during and after implementation to ensure no degradation.

4. **User Communication**: Notify users of any changes that affect the help system interface.

5. **Emergency Rollback**: Maintain ability to quickly revert to previous system if critical issues arise.

---
*This TODO list should be updated regularly as tasks are completed and new issues are discovered.*