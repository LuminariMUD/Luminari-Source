# Phase Transition Audit Report

**Audited**: 2026-08-07
**Repository**: LuminariMUD single repository
**Phase state**: Phase 00 complete, 9/9 sessions complete
**Result**: PASS

## Detection

- The required deterministic analysis ran first through the repository-local
  `.spec_system/scripts/analyze-project.sh --json` entry point.
- The repository was clean at `92db8da0e9a59df1bc95a9d3d7b7e031fab7f21c`, matching
  `origin/master`, before audit remediation began.
- `docs/CONVENTIONS.md` identifies a GNU C23 application with Autotools as the preferred
  build and CMake as the supported secondary build.
- `.spec_system/audit/known-issues.md` was absent. Known issues loaded: 0 paths, 0 rules, 0 tests.

## Bundle Comparison

| Priority | Bundle | Detected Tooling | Audit Action | Final Status |
|----------|--------|------------------|--------------|--------------|
| 1 | Formatting | clang-format and pre-commit | Adopted existing configuration | Configured |
| 2 | Linting | clang-tidy | Adopted existing configuration | Configured |
| 3 | Type Safety | GNU C23 with `-Wall -Wextra` | Recorded explicitly | Configured |
| 4 | Testing | CuTest, protocol harness, and auxiliary tests | Adopted existing configuration | Configured |
| 5 | Observability | Text logs, core capture, and GDB diagnostics only | Selected and implemented | Configured |
| 6 | Git Hooks | pre-commit configuration | Installed declared pre-commit and pre-push hooks | Configured |
| 7 | Database | MariaDB, master schema, component manifest, and schema harness | Adopted existing configuration | Configured |

Selected: Observability - structured error capture was not yet configured.

All seven recommended bundles are now configured. Existing repository tooling was retained rather
than replaced.

## Observability Implementation

The implementation extends the established `scripts/autorun/autorun.sh` supervisor rather than
adding a parallel logger or runtime dependency:

- Every abnormal MUD exit atomically publishes `log/last_error_<UTC-timestamp>.json`.
- The record follows the audit schema with `timestamp`, `level`, `msg`, `error`, and `context`.
- Context includes PID, exit code, uptime, crash count, immutable executable identity, and exact
  captured core and backtrace paths.
- JSON escaping is implemented in Bash so production does not depend on optional `jq` or Python
  installations.
- Temporary files are created securely and final records are restricted to mode `0600`.
- Environment values, database configuration, and credentials are excluded.
- Structured records use the existing ignored `log/` runtime directory and its 30-day retention
  policy. The repository already ignores `log/*` while retaining `log/.gitkeep`, so a duplicate
  `logs/` tree was not introduced.
- The autorun supervision harness forces SIGABRT, parses the generated JSON, and verifies schema,
  permissions, process identity, release provenance, core, and backtrace consistency.

## Fixes Applied

1. Added atomic structured abnormal-exit capture and retention to the existing supervisor.
2. Added production-like supervision coverage for valid JSON, mode `0600`, exact process identity,
   and diagnostic artifact links.
3. Installed the configured pre-push hook, which was missing from this checkout.
4. Recorded existing linting, type-safety, database, and dev-startup tooling in
   `docs/CONVENTIONS.md`.
5. Documented incident triage and the structured record in maintained project documentation.
6. Corrected the CMake audit invocation after the repository defaulted `BUILD_TESTS` to `OFF`, then
   rebuilt with `-DBUILD_TESTS=ON` and ran every CTest target.

## Evidence Ledger

| Bundle | Package | Command | Result | Fixes Applied | Remaining / Blocker |
|--------|---------|---------|--------|---------------|---------------------|
| Detection | root | `bash .spec_system/scripts/analyze-project.sh --json` | PASS | None | None |
| Formatting | root | pre-commit clang-format and hygiene hooks on audit files | PASS | None required | None |
| Linting | root | `clang-tidy -quiet -p build src/spec/spec_registry.c src/spec/spec_binding.c src/spec/spec_effective_binding.c src/olc/spec_menu.c` | PASS | None required | Configured non-fatal padding diagnostics originate in unchanged central structs |
| Type Safety | root | Fresh GNU C23 CMake Debug build with `-Wall -Wextra` | PASS | None required | None |
| Testing | root | `make test` | PASS | None required | Seven auxiliary checks and 550/550 CuTests passed |
| Installation | root | `make install` | PASS | None required | Versioned executable installed; no root `circle` remains |
| Secondary Build | root | CMake Debug build with `-DBUILD_TESTS=ON`, then `ctest --output-on-failure` | PASS | Corrected the initial default test setting | 11/11 CTest targets passed in 38.31 seconds |
| Observability | root | `scripts/autorun/test_autorun_supervision.sh` | PASS | Added structured-error assertions | None |
| Git Hooks | root | `.venv/bin/python3 -m pre_commit install --hook-type pre-commit --hook-type pre-push` | PASS | Installed missing pre-push hook | None |
| Database | root | `make test-character-rename-schema` | PASS | None required | Ephemeral MariaDB schema, migration, transaction, and rollback checks passed |
| Database Content | root | Session 09 temporary help tables, migration twice, and verifier | PASS | None required | Four verifier contracts passed with no persistent mutation |
| Dev Startup | root | `timeout 30 ./bin/circle -c -d lib` | PASS | Recorded supported syntax-check boot command | Database connected; world loaded and cleaned; process exited 0 |
| Documentation | root | encoding, line-ending, link, and `git diff --check` checks | PASS | Added audit and operator guidance | None |

clang-tidy completed successfully under the repository's configured policy. Its opt-in performance
analyzer reports inherited padding opportunities in `src/structs.h`; these are informational, are
not warnings-as-errors, and were not introduced or affected by the tooling-only audit change.

The database bundle is repository-native and component-oriented rather than a generic migration
CLI. Current CI applies `sql/master_schema.sql`, classifies every component exactly once through
`sql/components/ci_schema_manifest.txt`, and applies each active component to a fresh database.
Reversible component migrations carry explicit rollback SQL. The current audit changed no schema;
the ephemeral schema harness and the Phase 00 idempotent help migration provide the applicable
fresh-schema, rollback, reconnect, and repeated-seed evidence.

## Security And Integrity

- No credential file or local configuration header was modified.
- Structured records do not serialize environment variables or database credentials.
- Runtime error records are atomically published with mode `0600` under an ignored directory.
- No source manifest changed because the audit added no C translation unit.
- The installed `bin/circle` remains an executable versioned symlink and the project root contains
  no `circle` build artifact.

## Outcome

The audit added one bundle, Observability, and all configured tooling passes. No intentional
exception or external blocker remains, so `.spec_system/audit/known-issues.md` was not created.

The immediate next Apex workflow is `pipeline`. Phase 01 remains deferred and was not started by
this MVP implementation.
