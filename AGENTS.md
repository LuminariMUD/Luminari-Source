# AGENTS.md - guidance for AI agents working with code in this repo

LuminariMUD is a text-based MUD server implementing Pathfinder/D&D 3.5 mechanics on the
tbaMUD/CircleMUD foundation, in GNU C23. MySQL/MariaDB is REQUIRED; the server will not run
without it. Build identity: LuminariMUD is the only supported game identity in this repository.

## Critical Rules

### Workflow

- NEVER post Claude-Session links! NEVER attribute AI (Claude or anybody else) in commits or
  anywhere else.
- Always trace code; never assume naming conventions.
- After planning a task and before implementation, read and apply
  [ablation](.agents/skills/ablation/SKILL.md). Briefly record what can be removed or simplified,
  update the plan, then proceed. Keep this check brief for small tasks.
- When adding or removing a source file, update BOTH `Makefile.am` and `CMakeLists.txt`, then run
  `python3 scripts/ci/check_build_parity.py` (CI blocks on drift).

### Running the MUD

- The MUD game port is 4100 only, including local development. Do not start or configure a
  different game port based on historical notes or handoff summaries. Use
  `MUD_PORT=4100 ./scripts/autorun/autorun.sh`. DO NOT HESITATE TO USE THIS PORT FREELY FOR
  DEVELOPMENT AND TESTING!
- On local/dev, run the MUD with `autorun.sh`, not `luminari.service`.
- On local/dev, Ollama, I3, and Discord services are NOT expected to work (unless we are
  specifically working on those features).

### Credentials and production

- `lib/.env` and `lib/mysql_config` contain credentials. Never print or modify credentials. You
  may read the files, but never modify them without permission; edit `lib/.env.example` /
  `lib/mysql_config_example` instead.
- Production connection details may be present in `lib/.env`; they do not authorize remote actions.
- Before local mutation or running the MUD, read only `APP_ENV` from `lib/.env`, without printing
  credentials.
- Do not edit production code or create branches/worktrees in a production checkout. Production
  help-content changes follow the help-sync skill and the user's explicit scope.

### Local configuration headers

- NEVER modify `src/config/campaign.h`, `src/config/mud_options.h`, or `src/config/vnums.h`: they
  are local, customized, gitignored configuration. For a template change, edit the `.example.h`
  templates instead. Copy `.example.h` -> `.h` only on a fresh clone where the real headers do not
  exist yet.
- Configure, CMake, `deploy.sh`, and `setup.sh` stop while a local header is still directly under
  `src/`. The owner moves them once
  (`mkdir -p src/config && mv -n src/{campaign,mud_options,vnums}.h src/config/`); agents do not
  move them.

### Documentation and help files

- When adding or updating features, update documentation and help files where relevant.
- All documentation must be valid ASCII, UTF-8, LF line endings.
- Help files live in `lib/text/help/`. Update them in two places: the database and
  `lib/text/help/help.hlp`.
- Invoking the help-sync skill, or any request to sync help, means one complete autonomous
  `sync --authorize-production` run to verified publication on both endpoints. Never ask for
  confirmation, and never hold help back because production does not yet run the code it
  documents. Only engine-refused deletions, renames, and conflicts stop the run.

### SQL

- Every new SQL file must parse with sqlfluff, the SQL formatter, and new SQL must pass the
  sqlfluff hook.
- NEVER put stored procedures, stored functions, or multi-statement triggers (anything that needs
  a `DELIMITER` block) in `.sql` files: sqlfluff cannot parse `DELIMITER` blocks. Create them from
  C in `src/database/db_init.c`, which already does this (`create_vessel_procedures()`,
  `create_database_procedures()`).
- Never add `.sqlfluffignore` entries, inline `sqlfluff:` comments, or other sqlfluff
  configuration; `scripts/ci/check_sql_format_policy.py` rejects them.

## Build and Run

Autotools is preferred (faster incremental builds); CMake is supported.

