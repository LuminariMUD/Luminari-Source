# 2. Event-Driven Core Boundaries

**Status:** Accepted
**Date:** 2026-08-30
**Decision Makers:** LuminariMUD maintainers
**Technical Story:** Event-Driven Core Refactor

## Context

The legacy main loop combines network readiness, player commands, timed work,
and broad heartbeat scans over characters, mobiles, rooms, and objects. Calling
all of those mechanisms "events" obscures their different ownership and
correctness requirements.

## Decision

LuminariMUD uses four explicit boundaries:

- The game scheduler owns delayed and recurring work for a typed,
  generation-aware entity owner. Its default timing-wheel backend retains a
  boot-time legacy queue rollback.
- The private reactor owns descriptor and signal readiness and arms one wakeup
  from the nearest real deadline. `libevent` is the production driver and
  `select()` remains a boot-time rollback driver.
- The domain-event core synchronously reports typed facts after state changes.
  Its registry is sealed at boot, payloads are immutable and borrowed, entity
  references are resolved generation-aware handles, and nested causal chains
  are bounded.
- Player commands continue to enter through the interpreter. Pre-operation
  vetoes or value changes use separately specified typed decision hooks, never
  post-operation domain notifications.

Activities, combat, regeneration, automatic actions, AI, and active-world work
will migrate incrementally to owner-scheduled callbacks and domain-event
wakeups. The compatibility heartbeat remains until every scan has an explicit
replacement and rollback gate.

## Consequences

### Positive

- Due work is proportional to active owners instead of total world size.
- State changes can wake only relevant consumers without polling every entity.
- Main-thread gameplay ordering remains deterministic and easy to roll back.
- Deleted or replaced entities cannot be recovered through stale pointers.

### Negative

- During migration, the scheduler, typed bus, and old heartbeat coexist.
- Each publisher and subscriber needs an owning-system audit to prevent dual
  side effects.
- Hidden synchronous handler chains require strict diagnostics and causal
  limits.

### Risks

- Publishing a fact through old and new paths could duplicate gameplay effects.
- A slow or recursive handler could delay player service.
- Treating notification handlers as vetoes could create false rollback
  semantics after state has already changed.

## Alternatives Considered

### Replace Commands With Scheduled Events

Rejected because ordinary commands are immediate interpreter work; queueing all
commands adds latency and does not remove world scans.

### Reuse Database Pub/Sub

Rejected because string topics, player subscriptions, durable rows, delivery
priorities, and a heartbeat queue do not satisfy the typed synchronous gameplay
contract.

### Run Gameplay Handlers Concurrently

Rejected because shared game state is single-threaded. Slow handlers are defects
to diagnose, not work to race against entity mutation.

## Implementation Notes

The Phase 6a foundation registered eight inert fact contracts. Phase 6b added
the first production publisher/subscriber pair: `WorldPhenomenon` routes sights
and sounds through coordinate or bounded room-graph propagation, and Meteor
Swarm publishes it directly. Other publisher/subscriber pairs still land behind
separate migration boundaries. Broad scan removal begins only after the owning
system proves equivalent behavior, lifecycle safety, bounded work, and
rollback.

## References

- [Event-Driven Core Refactor Specification](../ongoing-projects/EVENT_DRIVEN_CORE_REFACTOR_SPEC.md)
- [MUD Event Systems](../systems/MUD_EVENTS.md)
- [Core Server Architecture](../systems/CORE_SERVER_ARCHITECTURE.md)
