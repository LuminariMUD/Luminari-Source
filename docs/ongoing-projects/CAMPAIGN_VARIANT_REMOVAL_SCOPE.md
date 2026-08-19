# Campaign Variant Removal Scope

Status: scoped; implementation not started

Analysis date: 2026-08-19

## Decision and objective

LuminariMUD is now a Luminari-only project. The DragonLance and Forgotten Realms variants have
forked into their own repositories, so this repository should no longer select, compile, configure,
route, document, or test either variant.

The objective is not merely to stop defining `CAMPAIGN_DL` and `CAMPAIGN_FR`. The completed change
must remove the optional-campaign system and make the current default Luminari behavior the only
behavior in this repository.

The authoritative behavior baseline is the current build with neither campaign macro defined and
with the runtime `campaign_setting` equal to `CAMPAIGN_LUMINARI` (numeric value 0).

## Required end state

When this project is complete:

- There is one compile-time product identity: LuminariMUD.
- `CAMPAIGN_DL`, `CAMPAIGN_FR`, their misspellings, and equivalent variant guards no longer exist in
  active source, scripts, tests, configuration, or current documentation.
- The local `src/campaign.h` header is not included or required. The tracked
  `src/campaign.example.h` template and every setup step that creates it are retired.
- The runtime campaign selector is gone. `campaign_setting`, `CONFIG_CAMPAIGN`,
  `IS_CAMPAIGN_*`, `NUM_CAMPAIGN_SETTINGS`, and the campaign selection menu no longer influence
  behavior.
- Current Luminari branches are ordinary unconditional code. A replacement
  `CAMPAIGN_LUMINARI` compile-time flag must not be introduced.
- DragonLance- and Forgotten Realms-only implementations and data tables are removed when they have
  no Luminari owner.
- Persisted Luminari identifiers, file formats, and current player/world behavior remain compatible.
- Fresh Autotools and CMake builds do not need a campaign header.
- Current documentation describes a single Luminari codebase. Historical changelogs remain
  unchanged.

## Current-state evidence

The campaign system has two independent selection layers and several secondary consumers.

### Compile-time selection

`src/structs.h` includes the gitignored `src/campaign.h`, which may define `CAMPAIGN_DL` or
`CAMPAIGN_FR`. With neither defined, preprocessor `#else` and negated branches form the supported
Luminari build.

The tracked `src/` tree currently contains:

- 489 exact occurrences of `CAMPAIGN_DL` or `CAMPAIGN_FR` in 75 files;
- 340 preprocessor condition lines involving those macros;
- 10 direct includes of `campaign.h`; and
- two live `CAMPAING_FR` typo guards plus one commented `CAMPGIN_DL` typo.

An exact-name cleanup alone would therefore miss variant logic.

### Runtime selection

Compile-time selection is not the only campaign mechanism:

- `src/structs.h` defines numeric Luminari, DragonLance, and Forgotten Realms campaign values and
  stores a `campaign` byte in `struct extra_game_data`.
- `src/db.c` defaults that byte to 0, then reads `campaign_setting` from the text game
  configuration.
- `src/olc/cedit.c` writes the value and exposes a three-choice campaign menu.
- `src/vessels/routing.h` converts it into `IS_CAMPAIGN_DL`, `IS_CAMPAIGN_FR`, and
  `IS_CAMPAIGN_LUMINARI`.
- Routing, transport, account, spell, and interpreter code use that runtime value even though
  compile-time selection is separate.

There are currently 92 source occurrences of the runtime campaign symbols. The two selectors can
disagree, which is one reason simply deleting the two compile-time definitions is insufficient.

### Build and setup coupling

Campaign header setup is embedded outside the game source:

- `CMakeLists.txt` fails configuration when `src/campaign.h` is absent.
- Ten GitHub workflow steps copy `campaign.example.h` to `campaign.h`.
- `scripts/deployment/deploy.sh` creates the header and prompts for one of three campaigns.
- `scripts/deployment/setup.sh` creates the header.
- Current setup, development, environment, testing, and troubleshooting documentation tells users
  to create or protect it.

Autotools gets the dependency indirectly through the C include graph rather than an explicit
campaign selection option.

### Campaign-derived feature flags

`src/campaign.example.h` also owns Luminari wilderness feature definitions. The default branch
enables enhanced wilderness crafting, resource depletion, and dynamic descriptions, while the two
fork branches disable or rename parts of that integration.

