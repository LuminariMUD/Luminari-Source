# Session Specification

**Session ID**: `phase00-session09-documentation-and-phase-validation`
**Phase**: 00 - Registry Safety and Observability
**Status**: Complete
**Created**: 2026-08-07
**Base Commit**: 1a73cd5c78530e6e08847f1d95d62150f9850e04

---

## 1. Session Overview

This session closes Phase 00 by reconciling builder, in-game help, developer, architecture, help
system, testing, and index documentation with the behavior delivered in Sessions 01 through 08. It
also turns the phase requirements and required-test matrix into a durable evidence map tied to the
production-linked suite.

No callback, registry, binding, world format, or runtime behavior changes in this session. The final
gate reruns both supported build paths, all production-linked and auxiliary tests, the database help
contract, manifest parity, and repository integrity before Phase 00 is declared complete.

---

## 2. Objectives

1. Document the current definition, authored-binding, effective-binding, OLC, persistence, and
   compatibility boundaries for builders, operators, and developers.
2. Make the database-first `SPECIALS` help entry cover every delivered builder workflow and safety
   rule with executable verification.
3. Map every Phase 00 exit criterion and required test area to exact production source, test, and
   prior-session evidence.
4. Prove the complete phase with Autotools, CMake/CTest, MariaDB help checks, documentation checks,
   and protected-data integrity checks.

---

## 3. Prerequisites

### Required Sessions

- [x] Sessions 01 through 03 - Registry, persistence, command, pulse, combat, and secondary
      characterization.
- [x] Sessions 04 and 05 - Validated immutable definitions and owner-aware OLC.
- [x] Sessions 06 through 08 - Owned authored identity, round-trip persistence, effective
      provenance, and moving-room collision safety.

### Required Tools Or Knowledge

- Root production-linked CuTest and independent CMake/CTest workflows.
- MariaDB client access through the existing development-only credential configuration.
- Knowledge of the database-first help system and checked-in SQL component conventions.

### Environment Requirements

- Clean development checkout at the published Session 08 commit.
- Existing `lib/.env` identifies a development environment; protected configuration and
  credential files remain read-only.
- The current installed server is available at `bin/circle` and no root-level `circle` exists.

---

## 4. Scope

### In Scope (MVP)

- Builder and staff can consult one current OLC guide and database help entry covering canonical
  names, aliases, owner filtering, event prerequisites, authored preservation, explicit replacement
  or clear, effective provenance, collision outcomes, and moving-room exclusivity.
- Maintainers can consult developer and core architecture documentation that separates immutable
  definitions, owned authored state, effective boot observations, legacy callback authority, and
  future event gateways.
- Test maintainers can use one Phase 00 validation matrix mapping all phase requirements to the 78
  dedicated production-linked tests, supporting source contracts, and validation commands.
- Operators can distinguish delivered normal and `no_specials` diagnostics from future dispatch or
  composition proposals.
- Both build manifests, the SQL component manifest, documentation encoding, checked-in world data,
  and protected local files receive a final phase-wide integrity audit.

### Out Of Scope (Deferred)

- Phase 01 event gateways, typed contexts, flow, invalidation, and call-site rewrites - these remain
  proposed until separately characterized implementation sessions.
- Declarative assignment conversion, content extraction, shared mechanics, typed-handler migration,
  and general composition - these belong to later phases.
- Runtime callback behavior changes or new world-file syntax - Phase 00 closes on the compatibility
  behavior already delivered and tested.

---

## 5. Technical Approach

### Architecture

Documentation follows the implemented control-plane layers from source authority outward:
`spec_registry` owns immutable definitions, `spec_binding` owns exact authored requests,
`spec_effective_binding` observes ordered boot writes, prototype callback pointers remain dispatch
authority, and OLC/world writers preserve authored identity. The architecture and developer guides
will name each ownership and lifetime boundary rather than presenting a future gateway as current.

The database-first help migration remains the authoritative in-game source. Its verifier will assert
the required builder concepts, keywords, and conflict cleanup. A new testing evidence document will
map Phase 00 requirements to exact source and test files without treating later project phases as
completed.

### Design Patterns

- **Single source of truth**: Link documentation claims to canonical registry, binding, help SQL,
  and production test owners.
- **Evidence matrix**: Map each acceptance criterion to executable evidence and its authoritative
  implementation surface.
- **Current versus proposed labeling**: Describe Phase 00 behavior in present tense and isolate
  gateway, composition, and typed-handler plans as future work.
- **Read-only validation**: Use temporary database tables and digest checks so closeout cannot
  mutate production or builder-owned data.

---

## 6. Deliverables

### Files To Create

| File | Purpose | Est. Lines |
|------|---------|------------|
| `docs/testing/SPECIAL_PROCEDURE_PHASE_00_VALIDATION.md` | Phase exit criteria, test ownership, evidence matrix, and reproducible gates. | ~220 |

### Files To Modify

