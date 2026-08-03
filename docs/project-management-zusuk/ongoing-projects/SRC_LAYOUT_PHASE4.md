# src/ Directory Layout - Phase 4

This document is self-contained. It does not depend on the earlier layout
documents and does not need to be read alongside them.

## 0) The rule this phase is applied against

`src/` uses one flat level of feature directories. A directory must hold
roughly 8-10 or more files and have a name a newcomer would guess without
being told. Membership is by "what is this file's primary job", not by what
it touches or what its filename prefix is. Anything smaller stays flat.

Headers resolve from a namespace rooted at `src/`. A header still in `src/`
is includable by bare name from any depth; a header inside a feature
directory must be path-qualified from outside it (`#include
"vessels/vessels.h"`), while files in that same directory include it bare.

State when this survey was taken: **fifteen** feature directories, **116
tracked `.c`/`.h` files flat** in `src/`. The four deletions in sections 2.4
and 2.5 have since been carried out, so the tree is now at **112 flat**.

## 1) Finding: no sixteenth directory is warranted

Every remaining flat cluster was measured against the two tests. None passes
both. The survey is recorded here so this question does not have to be
re-opened from scratch.

| Candidate | Files | Why it fails |
|-----------|------:|--------------|
| `db/` | 9 or 11 | See 1.1 |
| `ai/` | 7 | See 1.2 |
| `clan/` | 7 | Cohesive and correctly named, but under the floor: `clan.c/.h`, `clan_benefits.h`, `clan_economy.c/.h`, `clan_transactions.c/.h`. Nothing else joins it - `clan.h` at 25 includers is consumed by the rest of the game, not composed of it. |
| `util/` or `core/` | ~13 | `helpers`, `lists`, `zmalloc`, `bsd-snprintf`, `bool.h`, `dotenv`, `perfmon`, `random.c`, `copyover_diagnostic`, `telnet.h`, `screen.h`. Clears the file count and fails the name test outright. This is a dumping ground with no shared job, and it would sit next to a flat `utils.c`/`utils.h` that is not in it. |
| `player/` | 7-10 | Only reaches 10 by pulling in `limits.c`, which is the game tick (`point_update`, `regen_update`, `pulse_luminari`, `proc_d20_round`), and `ban.c/.h`, which is descriptor-level site banning. The genuine storage cluster is `players.c`, `account.c/.h`, `player_rename.c/.h`, `pfdefaults.h`, `rank.c` - 7. |
| `event/` | 9 | `mud_event.c/.h`, `mud_event_list.c`, `actions.c/.h`, `actionqueues.c/.h`, `lists.c/.h`. `mud_event.h` has 71 includers and `actions.h` has 31; this is the scheduler the whole codebase runs on, in the same category as `db.c` and `comm.c`. |
| `act/` | 7 | `act.comm.c`, `act.comm.do_spec_comm.c`, `act.h`, `act.informative.c`, `act.other.c`, `act.social.c`, `act.wizard.c`. Under the floor, and the `act.*` family is the command surface that belongs at top level. |

### 1.1 Why not `db/`

The schema and connection layer is `db_init.c/.h`, `db_init_data.c`,
`db_startup_init.c`, `db_admin_commands.c`, `mysql.c/.h`, and `dotenv.c/.h`.
That is nine files - under the floor.

It only reaches eleven by adding `db.c`/`db.h`, and that is the wrong trade
twice over. `db.h` has **217 includers**, the most of any header in the
codebase, so this would be the single largest include sweep the tree has ever
taken. And `db.c` is the world loader that boots `lib/world/` into memory; it
belongs at top level beside `comm.c` and `handler.c`. A `src/db/` that
excludes `db.c` is more confusing than leaving the group flat, and one that
includes it costs 217 file edits to make the core harder to find.

Rejected.

### 1.2 Why not `ai/`

`ai_cache.c`, `ai_events.c`, `ai_security.c`, `ai_service.c/.h`,
`ai_region_hints.c/.h` is seven files, under the floor.

It reaches eleven only by folding in `narrative_weaver.c/.h` and
`desc_engine.c/.h`, and those two fail the job test for an `ai/` directory.
Their exported surface is wilderness room description, not AI plumbing:

- `desc_engine.h` exports exactly one function, `gen_room_description()`.
- `narrative_weaver.h` exports `create_unified_wilderness_description()`,
  `enhanced_wilderness_description_unified()`, and
  `weave_vessel_wilderness_description()`.

Their includes point the same way - `desc_engine.c` pulls in
`wilderness/wilderness.h`, `wilderness/resource_descriptions.h`, and
`wilderness/region_hints.h`. Filing them under `ai/` would be grouping by
implementation detail. They belong in `src/wilderness/`, which already holds
`region_hints.c/.h` and `resource_descriptions.c/.h` - see section 2.1.

