# src/ Directory Layout - Phase 2

Working notes. Proposes three more feature directories plus a dead-file
cleanup. No functional change is involved at any step.

Phase 1 is complete and recorded in
[SRC_DIRECTORY_LAYOUT.md](SRC_DIRECTORY_LAYOUT.md): eleven directories, one
level deep, 181 files left flat. This phase takes flat from 181 to 133.

## 1) Scope

| Cluster | Files moved |
|---------|------------:|
| `src/character/` | 25 |
| `src/quest/` | 10 |
| `src/comms/` | 10 |
| dead-file cleanup | 3 deleted |

181 - 25 - 10 - 10 - 3 = **133 flat**, and 14 directories.

### 1.1 A recorded disagreement

`quest/` and `comms/` were originally recommended against, and the reason is
kept here so the decision is legible later rather than looking like an
oversight:

- `quest/` reaches the ten-file floor only by including `staff_events.c`,
  which describes itself as a "Staff Ran Event System" - admin tooling rather
  than quest content.
- `comms/` reaches ten only by including `ibt.c`, the ideas/bugs/typos
  reporter - player feedback rather than communication.

Both are included by decision. The counter-argument is reasonable: quests and
staff events are both scheduled world content that players opt into, and IBT
is a player-to-staff message channel that already shares the board storage
model. Neither grouping is wrong, only broader than the strictest reading of
the floor rule. Recording it here costs nothing and prevents a future reader
from "correcting" it.

## 2) `src/character/` - 25 files

Everything defining what a character is and how it progresses.

| Files | Role |
|-------|------|
| `class.c/.h`, `race.c/.h`, `feats.c/.h` | the definitional trio |
| `perks.c/.h` | 23,660 lines, the largest file in the tree |
| `talents.c/.h`, `evolutions.c/.h` | feat-adjacent progression |
| `backgrounds.c/.h`, `deities.c/.h`, `templates.c/.h`, `premadebuilds.c/.h` | build-time choices |
| `character_creation.c/.h`, `character_creation_content.c/.h` | creation flow |
| `study.c` | the level-up interface (no header) |

This clears the membership test that trimmed `combat/`. `class.h` has 59
includers and `feats.h` 50, but those includers are *consumers of character
definitions*, not diverse subsystems performing character logic - unlike
`fight.h`, whose 57 includers spanned movement, OLC, DG scripts, and vessels.
The files themselves each have one job. Same shape as `magic/`, where
`spells.h` had 113 includers and the move was still correct.

It also lands `evolutions.c` beside `feats.c`, which is where the `combat/`
analysis concluded it belongs.

### 2.1 Include rewrite cost

| Header | Includers | | Header | Includers |
|--------|----------:|-|--------|----------:|
| `class.h` | 59 | | `backgrounds.h` | 15 |
| `feats.h` | 50 | | `premadebuilds.h` | 12 |
| `race.h` | 31 | | `deities.h` | 10 |
| `perks.h` | 28 | | `talents.h` | 6 |
| `evolutions.h` | 25 | | `character_creation.h` | 6 |
| | | | `character_creation_content.h` | 5 |
| | | | `templates.h` | 3 |

**72 distinct files** outside the moved set need path-qualified includes.
Files already inside `src/character/` keep bare includes - quote-includes
search the including file's own directory first.

## 3) `src/quest/` - 10 files

`quest.c/.h`, `hlquest.c/.h`, `missions.c/.h`, `hunts.c/.h`,
`staff_events.c/.h`.

Includers: `quest.h` 28, `missions.h` 17, `hlquest.h` 11, `hunts.h` 10,
`staff_events.h` 8. **41 distinct files** outside the set need edits.

`encounters.c` is deliberately not here - it moved to `src/combat/` in phase 1
because it spawns and resolves fights rather than tracking objectives.

## 4) `src/comms/` - 10 files

`mail.c/.h`, `new_mail.c/.h`, `boards.c/.h`, `mysql_boards.c/.h`,
`ibt.c/.h`.

Includers: `mail.h` 11, `boards.h` 11, `ibt.h` 7, `mysql_boards.h` 6,
`new_mail.h` 3. **19 distinct files** outside the set need edits - the
cheapest of the three.

