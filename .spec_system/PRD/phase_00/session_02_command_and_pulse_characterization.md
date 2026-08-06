# Session 02: Command and Pulse Characterization

**Session ID**: `phase00-session02-command-and-pulse-characterization`
**Status**: Not Started
**Estimated Tasks**: ~20-24
**Estimated Duration**: 2-4 hours

---

## Objective

Freeze command-owner traversal and scheduled non-combat special-procedure behavior, including exact
arguments, activation gates, scheduling positions, return handling, and normal versus -s behavior.

---

## Scope

### In Scope (MVP)

- Characterize room, equipped-object, carried-object, mobile, and room-object command traversal
  order and first-nonzero stop behavior.
- Characterize mobile activity callbacks and their interaction with MOB_SPEC and default AI.
- Characterize worn and carried object auto-proc calls, null actors, zero and nonzero returns, and
  ITEM_AUTOPROC gating.
- Characterize moving-room relocation timing and its current null actor and moving-state payload.
- Verify the relative mobile_activity and proc_update heartbeat ordering.
- Cover relevant normal and -s or no_specials behavior without changing dispatch.

### Out of Scope

- Combat, identification, maneuver, shop, and quest invocation categories.
- Gateway extraction, successor caching, or intentional lifetime-safety changes.
- Registry metadata, OLC presentation, and binding provenance.

---

## Prerequisites

- [ ] Session 01 test fixtures and production-linked suite membership are available.
- [ ] Current command and heartbeat call sites have been retraced before test assertions are fixed.

---

## Deliverables

1. Production-linked command traversal characterization tests.
2. Mobile activity, object auto-proc, and moving-room characterization tests.
3. Activation-flag and normal versus -s coverage for the applicable paths.
4. A test-backed matrix of exact inputs, ordering, and caller-specific return behavior.

---

## Success Criteria

- [ ] All command owners execute in the verified order and a handled command stops later traversal.
- [ ] Mobile and object pulse behavior records exact flag, actor, fallback, and return contracts.
- [ ] Moving-room scheduling and payload translation are covered without changing their behavior.
- [ ] Applicable normal and -s differences are explicit and passing in production-linked tests.
