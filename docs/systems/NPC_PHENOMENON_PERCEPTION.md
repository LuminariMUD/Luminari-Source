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

## Bounded discovery and objective admission

Room propagation visits at most 256 rooms and clamps each channel to eight
exits. Coordinate propagation clamps the search radius to 32 cells, examines at
most 1,024 materialized candidates and publishes at most 256 perception facts.
Blind, deaf and unconscious observers are rejected before publication;
`MOB_NO_AI` also rejects NPCs. A connected player is admitted only while that
player has an active `AQ_WITNESS_PHENOMENON` objective for the exact phenomenon
kind. The room graph and materialized coordinate-room pool supply the
candidates, so this does not add a global character-list scan or wake dormant
world state. Capacity and publication failures increment rate-limited
telemetry.

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

## Outcome-based quest objectives

Three appended quest types preserve every existing persisted type number:

| Type | Authored target | Authoritative input |
|---|---|---|
| `AQ_MOB_RESOLVE` (25) | Mobile VNUM | A committed rescue or negotiation in the actor's current room. |
| `AQ_SKILL_SUCCESS` (26) | Ability number | A successful completed ability check. Dialogue publishes the exact roll, modifier and DC; room search publishes its composite result. |
| `AQ_WITNESS_PHENOMENON` (27) | Phenomenon kind | `PhenomenonPerceived` for that player with at least one usable sense. |

The quest editor, flat-file documentation, source-derived world-tool manifest
and semantic validator understand these target domains. Invalid ability or
phenomenon targets are rejected by the editor and world validation.

Quest consumers run after the relevant operation commits and re-resolve all
generation-safe handles. Successful `rescue` publishes `CharacterResolved`
after combat ownership and action cost have changed. Successful dialogue
publishes both `SkillResolved` and a negotiated `CharacterResolved`; failed
dialogue publishes only the failed skill result. Completed room search
publishes its Perception result after the atomic search mutation finishes.

Movement discovery, object discovery/delivery, dialogue, nonlethal resolution,
skill success and witnessed phenomena now use typed committed facts. The old
direct object-acquisition call was removed so one transfer cannot advance an
objective twice. Existing death/group/pet credit remains on its single
authoritative path, including its persistence and reward behavior. Quest
completion still uses the existing `eQUEST_COMPLETE` guard, so repeated or
nested publications cannot schedule the same final reward more than once.

## Verification contract

Tests cover closed-door visual blocking with audible transmission, faction
source knowledge, blindness, stealth suppression, bounded admission, duplicate
deduplication, replacement, temporary defense expiry and adjacent
investigation through normal movement. Production-linked quest coverage checks
committed nonlethal resolution, failed and successful skill outcomes, and a
blind witness receiving no credit before a later visible phenomenon succeeds.
World-tool tests cover the appended type table and target bounds.