`act.comm.c` and `act.comm.do_spec_comm.c` stay flat. The `act.*` family is
command-dispatch surface and is kept together; phase 1 rejected splitting it.

## 5) Dead-file cleanup

Three files, all verified absent from `Makefile.am` and `CMakeLists.txt` with
zero includers anywhere in `src/`:

| File | Lines | Why |
|------|------:|-----|
| `src/castle.c.tbamud` | 812 | Upstream tbaMUD leftover, never built |
| `src/test_metamagic.c` | 40 | A test file in the production source tree |
| `src/material_types.h` | 187 | Zero includers; includes `resource_system.h` |

Delete as one commit, separate from the moves, so a rename-only diff stays
reviewable as mechanical.

## 6) Reference categories to sweep

The six categories phase 1 established, with what each costs here:

1. **Bare `#include "name.h"`** - the bulk of the work; 132 distinct files
   across the three clusters.
2. **Relative form `../../src/name.h`** - two unit-test files, confirmed:
   - `unittests/CuTest/test_web_onboarding.c` - 7 includes (`backgrounds`,
     `character_creation`, `character_creation_content`, `deities`, `feats`,
     `premadebuilds`, `race`)
   - `unittests/CuTest/test_gameplay_e2e.c` - 2 includes (`missions`, `perks`)
   These compile as part of `cutest_SOURCES`, so missing them breaks
   `make test`.
3. **Path-qualified `systems/` or `world/` form** - none remain; both trees
   were dissolved in phase 1.
4. **Build files** - `Makefile.am` and `CMakeLists.txt` each list 13 character,
   5 quest, and 5 comms sources. Neither globs; both must move together.
5. **Scripts and mirror trees** - verified none reference these files.
6. **`sql/components/*.sql` header comments** - verified none.

Also: 5 current documents reference these paths and will need repointing.
`docs/CHANGELOG.md` and `docs/previous_changelogs/` stay untouched - they
record the tree as it was.

## 7) Sequence

Per cluster, one commit, in ascending order of blast radius so the cheapest
failure comes first: **comms (19) -> quest (41) -> character (72)**.

1. `git mv` the files.
2. Rewrite includes outside the set; leave intra-directory includes bare.
3. Update `Makefile.am` and `CMakeLists.txt`.
4. Delete orphaned `.o` and `.deps` entries for the moved files - `make clean`
   will not remove them, since they are no longer derived from a listed source.
5. `autoreconf -fi && ./configure && make -j$(nproc)`, expect zero warnings.
6. `make test`, expect OK (306 tests).
7. `make install`; confirm no root-level `circle` artifact remains.
8. Run pre-commit; accept clang-format's trailing-comment realignment where a
   longer include path shifted the comment column.
9. Rebuild and re-test after any reformat, then commit.

Then the cleanup commit (section 5), then documentation (section 8), then a
final verification: clean autotools build, clean CMake build, `make test`, the
protocol parser harness, both character-rename targets, and
`scripts/test_pubsub.sh`.

## 8) Documentation to update

- `AGENTS.md` (which `CLAUDE.md` symlinks to) - add the three directories to
  the layout table. Its "Game mechanics" section names `feats.h`, `feats.c`,
  `class.c`, and `race.c` directly; those references need the new paths.
- `SRC_DIRECTORY_LAYOUT.md` - final layout table and flat count.
- The 5 current documents identified in section 6.
- This file's status in the ongoing-projects `README.md`.

## 9) What stays flat, and why that is the finish line

After this phase, no group of 10+ cohesive files remains. The rest is
correctly flat:

- `ai_*` (7), `clan*` (7), `mysql*` (4), `spec*` (5), `treasure`/`traps`/
  `random` (3 each), `roleplay`+`char_descs`+`introduce` (5) - all below the
  floor.
- `db*` (7) - and `db.h` has **209 includers**, the most in the codebase.
  `db.c` is the world loader and belongs at top level beside `comm.c`.
- The genuine core: `comm.c`, `handler.c`, `interpreter.c`, `utils.c/.h`,
  `structs.h`, `constants.c`, `limits.c`, and the `act.*` family.

Phase 3 is not expected. If a future cluster is proposed, it should have to
clear the same two tests: ten or more files, and a name a newcomer would
guess without being told.
