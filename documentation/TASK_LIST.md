# TASK LIST - Critical Memory Issues from Valgrind Analysis

## Summary
Valgrind detected **8.77MB** definitely lost, **32MB** indirectly lost, and **11,448 errors** from 18 contexts. This represents critical memory management issues that need immediate attention.

### previous tasks deleted, CHANGELOG.md has entries for them

### 3. Character Follower Cleanup - FIX ATTEMPTED (2025-07-26)
- **File**: utils.c, db.c
- **Lines**: stop_follower (line 3124), destroy_db (line 753)
- **Issue**: Accessing follower data after character freed
- **Fix Attempted**: 
  - Modified destroy_db() to use two-pass cleanup
  - First pass clears all master/follower relationships without messages
  - Second pass frees all characters after relationships cleared
- **Status**: Needs testing during game shutdown

## Priority 4 - String Operation Issues

### 1. Overlapping strcpy - FIX ATTEMPTED (2025-07-26)
- **File**: handler.c
- **Line**: find_all_dots (line 3098, not 3072 as listed)
- **Issue**: Source and destination overlap in strcpy
- **Fix Attempted**: 
  - Replaced strcpy(arg, arg + 4) with memmove(arg, arg + 4, strlen(arg + 4) + 1)
  - memmove correctly handles overlapping memory regions
- **Status**: Should resolve undefined behavior warnings from valgrind

## Priority 5 - Performance and Monitoring

### 1. Performance Monitoring Overhead
- **Issue**: PERFMON showing extreme CPU spikes (up to 1677%)
- **Fix**: Review performance monitoring implementation for efficiency

### 2. Zone Reset Performance
- **Issue**: Potential O(n²) algorithms in zone reset
- **Fix**: Optimize zone reset algorithms as per recent changes
