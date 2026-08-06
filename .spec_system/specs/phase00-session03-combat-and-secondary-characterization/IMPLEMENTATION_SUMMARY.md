# Implementation Summary

**Session ID**: `phase00-session03-combat-and-secondary-characterization`
**Completed**: 2026-08-06
**Duration**: ~25 minutes

---

## Overview

Completed the Phase 00 legacy invocation compatibility matrix for combat, identification,
maneuvers, mounted charge, shops, and quests. The suite freezes exact callback actors, owners,
commands, tokens, activation gates, ignored versus normalized returns, combat ordering, secondary
nesting, assignment composition, and applicable `no_specials` behavior without changing production
code.

## Deliverables

### File Created

| File | Purpose | Lines |
|------|---------|-------|
| `unittests/CuTest/test_spec_combat_secondary.c` | Isolated combat and secondary characterization | 990 |

### Files Modified

| File | Changes |
|------|---------|
| `Makefile.am` | Added the suite to CuTest compilation and generated-test membership. |
| `CMakeLists.txt` | Added matching production-linked CuTest membership. |

## Technical Decisions

1. **Execute narrow production paths directly**: Runtime tests call item display,
   `weapon_special()`, `perform_violence()`, `shop_keeper()`, and `questmaster()` with isolated
   production structures.
2. **Bound random and boot evidence**: Defense, maneuver, combat-order, assignment, and boot checks
   use function-bounded source regions because driving their complete paths would introduce
   unrelated random combat or database/world boot behavior.
3. **Compose wrappers without static leakage**: The nested runtime scenario installs
   `questmaster -> shop_keeper -> original` directly; source contracts separately freeze the
   save-before-install assignment sequence.
4. **Restore before asserting**: Every replaced process global is restored before a CuTest
   assertion can leave the test.

## Test Results

| Metric | Value |
|--------|-------|
| Session tests added | 14 |
| Total CuTests | 509 |
| Passed | 509 |
| Root auxiliary checks | 7/7 passed |
| CMake production CTest | Passed |

`make test`, `make install`, CMake/CTest, formatting, changed-code static analysis, encoding,
security, world-data integrity, and artifact hygiene all passed. No production source or release
version changed, so a version bump was not applicable.

## Compatibility Baseline

- Mobile combat callbacks require `MOB_SPEC`, a valid pointer, non-pending extraction, and positive
  hit points; they run after normal attacks and cleave, receive `(ch, ch, 0, "")`, and ignore the
  result.
- Identification sends `"identify"`; weapon hits forward the exact hit token; high-level callers
  discard their notification results.
- Defense sends `"shieldblock"`, `"parry"`, `"glance"`, or `"dodge"`; shield maneuvers send
  `"shieldpunch"`, `"shieldcharge"`, or `"shieldslam"`; mounted charge sends `"charge"`.
- Shop and quest wrappers forward incoming context unchanged, continue on zero, and return `TRUE`
  on any nonzero secondary result.
- Shop assignment saves the existing callback before installing `shop_keeper`; quest assignment
  can save that wrapper before installing `questmaster`, producing quest-over-shop-over-original.
- `no_specials` skips shop loading and the assignment block but does not gate the direct callback
  sites or already-installed shop and quest wrappers.

## Future Considerations

1. Session 04 can build validated definition metadata on the now-complete legacy invocation matrix.
2. Later event gateways must retain each caller's distinct ignored, fallback, consumed, or
   normalized return meaning.
3. Observable binding provenance must report saved shop and quest secondaries without flattening
   their nesting.

## Session Statistics

- **Tasks**: 22 completed
- **Implementation files created**: 1
- **Implementation files modified**: 2
- **Tests added**: 14
- **Review findings**: 2 Low, all resolved
- **Blockers**: None
