# Task Checklist

**Session ID**: `phase00-session04-validated-definition-registry`
**Total Tasks**: 24
**Estimated Duration**: 3-4 hours
**Created**: 2026-08-06

---

Legend: `[x]` completed; `[ ]` pending; `[P]` parallelizable; `[SNNMM]` session ref; `TNNN` task ID.

---

## Setup (3 tasks)

- [x] T001 [S0004] Verify the development environment and workflow prerequisites
  (`.spec_system/scripts/check-prereqs.sh`)
- [x] T002 [S0004] Confirm Sessions 01-03 are complete and the worktree is clean at the recorded
  base commit (`.spec_system/state.json`, `git status`)
- [x] T003 [S0004] Retrace every registered handler's owners, events, activation prerequisites, and
  binding sources before populating metadata
  (`.spec_system/specs/phase00-session04-validated-definition-registry/implementation-notes.md`)

---

## Registry Foundation (6 tasks)

- [x] T004 [S0004] Define public owner, event, prototype, placement, binding, visibility, typed
  handler, event-contract, and definition types (`src/spec/spec_registry.h`)
- [x] T005 [S0004] Define bounds-safe canonical iteration and lookup declarations
  (`src/spec/spec_registry.h`)
- [x] T006 [S0004] Populate the immutable 28-definition canonical registry with complete metadata
  (`src/spec/spec_registry.c`)
- [x] T007 [S0004] Model `Guildmaster` as an explicit alias without changing `Guild` reverse lookup
  (`src/spec/spec_registry.c`)
- [x] T008 [S0004] Implement the historical 29-name compatibility projection and legacy accessor
  wrappers (`src/spec/spec_registry.c`)
- [x] T009 [S0004] Remove the superseded sentinel table and unsafe accessors while preserving all
  assignment logic (`src/spec_assign.c`)

---

## Validation And Access (6 tasks)

- [x] T010 [S0004] Implement bounded, case-insensitive canonical and alias lookup plus canonical
  handler reverse lookup (`src/spec/spec_registry.c`)
- [x] T011 [S0004] Implement owner-aware and event-aware compatibility accessors with invalid-mask
  rejection (`src/spec/spec_registry.c`)
- [x] T012 [S0004] Validate all required strings and case-insensitive canonical/alias uniqueness
  (`src/spec/spec_registry.c`)
- [x] T013 [S0004] Validate owner, event, prerequisite, placement, binding-source, and visibility
  domains and owner/event compatibility (`src/spec/spec_registry.c`)
- [x] T014 [S0004] Validate event-array shape, event uniqueness, and exactly one legacy or typed
  handler with actionable bounded diagnostics (`src/spec/spec_registry.c`)
- [x] T015 [S0004] Invoke fatal production-registry validation before `boot_world()`
  (`src/db.c`)

---

## Production-Linked Verification (5 tasks)

- [x] T016 [S0004] Assert complete production metadata, exact canonical inventory, and exact handler
  identities (`unittests/CuTest/test_spec_registry_validation.c`)
- [x] T017 [S0004] Assert alias, reverse, owner, event, and legacy compatibility behavior
  (`unittests/CuTest/test_spec_registry_validation.c`)
- [x] T018 [S0004] Assert extreme boundary and invalid mask safety
  (`unittests/CuTest/test_spec_registry_validation.c`)
- [x] T019 [S0004] Assert deterministic rejection and diagnostics for every malformed metadata family
  (`unittests/CuTest/test_spec_registry_validation.c`)
- [x] T020 [S0004] Assert registry boot validation precedes world parsing
  (`src/db.c`, `unittests/CuTest/test_spec_registry_validation.c`)

---

## Integration And Verification (4 tasks)

- [x] T021 [S0004] Add synchronized production and CuTest membership to Automake and CMake
  (`Makefile.am`, `CMakeLists.txt`)
- [x] T022 [S0004] Document the immutable registry and extension contract
  (`docs/guides/DEVELOPER_GUIDE_AND_API.md`, `docs/guides/OLC_SpecProcs.md`)
- [x] T023 [S0004] Pass targeted compilation, runtime, formatting, static, and `creview` checks
- [x] T024 [S0004] Pass `make test`, CMake `production-cutest`, `make install`, artifact hygiene,
  encoding, validation, and security gates
  (`.spec_system/specs/phase00-session04-validated-definition-registry/`)

---

## Completion Checklist

- [x] All tasks marked `[x]`
- [x] All tests and checks passing
- [x] All files ASCII-encoded with LF line endings
- [x] implementation-notes.md updated
- [x] `creview` and `validate` gates passed

---

## Next Steps

After completion, run `plansession` for Session 05.
