# Implementation Notes

**Session ID**: `phase00-session01-registry-and-persistence-characterization`
**Started**: 2026-08-06 21:38
**Last Updated**: 2026-08-06 22:31

---

## Session Progress

| Metric | Value |
|--------|-------|
| Tasks Completed | 23 / 23 |
| Estimated Remaining | 0 minutes |
| Blockers | 0 |

---

## Task Log

### Task T001 - Verify the development environment and workflow prerequisites

**Started**: 2026-08-06 21:38
**Completed**: 2026-08-06 21:38
**Duration**: <1 minute

**Notes**:
- Confirmed the active session from deterministic state and a single-repository development scope.
- Confirmed spec-system, jq, and Git prerequisites; no environment issues were reported.

**Files Changed**:
- `.spec_system/specs/phase00-session01-registry-and-persistence-characterization/implementation-notes.md` - initialized the implementation record.

**Verification**:
- Command/check: `bash .spec_system/scripts/analyze-project.sh --json`
  - Result: PASS - active session and both planning artifacts were reported.
  - Evidence: `current_session_dir_exists` is true and `current_session_files` contains `spec.md` and `tasks.md`.
- Command/check: `bash .spec_system/scripts/check-prereqs.sh --json --env`
  - Result: PASS - overall status is `pass` with no issues.
- UI product-surface check: N/A - no UI surface changed.
- UI craft check: N/A - no UI surface changed.

---

## Checkpoint

### Task T002 - Record the registry and named world-binding inventory

**Started**: 2026-08-06 21:38
**Completed**: 2026-08-06 21:39
**Duration**: 1 minute

**Notes**:
- Recorded 29 indexed names backed by 28 distinct legacy handlers; `Guild` and `Guildmaster` share
  the only duplicate handler pointer.
- Recorded five named world bindings: `Postmaster` once in `lib/world/mob/12.mob`, `Greyhawk Ship`
  in `lib/world/obj/14.obj` and `lib/world/obj/700.obj`, and `Greyhawk Ship Commands` in
  `lib/world/wld/14.wld` and `lib/world/wld/700.wld`.
- Captured the pre-test world-data digest as
  `421b3ed685339943a3d81a78aaced9f262b23bb0e2e921759a712affb890b509`.

**Files Changed**:
- `.spec_system/specs/phase00-session01-registry-and-persistence-characterization/implementation-notes.md` - recorded the compatibility inventory.

**Verification**:
- Command/check: `awk` inventory over `spec_func_list[]` in `src/spec_assign.c`
  - Result: PASS - 29 ordered rows and 28 unique handler tokens.
- Command/check: `rg --no-ignore -n '^SpecProc:|^Z$' lib/world/{mob,obj,wld}`
  - Result: PASS - one mobile, two object, and two room occurrences with the expected names.
- Command/check: `git status --short -- lib/world`
  - Result: PASS - no checked-in world-data modification was reported.
- UI product-surface check: N/A - no UI surface changed.
- UI craft check: N/A - no UI surface changed.

---

## Checkpoint

### Task T003 - Define the reusable fixture contracts

**Started**: 2026-08-06 21:40
**Completed**: 2026-08-06 21:41
**Duration**: 1 minute

**Notes**:
- Defined one opaque fixture API for lifecycle, named production loading and saving, owner-specific
  handler inspection, and real OLC parser execution.
- Kept mutable fixture internals private so later suites cannot bypass restoration invariants.

**Files Changed**:
- `unittests/CuTest/test_spec_fixtures.h` - added the shared fixture contract.

**Verification**:
- Command/check: `sed -n '1,200p' unittests/CuTest/test_spec_fixtures.h`
  - Result: PASS - every planned loader, writer, owner, OLC, and teardown operation has a typed API.
- Command/check: `LC_ALL=C rg -n '[^\\x00-\\x7F]' unittests/CuTest/test_spec_fixtures.h`
  - Result: PASS - no non-ASCII bytes found.
- UI product-surface check: N/A - test support only.
- UI craft check: N/A - test support only.

---

## Checkpoint

### Task T004 - Implement global and working-directory snapshot and restoration

**Started**: 2026-08-06 21:41
**Completed**: 2026-08-06 21:43
**Duration**: 2 minutes

