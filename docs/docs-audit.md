# Documentation Audit Ledger

This is the canonical repository documentation-audit ledger. It preserves the current Phase 00
transition audit and the earlier vessel-documentation consolidation audit without requiring a
second workflow-local report.

## Phase 00 Transition Documentation Audit

**Date:** 2026-08-07

**Project:** LuminariMUD

**Mode:** Phase-focused

**Phase:** P00 - Registry Safety and Observability (complete, 9 sessions)

**Phase base:** `fced8f852d5ad1741a135ed1b24c67de08840937`

### Scope

The deterministic analyzer reported Phase 00 complete, no active session, a single-repository
project, and nine completed sessions. The transition audit read all nine implementation notes and
used the first-session base commit to establish the phase manifest. The Phase 00 documentation and
health infrastructure surfaces received the deep audit; required root and standard documentation
entry points were also checked.

The session workflow artifacts were later removed after their enduring content was consolidated
into the maintained documents named in this ledger. Git history retains the original execution
records.

The audit did not claim a full behavioral review of every historical or subsystem document under
`docs/`. Unchanged legacy catalogs route readers to current entry points or carry an explicit
legacy warning where they remain linked.

### Coverage Summary

| Area | Required | Found | Status |
|------|---------:|------:|--------|
| Root files (`README.md`, `CONTRIBUTING.md`, `LICENSE`) | 3 | 3 | Updated and current for audited claims |
| Standard docs entry points | 7 | 7 | Architecture, onboarding, development, environments, deployment, incident response, and API present |
| Code ownership | 1 | 1 | Existing owner assignments preserved; moved source paths corrected |
| ADR support | 1 | 2 | Template plus accepted vessel ADR present |
| Active subdirectory overview naming | 0 legacy names allowed | 0 `README.md` violations | PASS outside archived `EXAMPLE/` trees |
| Monorepo package READMEs | 0 | 0 | N/A; analyzer reports `monorepo=false` |
| Phase 00 implementation notes | 9 | 9 | Read before consolidation |
| Phase 00 implementation summaries | 9 | 9 | Read before consolidation and used for sync |
| Phase 00 security reports | 9 | 9 | Read before consolidation and used for security/deployment wording |

### Files Created

- `docs/ARCHITECTURE.md` - Verified high-level component and data-flow map.
- `docs/onboarding.md` - Fresh-clone developer checklist.
- `docs/development.md` - Current daily commands, source map, and build gates.
- `docs/deployment.md` - CI/CD, release, managed-service, and rollback boundaries.
- `docs/api/README_api.md` - Loopback health/readiness HTTP contract.
- `docs/guides/SETUP_AND_BUILD_GUIDE.md` - Restored source-backed setup/build target referenced by
  deployment help and the web index.
- `docs/docs-audit.md` - Canonical evidence ledger, now including the Phase 00 and vessel audits.

### Files Updated

- `README.md`, `CONTRIBUTING.md`, `LICENSE`, and `SECURITY.md` provide concise current entry
  points, repository conventions, ASCII legal text, and an honest unresolved security-policy
  boundary.
- `docs/environments.md`, `docs/deployment/DEPLOYMENT_GUIDE.md`,
  `docs/runbooks/incident-response.md`, and
  `docs/guides/TROUBLESHOOTING_AND_MAINTENANCE.md` contain repository-backed environment, service,
  health, containment, and recovery claims.
- `docs/guides/DEVELOPER_GUIDE_AND_API.md` retains the source-verified Phase 00 API instead of
  unaudited legacy build, PHP, test, and code-style examples.
- `docs/systems/ARCHITECTURE.md` redirects old inbound links to the current architecture overview
  and detailed core reference.
- `docs/TECHNICAL_DOCUMENTATION_MASTER_INDEX.md` registers current entry points, corrects renamed
  overviews, and removes a nonexistent PHP guide from the operator path.
- `docs/GETTING_STARTED.md`, `docs/admin/FAQ.md`,
  `docs/systems/OLC_ONLINE_CREATION_SYSTEM.md`, and `docs/CHANGELOG.md` received targeted port,
  authority, link, naming, and audit corrections.
