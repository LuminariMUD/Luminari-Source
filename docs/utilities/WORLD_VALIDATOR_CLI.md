# World Validator and Lookup CLI

`wtool` is the standalone validator and lookup utility for LuminariMUD flat
world data. It parses the same eight formats used by the server without starting
the game, connecting to MariaDB, or compiling `circle`:

- zones (`.zon`)
- rooms (`.wld`)
- mobiles (`.mob`)
- objects (`.obj`)
- shops (`.shp`)
- DG Script triggers (`.trg`)
- quests (`.qst`)
- high-level quests (`.hlq`)

Validation, lookup, flag conversion, and documentation checks are read-only.
The only writing command is the explicit maintainer operation
`constants sync --write`, which replaces the checked-in derived constants
manifest; it never writes world data.

## Requirements and Entry Point

Run commands from the repository root with Python 3.10 or newer:

```sh
python3 scripts/world/wtool.py --help
python3 scripts/world/wtool.py --version
```

The current release reports `wtool 0.2.0`.

The default world root is `lib/world`. Override it for a staging tree or
fixture with the global `--world-root` option. Global options precede the
subcommand when calling Python directly:

```sh
python3 scripts/world/wtool.py \
  --world-root /srv/luminari-staging/lib/world \
  --json \
  validate --all
```

The parser reads `etc/config` beside the selected world root when available.
Use global `--config PATH` to select another file. The only grammar option
currently consumed is `diagonal_dirs`; when no readable configuration exists,
the source default (disabled) is reported as an assumption in the result.

## Architecture and Source of Truth

The entry point is `scripts/world/wtool.py`; implementation modules live under
`scripts/world/wtool_lib/`, and tests and fixtures live under
`scripts/world/tests/`.

The tool follows a staged pipeline:

1. A byte-preserving source reader tracks physical lines, C-style numeric
   parsing, and tilde-string boundaries.
2. Format parsers build typed zone, room, mobile, object, shop, trigger,
   quest, and host-keyed high-level quest records with source spans.
3. Index, reference, semantic, and topology passes operate on those records.
4. Validation, `show`, and `refs` consume the same model, so lookup cannot
   silently disagree with diagnostics.
5. Human and JSON reporters sort findings deterministically and apply the same
   exit policy.

`scripts/world/wtool_constants.json` is a checked-in derived manifest. Its
extractor reads explicit C tables and bounded define blocks instead of broad
name prefixes, which prevents unrelated constants such as mobile vnums from
being mistaken for flags. `constants sync --check` is the drift gate between
the manifest and the current Luminari source branch.

The accepted file grammar comes from the current server loaders and OLC
writers. The validator intentionally diagnoses several cases that the loader
silently drops or rewrites, including omitted index entries, dangling exits,
unsafe short object extensions, invalid typed references, unpersistable
multi-kill quests, ignored HLQ input commands, and unsafe HLQ runtime indexes.

## Validation Modes

Validate the complete normal indexes:

```sh
python3 scripts/world/wtool.py validate --all
```

Validate the mini-mud indexes:

```sh
python3 scripts/world/wtool.py validate --mini
```

Validate one or more zone packages while retaining the indexed world as the
reference graph:

```sh
python3 scripts/world/wtool.py validate --zone 30
python3 scripts/world/wtool.py validate --zone 30 31 32
```

The zone selector is the numeric package name, such as `30` for `30.zon`,
`30.wld`, `30.qst`, and `30.hlq`. Canonical package files that exist but are
absent from an index are still parsed and reported as unindexed.

Validate only explicitly named files in isolation:

```sh
python3 scripts/world/wtool.py validate --paths \
  /tmp/staged/30.zon /tmp/staged/30.wld \
  /tmp/staged/30.qst /tmp/staged/30.hlq
```

`--paths` does not borrow records from the live indexed world. Supply every
file needed for the reference checks you want to perform.

Warnings do not fail a normal validation run. Add `--strict` after the
validation selector when a clean warning budget is required:

```sh
python3 scripts/world/wtool.py validate --zone 30 --strict
```

## Findings and Exit Status

