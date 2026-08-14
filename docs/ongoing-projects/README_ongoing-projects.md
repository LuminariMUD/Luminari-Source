# Ongoing Projects

> This is a planning index, not a current behavior reference.

Developer workspace for work that is **not finished**. This is scratch space, not
official documentation - nothing here should be cited as a description of how the
game currently behaves.

When a project here completes, its enduring content belongs in the formal
documentation tree (`docs/systems/`, `docs/testing/`, `docs/guides/`,
`docs/deployment/`), with the outcome recorded in
[docs/CHANGELOG.md](../CHANGELOG.md). The working notes themselves can then be
deleted.

## Current contents

Statuses below were re-verified against the source tree on 2026-08-14.

| Document | Status | What remains |
|----------|--------|--------------|
| [REALMS_OF_LUMINARI_CANONICAL_CONVERSION_PLAN.md](RoL/REALMS_OF_LUMINARI_CANONICAL_CONVERSION_PLAN.md) | Complete through Phase 8 | No conversion work remains; retain the sealed run identities and acceptance contract until the working plan is archived. |
| [RoL-Changelog.md](RoL/RoL-Changelog.md) | Complete through Phase 8 | Preserve the Phase 6.5, Phase 7, and Phase 8 evidence; entries through Phase 6 remain in the linked archive. |
| [REALMS_OF_LUMINARI_WORKNOTES.md](RoL/plan-archive/REALMS_OF_LUMINARI_WORKNOTES.md) | Archived Phase 6.5 handoff | Preserve the completed Phase 6-6.5 run identities, validation state, and Phase 7 continuation notes. |
| [PHASE4_MANUAL_TESTING.md](RoL/plan-archive/PHASE4_MANUAL_TESTING.md) | Archived Phase 4-6.5 test matrix | Preserve the staged pilot and canonical rebase walkthrough/reset evidence for historical reference. |
| [PET_SYSTEM_COMPARISON_LUMINARI_CHRONICLES_OF_KRYNN.md](PET_SYSTEM_COMPARISON_LUMINARI_CHRONICLES_OF_KRYNN.md) | Comparison complete; P0 persistence repaired | Luminari now has fail-closed migrations and atomic snapshots. Admission policy, temporary lifetime, stable ownership, and full schema-source unification remain design work; the Chronicles sample is still reference-only. |
| [artifact-placement-plan.md](artifact-placement-plan.md) | Handoff | Content brief for a world builder: acquisition routes for all seventeen artifacts, the single-instance reset contract, and verification steps. No code work outstanding. The artifact project's engineering sections are complete and its working notes have been retired; enduring content moved to [ARTIFACT_SYSTEM.md](../systems/ARTIFACT_SYSTEM.md). |
| [AI_TODO_IDEAS.md](AI_TODO_IDEAS.md) | Not started | Implementation plan for AI NPC conversation history. No `conversation_history` code exists yet. Its stated foundation still holds, but the dialogue trigger has moved to `src/act.comm.c:526-529` from the `545-548` the document cites. |
| [DO_SKORE_PROJECT.md](DO_SKORE_PROJECT.md) | Partial | Phase 1 and Phases 2.1-2.5 complete; Phase 3 (detailed views) and Phase 4 open. System reference: [SKORE_SYSTEM.md](../systems/SKORE_SYSTEM.md) |
| [MERGE_MUD_EVENTS.md](MERGE_MUD_EVENTS.md) | Not started | Plan to unify the DG event queue and the MUD event layer. Both `src/dgscript/dg_event.c` and `src/mud_event.c` still exist separately. |
| [IDEA_LIST.md](IDEA_LIST.md) | Backlog | Production in-game idea queue snapshot from 2026-08-03, plus seven earlier imported ideas no longer present in the current queue. Not a plan - a source of candidates. |
| [INQUISITOR_DOMAIN_FINDINGS.md](INQUISITOR_DOMAIN_FINDINGS.md) | Fixed | Documents the three original blockers and the implemented feat grant, login migration, shared scaling rule, help updates, and production-linked regression coverage. |
| [agent-playthrough.md](agent-playthrough.md) | Reference | Record of a live new-account-to-level-2 production playtest, verified 2026-07-27. An observation log, not a project - useful as a first-hour-experience baseline. |

The temporary vessel workspace is retired. Durable vessel requirements and
the authoritative release-gate state live in
[Vessel System Product Requirements](../product-requirements/VESSEL_SYSTEM_REQUIREMENTS.md);
behavior and evidence live in the permanent system, testing, deployment, and
changelog documents.
