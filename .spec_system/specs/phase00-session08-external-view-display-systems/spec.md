# Session Specification

**Session ID**: `phase00-session08-external-view-display-systems`
**Phase**: 00 - Core Vessel System
**Status**: Not Started
**Created**: 2025-12-29

---

## 1. Session Overview

This session completes the external view and display systems for the vessel system, providing players with critical situational awareness capabilities. With interior rooms (Session 05), movement (Session 06), and persistence (Session 07) now functional, players need to see and understand the world outside their vessels.

The core deliverables are: integrating `look_outside` with the existing weather system (`get_weather()`), implementing the tactical display for visual map rendering, creating a contacts detection and display system for tracking nearby vessels, and completing the disembark mechanics so players can properly exit vessels to docks or water.

This session is the penultimate feature session before final testing/validation in Session 09. All infrastructure is in place; this session wires up the user-facing display and interaction features that make vessels usable for gameplay.

---

## 2. Objectives

1. Implement full `look_outside` integration with `get_weather()` function and wilderness terrain rendering
2. Implement `tactical` command with formatted ASCII tactical map display
3. Implement `contacts` command with vessel detection, distance/bearing calculations, and formatted output
4. Complete `disembark` command with dock/water exit handling and swimming requirements

---

## 3. Prerequisites

### Required Sessions
- [x] `phase00-session01-header-cleanup-foundation` - Clean header structure
- [x] `phase00-session02-dynamic-wilderness-room-allocation` - Dynamic room allocation pattern
- [x] `phase00-session03-vessel-type-system` - Vessel type capabilities
- [x] `phase00-session04-phase2-command-registration` - Command registration in interpreter.c
- [x] `phase00-session05-interior-room-generation-wiring` - Interior rooms working
- [x] `phase00-session06-interior-movement-implementation` - Movement between ship rooms
- [x] `phase00-session07-persistence-integration` - Vessel state persistence

### Required Tools/Knowledge
- Understanding of `get_weather()` function in wilderness.c (returns weather value 0-255)
- Familiarity with greyhawk_ships array and vessel coordinate system
- ASCII rendering constraints (80-column terminal width)

### Environment Requirements
- Vessel system functional with at least one test vessel
- Wilderness coordinate system operational
- Weather system accessible

---

## 4. Scope

### In Scope (MVP)
- `look_outside` command with full `get_weather()` integration
- Wilderness terrain description rendering from ship interior
- `tactical` command rendering ASCII tactical map (11x11 or similar grid)
- Contact detection algorithm scanning vessels within range
- `contacts` command displaying vessel list with distance/bearing
- `disembark` command with dock detection and water exit handling
- Swimming requirement check for water exits

### Out of Scope (Deferred)
- Combat display systems - *Reason: Requires combat system integration (Phase 2+)*
- Advanced radar/sonar features - *Reason: Beyond MVP scope*
- Weather effects on visibility range - *Reason: Nice-to-have enhancement*
- Hostile vessel identification - *Reason: Combat system dependency*
- Underwater tactical display - *Reason: Submarine-specific feature*

---

## 5. Technical Approach

### Architecture

The implementation follows the existing vessel system patterns:

1. **Weather Integration**: Use existing `get_weather(x, y)` from wilderness.c that returns 0-255 weather intensity. Map to descriptive strings (clear, cloudy, rain, storm).

2. **Tactical Display**: Render 11x11 ASCII grid centered on vessel position. Use greyhawk_tactical array for map data. Show vessel as center symbol, terrain as appropriate characters.

3. **Contact Detection**: Iterate greyhawk_ships array, calculate distance using `greyhawk_range()`, filter by detection range (default 50 units), sort by distance.

4. **Disembark**: Check vessel docking state, determine exit type (dock vs water), validate swimming if water exit, move character to appropriate room.

### Design Patterns
- **Static lookup tables**: Weather string mapping (already used in vessels.c)
- **Iterator pattern**: Contact scanning over greyhawk_ships array
- **Command pattern**: ACMD functions for each display command

### Technology Stack
- ANSI C90/C89 with GNU extensions
- Existing wilderness.c `get_weather()` function
- Existing greyhawk coordinate/range functions
- Standard terminal ASCII output (80 columns max)

