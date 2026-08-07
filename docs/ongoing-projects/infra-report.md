# Infrastructure Transition Report

Date: 2026-08-07
Phase: Phase 00 - Legacy Characterization and Safety Net
Branch: `master`

## Selection

Selected bundle: Health

The required analyzer ran before infrastructure inspection. This is a
single-package GNU C23 MUD server deployed through the repository's systemd
unit and autorun supervisor. The highest-priority missing bundle was Health:
the existing loopback Terrain API supported newline-delimited JSON but did not
provide an HTTP readiness endpoint or a service startup probe.

The remaining incomplete bundles, in priority order, are Security, Backup,
and Deploy. The existing backup script and release workflow are useful
building blocks, but they do not yet satisfy the bundle contracts for a
scheduled and restore-verified backup or a main-branch CD trigger.

## Implementation

- Added HTTP `GET` and `HEAD` handling to the loopback Terrain API listener.
- Added `/health` and `/health/ready` with MariaDB connectivity and HTTP 503
  degradation, plus `/health/live` without a database query.
- Added `TERRAIN_API_PORT` validation while retaining port 8182 as the default.
- Added `scripts/operations/healthcheck.sh` with bounded one-shot and wait
  modes, plus a shell regression test.
- Added a systemd `ExecStartPost` readiness probe and taught the deployment
  renderer and supervision regression to preserve and validate it.
- Added an isolated Integration workflow probe on port 4182 and a required
  graceful-shutdown assertion.
- Added the new test surface to both Autotools and CMake.
- Documented endpoint semantics, operator use, incident diagnosis, and the
  Infrastructure table.

Validation exposed an existing ownership bug in the smoke-test path: `-o`
stored an argument-vector pointer in a field freed at shutdown. The override
now duplicates the path and releases the prior owned value. The Integration
workflow now requires normal termination, so this regression is enforced.

## Validation Target

The repository is a development checkout. Validation used an isolated local
runtime, a disposable MariaDB 10.11 container bound to loopback, game port
4199, and health port 4182. No protected runtime configuration or credentials
were changed. Production application is intentionally deferred by repository
policy and is recorded in the
[Apex known-issues ledger](../../.spec_system/audit/known-issues.md).

## Evidence Ledger

| Bundle | Component | Package | Command | Result | Fixes Applied | Remaining / Blocker |
|--------|-----------|---------|---------|--------|---------------|---------------------|
| Health | Project detection | root | `bash .spec_system/scripts/analyze-project.sh --json` | PASS | None | None |
| Health | Route contracts | root | `LUMINARI_TEST_ROOT="$PWD" ./cutest` | PASS; 551/551 | Added readiness, liveness, 404, and 405 cases | None |
| Health | Probe script | root | `scripts/operations/test_healthcheck.sh` | PASS | Added fail-closed payload checks | None |
| Health | Live readiness | root | `LUMINARI_HEALTH_URL=http://127.0.0.1:4182/health scripts/operations/healthcheck.sh --wait` | PASS (local); HTTP 200, game loop healthy, MariaDB healthy | None | Production probe deferred by repository policy |
| Health | HTTP behavior | root | `curl` against `/health/live`, `/health/ready`, and an unknown path | PASS (local); 200, 200, and 404 | None | None |
| Health | Sanitized runtime | root | CMake ASan/UBSan build, live health requests, graceful shutdown | PASS with leak detection disabled | Fixed invalid `-o` free found by the first run | Existing process-lifetime shutdown leaks are outside this bundle |
| Health | systemd probe | root | Rendered local unit plus `systemd-analyze verify` | PASS (local) | Added probe renderer and fixture executable | Production unit reinstall and restart required after release |
| Health | Workflow and shell syntax | root | `actionlint .github/workflows/*.yml`; `shellcheck --severity=warning` on touched scripts | PASS | None | None |
| Health | Build-system parity | root | `cmake --build .infra-asan --target test-healthcheck` | PASS | Added Autotools and CMake test targets | None |
| Health | Aggregate repository gate | root | `make test` | PASS; 551/551 plus all shell regressions | Updated systemd supervision fixture | None |
| Health | Install cleanup | root | `make install`; root `circle` absence check | PASS | None | None |

## External Setup

After this change reaches an approved production release, an operator must run
the documented managed-service update and verify readiness:

```bash
./scripts/deployment/deploy.sh --install-systemd --restart-service
./scripts/operations/healthcheck.sh
```

This was not executed from the development checkout because repository policy
forbids production code changes. All repository-side configuration and local
equivalent validation are complete.

## Summary

The Health bundle is configured and passing on its local validation target.
The endpoint is loopback-only, reports required database readiness, has a
separate liveness route, and is enforced during systemd startup and CI smoke
testing. No credential-bearing file was modified.

## Next command

`carryforward`
