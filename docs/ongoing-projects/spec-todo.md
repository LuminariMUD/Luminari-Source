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

- [x] Trace every compiled assignment, helper, public function, campaign-compatibility branch, and
  direct callback declaration in `src/spec_assign.c`; distinguish live code from commented or
  dormant inventory.
- [x] Define a shallow `src/spec/` ownership split with a narrow public assignment header. Keep the
  public boot entry points `spec_assign_table_boot_validate()`, `assign_mobiles()`,
  `assign_objects()`, and `assign_rooms()` unless a separately tested caller migration replaces
  them.
- [x] Move shared owner-typed assignment and effective-provenance helpers under `src/spec/` without
  weakening VNUM types, diagnostics, collision history, or assignment failure behavior.
- [x] Move mobile, object, and room assignment inventories into coherent `src/spec/` sources.
  Cohesive zone assignment blocks may live beside their existing zone owners; reusable and
  feature-owned handlers must retain explicit owner-header dependencies.
- [x] Preserve the exact effective boot order: named world and parser bindings, mobile assignments,
  shop wrapping, object assignments, room assignments, then quest wrapping.
- [x] Preserve normal and `no_specials` behavior, direct-assignment precedence, shop and quest saved
  secondaries, callback results, and bounded effective-binding diagnostics.
- [x] Make assignment source locations report their real new files and update focused expectations;
  do not retain a false `src/spec_assign.c` diagnostic label after the file is gone.
- [x] Remove `src/spec_assign.c` and update both `Makefile.am` and `CMakeLists.txt` for every added or
  removed source.

### Eliminate `src/spec_procs.h`

- [x] Trace every direct and transitive include of `src/spec_procs.h` and every declaration it
  supplies before changing an include.
- [x] Put the assignment boot API in the new narrow `src/spec/` assignment header.
- [x] Move legacy registry projection declarations to `src/spec/spec_registry.h`, or migrate their
  callers to the canonical registry API and remove the compatibility functions when no caller
  remains.
- [x] Give `weapons_spells()` a narrow owner interface under `src/spec/` for its real cross-module
  consumers.
- [x] Move each remaining `SPECIAL_DECL()` to its actual owner header. Use existing subsystem and
  `src/spec/` headers where they already own the implementation; add a narrow header only when a
  real cross-file consumer requires one.
- [x] Replace umbrella inclusion with direct, path-qualified owner includes. Do not create a renamed
  all-procedure aggregation header.
- [x] Remove `src/spec_procs.h` after the compiler and repository search prove no include or stale
  declaration depends on it.

### Reconcile Documentation and Verification

- [x] Update current architecture, developer, source-map, and testing documentation after the two
  transitional files are removed. Remove every statement that either remains a compatibility
  surface.
- [x] Preserve the database-first `SPECIALS` help contract unless behavior actually changes; run
  its verifier for any help text modification.
- [x] Add or update production-linked tests for assignment module boundaries, boot ordering,
  effective source locations, direct includes, and any changed registry compatibility surface.
- [x] Compare production and CuTest source membership exactly across Automake and CMake.

## Implementation Checkpoints

### Checkpoint 1 - Baseline trace and ownership plan (2026-08-08)

- Environment: `APP_ENV=development`; clean `master` matched `origin/master` at `39a1888d` before
  this work began. Protected local configuration files were not changed.
- Assignment inventory: `src/spec_assign.c` is 1,324 lines. After comment stripping it contains
  three assignment macros and 669 live `ASSIGNMOB`, `ASSIGNOBJ`, or `ASSIGNROOM` calls across the
  preserved Luminari, `CAMPAIGN_FR`, and `CAMPAIGN_DL` branches. Another 111 calls are commented
  dormant inventory. The file owns one declarative Luminari object table, five private provenance
  or table helpers, four public boot entry points, and seven local callback declarations.
- Assignment split: `src/spec/spec_assign.c` will own shared owner-typed binding helpers;
  `src/spec/spec_assign_mobiles.c`, `src/spec/spec_assign_objects.c`, and
  `src/spec/spec_assign_rooms.c` will own their inventories. `src/spec/spec_assign.h` will expose
  only the four boot entry points, and `src/spec/spec_assign_internal.h` will expose only the three
  typed helpers used by those inventory modules. Source-location macros remain local to each
  inventory so diagnostics name the real file and line.
- Umbrella inventory: `src/spec_procs.h` directly includes 46 headers and is directly included by
  52 production files and 10 production-linked CuTest files. It carries the four assignment entry
  points, five registry projection declarations, `weapons_spells()`, and 80 callback declarations.
- Declaration ownership: registry projections move to `src/spec/spec_registry.h`; general object
  callbacks and `weapons_spells()` move to `src/spec/spec_objects.h`; Neverwinter controls and
  legacy vessel callbacks receive narrow owner headers; crafting, trade, player-shop, mail, board,
  shop, quest, and object-save callbacks use their actual subsystem headers. The five declarations
  with no implementation (`drow_scimitar`, `kt_shadowmaker`, `magi_staff`, `mithril_rapier`, and
  `treantshield`) are dormant and will be removed rather than republished.
