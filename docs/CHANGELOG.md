# Luminari MUD Changelog

Assembled by Zusuk  
Last Updated: January 2025

## Table of Contents

1. [Introduction](#introduction)
2. [Changelog Format](#changelog-format)
3. [Recent Updates](#recent-updates)
4. [Full Changelog](#full-changelog)

---

## Introduction

This changelog documents all significant changes to LuminariMUD, including new features, bug fixes, and system improvements.

## Changelog Format

Each entry follows the format:
- **Date** - Feature/Fix: Description
- Subsystems are noted in [brackets]

## Recent Updates

### January 2025

#### Vessel System Phase 1 - Foundation Implementation

**Date:** January 2025  
**Developer:** AI Assistant with Zusuk  
**Status:** COMPLETED  

**Core Infrastructure Implemented:**
- [VESSELS] Activated Greyhawk naval system from vessels_src.c/h
- [VESSELS] Integrated vessel system with main codebase architecture
- [BUILD] Added vessels.c to both autotools (Makefile.am) and CMake build systems
- [STRUCTS] Enhanced room_data structure with greyhawk_ship_data pointer
- [DB] Added greyhawk_initialize_ships() to boot sequence

**Data Structures Added:**
- [VESSELS] greyhawk_ship_data structure supporting 500 concurrent ships
- [VESSELS] 3D coordinate system (x, y, z) for ship positioning
- [VESSELS] Directional armor system (fore, aft, port, starboard)
- [VESSELS] Equipment slot system (10 slots per ship)
- [VESSELS] Crew management structures (sail crew, gun crew)

**Commands Registered:**
- tactical - Display tactical map (placeholder)
- shipstatus - Show ship status (placeholder)
- speed - Control ship speed (placeholder)
- heading - Set ship heading (placeholder)
- contacts - Show radar contacts (placeholder)
- disembark - Leave ship (placeholder)
- shipload - [ADMIN] Load ship template (placeholder)
- setsail - [ADMIN] Configure ship sails (placeholder)

**Technical Achievements:**
- Fixed all compilation errors and warnings
- Resolved ACMD macro conflicts between headers
- Maintained C90/C89 compatibility (no C99 features)
- Zero memory leaks in initialization
- Both build systems fully functional

**Files Modified:**
- src/vessels.c (created - 272 lines)
- src/vessels.h (updated - 574 lines)
- src/structs.h (added ship pointer to room_data)
- src/interpreter.c (registered 8 new commands)
- src/db.c (added initialization call)
- Makefile.am (added vessels.c to sources)
- CMakeLists.txt (added vessels.c to sources)

**Foundation Ready For:**
- Phase 2: Ship loading and template system
- Phase 3: Movement and navigation implementation
- Phase 4: Combat system activation
- Phase 5: Multi-room ship interiors

---

## Full Changelog

### 2025

See Recent Updates above for January 2025 Vessel System implementation.

### Previous Years

[Historical changelog entries to be migrated]

---