# Empty the top of src/

Completed 2026-09-15 on branch `arch-n-worktree` (base `ad8105421`): every batch landed,
and the final verification and the local CI matrix passed (see [Results](#results-2026-09-15)).
Counts come from `scripts/development/move_top_level_sources.py --dry-run` on the base.

Goal: no `.c`, `.h`, or `.o` file directly under `src/` - tracked, generated, or local.
Every source file lives in one directory directly under `src/`. No behavior change.

## Progress

| # | Batch | State |
|---|-------|-------|
| 0 | `conf.h` to the build root | done |
| 1 | delete `trails.h` | done |
| 2 | `ai/` | done |
| 3 | `clan/` | done |
| 4 | `database/` | done |
| 5 | `player/` | done |
| 6 | `act/` | done |
| 7 | into existing directories | done |
| 8 | `events/` | done |
| 9 | `core/` | done |
| 10 | `config/`, local-header guard, regression guard | done |
| 11 | rules text and remaining prose | done |

`git log --oneline origin/master..arch-n-worktree` lists the batch commits, one each, plus
the guide-regeneration fix after batch 6 and the verification records. What remains is the
owner's: merging the branch, and the one-time header move in every other checkout,
production included, before its next build. When the branch merges, the durable rules
already live in AGENTS.md, CONVENTIONS.md, README_development.md, and the setup guides, so
this file and the move script can be deleted (`docs/ongoing-projects/README.md`).

All batches are pushed. The owner authorized moving this checkout's local headers on
2026-09-15, and the final verification below then ran here. Every other checkout,
production included, runs the one-time move before its next build.

Baseline on `ad8105421`: a clean Autotools build has no warnings, and `make -j16 test`
passes (1,487 cutest tests); the clean build, tests, and install take about a minute.

## Bottom line

- The top of `src/` holds 150 tracked files (75 `.c`, 74 `.h`, and `src/.gitignore`, which
  stays) plus four untracked headers: the generated `conf.h` and the local `campaign.h`,
  `mud_options.h`, and `vnums.h`. The `.o`, `.deps/`, `.dirstamp`, and `stamp-h1` entries
  are build products that follow the sources once the stale copies are removed.
  `src/pubsub/` holds only stale objects from the retired pubsub code; nothing in it is
  tracked.
- 148 files move and one dead header is deleted. Eight new directories (`core/`,
  `events/`, `player/`, `config/`, `clan/`, `database/`, `act/`, `ai/`); existing
  directories take the other 22 files.
- The work is mechanical but wide: 3,558 include lines in 459 files (the `core/` batch
  alone is 2,670 lines in 431 files) and 1,110 `src/<name>` path references in 146 tracked
  files - both build manifests, scripts, tests, CI files, and documentation. The move
  script rewrites both.
- Three parts need design rather than search-and-replace:
  1. Autotools put `src/` on the include path only because `conf.h` was generated there.
  2. Every checkout must move its local configuration headers by hand, and the existing
     "copy the example if missing" paths would otherwise build with defaults.
  3. In-flight branches, including branches that add new top-level files.
- This reverses two recorded decisions: AGENTS.md ("The genuine MUD-server core belongs
  at top level") and the August 2026 layout survey, which kept below-floor groups flat
  under a roughly 8-10 file minimum. The rules text changes with the work.

## Decisions

Adopted 2026-09-15 when implementation began:

1. The directory names and the judgment calls below, as proposed.
2. `conf.h` in the build root.
3. Production moves its local headers immediately before building the release that
   contains the `config/` batch.
4. No branch waits: the six branches named in the survey merged as PRs #174 and #179-#183,
   and no pull request was open.
5. The `config/` batch lands last among the code batches (the plan had it eighth). It is
   the only batch that needs the owner's header move in every checkout, this one included,
   so the other batches land first. The regression guard moves with it.
6. `src/<name>` path references in scripts, tests, CI, and documentation move with their
   batch, so every commit is consistent. Batch 11 keeps the rules text and prose.
7. The regression guard also rejects tracked `.c` and `.h` files directly under `src/`,
   not only manifest entries: 53 of the 74 top-level headers are in no manifest, so a new
   unlisted header would pass a manifest-only check.
8. The local-header guard is permanent instead of being removed after every checkout moves;
   the owner asked that nothing be deferred (2026-09-15).

## Rules after the change

These replace the "Source layout" rules in AGENTS.md and the matching lines in
`docs/development/CONVENTIONS.md` and `docs/development/README_development.md`.

- Every `.c` and `.h` file lives in exactly one directory directly under `src/`. Nothing
  sits at the top of `src/`, and nothing is two levels deep.
- Membership follows the file's primary job (unchanged).
- A header is included by bare name from its own directory and path-qualified from
  everywhere else, for example `#include "core/structs.h"`. The common prefix becomes
  `conf.h`, `core/sysdep.h`, `core/structs.h`, `core/utils.h`.
- Generated headers (`conf.h`, `build_identity.h`) live in the build root and are
  included by bare name. The include roots are exactly the build root and `src/`; no
  per-directory `-I` flags.
- A new directory needs a name a newcomer would guess. The file-count floor is dropped
  because every file now needs a home; `util/`, `misc/`, and similar names stay banned.

## Directory mapping

The mapping is executable in `BATCHES` in `scripts/development/move_top_level_sources.py`;
keep the two in step.

### New directories

- `src/core/` (39): the server kernel and the base layer every directory includes -
  `structs.h`, `sysdep.h`, `bool.h`, `utils.c/.h`, `handler.c/.h`, `interpreter.c/.h`,
  `comm.c/.h`, `db.c/.h`, `persistence.h`, `constants.c/.h`, `screen.h`, `mudlim.h`,
  `limits.c`, `weather.c`, `modify.c/.h`, `lists.c/.h`, `helpers.c/.h`, `random.c`,
  `zmalloc.c/.h`, `bsd-snprintf.c/.h`, `help.c/.h`, `perfmon.c/.h`,
  `copyover_diagnostic.c/.h`, `elf_build_id.c/.h`.
- `src/events/` (39): the systems `docs/systems/MUD_EVENTS.md` describes - scheduler and
  runtime (`game_scheduler.c/.h`, `event_runtime.c/.h`, `event_handle.h`,
  `event_debug.c/.h`), table-driven events (`mud_event.c/.h`, `mud_event_list.c`,
  `mud_event_callback.h`), domain facts (`domain_events.c/.h`,
  `domain_event_runtime.c/.h`, `domain_event_types.c/.h`, `domain_event_world.c/.h`,
  `domain_object_transfer.c/.h`), owners (`affected_owners.c/.h`, `periodic_owners.c/.h`,
  `character_periodic.c/.h`, `point_update_periodic.c/.h`, `active_world.c/.h`), and
  action timing (`actions.c/.h`, `actionqueues.c/.h`, `activity_manager.c/.h`,
  `ready_action.c/.h`).
- `src/player/` (11): accounts, passwords, player files, and admission - `account.c/.h`,
  `password.c/.h`, `players.c`, `player_rename.c/.h`, `pfdefaults.h`, `ban.c/.h`,
  `rank.c`. `character/` keeps in-game character rules.
- `src/config/` (9 tracked, 3 local): `config.c/.h`, `dotenv.c/.h`,
  `campaign.example.h`, `mud_options.example.h`, `vnums.example.h`, `pet_vnums.h`,
  `harvest_vnums.h`, and the local `campaign.h`, `mud_options.h`, `vnums.h`.
- `src/clan/` (9): `clan.c/.h`, `clan_benefits.h`, `clan_economy.c/.h`,
  `clan_services.c/.h`, `clan_transactions.c/.h`. `olc/clan_edit.c` stays in `olc/`.
- `src/database/` (7): the MariaDB layer - `mysql.c/.h`, `db_init.c/.h`,
  `db_init_data.c`, `db_startup_init.c`, `db_admin_commands.c`. The flat-file world
  loader `db.c` is not database code and goes to `core/`.
- `src/act/` (7): `act.h`, `act.comm.c`, `act.comm.do_spec_comm.c`, `act.informative.c`,
  `act.other.c`, `act.social.c`, `act.wizard.c`.
- `src/ai/` (5): `ai_cache.c`, `ai_events.c`, `ai_security.c`, `ai_service.c/.h`.

### Existing directories

- `src/character/` (+11): `bardic_performance.c/.h`, `char_descs.c/.h`, `introduce.c`,
  `roleplay.c/.h`, `rol_feats.c/.h`, `rewards.c/.h`.
- `src/movement/` (+4): `graph.c/.h`, `asciimap.c/.h`.
- `src/net/` (+3): `reactor.c/.h`, `telnet.h`.
- `src/combat/` (+2): `tactical_effects.c/.h`.
- `src/mob/` (+2): `random_names.c/.h`.

### Deleted

- `trails.h`: nothing includes it, neither manifest lists it, and
  `src/movement/movement.c:37` records that it was merged into `movement_tracks.h`.
  `rank.c` is compiled and `do_slug_rank` and `search_key` are used elsewhere, so it moves.

### Judgment calls (adopted as proposed)

- `help.c/.h` in `core/`: help lookup and help sync have no subsystem directory, and two
  files are too few for their own.
- `act/` keeps the remaining `act.*` family together. Alternative: `act.comm.c` and
  `act.social.c` into `comms/`.
- `actions`, `actionqueues`, `activity_manager`, and `ready_action` in `events/`, matching
  the "Casting activities" and "Readied actions" sections of MUD_EVENTS.md.
  `tactical_effects` goes to `combat/`.
- `reactor.c/.h` in `net/`: it is the libevent/select I/O driver, and only `comm.c`
  includes it. `events/` is the alternative.
- `asciimap.c/.h` in `movement/` beside `graph.c/.h` (room maps and room-graph
  pathfinding). Most of the map code draws zone maps, so `wilderness/` fits less well.
- `bardic_performance` in `character/`, as a class feature like `evolutions`, rather than
  `magic/`.
- `limits.c` (per-tick upkeep) and `weather.c` in `core/`; `rewards` in `character/`;
  `ban` in `player/`; `dotenv` in `config/`.

## Move script

`scripts/development/move_top_level_sources.py [--dry-run] [--verbose] (--all | BATCH...)`
stays until this project closes, so any branch can re-run it. For the selected batches it
runs `git mv`; for every file already moved it then:

- rewrites quoted includes in `src/`, `unittests/`, and `util/`. An include resolves as the
  compiler resolves it (the includer's directory, then `src/`); one that no longer
  resolves is resolved again in the layout before the move, so stale spellings on other
  branches are repaired too. The new spelling is bare inside the header's directory and
  qualified from `src/` elsewhere; `../` spellings outside `src/` stay relative. No two
  tracked files under `src/` share a basename, so resolution is unambiguous. Existing
  qualified same-directory includes to headers that do not move stay as they are.
- rewrites `src/<name>` path references in tracked text files, including both manifests.
  It skips the changelogs, `lib/rol-conversion/`, this document, itself,
  `rol_special_reconciliation.py` (its `src/config.h` is the RoL build configuration), and any
  path token containing `EXAMPLE/` (for example `EXAMPLE/RealmsOfLuminari/src/db.c`).
- deletes the moved sources' orphaned `src/<stem>.o`, `src/*-<stem>.o`, and matching
  `src/.deps/*.Po` files (`make clean` does not remove objects of sources the Makefile no
  longer lists), and `src/.dirstamp` and `src/.deps/` once no `.c` file remains at the top.
  Automake regenerates missing `.Po` files, so deleting them before a rebuild is safe.

It never moves the protected local headers; with the `config/` batch in place it treats
them as living in `src/config/`.

## Build and include plumbing

### Generated conf.h moves to the build root (batch 0)

`configure.ac` generated `src/conf.h`, and `CMakeLists.txt` wrote the same file into the
source tree. Automake adds `-I` for the config header's directory, so the generated
Makefile had `DEFAULT_INCLUDES = -I. -I$(top_builddir)/src`, and nothing else put `src/`
on the Autotools include path. Moving the header out of `src/` would silently drop the
source root, so this lands first:

- `configure.ac`: `AC_CONFIG_HEADERS([conf.h])`, beside `build_identity.h`.
- `Makefile.am`: `AM_CPPFLAGS = -I$(top_srcdir)/src`. The bsd-snprintf test target
  already appends `$(AM_CPPFLAGS)`.
- `CMakeLists.txt`: `configure_file(... ${CMAKE_CURRENT_BINARY_DIR}/conf.h)`. The binary
  directory is already first in `luminari_build`'s include directories.
- Consumers that found `conf.h` through `-Isrc` search the build root ahead of it:
  `scripts/events/test_native_event_architecture.sh` and
  `test_demand_driven_architecture.sh` pass `-I<project root>`; `test_shops.py` and the
  spell-map test in `test_rol_transform.py` (it preprocesses `src/magic/spell_parser.c`;
  the survey missed it) pass `CPPFLAGS` and then the root; `unittests/CuTest/Makefile`
  uses `SOURCE_INCLUDES = -I../.. -I../../src`. CTest already passes
  `CPPFLAGS=-I<build dir>` to the event scripts and world tools, ahead of the root.
  `rol_discovery.py` and `rol_special_reconciliation.py` preprocess and scan
  `src/*.c` of the RoL tree under `EXAMPLE/`, not ours, and stay as they are.
- `scripts/ci/check_configure_probes.sh` reads each build's own `conf.h`. Its final
  reconfigure only repaired the `src/conf.h` that CMake overwrote, so it is gone.
- `.gitignore`: `src/conf.h` became `/conf.h`. `util/spelllist_html.c` includes `"conf.h"`.
- Documentation naming `src/conf.h`: `CMAKE_BUILD_GUIDE.md`, `SETUP_AND_BUILD_GUIDE.md`,
  `HELP_SYSTEM.md`, `strict-c23-toolchain.md`, `util/aider/aider_config_template.md`.
- A stale `src/conf.h` shadows the root one for any source still directly under `src/`,
  because the includer's directory is searched first. Delete it and `src/stamp-h1`.

Rejected: generating `src/core/conf.h` avoids touching those consumers, but it adds 405
include rewrites and needs Automake's `nostdinc`. Without that option Automake would add
`-Isrc/core`, letting unqualified core includes compile under Autotools while CMake
rejects them.

### Manifests

The script's path rewrite updates every `src/<name>` token in `Makefile.am`,
`CMakeLists.txt`, and `configure.ac`: the source lists, `bsd_snprintf_fallback_test`, the
`src/constants.c` rebuild rule, the local-header checks, `AC_CONFIG_SRCDIR`, and the
version comments. By hand: the `Makefile.am` object rule
`src/constants.$(OBJEXT) src/cutest-constants.$(OBJEXT)` in the `core/` batch. Run
`python3 scripts/ci/check_build_parity.py` after every batch.

### Regression guard

In the `config/` batch, make `check_build_parity.py` fail on any manifest entry or tracked
`.c`/`.h` file directly under `src/`. A branch that adds a top-level file merges without
conflict and quietly undoes the layout, and recent branches added `password.c/.h`,
`pet_vnums.h`, and `harvest_vnums.h` at the top level.

## Local configuration headers

The protected local headers move from `src/` to `src/config/` with their examples.
`pet_vnums.h` and `harvest_vnums.h` go to the same directory because `vnums.example.h:15`
includes `harvest_vnums.h` by bare name; a local `vnums.h` created from the example keeps
compiling without edits.

The hazard is silent defaults. Every "create from the example if missing" path would look
in the new location, find nothing, and copy the defaults, while the customized header sits
unused at the old path. The build then succeeds with default campaign settings, options,
and VNUMs. Those paths (the script rewrites their `src/<name>` spellings):

- `scripts/deployment/deploy.sh:154-173` and `scripts/deployment/setup.sh:31-41`
- `.github/actions/setup-build/action.yml:46-50` (spells `src/$header.h`; by hand) and
  `scripts/ci/local/run.py:113-115`
- `.github/workflows/test.yml`, `security.yml`, `release.yml`, and
  `scripts/ci/check_clean_archive.sh` (fresh trees; path change only)
- the error text at `CMakeLists.txt:105-111`
- AGENTS.md's fresh-clone copy rule and the snippets in
  `docs/guides/SETUP_AND_BUILD_GUIDE.md`, `docs/guides/TROUBLESHOOTING_AND_MAINTENANCE.md`,
  `docs/development/README_development.md`, and `docs/development/CMAKE_BUILD_GUIDE.md`

Guard, in the same batch: `configure.ac` and `CMakeLists.txt` stop with an error when any
of the three local headers still exists directly under `src/`, printing the move command,
and `deploy.sh` and `setup.sh` stop with the same message before their copy step. Spell the
old paths through a loop variable or brace expansion so a later run of the script does not
rewrite them. The guard stays permanently: it
costs a few lines per file and turns a stray header from old instructions into a clear
error, so there is no removal issue.

Operator step, once per checkout (development, production, and each worktree). The owner
runs it, because agents may not modify these files. This checkout cannot build the
`config/` batch until it runs, so the implementing session stops there and asks:

```bash
mkdir -p src/config && mv -n src/{campaign,mud_options,vnums}.h src/config/
```

Production runs it immediately before building the release that contains the batch.
Before then, check whether production's local headers include any other header by bare
name; the development copies include nothing.

Verifying the batch before the owner's move: this checkout's `make` stops at the guard, so
the batch is built and tested in a `git archive` of the working tree (`git stash create`)
holding copies of the local headers in `src/config/`, copies of `lib/{etc,misc,text,house,
mudmail,world}`, and a symlink to `lib/mysql_config`. The replica runs the Autotools build,
`make test`, `make check-world-docs`, `make test-world-tools`, the protocol and rename
checks, a `ci-gcc` CMake build with `ctest`, and the four guards with a scratch
`src/campaign.h`. Until the headers move, the two world-tool tests that compile sources
(`test_spell_map_targets_registered_luminari_spells` and `ShopConverterTests`) fail in this
checkout.

## Batches

One commit per batch. Each commit builds, passes the tests, and leaves nothing at the top
for what it moved. Leaves go first and `core/` goes last among the tracked batches, so
every earlier commit still finds the core headers by bare name; `config/` follows because
it needs the owner's header move. Counts are dry runs on the base; later batches shift
slightly once earlier ones land.

| # | Batch | Files | Include lines / files | Path references / files |
|---|-------|------:|----------------------:|------------------------:|
| 0 | `conf.h` to the build root | 0 | 1 / 1 | 5 / 5 |
| 1 | delete `trails.h` | 1 | 0 | 0 |
| 2 | `ai/` | 5 | 9 / 9 | 17 / 6 |
| 3 | `clan/` | 9 | 39 / 36 | 11 / 5 |
| 4 | `database/` | 7 | 57 / 52 | 41 / 15 |
| 5 | `player/` | 11 | 23 / 18 | 69 / 21 |
| 6 | `act/` | 7 | 150 / 149 | 79 / 25 |
| 7 | into existing directories | 22 | 138 / 98 | 51 / 14 |
| 8 | `events/` | 39 | 432 / 176 | 108 / 17 |
| 9 | `core/` | 39 | 2,670 / 431 | 575 / 98 |
| 10 | `config/`, local-header guard, regression guard | 9 | 40 / 36 | 159 / 36 |
| 11 | rules text and remaining prose | 0 | 0 | 0 |

The `config/` include count covers the three local headers' includers.

### Per-batch steps

1. `python3 scripts/development/move_top_level_sources.py --dry-run --verbose <batch>`,
   then the same without `--dry-run`. Never use `git clean -X`, which deletes `lib/.env`,
   `lib/mysql_config`, and the local headers.
2. Sweep what the script cannot see: the batch's manual rows in the next section, and
   `git grep` for the batch's bare file names in `scripts/`, `unittests/`, `.github/`, and
   `util/`, which finds sources joined to a directory at run time.
3. Review rewrites in documents about RoL, Duris, and the other `EXAMPLE/` codebases,
   which share file names such as `db.c` and `comm.c`.
4. `python3 scripts/ci/check_build_parity.py`.
5. `make check-world-docs` (not part of `make test`), `make -j$(nproc)` with no warnings,
   `make -j$(nproc) test`, then `make install`. When a batch rewrites
   `docs/world_game-data/ROOM_FLAGS.md`, `MOB_FLAGS.md`, or `OEDIT_GUIDE.md`, run
   `scripts/development/generate-web-guides.sh` so the generated `docs/web/guides/` pages
   match; batch 6 missed this and needed a follow-up commit. Check exit status directly,
   not through a pipe.
6. For batches 0, 9, and 10, also run an out-of-tree CMake build and `ctest`.
7. `pre-commit run --files <changed files>`. `.clang-format` sets `SortIncludes: Never`, so
   include order cannot change; accept trailing-comment realignment on lengthened lines.
8. Update the Progress table, commit, and push the branch. Moved files keep rename
   detection, and `git log --follow` finds their history.

## What names top-level source paths today

The script rewrites every `src/<name>` spelling (with any prefix such as `../../`,
`$project_root/`, or `%s/`). The rows marked manual need a hand edit in the named batch.

| Kind | Sites |
|------|-------|
| Includes | `src/`; `unittests/CuTest/*.c` (`../../src/` form); `util/*.c` (bare through `-I../src`, plus the `../src/` form in `spelllist_html.c`); `src/vessels/transport.c` (`../character_periodic.h`); `unittests/CuTest/Makefile` prerequisites |
| Tests reading sources at run time | `unittests/CuTest/test_copyover_timer.c` (`act.wizard.c`); `test_perfmon_production.c` (`interpreter.c`, `db.c`, `structs.h`); `test_spec_combat_secondary.c`, `test_spec_effective_binding.c`, and `test_spec_registry_validation.c` (`db.c`); `test_spec_command_pulse.c` (`comm.c`, `character_periodic.c`, `handler.c`, `act.wizard.c`) |
| Scripts reading sources | `scripts/events/test_native_event_architecture.sh`; `scripts/events/test_demand_driven_architecture.sh`; `scripts/events/test_pubsub_retirement.sh`; `scripts/autorun/test_autorun_supervision.sh` (`comm.h`); `scripts/character-rename/test_character_rename_static.sh`; `scripts/development/check_local_port_allocations.sh` (`config.c`); `scripts/development/dev_kohdee_login_smoke.sh`; `scripts/world/wtool_lib/constants.py` (`structs.h`, `constants.c`); `scripts/world/wtool_lib/docs_check.py` (`interpreter.c`) |
| Manual | `events/`: the `/src/(game_scheduler\|event_runtime)` filter in `test_native_event_architecture.sh`. `core/`: the `$(OBJEXT)` rule in `Makefile.am`; the `.pre-commit-config.yaml` clang-format exclude `^src/(olc/genolc\.c\|utils\.h)$`; regenerate `scripts/world/wtool_constants.json` with `wtool.py constants sync`; `util/aider/aider_setup.md` (`/add src/*.c` finds nothing once the top is empty). `database/`: check `scripts/help-sync/tests/test_endpoint_integration.py` (`db_init.c`). `act/`, `core/`: check `scripts/development/check-dg-docs.py` (`interpreter.c`, `constants.c`). `config/`: `.github/actions/setup-build/action.yml` (`src/$header.h`), `scripts/ci/local/run.py` (`f'src/{header}.h'`), the `scripts/ci/test_clean_archive.py` fixture (`src/{name}.example.h`), and the guards |
| Path-keyed baseline | `scripts/ci/sql_interpolation_baseline.txt` has 11 top-level keys; `--update` treats a new path as growth and refuses, so the script's textual rename is the right fix. |
| Silent if missed | `.github/workflows/pages.yml` path triggers (`constants.c`, `interpreter.c`); `docs/CODEOWNERS` (7 entries) |
| Comments | version comments in `configure.ac` and `CMakeLists.txt`; `scripts/autorun/autorun.sh`; `scripts/deployment/production_profile.sh`, `test_production_profile.sh`, `verify_hardened_binary.sh`; `sql/components/*.sql` headers |
| Documentation | about 500 references outside the changelogs, including the citations in the files checked by `wtool.py docs --check`; AGENTS.md, CONTRIBUTING.md, README.md, `.agents/skills/burnin/SKILL.md` |

Leave `docs/previous_changelogs/`, `docs/CHANGELOG.md`, and `lib/rol-conversion/` alone.
The last holds about 26,000 `src/` references to Realms of Luminari source, including a
`src/config.h` that is not ours.

Out of scope, noticed during the survey: `util/generate_spell_html.sh` and
`util/generate_spell_html_detailed.py` already read `src/spell_parser.c` and
`src/class.c`, which moved in August.

The server reads no source paths at run time, so production is affected only through the
build.

## Existing checkouts

After pulling the final batch, in each checkout:

- Move the local headers (above).
- Remove stale products at the top of `src/`: `src/*.o`, `src/.deps/`, `src/.dirstamp`,
  `src/stamp-h1`, `src/conf.h`, and `src/pubsub/` (objects only).
- If `make` does not regenerate cleanly, run `autoreconf -fvi && ./configure` once.
  Reconfigure existing CMake build directories.

## In-flight work

- On 2026-09-15 no pull request was open, and the six recently active branches that
  touched top-level files had merged (PRs #174, #179-#183).
- Git carries edits to renamed files through merges and rebases, but include hunks
  conflict. Re-running `move_top_level_sources.py --all` on the branch repairs includes and
  path references that still spell old locations.
- A branch that adds a file at the top of `src/` merges cleanly; the regression guard
  catches it.
- The batches rewrite most of `src/`. Run them while no other session commits there.

## Final verification

- `find src -maxdepth 1 -type f \( -name '*.c' -o -name '*.h' -o -name '*.o' \)` prints
  nothing after every step below has run.
- Clean Autotools build with no new warnings, `make -j$(nproc) test`, `make install`.
- Out-of-tree CMake builds with the `ci-gcc` and `ci-clang` presets, then `ctest`.
- `python3 scripts/ci/check_build_parity.py`, `python3 scripts/world/wtool.py docs --check`,
  `python3 scripts/world/wtool.py constants sync --check`,
  `scripts/ci/check_configure_probes.sh`, `scripts/ci/check_clean_archive.sh`.
- `cd unittests/CuTest && make test-all`.
- `pre-commit run clang-format --all-files`.
- The local CI runner (`scripts/ci/local/run.py`) for every job the diff triggers.
- Guard proof: with a scratch `src/campaign.h` present, configure and CMake both fail with
  the move message.
- `MUD_PORT=4100 ./scripts/autorun/autorun.sh`, then log in, since `comm.c` and `db.c`
  moved.

### Results (2026-09-15)

In this checkout, after the owner authorized the header move:

- The scratch-header guard proof stopped both `./configure` and `cmake` with the move
  command.
- A clean Autotools build had no warnings. `make test` (1,487 cutest tests),
  `make test-world-tools` (542), `make test-protocol` (31), both character-rename checks,
  and `make install` passed, and nothing remained directly under `src/`.
- `check_build_parity.py`, `wtool.py docs --check`, `wtool.py constants sync --check`,
  `check_configure_probes.sh`, and `pre-commit run clang-format --all-files` passed.
- The `ci-gcc` CMake build passed `ctest` 29/29. The `ci-clang` build passed 28/29: only
  `cutest-runner` failed, on this host's `-Wgcc-install-dir-libstdcxx` error (clang-22
  sees both GCC 13 and GCC 16 here). That test and its CuTest inputs are unchanged since
  the base commit.
- Run without the CI runtime, `check_clean_archive.sh` failed its syntax-boot test because
  the archive had no MariaDB configuration. The script needs the runtime from
  `scripts/ci/prepare_test_runtime.sh` or `LUMINARI_TEST_SKIP_SYNTAX_BOOT=1`.
- The dev MUD restarted under `autorun.sh` on the installed build of `d8c4aeb45` (status:
  active matches installed), and `dev_kohdee_login_smoke.sh` logged Kohdee in and out.
- The local CI runner ran all 28 jobs on the pushed head `c2d6fd6b7` in 13 minutes, and
  every job passed with its success marker in the log: both clean-archive jobs with the
  CI runtime, all eight CMake builds including clang-22 (`ctest` 29/29), the unit tests,
  sanitizers, the memory check (0 Valgrind errors), the coverage baseline, both warning
  budgets, the production profiles, the database-migration and world-validation
  integrations, format, hygiene, and gitleaks.

## Documentation (batch 11)

Path references in documentation move with their batches. Batch 11 changes the prose:

- AGENTS.md: the Source layout section and table (also add `spec/`, which the table omits
  today), the protected local-header paths, and the VNUM note.
- `docs/development/CONVENTIONS.md` (Includes and Interfaces, Files and Ownership) and
  `docs/development/README_development.md` (Source Map, Local Configuration).
- `docs/systems/ARCHITECTURE.md` (components table),
  `docs/systems/CORE_SERVER_ARCHITECTURE.md`, and the key-files table in
  `docs/systems/PLAYER_MANAGEMENT_SYSTEM.md`.
- The setup, deployment, and troubleshooting guides that show the local headers.
- When the project closes, move the durable outcome into permanent documentation, delete
  the move script, and delete this file (see `docs/ongoing-projects/README.md`).

## Ablation

- No per-directory `-I` flags and no forwarding headers at the old paths: either would
  recreate the flat namespace and hide missed rewrites.
- No further split of `core/` or `events/` (for example a `server/` directory): no
  requirement calls for it, and each directory has one job.
- No cleanup of existing qualified same-directory includes in files that do not move.
- No `.git-blame-ignore-revs`: renames carry the history, and blame on include lines has
  little value.
- The only deletion is `trails.h`, which is unreferenced and unbuilt.
- Implementation review (2026-09-15): no branch needs merging first; the script's path
  rewrite and product cleanup replace three hand sweeps per batch; the probe check's final
  reconfigure became dead once CMake stopped writing into the source tree.
