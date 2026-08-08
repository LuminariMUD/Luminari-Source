# LuminariMUD - Special Procedure Source Consolidation Remaining Work

**Status:** Incomplete (reopened 2026-08-08)

This document intentionally tracks unfinished work only. Completed behavior, architecture,
operational contracts, and historical validation belong in maintained project documentation. When
every item below is accepted, delete this document and remove its ongoing-project index entries in
the same change.

## Required Outcome

Finish the special-procedure ownership extraction by eliminating both transitional top-level files:

- `src/spec_assign.c`: relocate its assignment machinery and mobile, object, and room assignment
  ownership into shallow modules under `src/spec/`.
- `src/spec_procs.h`: replace its umbrella includes and declarations with narrow, directly included
  owner interfaces.

The work is not complete while either file exists. Raw numeric assignments do not justify retaining
`src/spec_assign.c`; relocating assignment ownership and converting legacy rows to declarative data
are separate concerns.

## Remaining Implementation Work

### Fold `src/spec_assign.c` into `src/spec/`

- [ ] Trace every compiled assignment, helper, public function, campaign-compatibility branch, and
  direct callback declaration in `src/spec_assign.c`; distinguish live code from commented or
  dormant inventory.
- [ ] Define a shallow `src/spec/` ownership split with a narrow public assignment header. Keep the
  public boot entry points `spec_assign_table_boot_validate()`, `assign_mobiles()`,
  `assign_objects()`, and `assign_rooms()` unless a separately tested caller migration replaces
  them.
- [ ] Move shared owner-typed assignment and effective-provenance helpers under `src/spec/` without
  weakening VNUM types, diagnostics, collision history, or assignment failure behavior.
- [ ] Move mobile, object, and room assignment inventories into coherent `src/spec/` sources.
  Cohesive zone assignment blocks may live beside their existing zone owners; reusable and
  feature-owned handlers must retain explicit owner-header dependencies.
- [ ] Preserve the exact effective boot order: named world and parser bindings, mobile assignments,
  shop wrapping, object assignments, room assignments, then quest wrapping.
- [ ] Preserve normal and `no_specials` behavior, direct-assignment precedence, shop and quest saved
  secondaries, callback results, and bounded effective-binding diagnostics.
- [ ] Make assignment source locations report their real new files and update focused expectations;
  do not retain a false `src/spec_assign.c` diagnostic label after the file is gone.
- [ ] Remove `src/spec_assign.c` and update both `Makefile.am` and `CMakeLists.txt` for every added or
  removed source.

### Eliminate `src/spec_procs.h`

- [ ] Trace every direct and transitive include of `src/spec_procs.h` and every declaration it
  supplies before changing an include.
- [ ] Put the assignment boot API in the new narrow `src/spec/` assignment header.
- [ ] Move legacy registry projection declarations to `src/spec/spec_registry.h`, or migrate their
  callers to the canonical registry API and remove the compatibility functions when no caller
  remains.
- [ ] Give `weapons_spells()` a narrow owner interface under `src/spec/` for its real cross-module
  consumers.
- [ ] Move each remaining `SPECIAL_DECL()` to its actual owner header. Use existing subsystem and
  `src/spec/` headers where they already own the implementation; add a narrow header only when a
  real cross-file consumer requires one.
- [ ] Replace umbrella inclusion with direct, path-qualified owner includes. Do not create a renamed
  all-procedure aggregation header.
- [ ] Remove `src/spec_procs.h` after the compiler and repository search prove no include or stale
  declaration depends on it.

### Reconcile Documentation and Verification

- [ ] Update current architecture, developer, source-map, and testing documentation after the two
  transitional files are removed. Remove every statement that either remains a compatibility
  surface.
- [ ] Preserve the database-first `SPECIALS` help contract unless behavior actually changes; run
  its verifier for any help text modification.
- [ ] Add or update production-linked tests for assignment module boundaries, boot ordering,
  effective source locations, direct includes, and any changed registry compatibility surface.
- [ ] Compare production and CuTest source membership exactly across Automake and CMake.

## Acceptance Gates

- [ ] `src/spec_assign.c` does not exist.
- [ ] `src/spec_procs.h` does not exist, and repository search finds no reference to it.
- [ ] No replacement umbrella header recreates the removed dependency fan-out.
- [ ] The legacy `SPECIAL` ABI, persisted names, world-file grammar, callback slots, traversal,
  scheduling, activation flags, return handling, boot precedence, and single-name persistence remain
  unchanged unless an explicit tested migration says otherwise.
- [ ] A clean warning-free Autotools build passes, followed by root `make test` and `make install`;
  `bin/circle` is executable and no root-level `circle` artifact remains.
- [ ] Fresh CMake builds of `circle` and `cutest` pass with `BUILD_TESTS=ON`, followed by all CTest
  targets.
- [ ] Documentation checks, changed-text ASCII/LF checks, `git diff --check`, and repository hooks
  pass.
- [ ] `src/campaign.h`, `src/mud_options.h`, `src/vnums.h`, `lib/.env`, and `lib/mysql_config` remain
  untouched.
- [ ] This document is deleted after all preceding gates pass, because no unfinished work remains.
