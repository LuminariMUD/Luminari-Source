# Campaign Variant Retirement Scope

Status: implementation complete; pending review and merge

Analysis date: 2026-08-19

Supersedes: `CAMPAIGN_VARIANT_REMOVAL_SCOPE.md`

## Implementation progress

- [x] Scope agreed and compiler-led removal approach selected.
- [x] Luminari baseline and persistent identifier manifests captured.
- [x] Retired compile-time definitions and branches removed.
- [x] Retired runtime selection and campaign-routed data removed.
- [x] Build, setup, tests, tooling, and current documentation cleaned.
- [x] Full verification and local smoke test completed.

Last checkpoint: 2026-08-20. Implementation and verification are complete on
`codex/retire-campaign-variants`; only review and merge remain.

### Source-retirement checkpoint

- A temporary poison in `structs.h` turned remaining retired preprocessor references into compile
  errors. It was removed after the repository-wide search became clean.
- The active Luminari branches were retained as ordinary code while the DragonLance and Forgotten
  Realms branches were deleted across 70 source files.
- The runtime campaign field, configuration parser and writer, CEDIT selector, display table, and
  routing macros were removed. Old numeric campaign values are not reused.
- Retired vessel routes, destinations, landmarks, zone entrances, flight helpers, and artifact
  availability metadata were removed with their declarations and tests.
- The resulting source checkpoint removes more than 9,000 lines of retired implementation.
- A clean Autotools production build passed with `-Wall -Wextra` and no warnings.
- The full production-linked suite passed all 778 tests after its source-shape assertions were
  updated for the Luminari-only assignment tables and boot path.
- `make install` passed and removed the root-level `circle` artifact.

### Repository-cleanup checkpoint

- Fresh deployment setup now copies the Luminari configuration without presenting a campaign
  selector. Binary deployment accepts only the development or live Luminari target.
- The world constants tool no longer contains a miniature preprocessor for choosing Luminari from
  retired branches. Its regenerated manifest SHA-256 is
  `48fcde732220fa7c9b1076a78fc1ec94cf95778f474db306f766a294ccc64865`.
- The alternate newbie-equipment implementation and its unused example VNUM catalog were deleted;
  the current equipment path is ordinary code.
- Retired race numbers remain reserved as `LEGACY_RACE_*` identifiers so persisted values are not
  renumbered or silently reused.
- `docs/systems/CAMPAIGN_SYSTEM_ARCHITECTURE.md` was deleted because the advertised system no
  longer exists. Current system, testing, environment, contributor, and generated web documents
  were cleaned without rewriting historical changelogs.
- The world-tool suite passes all 454 remaining tests; the three removed tests existed only to
  exercise retired campaign-branch preprocessing.
- A second clean Autotools build passed with `-Wall -Wextra` and no warnings. The full
  production-linked suite again passed all 778 tests, and `make install` removed the root-level
  `circle` artifact.

### Final-verification checkpoint

- Exact active-tree searches found no retired compile/runtime campaign identifiers, misspellings,
  parser fields, selectors, routing symbols, or artifact availability bits. Protected local
  headers, the two scope notes, and historical changelogs were excluded as required.
- Broad lore review retained only active content, persisted special-procedure names, historical
  inspiration, and explicitly reference-only comparison material.
- A fresh out-of-tree CMake configure and GNU C23 build completed successfully.
- The installed binary completed a full development world boot under `autorun.sh`, reached the
  game loop, and passed the Kohdee account login, world entry, character logout, and account logout
  smoke test in five seconds.
- The root-level `circle` artifact is absent, protected local configuration is unmodified, and
  `docs/CHANGELOG.md` records the retirement.

### Baseline evidence

Baseline commit: `b9dce643` on `codex/retire-campaign-variants`.

- Local `src/campaign.h` defines neither retired campaign. It was read only.
- `lib/.env` identifies this checkout as development.
- No persisted `campaign_setting` was found under `lib/`; `src/db.c` supplies the active default
  value 0.
- A clean Autotools build passed with `-Wall -Wextra` and no warnings.
- The full production-linked CuTest suite passed all 778 tests.
- `make install` passed, installed build ID `24713c3a572cb74cf54ae9b3155734af20d179dc`,
  and removed the root-level `circle` artifact.
- The world-tool suite passed all 457 tests.
- A fresh out-of-tree CMake configure and build passed with GNU C23.
- The checked-in `scripts/world/wtool_constants.json` baseline SHA-256 is
  `d8f020586321868e60a1803e9e24ba952ce0a73e5ff785fb56e7d26875d28d32`.
