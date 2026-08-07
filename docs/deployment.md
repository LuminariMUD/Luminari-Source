# Deployment and CI/CD

The authoritative host setup and managed-service procedure is the
[deployment guide](deployment/DEPLOYMENT_GUIDE.md). This page records the
repository-level pipeline, release, and rollback boundaries.

## Local Development

```bash
./scripts/deployment/deploy.sh --dev
./bin/circle -d lib
./scripts/operations/healthcheck.sh
```

Run the health check in a second terminal after startup. Use
`./scripts/deployment/deploy.sh --help` for the complete verified option list.

## CI/CD Pipeline

| Workflow | Trigger | Contract |
|----------|---------|----------|
| Code Quality | Push or pull request affecting C sources/config | Formatting, targeted static analysis, warning build |
| Build & Test | Relevant pushes and pull requests | World tools, production tests, sanitizers, Valgrind, coverage, and related gates |
| Security | Relevant pushes/PRs, manual, twice monthly | Gitleaks, CodeQL, and PR-only dependency review |
| Integration | Relevant pull requests or manual dispatch | MariaDB schema checks, isolated world/runtime validation, network and health smoke tests |
| GitHub Pages | Documentation push to `master` or manual dispatch | Publishes the `docs/` tree |
| Release | Tag matching `v*.*.*` | Builds and creates GitHub release notes |

The release workflow creates GitHub release metadata; it does not update a
running host. Production deployment remains an explicit operator action.

## Managed Service Update

The canonical unit runs `scripts/autorun/autorun.sh` and enforces readiness
through systemd `ExecStartPost`:

```bash
./scripts/deployment/deploy.sh --install-systemd --restart-service
./scripts/autorun/autorun.sh status
./scripts/operations/healthcheck.sh
```

Run this only on an approved host. A development checkout must not modify the
production service or runtime.

As of the Phase 00 transition on 2026-08-07, the endpoint, probe, and rendered
unit passed against an isolated local MariaDB runtime. Installing the released
unit, restarting the production service, and probing production readiness still
require an approved operator action; local success is not a production
activation claim.

## Release Storage and Rollback

`make install` stores immutable server and debug binaries under
`bin/releases/<ELF-build-ID>/` and atomically updates `bin/circle`. Autorun
records the active executable and build identity for crash analysis.

The repository does not provide one general application rollback command.
Operators must preserve the active release, database backup, and world data,
then use the approved host procedure for the affected deployment. Component
database migrations with explicit rollback scripts document those steps in
their subsystem deployment guides.

## Operational References

- [Environment boundaries](environments.md)
- [Health API contract](api/README_api.md)
- [Phase 00 security and privacy assessment](testing/SPECIAL_PROCEDURE_PHASE_00_VALIDATION.md#security-and-privacy-assessment)
- [Incident response](runbooks/incident-response.md)
- [Troubleshooting and maintenance](guides/TROUBLESHOOTING_AND_MAINTENANCE.md)
