# Ongoing Projects (Zusuk workspace)

Developer workspace for work that is **not finished**. This is scratch space, not
official documentation - nothing here should be cited as a description of how the
game currently behaves.

When a project here completes, its enduring content belongs in the formal
documentation tree (`docs/systems/`, `docs/testing/`, `docs/guides/`,
`docs/deployment/`), with the outcome recorded in
[docs/CHANGELOG.md](../../CHANGELOG.md). The working notes themselves can then be
deleted.

## Current contents

| Document | Status | What remains |
|----------|--------|--------------|
| [artifacts.md](artifacts.md) | Partial | Deployment, placement, integration coverage, balance, cooldown persistence, validation, group recall, and staff-tool hardening. System reference: [ARTIFACT_SYSTEM.md](../../systems/ARTIFACT_SYSTEM.md) |
| [AI_TODO_IDEAS.md](AI_TODO_IDEAS.md) | Not started | Implementation plan for AI NPC conversation history. No `conversation_history` code exists yet. |
| [DO_SKORE_PROJECT.md](DO_SKORE_PROJECT.md) | Partial | Phase 1 and Phases 2.1-2.5 complete; Phase 3 (detailed views) and Phase 4 open. System reference: [SKORE_SYSTEM.md](../../systems/SKORE_SYSTEM.md) |
| [PROTOCOL_TODO.md](PROTOCOL_TODO.md) | Partial | Security audit follow-ups; 4 of 6 recommendations still outstanding, including comprehensive null-pointer validation. |
| [MERGE_MUD_EVENTS.md](MERGE_MUD_EVENTS.md) | Not started | Plan to unify the DG event queue and the MUD event layer. Both `src/dg_event.c` and `src/mud_event.c` still exist separately. |
| [cbuild-issues.md](cbuild-issues.md) | Partial | CMake build report. Errors are resolved; 76 format warnings remain (plus ~8000 clang-tidy findings tracked separately). |
| [IDEA_LIST.md](IDEA_LIST.md) | Backlog | Player and staff feature suggestions gathered in-game. Not a plan - a source of candidates. |
| [WEB_ACCOUNT_CHARACTER_CREATION_EXPERIENCE.md](WEB_ACCOUNT_CHARACTER_CREATION_EXPERIENCE.md) | Partial | Cross-repository secure web account login, account lobby, and motion-rich server-authoritative character creator. Built through Phase 2 in `src/systems/web_client/` and `luminariweb/src/features/onboarding/`; Phase 3 role-play suite and rollout remain. |
| [manifest.md](manifest.md) | Partial | Media production checklist for the web onboarding experience. Pipeline, audio, scenes, shared art, and fallbacks delivered; race, class, and small-catalog art still in production. |

Vessel work lives in [../vessels/VESSEL_PRD_FINAL.md](../vessels/VESSEL_PRD_FINAL.md),
which carries its own Remaining Work section.
