# Session Specification

**Session ID**: `phase00-session01-header-cleanup-foundation`
**Phase**: 00 - Core Vessel System
**Status**: Not Started
**Created**: 2025-12-29

---

## 1. Session Overview

This session addresses critical technical debt in the vessel system header file (`vessels.h`) that must be resolved before any new functionality can be safely implemented. The file currently contains multiple duplicate struct definitions and constant declarations that were introduced during incremental development of three different vessel subsystems (CWG, Outcast, and Greyhawk).

The duplicates present an immediate risk: attempting to compile with certain feature flags enabled will cause redefinition errors, and the inconsistent guard patterns make it unclear which definitions are authoritative. More importantly, the second set of struct definitions (lines 498-612) includes Phase 2 multi-room extensions that are not present in the first set (lines 321-405), creating confusion about the intended data model.

By consolidating these definitions into a single, clean, well-organized header structure, this session establishes the foundation for all subsequent vessel system development. Every other session in Phase 00 depends on this cleanup being complete.

---

## 2. Objectives

1. Eliminate all duplicate struct definitions (greyhawk_ship_slot, greyhawk_ship_crew, greyhawk_ship_data, greyhawk_contact_data, greyhawk_ship_map) by consolidating to a single authoritative version
2. Remove duplicate constant definitions (VESSEL_STATE_*, VESSEL_SIZE_*, Greyhawk constants) while preserving all needed values
3. Verify the codebase compiles cleanly with zero errors and no new warnings
4. Confirm server startup works correctly (smoke test)

---

## 3. Prerequisites

### Required Sessions
- [x] None - this is the foundation session

### Required Tools/Knowledge
- GCC or Clang compiler
- Understanding of C preprocessor directives
- Familiarity with header file organization

### Environment Requirements
- Build system working (CMake or autotools)
- Configuration headers in place (campaign.h, mud_options.h, vnums.h)

---

## 4. Scope

### In Scope (MVP)
- Remove duplicate VESSEL_STATE_* constants (lines 50-53 vs 89-91)
- Remove duplicate VESSEL_SIZE_* constants (lines 56-59 vs 94-97)
- Remove duplicate Greyhawk constants (lines 65-85 vs 298-318)
- Remove duplicate greyhawk_ship_slot struct (lines 321-329 vs 498-506)
- Remove duplicate greyhawk_ship_crew struct (lines 331-337 vs 508-514)
- Remove duplicate greyhawk_ship_data struct (lines 340-386 vs 544-612) - keep extended version
- Remove duplicate greyhawk_contact_data struct (lines 388-395 vs 614-621)
- Remove duplicate greyhawk_ship_map struct (lines 403-405 vs 624-626)
- Remove duplicate function prototypes (lines 451-477 vs 629-656)
- Verify clean compilation
- Verify server starts successfully

### Out of Scope (Deferred)
- Adding new functionality - *Reason: foundation work only*
- Refactoring beyond duplicate removal - *Reason: minimize risk*
- Performance optimization - *Reason: not relevant to header cleanup*
- Changing any struct field definitions - *Reason: could break compatibility*
- Modifying the #if VESSELS_ENABLE_* guard structure - *Reason: needs separate analysis*

---

## 5. Technical Approach

### Architecture
The header file will be reorganized to have a single location for each definition. The extended struct definitions (with Phase 2 multi-room fields) will be kept as the authoritative versions. Constants will be consolidated at the top of the file in logical groupings.

### Design Patterns
- **Single Source of Truth**: Each struct/constant defined exactly once
- **Incremental Verification**: Compile after each major removal to catch issues early
- **Preserve Functionality**: Keep all needed definitions, only remove exact duplicates

### Technology Stack
- ANSI C90/C89 (no C99/C11 features)
- GNU Make or CMake build system
- GCC/Clang compiler with -Wall -Wextra

---

## 6. Deliverables

### Files to Create
| File | Purpose | Est. Lines |
|------|---------|------------|
| None | This session only modifies existing files | - |

### Files to Modify
| File | Changes | Est. Lines |
|------|---------|------------|
| `src/vessels.h` | Remove duplicate definitions, consolidate to single authoritative version | -150 (removal) |

---

## 7. Success Criteria

### Functional Requirements
- [ ] No duplicate struct definitions in vessels.h
- [ ] No duplicate constant definitions in vessels.h
- [ ] No duplicate function prototype definitions
- [ ] greyhawk_ship_data retains all Phase 2 multi-room fields
- [ ] All existing macros still reference valid struct fields