Deleting the header before resolving these definitions would silently compile out current Luminari
features. The Luminari implementations must become unconditional, or a genuinely independent
feature option must move to the existing MUD options system. No campaign-named replacement header
should be created.

Several definitions in the header, including `ENABLE_WILDERNESS_MATERIALS`,
`WILDERNESS_MATERIAL_THEME`, `RESOURCE_DESCRIPTION_DETAIL_LEVEL`, and
`ECOLOGICAL_NARRATIVE_DEPTH`, currently have no source consumer outside documentation. They should
be removed rather than relocated unless tracing at implementation time finds a new owner.

### Campaign-routed content and metadata

Not all fork support is directly guarded by a compile-time macro:

- `src/vessels/transport.c` unconditionally defines DragonLance and Forgotten Realms destination
  tables because runtime routing can select them.
- `src/vessels/routing.c` contains separate flight and landmark implementations.
- `src/constants.c` and `src/constants.h` expose Forgotten Realms zone entrances.
- `src/obj/spec_artifacts.h` defines a three-campaign availability bitmask, and
  `src/obj/spec_artifacts.c` maps the compiled campaign to that bitmask.
- Race, class, region, deity, language, direction, spell, quest, crafting, and special-procedure
  shapes vary behind preprocessor branches.
- `scripts/world/wtool_lib/constants.py` contains a mini preprocessor that deliberately selects the
  no-FR/no-DL branch while extracting constants.

The removal must trace these semantic dependencies, not stop after a macro search reaches zero.

## Scope rules for flattening branches

The existing no-FR/no-DL result is the baseline. Apply these rules to each conditional only after
tracing the complete block and its callers.

- For `#if CAMPAIGN_DL` or `#if CAMPAIGN_FR` with no Luminari branch, delete the guarded
  implementation and then remove or reserve its now-unused identifiers.
- For `#if !CAMPAIGN_DL && !CAMPAIGN_FR`, retain the body unconditionally.
- For a DL/FR/`#else` chain, retain the current `#else` body unconditionally.
- For an FR/`#else` or DL/`#else` chain, retain the branch selected by the current default build.
- For a runtime `IS_CAMPAIGN_*` chain, call or return the Luminari implementation directly.
- For a campaign availability bitmask, collapse to ordinary Luminari availability or remove the
  dimension if every row is available.
- For a campaign-only public declaration, remove it with all callers, or rename it if the Luminari
  path genuinely owns it.

Do not preserve dead branches under `#if 0`, a renamed macro, a new feature flag, or an unused
runtime value. The purpose is to reduce the maintenance surface, not hide it.

## Work included

### 1. Record a Luminari behavior baseline

Before the first removal batch:

- Confirm the local `src/campaign.h` defines neither fork macro. Read it only; never edit it.
- Confirm the development game configuration uses campaign value 0.
- Capture clean Autotools build and full CuTest results.
- Capture the world-tool test result, especially constant extraction tests.
- Capture representative current outputs for MUD identity, character creation catalogs, deities,
  regions, directions, tracks, spell preparation, flight and vessel destinations, crafting, and
  artifact availability.
- Record persistent numeric values that must not move, including active race, class, deity, region,
  language, spell/skill, direction, item, and artifact identifiers.

This baseline distinguishes behavior preservation from merely obtaining a successful compile.

### 2. Flatten compile-time branches

Process every direct and misspelled campaign guard across the source inventory below.

- Keep the current default Luminari branch.
- Delete branch-only functions, declarations, table rows, commands, messages, and special
  assignments for the forked variants.
- Remove negated guards around Luminari code.
- Reduce nested conditionals before deleting shared declarations; some files use campaign guards
  inside larger feature or platform guards.
- Remove obsolete comments that describe optional campaigns.
- Re-run compilation after each subsystem batch so an accidental dependency is found close to its
  source.

The default branch is not automatically correct merely because it is under `#else`. Trace places
where legacy comments or labels say Faerun, Krynn, or Ansalon despite being in the current default
path, and classify them using the content policy below.

### 3. Remove runtime campaign configuration

Remove the second selector completely:

- Delete the three campaign numeric constants and `NUM_CAMPAIGN_SETTINGS` from `src/structs.h`.
- Remove `extra_game_data.campaign` and `CONFIG_CAMPAIGN`.
- Stop initializing and parsing `campaign_setting` in `src/db.c`.
- Stop writing it in `src/olc/cedit.c`; remove the campaign menu row, selection mode, parser case,
  and `campaigns[]` display table and declaration.