| Task | Command |
| -- | -- |
| Build with Autotools (the repo is already configured, so usually just this) | `make -j$(nproc) && make install` |
| Build when configure/Makefile are missing | `autoreconf -fvi && ./configure && make -j$(nproc)` |
| Run the server (binary is `bin/luminari`) | `./bin/luminari -d lib` |
| Full environment setup aka fresh install (deps, MariaDB, world data, build) | `./scripts/deployment/deploy.sh` |
| Debug under gdb | `./scripts/debugging/debug_game.sh` |

Use `make clean` after changing configure/build flags or to repair stale dependencies, not for
routine source or header edits.

## Testing

Two separate test setups exist. There is NO `test_runner` binary - do not look for one.

### 1. Full-integration CuTest suite (root, autotools)

```bash
make -j$(nproc) test
make install
```

- `make test` builds a `cutest` binary that links ALL game sources compiled with
  `-DLUMINARI_CUTEST` plus the test files listed in `cutest_SOURCES` in `Makefile.am`, then runs
  it.
- The root `make test` path may also build `./luminari`. Always follow it with `make install`,
  which installs the current server as `bin/luminari` and removes the root-level binary. Do not
  leave a `luminari` build artifact in the project root.
- To add a test, create `unittests/CuTest/test_*.c` and add it to `cutest_SOURCES` and
  `cutest_test_files` in `Makefile.am` and to `CUTEST_TEST_SOURCES` in `CMakeLists.txt`.
- `unittests/CuTest/AllTests.c` (auto-generated by `make-tests.sh`) and `test_prototypes.h`
  regenerate automatically from functions whose names begin with `Test`.

### 2. Focused protocol parser harness

```bash
cd unittests/CuTest
make protocol-parser
make test-all       # run root production tests, then the protocol harness
make valgrind-protocol
```

- `unittests/CuTest/Makefile` builds the source-linked `protocol_parser_tests` executable.
- Vessel, autopilot, and vehicle behavior belongs in the root production-linked suite; the legacy
  standalone mirror sources have been removed.

### Filtering and replay

- Use `CUTEST_FILTER=<case-sensitive substring> ./cutest` for focused iteration. An empty or unset
  filter runs every test; an unmatched filter fails.
- `make test`, `make test-all`, and CTest ignore an exported filter, so full validation always
  runs every test.
- Tests taking over one second are listed after the summary.
- Every run prints `LUMINARI_TEST_SEED=<n>`; replay a failure with
  `LUMINARI_TEST_SEED=<n> CUTEST_FILTER=<test> ./cutest`. Never reseed a generator from the clock
  in a test.

### Other test entry points

`make test-character-rename-static` and `make test-character-rename-schema` (root), plus one-off
Makefiles in `unittests/` (e.g. `test_clan.c-Makefile`).

## Architecture

### Core flow

| File | Role |
| -- | -- |
| `core/comm.c` | main select()-based game loop, networking, heartbeat scheduling |
| `core/interpreter.c` | command parsing |
| `core/structs.h` | the central data model (`char_data`, `obj_data`, `room_data`, descriptors) |
| `core/utils.h` | the macro layer (`GET_LEVEL()`, `IS_NPC()`, `CREATE()`, `GET_SKILL()`, ...) |
| `core/db.c` | boots the world from flat files in `lib/world/` (`.zon`, `.wld`, `.mob`, `.obj`, `.shp`, `.trg`) into in-memory arrays |
| `database/mysql.c` | MariaDB layer for player/account persistence and many subsystems |
| `core/handler.c` | object/character manipulation primitives (equip, extract, move) |

- **Commands**: all are registered in the `cmd_info[]` table (`src/core/interpreter.c:119`),
  declared with `ACMD_DECL()` in `core/interpreter.h`, and implemented as `ACMD(do_xxx)`, mostly
  in `act/act.*.c` files (`act/act.informative.c`, `act/act.wizard.c`, plus `obj/act.item.c` and
  `combat/act.offensive.c`). There is no act.movement.c - movement commands live in
  `src/movement/`.
- **Includes**: nearly every .c file includes `conf.h`, `core/sysdep.h`, `core/structs.h`,
  `core/utils.h` in that order.

### Game mechanics

