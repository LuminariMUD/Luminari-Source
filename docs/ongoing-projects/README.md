# Ongoing Projects

Developer workspace for work that is **not finished**. This is scratch space, not
official documentation - nothing here should be cited as a description of how the
game currently behaves.

When a project here completes, its enduring content belongs in the formal
documentation tree (`docs/systems/`, `docs/testing/`, `docs/guides/`,
`docs/deployment/`), with the outcome recorded in
[docs/CHANGELOG.md](../CHANGELOG.md). The working notes themselves can then be
deleted.

## Current contents

Statuses below were re-verified against the source tree on 2026-08-04.

| Document | Status | What remains |
|----------|--------|--------------|
| [bardic-performance-msdp-overflow-audit.md](bardic-performance-msdp-overflow-audit.md) | Full-system diagnosis complete | Repair Songweaver and performance-state invariants first; then batch affect updates, make OOB frames atomic, reconcile all song and perk contracts, update player-facing text, and add production-linked bard plus cumulative-output regression tests. |
| [world-validator-quest-systems-plan.md](world-validator-quest-systems-plan.md) | Completion audit | Quest-system implementation and validator-specific CI are complete. Repair the pre-existing workflow failures exposed by the literal full-Actions acceptance audit, prove a green remote run, then retire the plan again. |
| [artifact-placement-plan.md](artifact-placement-plan.md) | Handoff | Content brief for a world builder: acquisition routes for all seventeen artifacts, the single-instance reset contract, and verification steps. No code work outstanding. The artifact project's engineering sections are complete and its working notes have been retired; enduring content moved to [ARTIFACT_SYSTEM.md](../systems/ARTIFACT_SYSTEM.md) and [ARTIFACT_OBJECT_STATS_FROM_SOURCE_MUDS.md](ARTIFACT_OBJECT_STATS_FROM_SOURCE_MUDS.md). |
| [ARTIFACT_OBJECT_STATS_FROM_SOURCE_MUDS.md](ARTIFACT_OBJECT_STATS_FROM_SOURCE_MUDS.md) | Reference | Source-verified static object stats, flags, affects, procedures, and Realms artifact overlays for the HomelandMUD and RealmsOfLuminari snapshots, plus the functional source map and the recoverable behavior of Homeland's three prototype-missing artifacts. |
| [AI_TODO_IDEAS.md](AI_TODO_IDEAS.md) | Not started | Implementation plan for AI NPC conversation history. No `conversation_history` code exists yet. Its stated foundation still holds, but the dialogue trigger has moved to `src/act.comm.c:526-529` from the `545-548` the document cites. |
| [DO_SKORE_PROJECT.md](DO_SKORE_PROJECT.md) | Partial | Phase 1 and Phases 2.1-2.5 complete; Phase 3 (detailed views) and Phase 4 open. System reference: [SKORE_SYSTEM.md](../systems/SKORE_SYSTEM.md) |
| [MERGE_MUD_EVENTS.md](MERGE_MUD_EVENTS.md) | Not started | Plan to unify the DG event queue and the MUD event layer. Both `src/dgscript/dg_event.c` and `src/mud_event.c` still exist separately. |
| [IDEA_LIST.md](IDEA_LIST.md) | Backlog | Production in-game idea queue snapshot from 2026-08-03, plus seven earlier imported ideas no longer present in the current queue. Not a plan - a source of candidates. |
| [agent-playthrough.md](agent-playthrough.md) | Reference | Record of a live new-account-to-level-2 production playtest, verified 2026-07-27. An observation log, not a project - useful as a first-hour-experience baseline. |

The temporary vessel workspace is retired. Durable vessel requirements and
the authoritative release-gate state live in
[Vessel System Product Requirements](../product-requirements/VESSEL_SYSTEM_REQUIREMENTS.md);
behavior and evidence live in the permanent system, testing, deployment, and
changelog documents.
