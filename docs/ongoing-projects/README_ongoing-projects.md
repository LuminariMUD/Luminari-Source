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

Retained legacy statuses were re-verified against the source tree on 2026-08-17. Newer entries carry
their own analysis dates. Most retained legacy notes are grouped in
[`todo-zusuk/`](todo-zusuk/); current cross-cutting plans may live beside this index.

| Document | Status | What remains |
|----------|--------|--------------|
| [CAMPAIGN_VARIANT_RETIREMENT_SCOPE.md](CAMPAIGN_VARIANT_RETIREMENT_SCOPE.md) | Implementation complete; pending review and merge | All removal and verification gates pass. No campaign-framework preservation or redesign was added. Supersedes `CAMPAIGN_VARIANT_REMOVAL_SCOPE.md`. |
| [PERFMON_PRODUCTION_STATUS.md](PERFMON_PRODUCTION_STATUS.md) | Production acceptance in progress | Correct registry discrepancies, remove the remaining zone/save tails, control reset-driven entity growth, and retain a complete inter-copyover capture. |
| [OBJ_FILE_FORMAT_MAPPING.md](OBJ_FILE_FORMAT_MAPPING.md) | Research complete | Field-by-field reference for the Luminari and Realms of Luminari `.obj` formats, traced to their loaders and writers, with a side-by-side comparison and the conversion hazards that follow from it. Reference material for the RoL converter work; nothing to implement here. |
| [ROL_CONVERTER_OBJECT_FILE_REFERENCES.md](ROL_CONVERTER_OBJECT_FILE_REFERENCES.md) | Released and applied to the dev world; gap list open | Object-conversion file index, six fidelity fixes made by auditing the converter against [OBJ_FILE_FORMAT_MAPPING.md](OBJ_FILE_FORMAT_MAPPING.md), the Phase 7/8 release that applied them, and eleven remaining gaps. Highest-impact open items are armor AC scale, armor type index, and weapon type index, each of which needs a balance decision rather than a format correction. |
| [ROL_CONVERTER_WEAPON_TYPE_INFERENCE.md](ROL_CONVERTER_WEAPON_TYPE_INFERENCE.md) | Proposal / Brainstorm | Multi-tier inference architecture for Item 3.3 (weapon type index) across the 1,319 RoL weapons. Corpus claims re-measured against the source tree; nine corrections to the first draft are recorded in its section 8, including the `MAX_WEAPON_NDICE` clamp that reframes the problem and a slot-ordering bug in the implementation plan. |
| [ROL_CONVERTER_ARMOR_TYPE_INFERENCE.md](ROL_CONVERTER_ARMOR_TYPE_INFERENCE.md) | Proposal / Brainstorm | Family-by-slot inference architecture for Item 3.2 (armor type index). Establishes that `SPEC_ARMOR_TYPE_*` is a 13-family by 4-slot grid, so the decision is 13-way rather than 58-way, and that only 1,157 of 2,523 records sit on slots `armor_list[]` covers. Proposes Item 3.2b for the other 1,366. |
| [ROL_CONVERTER_FILE_ORGANIZATION_SCOPE.md](ROL_CONVERTER_FILE_ORGANIZATION_SCOPE.md) | Analysis complete; implementation not started | Split RoL source parsing and target emission by record format behind stable facades, preserve byte-identical output, reorganize tests, and update build and Phase 8 evidence lists. |
| [AI_TODO_IDEAS.md](todo-zusuk/AI_TODO_IDEAS.md) | Not started; plan needs revalidation | No conversation-history implementation exists. Rework the plan around the current AI dialogue entry points in `src/act.comm.c` and `src/act.comm.do_spec_comm.c`, then implement bounded history, prompt integration, management, and tests. |
| [DO_SKORE_PROJECT.md](todo-zusuk/DO_SKORE_PROJECT.md) | Partial; detailed views complete | Phase 1, Phases 2.1-2.5, and Phase 3.1 are implemented. Group, achievement, and clan views remain in Phase 3; caching and a dedicated screen-reader mode remain in Phase 4. System reference: [SKORE_SYSTEM.md](../systems/SKORE_SYSTEM.md). |
| [IDEA_LIST.md](todo-zusuk/IDEA_LIST.md) | Backlog | Production in-game idea queue snapshot from 2026-08-03, plus four earlier imported ideas no longer present in that queue. This is a source of candidates, not an implementation plan. |
| [MERGE_MUD_EVENTS.md](todo-zusuk/MERGE_MUD_EVENTS.md) | Cleanup groundwork present; unification not started | Event-specific cleanup callbacks and coverage now exist, but the core still branches on `isMudEvent`, the DG queue and MUD layer remain separate, and the proposed `event_create_ex`/context API has not been implemented. |
| [PET_SYSTEM_COMPARISON_LUMINARI_CHRONICLES_OF_KRYNN.md](todo-zusuk/PET_SYSTEM_COMPARISON_LUMINARI_CHRONICLES_OF_KRYNN.md) | Comparison complete; P0 persistence repaired | Admission policy, temporary lifetime, stable ownership, schema-source unification, and related regression coverage remain. The Chronicles sample remains reference-only. |
| [artifact-placement-plan.md](todo-zusuk/artifact-placement-plan.md) | World-builder handoff | Choose and build player-facing acquisition routes for all seventeen vault-staged artifacts, keep contract metadata aligned with placement, and run the handoff verification. The permanent engineering reference is [ARTIFACT_SYSTEM.md](../systems/ARTIFACT_SYSTEM.md). |

The temporary vessel workspace is retired. Durable vessel requirements and
the authoritative release-gate state live in
[Vessel System Product Requirements](../product-requirements/VESSEL_SYSTEM_REQUIREMENTS.md);
behavior and evidence live in the permanent system, testing, deployment, and
changelog documents.