- Remove all `IS_CAMPAIGN_*` macros and direct runtime checks.
- Route callers directly to the Luminari destination, spell, account, and onboarding behavior.

The configuration file is text and unknown tags are already ignored, so an old
`campaign_setting = 0` line does not require a database migration. The implementation should stop
emitting the line. It may ignore an existing line silently; it must never activate a fork path.

### 4. Simplify vessel and travel routing

`src/vessels/routing.c` is largely a campaign multiplexer. Replace multiplexing functions with
direct Luminari accessors where those accessors still provide a useful boundary. Remove wrappers
whose only purpose was choosing among three tables.

Delete the fork-only destination and landmark tables, their declarations, and the DL/FR flight
entry points. Keep current Luminari table ordering, costs, coordinates, VNUM references, and travel
behavior unchanged.

This work includes semantic names in `src/vessels/transport.h`, `src/constants.h`, and callers that
do not contain an exact campaign macro.

### 5. Collapse artifact campaign metadata

All current artifact contract rows use `ART_CAMPAIGN_ALL`. In a one-campaign repository, the
availability dimension adds no information.

- Remove `ART_CAMPAIGN_DL`, `ART_CAMPAIGN_FR`, and compile-time campaign detection.
- Prefer removing the artifact campaign field, availability helper, validation, and unavailable
  display state entirely if all contracts remain Luminari-owned.
- If an availability field is still needed for a non-campaign reason, rename and model that reason
  directly rather than retaining a one-bit campaign mask.
- Update the artifact placement project note if its open work still refers to variant
  availability.

Artifact persistence must be checked before changing `struct artifact_data`. The registry metadata
appears to be rebuilt from contract rows, but that must be verified against save/load code rather
than assumed.

### 6. Retire `campaign.h` and setup support

After source consumers are gone:

- Remove all direct `#include "campaign.h"` lines.
- Delete the tracked `src/campaign.example.h` template.
- Remove the CMake existence check.
- Remove campaign-header copies from all GitHub workflows.
- Remove campaign creation and selection from deployment and setup scripts.
- Remove the deployment summary entry for the header.
- Update setup and contributor instructions so only `mud_options.h`, `vnums.h`, and credential
  templates remain protected local configuration.

Do not modify or delete the local gitignored `src/campaign.h` as part of the implementation. Once
it is no longer included, it is harmless. Keep the `.gitignore` entry as a transition tombstone so
existing developer files do not suddenly appear as untracked changes. Its eventual removal is
optional housekeeping after operators have retired local copies.

### 7. Make Luminari wilderness behavior explicit

Resolve every definition currently supplied by the default block in `campaign.example.h` before
deleting the template.

The preferred result is unconditional compilation of the current Luminari wilderness resource,
description, and crafting integration. This removes campaign-derived feature gates from:

- `src/craft/enhanced_crafting_recipes.h`;
- `src/wilderness/desc_engine.c`;
- `src/wilderness/resource_descriptions.c` and `.h`;
- `src/wilderness/resource_system.c` and `.h`; and
- `src/wilderness/wilderness_crafting_bridge.c` and `.h`.

Only move a setting to `mud_options.example.h` if maintainers want that behavior independently
optional in Luminari. Do not edit the protected local `src/mud_options.h`, and do not use this
project to redesign the wilderness system.

### 8. Simplify world tooling

After Luminari constants no longer sit behind campaign conditionals, remove the campaign-specific
cases from `_conditional_value()` and simplify or remove `_filter_luminari_branch()` in
`scripts/world/wtool_lib/constants.py`.

Update `scripts/world/tests/test_constants.py` so tests describe the remaining extractor contract,
not synthetic FR/DL branch selection. Preserve support for any non-campaign directives that the
selected constant blocks still require.

### 9. Update tests and documentation

Tests must be changed with the behavior they cover:

- Make Luminari-only assertions in `unittests/CuTest/test_web_onboarding.c` unconditional.
- Add or strengthen focused assertions around high-risk flattened tables and parsers.
- Remove test setup that creates `campaign.h`.
- Remove `docs/systems/CAMPAIGN_SYSTEM_ARCHITECTURE.md` when implementation completes.
- Update current build, deployment, environment, convention, movement, protocol, spell,
  wilderness, mount, and testing documents that describe campaign selection or variant behavior.
- Update `AGENTS.md`, `CONTRIBUTING.md`, utility templates, and current developer examples that
  still require the campaign header.
- Record the completed removal in `docs/CHANGELOG.md`.

