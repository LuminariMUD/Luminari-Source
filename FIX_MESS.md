# CRITICAL: wilderness_kb.c is corrupted with orphaned code

## DO NOT USE MULTI-EDIT. DO NOT WRITE SCRIPTS. DO NOT USE ANY BULLSHIT SHORTCUTS.
## GO THROUGH THE FUCKING CODE CAREFULLY AND FIX EACH ISSUE ONE BY ONE.
## TRACE THROUGH THE CODE TO UNDERSTAND WHAT'S BROKEN BEFORE FIXING.
## YOU ARE A FUCKING RETARD WHO BROKE THIS - NOW FIX IT PROPERLY.

## What happened
The AI assistant fucked up while trying to remove unreachable code from two functions. Failed edits left massive amounts of orphaned code fragments scattered throughout the file.

## The actual problems - UPDATED AFTER MY FUCKUP

### Problem 1: NO UNREACHABLE CODE EXISTS
- **Line 1049**: `analyze_spatial_relationships()` ENDS CLEANLY - NO PROBLEM HERE
- **Line 1588**: `construct_path_network_graph()` ENDS CLEANLY - NO PROBLEM HERE

### Problem 2: I FUCKED UP generate_ascii_visualization() 
When I tried to "fix" things, I MADE IT WORSE. The function at line 1646 is now CORRUPTED:
- **Line 1673-1677**: Should have map printing code but now has BROKEN CODE  
- **Line 1678-1689**: Has orphaned node_type switch statement that DOESN'T BELONG (this is from network graph code)
- **Lines 1691-1842**: ALL OF THIS IS ORPHANED CODE from construct_path_network_graph() that got mixed in
  - Network hubs calculation
  - Bottleneck detection  
  - Path connectivity matrix
  - This goes all the way until line 1842 where it says `report_progress("Path network graph construction", 100);`

The broken code starts here:
```c
/* Map content */
for (y = 0; y < 64; y++) {
    fprintf(fp, "|");
    for (x = 0; x < 64; x++) {
    switch (current->node_type) {  // THIS IS WRONG - current doesn't exist!
```

This entire section from line 1678 to 1842 is ORPHANED CODE that needs to be removed.

### Problem 3: Duplicate functions still exist
- There are TWO `write_query_reference()` functions (lines 1593 and 1843)
- There are TWO `generate_ascii_visualization()` functions (lines 1646 and 2000)
- The SECOND one at line 2000 is the GOOD one
- The FIRST one at line 1646 is BROKEN

## How to fix

1. ~~Delete ALL code after `return;` in `analyze_spatial_relationships()` (line 1052) up to the closing brace~~ NO PROBLEM HERE
2. ~~Delete ALL code after `return;` in `construct_path_network_graph()` (line 1588) up to the closing brace~~ NO PROBLEM HERE  
3. **DELETE the entire FIRST `write_query_reference()` function** (lines 1593-1643) 
4. **DELETE the entire FIRST `generate_ascii_visualization()` function** (lines 1646-1842) - it's completely broken with orphaned network graph code
5. **KEEP the SECOND versions** of both functions:
   - `write_query_reference()` at line 1847
   - `generate_ascii_visualization()` at line 2000

## What NOT to touch
All the other fixes in the file are GOOD:
- Memory allocation fixes using CREATE()
- WILD_DEBUG system
- External variable fixes (WATERLINE, etc.)
- The rest of the knowledge base generation code

## Compilation errors currently happening
```
src/wilderness_kb.c:1678: error: 'current' undeclared (first use in this function)
src/wilderness_kb.c:1691: error: 'intersections' undeclared
src/wilderness_kb.c:1692: error: 'waypoints' undeclared  
src/wilderness_kb.c:1693: error: 'endpoints' undeclared
src/wilderness_kb.c:1703: error: 'i' undeclared
src/wilderness_kb.c:1703: error: 'node_count' undeclared
src/wilderness_kb.c:1707: error: 'nodes' undeclared
src/wilderness_kb.c:1760: error: 'current' used again (redefinition)
src/wilderness_kb.c:1770: error: 'calculate_distance' undefined
src/wilderness_kb.c:1841: error: 'report_progress' with wrong context
```

These are ALL from the orphaned network graph code that's mixed into generate_ascii_visualization().