Human output uses compiler-style locations and stable finding codes:

```text
wld/30.wld:42: error REF001: exit destination room 3099 does not exist [room 3001]
```

The code prefix identifies the validation layer:

| Prefix | Area |
|--------|------|
| `SRC` | Physical lines, strings, and numeric tokens |
| `FLG` | Flat-file flag encoding (four chunks normally, one for quest flags) |
| `IDX` | `index` and `index.mini` membership |
| `ZON`, `WLD`, `MOB`, `OBJ`, `SHP`, `TRG`, `QST`, `HLQ` | Format-specific parsing |
| `REF` | Typed cross-file references |
| `SEM` | Semantic and topology checks |
| `DOC` | World-building documentation drift |

Quest-system finding codes added in version 0.2.0 are stable. A code marked
"mixed" has a warning for safe noncanonical input and an error for a related
unsafe form.

| Code | Severity | Contract |
|------|----------|----------|
| `QST001` | Error | Unsafe physical line width. |
| `QST002` | Error | Missing or unterminated tilde string. |
| `QST003` | Error | Tilde string exceeds the loader storage limit. |
| `QST004` | Error | Quest file cannot be read. |
| `QST005` | Error | Missing or invalid `#<quest-vnum>` header. |
| `QST006` | Error | Negative quest VNUM. |
| `QST008` | Error | Invalid seven-field numeric row one or ignored trailing data. |
| `QST009` | Error | Invalid, oversized, or unknown-bit one-token quest flags. |
| `QST010` | Error | Invalid seven-field numeric row two or ignored trailing data. |
| `QST011` | Error | Invalid three- or seven-field reward row. |
| `QST012` | Mixed | Noncanonical `D` marker or invalid four-field dialogue row. |
| `QST013` | Error | Unknown extension marker that can loop the loader. |
| `QST014` | Error | Missing or noncanonical `S` record terminator. |
| `QST015` | Error | Missing `$` file terminator. |
| `QST016` | Error | Repeated `D` block overwrites dialogue state. |
| `QST017` | Mixed | Noncanonical file terminator or ignored content after it. |
| `QST040` | Error | Duplicate quest VNUM. |
| `QST041` | Error | Quest VNUMs are not strictly increasing within a package. |

| Code | Severity | Contract |
|------|----------|----------|
| `HLQ001` | Error | Unsafe physical or comment line width. |
| `HLQ002` | Error | Missing or unterminated tilde string. |
| `HLQ003` | Error | Tilde string exceeds the loader storage limit. |
| `HLQ004` | Error | High-level quest file cannot be read. |
| `HLQ005` | Error | Invalid or lossy `#<host-mobile-vnum>` header. |
| `HLQ006` | Error | Invalid host VNUM or entry before a valid host. |
| `HLQ007` | Error | Unknown top-level marker truncates the file. |
| `HLQ008` | Warning | Approval suffix is not empty or canonical `!`. |
| `HLQ009` | Error | Missing, invalid, or lossy ROOM number. |
| `HLQ010` | Error | Command arity/numeric failure or ignored trailing data. |
| `HLQ011` | Mixed | Multi-character code token or unknown unsafe command code. |
| `HLQ012` | Error | Invalid input/output direction is discarded. |
| `HLQ013` | Error | Missing or noncanonical `S` command-chain terminator. |
| `HLQ014` | Error | Missing `$` file terminator. |
| `HLQ015` | Mixed | Noncanonical file terminator or ignored content after it. |
| `HLQ040` | Error | Duplicate host block. |
| `HLQ041` | Error | Host VNUMs are not strictly increasing within a package. |

