# MySQL Implementation Audit Plan

**Created:** December 2025  
**Purpose:** Comprehensive audit and improvement plan for src/mysql.c and src/mysql.h  
**Priority:** CRITICAL - Blocking help system functionality

## Executive Summary

The MySQL implementation in LuminariMUD has critical issues with prepared statements returning 0 rows despite data existing in the database. Standalone test programs prove the queries and data are correct, but the game's implementation fails. This document outlines a systematic audit and remediation plan.

## Current State Assessment

### Files Under Review
- **src/mysql.c** (2,269 lines) - Main MySQL implementation
- **src/mysql.h** (120 lines) - Header with function declarations
- **src/mysql/** - MySQL client library headers (appears to be MySQL 5.x era)

### Known Issues

#### 1. CRITICAL: Prepared Statements Returning 0 Rows
- **Symptom:** `mysql_stmt_store_result()` returns 0 rows for valid queries
- **Test Results:** 
  - Standalone C programs execute identical queries successfully
  - Python scripts confirm data exists
  - Game code consistently returns 0 rows
- **Impact:** Help system completely broken for database lookups

#### 2. Code Quality Issues
- Mixed error handling strategies (exit vs return)
- Inconsistent mutex usage patterns
- Memory management concerns with prepared statements
- Debug flag hardcoded to 1 (line 21)

#### 3. MySQL Version Compatibility
- Using older MySQL C API patterns
- Headers in src/mysql/ appear to be MySQL 5.x vintage
- May have compatibility issues with modern MariaDB

## Audit Plan

### Phase 1: Immediate Critical Fix (Day 1)

#### Task 1.1: Debug Prepared Statement Execution Flow
```c
// Add comprehensive logging at each step:
1. After mysql_stmt_prepare() - log query and param count
2. After parameter binding - log each parameter value and type
3. After mysql_stmt_bind_param() - verify binding success
4. After mysql_stmt_execute() - check error state
5. After mysql_stmt_store_result() - log row count
6. After mysql_stmt_bind_result() - verify result binding
```

#### Task 1.2: Compare Working vs Non-Working Code
- [ ] Create side-by-side comparison of test_exact_replication.c vs mysql.c
- [ ] Identify ANY differences in:
  - Statement initialization
  - Parameter binding order
  - Result buffer allocation
  - Error checking
  - Mutex locking patterns

#### Task 1.3: Test Mutex Interference
- [ ] Temporarily disable mutex locking in mysql_stmt_execute_prepared()
- [ ] Test if mutexes are causing statement corruption
- [ ] Review pthread mutex initialization

### Phase 2: Systematic Code Review (Day 2-3)

#### Task 2.1: Prepared Statement Lifecycle Audit
```c
// Review entire lifecycle:
mysql_stmt_create()
  -> mysql_stmt_prepare_query()
    -> mysql_stmt_bind_param_*()
      -> mysql_stmt_execute_prepared()
        -> mysql_stmt_fetch_row()
          -> mysql_stmt_get_*()
            -> mysql_stmt_cleanup()
```

#### Task 2.2: Memory Management Review
- [ ] Audit all CREATE() macro usage
- [ ] Verify all free() calls have corresponding allocations
- [ ] Check for memory leaks in error paths
- [ ] Review MYSQL_BIND structure initialization

#### Task 2.3: Thread Safety Analysis
- [ ] Verify mutex initialization in connect_to_mysql()
- [ ] Check for deadlock possibilities
- [ ] Review lock/unlock pairs for consistency
- [ ] Analyze connection pooling (conn, conn2, conn3)

### Phase 3: MySQL Library Upgrade Assessment (Day 4-5)

#### Task 3.1: Version Compatibility Check
```bash
# Check system MySQL version
mysql_config --version

# Compare with headers in src/mysql/
grep VERSION src/mysql/mysql_version.h

# Check for deprecated functions
grep -r "mysql_" src/mysql.c | grep -v "//"
```

#### Task 3.2: Modern MySQL/MariaDB Migration Path
- [ ] Evaluate upgrading to MySQL Connector/C 8.0+
- [ ] Consider MariaDB Connector/C for better compatibility
- [ ] Document API changes required
- [ ] Test with system MySQL libraries instead of bundled

### Phase 4: Refactoring Recommendations (Week 2)

#### Task 4.1: Create Abstraction Layer
```c
// Proposed structure:
typedef struct db_connection {
    MYSQL *handle;
    pthread_mutex_t mutex;
    char name[32];
    bool in_use;
    time_t last_ping;
} db_connection_t;

typedef struct db_pool {
    db_connection_t *connections;
    int size;
    int max_size;
} db_pool_t;
```

#### Task 4.2: Implement Connection Pool Manager
- [ ] Replace hardcoded conn/conn2/conn3 with pool
- [ ] Add automatic reconnection on failure
- [ ] Implement connection health checks
- [ ] Add connection usage statistics

#### Task 4.3: Standardize Error Handling
```c
// Proposed error handling:
typedef enum {
    DB_SUCCESS = 0,
    DB_ERROR_CONNECTION = -1,
    DB_ERROR_QUERY = -2,
    DB_ERROR_PARAMS = -3,
    DB_ERROR_MEMORY = -4
} db_error_t;

db_error_t db_execute_query(const char *query, db_result_t **result);
```

## Testing Strategy

### Unit Tests Required
1. **test_prepared_stmt_basic** - Simple SELECT with parameters
2. **test_prepared_stmt_concurrent** - Multiple simultaneous queries
3. **test_prepared_stmt_large_result** - GROUP_CONCAT with 64KB results
4. **test_connection_recovery** - Connection loss and recovery
5. **test_mutex_deadlock** - Stress test mutex handling

### Integration Tests
1. Help system database lookups
2. Region/wilderness queries under load
3. Concurrent player data access
4. Transaction rollback scenarios

## Implementation Priority

### IMMEDIATE (Blocking Production)
1. Fix prepared statement 0-row issue
2. Restore help system functionality

### HIGH (This Week)
1. Remove hardcoded debug flag
2. Fix memory leaks in error paths
3. Standardize error handling

### MEDIUM (Next Sprint)
1. Upgrade MySQL client library
2. Implement connection pooling
3. Add comprehensive logging

### LOW (Future)
1. Refactor to use ORM pattern
2. Add query performance monitoring
3. Implement query caching layer

## Risk Assessment

### Critical Risks
- **Data Loss:** Prepared statement failures could affect player saves
- **Security:** SQL injection if falling back to non-prepared statements
- **Stability:** Memory leaks under high connection load

### Mitigation Strategies
1. Extensive testing before production deployment
2. Gradual rollout with monitoring
3. Rollback plan with previous mysql.c backup
4. Database backup before any changes

## Success Metrics

1. **Functional:** Help system returns correct results from database
2. **Performance:** Query response time < 50ms for help lookups
3. **Reliability:** Zero failed queries over 24-hour period
4. **Memory:** No memory growth over 7-day uptime

## Next Steps

1. **Today:** Add detailed logging to trace prepared statement issue
2. **Tomorrow:** Compare working test code with game implementation
3. **This Week:** Fix critical bug and deploy to test server
4. **Next Week:** Begin refactoring based on audit findings

## Appendix: Debug Commands

```c
// Add these debug functions to mysql.c for testing:

void debug_prepared_stmt(PREPARED_STMT *pstmt) {
    log("=== PREPARED STATEMENT DEBUG ===");
    log("  stmt: %p", pstmt->stmt);
    log("  connection: %p", pstmt->connection);
    log("  param_count: %d", pstmt->param_count);
    log("  result_count: %d", pstmt->result_count);
    log("  metadata: %p", pstmt->metadata);
    
    if (pstmt->stmt) {
        log("  stmt_errno: %u", mysql_stmt_errno(pstmt->stmt));
        log("  stmt_error: %s", mysql_stmt_error(pstmt->stmt));
        log("  stmt_sqlstate: %s", mysql_stmt_sqlstate(pstmt->stmt));
        log("  stmt_field_count: %u", mysql_stmt_field_count(pstmt->stmt));
        log("  stmt_param_count: %lu", mysql_stmt_param_count(pstmt->stmt));
        log("  stmt_num_rows: %llu", mysql_stmt_num_rows(pstmt->stmt));
    }
}

void test_direct_query(const char *query) {
    MYSQL_RES *result;
    MYSQL_ROW row;
    unsigned int num_fields;
    
    log("=== DIRECT QUERY TEST ===");
    log("Query: %s", query);
    
    if (mysql_query(conn, query)) {
        log("ERROR: %s", mysql_error(conn));
        return;
    }
    
    result = mysql_store_result(conn);
    if (!result) {
        log("No result set");
        return;
    }
    
    num_fields = mysql_num_fields(result);
    log("Fields: %u, Rows: %llu", num_fields, mysql_num_rows(result));
    
    while ((row = mysql_fetch_row(result))) {
        for (unsigned int i = 0; i < num_fields; i++) {
            log("  [%u]: %s", i, row[i] ? row[i] : "NULL");
        }
    }
    
    mysql_free_result(result);
}
```

## Conclusion

The MySQL implementation requires immediate attention to resolve the prepared statement issue blocking the help system. This audit plan provides a systematic approach to identify and fix the root cause while also addressing broader code quality concerns. The phased approach ensures critical functionality is restored quickly while laying groundwork for long-term improvements.