Historical files in `docs/previous_changelogs/` must remain unchanged. Historical path and feature
descriptions are records of the repository at that time.

No player help entry currently appears to document the campaign selector itself. If removal changes
any player-facing command, race, class, deity, region, spell, crafting, or travel help, update both
the database help row and `lib/text/help/help.hlp` as required by repository policy.

## Direct source inventory

The following 75 files contain exact `CAMPAIGN_DL` or `CAMPAIGN_FR` references. The list is an
initial work queue, not a substitute for call tracing.

### Core and commands

`src/account.c`, `src/act.informative.c`, `src/act.other.c`, `src/act.wizard.c`,
`src/campaign.example.h`, `src/clan.c`, `src/comm.c`, `src/constants.c`, `src/db.c`,
`src/graph.c`, `src/interpreter.c`, `src/interpreter.h`, `src/limits.c`, `src/roleplay.c`,
`src/structs.h`, `src/utils.c`, and `src/utils.h`.

### Character

`src/character/abilities.c`, `src/character/backgrounds.c`,
`src/character/character_creation_content.c`, `src/character/class.c`,
`src/character/deities.c`, `src/character/deities.h`, `src/character/feats.c`,
`src/character/premadebuilds.c`, `src/character/race.c`, and
`src/character/skill_lists.c`.

### Combat and communications

`src/combat/act.offensive.c`, `src/combat/encounters.c`, `src/combat/encounters.h`,
`src/combat/fight.c`, `src/comms/boards.c`, and `src/comms/boards.h`.

### Crafting and magic

`src/craft/craft.c`, `src/craft/craft.h`, `src/craft/crafting_molds.c`,
`src/magic/domain_powers.c`, `src/magic/domains_schools.c`, `src/magic/magic.c`,
`src/magic/spell_parser.c`, `src/magic/spell_prep.c`, `src/magic/spells.c`, and
`src/magic/spells.h`.

### Mobs and movement

`src/mob/mob_spells.c`, `src/mob/mob_spells.h`, `src/movement/movement.c`,
`src/movement/movement_events.c`, and `src/movement/movement_tracks.c`.

### Network and objects

`src/net/discord_bridge.h`, `src/net/protocol.c`, `src/net/protocol.h`,
`src/obj/act.item.c`, `src/obj/shop.c`, `src/obj/spec_artifacts.c`,
`src/obj/spec_artifacts.h`, and `src/obj/treasure.c`.

### OLC, quests, and special assignments

`src/olc/genqst.c`, `src/olc/zedit.c`, `src/quest/hunts.c`, `src/quest/hunts.h`,
`src/quest/missions.c`, `src/quest/quest.c`, `src/spec/spec_assign_mobiles.c`,
`src/spec/spec_assign_objects.c`, and `src/spec/spec_assign_rooms.c`.

### Vessels and wilderness

`src/vessels/routing.c`, `src/vessels/routing.h`, `src/vessels/transport.c`,
`src/wilderness/narrative_weaver.c`, `src/wilderness/region_hints.c`,
`src/wilderness/terrain_bridge.h`, `src/wilderness/wilderness.c`,
`src/wilderness/wilderness.h`, `src/wilderness/wilderness_crafting_bridge.c`, and
`src/wilderness/wilderness_crafting_bridge.h`.

Additional semantic consumers without an exact macro include at least:

- `src/constants.h`;
- `src/olc/cedit.c`;
- `src/vessels/transport.h`;
- wilderness resource and description implementation files guarded by feature macros from
  `campaign.example.h`; and
- `src/vnums.example.h` and `src/mud_options.example.h`, whose campaign-labeled values must be
  traced before comments or definitions are removed.

## Infrastructure and current documentation inventory

The implementation audit must include:

- `CMakeLists.txt`, `.gitignore`, and all `.github/workflows/*.yml` files;
- `scripts/deployment/deploy.sh` and `scripts/deployment/setup.sh`;
- `scripts/world/wtool_lib/constants.py` and `scripts/world/tests/test_constants.py`;
- `AGENTS.md`, `CONTRIBUTING.md`, `util/aider/aider_config_template.md`, and
  `util/claude_code/CLAUDE.example.md`;
- current setup, development, environment, deployment, testing, troubleshooting, known-issue, and
  convention documents; and
- current system documents for campaign architecture, movement, mounts, protocols, spell
  preparation, database integration, dynamic resource descriptions, weather, wilderness crafting,
  vessels, and artifacts.

References should be re-discovered at implementation time. This inventory records current evidence
and will drift if related code changes before the removal begins.