**Notes**:
- Added an opaque fixture with isolated world, prototype, index, zone, descriptor, builder, and OLC
  storage.
- Snapshot and teardown restore every replaced global pointer, top index, diagonal-direction and
  wilderness setting, and the original working directory.
- Cleanup owns only parser-created strings and trail data, and accepts null or partial fixtures.

**Files Changed**:
- `unittests/CuTest/test_spec_fixtures.c` - implemented lifecycle and restoration.

**Verification**:
- Command/check: `gcc -std=gnu2x -DLUMINARI_CUTEST -Wall -Wextra -Isrc -Iunittests/CuTest -fsyntax-only unittests/CuTest/test_spec_fixtures.c`
  - Result: PASS - fixture lifecycle compiles with warnings enabled.
- Command/check: `clang-format --dry-run --Werror unittests/CuTest/test_spec_fixtures.c`
  - Result: PASS - source matches repository formatting.
- BQC resource cleanup: PASS - destroy restores the working directory and all replaced globals,
  frees loaded owner data, saved text, and the fixture allocation.
- UI product-surface check: N/A - test support only.
- UI craft check: N/A - test support only.

---

## Checkpoint

### Task T005 - Implement the private filesystem sandbox and bounded file reading

**Started**: 2026-08-06 21:43
**Completed**: 2026-08-06 21:45
**Duration**: 2 minutes

**Notes**:
- Added a mode-0700 `mkdtemp()` sandbox with only the relative directories required by production
  OLC writers.
- Added explicit file and directory cleanup and a 1 MiB bounded reader for saved fixture files.
- Kept all destructive cleanup limited to six explicit files under the resolved sandbox path.

**Files Changed**:
- `unittests/CuTest/test_spec_fixtures.c` - added sandbox creation, bounded reads, and cleanup.

**Verification**:
- Command/check: `gcc -std=gnu2x -DLUMINARI_CUTEST -Wall -Wextra -Isrc -Iunittests/CuTest -fsyntax-only unittests/CuTest/test_spec_fixtures.c`
  - Result: PASS - sandbox and reader code has no syntax or warning output.
- Command/check: `clang-format --dry-run --Werror unittests/CuTest/test_spec_fixtures.c unittests/CuTest/test_spec_fixtures.h`
  - Result: PASS - fixture files match repository formatting.
- BQC resource cleanup: PASS - fixed the read path to call `fclose()` even when `fread()` is short,
  and partial sandbox creation is removed by fixture destruction.
- BQC failure path completeness: PASS - allocation, path length, directory, open, size, read, and
  close failures return bounded caller-visible errors.
- UI product-surface check: N/A - test support only.
- UI craft check: N/A - test support only.

---

## Checkpoint

### Task T006 - Load minimal named records through the production parsers

**Started**: 2026-08-06 21:45
**Completed**: 2026-08-06 21:47
**Duration**: 2 minutes

**Notes**:
- Added current-format in-memory enhanced-mobile, object, and room records with the three known
  persisted procedure names.
- The fixture calls `parse_mobile()`, `parse_object()`, and `parse_room()` directly and rejects null
  or repeated loads because the legacy parsers retain private static indexes.
- Added owner-typed access to the callback slots populated by those production parsers.

**Files Changed**:
- `unittests/CuTest/test_spec_fixtures.c` - added record streams, production loads, and handler access.

**Verification**:
- Command/check: `gcc -std=gnu2x -DLUMINARI_CUTEST -Wall -Wextra -Isrc -Iunittests/CuTest -fsyntax-only unittests/CuTest/test_spec_fixtures.c`
  - Result: PASS - all three production parser calls compile with their real declarations.
- Command/check: inspected fixture records against `lib/world/mob/12.mob`, `lib/world/obj/14.obj`,
  and `lib/world/wld/14.wld`
  - Result: PASS - field counts, terminators, `E`/`Z` markers, and persisted names match current valid records.
- BQC state freshness: PASS - a second load is rejected explicitly instead of reusing stale parser state.
- BQC failure path completeness: PASS - temporary stream create, write, seek, and close failures are
  reported and teardown remains valid after a partial load.
