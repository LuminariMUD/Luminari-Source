# Code Review and Repair Report

**Session ID**: `phase00-session01-registry-and-persistence-characterization`
**Reviewed**: 2026-08-06
**Base Commit**: `fced8f852d5ad1741a135ed1b24c67de08840937`
**Scope**: All changes since the base commit (uncommitted work plus mid-session commits)
**Result**: RESOLVED

## Review Surface

**Files reviewed** (all changes since the base commit):

- `docs/ongoing-projects/spec-todo.md` - master PRD, relocated after phase closeout
- `.spec_system/PRD/phase_00/PRD_phase_00.md` - tracked modified
- `.spec_system/PRD/phase_00/session_01_registry_and_persistence_characterization.md` - untracked
- `.spec_system/PRD/phase_00/session_02_command_and_pulse_characterization.md` - untracked
- `.spec_system/PRD/phase_00/session_03_combat_and_secondary_characterization.md` - untracked
- `.spec_system/PRD/phase_00/session_04_validated_definition_registry.md` - untracked
- `.spec_system/PRD/phase_00/session_05_owner_aware_olc.md` - untracked
- `.spec_system/PRD/phase_00/session_06_authored_binding_model.md` - untracked
- `.spec_system/PRD/phase_00/session_07_binding_round_trip_persistence.md` - untracked
- `.spec_system/PRD/phase_00/session_08_effective_binding_observability.md` - untracked
- `.spec_system/PRD/phase_00/session_09_documentation_and_phase_validation.md` - untracked
- `.spec_system/specs/phase00-session01-registry-and-persistence-characterization/code-review.md` - untracked review artifact
- `.spec_system/specs/phase00-session01-registry-and-persistence-characterization/implementation-notes.md` - untracked
- `.spec_system/specs/phase00-session01-registry-and-persistence-characterization/spec.md` - untracked
- `.spec_system/specs/phase00-session01-registry-and-persistence-characterization/tasks.md` - untracked
- `.spec_system/state.json` - tracked modified
- `CMakeLists.txt` - tracked modified
- `Makefile.am` - tracked modified
- `unittests/CuTest/test_spec_fixtures.c` - untracked
- `unittests/CuTest/test_spec_fixtures.h` - untracked
- `unittests/CuTest/test_spec_registry_persistence.c` - untracked

There are no staged changes and no commits after the base commit. Generated `AllTests.c`, object
files, the test executable, and installed binaries are ignored build outputs rather than source
changes; they were validated through their generating build targets and did not require byte-level
source review.

**Inventory commands**: `git status`,
`git log --oneline fced8f852d5ad1741a135ed1b24c67de08840937..HEAD`,
`git diff fced8f852d5ad1741a135ed1b24c67de08840937`,
`git diff --cached fced8f852d5ad1741a135ed1b24c67de08840937`,
`git ls-files --others --exclude-standard`

## Findings by Severity

### Critical

No findings.

### High

No findings.

### Medium

- `unittests/CuTest/test_spec_registry_persistence.c:221` - The isolated runner originally let the
  child own its temporary world sandbox. A signal, alarm, crash, or `_exit()` before fixture
  destruction therefore bypassed cleanup and could leave a sandbox behind. This was reproduced by
  the early OLC fixture crashes. | Fix: The parent now creates the mode-0700 sandbox, passes its
  validated path to the child, waits for every exit form, and invokes an idempotent checked cleanup.
  `Test_spec_isolated_runner_cleans_abnormal_child_sandbox` forces `_exit(3)` and proves the parent
  removes the sandbox. | Status: FIXED

### Low

- `unittests/CuTest/test_spec_fixtures.c:27` - The saved-file size limit multiplied two `int`
  literals and then widened the result for comparison with `long`, producing a configured
  `clang-tidy` warning. | Fix: Perform the constant multiplication as `long`. | Status: FIXED
- `.spec_system/specs/phase00-session01-registry-and-persistence-characterization/spec.md:87` and
  `.spec_system/specs/phase00-session01-registry-and-persistence-characterization/implementation-notes.md:337`
  - The documentation described a single managed parser lifecycle and implied all checks happened
  after teardown, while the final implementation uses one child per parser-backed scenario and
  records scenario failures before teardown. | Fix: Documented the process boundary, parent-owned
  cleanup, and parent assertion sequence precisely. | Status: FIXED