## Persistence and compatibility constraints

### Preserve current numeric identities

Many campaign branches change table shape or expose campaign-specific constants. Player records,
world files, scripts, configuration, and database rows may store numbers rather than names.

- Do not compact or renumber surviving race, class, deity, region, language, direction,
  spell/skill, feat, item, room, mobile, object, quest, or artifact values.
- Prefer reserved holes over an unproven identifier migration.
- If a branch-only identifier is removed, prove that no Luminari player, account, world, DG script,
  help row, or database record uses it.
- Compare relevant constant manifests before and after the change.

### Preserve active file grammars

`src/quest/quest.c` has a DragonLance-only extra input field, and other loaders or writers may have
similar variations. Retain the existing Luminari grammar and validate it against the active world.
Do not convert fork data or make the Luminari loader accept fork-only formats as a compatibility
layer.

### Preserve active behavior, not dead names

The repository contains unguarded Faerun, Krynn, DragonLance, and Ansalon words in lore, races,
treasure strings, comments, utilities, and historical material. A broad text deletion would damage
content that Luminari may intentionally use.

For each unguarded occurrence:

1. Remove it if it exists solely to run or configure one of the forked products.
2. Retain it if current Luminari world content or mechanics intentionally own it.
3. Rename it only when the implementation is Luminari-owned but its technical symbol still falsely
   describes fork routing.
4. Leave historical changelogs untouched.

This content-ownership audit is required, but a wholesale lore purge is not.

## Work excluded

- Modifying either fork repository or synchronizing future changes with it.
- Editing or deleting local `src/campaign.h`, `src/mud_options.h`, or `src/vnums.h`.
- Modifying production code, production configuration, or production data.
- Renumbering persisted identifiers merely to close gaps left by removed branches.
- Rewriting character creation, classes, races, deities, spells, vessels, crafting, wilderness, or
  artifacts beyond what is needed to retain the current Luminari path.
- Removing every setting-specific lore reference without proving it is fork-only.
- Changing active world files or database schemas unless tracing proves a campaign field exists
  there and a migration is necessary.
- Updating historical changelogs or deliberately stale historical paths.
- Refactoring large files for style or size while touching campaign guards.

## Principal risks

### Compile-time and runtime selectors disagree

Flattening only preprocessor branches can leave the saved runtime selector routing Luminari builds
into fork data. Remove both selector layers in the same overall project and do not ship a state in
which runtime values 1 or 2 still change behavior.

### Numeric table drift

Conditional arrays and constants affect indexes, counts, and serialization. A deletion that
compiles may still reinterpret stored player or world numbers. Baseline and compare numeric
manifests; preserve holes where ownership is uncertain.

### Default behavior is hidden behind negation

Much Luminari behavior is inside `!defined(CAMPAIGN_DL) && !defined(CAMPAIGN_FR)` blocks. Blindly
deleting lines containing campaign names would delete the supported implementation. Flatten whole
conditional regions, not individual matching lines.

### Misspelled and semantic residues

Two live guards use `CAMPAING_FR`, and runtime destination tables use `_dl` or `_fr` names without a
compile-time macro. Completion searches must cover misspellings, runtime constants, technical
symbols, and campaign-branded strings, followed by manual classification.

### Wilderness features disappear with the header

The default header enables current Luminari resource and description code. Removing the header
first would produce a successful but feature-reduced build. Make the current Luminari path explicit
before deleting header setup.

### Fresh builds continue depending on obsolete local state

A developer with an existing ignored `campaign.h` can mask a missing cleanup in CI or CMake. Verify
fresh builds in a clean temporary checkout or build context where that file is absent.

### Documentation and help drift

Campaign setup is repeated across many current documents and examples. Removing only the system
architecture page would leave executable instructions that recreate the retired header. Search all
current documentation, scripts, workflows, and utility templates.

## Recommended implementation sequence

1. Record the Luminari baseline, numeric manifests, and clean build/test results.
2. Flatten compile-time branches subsystem by subsystem while `campaign.h` still exists but defines
   neither fork macro.
3. Remove fork-only declarations, data tables, and semantic callers exposed by each flattened
   subsystem; keep persisted numeric holes where necessary.
4. Collapse runtime campaign routing, the saved configuration field, CEDIT selection, vessels, and
   artifact availability to their Luminari behavior.
5. Make the current Luminari wilderness integrations unconditional and remove obsolete feature
   definitions.
6. Remove campaign-header includes, the tracked template, CMake requirement, workflow copies,
   setup/deployment selection, and world-tool branch filtering.
