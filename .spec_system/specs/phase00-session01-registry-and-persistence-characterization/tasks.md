# Task Checklist

**Session ID**: `phase00-session01-registry-and-persistence-characterization`
**Total Tasks**: 23
**Estimated Duration**: 3-4 hours
**Created**: 2026-08-06

---

Legend: `[x]` completed; `[ ]` pending; `[P]` parallelizable; `[SNNMM]` session ref; `TNNN` task ID.

---

## Setup (2 tasks)

- [x] T001 [S0001] Verify the development environment and workflow prerequisites
  (`.spec_system/scripts/check-prereqs.sh`)
- [x] T002 [S0001] Record the current 29-row registry, 28-handler alias shape, and five named world
  bindings without modifying checked-in world data
  (`.spec_system/specs/phase00-session01-registry-and-persistence-characterization/implementation-notes.md`)

---

## Foundation (5 tasks)

- [x] T003 [S0001] Define the reusable fixture lifecycle, parser, writer, and OLC contracts
  (`unittests/CuTest/test_spec_fixtures.h`)
- [x] T004 [S0001] Implement global pointer, top-index, configuration, and working-directory snapshot
  and restoration with cleanup on every fixture exit (`unittests/CuTest/test_spec_fixtures.c`)
- [x] T005 [S0001] Implement private temporary `world/mob`, `world/obj`, and `world/wld` sandbox
  creation and bounded saved-file reading (`unittests/CuTest/test_spec_fixtures.c`)
- [x] T006 [S0001] Build minimal valid named mobile, object, and room streams and load them through
  `parse_mobile()`, `parse_object()`, and `parse_room()` (`unittests/CuTest/test_spec_fixtures.c`)
- [x] T007 [S0001] Initialize reusable descriptor and `oasis_olc_data` state for the real medit,
  oedit, and redit parsers (`unittests/CuTest/test_spec_fixtures.c`)

---

## Registry Characterization (4 tasks)

- [x] T008 [S0001] Assert the exact current registry count and indexed persisted-name sequence
  (`unittests/CuTest/test_spec_registry_persistence.c`)
- [x] T009 [S0001] Assert case-insensitive known lookup plus null, empty, and unknown rejection
  (`unittests/CuTest/test_spec_registry_persistence.c`)
- [x] T010 [S0001] Assert `Guild` reverse lookup and `Guildmaster` shared-handler compatibility
  (`unittests/CuTest/test_spec_registry_persistence.c`)
- [x] T011 [S0001] Assert negative and sentinel name and function accessor boundaries
  (`unittests/CuTest/test_spec_registry_persistence.c`)

---

## Persistence Characterization (5 tasks)

- [x] T012 [S0001] Assert production mobile parsing resolves the known `Postmaster` binding
  (`unittests/CuTest/test_spec_registry_persistence.c`)
- [x] T013 [S0001] Assert production object parsing resolves the known `Greyhawk Ship` binding
  (`unittests/CuTest/test_spec_registry_persistence.c`)
- [x] T014 [S0001] Assert production room parsing resolves the known `Greyhawk Ship Commands` binding
  (`unittests/CuTest/test_spec_registry_persistence.c`)
- [x] T015 [S0001] Save all three loaded prototypes through `save_mobiles()`, `save_objects()`, and
  `save_rooms()` and assert their exact current canonical fields
  (`unittests/CuTest/test_spec_registry_persistence.c`)
- [x] T016 [S0001] Assert the checked-in source inventory remains one mobile, two object, and two
  room named-binding occurrences (`unittests/CuTest/test_spec_registry_persistence.c`)

---

## OLC Characterization (3 tasks)

- [x] T017 [S0001] Characterize medit valid selection, invalid bounds, quit, and explicit clear
  through `medit_parse()` (`unittests/CuTest/test_spec_registry_persistence.c`)
- [x] T018 [S0001] Characterize oedit valid selection, invalid bounds, quit, and explicit clear
  through `oedit_parse()` (`unittests/CuTest/test_spec_registry_persistence.c`)
- [x] T019 [S0001] Characterize redit valid selection, invalid bounds, quit, and explicit clear
  through `redit_parse()` (`unittests/CuTest/test_spec_registry_persistence.c`)

---

## Integration And Verification (4 tasks)

- [x] T020 [S0001] Add both fixture and characterization sources to Automake compile and generated
  test inputs and CMake CuTest sources (`Makefile.am`, `CMakeLists.txt`)
- [x] T021 [S0001] Regenerate the CuTest registry and run the production-linked suite with zero new
  warnings (`make test`)
- [x] T022 [S0001] Install the tested server and verify `bin/circle` is current with no root-level
  binary (`make install`, `bin/circle`, `circle`)
- [x] T023 [S0001] Verify changed files are ASCII with LF endings and record test, inventory, and
  artifact evidence
  (`.spec_system/specs/phase00-session01-registry-and-persistence-characterization/implementation-notes.md`)

---

## Completion Checklist

- [x] All tasks marked `[x]`
- [x] All tests and checks passing
- [x] All files ASCII-encoded with LF line endings
- [x] implementation-notes.md updated
- [x] `creview` and `validate` gates passed

---

## Next Steps

Session complete. Run `plansession` for Session 02.
