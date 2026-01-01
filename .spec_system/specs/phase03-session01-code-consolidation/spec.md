# Session Specification

**Session ID**: `phase03-session01-code-consolidation`
**Phase**: 03 - Optimization & Polish
**Status**: Not Started
**Created**: 2025-12-30

---

## 1. Session Overview

This session addresses technical debt accumulated during rapid Phase 00-02 development by consolidating duplicate constant definitions in vessels.h. The file currently contains redundant definitions where constants are declared both unconditionally (for general use) and within conditional compilation blocks (`#if VESSELS_ENABLE_GREYHAWK` and `#if VESSELS_ENABLE_OUTCAST`), leading to potential conflicts and maintenance confusion.

The primary issue is duplicate macro definitions for ship system constants like `GREYHAWK_MAXSHIPS`, `GREYHAWK_MAXSLOTS`, position constants (`GREYHAWK_FORE`, etc.), weapon range types, and critically, conflicting values for `GREYHAWK_ITEM_SHIP` (56 vs 57). These duplications create risks of silent conflicts when conditional blocks are enabled/disabled and make the codebase harder to reason about.

This foundational cleanup session must complete before Phase 03's feature sessions (interior movement, command registration, etc.) to ensure all subsequent work builds on a clean, maintainable foundation. The session involves no new features - purely refactoring with full backward compatibility.

---

## 2. Objectives

1. Consolidate duplicate GREYHAWK_* constant definitions into single canonical locations
2. Resolve conflicting GREYHAWK_ITEM_SHIP values (56 vs 57) with one authoritative definition
3. Consolidate DOCKABLE macro definitions (ROOM_DOCKABLE alias vs literal 41)
4. Ensure all conditional compilation blocks reference shared constants rather than redefining
5. Verify all 159 existing unit tests continue passing after consolidation

---

## 3. Prerequisites

### Required Sessions
- [x] `phase02-session07-testing-validation` - Provides stable test baseline (159 tests passing)

