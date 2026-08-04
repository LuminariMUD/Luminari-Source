# World Validator CLI - Implementation Plan

Status: not started. No code exists yet.

This is the build plan for `wtool`, a standalone world-data validator and
lookup CLI intended primarily for AI agents doing world-building work, and
usable by human builders and by CI.

The guiding decision: **build the oracle before the generator.** Producing
`.wld`/`.mob`/`.zon` text is the easy half of world building. Verifying it is
the hard half, and today there is effectively no verification layer. Every
later convenience (structured authoring, bulk edits, generated zones) is only
safe once a fast and precise correctness check exists.

## 1. Why the current situation needs this

### 1.1 The only existing checker is a stub

`lib/world/validate-zone.sh` is 33 lines. It checks that four files exist,
greps for lines ending in `~`, and otherwise shells out to a full server boot.
It cannot parse a single record.

### 1.2 The boot-time check is a poor verification loop

`bin/circle -c` sets `scheck` (`src/comm.c:360`) and calls `boot_world()`
(`src/comm.c:437`). Three properties make it unsuitable as an authoring loop:

- **It requires MySQL.** `boot_world()` calls `connect_to_mysql()` before any
  world file is read (`src/db.c:665`). A pure text validation cannot be run
  without a live database.
- **It requires a built binary.** Any agent working on world data alone must
  first complete a full C build.
- **It stops at the first error.** `parse_room`, `load_zones`, and
  `setup_dir` all call `exit(1)` on a format error. One run reports one
  problem, so a zone with twelve mistakes takes twelve boot cycles.

### 1.3 Real defect classes the boot does not catch at all

- **Dangling exits.** `renum_world()` (`src/db.c`) maps any `to_room` that
  does not resolve straight to `NOWHERE` with no log line. A typo in an exit
  target is silently swallowed and only found by walking the zone in game.
- **Silent zone-header truncation.** `load_zones` tries `sscanf` for 14, then
  11, then 10, then 4 fields. A header with 12 or 13 fields satisfies the
  11-field scan and the trailing `region`, `faction`, and `city` values are
  discarded without a warning.
- **Flag-name drift.** Builder docs carry hand-copied flag tables. The
  authoritative lists are `room_bits[]` (`src/constants.c:847`),
  `sector_types[]` (`:947`), `action_bits[]` (`:1118`), and `extra_bits[]`
  (`:1939`). Nothing keeps the two in sync.
- **Numeric/letter flag ambiguity.** `asciiflag_conv` (`src/db.c:1947`) treats
  an all-digit token as a decimal number and anything else as letter flags.
  A field of `12` is the number 12, not flags `a|b`. Nothing warns.

By contrast, `renum_zone_table()` *does* log `ZONE ERROR` for reset commands
referencing missing object vnums, so that class is partially covered already.
The validator should match those messages rather than invent new wording.

### 1.4 Constraint: world data is not version controlled

`.gitignore:294` excludes `lib/world/wld/*.wld` and its siblings. Only 43
files under `lib/world/` are tracked, and the world proper is owned by
builders on the live host. Consequences for this project:

- The tool lives in the repository; the data it reads does not.
- The tool must never write to `lib/world/` in Phase 1 through 4. It reads and
  reports only.
- `lib/world/artifacts/1699.{wld,mob,obj,zon}` **is** tracked, and is a small
  real zone package. It is the natural non-synthetic test fixture.

## 2. Scope

**In scope:** parsing and validating `.wld`, `.mob`, `.obj`, `.zon`, `.shp`,
`.trg`, and the `index` / `index.mini` files; flag and constant lookup;
machine-readable output.

**Out of scope for this plan:** any write path. A structured-input emitter
(JSON to flat file) is sketched in Phase 5 as a follow-on and should not be
started until Phases 1 through 3 are in use. Also out of scope: the wilderness
coordinate system, `.qst` and `.hlq` files, and MySQL-backed content such as
help entries. These may be added later behind the same command surface.

## 3. Design decisions

| Decision | Choice | Reason |
|----------|--------|--------|
| Language | Python 3.12, standard library only | Already present; `scripts/world/` already contains Python. No build step, no MySQL, no new C source. Avoids touching `Makefile.am` and `CMakeLists.txt`. |
| Location | `scripts/world/wtool/` package, `scripts/world/wtool.py` entry point | `scripts/world/` is the established home for world tooling. |
| Constants source | Generated from `src/constants.c` into a checked-in JSON | Removes drift by construction. A test fails when the JSON is stale. |
| Fidelity rule | Mirror `db.c` behavior, do not mirror documentation | The parser in `db.c` is the specification. Where docs and code disagree, code wins and the doc gets a bug entry. |
| Output | Human-readable by default, `--json` for agents | Agents need structured findings; humans need `file:line`. |