- **Spells and skills** share ONE number space: skills are "skill-spells" starting at
  `START_SKILLS` (2000) in `magic/spells.h`. There is no skills.c - spell/skill logic lives in
  `src/magic/`: `spells.c`, `magic.c`, `spell_parser.c` (registration via `spello()` calls in
  `mag_assign_spells()`), and `spell_prep.c` (the preparation system).
- **Feats**: constants in `character/feats.h`, registered via `feato()` calls inside
  `assign_feats()` in `character/feats.c` (populates `feat_list[NUM_FEATS]`), logic wired into the
  relevant system files (`combat/fight.c`, etc.). `character/evolutions.c` is part of this system,
  not combat.
- **Combat**: `src/combat/fight.c`. **Classes**: `character/class.c`. **Races**:
  `character/race.c`.
- **D20 rolls and checks**: look in `core/utils.c`/`act/` - do not assume an ability_check.c
  exists.

### Source layout

Every `.c` and `.h` file lives in exactly one directory directly under `src/`: nothing sits at the
top of `src/` (`scripts/ci/check_build_parity.py` fails on a source file there), and nothing is
nested a second level deep.

| Directory | Holds |
| -- | -- |
| `src/core/` | server kernel and base layer: `structs.h`, `sysdep.h`, `bool.h`, `utils`, `handler`, `interpreter`, `comm`, `db` (flat-file world loader), `constants`, `limits`, `weather`, `modify`, `lists`, `helpers`, `random`, `zmalloc`, `bsd-snprintf`, `help`, `perfmon` (performance monitoring; `core/perfmon.c` is plain C, older docs mentioning perfmon.cpp/C++11 are obsolete), `copyover_diagnostic`, `elf_build_id` |
| `src/events/` | `game_scheduler`, `event_runtime`, `event_debug`, `mud_event*`, `domain_event*`, `domain_object_transfer`, periodic and affect owners, `active_world`, `actions`, `actionqueues`, `activity_manager`, `ready_action` |
| `src/config/` | `config`, `dotenv`, `pet_vnums.h`, `harvest_vnums.h`, the `*.example.h` templates, and the local `campaign.h`, `mud_options.h`, `vnums.h` |
| `src/database/` | MariaDB layer: `mysql`, `db_init*`, `db_startup_init`, `db_admin_commands` |
| `src/player/` | `account`, `password`, `players`, `player_rename`, `pfdefaults.h`, `ban`, `rank` |
| `src/act/` | `act.h` and the `act.comm*`, `act.informative`, `act.other`, `act.social`, `act.wizard` command families |
| `src/ai/` | `ai_service`, `ai_cache`, `ai_events`, `ai_security` |
| `src/clan/` | `clan*` (`olc/clan_edit.c` stays in `olc/`) |
| `src/olc/` | OLC (online creation), in-game world editing that writes the flat world files: `genolc.c`, `gen*.c`, `*edit.c`, the `oasis*` framework, `improved-edit.c` |
| `src/wilderness/` | `resource_*`, `wilderness*`, `perlin`, `kdtree`, `spatial_*`, `region_hints`, `terrain_bridge`, `desc_engine`, `narrative_weaver` |
| `src/vessels/` | `vessels_*`, `vehicles*`, `transport*`, `routing` |
| `src/magic/` | `magic.c`, `spells*`, `spell_parser`, `spell_prep`, `spellbook_scroll`, `casting_visuals`, `metamagic_science`, `domain_powers`, `domains_schools`, `moon_bonus_spells`, `psionics` |
| `src/mob/` | `mob_*`, `random_names` |
| `src/movement/` | `movement*`, `graph` (room-graph pathfinding), `asciimap` |
| `src/dgscript/` | `dg_*` (DG Scripts: trigger-based scripting attached to mobs/objects/rooms; script data lives in `lib/world/trg/`) |
| `src/spec/` | special procedures: `spec_registry`, `spec_dispatch`, `spec_binding`, `spec_assign*`, `spec_zone_*`, `spec_rol_*` |
| `src/character/` | `class`, `race`, `feats`, `perks`, `talents`, `evolutions`, `backgrounds`, `deities`, `templates`, `premadebuilds`, `character_creation*`, `study.c`, `bardic_performance`, `char_descs`, `introduce`, `roleplay`, `rol_feats`, `rewards` |
| `src/combat/` | `fight`, `act.offensive.c`, `assign_wpn_armor`, `encounters`, `spec_abilities`, `grapple`, `combat_modes`, `traps*`, `tactical_effects` |
| `src/quest/` | `quest`, `hlquest`, `missions`, `hunts`, `staff_events` |
| `src/comms/` | `mail`, `new_mail`, `boards`, `mysql_boards`, `ibt` |
| `src/craft/` | `craft*`, `crafting*`, `brew`, `alchemy` |
| `src/net/` | `protocol`, `discord_bridge`, `i3_*` (intermud3), `onboarding`, `reactor` (I/O driver), `telnet.h` |
| `src/obj/` | `act.item.c`, `item.h`, `objsave`, `treasure*`, `spec_artifacts`, `shop`, `trade`, `house` |