- Initial source inventory: 494 retired compile-symbol occurrences across 75 files and 64 runtime
  campaign-symbol occurrences across 10 files.

Key active Luminari limits are `NUM_OF_DIRS=10`, `NUM_OF_INGAME_DIRS=6`, `NUM_RACES=28`,
`NUM_CLASSES=38`, `NUM_DEITIES=22`, `NUM_REGIONS=14`, `NUM_LANGUAGES=48`, `NUM_SPELLS=528`,
`NUM_SKILLS=2239`, and `NUM_FEATS=1263`. Baseline tests also observed 982 registered commands,
17 initialized artifacts, and 565 defined perks.

## Decision and objective

LuminariMUD is the only campaign currently shipped and supported by this repository. The
DragonLance and Forgotten Realms variants have moved to their own repositories, so their code,
data, configuration choices, routing, and current documentation should be retired here.

This is a removal project, not a campaign-framework project. It does not need to prove that another
campaign can be added, preserve the current architecture as a supported extension API, or redesign
that architecture for possible future use.

The implementation should make the smallest changes needed to:

- remove the two retired campaign implementations;
- keep current Luminari behavior working;
- stop builds and runtime configuration from selecting either retired campaign; and
- clean up the source, tests, scripts, and current documentation that directly supported them.

Generic campaign machinery is not a target merely because only Luminari remains. It may be left in
place when doing so is harmless and simpler. If campaign support is revisited later, its remaining
machinery can be evaluated, discarded, or redesigned under a separate project.

The authoritative behavior baseline is the current build with neither `CAMPAIGN_DL` nor
`CAMPAIGN_FR` defined and with the runtime `campaign_setting` equal to
`CAMPAIGN_LUMINARI` (numeric value 0).

## Scope boundary

This project retires two implementations. It does not establish a permanent campaign extension
contract.

When removal reaches generic-looking campaign code:

- leave it alone if it still compiles, does not expose a retired choice, and does not route into
  retired behavior;
- make the smallest Luminari-safe adjustment if the removed definitions break it;
- delete it if it becomes clearly unused and deletion is the simplest local fix;
- do not introduce registries, providers, callback tables, plugin interfaces, capability systems,
  or other abstractions for hypothetical future campaigns;
- do not add a sample or synthetic third campaign; and
- do not claim that the remaining machinery is a supported or tested way to add a campaign.

This scope also does not require removing `src/campaign.h`, `src/campaign.example.h`,
`CONFIG_CAMPAIGN`, or generic routing accessors solely because they have one remaining user.
Their treatment should follow the compiler-led cleanup and the smallest-correct-fix rule.

## Required end state

When this project is complete:

- Luminari is the only active campaign in this repository.
- `CAMPAIGN_DL`, `CAMPAIGN_FR`, their misspellings, and equivalent retired variant guards no
  longer exist in active source, scripts, tests, workflows, setup configuration, or current
  documentation.
- DragonLance- and Forgotten Realms-only functions, declarations, tables, commands, messages,
  metadata, routes, setup choices, and test cases are removed when they have no Luminari owner.
- No build or runtime configuration can activate either retired campaign.
- Current Luminari branches execute as ordinary code or through whatever small generic interfaces
  remain after cleanup.
- Persisted Luminari identifiers, active file grammars, and current player/world behavior remain
  compatible.
- Retired numeric campaign values are not silently reused.
- Fresh Autotools and CMake builds work with the documented Luminari configuration.
- Current documentation no longer presents DragonLance or Forgotten Realms as supported choices.
- Historical changelogs remain unchanged.
- No third campaign, future campaign API, or campaign-framework redesign is delivered or implied.

## Current-state evidence

The retired variants are spread across more than simple header definitions.

### Compile-time selection

`src/structs.h` includes the gitignored `src/campaign.h`, which may define
`CAMPAIGN_DL` or `CAMPAIGN_FR`. With neither defined, preprocessor `#else` and negated
branches form the supported Luminari build.

The previous inventory found hundreds of direct campaign macro references across core, character,
combat, communications, crafting, magic, movement, network, object, OLC, quest, vessel, and
wilderness code. It also found live misspelled guards. Counts must be regenerated when
implementation begins.

Simply removing the macro definitions does not expose every use because undefined identifiers are
valid in `#ifdef`, `#if defined(...)`, and negated conditions.