- UI product-surface check: N/A - test support only.
- UI craft check: N/A - test support only.

---

## Checkpoint

### Task T007 - Initialize reusable descriptor and OLC parser state

**Started**: 2026-08-06 21:47
**Completed**: 2026-08-06 21:49
**Duration**: 2 minutes

**Notes**:
- Added deterministic descriptor, builder, player-special, and `oasis_olc_data` initialization for
  each owner type.
- Routed mutable bounded copies of test input through the real `medit_parse()`, `oedit_parse()`, and
  `redit_parse()` functions.
- Used fixture-owned large output storage so menu rendering never acquires a global output-pool block.

**Files Changed**:
- `unittests/CuTest/test_spec_fixtures.c` - added descriptor and real OLC parser support.

**Verification**:
- Command/check: `gcc -std=gnu2x -DLUMINARI_CUTEST -Wall -Wextra -Isrc -Iunittests/CuTest -fsyntax-only unittests/CuTest/test_spec_fixtures.c`
  - Result: PASS - all OLC mode constants, owner pointers, and parser calls compile cleanly.
- Command/check: `clang-format --dry-run --Werror unittests/CuTest/test_spec_fixtures.c unittests/CuTest/test_spec_fixtures.h`
  - Result: PASS - fixture sources match repository formatting.
- BQC resource cleanup: PASS - OLC output is fixture-owned and requires no global buffer-pool cleanup.
- BQC contract alignment: PASS - input is copied into the mutable, bounded buffer required by the
  existing parser signatures and invalid owners are rejected.
- UI product-surface check: N/A - production menus are exercised but not changed.
- UI craft check: N/A - production menus are exercised but not changed.

---

## Checkpoint

### Task T008 - Assert the exact current registry count and sequence

**Started**: 2026-08-06 21:49
**Completed**: 2026-08-06 21:50
**Duration**: 1 minute

**Notes**:
- Added an ordered 29-name compatibility inventory and an assertion against the production count.
- The assertion deliberately identifies legacy row order so Session 04 must migrate alias layout
  intentionally rather than changing persisted output accidentally.

**Files Changed**:
- `unittests/CuTest/test_spec_registry_persistence.c` - added registry inventory coverage.

**Verification**:
- Command/check: `gcc -std=gnu2x -DLUMINARI_CUTEST -Wall -Wextra -Isrc -Iunittests/CuTest -fsyntax-only unittests/CuTest/test_spec_registry_persistence.c`
  - Result: PASS - the test compiles against production registry declarations without warnings.
- Command/check: inspected the expected array against the T002 `spec_func_list[]` inventory
  - Result: PASS - all 29 names are present once and in current index order.
- UI product-surface check: N/A - test-only change.
- UI craft check: N/A - test-only change.

---

## Checkpoint

### Task T009 - Assert case-insensitive lookup and rejected inputs

**Started**: 2026-08-06 21:50
**Completed**: 2026-08-06 21:51
**Duration**: 1 minute

**Notes**:
- Added mixed-case lookup assertions for a mobile and object definition.
- Added explicit null, empty, and unknown-name rejection assertions.

**Files Changed**:
- `unittests/CuTest/test_spec_registry_persistence.c` - added lookup-input coverage.

**Verification**:
- Command/check: `gcc -std=gnu2x -DLUMINARI_CUTEST -Wall -Wextra -Isrc -Iunittests/CuTest -fsyntax-only unittests/CuTest/test_spec_registry_persistence.c`
  - Result: PASS - lookup assertions compile against actual handler symbols.
- Contract inspection: `find_spec_func_by_name()` in `src/spec_assign.c`
  - Result: PASS - every null/empty/unknown and mixed-case branch is covered by the new test.
- UI product-surface check: N/A - test-only change.
- UI craft check: N/A - test-only change.

---

## Checkpoint

### Task T010 - Assert Guild alias and reverse-lookup behavior

**Started**: 2026-08-06 21:51
**Completed**: 2026-08-06 21:52
**Duration**: 1 minute

**Notes**:
- Proved that both persisted names resolve to the same `guild` handler.
- Proved reverse lookup emits `Guild`, the first current table row for that shared pointer.