With those two pairs correctly placed, what is left of `ai/` is five files -
`ai_cache.c`, `ai_events.c`, `ai_security.c`, `ai_service.c/.h` - the actual
LLM service plumbing. Correctly flat.

Rejected.

## 2) What this phase actually does

Twelve files are sitting flat that either belong in a directory that already
exists, or are dead and should be removed. That is the whole of the remaining
opportunity, and none of it requires arguing for a new name.

**116 flat -> 104 flat, still fifteen directories.** The four deletions
(sections 2.4 and 2.5) are already done, so the tree is now at **112 flat**
and the work remaining is the eight files that move: 112 -> 104.

### 2.1 Four files -> `src/wilderness/`

`desc_engine.c/.h`, `narrative_weaver.c/.h`

Rationale in 1.2. These are the description-generation half of the wilderness
system, and `wilderness/` already holds the region-hint and
resource-description machinery they call into.

Include cost is small. Externals needing path qualification:

| Header | Includers | External files to edit |
|--------|----------:|------------------------|
| `desc_engine.h` | 5 | `act.informative.c`, `quest/missions.c`, `wilderness/wilderness.c` |
| `narrative_weaver.h` | 3 | `vessels/vessels_narrative.c` |

Four external files total. `wilderness/wilderness.c` moves to a bare include
rather than a qualified one, since it will then share the directory.

Build files: `Makefile.am` lines 77 and 174; `CMakeLists.txt` lines 443 and
615.

### 2.2 Two files -> `src/vessels/`

`routing.c/.h`

`routing.h` is transport destination data end to end -
`get_transport_sailing_name`, `get_transport_carriage_name`,
`get_carriage_locale_vnum`, `get_sailing_locale_x/y`,
`get_walkto_landmark_vnum`, `start_flight_to_zone_dl`. `routing.c` includes
`vessels/transport.h` and `vessels/transport.c` includes `routing.h`. It is
the data half of `transport.c`, separated only by filename.

Eight includers, seven external: `act.informative.c`, `account.c`,
`interpreter.c`, `magic/spells.c`, `movement/movement.c`, `combat/fight.c`,
`vessels/transport.c` (which becomes bare).

Build files: `Makefile.am` line 191; `CMakeLists.txt` line 540.

Note the name collision hazard: `routing.h` closes with `#endif /* _UTILS_H_ */`
on a `_ROUTING_H_` guard. Harmless, but do not let a search-and-replace on
include paths touch it.

### 2.3 Two files -> `src/combat/`

`traps.h`, `traps_new.c`

Traps are room and encounter hazards. `traps_new.c` includes
`combat/fight.h`, `magic/spells.h`, and `character/perks.h`. This move was
already identified as correct when traps were evaluated for and rejected from
`src/obj/`.

`traps.h` has 9 includers, 7 external: `act.other.c`, `interpreter.c`,
`movement/movement.c`, `movement/movement_doors.c`,
`movement/movement_events.c`, `obj/act.item.c`, and
`unittests/CuTest/test_traps_production.c`.

That last one uses the relative form and must be edited by hand:

```
unittests/CuTest/test_traps_production.c:8:  #include "../../src/traps.h"
```

It compiles as part of `cutest_SOURCES`, so missing it breaks `make test`.

Build files: `Makefile.am` line 209; `CMakeLists.txt` line 560.

### 2.4 Three files deleted, not moved (done)

The survey turned up dead source that is in neither build file. Moving it
would have been worse than useless - it would make dead code look maintained.

**`src/traps.c` - superseded duplicate, not compiled.**

Three independent confirmations were taken before deleting:

1. **The binary links without it.** This is the decisive test: if any compiled
   translation unit referenced a symbol defined only in `traps.c`, the link
   would fail with an undefined reference. A full clean build links
   `src/traps_new.o` and no `traps.o`. No `src/traps.o` existed on disk - the
   file had never been compiled.
2. **It was a strict subset.** Every function it defined was also defined in
   `traps_new.c`: `check_trap`, `set_off_trap`, `perform_detecttrap`,
   `is_trap_detected`, `set_trap_detected`, `ACMD(do_disabletrap)`,
   `ACMD(do_detecttrap)`, `EVENTFUNC(event_trap_triggered)`. That is all eight
   of them, against 43 functions in `traps_new.c`, which additionally carries
   the entire modern surface `traps.c` never had (`create_trap`,
   `search_for_traps`, `generate_random_trap`, the disarm and detection DC
   system).
3. **It appeared in neither `Makefile.am` nor `CMakeLists.txt`**, so no build
   edit was needed to delete it.

`traps.h` is unaffected - every declaration in it resolves to `traps_new.c`.
It moves to `combat/` in commit 4 as that file's header.