- Membership is by "what is this file's primary job", not by what it touches.
- A new directory needs a name a newcomer would guess; catch-alls such as `util/` or `misc/` are
  not allowed.
- A header is included by bare name from its own directory and path-qualified from everywhere else
  (`#include "vessels/vessels.h"`).
- The include roots are exactly the build root, which holds the generated `conf.h` and
  `build_identity.h`, and `src/`. Do not add per-directory `-I` flags to avoid the qualification -
  the explicit path is what makes cross-subsystem coupling visible.
- Historical paths in `docs/previous_changelogs/` are deliberately left stale - they record the
  tree as it was.

## Conventions

### Simplicity

- Write the simplest code that fully solves the task.
- Build only what is needed now: no new abstractions, helpers, options, or layers for needs that
  don't exist yet, and no handling for situations that can't happen.
- Reuse what the codebase already has before adding anything new.
- Simple means easy to read and follow, not short or clever, so keep the error handling and tests
  the task really needs.
- If a fix would stack another patch onto tangled code, straighten out the piece you are touching
  instead.
- Before finishing, reread the change and cut anything the task would not miss.

### Code

- The GNU C23 migration retains the established source style: use `/* */` comments, keep
  declarations at the top of blocks, and do not use variable-length arrays. Do not mechanically
  restyle legacy code.
- 2-space indent, Allman braces, 100-column limit, right-aligned pointers; `.clang-format` is
  provided.
- Treat files over ~1,000 non-generated LOC as a review prompt, not a violation. Exclude comments,
  generated code, tables, URLs, and unavoidable literals from these guidelines.
- `lower_snake_case` functions/variables, `UPPER_SNAKE_CASE` macros/constants, structs named
  `*_data`.
- Safe string functions (`snprintf`, never `sprintf`); NULL-check before dereference;
  `log("SYSERR: ...")` for errors.
- Fix all compiler warnings (`-Wall -Wextra`).
- VNUMs: use the defines in `config/vnums.h`; never hardcode virtual numbers.

### Formatting

- The pre-commit hooks in `.pre-commit-config.yaml` enforce formatting (`pre-commit install` once
  per clone): clang-format (C), ruff (Python), shfmt (shell), sqlfluff (SQL), mdformat (Markdown),
  prettier (YAML, JSON, HTML, CSS, JavaScript), gersemi (CMake), php-cs-fixer (PHP), and
  PSScriptAnalyzer (PowerShell).
- Bulk-format with `pre-commit run <hook-id> --all-files`, never by invoking a formatter directly.
- Committing PHP or PowerShell needs `php` and `pwsh` on `PATH`.
- Do not assume hooks are installed. When installed, the configured pre-commit hook may reformat
  changed source or run checks; inspect any resulting diff and rerun affected checks before
  committing.

## Documentation Map

- `docs/TECHNICAL_DOCUMENTATION_MASTER_INDEX.md` - master index
- `docs/systems/CORE_SERVER_ARCHITECTURE.md`
- `docs/guides/DEVELOPER_GUIDE_AND_API.md`
- `docs/guides/SETUP_AND_BUILD_GUIDE.md`
- `docs/guides/TESTING_GUIDE.md`
- `docs/guides/TROUBLESHOOTING_AND_MAINTENANCE.md`