**Files Changed**:
- `unittests/CuTest/test_spec_registry_persistence.c` - added alias and reverse-lookup coverage.

**Verification**:
- Command/check: `gcc -std=gnu2x -DLUMINARI_CUTEST -Wall -Wextra -Isrc -Iunittests/CuTest -fsyntax-only unittests/CuTest/test_spec_registry_persistence.c`
  - Result: PASS - function-pointer equality and reverse-name assertions compile cleanly.
- Command/check: `clang-format --dry-run --Werror unittests/CuTest/test_spec_registry_persistence.c`
  - Result: PASS - test source matches repository formatting.
- UI product-surface check: N/A - test-only change.
- UI craft check: N/A - test-only change.

---

## Checkpoint

### Task T011 - Assert lower and sentinel accessor bounds

**Started**: 2026-08-06 21:52
**Completed**: 2026-08-06 21:53
**Duration**: 1 minute

**Notes**:
- Added negative-index and count-sentinel assertions for both name and function accessors.
- Added a last-valid-row assertion and null reverse-lookup coverage without probing the known unsafe
  arbitrary-high legacy index behavior reserved for Session 04.

**Files Changed**:
- `unittests/CuTest/test_spec_registry_persistence.c` - added legacy accessor boundaries.

**Verification**:
- Command/check: `unittests/CuTest/make-tests.sh unittests/CuTest/test_spec_registry_persistence.c | rg 'SUITE_ADD_TEST'`
  - Result: PASS - all four registry tests are discovered by the production CuTest generator.
- Command/check: `gcc -std=gnu2x -DLUMINARI_CUTEST -Wall -Wextra -Isrc -Iunittests/CuTest -fsyntax-only unittests/CuTest/test_spec_fixtures.c unittests/CuTest/test_spec_registry_persistence.c`
  - Result: PASS - the fixture and registry block compile cleanly together.
- UI product-surface check: N/A - test-only change.
- UI craft check: N/A - test-only change.

---

## Checkpoint - Registry Block

- Completed: 11 / 23 tasks.
- Checks: both new sources pass warning-enabled syntax checks; four tests are generator-discoverable.
- Scope review: registry count, ordering, lookup, alias, reverse lookup, and defined legacy bounds
  match Objectives 1 and the Session 01 compatibility boundary.
- **Next Task**: T012 - Assert production mobile binding parsing.

### Task T012 - Assert production mobile binding parsing

**Started**: 2026-08-06 21:53
**Completed**: 2026-08-06 21:55
**Duration**: 2 minutes

**Notes**:
- Added a production-loader integration scenario that checks the mobile callback before fixture
  teardown and reports the result to the parent test only after cleanup.
- The test asserts the `SpecProc: Postmaster` field resolves to the real `postmaster` handler.

**Files Changed**:
- `unittests/CuTest/test_spec_registry_persistence.c` - added mobile loader characterization.

**Verification**:
- Command/check: warning-enabled syntax compile of both new CuTest sources
  - Result: PASS - the production mobile parser fixture and handler assertion compile cleanly.
- Command/check: CuTest generator discovery
  - Result: PASS - `Test_spec_world_binding_loaders_resolve_known_names` is registered.
- BQC resource cleanup: PASS - the scenario records any handler mismatch, completes fixture teardown,
  and only then returns the result for the parent CuTest assertion.
- UI product-surface check: N/A - test-only change.
- UI craft check: N/A - test-only change.

---

**Next Task**: T013 - Assert production object binding parsing.

### Task T013 - Assert production object binding parsing

**Started**: 2026-08-06 21:55
**Completed**: 2026-08-06 21:55
**Duration**: <1 minute

**Notes**:
- Extended the managed loader assertion to the object `Z` field and captured its callback before teardown.
- The test asserts `Greyhawk Ship` resolves to the real `greyhawk_ship_object` handler.

**Files Changed**:
- `unittests/CuTest/test_spec_registry_persistence.c` - added object loader characterization.

**Verification**:
- Command/check: warning-enabled syntax compile of both new CuTest sources
  - Result: PASS - the production object parser fixture and handler assertion compile cleanly.
- Contract inspection: `case 'Z'` in `parse_object()` (`src/db.c`)
  - Result: PASS - the test stream reaches the production named-binding assignment branch.
