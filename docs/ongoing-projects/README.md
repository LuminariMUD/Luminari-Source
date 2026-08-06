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

Statuses below were re-verified against the source tree on 2026-08-06.

| Document | Status | What remains |
|----------|--------|--------------|
| [PET_SYSTEM_COMPARISON_LUMINARI_CHRONICLES_OF_KRYNN.md](PET_SYSTEM_COMPARISON_LUMINARI_CHRONICLES_OF_KRYNN.md) | Comparison complete; design decisions open | Luminari has stronger runtime-state persistence, while the detached Chronicles sample has a clearer category model and preserves temporary-pet purge timers. Neither implementation is safe to adopt wholesale. Transactional saves, unconditional migrations, one central admission policy, explicit durability rules, and regression coverage remain. |
| [production-crash-2026-08-05-pet-persistence.md](production-crash-2026-08-05-pet-persistence.md) | Investigated - P0 containment pending | Production aborted after glibc detected corrupted heap metadata. The exact writer is unlocalized, but the missing `pet_data.runtime_state` migration and destructive delete-before-insert pet persistence path are confirmed. Production schema repair, affected-row recovery, transactional persistence, and a core-backed reproduction remain. |
| [ARTIFACT_MECHANICS_GAP_AUDIT.md](ARTIFACT_MECHANICS_GAP_AUDIT.md) | Audit complete; remediation open | All 17 live artifacts were traced against current runtime, prototypes, tests, and both source-MUD snapshots. Three identity-defining combat packages are missing, three current runtime defects need repair, and the remaining passive/content decisions are prioritized in ART-AUD-001 through ART-AUD-014. |
| [bardic-instrument-slot-audit.md](bardic-instrument-slot-audit.md) | Complete (retained audit record) | BI-001 through BI-010 are verified. The warning-free suite passes 410/410; authoritative help passes all database checks; generated docs, install, and repository safeguards are clean. No bardic instrument work remains. |
| [syntax-check-event-init-order.md](syntax-check-event-init-order.md) | Complete | Syntax-check initializes and cleans up the event queue exactly once, tolerates partial-boot subsystem cleanup, and exits cleanly for an encounter-bearing world. The production-linked suite covers both empty and encounter event lifecycles. |
| [mudlet-msdp-json-decoder-investigation.md](mudlet-msdp-json-decoder-investigation.md) | Complete (retained investigation record) | MJD-001, MJD-004, and MJD-005 are verified. Native scalar values are plain, the GMCP fallback is strict JSON in both directions, and all server gates pass. MJD-002 and MJD-003 are documented external findings outside server scope. |
| [bardic-performance-msdp-overflow-audit.md](bardic-performance-msdp-overflow-audit.md) | Complete (retained audit record) | BP-001 through BP-019 are verified. Clean warning-free ASan/UBSan and optimized suites pass 399/399, focused protocol coverage passes 22/22, and the installed artifact is clean. No work remains. |
| [artifact-placement-plan.md](artifact-placement-plan.md) | Handoff | Content brief for a world builder: acquisition routes for all seventeen artifacts, the single-instance reset contract, and verification steps. No code work outstanding. The artifact project's engineering sections are complete and its working notes have been retired; enduring content moved to [ARTIFACT_SYSTEM.md](../systems/ARTIFACT_SYSTEM.md). |
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
