# Code Review and Repair Report

**Session ID**: `phase00-session08-effective-binding-observability`
**Reviewed**: 2026-08-07
**Base Commit**: `2ee93973a299772dc29e301e04c7dfb98340ae01`
**Scope**: All Session 08 changes since the base commit
**Result**: RESOLVED

## Review Surface

The review covered the owned effective-binding model, all prototype lifecycle integration, world
and moving-room parsing, legacy and castle assignment instrumentation, shop and quest wrapper
composition, startup reporting, REdit and room-writer conflict rejection, production-linked
fixtures and tests, both build manifests, the database help migration and verifier, documentation,
the world-tool registry consumer, and all Session 08 workflow artifacts.

The implementation was checked against the master PRD contracts for exact authored identity,
complete boot-order provenance, unchanged callback precedence, saved wrapper secondaries,
`no_specials` path fidelity, safe moving-room ownership, bounded structured diagnostics, and owned
metadata that follows prototype copies without becoming serialized runtime state.

## Findings By Severity

### Critical

No findings.

### High

No findings.

### Medium

- `src/spec/spec_effective_binding.c` - The first formatter implementation reported success even
  when `snprintf` truncated a diagnostic. | Fix: Require a nonnegative result strictly smaller than
  the caller buffer and cover both contribution and final-line truncation. | Status: FIXED
- `src/spec/spec_effective_binding.c` - A resolved contribution following an unresolved world name
  was initially classified as `overridden` even though no effective callback existed. | Fix: Base
  first-selection status on the effective contribution, not list presence, and add regression
  coverage. | Status: FIXED
- `src/spec/spec_effective_binding.c` - Wrapper, source, owner, and secondary combinations were not
  mutually constrained, so invalid records could describe impossible boot states. | Fix: Enforce
  exact shop/quest wrapper agreement, mobile ownership, non-null wrapper handlers, parser-hook room
  ownership, and secondary consistency transactionally. | Status: FIXED
- `src/spec/spec_effective_binding.c` - An initial 255-byte text limit could discard a valid world
  parser line supported by the existing `READ_SIZE` contract. | Fix: Align the model with
  `READ_SIZE - 1`, size escaping buffers for the worst case, and test the exact accepted and rejected
  boundaries. | Status: FIXED
- `src/spec/spec_effective_binding.c` - Final diagnostics selected the first world-authored identity
  rather than the latest replacement. | Fix: Retain the latest world contribution during final-line
  formatting and cover replacement output. | Status: FIXED
- `sql/components/ci_schema_manifest.txt` - The new help migration and verifier were packaged but
  not classified in the repository's exhaustive component-schema manifest. | Fix: Classify the
  migration as `apply`, the read-only verifier as `skip`, and rerun the exact manifest inventory
  comparison. | Status: FIXED
- `scripts/world/wtool_lib/spec_registry.py` - Independent CTest validation exposed that the world
  tooling still parsed the removed legacy `spec_func_list`, causing both world-tool targets to fail
  after the registry migration. | Fix: Parse canonical definitions and referenced alias arrays from
  `src/spec/spec_registry.c` with balanced initializer and comment handling, then cover the live
  registry, malformed aliases, and commented entries. | Status: FIXED

### Low

- `src/db.c` - World provenance initially recovered handler identity by pointer, which can collapse
  distinct registered definitions that share an implementation. | Fix: Read canonical identity
  from the exact resolved authored definition. | Status: FIXED
- `src/spec_assign.c`, `src/zone_procs.c` - `__FILE__` could make call-site diagnostics depend on
  compiler invocation paths and disclose an absolute build path. | Fix: Emit stable repository
  paths plus expansion-site line numbers and assert the production assignment prefix. | Status:
  FIXED
- `unittests/CuTest/test_spec_fixtures.c`, `test_spec_effective_binding.c` - Review found incomplete
  effective-record cleanup, a weak `waitpid` success check, and contribution dereferences that
  relied on structural invariants. | Fix: Free every fixture owner, require the expected child PID,
  guard retrieved contributions and strings, and rerun the suite and static analysis. | Status:
  FIXED
- Builder help was first drafted in the ignored runtime help file even though the database is the
  authoritative help source. | Fix: Leave the ignored deployment file untouched and add an
  idempotent database migration plus read-only verification queries. | Status: FIXED

## Behavioral Quality Review

