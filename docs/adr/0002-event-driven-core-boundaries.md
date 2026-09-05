# 2. Event-Driven Core Boundaries

**Status:** Accepted; amended 2026-09-05
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
  generation-aware entity owner. The ordinary product has one timing wheel;
  the legacy queue, adapters, and rollback build switches have been removed.
- The private reactor owns descriptor and signal readiness and arms one wakeup
  from the nearest real deadline. `libevent` is the ordinary product driver;
  `select()` is also supported as an I/O driver over the same native scheduler.
- The domain-event core synchronously reports typed facts after state changes.
  Its registry is sealed at boot, payloads are immutable and borrowed, entity
  references are resolved generation-aware handles, and nested causal chains
  are bounded.
- Player commands continue to enter through the interpreter. Pre-operation
  vetoes or value changes use separately specified typed decision hooks, never
  post-operation domain notifications.

Activities, combat, regeneration, automatic actions, AI, and active-world work
now use owner-scheduled callbacks and domain-event wakeups. Autonomous NPCs own
an agenda only while concrete work exists; spent resources, active behavior,
and bounded local facts add work, while completion and lifecycle teardown remove
it. The compatibility heartbeat, population-loop fallbacks, and their selectors
have been physically removed. Remaining service-driven feature scans are tracked
in the event mechanism inventory.

## Consequences

### Positive

- Due work is proportional to active owners instead of total world size.
- State changes can wake only relevant consumers without polling every entity.
- Main-thread gameplay ordering remains deterministic and easy to roll back.
- Deleted or replaced entities cannot be recovered through stale pointers.

### Negative

- New saves are not guaranteed to work in older executables; migration readers
  are retained, but the legacy save writer is removed.
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

The Phase 6a foundation registered typed fact contracts. Phase 6b added the
first production publisher/subscriber pair: `WorldPhenomenon` routes sights and
sounds through coordinate or bounded room-graph propagation, and Meteor Swarm
publishes it directly. Subsequent phases moved combat, activities, periodic
owners, autonomous mobiles, automatic procedures, DG triggers, vessels, and
active trail locations behind explicit ownership. Normal callbacks use stable
registries or bounded local graphs; population scans remain only in bootstrap,
staff validation, and explicitly inventoried service-owned work.

## References

- [Event-Driven Core Refactor Specification](../ongoing-projects/EVENT_DRIVEN_CORE_REFACTOR_SPEC.md)
- [MUD Event Systems](../systems/MUD_EVENTS.md)
- [Core Server Architecture](../systems/CORE_SERVER_ARCHITECTURE.md)

The 2026-09-05 door tranche removes the unused utility scheduler and establishes
committed, room-scoped door facts. Compound door mutations finish before
notification; pre-operation DG vetoes retain their decision semantics. Readied
door commands execute at a subsequent native boundary with owner/exit checks.
