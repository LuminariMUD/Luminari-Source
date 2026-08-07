# CI/CD Pipeline Transition Report

Date: 2026-08-07
Phase: Phase 00 - Legacy Characterization and Safety Net
Branch: `master`

## Scope

This transition run validated the repository's existing CI/CD surface rather
than adding another workflow bundle. All five pipeline bundles were already
present and were adopted as the project baseline in `.spec_system/CONVENTIONS.md`.

| Bundle | Repository surface | Result |
|--------|--------------------|--------|
| Quality | `.github/workflows/quality.yml` | Validated and green |
| Tests | `.github/workflows/test.yml` | Repaired, validated, and green |
| Security | `.github/workflows/security.yml` | Validated and green |
| Integration | `.github/workflows/integration.yml` | Repaired, dispatched, and green |
| Operations | release, Pages, and Dependabot configuration | Validated; Pages green |

The release workflow remains tag-triggered. No release tag was created during
this transition run; its YAML and shell surface were validated statically.

## Repairs

Commit `c4cac7f6` (`Repair CI runtime validation`) fixed the initial clean-run
failures:

- Serialized generation of `build_identity.h` before direct parallel CuTest
  object builds.
- Added a source-controlled five-binding inventory fixture for clean CI
  checkouts while retaining full-world validation in developer environments.
- Added an isolated MariaDB-backed runtime preparer that refuses protected
  repository data and credential paths.
- Routed production-linked tests, coverage, syntax boot, and the network smoke
  test through the isolated runtime.
- Added manual Integration dispatch and corrected pre-existing workflow lint
  findings.

Commit `9dbd4c9d` (`Fix sanitizer and coverage CI failures`) fixed defects found
by the repaired matrix:

- Prevented undefined shifts when a flag-name table is wider than the runtime
  bitvector and used a width-correct bit mask.
- Released superseded object prototype strings during OLC updates.
- Released room trail runtime state independent of wilderness mode and across
  OLC copies.
- Restored `json-c` linkage in the coverage-specific protocol harness command.

## Local Evidence

The following checks passed on the final source changes:

- `actionlint .github/workflows/*.yml`
- `shellcheck scripts/ci/prepare_test_runtime.sh`
- clean direct parallel `make -j"$(nproc)" cutest` build
- normal production-linked suite: 550 runs, 550 passes
- MariaDB-backed isolated suite including syntax boot: 550 runs, 550 passes
- live TCP server startup smoke test against the isolated runtime
- ASan and UBSan suite with halt-on-error and leak detection: 550 passes
- Valgrind production-linked suite: 550 passes, zero errors and zero definite
  leaks
- covered protocol parser harness: 29 passes
- bounded protocol fuzzing: 15 seconds without ASan or UBSan findings
- root `make test`, followed by `make install`, with no root-level `circle`
  artifact

## Remote Evidence

GitHub Actions results for final repair commit
`9dbd4c9d63c71951a5d991e044a86f1cc4d92ead`:

| Workflow | Run | Result at evidence capture |
|----------|-----|----------------------------|
| Code Quality | 31151523644 | Success |
| Dynamic Code Quality | 31151523113 | Success |
| Integration | 31151543684 | Success |
| GitHub Pages | 31151522841 | Success |
| Build & Test | 31151523577 | Success across all six jobs |
| Security | 31151523618 | Success; dependency review skipped as expected for a push event |

The immediately preceding commit also completed Security run 31150869631 and
Integration run 31150792631 successfully.

## Pull Request Review

- PR 53 had no comments or reviews, was merge-clean, and had green security
  checks.
- PR 55 had no comments or reviews. Its stale matrix reproduced the same three
  pre-fix failures. The branch was refreshed against repaired `master` through
  the GitHub update-branch API, producing head `6d908f74`; replacement checks
  had green quality, integration, world-tool, secret-scan, and both Valgrind
  jobs at the bounded poll limit. The remaining duplicated long-running jobs
  were still pending and are backed by the green final-master matrix and exact
  local commands above.
- No pull request was merged by this workflow.

## Security and Credential Handling

- No credential-bearing file was modified.
- Test database settings are fixed CI-only values and are restricted to local
  loopback hosts and database names containing `test` or `ci`.
- The runtime preparer rejects the repository's protected `lib/` directory and
  broad or unsafe targets.
- Gitleaks and CodeQL completed successfully on the first repaired commit;
  Gitleaks also completed successfully on the final repair commit.

## Summary

The five existing CI/CD bundles are adopted and executable. Clean-checkout
runtime failures, sanitizer undefined behavior, OLC memory leaks, and coverage
linkage were repaired with local and remote evidence. No release or deployment
was performed.

## Next command

`infra`