- UI product-surface check: N/A - test-only change.
- UI craft check: N/A - test-only change.

---

**Next Task**: T014 - Assert production room binding parsing.

### Task T014 - Assert production room binding parsing

**Started**: 2026-08-06 21:55
**Completed**: 2026-08-06 21:56
**Duration**: 1 minute

**Notes**:
- Extended the managed loader assertion to the room `Z` field and captured its callback before teardown.
- The test asserts `Greyhawk Ship Commands` resolves to `greyhawk_ship_commands` and verifies repeated
  parser fixture loading is explicitly rejected.

**Files Changed**:
- `unittests/CuTest/test_spec_registry_persistence.c` - added room loader characterization and stale-state guard coverage.

**Verification**:
- Command/check: `gcc -std=gnu2x -DLUMINARI_CUTEST -Wall -Wextra -Isrc -Iunittests/CuTest -fsyntax-only unittests/CuTest/test_spec_fixtures.c unittests/CuTest/test_spec_registry_persistence.c`
  - Result: PASS - all three loader paths and assertions compile cleanly together.
- Contract inspection: `case 'Z'` in `parse_room()` (`src/db.c`)
  - Result: PASS - the test stream reaches the production room callback assignment branch.
- BQC state freshness: PASS - the fixture's private parser indexes cannot be reused through a second load.
- UI product-surface check: N/A - test-only change.
- UI craft check: N/A - test-only change.

---

**Next Task**: T015 - Save all three bindings through production writers.

### Task T015 - Save all three bindings through production writers

**Started**: 2026-08-06 21:56
**Completed**: 2026-08-06 22:04
**Duration**: 8 minutes

**Notes**:
- Called the production mobile, object, and room writers inside a mode-0700 temporary world tree.
- Asserted the canonical `SpecProc: Postmaster`, `Z`/`Greyhawk Ship`, and
  `Z`/`Greyhawk Ship Commands` fields in the saved files.
- Ran parser-backed scenarios in child processes after the first integration run demonstrated that
  the loaders' private static counters otherwise outlive fixture pointer restoration.

**Files Changed**:
- `unittests/CuTest/test_spec_fixtures.c` - completed writer execution, bounded reads, and cleanup.
- `unittests/CuTest/test_spec_registry_persistence.c` - added isolated canonical-save assertions.

**Verification**:
- Command/check: `make cutest && LUMINARI_TEST_ROOT="$PWD" ./cutest`
  - Result: PASS - all 481 production-linked CuTests passed.
- BQC resource cleanup: PASS - child-process isolation contains parser static state; fixture teardown
  restores globals and cwd and destroys descriptor protocol state, while the parent independently
  removes the explicit sandbox files after every normal or abnormal child exit.
- UI product-surface check: N/A - production persistence paths are characterized but unchanged.
- UI craft check: N/A - no UI surface changed.

---

**Next Task**: T016 - Assert the checked-in named-binding inventory.

### Task T016 - Assert the checked-in named-binding inventory

**Started**: 2026-08-06 22:00
**Completed**: 2026-08-06 22:04
**Duration**: 4 minutes

**Notes**:
- Added a read-only directory scan across every checked-in `.mob`, `.obj`, and `.wld` file.
- Asserted one mobile field, two object fields, and two room fields, and verified every occurrence
  uses the expected current persisted name.

**Files Changed**:
- `unittests/CuTest/test_spec_registry_persistence.c` - added source-inventory scanning and assertions.

**Verification**:
- Command/check: generated root `cutest`
  - Result: PASS - `Test_spec_world_binding_source_inventory` passed in the 481-test suite.
- BQC failure handling: PASS - open, enumeration, read, incomplete field, and close failures produce
  bounded assertion messages without repository writes.
- UI product-surface check: N/A - test-only change.
- UI craft check: N/A - test-only change.

---

**Next Task**: T017 - Characterize medit selection behavior.

### Tasks T017-T019 - Characterize all three OLC selection branches

**Started**: 2026-08-06 22:00
**Completed**: 2026-08-06 22:04
**Duration**: 4 minutes

