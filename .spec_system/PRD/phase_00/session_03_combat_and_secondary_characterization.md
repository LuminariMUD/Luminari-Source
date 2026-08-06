# Session 03: Combat and Secondary Characterization

**Session ID**: `phase00-session03-combat-and-secondary-characterization`
**Status**: Complete
**Completed**: 2026-08-06
**Tasks**: 22
**Estimated Duration**: 2-4 hours

---

## Objective

Freeze the combat, identification, maneuver, shop, and quest invocation contracts that share the
legacy SPECIAL callback but supply different tokens and interpret returns differently.

---

## Scope

### In Scope (MVP)

- Characterize mobile combat-turn callbacks and their position after normal attacks and cleave.
- Characterize item identification and weapon-hit notifications.
- Cover shieldblock, parry, glance, and dodge defense tokens exactly.
- Cover shieldpunch, shieldcharge, shieldslam, and mounted charge tokens exactly.
- Characterize ignored return values for notification-only calls.
- Characterize shop and quest secondary forwarding, nonzero propagation, and nested callbacks.
- Cover applicable actor, owner, target, combat-state, activation, and no_specials conditions.

### Out of Scope

- Typed runtime contexts, gateways, invalidation outcomes, or dispatch rewrites.
- Fixing unsafe iteration that belongs to the Phase 01 gateway migration.
- Registry metadata, OLC filtering, or binding-source persistence.

---

## Prerequisites

- [x] Sessions 01 and 02 provide production-linked test fixtures and manifest integration.
- [x] Every covered call site has been retraced for current arguments and return handling.

---

## Deliverables

1. Production-linked coverage for all remaining verified invocation categories.
2. Exact regression assertions for defense, maneuver, charge, and identify tokens.
3. Shop and quest secondary-composition characterization.
4. A complete Phase 00 legacy invocation and return-semantics matrix.

---

## Success Criteria

- [x] Each remaining verified invocation category has at least one production-linked assertion.
- [x] Exact tokens and caller-specific ignored or propagated returns are frozen.
- [x] Shop-over-original and quest-over-existing secondary behavior is characterized.
- [x] Tests preserve current dispatch and pass with zero new compiler warnings.