| File | Changes | Est. Lines |
|------|---------|------------|
| `docs/guides/OLC_SpecProcs.md` | Reconcile complete builder lifecycle, diagnostics, aliases, and future boundaries. | ~45 |
| `sql/components/help_specproc_entries.sql` | Expand database-first builder and staff help to the complete Phase 00 contract. | ~25 |
| `sql/components/verify_help_specproc_entries.sql` | Assert the expanded help contract and keyword ownership. | ~20 |
| `docs/guides/DEVELOPER_GUIDE_AND_API.md` | Document definition, authored, effective, persistence, and extension APIs. | ~100 |
| `docs/systems/CORE_SERVER_ARCHITECTURE.md` | Document boot precedence, callback authority, lifecycle ownership, and diagnostics. | ~90 |
| `docs/systems/HELP_SYSTEM.md` | Clarify database-first authority and checked-in migration workflow. | ~35 |
| `docs/guides/TESTING_GUIDE.md` | Register the Phase 00 suite and reproducible validation matrix. | ~45 |
| `docs/TECHNICAL_DOCUMENTATION_MASTER_INDEX.md` | Register builder and phase-validation references and refresh index metadata. | ~15 |
| `docs/CHANGELOG.md` | Summarize the delivered Phase 00 registry-safety slice. | ~15 |

---

## 7. Success Criteria

### Functional Requirements

- [x] Builder, operator, developer, architecture, help-system, and testing documents agree on the
      implemented Phase 00 behavior.
- [x] The in-game help source covers canonical selection, aliases, owner filtering, prerequisites,
      unresolved preservation, explicit clear or replace, effective diagnostics, and moving-room
      rejection.
- [x] Every Phase 00 exit criterion and required coverage area maps to exact passing evidence.
- [x] Future gateways, assignment conversion, extraction, shared helpers, typed handlers, and
      composition are clearly labeled as deferred.

### Testing Requirements

- [x] All 78 dedicated special-procedure tests remain present in the production-linked suite.
- [x] Root `make test` passes, followed immediately by `make install`.
- [x] A fresh CMake build passes the complete CTest matrix.
- [x] The help migration is idempotent and every read-only verifier query passes on temporary
      tables.

### Non-Functional Requirements

- [x] Automake and CMake contain identical production and CuTest source membership for Phase 00.
- [x] No root `circle`, temporary validation tree, credential edit, protected-header edit, or
      checked-in world-data change remains.
- [x] Documentation references resolve to real files, APIs, test owners, and current counts.

### Quality Gates

- [x] All changed text is ASCII-compatible UTF-8.
- [x] All changed text uses Unix LF line endings.
- [x] Documentation follows repository naming, ownership, and current-versus-proposed conventions.
- [x] Builder-facing help contains product guidance; raw implementation details remain in developer
      and operator sections.

---

## 8. Implementation Notes

### Working Assumptions

- The dedicated Phase 00 inventory is 78 production-linked test functions across eight test source
  files at the base commit. This is derived directly from `void Test...` definitions and will be
  rechecked rather than treated as an architectural constant.
- Existing Session 01 through 08 validation reports are accepted evidence only when the final full
  phase gates also pass; prior PASS status does not replace Session 09 execution.

### Conflict Resolutions

- The Session 09 stub and older master-PRD wording name `lib/text/help/help.hlp`, but the implemented
  help system and Session 08 review establish database-first help with checked-in SQL migrations as
  authoritative. Update `sql/components/help_specproc_entries.sql` and its verifier; leave the
  ignored runtime file untouched.
- The master project success list includes gateway and shared-helper outcomes assigned to later
  phases, while the Phase 00 PRD and detailed Phase 00 contract define this phase's exit. Close only
  the Phase 00 criteria and explicitly retain later project outcomes as deferred.

### Key Considerations

- Exact canonical names, counts, aliases, source order, and diagnostic tokens must be traced from
  current source before documentation changes.
- Database verification must use connection-local temporary tables and must not alter persistent
  help data.
- Final build validation must retain the required `make test` then `make install` order.

### Potential Challenges

- **Stale cross-document claims**: Use exact source/API searches and the evidence matrix to reconcile
  each claim before closure.
- **Double-counting tests**: Report 78 dedicated Phase 00 tests separately from the full CuTest count
  and from repeated CTest execution.
- **Help-source ambiguity**: Document the migration and verifier as source control authority while
  treating the legacy flat file as optional import compatibility only.

---

## 9. Testing Strategy

### Unit Tests

- Run all 78 dedicated special-procedure tests through the full production-linked CuTest binary.
- Confirm every Phase 00 test source is listed twice in Automake and once in CMake.

### Integration Tests

- Run root `make test`, then `make install`.
- Configure a fresh independent CMake build, compile the `cutest` target, and run full CTest.
- Apply the help migration twice to connection-local temporary tables and execute every verifier
  query.

### Runtime Verification

- Verify `bin/circle` is executable and current while the root-level `circle` artifact is absent.
- Verify documented APIs, source paths, SQL files, diagnostic tokens, and test owners exist.

### Edge Cases

- Alias compatibility must not be documented as a second canonical menu entry.
- Unknown and incompatible authored names must be documented as preserved but non-dispatching.
- `no_specials` must be described as path-specific behavior rather than a global callback gate.
- Moving-room `M` plus named room `Z` must be documented as rejected in both load orders and before
  OLC disk mutation.

---

## 10. Dependencies

### Other Sessions

- Depends on: all completed Phase 00 Sessions 01 through 08.
- Depended by: Phase 00 audit and Phase 01 planning.

---

## Next Steps

Run the `implement` workflow step to begin implementation.
