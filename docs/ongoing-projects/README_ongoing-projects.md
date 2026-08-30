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
| [EVENT_DRIVEN_CORE_REFACTOR_SPEC.md](EVENT_DRIVEN_CORE_REFACTOR_SPEC.md) | Controlling specification v1.3; Phases 1 through 6a accepted, Phase 6b next | Retire the database-backed pub/sub runtime, commands or replacements, wilderness metadata, and heartbeat queue with an explicit data deprecation and rollback plan. |
| [EVENT_DRIVEN_CORE_REFACTOR_PHASE6A_VALIDATION.md](EVENT_DRIVEN_CORE_REFACTOR_PHASE6A_VALIDATION.md) | Phase 6a acceptance evidence complete 2026-08-30 | Boot-sealed typed registry, deterministic bounded dispatch, generation-aware handles, diagnostics, eight foundational contracts, matrix, sanitizer, Valgrind, and no-dual-publication evidence. |
| [EVENT_DRIVEN_CORE_REFACTOR_PHASE5_VALIDATION.md](EVENT_DRIVEN_CORE_REFACTOR_PHASE5_VALIDATION.md) | Phase 5 acceptance evidence complete 2026-08-30 | Exhaustive persistence classification, versioned stable-owner restore, offline policy, boot reconstruction, live reboot/copyover/rollback, matrix, sanitizer, Valgrind, and cleanup evidence. |
| [EVENT_DRIVEN_CORE_REFACTOR_PHASE4_VALIDATION.md](EVENT_DRIVEN_CORE_REFACTOR_PHASE4_VALIDATION.md) | Phase 4 acceptance evidence complete 2026-08-30 | Permanent scheduler/reactor bridge, operational budget, representative workload, structural telemetry, matrix, live copyover, sanitizer, Valgrind, rollback, and residual-risk record. |
| [EVENT_DRIVEN_CORE_REFACTOR_PHASE3_VALIDATION.md](EVENT_DRIVEN_CORE_REFACTOR_PHASE3_VALIDATION.md) | Phase 3 acceptance evidence complete 2026-08-30 | Permanent implementation, fd/signal/copyover ownership, matrix, live-session, sanitizer, Valgrind, rollback, and residual-risk record for the compatibility reactor. |
| [EVENT_DRIVEN_CORE_REFACTOR_SPEC_REVIEW.md](EVENT_DRIVEN_CORE_REFACTOR_SPEC_REVIEW.md) | Adversarial review complete 2026-08-30; incorporated by specification v0.8 | Historical review of v0.4 with 28 findings. Version 0.8 records each disposition and assigns the work to explicit gates; the Phase 2.5 foundation and Phase 3 reactor obligations are now implemented and accepted. |
| [HELPFILE_SYNCHRONIZATION_DESIGN.md](HELPFILE_SYNCHRONIZATION_DESIGN.md) | Implementation complete; isolated end-to-end tests passing | Deploy the same revision to both endpoints, restart to apply schema version 2026082408, run a read-only live audit, explicitly choose the first common baseline, and perform the first separately authorized development/production rehearsal. No production mutation has been performed. |
| [CAMPAIGN_VARIANT_RETIREMENT_LIVE_TEST_REPORT.md](todo-zusuk/CAMPAIGN_VARIANT_RETIREMENT_LIVE_TEST_REPORT.md) | Live regression test passed | Gameplay evidence, test limits, state restoration, and six non-blocking follow-up findings for the retirement branch. |
| [OBJ_FILE_FORMAT_MAPPING.md](OBJ_FILE_FORMAT_MAPPING.md) | Research complete | Field-by-field reference for the Luminari and Realms of Luminari `.obj` formats, traced to their loaders and writers, with a side-by-side comparison and the conversion hazards that follow from it. Reference material for the RoL converter work; nothing to implement here. |
| [TBAMUD_DG_DAMAGE_TRIGGER_AUDIT.md](TBAMUD_DG_DAMAGE_TRIGGER_AUDIT.md) | Remediation complete 2026-08-23 | Source-level audit and completed remediation of upstream Mobile Damage trigger PR #151: owner-specific OLC bounds, safe synchronous return/wait semantics, positive-damage scope, lifecycle hardening, combat metadata, dual-source help, a real world example, comprehensive tests, and a live builder smoke pass. |
| [ROL_CONVERTER_OBJECT_FILE_REFERENCES.md](ROL_CONVERTER_OBJECT_FILE_REFERENCES.md) | Released and applied to the dev world; gap list open | Object-conversion file index, six fidelity fixes made by auditing the converter against [OBJ_FILE_FORMAT_MAPPING.md](OBJ_FILE_FORMAT_MAPPING.md), the Phase 7/8 release that applied them, and the remaining gaps. Weapon type index (3.3), the ranged chain (3.5), and the zone equipment-position shift (3.11) are resolved. Highest-impact open items are the armor AC scale and the armor type index. |
| [ROL_CONVERTER_WEAPON_TYPE_INFERENCE.md](ROL_CONVERTER_WEAPON_TYPE_INFERENCE.md) | Updated for native thrown weapons; builder review and next dev release open | All 1,319 melee weapons, 99 ranged records, and 44 quivers convert with native dart/blowgun separation, mixed ammo-pouch support, corrected equipment positions, archer launcher reload, and throwable-only archer mode. The full affected-object review is recorded in the thrown-weapons conversion audit; the broader builder packet remains open. |
| [THROWN_WEAPONS_IMPLEMENTATION_PLAN.md](THROWN_WEAPONS_IMPLEMENTATION_PLAN.md) | Complete; guarded Phase 8 apply and development live QA passed | Melee-by-default throwable weapons, explicit `throw` combat, deterministic pouch/inventory/wielded-last depletion, safe physical-projectile ownership, dart/blowgun separation, mixed ammo-pouch support, RoL converter integration, tests, synchronized help, and corrected reciprocal combat shutdown are complete. The final 827-test suite, sealed Phase 8 completion, and live development matrix pass. |
| [THROWN_WEAPONS_CONVERSION_AUDIT.md](THROWN_WEAPONS_CONVERSION_AUDIT.md) | Affected-object review complete; guarded development apply verified | Native type-14 inventory, all source dart/blowgun records, 20 throwing quivers, and all 42 `MOB_ROL_ARCHER` loadout outcomes, with reproducible counts, sealed Phase 8 evidence, and live converted-archer verification. |
| [ROL_CONVERTER_ARMOR_TYPE_INFERENCE.md](ROL_CONVERTER_ARMOR_TYPE_INFERENCE.md) | Proposal / Brainstorm | Family-by-slot inference architecture for Item 3.2 (armor type index). Establishes that `SPEC_ARMOR_TYPE_*` is a 13-family by 4-slot grid, so the decision is 13-way rather than 58-way, and that only 1,157 of 2,523 records sit on slots `armor_list[]` covers. Proposes Item 3.2b for the other 1,366. |
| [ROL_TAIL_SLOT_IMPLEMENTATION.md](ROL_TAIL_SLOT_IMPLEMENTATION.md) | Complete; runtime, converter, Yuan-Ti anatomy/Dexterity follow-up, help, and full validation pass | RoL tail-slot constants, Yuan-Ti eligibility and restricted slots, dual-location rings, wear flow, AC behavior, display, persistence, world-data encoding, compatibility traps, and the LuminariMUD port surface. |
| [ROL_CONVERTER_FILE_ORGANIZATION_SCOPE.md](todo-zusuk/ROL_CONVERTER_FILE_ORGANIZATION_SCOPE.md) | Analysis complete; implementation not started | Split RoL source parsing and target emission by record format behind stable facades, preserve byte-identical output, reorganize tests, and update build and Phase 8 evidence lists. |
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