### Required Tools/Knowledge
- Understanding of C preprocessor conditional compilation (#if, #ifndef)
- Familiarity with MUD item type and room flag systems
- CuTest unit testing framework

### Environment Requirements
- GCC or Clang with -Wall -Wextra flags
- Valgrind for memory verification
- Clean git working state (no uncommitted changes)

---

## 4. Scope

### In Scope (MVP)
- Remove duplicate GREYHAWK_MAXSHIPS definition (line 339 inside conditional)
- Remove duplicate GREYHAWK_MAXSLOTS definition (line 343 inside conditional)
- Remove duplicate GREYHAWK_FORE/PORT/REAR/STARBOARD definitions (lines 347-350)
- Remove duplicate GREYHAWK_SHRTRANGE/MEDRANGE/LNGRANGE definitions (lines 353-355)
- Resolve GREYHAWK_ITEM_SHIP conflict (line 85 vs line 358) - standardize on value 56
- Consolidate DOCKABLE definition (line 88 vs line 272)
- Remove duplicate ITEM_SHIP definition in OUTCAST section if conflicting
- Update any code that references removed definitions
- Verify compilation with all conditional flags enabled/disabled
- Run full test suite to confirm no regressions

### Out of Scope (Deferred)
- New feature implementation - *Reason: Pure refactoring session*
- Performance optimization - *Reason: Phase 03 Session 06 scope*
- Database schema changes - *Reason: No schema impact*
- Interior movement implementation - *Reason: Phase 03 Session 02 scope*
- Struct definition changes - *Reason: Current structs are clean (single definitions)*

---

## 5. Technical Approach

### Architecture
The consolidation follows a "define once, use everywhere" pattern. All constants will be defined unconditionally at the top of vessels.h (current lines 62-88), and conditional blocks will be modified to remove their duplicate definitions while keeping conditional-specific code.

### Design Patterns
- **Single Source of Truth**: Each constant defined exactly once
- **Guard Pattern**: Use `#ifndef CONSTANT` / `#define CONSTANT` only for constants that might be overridden by external config
- **Incremental Refactoring**: Change one category of duplicates at a time with compilation verification

### Technology Stack
- ANSI C90/C89 with GNU extensions
- GCC/Clang compiler with -Wall -Wextra
- CuTest unit testing framework
- Valgrind memory checking

### Resolution Strategy for Conflicts
1. **GREYHAWK_ITEM_SHIP**: Use value 56 (matches existing ITEM_GREYHAWK_SHIP usage in codebase)
2. **DOCKABLE**: Keep as `ROOM_DOCKABLE` alias for consistency with room flag system
3. **ITEM_SHIP in OUTCAST**: Remove if unused, or rename to avoid conflict

---

## 6. Deliverables

### Files to Create
| File | Purpose | Est. Lines |
|------|---------|------------|
| (none) | Pure refactoring - no new files | 0 |

### Files to Modify
| File | Changes | Est. Lines Changed |
|------|---------|------------|
| `src/vessels.h` | Remove duplicate constant definitions from conditional blocks | ~30 |
| `src/vessels.c` | Update any references to removed definitions (if any) | ~5 |
| `src/vessels_rooms.c` | Update any references to removed definitions (if any) | ~5 |
| `src/vessels_docking.c` | Update any references to removed definitions (if any) | ~5 |

---

## 7. Success Criteria

### Functional Requirements
- [ ] No duplicate `#define` for any constant in vessels.h
- [ ] No conflicting values for any constant (e.g., GREYHAWK_ITEM_SHIP)
- [ ] All conditional blocks compile correctly when enabled
- [ ] All conditional blocks compile correctly when disabled
- [ ] Project builds successfully with `./cbuild.sh`

### Testing Requirements
- [ ] All 159 existing unit tests pass
- [ ] Valgrind reports no new memory issues
- [ ] Manual compilation test with VESSELS_ENABLE_GREYHAWK=1
- [ ] Manual compilation test with VESSELS_ENABLE_GREYHAWK=0

### Quality Gates
- [ ] All files ASCII-encoded
- [ ] Unix LF line endings
- [ ] Code follows project conventions (two-space indent, /* */ comments)
- [ ] No compiler warnings with -Wall -Wextra

---

## 8. Implementation Notes

### Key Considerations
- The `#ifndef GREYHAWK_MAXSHIPS` pattern at line 65 suggests these constants might be overridden externally - preserve this pattern for the canonical definition
- GREYHAWK_ITEM_SHIP value 56 appears in line 85 (unconditional) and is used throughout the codebase - this is the canonical value
- The OUTCAST and GREYHAWK sections may be legacy/disabled code - verify before removing

### Potential Challenges
- **Hidden dependencies**: Some code might depend on conditional definitions being present - mitigation: grep for all usages before removing
- **Build flag variations**: Different build configs might enable/disable conditional blocks - mitigation: test both enabled and disabled states
- **Item type conflicts**: ITEM_SHIP (56) vs GREYHAWK_ITEM_SHIP conflict - mitigation: audit all usages to determine canonical value

### Relevant Considerations
- **[P01]** **Duplicate struct definitions in vessels.h**: Investigation shows structs are now clean (single definitions). Focus on constant duplicates instead.
- **[P01]** **VESSEL_STATE constants duplicated**: Investigation shows these are NOT duplicated in current code (only at lines 50-53). Mark as resolved.
- **[P01]** **Standalone unit test files pattern**: Use existing test infrastructure to validate changes.

### ASCII Reminder
All output files must use ASCII-only characters (0-127).

---

## 9. Testing Strategy

### Unit Tests
- Run existing 159 unit tests after each consolidation step
- No new unit tests required (pure refactoring with existing coverage)

### Integration Tests
- Build project with `./cbuild.sh` after consolidation
- Verify no compilation warnings

### Manual Testing
- Enable VESSELS_ENABLE_GREYHAWK and compile
- Disable VESSELS_ENABLE_GREYHAWK and compile
- Enable VESSELS_ENABLE_OUTCAST and compile (if applicable)
- Verify all greyhawk ship commands still function (board, shipstatus, etc.)

### Edge Cases
- Conditional blocks with no remaining content after duplicate removal (may need cleanup)
- References to removed definitions in dependent files
- External configuration that might override constants

---

## 10. Dependencies

### External Libraries
- MySQL/MariaDB client libraries (existing dependency, unchanged)

### Other Sessions
- **Depends on**: `phase02-session07-testing-validation` (provides test baseline)
- **Depended by**: All subsequent Phase 03 sessions (provides clean foundation)

---

## Duplicate Constants Inventory

| Constant | Line (Unconditional) | Line (Conditional) | Resolution |
|----------|---------------------|-------------------|------------|
| GREYHAWK_MAXSHIPS | 66 | 339 | Keep line 66, remove line 339 |
| GREYHAWK_MAXSLOTS | 70 | 343 | Keep line 70, remove line 343 |
| GREYHAWK_FORE | 74 | 347 | Keep line 74, remove line 347 |
| GREYHAWK_PORT | 75 | 348 | Keep line 75, remove line 348 |
| GREYHAWK_REAR | 76 | 349 | Keep line 76, remove line 349 |
| GREYHAWK_STARBOARD | 77 | 350 | Keep line 77, remove line 350 |
| GREYHAWK_SHRTRANGE | 80 | 353 | Keep line 80, remove line 353 |
| GREYHAWK_MEDRANGE | 81 | 354 | Keep line 81, remove line 354 |
| GREYHAWK_LNGRANGE | 82 | 355 | Keep line 82, remove line 355 |
| GREYHAWK_ITEM_SHIP | 85 (=56) | 358 (=57) | Keep line 85 (value 56), remove line 358 |
| DOCKABLE | 88 (=ROOM_DOCKABLE) | 272 (=41) | Keep line 88, remove line 272 |
| ITEM_SHIP | N/A | 271 (=56) | Evaluate - may conflict with GREYHAWK_ITEM_SHIP |

---

## Next Steps

Run `/tasks` to generate the implementation task checklist.
