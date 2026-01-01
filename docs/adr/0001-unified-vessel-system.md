# 1. Unified Vessel System Architecture

**Status:** Accepted
**Date:** 2025-12-29
**Decision Makers:** Development Team
**Technical Story:** Vessel System Implementation (Phase 00-03)

## Context

LuminariMUD had three separate legacy vessel systems (CWG, Outcast, Greyhawk) with overlapping functionality, inconsistent interfaces, and maintenance burden. The wilderness coordinate system existed but vessels couldn't navigate it freely.

Key issues:
- Three separate codebases to maintain
- Inconsistent player commands across vessel types
- No free-roaming navigation on wilderness grid
- Limited terrain awareness
- No unified persistence layer

## Decision

Create a unified vessel system using Greyhawk as foundation with wilderness integration:

1. **Single Codebase:** Consolidate all vessel functionality into `vessels*.c` files
2. **Wilderness Integration:** Full X/Y/Z coordinate navigation (-1024 to +1024)
3. **Terrain Awareness:** Vessel types have defined terrain capabilities
4. **Multi-Room Interiors:** Dynamic interior room generation per vessel type
5. **Automation Layer:** Autopilot, waypoints, routes, NPC pilots
6. **Vehicle Support:** Lightweight vehicle tier for land transport
7. **Unified Interface:** Common commands across all transport types

## Consequences

### Positive
- Single codebase reduces maintenance burden by ~60%
- Consistent player experience across all vessel types
- Free-roaming navigation enables emergent gameplay
- Scalable design supports 500+ concurrent vessels
- Clean API enables future extensions

### Negative
- Legacy systems disabled (requires migration for existing content)
- Higher initial development cost (29 sessions across 4 phases)
- Increased memory per vessel (~1016 bytes vs simpler alternatives)

### Risks
- Content migration may have edge cases
- Players accustomed to old commands need to relearn

## Alternatives Considered

### Option 1: Maintain All Three Systems
**Description:** Keep CWG, Outcast, and Greyhawk as separate systems
**Pros:** No migration needed, backward compatible
**Cons:** Triple maintenance burden, inconsistent experience
**Why not chosen:** Unsustainable long-term

### Option 2: Simple Track-Based System
**Description:** Pre-defined paths between ports (like a rail system)
**Pros:** Simple implementation, predictable behavior
**Cons:** No free exploration, limited gameplay possibilities
**Why not chosen:** Too restrictive for open-world MUD

### Option 3: Client-Side Rendering
**Description:** Offload navigation visualization to MUD clients
**Pros:** Rich visual experience possible
**Cons:** Requires specific client support, inconsistent across clients
**Why not chosen:** Accessibility concerns, client dependency

## Implementation Notes

Implemented across 4 phases:
- Phase 00: Core vessel system (9 sessions)
- Phase 01: Automation layer (7 sessions)
- Phase 02: Simple vehicle support (7 sessions)
- Phase 03: Optimization and polish (6 sessions)

Total: 29 sessions, 353 unit tests, Valgrind clean

## References

- [VESSEL_SYSTEM.md](../VESSEL_SYSTEM.md) - System documentation
- [VESSEL_BENCHMARKS.md](../VESSEL_BENCHMARKS.md) - Performance data
- [PRD.md](../../.spec_system/PRD/PRD.md) - Full requirements