| Category | Result | Evidence |
|----------|--------|----------|
| Resource cleanup | PASS | Contribution strings, lists, prototype records, REdit copies, and fixture state have explicit deep-copy/free paths. |
| State freshness | PASS | Loader and new-prototype slots initialize effective state to null; room editor and insertion paths deep-copy instead of aliasing. |
| Trust boundary enforcement | PASS | Owner/source bits, identities, locations, wrapper shape, and secondaries are validated before mutation. |
| Failure completeness | PASS | Model calls return errors; boot instrumentation logs failures; loaders and every moving-room save boundary reject explicitly. |
| Mutation safety | PASS | Contribution replacement is transactional and room-zone preflight runs before output creation or mover mutation. |
| Contract alignment | PASS | Outcomes, source tokens, wrapper secondaries, and final callback state follow the verified production sequence. |
| Concurrency safety | N/A | Boot and OLC mutation run in the existing single-threaded server lifecycle. |
| External dependencies | N/A | No new runtime service or network dependency was introduced. |
| Accessibility/platform | N/A | No graphical or web interaction surface changed. |
| Product surface | PASS | Structured lines are the required operator startup surface; builder rejection text is limited to OLC. |

## Security And Compliance Review

| Category | Result | Evidence |
|----------|--------|----------|
| Injection | PASS | Diagnostic fields reject control bytes and escape quote/backslash delimiters; SQL contains no dynamic input. |
| Secrets/authentication | PASS | No credentials or authentication paths changed; protected credential files remain outside the diff. |
| Sensitive data exposure | PASS | Diagnostics contain static procedure identities, source locations, owner types, and VNUMs only. |
| Dependencies/configuration | PASS | No package, service, debug-mode, CORS, or deployment-default change was introduced. |
| Database security | PASS | The help migration is static, transactional, idempotent, classified for schema CI, and paired with read-only verification. |
| GDPR | N/A | The session collects, stores, or transmits no player, account, or other personal data. |

## Compatibility Review

- The callback ABI, prototype callback slots, boot order, and actual assignment statements remain
  runtime authority.
- World, parser-hook, legacy assignment, shop, object, room, and quest writes occur in their
  characterized order; instrumentation follows each existing write without dispatching callbacks.
- Shop and quest wrappers retain their existing nested secondary slots and behavior.
- `no_specials` still controls only the existing guarded assignment block; reporting does not add a
  new runtime gate.
- Mobile `SpecProc`, object `Z`, room `Z`, and moving-room `M` grammar are unchanged except that the
  previously unsafe combined room ownership is now rejected in either order as required.
- Effective history is not consulted by writers or invocation sites; authored-first persistence
  from Sessions 06 and 07 remains intact.
- Production and test source membership is synchronized across Automake and CMake, and component
  SQL classification is exhaustive.

## Deliberate Non-Fixes

- Provenance allocation failure remains best-effort and never suppresses an existing callback
  assignment. This is an explicit Session 08 compatibility decision; the error is still visible as
  a bounded `SYSERR`.
- Runtime OLC reassignment does not rewrite the already-emitted boot provenance chain. The Session
  08 record describes effective boot decisions, while runtime gateways and mutable dispatch history
  are explicitly out of scope.
- Hard-coded room assignments can retain legacy precedence over the moving-room callback. The
  required safety rule targets authored room `Z` ownership; broader typed-hook migration remains a
  later phase.
- Full-file `clang-tidy` reports inherited structure-padding advice in `src/structs.h`. Restricted
  analysis found no changed-code analyzer diagnostic, so unrelated data-layout churn was avoided.

## Evidence Ledger

| Check | Command Or Inspection | Result | Evidence |
|-------|-----------------------|--------|----------|
| Project analysis | `.spec_system/scripts/analyze-project.sh --json` | PASS | Session 08, phase metadata, and base commit resolved correctly. |
| Production-linked suite | `make -j$(nproc) cutest && ./cutest` | PASS | 550/550 CuTests passed after all code-review repairs. |
| Compiler | Autotools GNU C23 build | PASS | Changed production and test sources compiled with no new `-Wall -Wextra` warning. |
| Static analysis | `clang-tidy -p build` on the model and new suite | PASS | Exit zero; only inherited structure-padding diagnostics. |
| Assignment audit | Complete callback-write and macro call-site scan | PASS | All boot contribution sources are instrumented; runtime-only writes remain outside the boot report. |
| Schema inventory | CI-equivalent sorted component/manifest comparison | PASS | Every component SQL file is classified exactly once. |
| SQL behavior | Isolated temporary-table migration twice plus verifier | PASS | Idempotency and all three verifier checks passed without persistent database mutation. |
| World tooling | Full Python world-tool suite | PASS | 173/173 tests passed, including three effective-registry extraction tests. |
| CMake/CTest | Independent CMake build and full CTest matrix | PASS | All 11 targets passed after repairing the stale registry consumer. |
| Formatter/hygiene | `clang-format`, `git diff --check`, unsafe-API scan | PASS | No formatting drift, whitespace error, or newly added unsafe string API. |

## Summary

The complete Session 08 surface was reviewed and all seven Medium and four Low findings were repaired.
No unresolved Critical, High, Medium, or Low finding remains. Effective boot decisions are now
owned, attributable, bounded, and observable without changing established callback precedence, and
the moving-room callback slot cannot be combined with authored room procedure ownership.