### 3.1 Command surface

```
wtool validate [--zone N | --all | --paths ...] [--json] [--strict]
wtool flags <room|mob|obj|zone|affect> [--list | --decode VALUE | --encode NAMES]
wtool sectors [--list]
wtool show <room|mob|obj> <vnum>
wtool refs <vnum>              # what references this vnum
wtool sync-constants           # regenerate the constants JSON from src/constants.c
```

Exit codes: `0` clean, `1` findings at error severity, `2` tool failure
(unreadable input, internal error). `--strict` promotes warnings to errors.

Finding severities:

- **error** - the game will refuse to boot, or data is silently corrupted.
- **warning** - boots and runs, but is near-certainly a mistake.
- **info** - style and consistency lint.

## 4. Phases

### Phase 0 - Skeleton and constants extraction

1. Package skeleton, argument parsing, finding model (`severity`, `code`,
   `file`, `line`, `vnum`, `message`), text and JSON reporters.
2. `sync-constants`: parse the named tables out of `src/constants.c` into
   `scripts/world/wtool/constants.json`, recording each table's element count.
   The counts matter because `check_bitvector_names` (`src/db.c:196`) rejects
   bits past the table length.
3. Commit the generated JSON. Add a test asserting a fresh extraction matches
   the committed file, so a `constants.c` edit that outpaces the tool fails
   loudly.

Exit criteria: `wtool flags room --list` prints the same names, in the same
bit order, as `room_bits[]`.

### Phase 1 - Structural parsers

One parser per file type, each written directly against its `db.c`
counterpart. Every parser reports **all** findings in a file rather than
stopping at the first.

**`.wld`** (against `parse_room`, `src/db.c:2016`, and `setup_dir`, `:2510`)

- `#vnum` header, name and description terminated by `~`.
- Flag line field count: 6 fields is the 128-bit form, 3 fields is the legacy
  form that `bitwarning` rejects.
- `D<dir>` blocks: description, keyword, then exactly three integers.
- `E` extra descriptions, `C`, `T <trigger vnum>`, terminating `S`.
- **Room vnum must fall inside its zone's `bot..top`.** `parse_room` exits
  when it does not.
- **Rooms must appear in ascending vnum order.** The zone cursor in
  `parse_room` only advances forward, so a room out of order is fatal.

**`.zon`** (against `load_zones`, `src/db.c:4139`)

- Header numeric line must contain exactly 4, 10, 11, or 14 fields. Any other
  count is either fatal or silently truncating; both are errors.
- `bot <= top`.
- Command letters: `M O P G E R D T V J I L` plus terminating `S`.
- Per-command argument counts, taken from the switch at `src/db.c:4304`:
  `M O E P` take 4 or 5; `G` takes 3 or 4; `D T` take exactly 4; `V` takes 4
  integers plus two strings; `J` takes 2 or 3; `L` takes 3.
- **Reset lines must start at column 0.** The command prescan tests
  `buf[0]` directly, while the parse loop calls `skip_spaces` first. An
  indented reset desyncs the two counts and triggers the fatal
  "Zone command count mismatch".
- **Note for the implementer:** `R` and `I` are absent from the dispatch
  string `"MOGEPDTVJL"` at `src/db.c:4304`, so both fall through to the
  generic 3-argument branch and their explicit `case` labels below are
  unreachable. This means a 2-argument `I` line fails to parse despite the
  `case 'I'` code expecting two arguments. Mirror the real behavior, and file
  the discrepancy in [known-issues.md](../known-issues.md) rather than papering
  over it in the validator.

**`.mob`** (against `parse_mobile` `:3385`, `parse_simple_mob` `:2785`,
`parse_enhanced_mob` `:3361`) - simple `S` versus enhanced `E` forms, flag
fields via `asciiflag_conv` and `asciiflag_conv_aff`, position and level
ranges.

**`.obj`** (against `parse_object` `:3575`) - type, the four flag fields, the
value vector, wear and extra flags, `A` affect lines within the affect slot
limit.

**`.shp`**, **`.trg`** - header, record, and terminator structure.

Exit criteria: parsing all 579 tracked-format zones on the live host produces
zero **error** findings on a world that currently boots, and reproduces every
error on a set of deliberately broken fixtures.