- `docs/CODEOWNERS` retains organizational assignments while matching the current `src/combat/`,
  `src/magic/`, `src/vessels/`, `src/dgscript/`, build, and database paths.
- `scripts/deployment/deploy.sh` reports port 4100, matching `src/config.c` and the autorun default.
  `Makefile.am` distributes the renamed script and unit-test overviews.

### Overview Files Renamed

The following active subdirectory overviews follow the unique `README_<directory>.md` convention.
Inbound links and distribution entries were updated; historical paths in the changelog were
intentionally not rewritten.

- `docs/admin/README_admin.md`
- `docs/development/README_development.md`
- `docs/legal/README_legal.md`
- `docs/ongoing-projects/README_ongoing-projects.md`
- `docs/utilities/README_utilities.md`
- `scripts/README_scripts.md`
- `unittests/README_unittests.md`
- `util/powershell/README_powershell.md`

The archived `EXAMPLE/` trees were excluded from active documentation naming because they preserve
example project snapshots rather than current project entry points.

### Verified Current

- Phase 00 builder, API, boot architecture, database-first help, test ownership, and validation
  documents remain aligned with the 28 canonical definitions, 29-name compatibility projection,
  three binding layers, moving-room conflict policy, and 78 dedicated tests recorded by Session 09.
- The health documentation matches the loopback bind, GET/HEAD route parser, JSON bodies, 200/503
  readiness, liveness semantics, environment bounds, shell probe, systemd `ExecStartPost`, and CI
  integration in source.
- The CI/CD overview matches the checked-in Quality, Build and Test, Security, Integration, Pages,
  Release, and Dependabot configuration. The tag workflow creates release metadata but does not
  deploy a host.
- The root test contract matches `Makefile.am`: `make test` runs the production-linked suite and
  registered shell regressions; `make test-all` adds world tools, protocol, character-rename
  checks, and final installation.
- The active README naming scan finds only the root `README.md`; active subdirectory overview
  filenames are unique.

### Corrections Made

| Finding | Evidence | Resolution |
|---------|----------|------------|
| Root README duplicated Quick Start and used obsolete commands | Root file compared with deploy/autorun help and Make targets | Replaced with one verified setup path and authoritative test/install gate |
| User docs and deploy output said port 4000 | `src/config.c`, autorun default, systemd unit, and deploy completion output all default to 4100 | Updated docs and deploy completion output to 4100 |
| Environment guide invented staging URL, database, variables, and `--staging` | Deploy help has no staging option; repository state defines development/production safety only | Replaced with verified boundaries and variables |
| Operator docs prescribed broad `kill -9`, firewall/sysctl writes, contacts, SLAs, and maintenance schedules | No repository authority supports those actions or policies | Replaced with read-only diagnosis, normal supervisor controls, and explicit external-policy gaps |
| Setup guide referenced by deployment help was missing | `deploy.sh --help`, `AGENTS.md`, and `docs/web/index.html` named the absent path | Created the guide from current deploy/build/test sources |
| Architecture overview listed obsolete flat source paths and behavior | Current source tree, `AGENTS.md`, and core architecture disagreed with the old inventory | Added a current overview and turned the old path into a redirect |
| Developer guide contained K&R braces, `//` comments, nonexistent tests and profiling commands, and stale PHP guidance | Repository style, Make targets, scripts, and Session 09 evidence | Reduced it to the source-verified special-procedure API and current entry links |
| `SECURITY.md` was an unfilled template with unsupported version claims | No supporting release policy or private reporting route exists in the repository | Removed false claims and documented the required owner decision |
| Active subdirectories reused ambiguous `README.md` names | Filesystem scan found eight active names outside root | Renamed them and repaired maintained references |

### Evidence Ledger

