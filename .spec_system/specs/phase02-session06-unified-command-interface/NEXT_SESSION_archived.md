# NEXT_SESSION.md

## Session Recommendation

**Generated**: 2025-12-30
**Project State**: Phase 02 - Simple Vehicle Support
**Completed Sessions**: 21 (5 in current phase)

---

## Recommended Next Session

**Session ID**: `phase02-session06-unified-command-interface`
**Session Name**: Unified Command Interface
**Estimated Duration**: 2-4 hours
**Estimated Tasks**: 15-20

---

## Why This Session Next?

### Prerequisites Met
- [x] Session 05 complete (vehicle-in-vehicle mechanics)
- [x] All vehicle commands functional (sessions 01-04)
- [x] All vessel commands functional (Phase 00-01)

### Dependencies
- **Builds on**: phase02-session05-vehicle-in-vehicle-mechanics (vehicle loading/unloading)
- **Enables**: phase02-session07-testing-validation (final phase validation)

### Project Progression
This session is the natural culmination of the vehicle implementation work. With vehicle data structures (01), creation (02), movement (03), player commands (04), and vehicle-in-vehicle mechanics (05) all complete, the next logical step is to unify the command interface across both vehicles and vessels. This abstraction layer will:

1. Provide consistent player experience regardless of transport type
2. Reduce code duplication between vehicle and vessel command handlers
3. Enable future extensions to work across all transport types automatically
4. Simplify documentation and player learning curve

---

## Session Overview

### Objective
Create an abstraction layer that provides consistent commands across all transport types (vehicles and vessels), allowing players to use common commands regardless of transport type.

### Key Deliverables
1. Transport abstraction structure (`transport_data`)
2. `get_transport_type()` helper function
3. `do_enter()` unified command handler (detects vehicle vs vessel)
4. `do_exit_transport()` unified command handler (dismount/disembark)
5. `do_transport_go()` unified movement command
6. `do_transportstatus()` unified status command
7. Documentation for unified interface

### Scope Summary
- **In Scope (MVP)**: Transport abstraction structure, unified enter/exit/go/status commands, transport type detection, backward compatibility
- **Out of Scope**: Unified autopilot commands, unified combat interface, transport switching while moving, GUI/client extensions

---

## Technical Considerations

### Technologies/Patterns
- ANSI C90/C89 (no C99/C11 features)
- Abstraction via function pointers or type dispatch
- Consistent with existing vessel command patterns
- CuTest for unit testing

### Potential Challenges
- Ensuring backward compatibility with existing `mount`, `dismount`, `board`, `disembark` commands
- Clean abstraction without excessive type-specific conditionals
- Memory-efficient transport detection (avoid O(n) scans)
- Proper NULL checking at abstraction boundaries

### Relevant Considerations
- [P01] **Don't use C99/C11 features**: Stick to ANSI C90/C89 for all new code
- [P01] **Standalone unit test files**: Keep tests self-contained without server dependencies
- [P01] **Incremental session approach**: This session builds cleanly on sessions 01-05

---

## Alternative Sessions

If this session is blocked:
1. **phase02-session07-testing-validation** - Could write tests for sessions 01-05 first, then return to session 06 (less ideal - testing unified interface would be incomplete)

---

## Next Steps

Run `/sessionspec` to generate the formal specification.