### Runtime selection

The source also defines numeric Luminari, DragonLance, and Forgotten Realms identities.
`src/db.c` parses `campaign_setting`, `src/olc/cedit.c` exposes a three-choice menu, and
routing macros use that saved value.

Removing compile-time definitions alone would therefore leave a second path capable of selecting
retired data or behavior.

### Semantic and infrastructure references

Some retired support does not contain an exact campaign macro:

- vessel transport and routing contain named destination and landmark tables;
- constants expose Forgotten Realms zone entrances;
- artifact metadata contains campaign availability bits;
- setup and deployment scripts offer campaign choices;
- workflows copy the campaign template;
- world tooling deliberately interprets conditional branches; and
- current documentation describes campaign selection and variant behavior.

Compilation will not find all of these. They require a final structural and ownership search.

### Campaign-adjacent feature flags

`src/campaign.example.h` also defines wilderness feature settings. The current default enables
enhanced wilderness crafting, resource depletion, and dynamic descriptions.

Removing or rewriting legacy header content must not silently disable those Luminari features.
Make current Luminari behavior unconditional, move a genuinely independent option to
`mud_options.example.h`, or remove an unused definition after tracing. Never edit the protected
local `src/mud_options.h`.

## Implementation approach

### 1. Capture a known-good baseline

Before pulling definitions:

- confirm the local `src/campaign.h` selects Luminari; read it only and never edit it;
- confirm this is a development checkout using `lib/.env`;
- confirm the development game configuration uses campaign value 0;
- capture clean Autotools, full CuTest, CMake, and world-tool test results;
- capture representative identity, character creation, spell, travel, wilderness, crafting, and
  artifact behavior; and
- record persistent numeric values that must not move.

The baseline is a guard against a successful build that accidentally changes Luminari behavior.

### 2. Pull the retired definitions

Start with the defining headers and central registrations:

- remove the `CAMPAIGN_DL` and `CAMPAIGN_FR` choices and descriptions from
  `campaign.example.h`;
- remove their runtime identity definitions, display-name entries, availability bits, and other
  central registrations;
- keep the current Luminari identity where existing code still needs it;
- add a temporary early guard that rejects a local `campaign.h` selecting a retired campaign; and
- temporarily poison `CAMPAIGN_DL`, `CAMPAIGN_FR`, `CAMPAING_FR`, and
  `CAMPGIN_DL` so remaining preprocessor references become compiler errors.

Do not modify the protected local `src/campaign.h`. If it selects a retired campaign, the
temporary guard should fail clearly and direct the developer to regenerate it from the updated
template.

The poison is a migration tool, not a permanent feature.

### 3. Compile, fix, and repeat

Run the preferred Autotools build immediately. Treat compiler failures as the primary work queue.

For each coherent error cluster:

1. Trace the complete conditional block and relevant callers.
2. Delete the retired campaign branches.
3. Keep the current Luminari branch.
4. Remove declarations, tables, and callers that become unused.
5. Preserve numeric holes where persistence is uncertain.
6. Rebuild.
7. Run the closest available tests.

Use a keep-going build occasionally if a broader error inventory is useful, but keep fixes in
reviewable subsystem batches. Run the full production-linked CuTest suite at major checkpoints.

### 4. Apply simple branch rules

Use the current no-FR/no-DL result as the baseline:

- For a DL-only or FR-only block with no Luminari branch, delete the block and its now-unused
  callers or declarations.
- For a negated DL/FR block, retain its body without the retired guard.
- For a DL/FR/`#else` chain, retain the current `#else` body.
- For a runtime `IS_CAMPAIGN_*` chain, remove the retired cases and preserve the Luminari result.
- For a campaign-specific table, delete retired rows or tables without reordering surviving
  persistent identifiers.
- For a generic wrapper, keep it if callers still use it and it remains useful; otherwise remove it
  as ordinary dead code.
- Do not preserve retired branches under `#if 0`, renamed flags, examples, or fixtures.

Do not add an abstraction merely to avoid choosing between keeping a simple Luminari function and
deleting dead code.

### 5. Remove runtime access to the retired campaigns

Remove DragonLance and Forgotten Realms from:

- runtime campaign constants and names;
- CEDIT choices and parsing;
- configuration validation;
- account, onboarding, spell, interpreter, and protocol routing;
- vessel destinations, travel routing, flights, sailing, carriage, and landmarks; and
- artifact availability or other campaign-indexed metadata.