7. Update tests, current documentation, examples, help where applicable, and the changelog.
8. Run structural gates, full builds and tests, a fresh-checkout build, and a local `autorun.sh`
   smoke test.

Keep every intermediate commit buildable as the default Luminari product. Do not attempt to make
the already-forked variants compile at intermediate milestones.

## Verification plan

### Structural gates

After the working scope note is retired or excluded, active-tree searches must show:

- no `CAMPAIGN_DL`, `CAMPAIGN_FR`, `CAMPAING_FR`, or `CAMPGIN_DL` in source, scripts, workflows,
  tests, setup configuration, or current system documentation;
- no `CONFIG_CAMPAIGN`, `IS_CAMPAIGN_*`, `NUM_CAMPAIGN_SETTINGS`,
  `CAMPAIGN_DRAGONLANCE`, or `CAMPAIGN_FORGOTTEN_REALMS`;
- no active parser or writer for `campaign_setting`;
- no source include or build/setup requirement for `campaign.h`;
- no fork-only routing functions or tables such as `start_flight_to_zone_dl`,
  `start_fr_flight_to_zone`, `carriage_locales_dl`, `sailing_locales_dl`,
  `walkto_landmarks_dl`, `walkto_landmarks_fr`, or `zone_entrances_fr`; and
- no `ART_CAMPAIGN_DL` or `ART_CAMPAIGN_FR`.

Historical changelogs and the transitional `.gitignore` tombstone are allowed exceptions. Broad
Faerun, Krynn, DragonLance, and Forgotten Realms searches require classification rather than an
automatic zero-match rule.

### Build and automated tests

Run from a development checkout, never production:

```bash
make clean
make -j$(nproc)
make test
make install

python3 -m unittest discover -s scripts/world/tests -t scripts/world -v
```

Also configure and build from a fresh CMake build directory with no `src/campaign.h`. Confirm that
`make test` is followed by `make install` and that no root-level `circle` artifact remains.

The full CuTest binary is required because CuTest has no per-function filter and campaign branches
cross core initialization, onboarding, combat, magic, quests, vessels, and artifacts.

### Behavior and data checks

- Boot locally with `autorun.sh` and verify no campaign-header or campaign-setting warning.
- Verify MUD name, hostname, website, protocol metadata, Discord port, and terrain API port are the
  current Luminari values.
- Exercise account login and character creation catalogs for race, class, deity, region, and
  background selection.
- Verify active direction counts and movement commands.
- Verify tracks, encounters, spell preparation overrides, crafting initialization, dynamic
  wilderness descriptions, flight destinations, vessel routes, landmark lookups, quests, and
  missions follow the baseline.
- Verify artifact boot validation, availability, chronicle output, and persistence.
- Compare saved constant manifests and active world parser results with the baseline.
- Load a copy of representative development player data to detect race, class, deity, language,
  and region reinterpretation.
- Test CEDIT save output and confirm it no longer writes or displays a campaign selection.

### Setup and documentation checks

- Exercise non-production setup in an isolated temporary checkout and confirm it never asks for a
  campaign or creates `src/campaign.h`.
- Validate all GitHub workflow YAML after removing copy steps.
- Search current documentation and examples for obsolete setup commands.
- If any help changed, verify the database and `lib/text/help/help.hlp` copies match.
- Confirm documentation is ASCII, UTF-8, and LF terminated.

## Acceptance criteria

The project is complete only when all of the following are true:

1. The compile-time FR/DL selection surface and all live typo variants are removed.
2. The runtime campaign setting and all campaign routing are removed.
3. The code always executes the former default Luminari behavior without a replacement campaign
   flag.
4. Fork-only functions, tables, declarations, metadata, commands, and comments have no active
   residue.
5. `src/campaign.example.h` and all build, CI, deployment, setup, and current-documentation
   dependencies on `src/campaign.h` are gone.
6. Current Luminari wilderness features still compile and behave as before.
7. Surviving persistent identifiers and active data grammars match the baseline.
8. Autotools, full CuTest, world-tool tests, and a fresh CMake build pass without a campaign header.
9. A local boot and targeted Luminari smoke test pass.
10. Current system, developer, deployment, testing, example, and help documentation is consistent
    with a Luminari-only repository.
11. Historical changelogs and protected local configuration files were not modified.
12. `docs/CHANGELOG.md` records the completed removal, and this working scope is then retired in
    accordance with the ongoing-project documentation policy.