- `.spec_system/PRD/phase_00/session_01_registry_and_persistence_characterization.md:3` (and line 3
  of the Session 02 through Session 09 stubs) - Escaped backtick delimiters rendered the session IDs
  as literal Markdown punctuation instead of inline code. | Fix: Removed the stray escape
  characters from all nine stubs. | Status: FIXED

## Assumptions and Deliberate Non-Fixes

- Configured `clang-tidy` reports three `clang-analyzer-optin.performance.Padding` advisories in
  unchanged `src/structs.h` structures included by the new tests. Those definitions predate the
  review base, are outside the review surface, and are ABI- and persistence-sensitive. Reordering
  them would be unrelated to this test-only session, so they were deliberately left unchanged.
- Expected `SYSERR` and warning log lines emitted by negative-path production tests are existing test
  behavior. The suite result, assertions, and exit status establish success; no user-facing product
  surface was added or changed.

## Behavior Changes

No production behavior changed. The test harness now removes its private sandbox even when an
isolated scenario exits abnormally, and planning Markdown renders session IDs correctly.

## Evidence Ledger

| Check | Command or Inspection | Result | Evidence / Blocker |
|-------|-----------------------|--------|--------------------|
| Tests | `make test` | PASS | All auxiliary checks and 482 production-linked CuTests passed. |
| Secondary build | `cmake -S . -B "$review_build_dir" -DBUILD_TESTS=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && cmake --build "$review_build_dir" --target cutest -j$(nproc) && ctest --test-dir "$review_build_dir" --output-on-failure -R '^production-cutest$'` | PASS | CMake compiled both new sources and its production CuTest target passed. |
| Installation | `make install` | PASS | Installed build ID `4b1ce2285eb8dd9282f89d14693011d5a059b6f5`; `bin/circle` is active and the root artifact is absent. |
| Linter | `clang-tidy -p "$review_tidy_dir" unittests/CuTest/test_spec_fixtures.c unittests/CuTest/test_spec_registry_persistence.c` | PASS | Zero review-surface warnings after the fix; only the three documented inherited `src/structs.h` padding advisories remain. |
| Formatter | `clang-format --dry-run --Werror unittests/CuTest/test_spec_fixtures.h unittests/CuTest/test_spec_fixtures.c unittests/CuTest/test_spec_registry_persistence.c` | PASS | No formatting drift. |
| Type checker | Targeted inspection of the C-only build and repository configuration | N/A | No separate type checker is configured; both GCC build paths compile with `-Wall -Wextra`. |
| Security | Review against `security-compliance-checklist.md`; targeted path and credential inspection | PASS | No secrets or production writes; cleanup accepts only private `/tmp/luminari-spec-registry-*` paths and enumerates explicit files. |
| Data integrity | Checked-in world digest before and after tests | PASS | Digest remained `421b3ed685339943a3d81a78aaced9f262b23bb0e2e921759a712affb890b509`; `git status --short lib/world` is empty. |
| Repository hygiene | `git diff --check`, `file --mime-encoding`, CR scan, manifest counts, sandbox scan, and artifact assertions | PASS | All 21 review files are ASCII/LF; both manifests contain both sources; ten generated tests; no fixture sandbox or root `circle`. |
| Final diff re-read | `git diff fced8f852d5ad1741a135ed1b24c67de08840937` plus every file from `git ls-files --others --exclude-standard` | PASS | All changed hunks and untracked text files were re-read; no unresolved finding or debug artifact remains. |

## Summary

1. Reviewed 21 files: phase/session planning, state, both build manifests, the reusable fixture,
   characterization tests, and this report.
2. Resolved one Medium cleanup defect and three Low static-analysis/documentation issues. Every code
   defect has regression or build evidence.
3. Deliberately left only three inherited analyzer padding advisories in unchanged, ABI-sensitive
   structures; no review-surface warning remains.
4. Autotools tests and installation, CMake build/tests, static analysis, formatting, security,
   world-data integrity, encoding, line endings, manifest parity, and final diff review all pass.