### Phase 2 - Cross-file referential integrity

This is the phase that pays for the project.

- Exit `to_room` targets resolve to a room that exists. Recall that
  `renum_world` silently nulls these.
- Exit `key` values other than `-1` resolve to an object of type `ITEM_KEY`.
- Zone resets: `M` mob exists; `O` object and room exist; `G` and `E` are
  preceded by a loading `M`; `P` target is an existing container object;
  `E` wear position is in range; `D` names a real room and direction.
- `if_flag` chains: a command with `if_flag = 1` that has no preceding
  command it can depend on.
- Trigger attachments: every `T` vnum exists in `.trg`, and the trigger's
  attach type matches its host (mob trigger on a mob, and so on).
- Shops: keeper mob exists, shop rooms exist, every product exists.
- Index membership in both directions: every `N.wld` present on disk is listed
  in `lib/world/wld/index`, and every listed file exists. Same for the other
  five types and for `index.mini`.
- Global vnum uniqueness per type, and zone range overlap between zones.

Exit criteria: `wtool validate --all` completes over the full live world in
under 10 seconds and reports the dangling-exit set, which today is unknown.

### Phase 3 - Semantic and balance lint

Warning and info severity only.

- Flag bits set beyond the length of their constant table, matching
  `check_bitvector_names`.
- `sector_type` outside the range of `sector_types[]`.
- All-digit flag fields in 128-bit files, flagged as ambiguous per
  `asciiflag_conv`.
- Rooms with no exits; one-way exits; rooms unreachable from the zone's
  entrance.
- Empty or placeholder descriptions.
- Mob levels outside the zone's declared `min_level`/`max_level`.
- `DEATH` or `GODROOM` flags in a zone otherwise open to mortals.
- Object value vectors that do not match their item type.

Each check gets a stable code (for example `W301 one-way-exit`) so findings can
be suppressed individually and referenced in review.

### Phase 4 - Lookup surface, and retiring the flag tables

Ship `flags`, `sectors`, `show`, and `refs`. Then update
`docs/world_game-data/`:

- `ROOM_FLAGS.md` (22 KB) and `MOB_FLAGS.md` (29 KB) lose their bit tables,
  which become `wtool flags room --list` output, and keep only the prose that
  explains what a flag means and when to use it.
- `builder_manual.md` and `OEDIT_GUIDE.md` gain a short section on the
  validation loop.
- `lib/world/validate-zone.sh` becomes a thin wrapper that calls `wtool`, so
  existing muscle memory keeps working.

The judgment-shaped documentation stays as is: `gear_guide.md`,
`wilderness_system.md`, zone theming, and level-band guidance. Those cannot be
validated and should not be moved into a tool.

### Phase 5 - Emitter (follow-on, do not start early)

`wtool emit <spec.json>` producing correctly bit-packed, column-exact flat
files, validated by Phases 1 through 3 before being written. The existing
`scripts/world/populate_zone8000_from_south_of_waterdeep.py` is a proof that
this shape works; the emitter generalizes it. Because world data is not under
version control, the emitter must write to a staging directory by default and
require an explicit flag to touch `lib/world/`.

## 5. Testing

Python-side tests use the standard library `unittest` module. No new
dependency, and no change to `Makefile.am` or `CMakeLists.txt`, since no C
source is added.

Fixtures, under `scripts/world/wtool/tests/fixtures/`:

- **Good, real:** the tracked `lib/world/artifacts/1699.*` package.
- **Good, synthetic:** a minimal hand-written zone exercising every record
  type, including `V` string arguments and enhanced-form mobs.
- **Broken, synthetic:** one fixture per error code. This set is the real
  specification of the tool and should grow whenever a new defect is found in
  the live world.

Add a `make test-world-tools` phony target that runs the suite, and mention it
in `docs/TESTING_GUIDE.md`. Keep it out of the C `make test` path.

## 6. Sequencing and stopping points

Phases 1 and 2 together are the minimum useful product; either alone is not
worth shipping. Phase 3 is high value but can trail. Phase 4 should follow
soon after 2, because leaving stale flag tables in the docs alongside a
correct lookup command is worse than either alone.

Recommended checkpoint: after Phase 2, run `wtool validate --all` against the
live world and record the finding counts by code in this document. That number
is both the justification for the work and the baseline for cleanup.

## 7. Completion

When this finishes, the enduring content belongs in
`docs/systems/` as a world tooling reference, with the outcome recorded in
`docs/CHANGELOG.md`. These working notes can then be deleted per
[README.md](README.md).
