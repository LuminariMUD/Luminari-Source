# NEXT_SESSION.md

## Session Recommendation

**Generated**: 2025-12-29
**Project State**: Phase 00 - Core Vessel System
**Completed Sessions**: 7 of 9

---

## Recommended Next Session

**Session ID**: `phase00-session08-external-view-display-systems`
**Session Name**: External View & Display Systems
**Estimated Duration**: 3-4 hours
**Estimated Tasks**: 20-25

---

## Why This Session Next?

### Prerequisites Met
- [x] Sessions 01-07 completed
- [x] Interior rooms working (Session 05)
- [x] Movement working (Session 06)
- [x] Weather system accessible (get_weather() exists)

### Dependencies
- **Builds on**: Session 06 (interior movement) and Session 07 (persistence)
- **Enables**: Session 09 (testing & validation requires all features functional)

### Project Progression
Session 08 is the natural next step - it completes the remaining user-facing vessel features before the final testing/validation phase. All core infrastructure (rooms, movement, persistence) is in place; this session wires up the display and interaction features that players will use.

---

## Session Overview

### Objective
Complete the external view and display systems: integrate look_outside with weather/wilderness view, implement tactical display rendering, implement contact detection, and complete disembark mechanics.

### Key Deliverables
1. `look_outside` command shows exterior with weather conditions
2. `tactical` command renders formatted ASCII tactical map
3. `contacts` command shows nearby vessels with distance/bearing
4. `disembark` command fully functional (dock/water exit handling)

### Scope Summary
- **In Scope (MVP)**: look_outside with get_weather() integration, wilderness view rendering, tactical display, contact detection, contacts display, disembark mechanics
- **Out of Scope**: Combat display systems, advanced radar/sonar, weather effects on visibility

---

## Technical Considerations

### Technologies/Patterns
- Integration with existing weather.c get_weather() function
- ASCII art tactical map rendering
- Coordinate-based distance/bearing calculations
- Room transition handling for disembark

### Potential Challenges
1. **Weather integration** - Ensuring get_weather() provides data in expected format
2. **Tactical display** - ASCII rendering needs to handle terminal width constraints
3. **Contact detection** - Efficient scanning of vessels within range
4. **Disembark edge cases** - Dock vs water exit, swimming requirements

### Relevant Considerations
- [P00] **Phase 2 commands not registered**: look_outside still needs full implementation (noted in Active Concerns)
- [P00] **Max 500 concurrent vessels target**: Contact detection algorithm should be efficient for scalability

---

## Files to Modify

- `src/vessels_docking.c` - look_outside, disembark implementation
- `src/vessels.c` - tactical, contacts commands
- Integration with `src/weather.c` for get_weather()

---

## Alternative Sessions

If this session is blocked:
1. **phase00-session09-testing-validation** - Could start partial testing of already-implemented features, but full testing requires session 08 completion
2. **No other candidates** - Session 09 is the only remaining session in Phase 00

---

## Next Steps

Run `/sessionspec` to generate the formal specification.