| Area | Document | Codebase or Spec Evidence | Result |
|------|----------|---------------------------|--------|
| Project state | This report | Phase 00 closeout snapshot retained in Git history | P00 complete; 9 sessions; non-monorepo |
| Phase manifest | Phase-focused docs | First session `Base Commit` plus `git diff --name-only fced8f85..HEAD` | Authoritative transition manifest established |
| Phase semantics | Phase 00 docs | Nine implementation records, retained in Git history after consolidation | Read and synchronized |
| Carryforward inputs | Operations/security docs | `docs/CONSIDERATIONS.md`, `docs/known-issues.md`, and the Phase 00 validation security section | Consolidated without claiming production activation |
| Quick start | `README.md`, onboarding/build docs | `deploy.sh --help`; deploy build/database/world functions inspected | Updated |
| Build/test commands | Development and contributing docs | `Makefile.am` test/test-all/install targets; dry-run target inspection | Verified |
| CMake commands | Development/build docs | `CMakeLists.txt` `BUILD_TESTS` and install blocks | Verified |
| Game port | README, getting started, environments, deployment | `src/config.c`, autorun default, systemd unit, deploy completion output | Aligned at 4100 |
| Health API | API, deployment, environment, runbook docs | `terrain_bridge.c/.h`, healthcheck script, systemd unit, gameplay/shell regressions | Verified |
| Operational commands | Deployment/runbook docs | Deploy, autorun, healthcheck, process-memory, and world-tool help surfaces | Verified |
| CI/CD | `docs/deployment.md` | Six workflow trigger/job files plus `.github/dependabot.yml`; pipeline report | Verified |
| Architecture | `docs/ARCHITECTURE.md` | Current source directories and core entry files asserted present | Verified |
| Special-procedure docs | Developer/architecture/builder/testing docs | Session 09 link/symbol/count audit and Phase 00 validation matrix | Verified current |
| Code ownership | `docs/CODEOWNERS` | Current filesystem paths checked; owner identities preserved | Updated |
| Documentation links | Changed Markdown | Local target and fragment resolver | PASS in the transition audit and consolidation pass |
| Text hygiene | Changed documentation | ASCII, CR, final-newline scan and `git diff --check` | PASS in the transition audit and consolidation pass |
| Shell safety | Changed deployment and health scripts | `bash -n`; `shellcheck --severity=warning` | PASS |
| README naming | Active tree | `find` excluding `.git/` and archived `EXAMPLE/` trees | PASS; no subdirectory `README.md` remains |

### Remaining External Decision Gaps

1. Project owners must define supported security release lines, configure or publish a private
   vulnerability-reporting channel, and choose response and disclosure targets. `SECURITY.md`
   deliberately makes no unsupported promise.
2. Operators must define the production on-call roster, escalation channel, response targets, and
   maintenance windows. The repository cannot assign people or service-level commitments.

### Documented Operational Follow-Up

- The approved production release still needs the canonical systemd unit installed/restarted and
  the readiness endpoint probed. Repository policy prohibited that production mutation from this
  development checkout; the local equivalent passed and the exception remains in the
  [known-issues ledger](known-issues.md).
- The repository has no single general application rollback command. Current docs state that
  constraint and require identified immutable release, database, world-data, and component-runbook
  evidence before production rollback.

### Next Action

The [Special Procedure Refactor PRD](ongoing-projects/spec-todo.md) is the completed Phase 00-06
decision record. No scheduled special-procedure phase remains. Future composition or lifecycle work
must satisfy its consumer, ordering, lifetime, OLC, and persistence reopen criteria and use the
repository's standard development workflow.

## Vessel Documentation Consolidation Audit

**Last audited:** August 2, 2026

### Audit Verdict

The maintained vessel documentation distinguishes three facts that older project records
conflated:

1. The gameplay, campaign content, presentation, and operator layers through Phase 17 are
   implemented and documented as current behavior.
2. The local regression, final development preflight, 500-vessel performance gate, recovery
   checks, and production-snapshot rehearsal pass.
3. General production release is still withheld pending real-player balance, structured human
   beta, and authorized staged rollout.

The vessel system must not be described as production-ready until the release criteria in the
[Vessel System Product Requirements](product-requirements/VESSEL_SYSTEM_REQUIREMENTS.md) have
current evidence.

