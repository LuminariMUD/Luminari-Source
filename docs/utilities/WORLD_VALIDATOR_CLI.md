# World Validator and Lookup CLI

`wtool` is the standalone validator and lookup utility for LuminariMUD flat
world data. It parses the same six formats used by the server without starting
the game, connecting to MariaDB, or compiling `circle`:

- zones (`.zon`)
- rooms (`.wld`)
- mobiles (`.mob`)
- objects (`.obj`)
- shops (`.shp`)
- DG Script triggers (`.trg`)

Validation, lookup, flag conversion, and documentation checks are read-only.
The only writing command is the explicit maintainer operation
`constants sync --write`, which replaces the checked-in derived constants
manifest; it never writes world data.

## Requirements and Entry Point

Run commands from the repository root with Python 3.10 or newer:

```sh
python3 scripts/world/wtool.py --help
```

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
2. Format parsers build typed zone, room, mobile, object, shop, and trigger
   records with source spans.
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
unsafe short object extensions, and invalid typed references.

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

The zone selector is the numeric package name, such as `30` for `30.zon` and
`30.wld`. Canonical package files that exist but are absent from an index are
still parsed and reported as unindexed.

Validate only explicitly named files in isolation:

```sh
python3 scripts/world/wtool.py validate --paths \
  /tmp/staged/30.zon /tmp/staged/30.wld
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
| `FLG` | Four-chunk flat-file flag encoding |
| `IDX` | `index` and `index.mini` membership |
| `ZON`, `WLD`, `MOB`, `OBJ`, `SHP`, `TRG` | Format-specific parsing |
| `REF` | Typed cross-file references |
| `SEM` | Semantic and topology checks |
| `DOC` | World-building documentation drift |

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

## Typed Record and Reference Lookup

World record types overlap by design, so every lookup names its type. The CLI
accepts `zone`, `room`, `mob`, `obj`, `shop`, or `trigger`:

```sh
python3 scripts/world/wtool.py show room 3000
python3 scripts/world/wtool.py show obj 3000
python3 scripts/world/wtool.py --json show shop 3000
```

`refs` prints both outgoing and incoming typed edges from the same parsed model
used by validation:

```sh
python3 scripts/world/wtool.py refs room 3000
python3 scripts/world/wtool.py refs obj 3000
```

Examples include room exits and keys, reset loads, shop products and keepers,
trigger attachments, moving-room connections, and item-type-specific vnums.
A missing typed record returns status 1.

## Flags and Constants

List, decode, and encode the serialized flag sets:

```sh
python3 scripts/world/wtool.py flags list room
python3 scripts/world/wtool.py flags decode room 0 j 0 0
python3 scripts/world/wtool.py flags encode obj-extra ITEM_GLOW "Can-Be-Reforged"
```

Encoding always prints all four flat-file tokens. Names accept unambiguous
source macros, OLC display names, or defined aliases.

List bounded source-derived constant tables:

```sh
python3 scripts/world/wtool.py constants list sectors
python3 scripts/world/wtool.py constants list item-types
python3 scripts/world/wtool.py constants list directions
python3 scripts/world/wtool.py constants list trigger-types
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
  and sector reference tables;
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

The validator mirrors loader behavior but does not replace gameplay testing.
It does not execute DG Script bodies, query wilderness data from MariaDB, or
model dynamic references assembled by scripts.

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
contracts. The live `lib/world/{wld,mob,obj,zon,shp,trg}` data is ignored by
Git and is not available to CI, so a green workflow is not a claim that the
complete builder-owned world is clean.
