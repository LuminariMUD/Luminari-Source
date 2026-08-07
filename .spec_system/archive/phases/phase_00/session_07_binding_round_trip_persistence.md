# Session 07: Binding Round-Trip Persistence

**Session ID**: `phase00-session07-binding-round-trip-persistence`
**Status**: Complete
**Estimated Tasks**: ~18-22
**Estimated Duration**: 2-4 hours

---

## Objective

Make mobile, object, and room OLC writers persist authored binding intent instead of reconstructing
identity from the effective callback pointer.

---

## Scope

### In Scope (MVP)

- Carry authored binding metadata into and out of medit, oedit, and redit working copies.
- Make all three writers prefer authored identity over reverse function-pointer lookup.
- Preserve known authored names across unrelated edits and hard-coded effective overrides.
- Preserve unresolved raw names across unrelated edits without silently clearing them.
- Define explicit builder replace and clear actions for known, aliased, incompatible, and unresolved
  identities.
- Retain canonical save behavior for intentionally selected definitions and stable compatibility
  aliases.
- Add load-edit-save-reload tests for mobile, object, and room world formats.

### Out of Scope

- Changing world-file syntax or adding multiple names per prototype.
- Changing hard-coded, shop, or quest precedence.
- Startup effective-binding summaries and moving-room conflict policy.

---

## Prerequisites

- [x] Session 06 provides owned authored binding state and loader diagnostics.
- [x] Session 05 provides compatible select, replace, and clear editor flows.

---

## Deliverables

1. Authored-state-aware mobile, object, and room writers.
2. Explicit replace and clear semantics for resolved and unresolved names.
3. Production-linked three-owner round-trip and override-provenance tests.
4. Removal of implicit authored identity recovery from effective pointers where state is available.

---

## Success Criteria

- [x] An unrelated OLC save never erases an unresolved authored name.
- [x] A boot-time override never promotes its effective callback into authored world data.
- [x] Explicit select, replace, and clear actions produce deterministic canonical output.
- [x] Existing single-name world files remain backward compatible and round-trip cleanly.