### Source-of-Truth Map

| Question | Authoritative document |
|----------|------------------------|
| Why the product exists and what release means | [Vessel System Product Requirements](product-requirements/VESSEL_SYSTEM_REQUIREMENTS.md) |
| Which architecture was chosen and why | [0001-unified-vessel-system.md](adr/0001-unified-vessel-system.md) |
| What the current implementation does | [VESSEL_SYSTEM.md](systems/VESSEL_SYSTEM.md) |
| How to run the live command regression | [VESSEL_SYSTEM_TESTING.md](testing/VESSEL_SYSTEM_TESTING.md) |
| What performance and test evidence exists | [VESSEL_BENCHMARKS.md](testing/VESSEL_BENCHMARKS.md) |
| How schema install, verification, and rollback work | [VESSEL_SCHEMA_DEPLOYMENT.md](deployment/VESSEL_SCHEMA_DEPLOYMENT.md) |
| What remains unfinished and who owns it | [Vessel System Product Requirements, Release Gate State](product-requirements/VESSEL_SYSTEM_REQUIREMENTS.md#release-gate-state) |
| What shipped and when | [CHANGELOG.md](CHANGELOG.md) |
| Which maintenance lessons should persist | [CONSIDERATIONS.md](CONSIDERATIONS.md#vessel-system) |

The temporary vessel workspace was retired after its completed history was verified against the
permanent documents and its three open gates were moved to the vessel product requirements with
explicit state, ownership, and exit conditions.

### Corrections Made

- Replaced the foundation-era requirements draft with a durable vessel product contract covering
  the living-frontier vision, multiplayer and builder outcomes, shared wilderness rules, release
  budgets, scope, scorecard, and risks.
- Corrected the base ship structure from the obsolete 1,016-byte claim to the measured 4,928 bytes,
  about 2.35 MiB for 500 fixed fleet entries.
- Separated historical movement microbenchmarks from the unmeasured complete 500-ship tick. The
  release target remains 25 ms with all vessel subsystems active.
- Replaced references to removed standalone vessel mirror suites and the nonexistent `test_runner`
  with the production-linked root CuTest workflow.
- Repaired the legacy identity blocker and recorded the complete 30-step Kohdee regression,
  runtime-room reclamation, full-restart hull relinking, and test-data cleanup.
- Recorded the proven cedit command-and-tick kill switch, production-off debug default with runtime
  development categories, and authoritative 31-entry, 74-command help audit.
- Generalized the schema runbook across phases 2, 4, 6, 7, and 8 plus the authoritative help
  migration, snapshot rehearsal, property comparison, and reverse-order rollback.
- Retired the temporary final vessel requirements draft after moving its enduring content into the
  maintained documents above.
- Retired `VESSELS_TODO.md` after confirming that behavior, test evidence, benchmark results,
  schema evidence, and completed history already existed in permanent references. Remaining
  release gates now live in the vessel product requirements.

### Evidence Rules

- Trace behavior to current source before updating the system reference.
- Date every benchmark, test count, Valgrind result, manual pass, and soak result. Historical
  evidence stays labeled historical.
- Do not turn an implementation checklist into a production-readiness claim.
- Keep requirements and release-gate state in the vessel product requirements, architecture
  decisions in the ADR, current behavior in the system reference, evidence in testing documents,
  and completed work in the changelog.
- Update inbound links when a document moves or is retired.
- Keep documentation ASCII, UTF-8, and LF.

### Remaining Documentation Gates

The documentation set is structurally consolidated, but three release gates remain open. As
evidence lands:

- Record new measured results in `VESSEL_BENCHMARKS.md`.
- Keep the live status and fixture expectations in `VESSEL_SYSTEM_TESTING.md`.
- Add proven operational remedies to the behavior reference and incident runbook.
- Update the vessel product requirements gate table when player-data balance, structured human
  beta, or a staged rollout changes state; keep supporting evidence in its permanent behavior,
  testing, deployment, or changelog document.
