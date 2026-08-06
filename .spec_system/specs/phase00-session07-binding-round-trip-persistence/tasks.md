# Task Checklist

**Session ID**: `phase00-session07-binding-round-trip-persistence`
**Total Tasks**: 21
**Estimated Duration**: 2-4 hours
**Created**: 2026-08-07

---

Legend: `[x]` completed; `[ ]` pending; `[P]` parallelizable; `[SNNMM]` session ref; `TNNN` task ID.

---

## Setup And Trace (4 tasks)

- [x] T001 [S0007] Verify the clean published development base and Session 05/06 prerequisites
- [x] T002 [S0007] Trace mobile, object, and room writer identity selection
- [x] T003 [S0007] Trace editor setup/save ownership and explicit replace/clear behavior
- [x] T004 [S0007] Freeze authored-first, alias, unresolved, override, and legacy-fallback contracts

## Persistence API And Writers (4 tasks)

- [x] T005 [S0007] Add a validated world-authored persistence-name accessor
- [x] T006 [S0007] Make the mobile writer prefer authored identity and retain absent-state fallback
- [x] T007 [S0007] Make the object writer prefer authored identity and retain absent-state fallback
- [x] T008 [S0007] Make the room writer prefer authored identity and retain absent-state fallback

## Production-Linked Round Trips (6 tasks)

- [x] T009 [S0007] Extend the fixture with safe effective-handler override support
- [x] T010 [S0007] Extend the fixture to reload all three production-emitted world files
- [x] T011 [S0007] Add fresh-process writer/reloader orchestration without parser counter reuse
- [x] T012 [S0007] Test stable alias and canonical records through unrelated saves and overrides
- [x] T013 [S0007] Test unresolved and incompatible records through unrelated saves and overrides
- [x] T014 [S0007] Test canonical replace, explicit clear, and callback-only legacy fallback

## Documentation And Manifests (3 tasks)

- [x] T015 [S0007] Add the round-trip suite to both Automake test source lists and CMake
- [x] T016 [S0007] Update the OLC SpecProc guide with authored-first persistence semantics
- [x] T017 [S0007] Check world grammar compatibility, manifest parity, ASCII/LF, and diff hygiene

## Review, Validation, And Completion (4 tasks)

- [x] T018 [S0007] Pass focused compilation and runtime tests for all three formats
- [x] T019 [S0007] Run Apex `creview`, resolve every material finding, and rerun affected checks
- [x] T020 [S0007] Pass full Autotools/CMake tests, install, integrity, and security checks
- [x] T021 [S0007] Run Apex `validate` and `updateprd`, finalize evidence, commit, and publish

## Completion Checklist

- [x] All 21 tasks marked `[x]`
- [x] All tests and checks passing
- [x] All changed text is ASCII with LF endings
- [x] implementation-notes.md updated
- [x] `creview` and `validate` gates passed

## Next Step

Run `plansession` for Session 08.
