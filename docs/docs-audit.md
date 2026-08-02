# Vessel Documentation Audit

**Last audited:** August 2, 2026

## Audit Verdict

The maintained vessel documentation now distinguishes three facts that older
project records conflated:

1. The gameplay, campaign content, presentation, and operator layers through
   Phase 17 are implemented and documented as current behavior.
2. The local regression, final development preflight, 500-vessel performance
   gate, recovery checks, and production-snapshot rehearsal pass.
3. General production release is still withheld pending real-player balance,
   structured human beta, and authorized staged rollout.

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
| What remains unfinished and who owns it | [PRD.md, Release Gate State](PRD.md#release-gate-state) |
| What shipped and when | [CHANGELOG.md](CHANGELOG.md) |
| Which maintenance lessons should persist | [CONSIDERATIONS.md](CONSIDERATIONS.md) |

The temporary vessel workspace was retired after its completed history was
verified against the permanent documents and its three open gates were moved
to the PRD with explicit state, ownership, and exit conditions.

## Corrections Made

- Replaced the foundation-era root PRD with a durable product contract covering
  the living-frontier vision, multiplayer and builder outcomes, shared
  wilderness rules, release budgets, scope, scorecard, and risks.
- Corrected the base ship structure from the obsolete 1,016-byte claim to the
  measured 4,928 bytes, about 2.35 MiB for 500 fixed fleet entries.
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
- Retired `VESSELS_TODO.md` after confirming that behavior, test evidence,
  benchmark results, schema evidence, and completed history already existed in
  their permanent references; the remaining release gates now live in PRD
  Section 8.

## Evidence Rules

- Trace behavior to current source before updating the system reference.
- Date every benchmark, test count, Valgrind result, manual pass, and soak
  result. Historical evidence stays labeled historical.
- Do not turn an implementation checklist into a production-readiness claim.
- Keep requirements and release-gate state in the PRD, architecture decisions
  in the ADR, current behavior in the system reference, evidence in testing
  documents, and completed work in the changelog. Do not create a second
  vessel release checklist in a temporary workspace.
- Update inbound links when a document moves or is retired.
- Keep documentation ASCII, UTF-8, and LF.

## Remaining Documentation Gates

The documentation set is structurally consolidated, but three release gates
remain open. As evidence lands:

- Record new measured results in `VESSEL_BENCHMARKS.md`.
- Keep the live status and fixture expectations in
  `VESSEL_SYSTEM_TESTING.md`.
- Add proven operational remedies to the behavior reference and incident
  runbook.
- Update the PRD gate table when player-data balance, structured human beta, or
  a staged rollout changes state; keep the supporting evidence in its
  permanent behavior, testing, deployment, or changelog document.