| Code | Severity | Contract |
|------|----------|----------|
| `REF032` | Error | A QST typed reference is missing. |
| `REF033` | Error | A QST reference resolves only as the wrong record type. |
| `REF034` | Error | An HLQ typed reference is missing. |
| `REF035` | Error | An HLQ reference resolves only as the wrong record type. |
| `SEM023` | Error | Quest type is outside the source-defined domain. |
| `SEM024` | Warning | Quest string exceeds the QEDIT stored bound. |
| `SEM025` | Error | Quest scalar or level relationship is outside QEDIT bounds. |
| `SEM026` | Error | Required/type-specific quest field, reward, or target is unsafe. |
| `SEM027` | Mixed | Dialogue bounds, applicability, or alternative topology is invalid. |
| `SEM028` | Mixed | Quest chain self-link or reciprocity defect. |
| `SEM029` | Mixed | Previous-link cycle (error) or next-link cycle (warning). |
| `SEM030` | Error | HLQ entry shape, string, room, or chain contract is invalid. |
| `SEM031` | Error | HLQ command type/direction is not consumed safely at runtime. |
| `SEM032` | Mixed | HLQ runtime scalar/reference bound; excess positive coins warn. |
| `SEM033` | Warning | HLQ command persists parameters ignored by the runtime. |

Quest packages also use the shared `IDX` checks and `REF010` canonical-package
check. The detailed field contracts and limits are in
`docs/world_game-data/QUEST_FILE_FORMAT.md` and
`docs/world_game-data/HLQUEST_FILE_FORMAT.md`.

Exit status is part of the command contract:

| Status | Meaning |
|--------|---------|
| 0 | No active errors; warnings are allowed unless `--strict` is set |
| 1 | Data findings failed the requested check, or a lookup was not found |
| 2 | Usage, configuration, filesystem, or other operational failure |

Use the repeatable global `--ignore-code CODE` option for an accepted warning
or info finding. Errors cannot be suppressed. Keep exceptions narrow and
record why they are safe:

```sh
python3 scripts/world/wtool.py \
  --ignore-code SEM010 \
  validate --zone 30 --strict
```

## JSON Output

Place global `--json` before the subcommand for deterministic, versioned JSON:

```sh
python3 scripts/world/wtool.py --json validate --zone 30 > /tmp/zone-30.json
```

Validation output includes `schema_version`, `tool_version`, selected config,
parse completeness, sorted findings, and active/suppressed counts by severity
and code. Standard output contains only the JSON document. Operational errors
go to standard error and return status 2.

Version 0.2.0 keeps JSON schema version 1. Quest and HLQ records, references,
and findings are additive; the payloads for the original six record types are
unchanged.

## Typed Record and Reference Lookup

World record types overlap by design, so every lookup names its type. The CLI
accepts `zone`, `room`, `mob`, `obj`, `shop`, `trigger`, `quest`, or
`hlquest`:

```sh
python3 scripts/world/wtool.py show room 3000
python3 scripts/world/wtool.py show obj 3000
python3 scripts/world/wtool.py --json show shop 3000
python3 scripts/world/wtool.py show quest 3001
python3 scripts/world/wtool.py show hlquest 3000
```

`refs` prints both outgoing and incoming typed edges from the same parsed model
used by validation:

```sh
python3 scripts/world/wtool.py refs room 3000
python3 scripts/world/wtool.py refs obj 3000
python3 scripts/world/wtool.py refs quest 3001
python3 scripts/world/wtool.py --json refs hlquest 3000
```

Examples include room exits and keys, reset loads, shop products and keepers,
trigger attachments, moving-room connections, and item-type-specific vnums.
Quest lookups add questmaster, target, reward, follower, chain, and dialogue
roles. HLQ lookups add host, ROOM, item/load, destination, and door roles.
HLQ `show` prints physical and effective runtime entry/command order and keeps
duplicate host blocks as separate matches. `qst` and `hlq` are accepted short
aliases. A missing typed record returns status 1.

## Flags and Constants

List, decode, and encode the serialized flag sets:

```sh
python3 scripts/world/wtool.py flags list room
python3 scripts/world/wtool.py flags decode room 0 j 0 0
python3 scripts/world/wtool.py flags encode obj-extra ITEM_GLOW "Can-Be-Reforged"
python3 scripts/world/wtool.py flags list quest
python3 scripts/world/wtool.py flags decode quest b
python3 scripts/world/wtool.py flags encode quest AQ_REPEATABLE
```

Encoding prints the format's exact serialized width: four tokens for the
existing world bitvectors and one token for quest flags. Names accept
unambiguous source macros, OLC display names, or defined aliases.