### Testing Requirements
- [ ] Code compiles with `cmake --build build/` with zero errors
- [ ] No new compiler warnings introduced
- [ ] Server starts successfully (`./bin/circle -d lib`)
- [ ] Basic vessel commands still functional (if testable)

### Quality Gates
- [ ] All files ASCII-encoded
- [ ] Unix LF line endings
- [ ] Code follows project conventions (/* */ comments only)
- [ ] Header guards intact (#ifndef _VESSELS_H_)

---

## 8. Implementation Notes

### Key Considerations
- The second greyhawk_ship_data struct (lines 544-612) includes Phase 2 multi-room additions not in the first - this is the version to keep
- Forward declarations on lines 492-495 are redundant once duplicates are removed
- The #if VESSELS_ENABLE_GREYHAWK guard (line 296) protects the first set but not the second
- Macros (lines 407-448) must continue to work after consolidation

### Potential Challenges
- **Determining authoritative definitions**: Resolved by analysis - second set has Phase 2 extensions, keep it
- **Breaking the #if guard logic**: Keep guards but ensure definitions are outside or properly restructured
- **Hidden dependencies**: Compile frequently to catch any issues immediately

### Relevant Considerations
- [P00] **Duplicate struct definitions in vessels.h**: Lines 321-405 vs 498-612 - greyhawk_ship_slot, greyhawk_ship_crew, greyhawk_ship_data need consolidation. The second set (498-612) is more complete and should be kept.
- [P00] **VESSEL_STATE constants duplicated**: Lines 50-58 vs 89-97 - need to remove one set (keep first, remove second)
- [P00] **Don't use C99/C11 features**: All changes must use /* */ comments, no declarations after statements

### ASCII Reminder
All output files must use ASCII-only characters (0-127).

---

## 9. Testing Strategy

### Unit Tests
- Not applicable for header-only changes (no new logic)

### Integration Tests
- Compile entire codebase to verify no redefinition errors
- Link binary to verify no missing symbols

### Manual Testing
- Start server: `./bin/circle -d lib`
- Verify server boots without errors
- If possible, test basic vessel presence in game

### Edge Cases
- Verify compilation with different VESSELS_ENABLE_* flag combinations
- Check that all macros (GREYHAWK_SHIPX, etc.) still resolve correctly

---

## 10. Dependencies

### External Libraries
- None specific to this session

### Other Sessions
- **Depends on**: None (foundation session)
- **Depended by**: Sessions 02-09 (all subsequent Phase 00 sessions)

---

## Duplicate Location Summary

For implementation reference, here are the exact duplicate locations identified:

### Constants (Remove second occurrence, lines 89-97)
```
Lines 50-53: VESSEL_STATE_DOCKED/TRAVELING/COMBAT/DAMAGED (keep)
Lines 56-59: VESSEL_SIZE_SMALL/MEDIUM/LARGE/HUGE (keep)
Lines 89-91: VESSEL_STATE_TRAVELING/COMBAT/DAMAGED (REMOVE - duplicate)
Lines 94-97: VESSEL_SIZE_SMALL/MEDIUM/LARGE/HUGE (REMOVE - duplicate)
```

### Greyhawk Constants (Already guarded - verify no redefinition)
```
Lines 65-85: Under unconditional defines (keep - first occurrence)
Lines 298-318: Under #if VESSELS_ENABLE_GREYHAWK (guarded - OK)
```

### Structs (Remove first occurrence under #if, keep extended versions)
```
Lines 321-329: greyhawk_ship_slot (REMOVE)
Lines 331-337: greyhawk_ship_crew (REMOVE)
Lines 340-386: greyhawk_ship_data (REMOVE - missing Phase 2 fields)
Lines 388-395: greyhawk_contact_data (REMOVE)
Lines 403-405: greyhawk_ship_map (REMOVE)

Lines 498-506: greyhawk_ship_slot (KEEP)
Lines 508-514: greyhawk_ship_crew (KEEP)
Lines 544-612: greyhawk_ship_data (KEEP - has Phase 2 fields)
Lines 614-621: greyhawk_contact_data (KEEP)
Lines 624-626: greyhawk_ship_map (KEEP)
```

### Function Prototypes (Remove first occurrence)
```
Lines 451-477: Under #if VESSELS_ENABLE_GREYHAWK (REMOVE)
Lines 629-656: Unconditional (KEEP)
```

---

## Next Steps

Run `/tasks` to generate the implementation task checklist.