With only Luminari remaining, a campaign field may be left inert, made read-only, or removed if it
becomes clearly unused. Choose the smallest change that prevents retired values from changing
behavior.

An old configuration containing value 1 or 2 must not activate partial legacy behavior. Handle it
with a clear diagnostic or a documented Luminari fallback. Do not reuse those values.

### 6. Clean semantic leftovers

After compilation is green, search beyond the removed identifiers:

- misspelled macro names;
- `_dl`, `_fr`, `dragonlance`, `forgotten_realms`, and campaign-specific technical names;
- retired destination, landmark, zone entrance, and flight symbols;
- artifact campaign bits;
- scripts, workflows, setup prompts, configuration comments, tests, and current documentation; and
- broad Krynn, Ansalon, Faerun, DragonLance, and Forgotten Realms strings.

Broad lore matches require manual ownership review. Retain text intentionally used by the active
Luminari world. Leave historical changelogs untouched.

### 7. Update setup, tests, and documentation

Keep setup behavior aligned with whatever campaign header/configuration remains after the
compile-and-fix loop:

- fresh setup must create or validate the Luminari configuration;
- deployment must not offer retired choices;
- CI workflows must use the supported default configuration;
- world-tool tests must stop simulating retired branches;
- current tests must assert Luminari behavior without retired fixtures; and
- current documentation must describe only supported behavior.

Revise or retire `docs/systems/CAMPAIGN_SYSTEM_ARCHITECTURE.md` based on what code remains. It
must not advertise a supported multi-campaign system or provide instructions for adding a campaign
unless a future project deliberately designs and validates one.

Update `AGENTS.md`, `CONTRIBUTING.md`, utility templates, developer examples, and the changelog
where relevant. If player-facing help changes, update both the database help row and
`lib/text/help/help.hlp`.

## Source and infrastructure audit

The compiler-led pass and final searches must cover at least:

- core campaign includes, constants, structures, initialization, parsing, identity, and protocols;
- account, onboarding, command, character, combat, movement, magic, quest, crafting, and OLC code;
- boards, shops, treasure, artifacts, special assignments, and world editing;
- vessel routing, transport, flight, sailing, carriage, and landmarks;
- wilderness generation, descriptions, resources, and crafting integration;
- `CMakeLists.txt`, `.gitignore`, and `.github/workflows/*.yml`;
- deployment and setup scripts;
- world constant extraction and tests; and
- current system, guide, testing, deployment, contributor, and utility documentation.

The previous scope's file inventory is useful as a starting search list, but implementation must
rediscover references because the tree may have changed.

## Persistence and compatibility constraints

- Do not compact or renumber surviving race, class, deity, region, language, direction,
  spell/skill, feat, item, room, mobile, object, quest, artifact, or campaign identities.
- Prefer reserved holes over an unproven migration.
- Prove that no active player, account, world, DG script, help row, or database record uses a
  branch-only value before removing or reusing it.
- Preserve the current Luminari file grammars.
- Compare relevant constant manifests before and after removal.
- Do not convert fork data or add compatibility parsing for retired formats.
- Do not modify production configuration or data.

## Work excluded

- Designing, formalizing, testing, or documenting a future campaign framework.
- Implementing a real, sample, or synthetic third campaign.
- Guaranteeing that the remaining campaign machinery is suitable for future reuse.
- Removing all generic campaign machinery merely because only Luminari remains.
- Redesigning compile-time and runtime campaign selection beyond what retirement requires.
- Supporting multiple simultaneous campaigns, runtime world switching, hot reload, or plugins.
- Modifying either fork repository.
- Editing or deleting local `src/campaign.h`, `src/mud_options.h`, or `src/vnums.h`.
- Modifying production code, production configuration, or production data.
- Renumbering persisted identifiers to close gaps.
- Rewriting unrelated game systems.
- Removing setting-specific lore without proving it is retired-variant-only.
- Updating historical changelogs or deliberately stale historical paths.
- Refactoring large files for style or size while touching campaign code.

## Principal risks

### Undefined macros hide work

Deleting `#define` lines alone does not make `#ifdef` code fail. The temporary poison guard and
final searches are required to expose inactive branches.

### Runtime selection survives compile-time cleanup

Removing preprocessor variants does not remove numeric campaign selection. Retired runtime values
must be handled in the same project.

### Luminari behavior is hidden behind negation

