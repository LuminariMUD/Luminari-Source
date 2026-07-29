# Vessel Documentation Audit

**Last audited:** July 29, 2026

## Audit Verdict

The maintained vessel documentation now distinguishes three facts that older
project records conflated:

1. The transport foundation and phases 04 through 09 gameplay code are
   implemented.
2. The local 30-step regression and development release-boundary checks pass.
3. Historical automated and memory evidence exists, but full-load performance,
   soak, complete recovery, content, balance, and rollout acceptance are not
   complete.

The vessel system must not be described as production-ready until the release
criteria in [PRD.md](PRD.md) have current evidence.

## Source-of-Truth Map

| Question | Authoritative document |
|---|---|
| Why the product exists and what release means | [PRD.md](PRD.md) |
| Which architecture was chosen and why | [0001-unified-vessel-system.md](adr/0001-unified-vessel-system.md) |
| What the current implementation does | [VESSEL_SYSTEM.md](systems/VESSEL_SYSTEM.md) |
| How to run the live command regression | [VESSEL_SYSTEM_TESTING.md](testing/VESSEL_SYSTEM_TESTING.md) |
| What performance and test evidence exists | [VESSEL_BENCHMARKS.md](testing/VESSEL_BENCHMARKS.md) |
| How schema install, verification, and rollback work | [VESSEL_SCHEMA_DEPLOYMENT.md](deployment/VESSEL_SCHEMA_DEPLOYMENT.md) |
| What remains unfinished | [VESSELS_TODO.md](project-management-zusuk/vessels/VESSELS_TODO.md) |
| What shipped and when | [CHANGELOG.md](CHANGELOG.md) |
| Which maintenance lessons should persist | [CONSIDERATIONS.md](CONSIDERATIONS.md) |

The temporary vessel workspace contains only `VESSELS_TODO.md`. It is a
backlog, not a current-behavior reference.

## Corrections Made

- Replaced the foundation-era root PRD with a durable product contract covering
  the living-frontier vision, multiplayer and builder outcomes, shared
  wilderness rules, release budgets, scope, scorecard, and risks.
- Corrected the base ship structure from the obsolete 1,016-byte claim to the
  measured 4,744 bytes, about 2.26 MiB for 500 fixed fleet entries.
- Separated historical movement microbenchmarks from the unmeasured complete
  500-ship tick. The release target remains 25 ms with all vessel subsystems
  active.
- Replaced references to removed standalone vessel mirror suites and the
  nonexistent `test_runner` with the production-linked root CuTest workflow.
- Repaired the legacy identity blocker and recorded the complete 30-step
  Kohdee regression, runtime-room reclamation, full-restart hull relinking, and
  test-data cleanup.
- Recorded the proven cedit command-and-tick kill switch, production-off debug
  default with runtime development categories, and authoritative 31-entry,
  74-command help audit.
- Generalized the schema runbook across phases 2, 4, 6, 7, and 8 plus the
  authoritative help migration, snapshot rehearsal, property comparison, and
  reverse-order rollback.
- Retired the temporary final PRD after moving its enduring content into the
  maintained documents above.

## Evidence Rules

- Trace behavior to current source before updating the system reference.
- Date every benchmark, test count, Valgrind result, manual pass, and soak
  result. Historical evidence stays labeled historical.
- Do not turn an implementation checklist into a production-readiness claim.
- Keep requirements in the PRD, architecture decisions in the ADR, current
  behavior in the system reference, evidence in testing documents, completed
  work in the changelog, and only unfinished work in the temporary workspace.
- Update inbound links when a document moves or is retired.
- Keep documentation ASCII, UTF-8, and LF.

## Remaining Documentation Gates

The documentation set is structurally consolidated, but release evidence is
still incomplete. As work lands:

- Record new measured results in `VESSEL_BENCHMARKS.md`.
- Keep the live status and fixture expectations in
  `VESSEL_SYSTEM_TESTING.md`.
- Add proven operational remedies to the behavior reference and incident
  runbook.
- Move completed backlog outcomes into the changelog and permanent references,
  then remove the completed checklist entries.
- Retire `VESSELS_TODO.md` only when no vessel work remains or move explicitly
  deferred optional work into the general project backlog.
