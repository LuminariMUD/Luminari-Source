# Phase 00: Core Vessel System

**Status:** Not Started
**Progress:** 0/0 sessions (0%)

## Overview

Complete the core vessel system implementation by wiring existing Phase 1 and Phase 2 code, implementing missing functionality, and ensuring all features are properly integrated and tested.

### Current State Summary

The vessel system has substantial code already written but not fully wired:

**Working:**
- Build integration (CMake/autotools)
- Boot initialization
- 9 commands registered
- Wilderness coordinate navigation (X/Y/Z)
- Terrain restrictions and weather effects
- Basic boarding via ITEM_GREYHAWK_SHIP

**Code Exists, Not Wired:**
- Room generation (vessels_rooms.c)
- Docking handlers (vessels_docking.c)
- DB tables (5 tables auto-created)
- Room templates (19 types)

**Missing Implementation:**
- Dynamic wilderness room allocation
- Per-vessel type mapping
- Phase 2 command registration
- Interior movement functions
- Persistence integration

## Progress Tracker

| Session | Name | Status | Validated |
|---------|------|--------|-----------|
| - | No sessions defined | - | - |

## Critical Path

1. Dynamic wilderness room allocation
2. Per-vessel type mapping
3. Register Phase 2 commands
4. Wire interior generation
5. Implement interior movement

## Next Steps

Run `/nextsession` to get the first session recommendation.

## Key Files

| File | Purpose |
|------|---------|
| src/vessels.h | Structures, constants, prototypes |
| src/vessels.c | Core commands, wilderness movement |
| src/vessels_rooms.c | Interior room generation |
| src/vessels_docking.c | Docking and boarding |
| src/vessels_db.c | Database persistence |
| src/interpreter.c | Command registration |
| src/db.c | Boot sequence |

## Testing Requirements

- Unit tests in unittests/CuTest
- Memory validation with Valgrind
- Performance testing (100/250/500 vessels)
- Test zone 213 for integration testing