Much current behavior is inside negated DL/FR blocks or final `#else` branches. Remove complete
conditional regions and retain the active body; do not delete individual matching lines.

### Numeric table drift

A build can succeed while stored numeric values change meaning. Preserve indexes and reserved
holes, compare manifests, and test representative data.

### Wilderness features disappear with header cleanup

The default campaign template enables active wilderness features. Trace those definitions before
removing their legacy conditional container.

### Cleanup turns into a redesign

The existing campaign system may be awkward, but improving it is not required to retire the two
variants. Prefer deletion and direct Luminari behavior. Defer broader architectural work.

### Compiler misses semantic residue

Unreferenced tables, scripts, docs, configuration strings, and technical names may compile cleanly.
Finish with structural searches and manual ownership classification.

## Recommended implementation sequence

1. Record the Luminari baseline and persistent numeric manifests.
2. Remove the retired header definitions and central runtime registrations.
3. Add the temporary rejection and poison guard.
4. Build, fix one coherent error cluster, run nearby tests, and repeat.
5. Run the full CuTest suite at major green checkpoints.
6. Remove runtime choices and semantic data that compilation does not eliminate.
7. Search scripts, workflows, tests, configuration, current documentation, and technical names.
8. Update setup and documentation to match the remaining Luminari configuration.
9. Remove the temporary poison after all active-tree searches are clean.
10. Run the complete verification set and record the result in the changelog.

Keep every committed batch buildable as the default Luminari product. The working tree may be
temporarily broken between removing a definition and fixing its resulting error cluster.

## Verification plan

### Structural gates

After excluding this working note and its superseded predecessor, active-tree searches must show:

- no `CAMPAIGN_DL`, `CAMPAIGN_FR`, `CAMPAING_FR`, or `CAMPGIN_DL`;
- no `CAMPAIGN_DRAGONLANCE`, `CAMPAIGN_FORGOTTEN_REALMS`, `IS_CAMPAIGN_DL`, or
  `IS_CAMPAIGN_FR`;
- no active parser, writer, menu choice, or routing branch for a retired campaign;
- no fork-only route functions or tables;
- no DragonLance or Forgotten Realms artifact availability bits; and
- no build, setup, CI, test, or current-documentation instruction selecting either campaign.

Historical changelogs may contain the retired names. Broad lore searches require classification,
not automatic deletion.

### Build and automated tests

Run from a development checkout, never production:

```bash
make clean
make -j$(nproc)
make test
make install

python3 -m unittest discover -s scripts/world/tests -t scripts/world -v
```

Also configure and build from a fresh CMake directory using the documented Luminari setup. Confirm
that `make test` is followed by `make install` and that no root-level `circle` artifact
remains.

### Behavior and data checks

- Boot locally with `autorun.sh`.
- Verify Luminari identity and protocol metadata.
- Exercise account login and character creation catalogs.
- Verify directions, tracks, encounters, spells, crafting, wilderness descriptions, travel,
  vessels, landmarks, quests, missions, and artifacts against the baseline.
- Compare constant manifests and active world parser results.
- Load representative development player data to detect numeric reinterpretation.
- Confirm values 1, 2, and unknown campaign values cannot activate retired behavior.
- Confirm CEDIT and saved configuration do not offer DragonLance or Forgotten Realms.

### Setup and documentation checks

- Exercise non-production setup in an isolated temporary checkout.
- Confirm setup does not ask for either retired campaign.
- Validate changed GitHub workflow YAML.
- Search current documentation and examples for obsolete instructions.
- If help changed, verify the database and `lib/text/help/help.hlp` copies match.
- Confirm documentation is ASCII, UTF-8, and LF terminated.

## Acceptance criteria

The project is complete only when:

1. DragonLance and Forgotten Realms compile-time guards and implementations are removed.
2. Their runtime identities, choices, routing, data, and metadata are removed.
3. The current Luminari behavior and persisted identifiers remain intact.
4. No configuration can activate partial retired behavior.
5. Autotools, full CuTest, world-tool tests, a fresh CMake build, and local smoke testing pass.
6. Setup, CI, tests, and current documentation no longer present the retired campaigns.
7. No campaign framework, third campaign, or future extension design was added to the scope.
8. Protected local configuration and production data were not modified.
9. Historical changelogs were not rewritten.
10. `docs/CHANGELOG.md` records the completed retirement, after which this working note can be
    retired according to the ongoing-project documentation policy.
