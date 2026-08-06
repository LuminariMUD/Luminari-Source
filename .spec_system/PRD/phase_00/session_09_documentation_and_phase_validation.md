# Session 09: Documentation and Phase Validation

**Session ID**: `phase00-session09-documentation-and-phase-validation`
**Status**: Not Started
**Estimated Tasks**: ~14-18
**Estimated Duration**: 2-4 hours

---

## Objective

Bring builder, help, developer, architecture, and test documentation into agreement with Phase 00
and prove the complete phase against both supported build manifests and production-linked tests.

---

## Scope

### In Scope (MVP)

- Update docs/guides/OLC_SpecProcs.md for canonical names, aliases, owner filtering, prerequisites,
  provenance, unresolved names, explicit clear or replace, and collision diagnostics.
- Update the SPECIALS help entry in lib/text/help/help.hlp for implemented builder and staff
  behavior.
- Update architecture and developer documentation for definition, authored binding, effective
  binding, validation, and compatibility boundaries.
- Update the technical documentation index if a new long-lived document is introduced.
- Audit the full Phase 00 acceptance and required-test matrix for gaps.
- Verify ASCII-only UTF-8 and LF endings in changed documentation and helpfiles.
- Run the clean production-linked test and install sequence and verify no root circle artifact.
- Verify new production and test source membership matches in Makefile.am and CMakeLists.txt.

### Out of Scope

- Phase 01 gateways, typed contexts, invalidation, and call-site rewrites.
- Declarative assignment conversion, content extraction, shared mechanics, or handler composition.
- Documentation for behavior that remains only proposed.

---

## Prerequisites

- [ ] Sessions 01 through 08 are completed and their behavior is stable.
- [ ] All session-specific production-linked tests pass independently.

---

## Deliverables

1. Updated builder, in-game help, developer, architecture, and index documentation as applicable.
2. A closed Phase 00 acceptance and test-coverage matrix.
3. Passing make test followed by make install with zero new warnings.
4. Verified build-manifest parity, ASCII/LF documentation, and clean artifact state.

---

## Success Criteria

- [ ] Documentation distinguishes delivered Phase 00 behavior from later proposals.
- [ ] Every Phase 00 requirement maps to passing production-linked evidence.
- [ ] Automake and CMake list every added production and test source consistently.
- [ ] make test and make install pass, bin/circle is current, and no root-level circle remains.
- [ ] Changed documentation and helpfiles are ASCII-only UTF-8 with LF endings.