**Notes**:
- Exercised real `medit_parse()`, `oedit_parse()`, and `redit_parse()` branches with valid current
  selections, lower and upper invalid bounds, quit, and explicit zero clear.
- Initialized and destroyed real descriptor protocol state because all three production menus route
  output through protocol formatting.
- Verified valid selections set the expected owner handler and changed flag, invalid and quit inputs
  preserve both handler and clean state, and zero clears the handler while marking the edit changed.

**Files Changed**:
- `unittests/CuTest/test_spec_fixtures.c` - completed descriptor protocol and OLC parser lifecycle.
- `unittests/CuTest/test_spec_registry_persistence.c` - added the three owner scenarios.

**Verification**:
- Command/check: `make cutest && LUMINARI_TEST_ROOT="$PWD" ./cutest`
  - Result: PASS - all three new editor tests and all 478 pre-existing tests passed (481 total).
- BQC state freshness: PASS - each owner scenario receives a fresh process, fixture, descriptor,
  parser state, output buffer, and temporary world directory.
- UI product-surface check: PASS - builder-facing menu branches render through the production output path.
- UI craft check: N/A - no visual UI surface changed.

---

**Next Task**: T020 - Synchronize build manifests.

### Task T020 - Synchronize Automake and CMake test membership

**Started**: 2026-08-06 21:53
**Completed**: 2026-08-06 22:04
**Duration**: 11 minutes

**Notes**:
- Added both new sources to `cutest_SOURCES`, `cutest_test_files`, and `CUTEST_TEST_SOURCES`.
- Confirmed the generated registry discovers all nine planned `Test_spec_*` functions; the abnormal
  cleanup regression added during `creview` raises the final session total to ten.

**Files Changed**:
- `Makefile.am` - added compile and generated-test inputs.
- `CMakeLists.txt` - added matching CuTest sources.

**Verification**:
- Command/check: incremental `make cutest`
  - Result: PASS - both sources compiled with `-Wall -Wextra`, linked into `cutest`, and generated
    tests executed.
- Manifest parity inspection: PASS - Automake and CMake contain the same two source paths.
- UI product-surface check: N/A - build metadata only.
- UI craft check: N/A - build metadata only.

---

**Next Task**: T021 - Run the full root test target with zero new warnings.

### Task T021 - Run the full production-linked test target

**Started**: 2026-08-06 22:05
**Completed**: 2026-08-06 22:06
**Duration**: 1 minute

**Notes**:
- Regenerated `AllTests.c` from the synchronized test manifest and ran every root test prerequisite
  plus the production-linked CuTest executable.
- The fixture and characterization sources compile cleanly with the target's `-Wall -Wextra` flags.

**Files Changed**:
- `unittests/CuTest/AllTests.c` - regenerated build artifact; not a tracked source change.

**Verification**:
- Command/check: `make test`
  - Result: PASS - autorun, install-script, help, vessel, process-memory, benchmark parser checks,
    and all 481 CuTests passed.
- Compiler check: PASS - no new compiler warnings were emitted by the new sources.
- UI product-surface check: N/A - test-only session.
- UI craft check: N/A - no UI surface changed.

---

**Next Task**: T022 - Install the tested server and verify artifacts.

### Task T022 - Install the tested server and verify artifacts

**Started**: 2026-08-06 22:06
**Completed**: 2026-08-06 22:07
**Duration**: 1 minute

**Notes**:
- Installed the exact server build exercised by the root test target through the versioned binary
  installer.
- Confirmed `bin/circle` resolves to the new build-ID release and the root `circle` artifact was removed.

**Files Changed**:
- `bin/releases/4b1ce2285eb8dd9282f89d14693011d5a059b6f5/circle` - installed ignored build artifact.
- `bin/circle` - activated ignored symlink managed by the installer.

**Verification**:
- Command/check: `make install`
  - Result: PASS - installed build ID `4b1ce2285eb8dd9282f89d14693011d5a059b6f5` with SHA-256
    `8eeaa1f51912ee4d75bd161a11594ec8d096cb7cefe62bc23e2b9e4c80ed1cae`.
- Command/check: `test -x bin/circle && test -L bin/circle && test ! -e circle`
  - Result: PASS - installed alias is executable and no root-level binary remains.
