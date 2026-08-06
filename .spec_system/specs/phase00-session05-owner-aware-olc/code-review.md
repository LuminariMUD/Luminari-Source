# Code Review and Repair Report

**Session ID**: `phase00-session05-owner-aware-olc`
**Reviewed**: 2026-08-07
**Base Commit**: `03a356db5a16e9c1c6fce6510d79b05a1f9fcb4e`
**Scope**: All Session 05 changes since the base commit
**Result**: RESOLVED

## Review Surface

The review covered the shared owner-aware menu API and renderer, all three production editor
integrations, the parser and OLC fixture extensions, the Session 01 compatibility adjustment, the
new production-linked tests, both build manifests, the builder guide, and the Session 05 workflow
artifacts.

The implementation was checked against the validated Session 04 registry, the frozen Session 01
world persistence and legacy accessor contract, the production Oasis mode and callback slots, and
the Session 05 exact filtered inventory and prerequisite trace.

## Findings By Severity

### Critical

No findings.

### High

No findings.

### Medium

- `unittests/CuTest/test_spec_owner_aware_olc.c` - The initial scenarios reloaded mobile, object,
  and room parser fixtures repeatedly in the parent CuTest process. Those legacy parsers retain
  process-global load counters, so the second scenario could index beyond the one-record fixture.
  | Fix: Move the existing fork-isolated scenario runner into the shared fixture module and run
  every parser-backed owner-aware scenario once in its own child and private sandbox. | Status:
  FIXED

### Low

- `unittests/CuTest/test_spec_fixtures.c` - The first menu-opening helper repeated three structurally
  identical switch branches and triggered a changed-code branch-clone diagnostic. | Fix: Replace
  the switch with a bounds-checked owner-to-main-mode table. | Status: FIXED
- `unittests/CuTest/test_spec_owner_aware_olc.c` and `src/olc/spec_menu.c` - Review coverage did not
  initially exercise empty and whitespace-only choices, and valid-result assertions dereferenced
  definitions without explicit analyzer guards; `strtol` also relied on an indirect standard
  include. | Fix: Add direct empty/whitespace and production state-preservation checks, explicit
  null guards, and `<stdlib.h>`. | Status: FIXED

## Behavioral Quality Review

| Category | Result | Evidence |
|----------|--------|----------|
| Inputs and preconditions | PASS | Null, empty, whitespace, negative, partially numeric, overflowing, low, high, and invalid-owner inputs are bounded and tested. |
| Happy path | PASS | Exact 18/5/6 inventories and every valid production editor selection map to the canonical handler. |
| Failure paths | PASS | Invalid input preserves the previous callback and dirty state; unsupported and empty views are explicit. |
| State and side effects | PASS | Selection changes only the existing OLC callback slot; `MOB_SPEC` and `ITEM_AUTOPROC` remain unchanged. |
| Mapping | PASS | Canonical registry order defines each filtered view; aliases never create duplicate rows. |
| Observability | PASS | Every row includes category, description, events, and per-event flags or placement prerequisites. |
| Cleanup | PASS | Parser-backed scenarios run in isolated children and restore fixture globals, descriptors, working directories, and sandboxes. |

## Compatibility Review

- The 29-position legacy compatibility accessor remains unchanged.
- Persisted names, mobile `SpecProc:` records, object and room `Z` records, writer behavior, and the
  `SPECIAL` callback ABI remain unchanged.
- Medit, oedit, and redit retain their existing clear, quit, save, and dirty-state semantics.
- Selection does not set prototype flags or alter runtime dispatch order.
- `Guildmaster` remains loadable as an alias while the menu presents one canonical `Guild` row.
- New production and test membership is synchronized across Automake and CMake.

## Deliberate Non-Fixes

- The editor continues to store one legacy handler pointer. Authored raw identity and provenance are
  Sessions 06-07 scope; typed dispatch and multiple handlers remain later-phase work.
- Scheduling and placement prerequisites are descriptive only. Builders retain explicit control of
  `MOB_SPEC`, `ITEM_AUTOPROC`, equipment, and placement state.
- The final in-game `SPECIALS` help replacement remains Session 09 scope, when all Phase 00
  observability behavior is available.
- Restricted analyzer runs exclude inherited structure-padding advisories and report no active
  diagnostic in changed code. Two analyzer paths in untouched legacy editor code remain outside
  this session.

## Evidence Ledger

| Check | Command Or Inspection | Result | Evidence |
|-------|-----------------------|--------|----------|
| Production-linked suite | `make -j$(nproc) cutest && ./cutest` | PASS | 529/529 CuTests passed. |
| Secondary build | Independent CMake `cutest` target | PASS | All final sources compiled under GNU C23. |
| Secondary test | CTest `production-cutest` | PASS | Passed in 22.84 seconds. |
| Compiler | Autotools and CMake builds | PASS | No new `-Wall -Wextra` warning. |
| Static analysis | Restricted and line-filtered `clang-tidy` | PASS | No active changed-code diagnostic after repairs. |
| Formatter | `clang-format --dry-run --Werror` | PASS | No formatting drift. |
| Integrity | Manifest, encoding, protected-path, and diff scans | PASS | Membership is exact; no protected or world file changed. |

## Summary

The complete Session 05 surface was reviewed and one Medium plus two Low findings were repaired.
No unresolved Critical, High, Medium, or Low finding remains. The shared menu is owner-correct,
metadata-complete, strictly mapped, production-linked, and compatible with existing persistence and
runtime activation behavior.