**`src/ai_region_hints.c` and `src/ai_region_hints.h` - dead, and not
salvageable in place.**

1. In neither build file, and no `.o` ever produced.
2. Nothing includes `ai_region_hints.h` except `ai_region_hints.c` itself.
   Its single advertised entry point,
   `enhance_wilderness_description_with_ai()`, is documented in the header as
   "called from desc_engine.c" - and `desc_engine.c` does not call it.
3. **It does not compile.** Compiling it with the project's own flags
   (`gcc -DHAVE_CONFIG_H -I. -I./src -std=gnu2x -Wall -Wextra -fsyntax-only`)
   produces **8 errors**:
   - Four functions defined twice, in full:
     `enhance_wilderness_description_with_ai`, `free_region_hints`,
     `free_region_profile`, `generate_ai_enhanced_description`.
   - References to struct members that do not exist:
     `weather_info.condition`, `weather_info.month`, `weather_info.hours`.
     `struct weather_data` has none of them.
   - An implicit declaration of `get_region_for_room()`, which exists nowhere
     in the tree.

That third point is the one that settles it. This is not a finished feature
that fell out of the build - it is a draft written against a data model that
either changed underneath it or never existed. Adding it to `Makefile.am`
today would produce a compile failure, not a working AI-hints system. If that
path is wanted later it is a design task, not a rescue of these 23K.

Changes made: deleted `src/traps.c`, `src/ai_region_hints.c`, and
`src/ai_region_hints.h`. No build file edits were required - none of the
three was listed. No include edits were required - nothing referenced them.

Verified: `make clean && make -j$(nproc)` clean with zero warnings and zero
errors under `-Wall -Wextra`; `make test` OK (307 tests, unchanged, including
the six trap cases in `test_traps_production.c` which exercise the surviving
`traps.h` / `traps_new.c` pair); `make install` run with no root-level
`circle` artifact left behind.

### 2.5 `src/gain.c` deleted (done)

`gain.c` was the class/feat/skill advancement menu system -
`class_disp_menu`, `feat_disp_menu`, `skill_disp_menu`, `sorc_disp_menu`,
`bard_disp_menu`, `favored_enemy_menu`, `animal_companion_menu`. That is the
same job as `character/study.c`, which is where the live implementation
lives. `ACMD(do_study)` was defined in both, at `gain.c:167` and
`character/study.c:561`.

It was dead by construction, not by neglect. Five independent confirmations
were taken before deleting:

1. **Disabled in the file itself.** Line 21 was `#undef NEWGAINREADY`,
   immediately before the `#ifdef NEWGAINREADY` on line 22. The code could
   not be enabled even by defining the macro on the command line.
2. **The guard covered everything.** The file contained exactly two
   preprocessor conditionals - `#ifdef` at line 22 and `#endif` at line 1293,
   the last line. Lines 1-21 were a comment block, eleven `#include`s, and
   the `#undef`.
3. **The compiled object was empty.** `nm src/gain.o` returned zero symbols,
   defined and undefined both. Nothing could link against it, and it
   referenced nothing.
4. **`character/study.c` is a strict superset**, at 6145 lines to `gain.c`'s
   1293. `gain.c`'s `sorc_disp_menu` and `bard_disp_menu` correspond to
   `sorc_known_spells_disp_menu` and `bard_known_spells_disp_menu`, and
   `study.c` additionally covers summoner, inquisitor, warlock, and
   psionicist - classes `gain.c` never knew about. The four functions sharing
   a name across both files (`favored_enemy_menu`, `animal_companion_menu`,
   `familiar_menu`, `sorc_study_menu`) have their live copies in `study.c`.
5. **No orphaned declarations.** The file header claimed "Header info in
   oasis.h", but `olc/oasis.h` contains no `gain`- or `disp_menu`-related
   declarations.

No substantive edit had been made since July 2025; every commit touching it
after that was a directory move or a repo-wide formatting pass. The content
remains recoverable from git history.

Changes made: deleted `src/gain.c`; removed `src/gain.c` from `Makefile.am`
(line 100) and `CMakeLists.txt` (line 464); removed the stale `src/gain.o`,
`src/.deps/gain.Po`, and `src/.deps/cutest-gain.Po` artifacts.

Verified: `autoreconf -fi && ./configure && make clean && make -j$(nproc)`
clean with zero warnings and zero errors under `-Wall -Wextra`; `gain.o`
absent from the link line; `make test` OK (307 tests); `make install` run
with no root-level `circle` artifact left behind; separate out-of-tree CMake
configure and build also clean.

## 3) Sequence

Four commits, ascending blast radius, so the cheapest failure comes first. No
file is touched by two commits.

