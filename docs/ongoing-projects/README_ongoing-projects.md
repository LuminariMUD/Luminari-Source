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

The Realms of Luminari conversion is under recovery. The production Luminari flat-file world has
been restored and boot-verified in development. An isolated, non-applied RoL candidate now has
exact source connection-graph parity and zero cross-world connections. The destructive namespace
policy, preserved damaged-world evidence, and outstanding persistent-state and mechanics work are
mapped in [ROL_CONVERSION_DAMAGE_MAP.md](ROL_CONVERSION_DAMAGE_MAP.md).

## Current contents

Statuses below were re-verified against the source tree on 2026-08-15.

| Document | Status | What remains |
|----------|--------|--------------|
| [VALGRIND_FULL_COMMAND_AUDIT_2026-08-15.md](VALGRIND_FULL_COMMAND_AUDIT_2026-08-15.md) | Audit complete; 5 findings open | Repair one craft-skill bounds error, two group-ability hangs, four command ownership leaks, Split Enchantment cooldown gating, and maximum-page descriptor overflow. |
| [FULL_COMMAND_SWEEP_FINDINGS_2026-08-15.md](FULL_COMMAND_SWEEP_FINDINGS_2026-08-15.md) | Resolved and verified | All 19 findings are repaired in development. Preserve the report as the sweep evidence and resolution map. |
| [ROL_CONVERSION_DAMAGE_MAP.md](ROL_CONVERSION_DAMAGE_MAP.md) | Luminari restored; isolated RoL graph candidate rebuilt and verified but not applied | Repair conversion-era database and player/house references, prove incompatible RoL mechanics are isolated, finish ownership and exclusion work, and run final release gates before applying the disjoint RoL overlay. |
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
