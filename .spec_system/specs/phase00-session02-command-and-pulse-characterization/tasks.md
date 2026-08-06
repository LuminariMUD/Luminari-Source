# Task Checklist

**Session ID**: `phase00-session02-command-and-pulse-characterization`
**Total Tasks**: 24
**Estimated Duration**: 3-4 hours
**Created**: 2026-08-06

---

Legend: `[x]` completed; `[ ]` pending; `[P]` parallelizable; `[SNNMM]` session ref; `TNNN` task ID.

---

## Setup (2 tasks)

- [x] T001 [S0002] Verify the development environment and workflow prerequisites
  (`.spec_system/scripts/check-prereqs.sh`)
- [x] T002 [S0002] Retrace command traversal, mobile activity, object auto-proc, moving-room, and
  heartbeat contracts before fixing assertions
  (`.spec_system/specs/phase00-session02-command-and-pulse-characterization/implementation-notes.md`)

---

## Foundation (5 tasks)

- [x] T003 [S0002] Implement snapshot and restoration for world, index, list, moving-room,
  `no_specials`, and command-table globals (`unittests/CuTest/test_spec_command_pulse.c`)
- [x] T004 [S0002] Implement a bounded callback recorder with per-owner return configuration
  (`unittests/CuTest/test_spec_command_pulse.c`)
- [x] T005 [S0002] Build deterministic room, equipment, inventory, mobile, and room-object command
  owners (`unittests/CuTest/test_spec_command_pulse.c`)
- [x] T006 [S0002] Build deterministic mobile-AI, object auto-proc, and moving-room fixtures
  (`unittests/CuTest/test_spec_command_pulse.c`)
- [x] T007 [S0002] Implement a bounded repository source-contract helper
  (`unittests/CuTest/test_spec_command_pulse.c`)

---

## Command Characterization (5 tasks)

- [x] T008 [S0002] Assert complete room, equipment, inventory, mobile, and room-object traversal order
  (`unittests/CuTest/test_spec_command_pulse.c`)
- [x] T009 [S0002] Assert exact actor, owner, command, and argument payloads
  (`unittests/CuTest/test_spec_command_pulse.c`)
- [x] T010 [S0002] Assert every first-nonzero stop position and returned handled result
  (`unittests/CuTest/test_spec_command_pulse.c`)
- [x] T011 [S0002] Assert `NOWHERE` rejection and pending-extraction mobile skipping
  (`unittests/CuTest/test_spec_command_pulse.c`)
- [x] T012 [S0002] Assert direct mobile-pointer invocation without `MOB_SPEC` and normal versus
  `no_specials` command behavior (`unittests/CuTest/test_spec_command_pulse.c`)

---

## Mobile Activity Characterization (3 tasks)

- [x] T013 [S0002] Assert mobile callback payload and nonzero suppression of default AI
  (`unittests/CuTest/test_spec_command_pulse.c`)
- [x] T014 [S0002] Assert zero return permits deterministic default-AI fallback
  (`unittests/CuTest/test_spec_command_pulse.c`)
- [x] T015 [S0002] Assert `MOB_SPEC`, `no_specials`, and missing-pointer activation behavior
  (`unittests/CuTest/test_spec_command_pulse.c`)

---

## Object Auto-Proc Characterization (4 tasks)

- [x] T016 [S0002] Assert a handled worn auto-proc receives only its wearer
  (`unittests/CuTest/test_spec_command_pulse.c`)
- [x] T017 [S0002] Assert carried and unowned null-actor first calls plus zero-return fallback
  (`unittests/CuTest/test_spec_command_pulse.c`)
- [x] T018 [S0002] Assert `ITEM_AUTOPROC`, inert-weapon, and callback-pointer gates
  (`unittests/CuTest/test_spec_command_pulse.c`)
- [x] T019 [S0002] Assert `no_specials` does not suppress object auto-procs
  (`unittests/CuTest/test_spec_command_pulse.c`)

---

## Moving Rooms And Scheduling (3 tasks)

- [x] T020 [S0002] Assert moving-room countdown, reset, exact null payload, and ignored returns
  (`unittests/CuTest/test_spec_command_pulse.c`)
- [x] T021 [S0002] Assert `no_specials` does not suppress moving-room callbacks
  (`unittests/CuTest/test_spec_command_pulse.c`)
- [x] T022 [S0002] Assert ten-second moving-room scheduling and mobile-before-object pulse order
  (`src/comm.c`, `unittests/CuTest/test_spec_command_pulse.c`)

---

## Integration And Verification (2 tasks)

- [x] T023 [S0002] Add synchronized Automake and CMake CuTest membership and pass targeted review,
  formatting, and static checks (`Makefile.am`, `CMakeLists.txt`)
- [x] T024 [S0002] Pass `make test`, `make install`, artifact hygiene, encoding, validation, and
  security gates (`.spec_system/specs/phase00-session02-command-and-pulse-characterization/`)

---

## Completion Checklist

- [x] All tasks marked `[x]`
- [x] All tests and checks passing
- [x] All files ASCII-encoded with LF line endings
- [x] implementation-notes.md updated
- [x] `creview` and `validate` gates passed

---

## Next Steps

After completion, run `plansession` for Session 03.
