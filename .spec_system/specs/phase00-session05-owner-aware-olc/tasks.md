# Task Checklist

**Session ID**: `phase00-session05-owner-aware-olc`
**Total Tasks**: 23
**Estimated Duration**: 2-4 hours
**Created**: 2026-08-07

---

Legend: `[x]` completed; `[ ]` pending; `[P]` parallelizable; `[SNNMM]` session ref; `TNNN` task ID.

---

## Setup And Trace (3 tasks)

- [x] T001 [S0005] Verify the development environment, clean Session 04 base, and Apex prerequisites
- [x] T002 [S0005] Trace all three production selector display, parse, clear, quit, and callback-slot paths
- [x] T003 [S0005] Freeze exact filtered inventories, prerequisites, and deferred binding/help scope

## Shared Owner-Aware Menu (6 tasks)

- [x] T004 [S0005] Define the shared filtered-view and selection-result API (`src/olc/spec_menu.h`)
- [x] T005 [S0005] Filter definitions by one owner bit, visibility, world binding, and legacy slot compatibility
- [x] T006 [S0005] Map signed filtered indexes safely in canonical order
- [x] T007 [S0005] Parse bounded decimal choices into invalid, clear, or definition results
- [x] T008 [S0005] Render display name, category, description, events, and per-event prerequisites
- [x] T009 [S0005] Render unsupported or empty owner views safely and explicitly

## Editor Integration (4 tasks)

- [x] T010 [S0005] Integrate the shared mobile view and selection mapping in medit
- [x] T011 [S0005] Integrate the shared object view and selection mapping in oedit
- [x] T012 [S0005] Integrate the shared room view and selection mapping in redit
- [x] T013 [S0005] Preserve clear, quit, dirty-state, callback-slot, and activation-flag semantics

## Production-Linked Verification (6 tasks)

- [x] T014 [S0005] Extend shared fixtures for menu entry and activation-state observation
- [x] T015 [S0005] Assert exact filtered inventories and every returned definition constraint
- [x] T016 [S0005] Assert representative metadata and every current prerequisite label in output
- [x] T017 [S0005] Assert valid selection mapping through all three production editors
- [x] T018 [S0005] Assert clear, quit, malformed, extreme, unsupported-owner, and empty-list paths
- [x] T019 [S0005] Assert prerequisite-bearing selection never mutates prototype activation flags

## Integration And Completion (4 tasks)

- [x] T020 [S0005] Reconcile Session 01 editor positions with the deliberate filtered-view change
- [x] T021 [S0005] Synchronize new source/test membership and update the builder guide
- [x] T022 [S0005] Pass targeted compilation, runtime, formatting, static, and `creview` gates
- [x] T023 [S0005] Pass full tests, CMake/CTest, installation, integrity, security, and `validate` gates

## Completion Checklist

- [x] All 23 tasks marked `[x]`
- [x] All tests and checks passing
- [x] All changed text is ASCII with LF endings
- [x] implementation-notes.md updated
- [x] `creview` and `validate` gates passed

## Next Step

After completion, run `plansession` for Session 06.