List bounded source-derived constant tables:

```sh
python3 scripts/world/wtool.py constants list sectors
python3 scripts/world/wtool.py constants list item-types
python3 scripts/world/wtool.py constants list directions
python3 scripts/world/wtool.py constants list trigger-types
python3 scripts/world/wtool.py constants list quest-types
python3 scripts/world/wtool.py constants list hlquest-entry-types
python3 scripts/world/wtool.py constants list hlquest-commands
python3 scripts/world/wtool.py constants list mission-difficulties
```

Maintainers verify that the checked-in manifest still matches the selected C
source blocks with:

```sh
python3 scripts/world/wtool.py constants sync --check
```

Run `constants sync --write` only when an intentional source change requires a
manifest update, then review and commit the generated diff.

## Documentation Drift Check

The bounded documentation gate checks the audited world-building set for:

- complete and correctly indexed room, mobile, object-extra, wear, item-type,
  sector, quest-type, quest-flag, HLQ-entry, and HLQ-command reference tables;
- valid explicit paths rooted under `src/` and function citations;
- registered OLC commands in the selected command-reference sections;
- ASCII content, valid UTF-8, and LF line endings; and
- generated builder HTML that matches its Markdown source.

Run it with:

```sh
python3 scripts/world/wtool.py docs --check
```

The generated-guide check delegates to
`scripts/development/generate-web-guides.sh --check` and therefore requires
Pandoc. If the HTML is stale, regenerate it with the same script without
`--check` and commit the Markdown and generated output together.

## Compatibility Wrapper

Existing automation can continue using:

```sh
lib/world/validate-zone.sh 30
lib/world/validate-zone.sh 30 --json --strict
```

The wrapper resolves the repository from its own location, so it works from an
unrelated current directory. It forwards global and validation options and
preserves the `wtool` exit status. It is a compatibility entry point, not a
separate validator.

## Builder Loop

Use the same loop for hand-edited files and OLC changes:

1. Save the zone or copy the candidate files into a staging world.
2. Run `validate --zone N` and fix all errors.
3. Review warnings; use `--strict` when the zone is expected to have none.
4. Use `show` to inspect normalized records and `refs` to trace dependencies.
5. Boot and playtest the validated content in a development environment.

For QEDIT and HLQEDIT, the common focused loop is:

```sh
python3 scripts/world/wtool.py validate --zone N
python3 scripts/world/wtool.py show quest QUEST_VNUM
python3 scripts/world/wtool.py refs quest QUEST_VNUM
python3 scripts/world/wtool.py show hlquest HOST_MOBILE_VNUM
python3 scripts/world/wtool.py refs hlquest HOST_MOBILE_VNUM
```

The validator mirrors loader behavior but does not replace gameplay testing.
It does not execute DG Script bodies, query wilderness data from MariaDB, or
model dynamic references assembled by scripts. It also does not execute quest
dialogue rolls, rewards, combat, spell effects, ROOM triggers, or input/output
command chains. Static validation cannot prove that player-facing text,
difficulty, event sequencing, or intended gameplay is correct.

## Emitter Follow-on Is Not Included

There is no `emit` command in this release. Any future structured emitter must
have its own reviewed plan, write atomically to an explicit staging directory,
and run validation before reporting success. Writing generated content
directly into `lib/world/` would require a separate unmistakable opt-in,
pre-write backups, and additional review; the validator project does not
authorize it.

## Test and CI Gates

Run the complete standalone gate with either build system:

```sh
make test-world-tools
cmake --build build --target test-world-tools
ctest --test-dir build --output-on-failure -R '^world-tool'
```

These gates run the Python suite, constants drift check, documentation check,
and compatibility-wrapper smoke test. They require Python 3.10+ and Pandoc,
but not MariaDB or a `circle` build.

GitHub Actions validates tracked fixtures and the constants/documentation
contracts. The live `lib/world/{wld,mob,obj,zon,shp,trg,qst,hlq}` data is ignored by
Git and is not available to CI, so a green workflow is not a claim that the
complete builder-owned world is clean.