- Next implementation step: add the narrow headers and split the assignment source without
  changing boot order or assignment inventory.

### Checkpoint 2 - Source and interface consolidation (2026-08-08)

- Assignment ownership now lives in four shallow modules: shared binding helpers in
  `src/spec/spec_assign.c` and the mobile, object, and room inventories in their matching
  `src/spec/spec_assign_*.c` sources. The public assignment header exposes only the four boot entry
  points; the internal header exposes only owner-typed helpers.
- `src/spec_assign.c` and `src/spec_procs.h` are removed. All former umbrella consumers now include
  the assignment, registry, object, zone, vessel, crafting, trade, player-shop, object-save, quest,
  spell-list, skill-list, or ability owner that supplies the symbols they use. Repository search of
  production and test sources finds no transitional-path reference.
- Effective-binding provenance now names the actual mobile, object, or room inventory source.
  Focused production-linked coverage validates the new module boundaries, manifest membership,
  boot ordering, source locations, narrow public surface, and removed transitional files.
- Both build manifests contain all four new assignment sources and omit the removed top-level
  source. The incremental Autotools build passed, followed by all 590 production-linked CuTest
  cases and `make install`; the installed `bin/circle` was refreshed and the root build artifact
  was removed.
- Next implementation step: update maintained documentation, compare complete Automake/CMake source
  membership, and run clean warning-free Autotools and fresh CMake validation.

### Checkpoint 3 - Documentation and manifest reconciliation (2026-08-08)

- Maintained architecture, developer API, source-map, campaign, shop, mobile-flag, conventions,
  consideration, testing, audit, changelog, and documentation-index references now describe the
  four assignment modules and direct callback-owner interfaces. The generated mobile-flag HTML is
  synchronized with its Markdown source.
- `docs/testing/SPECIAL_PROCEDURE_PHASE_07_VALIDATION.md` records the final ownership model, narrow
  interfaces, unchanged compatibility contracts, focused coverage, manifest inventory, and live
  validation status. Earlier phase matrices remain historical evidence rather than current source
  maps.
- Exact compiled membership matches: 288 production C sources occur in both Automake and CMake, and
  the same 41 CuTest owner files occur in `cutest_SOURCES`, `cutest_test_files`, and
  `CUTEST_TEST_SOURCES`. Incidental header listings are explicitly excluded from compiled-source
  parity.
- The database-first `SPECIALS` help text and its migration/verifier SQL are unchanged, so the
  established help contract is preserved without a new help migration.
- Next implementation step: run the clean Autotools build/test/install gate, then fresh CMake/CTest,
  documentation, text-integrity, and repository-hook gates.

### Checkpoint 4 - Clean Autotools validation (2026-08-08)

- The first clean build exposed six remaining callers that had received `compute_ability()` through
  the removed umbrella. `bardic_performance.c`, `character/backgrounds.c`,
  `combat/act.offensive.c`, `combat/fight.c`, `graph.c`, and `handler.c` now include
  `character/abilities.h` directly (or `abilities.h` within `src/character/`).
- A second `make clean` plus `make -j"$(nproc)"` compiled all production sources with
  `-Wall -Wextra` and no warnings.
- Root `make test` passed all 590 cases, including the new module-boundary regression and auxiliary
  supervision, install, health, background-help, and vessel checks.
- The immediately following `make install` activated the tested release under `bin/releases/`, left
  `bin/circle` executable, and removed the root-level `circle` artifact.
- Next implementation step: run fresh CMake `circle` and `cutest` builds plus every CTest target,
  then finish documentation/integrity gates and retire this todo.

## Acceptance Gates

- [x] `src/spec_assign.c` does not exist.
- [x] `src/spec_procs.h` does not exist, and repository search finds no reference to it.
- [x] No replacement umbrella header recreates the removed dependency fan-out.
- [x] The legacy `SPECIAL` ABI, persisted names, world-file grammar, callback slots, traversal,
  scheduling, activation flags, return handling, boot precedence, and single-name persistence remain
  unchanged unless an explicit tested migration says otherwise.
- [x] A clean warning-free Autotools build passes, followed by root `make test` and `make install`;
  `bin/circle` is executable and no root-level `circle` artifact remains.
- [ ] Fresh CMake builds of `circle` and `cutest` pass with `BUILD_TESTS=ON`, followed by all CTest
  targets.
- [ ] Documentation checks, changed-text ASCII/LF checks, `git diff --check`, and repository hooks
  pass.
- [ ] `src/campaign.h`, `src/mud_options.h`, `src/vnums.h`, `lib/.env`, and `lib/mysql_config` remain
  untouched.
- [ ] This document is deleted after all preceding gates pass, because no unfinished work remains.
