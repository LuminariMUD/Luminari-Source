# Session 08: Effective Binding Observability

**Session ID**: `phase00-session08-effective-binding-observability`
**Status**: Complete
**Estimated Tasks**: ~20-24
**Estimated Duration**: 2-4 hours

---

## Objective

Expose every effective post-boot binding decision and reject incompatible room-slot ownership while
preserving current world, assignment, shop, quest, and no_specials outcomes.

---

## Scope

### In Scope (MVP)

- Record named world, parser-hook, legacy assignment, shop wrapper, and quest wrapper contributions.
- Preserve and report the verified boot precedence and final effective callback.
- Record collision outcomes and saved shop or quest secondary callbacks.
- Emit structured startup diagnostics with owner type, VNUM, requested identity, sources, and chosen
  result.
- Cover normal and -s behavior without turning no_specials into a new global dispatch gate.
- Reject combined moving-room M data and room Z binding during loading and OLC save or selection.
- Add production-linked precedence, collision, secondary, diagnostics, and room-conflict tests.

### Out of Scope

- Converting legacy assignments to declarative tables.
- Changing boot precedence, flattening shop or quest composition, or migrating bindings to world
  data.
- Runtime gateways, multiple-handler chains, or a separate moving-room typed hook.

---

## Prerequisites

- [x] Sessions 02 and 03 provide scheduling, mode, and secondary-callback characterization.
- [x] Sessions 06 and 07 provide durable authored identity and writer behavior.
- [x] Session 04 provides validated definitions and binding-source compatibility.

---

## Deliverables

1. Effective binding and collision records for every contributing source.
2. Startup diagnostics for final bindings, overrides, and shop or quest secondaries.
3. Safe loader and OLC rejection of moving-room M plus room Z ownership.
4. Production-linked normal, -s, precedence, collision, and rejection tests.

---

## Success Criteria

- [x] Operators can identify requested, contributing, and effective binding state for each
  prototype.
- [x] World, hard-coded, shop, and quest outcomes match the characterized boot sequence.
- [x] Saved secondary callbacks remain visible and behaviorally unchanged.
- [x] Normal and -s diagnostics reflect actual per-path behavior.
- [x] Incompatible moving-room and registered room-procedure ownership cannot be persisted or
  loaded.
