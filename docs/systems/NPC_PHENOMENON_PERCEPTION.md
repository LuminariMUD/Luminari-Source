# NPC phenomenon perception and response

`WorldPhenomenon` is the authoritative fact that a source produced a spatial
signal. It carries an opaque phenomenon ID, source and source-room handles,
phenomenon kind, source faction, optional stealth DC, channels, propagation,
range, intensity and player-facing descriptions. Descriptions are presentation
only. NPC behavior never parses them.

The spatial consumer resolves each eligible, materialized NPC through the same
visual or audio rules used for player delivery and publishes one merged
`PhenomenonPerceived` fact per observer. That fact contains the senses that
reached the observer, distance, attenuated intensity and source knowledge.
Source identity is present only when the observer knows it through faction
membership or unobstructed sight; an audible unknown source does not leak its
entity handle.

## Bounded discovery

Room propagation visits at most 256 rooms and clamps each channel to eight
exits. Coordinate propagation clamps the search radius to 32 cells, examines at
most 1,024 materialized NPC candidates and publishes at most 256 perception
facts. Blind, deaf, unconscious and `MOB_NO_AI` observers are rejected before
publication. Capacity and publication failures increment rate-limited
telemetry. No global character-list scan or dormant-world wakeup occurs.

Coordinate discovery currently queries materialized rooms in the bounded cell
window and then applies the existing elevation, terrain, weather and obstacle
strategies. A future spatial index may replace that discovery step without
changing the event contract.

## Native response pilot

NPCs authored with `MOB_LISTEN`, `MOB_GUARD`, `MOB_HELPER` or
`MOB_MOB_ASSIST` can own one current phenomenon interest. A perceived fact
schedules `npc.phenomenon-interest` one tick later. Duplicate IDs do not create
more work; a newer ID replaces the old interest and its deadline.

The first callback warns the room. Fire and magical impact make the NPC take a
temporary total-defense posture. A listening, non-sentinel NPC investigates an
adjacent reachable source room through normal committed movement. The interest
and any posture installed by it expire after 30 seconds. Character extraction,
replacement, runtime shutdown and stale generation handles cancel safely.

This is an intentionally narrow policy. Longer pursuit, reinforcement groups,
faction escalation and pathfinding should subscribe as separate consumers of
the same perception fact rather than being embedded in propagation.

## Gateway classification

Every integration must be classified before it is bridged:

| Gateway | Boundary | Rule |
|---|---|---|
| DG leave/entry/greet and blocking special procedures | Pre-operation decision | May veto or redirect before a committed fact exists. |
| Command-intercepting special procedures | Pre-operation decision | Retain synchronous command ownership; publish a later outcome only if a consumer needs it. |
| Committed movement, transfer, damage and death | Post-operation notification | Consumers re-resolve handles and never undo the operation. |
| Quest delivery and location discovery | Post-operation notification | Subscribe only while the objective is active; award once from final state. |
| Nonlethal resolution and rescue | Post-operation notification | Add a typed resolution fact only when an authored objective consumes it; preserve the existing defeat/save authority. |
| Dialogue and skill outcomes | Post-operation notification | Publish the resolved skill, DC and outcome, never inferred command text. |
| Witnessed actions | Perception-derived notification | A consequence requires a `PhenomenonPerceived` result for that witness; the underlying action fact alone is insufficient. |

The current authoritative quest pilots are location discovery from committed
relocation and item discovery/delivery from committed transfer. Existing
death/group/pet credit remains authoritative until it is migrated as one award
path with persistence and exactly-once tests. Rescue, dialogue, general skill
and witnessed-action objectives require authored quest schema before their
facts are added; creating unused broad event types would weaken the contract.

## Verification contract

Tests cover closed-door visual blocking with audible transmission, faction
source knowledge, blindness, stealth suppression, bounded admission, duplicate
deduplication, replacement, temporary defense expiry and adjacent
investigation through normal movement.