**Commit 1 - remove dead source (0 external files). DONE.** Deleted
`traps.c`, `ai_region_hints.c`, `ai_region_hints.h`. No include edits, no
build file edits - none of the three was listed. Evidence and verification in
section 2.4. The `gain.c` deletion (section 2.5) shipped alongside it.

**Commit 2 - wilderness descriptions (4 external files).** `desc_engine.c/.h`,
`narrative_weaver.c/.h` -> `src/wilderness/`.

**Commit 3 - routing (7 external files).** `routing.c/.h` -> `src/vessels/`.

**Commit 4 - traps (7 external files, one of them a unit test).** `traps.h`,
`traps_new.c` -> `src/combat/`.

Per commit:

1. `git mv` (or `git rm`) only. No content edits in the same step.
2. Rewrite includes outside the moved set; leave intra-directory includes
   bare, and convert same-directory includes that were previously qualified
   down to bare.
3. Update `Makefile.am` and `CMakeLists.txt`.
4. Delete orphaned `.o` and `.deps` entries for the moved files. `make clean`
   will not remove them once they are no longer derived from a listed source.
5. `autoreconf -fi && ./configure && make -j$(nproc)`, expect zero warnings
   under `-Wall -Wextra`.
6. `make test`, expect OK.
7. `make install`; confirm no root-level `circle` artifact remains.
8. Run pre-commit; accept clang-format's trailing-comment realignment where a
   longer include path shifted the comment column.
9. Rebuild and re-test after any reformat, then commit.

Commit 1 and section 2.5 are already done. After commit 4, do the
documentation (section 5), then a final verification: clean autotools build,
clean CMake build, `make test`, the protocol parser harness, both
character-rename targets, and `scripts/test_pubsub.sh`.

## 4) Reference categories to sweep

1. **Bare `#include "name.h"`** - 18 external files across the three move
   commits.
2. **Relative form `../../src/name.h`** - exactly one, confirmed:
   `unittests/CuTest/test_traps_production.c:8`. It is in `cutest_SOURCES`;
   missing it breaks `make test`.
3. **Build files** - `Makefile.am` lines 77, 174, 191, 209 and
   `CMakeLists.txt` lines 443, 540, 560, 615. Neither globs; both must move
   together. `traps.c` and `ai_region_hints.c` appear in neither and need no
   build edit when deleted.
4. **Scripts and mirror trees** - verify before each commit.
5. **`sql/components/*.sql` header comments** - verify before each commit.
6. **Documentation** - section 5.

## 5) Documentation to update

- `AGENTS.md` (which `CLAUDE.md` symlinks to) - the layout table gains
  `desc_engine`/`narrative_weaver` under `src/wilderness/`, `routing` under
  `src/vessels/`, and `traps*` under `src/combat/`. The current sentence
  stating that `traps*.c` stay flat must be removed; it is superseded by 2.3
  and 2.4.
- Any system document referencing `src/traps.c`, `src/routing.c`,
  `src/desc_engine.c`, `src/narrative_weaver.c`, or `src/ai_region_hints.c`
  by path. Sweep `docs/` for these five names before the documentation
  commit.
- `docs/CHANGELOG.md` and `docs/previous_changelogs/` stay untouched - they
  record the tree as it was.

## 6) Risk notes

- **The two deletions in commit 1 are the highest-value and lowest-risk work
  here.** All three files are outside the build. The only way to get this
  wrong is to fail to check that a caller exists; the checks are recorded in
  2.4 and should be re-run rather than trusted.
- `narrative_weaver.c` is 154K and `desc_engine.c` is 42K. Moving them is
  mechanical; splitting them is out of scope and must not be folded into a
  rename commit.
- `routing.h` is included by `combat/fight.c` and `magic/spells.c`, which
  look unrelated to vessels. They are consumers of the campaign-routing
  macros (`IS_CAMPAIGN_LUMINARI` and friends) that `routing.h` also defines.
  Expect the include diff to touch directories that look wrong; it is not.
- `traps.h` is included by `obj/act.item.c` and three `movement/` files.
  After the move those become `combat/traps.h` from a sibling directory -
  path-qualified, not bare.
- `make test` does not cover the wilderness description path. Exercise a
  wilderness room look and a vessel narrative description in-game before
  commit 2 is considered done.

## 7) What stays flat after this

104 files. Remaining prefix groups are all below the floor: `ai_*` (5),
`clan*` (7), `db*` (7), `mysql*` (2), `spec*` (3), `random*` (3), and
`roleplay`+`char_descs`+`introduce` (5).

The genuine core stays flat and should: `comm.c`, `db.c/.h`, `handler.c`,
`interpreter.c`, `utils.c/.h`, `structs.h`, `constants.c`, `limits.c`,
`mud_event.c/.h`, `players.c`, and the `act.*` family.

Phase 5 is not expected. The two tests in section 0 stand for any future
proposal.
