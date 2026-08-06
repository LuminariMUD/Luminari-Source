# Session 06: Authored Binding Model

**Session ID**: `phase00-session06-authored-binding-model`
**Status**: Not Started
**Estimated Tasks**: ~20-24
**Estimated Duration**: 2-4 hours

---

## Objective

Retain the requested authored identity and source for mobile, object, and room bindings during world
loading, including owned unresolved names and explicit incompatibility diagnostics.

---

## Scope

### In Scope (MVP)

- Add authored binding metadata to the traced mobile, object, room, prototype, and OLC lifecycles.
- Record owner type, prototype VNUM, requested persisted name, resolved definition, source kind, and
  source location.
- Resolve canonical names and aliases without losing the originally requested identity needed for
  diagnostics.
- Own, copy, replace, and free unresolved raw names safely.
- Diagnose unknown and wrong-owner names with source location, owner type, VNUM, and requested name.
- Preserve the current callback slot outcome and world-file syntax during loading.
- Add known, aliased, unknown, incompatible, copy, and cleanup tests for all three owner types.

### Out of Scope

- OLC writer and unrelated-save round-trip behavior.
- Effective post-boot precedence, collision records, and startup summaries.
- Changes to legacy assignments, shop or quest nesting, or runtime dispatch.

---

## Prerequisites

- [ ] Session 04 provides validated canonical, alias, owner, and binding-source metadata.
- [ ] Session 05 defines valid builder-facing owner selection behavior.

---

## Deliverables

1. A memory-safe authored binding record integrated with all three owner types.
2. Loader integration for known, aliased, incompatible, and unresolved names.
3. Context-rich content diagnostics that retain unresolved identity.
4. Production-linked ownership, copy, cleanup, and load tests.

---

## Success Criteria

- [ ] Every named world binding retains its authored identity and source independently of its
  effective callback.
- [ ] Unknown names remain owned and diagnosable instead of collapsing to an untraceable null.
- [ ] Wrong-owner names are reported and cannot become an effective incompatible callback.
- [ ] Copy and free paths are leak-free and safe across prototypes and OLC working copies.