- UI product-surface check: N/A - build artifact only.
- UI craft check: N/A - no UI surface changed.

---

**Next Task**: T023 - Record final format, inventory, and artifact evidence.

### Task T023 - Record final format, inventory, and artifact evidence

**Started**: 2026-08-06 22:07
**Completed**: 2026-08-06 22:08
**Duration**: 1 minute

**Notes**:
- Recomputed the checked-in `.mob`, `.obj`, and `.wld` digest and matched the pre-test value exactly.
- Checked every tracked modification and untracked session file for ASCII encoding and LF endings.
- Removed three empty temporary sandboxes left by the intentionally diagnosed pre-fix child crashes;
  the passing suite leaves no fixture sandbox behind.
- Re-read the session objectives and success criteria; all delivered changes remain test-only and
  within the planned characterization scope.

**Files Changed**:
- `.spec_system/specs/phase00-session01-registry-and-persistence-characterization/tasks.md` - completed the checklist.
- `.spec_system/specs/phase00-session01-registry-and-persistence-characterization/implementation-notes.md` - recorded final evidence.

**Verification**:
- Command/check: world digest over all top-level `.mob`, `.obj`, and `.wld` files
  - Result: PASS - digest remained
    `421b3ed685339943a3d81a78aaced9f262b23bb0e2e921759a712affb890b509`.
- Command/check: `git diff --check`, changed-file ASCII/CR scan, and `clang-format --dry-run --Werror`
  - Result: PASS - no whitespace errors, non-ASCII bytes, CR bytes, or formatting drift.
- Command/check: manifest, generated-test, sandbox, and binary artifact assertions
  - Result: PASS - manifest counts are 2/2/1/1, the nine initially planned tests are generated, no
    sandbox remains, `bin/circle` is active, and root `circle` is absent. The later code-review
    regression raises the final session total to ten generated tests.
- UI product-surface check: N/A - test-only session.
- UI craft check: N/A - no UI surface changed.

---

## Blockers & Solutions

### Blocker 1: Legacy loader state escaped pointer restoration

**Description**: The first full run showed that `parse_mobile()`, `parse_object()`, and
`parse_room()` keep private static indexes; restoring world pointers alone made a later parser test
order-dependent.
**Impact**: The initial `make test` run failed in the existing minimal-world parser case.
**Resolution**: Execute every parser-backed scenario in a bounded child process and return a compact
result over a pipe, preserving production code and parent parser state.
**Time Lost**: 5 minutes.

### Blocker 2: OLC menu output required descriptor protocol state

**Description**: The first editor run reached production menu rendering with a null protocol object.
**Impact**: The three new editor child processes exited before reporting results.
**Resolution**: Complete the fixture's real descriptor lifecycle with `ProtocolCreate()` and
`ProtocolDestroy()`; all three editor scenarios then passed.
**Time Lost**: 3 minutes.

## Design Decisions

### Decision 1: Isolate private parser counters at the process boundary

**Context**: The public production loaders are required for compatibility evidence but expose no
reset or snapshot API for their function-local static counters.
**Options Considered**:
1. Add a production test hook - exposes test-only behavior in application source and was excluded by the spec.
2. Depend on generated test order - fragile and still couples fixture indexes to another suite.
3. Run each parser scenario in a child process - preserves exact production behavior and parent state.

**Chosen**: Child-process isolation with a 30-second alarm and bounded result pipe.
**Rationale**: It contains normal returns, parser exits, signals, cwd changes, globals, and private
static state without modifying product code.

## Session Summary

- Completed 23 of 23 tasks with no remaining blockers.
- Added ten production-linked compatibility tests and reusable owner-aware fixture support, including
  a code-review regression for abnormal child cleanup.
- Implementation gates: `make test` PASS (481 tests), `make install` PASS, formatting and
  repository-data hygiene PASS. The post-review suite passes all 482 CuTests; full post-review gate
  evidence is recorded in `code-review.md`.
- BQC fixes: complete file closing, actionable cwd cleanup failure, process-isolated parser state,
  parent-owned abnormal-exit sandbox cleanup, bounded child timeout/result transport, and full
  descriptor protocol cleanup.

**Next command**: `plansession` for Session 02
