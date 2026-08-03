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

Statuses below were re-verified against the source tree on 2026-08-03.

| Document | Status | What remains |
|----------|--------|--------------|
| [artifacts.md](artifacts.md) | Partial | Sections 1-4 open: deployment packaging (`lib/world/artifacts/` is still git-ignored, so a fresh clone cannot provision), player-facing placement, booted-world integration coverage, and a balance pass. Sections 5-8 are resolved and pruned to their outcomes. System reference: [ARTIFACT_SYSTEM.md](../../systems/ARTIFACT_SYSTEM.md) |
| [ARTIFACT_OBJECT_STATS_FROM_SOURCE_MUDS.md](ARTIFACT_OBJECT_STATS_FROM_SOURCE_MUDS.md) | Reference | Source-verified static object stats, flags, affects, procedures, and Realms artifact overlays for the HomelandMUD and RealmsOfLuminari snapshots. |
| [AI_TODO_IDEAS.md](AI_TODO_IDEAS.md) | Not started | Implementation plan for AI NPC conversation history. No `conversation_history` code exists yet. Its stated foundation still holds, but the dialogue trigger has moved to `src/act.comm.c:526-529` from the `545-548` the document cites. |
| [DO_SKORE_PROJECT.md](DO_SKORE_PROJECT.md) | Partial | Phase 1 and Phases 2.1-2.5 complete; Phase 3 (detailed views) and Phase 4 open. System reference: [SKORE_SYSTEM.md](../../systems/SKORE_SYSTEM.md) |
| [PROTOCOL_TODO.md](PROTOCOL_TODO.md) | Partial | Re-verified against source 2026-08-03 and rewritten. The three "critical RCE" overflows it used to list are fixed, and the guarded `strcat` calls were never a defect. One real memory-safety item remains (unbounded `sprintf` into `MSSPPair[128]`), plus a `sprintf`/`malloc` sweep and three quality items. |
| [MERGE_MUD_EVENTS.md](MERGE_MUD_EVENTS.md) | Not started | Plan to unify the DG event queue and the MUD event layer. Both `src/dgscript/dg_event.c` and `src/mud_event.c` still exist separately. |
| [cbuild-issues.md](cbuild-issues.md) | Partial | CMake build report. Errors are resolved; 76 format warnings remained as of the 2026-01-25 clean build (plus ~8000 clang-tidy findings tracked separately). The count has not been re-measured since and the per-file breakdown predates the phase 2 moves - re-run a clean build before trusting it. |
| [IDEA_LIST.md](IDEA_LIST.md) | Backlog | Player and staff feature suggestions gathered in-game. Not a plan - a source of candidates. |
| [SRC_LAYOUT_PHASE3.md](SRC_LAYOUT_PHASE3.md) | Not started | Plan to add `src/obj/` (14 files: `act.item`, `item.h`, `objsave`, `treasure*`, `spec_artifacts`, `shop`, `trade`, `house`), taking flat `src/` from 134 to 120 across fifteen directories. 49 external files need include rewrites, in two commits (21 then 33). Blocked on the vessel help branch landing. |
| [SRC_LAYOUT_PHASE2.md](SRC_LAYOUT_PHASE2.md) | Complete - retained | Added `character/` (25 files), `quest/` (10), and `comms/` (10), and deleted three dead files. Flat `src/` went from 181 to 134 across fourteen directories. Kept because `SRC_LAYOUT_PHASE3.md` cites it as its baseline; delete both layout records together once phase 3 lands. |
| [SRC_DIRECTORY_LAYOUT.md](SRC_DIRECTORY_LAYOUT.md) | Complete - retained | Replaced the inconsistent `src/systems/` nest with one flat level of nine feature-sized directories. The old `systems/` and `world/` nests are gone; 212 files remain flat. Kept as the phase 3 baseline, same as above. |
| [agent-playthrough.md](agent-playthrough.md) | Reference | Record of a live new-account-to-level-2 production playtest, verified 2026-07-27. An observation log, not a project - useful as a first-hour-experience baseline. |

The temporary vessel workspace is retired. Durable vessel requirements and
the authoritative release-gate state live in
[Vessel System Product Requirements](../../product-requirements/VESSEL_SYSTEM_REQUIREMENTS.md);
behavior and evidence live in the permanent system, testing, deployment, and
changelog documents.