---

## 6. Deliverables

### Files to Create
| File | Purpose | Est. Lines |
|------|---------|------------|
| None | All implementation in existing files | - |

### Files to Modify
| File | Changes | Est. Lines |
|------|---------|------------|
| `src/vessels_docking.c` | Complete `do_look_outside()` with weather integration, implement `do_disembark()` | ~80 |
| `src/vessels.c` | Implement `do_greyhawk_tactical()`, implement `do_greyhawk_contacts()` | ~150 |

---

## 7. Success Criteria

### Functional Requirements
- [ ] `look_outside` displays wilderness terrain at ship coordinates
- [ ] `look_outside` integrates with `get_weather()` and shows weather conditions
- [ ] `look_outside` shows nearby vessels in sight
- [ ] `tactical` renders ASCII grid map centered on vessel
- [ ] `tactical` shows terrain symbols and compass directions
- [ ] `contacts` lists vessels within detection range
- [ ] `contacts` shows distance and bearing for each contact
- [ ] `disembark` exits to dock when docked
- [ ] `disembark` exits to water with swimming check when not docked
- [ ] `disembark` provides appropriate error messages for invalid states

### Testing Requirements
- [ ] Manual testing of look_outside in various weather conditions
- [ ] Manual testing of tactical display rendering
- [ ] Manual testing of contacts with multiple vessels
- [ ] Manual testing of disembark at dock and in open water

### Quality Gates
- [ ] All files ASCII-encoded
- [ ] Unix LF line endings
- [ ] Code follows project conventions (/* */ comments, 2-space indent)
- [ ] No compiler warnings
- [ ] NULL checks on all pointer operations

---

## 8. Implementation Notes

### Key Considerations
- Weather values from `get_weather()` are 0-255; map to readable strings
- Tactical display must fit 80-column terminal; 11x11 grid recommended
- Contact detection should be efficient for 500+ vessel scalability target
- Disembark must handle edge cases: docked, at dock but not connected, open water

### Potential Challenges
- **Weather mapping**: Need to understand `get_weather()` return value ranges for proper string mapping
  - *Mitigation*: Check wilderness.c implementation and existing usage patterns
- **Terminal width**: ASCII tactical display must not exceed 80 columns
  - *Mitigation*: Use 11x11 grid with 2-char symbols = 22 chars + borders < 30 chars
- **Contact detection performance**: Iterating all vessels for each contacts command
  - *Mitigation*: Simple iteration is O(n) for n=500 vessels, acceptable for MVP
- **Disembark swimming**: Need to integrate with existing swimming check
  - *Mitigation*: Check how other water-entry code handles swimming requirements

### Relevant Considerations
- [P00] **Phase 2 commands not registered**: look_outside was registered in Session 04, verify registration is complete
- [P00] **Max 500 concurrent vessels target**: Contact detection algorithm uses simple O(n) iteration - acceptable for target scale

### ASCII Reminder
All output files must use ASCII-only characters (0-127).

---

## 9. Testing Strategy

### Unit Tests
- Not applicable for this session (display/UI code difficult to unit test)

### Integration Tests
- Verify `get_weather()` integration returns valid values
- Verify `greyhawk_range()` calculations are correct

### Manual Testing
- Create test vessel in wilderness
- Test `look_outside` in various positions
- Test `tactical` display rendering
- Test `contacts` with 0, 1, and multiple nearby vessels
- Test `disembark` while docked
- Test `disembark` in open water (should require swimming)
- Test `disembark` without valid exit point (error case)

### Edge Cases
- `look_outside` from room without external view (should show error)
- `tactical` at map edge coordinates
- `contacts` with no vessels in range
- `disembark` while vessel is moving
- `disembark` when character cannot swim and in deep water

---

## 10. Dependencies

### External Libraries
- None (uses existing codebase functions)

### Other Sessions
- **Depends on**: Sessions 01-07 (all completed)
- **Depended by**: Session 09 (testing-validation) - requires all features functional

---

## Next Steps

Run `/tasks` to generate the implementation task checklist.
