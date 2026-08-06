# Task Checklist

**Session ID**: `phase00-session03-combat-and-secondary-characterization`
**Total Tasks**: 22
**Estimated Duration**: 3-4 hours
**Created**: 2026-08-06

---

Legend: `[x]` completed; `[ ]` pending; `[P]` parallelizable; `[SNNMM]` session ref; `TNNN` task ID.

---

## Setup (2 tasks)

- [x] T001 [S0003] Verify the development environment and workflow prerequisites
  (`.spec_system/scripts/check-prereqs.sh`)
- [x] T002 [S0003] Retrace combat, identify, maneuver, shop, quest, assignment, and boot contracts
  before fixing assertions (`.spec_system/specs/phase00-session03-combat-and-secondary-characterization/implementation-notes.md`)

---

## Foundation (4 tasks)

- [x] T003 [S0003] Implement snapshot and restoration for world, index, shop, quest, command, and
  `no_specials` globals (`unittests/CuTest/test_spec_combat_secondary.c`)
- [x] T004 [S0003] Implement exact callback and nesting recorders with configurable returns
  (`unittests/CuTest/test_spec_combat_secondary.c`)
- [x] T005 [S0003] Build deterministic identify, weapon, combat, shop, and quest fixtures
  (`unittests/CuTest/test_spec_combat_secondary.c`)
- [x] T006 [S0003] Implement bounded repository source-region helpers
  (`unittests/CuTest/test_spec_combat_secondary.c`)

---

## Combat Characterization (7 tasks)

- [x] T007 [S0003] Assert item identification exact actor, owner, command, and `identify` token
  (`unittests/CuTest/test_spec_combat_secondary.c`)
- [x] T008 [S0003] Assert identification ignores zero and nonzero callback returns and bypasses
  `no_specials` (`unittests/CuTest/test_spec_combat_secondary.c`)
- [x] T009 [S0003] Assert weapon-hit exact hit token, direct-pointer gate, and wrapper return
  (`unittests/CuTest/test_spec_combat_secondary.c`)
- [x] T010 [S0003] Assert the high-level weapon caller discards the wrapper return
  (`src/combat/fight.c`, `unittests/CuTest/test_spec_combat_secondary.c`)
- [x] T011 [S0003] Assert all defense tokens and ignored returns
  (`src/combat/fight.c`, `unittests/CuTest/test_spec_combat_secondary.c`)
- [x] T012 [S0003] Assert all shield-maneuver and mounted-charge tokens and ignored returns
  (`src/combat/act.offensive.c`, `unittests/CuTest/test_spec_combat_secondary.c`)
- [x] T013 [S0003] Assert mobile combat payload, activation, ignored return, `no_specials`
  independence, and after-attacks/cleave ordering (`src/combat/fight.c`, `unittests/CuTest/test_spec_combat_secondary.c`)

---

## Secondary Composition (5 tasks)

- [x] T014 [S0003] Assert shop secondary exact incoming context and nonzero propagation
  (`unittests/CuTest/test_spec_combat_secondary.c`)
- [x] T015 [S0003] Assert shop secondary zero-return fallthrough and direct-call `no_specials`
  behavior (`unittests/CuTest/test_spec_combat_secondary.c`)
- [x] T016 [S0003] Assert quest secondary exact incoming context, zero fallthrough, and nonzero
  propagation (`unittests/CuTest/test_spec_combat_secondary.c`)
- [x] T017 [S0003] Assert quest-over-shop-over-original nesting order, unchanged context, and handled
  propagation (`unittests/CuTest/test_spec_combat_secondary.c`)
- [x] T018 [S0003] Assert assignment preservation, install order, and applicable `no_specials` boot
  gates (`src/obj/shop.c`, `src/quest/quest.c`, `src/db.c`, `unittests/CuTest/test_spec_combat_secondary.c`)

---

## Integration And Verification (4 tasks)

- [x] T019 [S0003] Add synchronized Automake and CMake CuTest membership
  (`Makefile.am`, `CMakeLists.txt`)
- [x] T020 [S0003] Pass targeted compilation, runtime, formatting, and static checks
- [x] T021 [S0003] Pass `creview` with no unresolved behavioral or maintainability finding
- [x] T022 [S0003] Pass `make test`, `make install`, artifact hygiene, encoding, validation, and
  security gates (`.spec_system/specs/phase00-session03-combat-and-secondary-characterization/`)

---

## Completion Checklist

- [x] All tasks marked `[x]`
- [x] All tests and checks passing
- [x] All files ASCII-encoded with LF line endings
- [x] implementation-notes.md updated
- [x] `creview` and `validate` gates passed

---

## Next Steps

After completion, run `plansession` for Session 04.
