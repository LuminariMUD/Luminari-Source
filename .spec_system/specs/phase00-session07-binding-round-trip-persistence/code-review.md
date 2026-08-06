# Code Review and Repair Report

**Session ID**: `phase00-session07-binding-round-trip-persistence`
**Reviewed**: 2026-08-07
**Base Commit**: `355152c102e885d75ff9372c626017e9546d5e95`
**Scope**: All Session 07 changes since the base commit
**Result**: RESOLVED

## Review Surface

The review covered the world-authored persistence accessor, mobile/object/room writer decisions,
loader and editor ownership flow inherited from Session 06, shared fixture extensions, nested
fresh-parser orchestration, all new production-linked tests, build manifests, builder guidance, and
the Session 07 workflow artifacts.

The implementation was checked against the master PRD rule that writers consult authored state
first and use pointer lookup only as a legacy fallback. Review scenarios included canonical and
alias records, null-callback unresolved/incompatible records, later callback overrides, untouched
editor saves, explicit replacement, explicit clear, callback-only prototypes, malformed direct API
input, parser static counters, partial fixture failures, and temporary cleanup.

## Findings By Severity

### Critical

No findings.

### High

No findings.

### Medium

No findings.

### Low

- `src/spec/spec_binding.c` - The first persistence accessor accepted requested text containing CR
  or LF bytes. Production loader and selector paths cannot create such text, but a future direct
  model caller could have injected an extra world-file line. | Fix: Require a non-empty single-line
  requested name at the persistence boundary and add direct regression coverage. | Status: FIXED

## Behavioral Quality Review

| Category | Result | Evidence |
|----------|--------|----------|
| Authored priority | PASS | Each writer branches on record presence before any callback lookup. |
| Alias behavior | PASS | Loaded `Guildmaster` emits and reloads exactly while resolving to canonical `Guild`. |
| Unresolved behavior | PASS | Unknown and incompatible names emit exactly despite null authored handlers. |
| Override isolation | PASS | Different effective callbacks survive editor setup/save but never enter emitted world fields. |
| Explicit actions | PASS | Selector choices emit canonical names; `0` emits no field and reloads null state. |
| Legacy compatibility | PASS | Callback-only prototypes reverse-map, emit, and reload through the existing syntax. |
| Output safety | PASS | Persistence accepts only non-empty single-line world-authored names. |
| Fresh reload | PASS | Writer child and untouched parent parser exercise emitted files without counter reuse. |
| Cleanup | PASS | Parent fixture owns cleanup; children exit without deleting shared files; no sandbox remains. |

## Compatibility Review

- Mobile `SpecProc`, object `Z`, and room `Z` grammar are byte-for-byte structurally unchanged.
- Callback ABI, callback slots, activation flags, invocation, and boot precedence are unchanged.
- A present authored record changes only which identity is serialized, as required by the PRD.
- Reverse lookup remains for prototypes that have a callback but no authored record.
- Loaded compatibility aliases remain stable; deliberate builder selection remains canonical.
- Shops, quests, parser hooks, hard-coded assignment behavior, effective summaries, and moving-room
  policy remain Session 08 scope.
- Production/test source membership is synchronized across Automake and CMake.

## Deliberate Non-Fixes

- The writers omit a present but structurally invalid/non-world record instead of serializing its
  callback. Prototype authored slots are world-owned by contract; promoting the callback would
  violate provenance safety.
- Full-file `clang-tidy` reports inherited C Annex K recommendations, structure-padding advice, and
  three dead stores in unchanged moving-room output lines. The repository uses standard bounded
  `snprintf`/`fprintf`, the changed output is single-line validated, and no changed-code analyzer
  finding remains.
- Effective source contributions, collision summaries, and combined moving-room plus room binding
  policy remain Session 08 scope.

## Evidence Ledger

| Check | Command Or Inspection | Result | Evidence |
|-------|-----------------------|--------|----------|
| Production-linked suite | `make -j$(nproc) cutest && ./cutest` | PASS | 543/543 CuTests passed after repair. |
| Compiler | Focused Autotools target | PASS | Changed sources compiled under GNU C23 with no new `-Wall -Wextra` warning. |
| Static analysis | Restricted `clang-tidy` plus changed-line inspection | PASS | Only inherited diagnostics outside changed logic; no open finding. |
| Formatter | `clang-format --dry-run --Werror` | PASS | New and directly modified C/H files have no drift. |
| Integrity | Manifest, protected-path, world-data, sandbox, and diff inspections | PASS | Membership is exact; protected/world files are unchanged; temporary sandboxes are absent. |

## Summary

The complete Session 07 surface was reviewed and one Low finding was repaired. No unresolved
Critical, High, Medium, or Low finding remains. The writers now preserve authored intent through
real three-format save/reload cycles without changing runtime precedence or legacy callback-only
persistence.
