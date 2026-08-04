# World Validator CLI - Implementation Plan

Status: implementation in progress. Work began on 2026-08-04 from commit
`f3f105af`. This plan was re-audited against the current source tree on that
date.

Implementation progress:

- [x] Phase 0 - Foundation, constants, and indexes
- [x] Phase 1 - Zones, rooms, and critical references
- [x] Phase 2 - Mobiles, objects, triggers, shops, and full references
- [ ] Phase 3 - Semantic and topology lint
- [ ] Phase 4 - Lookup, documentation drift gates, and adoption
- [ ] Completion audit, operational validation, and final documentation move

Current checkpoint (2026-08-04): Phases 0 through 2 are implemented. Phase 2
adds all six structural parsers, source-derived wear-slot and limit contracts,
typed reference edges, and the complete graph pass. In addition
to the source-derived constants, source cursor, deterministic reporting, index
validation, and build integration from Phase 0, `wtool` now parses zones and
rooms, models reset queue and host state, validates room ownership and load
order, and checks exits, moving-room links, reset room targets, door targets,
and persisted room spec-proc names. `validate --zone` merges unindexed
canonical packages into the normal reference graph, while `validate --paths`
remains isolated from the live world.

`make test-world-tools` passes 78 tests; the equivalent CMake target and CTest
entry pass the same suite; `constants sync --check` is clean. The tracked
artifact and minimal zone/room bundles parse without errors and remain
byte-for-byte unchanged. A hash-guarded `validate --all` development-world run
completed in one pass without changing any file. It reported 57 errors:
`REF001` 9, `REF003` 38, `WLD014` 1, `WLD040` 4, `WLD041` 1, `ZON028` 1,
`ZON037` 1, and `ZON043` 2. It also reported 565 warnings: `IDX008` 329,
`ZON027` 230, `ZON033` 2, `ZON036` 3, and `ZON037` 1. These are operational
findings in ignored builder-owned data, not test failures or repository data
changes. The tracked Phase 2 fixture now covers all six file types, every
object extension, enhanced mobiles, all trigger host types, modern and legacy
shops, moving rooms, reset host state, and normal/mini indexes. The real
artifact bundle remains clean, while the minimal bundle now exposes its two
out-of-range mobile positions without being modified.

The hash-guarded Phase 2 development-world run loaded 516 zone files with 516
records, 517 room files with 50,357 records, 517 mobile files with 14,661
records, 516 object files with 12,261 records, 332 trigger files with 1,974
records, and 509 shop files with 709 records. A warm JSON run completed in
7.97 seconds with a 257,792 KiB maximum resident set and did not change any
world file. It reported 3,627 errors and 9,954 warnings. Error triage found no
remaining validator bugs: `OBJ021` (2,863) is the source loader's stale-field
bug on short `A` records, while the remaining 764 errors are builder-owned
world-data defects or unsafe values. Those remaining errors are `MOB001` 1,
`MOB010` 350, `MOB014` 99, `MOB015` 3, `MOB018` 2, `OBJ022` 1, `OBJ040` 2,
`OBJ041` 1, `REF001` 9, `REF003` 38, `REF021` 17, `REF022` 180, `REF023` 48,
`SHP016` 3, `WLD014` 1, `WLD040` 4, `WLD041` 1, `ZON028` 1, `ZON037` 1, and
`ZON043` 2. Warning counts are `IDX008` 329, `MOB017` 4,139, `MOB026` 25,
`OBJ015` 4,819, `OBJ016` 3, `REF025` 87, `REF030` 315, `REF031` 1, `ZON027`
230, `ZON033` 2, `ZON036` 3, and `ZON037` 1. Phase 3 semantic and topology
lint is next. Phase 5
remains the separately gated emitter follow-on and is not part of this
validator closeout.

This is the build plan for `wtool`, a standalone world-data validator and
lookup CLI intended primarily for AI agents doing world-building work, and
usable by human builders, pre-deployment checks, and CI fixture tests.

The guiding decision is: **build the oracle before the generator.** Producing
`.wld`, `.mob`, or `.zon` text is the easy half of world building. Proving that
the server will interpret it as intended is the hard half. Structured
authoring and bulk generation are not safe until a fast, precise, read-only
correctness check exists.

Source references below use file paths and function or table names rather than
line numbers. The world-documentation audit found that line-number citations
had become stale wholesale; symbol anchors are the more durable contract.

## 1. Why the current situation needs this

### 1.1 The existing checker is a stub

`lib/world/validate-zone.sh` is 33 lines. It checks that four files exist,
looks for at least one line ending in `~`, and, if a built server is present,
shells out to a full syntax-check boot. It cannot parse one record or associate
an error with its actual source line.

### 1.2 The boot-time check is a poor authoring loop

`bin/circle -c` sets `scheck` in `src/comm.c:main()` and calls
`src/db.c:boot_world()`. Three properties make it unsuitable as the primary
validator:

- **It requires MySQL.** `boot_world()` calls `connect_to_mysql()` before any
  world file is read. Pure flat-file validation should not need a database.
- **It requires a full C build.** An agent changing only world data should not
  need to compile the server first.
- **Most structural failures are fail-fast.** `parse_room()`, `load_zones()`,
  `setup_dir()`, and the other loaders commonly call `exit(1)`. One run often
  exposes only one problem.

The validator must report every independently recoverable finding. It must not
promise recovery after an unterminated string or another ambiguity where the
next record boundary cannot be identified safely; in that case it reports one
primary error, marks the file parse incomplete, and resumes only at a credible
record header.

### 1.3 Important defects the boot misses or handles destructively

- **Dangling exits.** `src/db.c:renum_world()` replaces an unresolved
  `to_room` vnum with `NOWHERE` without logging it.
- **Sorted-table assumptions.** `real_room()`, `real_mobile()`,
  `real_object()`, `real_zone()`, `real_trigger()`, and `real_shop()` use
  binary search, but their loaders do not consistently reject duplicate or
  out-of-order vnums. An out-of-order room within one zone is not necessarily
  fatal at load time; it can instead make valid records unreachable later.
- **Silent zone-header truncation.** `load_zones()` tries 14, then 11, then
  10, then 4 fields. Headers with 5-9, 12-13, or more than 14 fields can be
  accepted after silently discarding data. Its additional `zone_fix` fallback
  can reinterpret a numeric-looking zone name as the header.
- **Zone reset parser/runtime disagreement.** The command prescan and parse
  pass accept different whitespace. `I` takes an unintended extra integer,
  `L` is non-functional, and `save_zone()` writes an extra numeric placeholder
  for `V` lines that `load_zones()` then consumes as the variable name. These
  behaviors are documented in `docs/known-issues.md` where already known; the
  validator must diagnose their data manifestations.
- **Reset dependencies are not Boolean.** `src/db.c:test_result()` interprets
  the `if_flag` field as a signed offset into a 127-result queue. Positive
  offsets require an earlier success and negative offsets invert an earlier
  result. Treating it as only `0` or `1` misses real behavior.
- **Unresolved content is often dropped rather than rejected.** Missing shop
  products, keepers, trigger attachments, and some reset targets can be
  discarded or disabled while boot continues.
- **Flag chunks have a 32-bit logical width.** `Q_FIELD()`, `Q_BIT()`, and
  `sprintascii()` use four 32-bit chunks, even though `bitvector_t` is an
  `unsigned long` on this platform. `asciiflag_conv()` accepts letters through
  `Z`, so bits 32-51 can be stored in a chunk even though normal bit-array
  access cannot address them there. `check_bitvector_names()` only logs and
  does not reliably account for each chunk's global offset.
- **Index omission is silent.** A valid file on disk that is absent from the
  normal `index` is simply not loaded.

`renum_zone_table()` does log familiar `ZONE ERROR` messages for several
missing reset targets. `wtool` should preserve the same concepts and identify
the same command and vnum, but its stable finding code is the public contract;
it should not copy incidental wording or typos from server logs.

### 1.4 Flag documentation is currently correct, but can drift

The 2026-08-04 world-documentation audit verified all 42 room flags and all
105 mobile flags and repaired the surrounding documentation. The problem is
now drift prevention, not present table corruption. The authoritative display
tables remain `room_bits[]`, `zone_bits[]`, `action_bits[]`,
`affected_bits[]`, `wear_bits[]`, `extra_bits[]`, and `sector_types[]` in
`src/constants.c`; numeric defines live in bounded blocks in `src/structs.h`.

The extractor must not gather constants by a broad prefix. For example,
`MOB_BLOCK_E` is a mobile flag while `MOB_DIRE_SPIDER` is a vnum with a
colliding numeric value. Extraction needs explicit source blocks and count
terminators.

### 1.5 World data is not version controlled

`.gitignore` excludes the live files in `lib/world/{wld,mob,obj,zon,shp,trg}`.
Only support files and small bundles are tracked. Consequences:

- The tool lives in the repository; most data it reads does not.
- Validation, lookup, and documentation-check commands must never modify
  `lib/world/`.
- CI can validate tracked fixtures and constants drift, but it cannot validate
  the full builder-owned world unless a read-only world snapshot is supplied.
- `lib/world/artifacts/1699.{wld,mob,obj,zon}` is a useful real fixture.
  `lib/world/minimal/` supplies a second tracked bundle. Neither covers every
  shop, trigger, moving-room, and index construct, so synthetic fixtures are
  still required.

## 2. Scope and safety boundary

**In scope:** parsing and validating `.wld`, `.mob`, `.obj`, `.zon`, `.shp`,
`.trg`, `index`, and `index.mini`; cross-file reference checks; flag and
constant lookup; record and reverse-reference lookup; deterministic text and
JSON output; and bounded drift checks for world-building documentation.

**Out of scope:** any world-data mutation; wilderness generation and its
MySQL-backed region/path data; `.qst`, `.hlq`, help entries, and other
MySQL-backed content; arbitrary semantic analysis of DG script bodies; and
automatic repair. Literal vnums in well-defined flat-file fields are checked;
vnums assembled dynamically inside scripts are not.

`sync-constants --write` is the one pre-emitter command allowed to write. It
writes only the checked-in derived constants manifest in the repository, and
only when explicitly requested. `validate`, `show`, `refs`, `flags`, and
`docs --check` are read-only. Phase 5 writes only to an explicit staging
directory by default.

## 3. Design decisions

- **Runtime:** Python 3.10+ using only the standard library, tested on the
  current Python 3.12 runtime. This avoids a MySQL connection, C build,
  virtual environment, and third-party parser dependencies.
- **Layout:** `scripts/world/wtool.py` as the entry point,
  `scripts/world/wtool_lib/` as the package, and `scripts/world/tests/` for
  tests. A distinct package name avoids import ambiguity beside `wtool.py`.
- **Source of truth:** the current Luminari branches of the C loaders and OLC
  writers. Accepted grammar comes from what boots, while OLC output supplies
  required round-trip cases.
- **Constants:** generate `scripts/world/wtool_constants.json` from explicit C
  tables and bounded define blocks. Lookup then works without parsing C at
  runtime, while `--check` prevents drift.
- **Parser model:** typed intermediate records with source spans, followed by
  separate structural, reference, and semantic passes. `show`, `refs`, and
  diagnostics therefore use one interpretation of the files.
- **Input handling:** a byte-preserving reader with explicit ASCII token
  parsing and UTF-8 plus `surrogateescape` for display text. One legacy
  non-UTF-8 description must not prevent validation of the rest of a world.
- **Output:** deterministic human text by default and versioned JSON on
  request. Humans need `path:line`; agents and CI need a stable schema.
- **Data selection:** an explicit world root and index mode, with a separate
  isolated-path mode. Full-world and staged-package validation have different
  reference universes.

### 3.1 Compatibility policy

The parser must distinguish three things:

1. **Boot grammar:** reproduce `get_line()` skipping blank lines and lines
   beginning with `*`, raw tilde-string reads, sentinel values, and the
   conversion prefix accepted by each `sscanf()` call. Reset-line annotations
   after the required numeric prefix are valid because the C loader ignores
   them.
2. **Known destructive acceptance:** parse enough to continue, but emit a
   finding when the server silently truncates, clamps, drops, rewrites, or
   misroutes data.
3. **Recommended form:** report legacy forms as warnings, not structural
   errors, when the current server accepts them.

`src/config.c` currently sets `bitwarning = FALSE`, so the legacy 3-field room,
4-field mobile, and 4-field object bitvector forms are accepted and can be
rewritten by the server's conversion path. The original plan incorrectly said
they were rejected. `wtool` must accept them, warn that they are legacy, and
never rewrite them. The object loader also enters that branch for a 3-field
line but then passes uninitialized `f3` bytes to `asciiflag_conv_aff()`; treat
that form as an error rather than trying to reproduce undefined behavior.

Legacy mobile and object affect tokens use `asciiflag_conv_aff()`, whose
alphabetic form shifts each bit by one. Modern affect chunks use
`asciiflag_conv()`. These forms need separate compatibility tests rather than
one generic flag decoder.

Python integer parsing must also enforce the signed or unsigned range of the C
destination. Unlimited Python integers must not make an overflowing `%d` field
look valid.

### 3.2 Command surface

Examples below use `wtool` as shorthand for `./scripts/world/wtool.py`.

```
wtool [--world-root PATH] [--config PATH] [--json] [--ignore-code CODE] validate --all [--strict]
wtool [--world-root PATH] [--config PATH] [--json] [--ignore-code CODE] validate --mini [--strict]
wtool [--world-root PATH] [--config PATH] [--json] [--ignore-code CODE] validate --zone N [N ...] [--strict]
wtool [--json] [--ignore-code CODE] validate --paths PATH [PATH ...] [--strict]

wtool flags list <room|zone|mob|affect|affect2|obj-extra|obj-wear|obj-affect|obj-affect2>
wtool flags decode <set> TOKEN [TOKEN ...]
wtool flags encode <set> NAME [NAME ...]
wtool constants list <sectors|item-types|positions|directions|trigger-types>

wtool [--world-root PATH] [--json] show <zone|room|mob|obj|shop|trigger> VNUM
wtool [--world-root PATH] [--json] refs <zone|room|mob|obj|shop|trigger> VNUM

wtool constants sync --check
wtool constants sync --write
wtool docs --check
```

Defaults and selector semantics:

- `--world-root` defaults to `<repo>/lib/world`, found relative to the entry
  point rather than the caller's current directory.
- `--config` defaults to the sibling `lib/etc/config` when one exists. Parse
  the `diagonal_dirs` setting because it changes both room-exit loading and
  reset `DIR_COUNT`. An isolated package without a config uses the source
  default and reports that assumption.
- `--all` loads the normal `index` files and validates that complete indexed
  graph.
- `--mini` loads `index.mini`. A mini index is intentionally a subset; files
  not listed in it are not reported as omissions.
- `--zone` builds the normal indexed graph for reference resolution but emits
  record findings for the selected zone packages. It also reads canonical
  `N.ext` files that exist but are not indexed, merges those selected records
  into the graph, and reports the index omission. This keeps new zones,
  cross-zone exits, and shared prototypes meaningful.
- `--paths` validates explicit files or package directories as an isolated
  graph. It does not silently consult the live world. A later option may add an
  explicit reference world if staging workflows demonstrate the need.
- `--strict` makes warnings affect the exit status; it does not rewrite their
  severity in JSON.
- A repeatable `--ignore-code` marks matching warning and info findings as
  suppressed. Human output omits them by default; JSON retains them with
  `suppressed: true`, and summaries show active and suppressed counts. Error
  findings are never suppressible in the first release.

`refs` requires a record type because room, mobile, object, shop, and trigger
vnum spaces overlap. The original untyped `refs <vnum>` surface was
ambiguous.

Exit codes:

- `0`: validation completed and no active error findings were produced; with
  `--strict`, no active unsuppressed warnings were produced either.
- `1`: input data findings reached the active failure threshold, or a check
  command such as `constants sync --check` found drift. A missing file named
  by an index is a data finding and uses this status.
- `2`: invalid invocation, inaccessible requested root, constants/schema
  mismatch that prevents operation, or an internal tool failure.

JSON goes to stdout with no banners or ANSI escapes. Operational diagnostics
go to stderr. Human and JSON findings use repository-, package-, or
world-root-relative paths, and the envelope records a normalized root label
rather than an absolute machine path, so output is stable across machines.

### 3.3 Finding and JSON contract

Finding codes use a stable subsystem prefix and number, for example
`IDX001`, `ZON014`, `WLD021`, `REF003`, and `SEM012`. Severity is data, not part
of the code, so changing a warning threshold does not rename the finding.

Severity meanings are:

- **error:** boot fails or crashes, a required record is inaccessible, or the
  server silently loses, redirects, or cannot address the supplied data;
- **warning:** the server accepts the data, but it is legacy, silently clamped,
  or highly likely to be unintended; and
- **info:** style, topology, or consistency advice with legitimate common
  exceptions.

Each finding contains:

- `code`, `severity`, and `message`;
- `path`, physical `line`, optional `column`, and optional end line;
- record type and vnum when known;
- optional related path/line/vnum for the other end of a reference; and
- `suppressed`, set by an explicit warning/info suppression.

The JSON envelope contains `schema_version`, tool version, normalized root
label, mode, effective config assumptions, a deterministic findings array,
active and suppressed counts by severity and code, and a `complete` Boolean.
Findings sort by normalized path, line, column, code, and message. Two
identical runs over identical bytes must produce byte-identical JSON apart from
an explicitly excluded timing field; the initial schema should omit timestamps
and timing entirely.

## 4. Implementation phases

### Phase 0 - Foundation, constants, and indexes

1. Add the entry point, package, typed finding model, deterministic reporters,
   exit handling, and a source cursor that supports both raw tilde strings and
   `get_line()`-style significant lines while retaining physical positions.
2. Define dataclasses for zone, room, mobile, object, shop, trigger, reset
   command, exit, attachment, and index records. Parsers return partial records
   plus findings rather than printing or exiting.
3. Implement constants extraction for the supported Luminari preprocessor
   branch. Use a fail-closed conditional filter for the campaign guards found
   inside selected blocks; do not concatenate both sides of a campaign
   conditional or silently ignore an unfamiliar directive. At minimum include:

   - `room_bits`, `zone_bits`, `action_bits`, `affected_bits`,
     `affected2_bits`, `wear_bits`, `extra_bits`, `sector_types`, `item_types`,
     `position_types`, and `dirs`;
   - `trig_types`, `otrig_types`, and `wtrig_types`;
   - the matching bounded define blocks and count terminators from
     `src/structs.h`, `src/db.h`, `src/dgscript/dg_scripts.h`, and
     `src/obj/shop.h`;
   - logical chunk width 32, four serialized chunks, the `a-zA-F` writer
     alphabet, aliases, reserved entries, source symbols, and a manifest schema
     version; and
   - parser limits such as `READ_SIZE`, `MAX_STRING_LENGTH`, `MAX_PATH`,
     `MAX_OBJ_AFFECT`, `MAX_WEAPON_SPELLS`, `MAX_SHOP_OBJ`, and `RQ_MAXSIZE`.
4. Make `constants sync --check` compare a fresh normalized extraction with the
   checked-in JSON without writing. `--write` uses an atomic replace and is the
   only write form.
5. Parse normal and mini indexes using the actual safe-component rule from
   `src/utils.c:is_safe_path_component()` and validate:

   - duplicate entries, unsafe names, wrong extensions, missing listed files,
     and missing conventional `$` termination;
   - numeric package filenames that move backward in index order as a warning;
   - non-empty zone, room, mobile, and object datasets for full boot mode;
   - unlisted `N.ext` files as a warning for the normal index only; and
   - mini entries as an intentional subset, never a bidirectional disk check.
6. Update `Makefile.am` and `CMakeLists.txt` together when the Python sources
   and tests are added. Add a phony `test-world-tools` make target, an
   equivalent CTest entry using `Python3::Interpreter`, and the new
   source/fixture files to the applicable distribution lists. Keep the target
   out of production-linked C `make test`, but invoke it from `test-all`.
7. Update `.github/workflows/test.yml` path filters for `scripts/world/**`, the
   tracked fixtures, and constants sources, and run the Python suite in a
   lightweight job from the first implementation phase.

Exit criteria:

- `wtool flags list room` has the same 42 entries, indices, display names, and
  macro names as the source.
- Encoding a flag above index 31 places it in the correct later token, and a
  round trip preserves all four 32-bit chunks.
- `constants sync --check` is clean and detects a deliberately stale copy.
- Index tests prove that `index.mini` does not report every normal-world file
  as missing from the mini set.

### Phase 1 - Useful vertical slice: zones, rooms, and critical references

This phase deliberately delivers the highest-value oracle before every other
file type is implemented.

**`.zon`**, against `src/db.c:load_zones()`,
`src/db.c:renum_zone_table()`, `src/db.c:test_result()`,
`src/db.c:reset_zone()`, and `src/olc/genzon.c:save_zone()`:

- One `#vnum` record per file, one-line builders and name fields with optional
  trailing `~`, numeric header, reset commands, `S`, and conventional `$`.
- Header field counts of exactly 4, 10, 11, or 14. Counts 5-9, 12-13, and more
  than 14 are errors because the C fallback silently loses data. Do not accept
  the `zone_fix` name-as-header fallback as valid input.
- `bot <= top`; reset mode, lifespan, level band, zone flags, and integer range
  checks; unique and strictly increasing zone vnums in index load order;
  ranges sorted by bottom; no overlap.
- Command prescan compatibility: command letter in column 0, a literal space
  as the second byte, and `S` alone. Tabs or indentation are errors because the
  prescan and parse loop otherwise disagree.
- Required integer conversions after the command letter, **including the
  dependency offset**: `M O E P` take 4 or 5; `G` takes 3 or 4; `D T` take 4;
  `V` takes 4 integers, one word, and a non-empty remainder; `J` takes 2 or 3;
  `L` takes 3; and the effective current forms of `R` and `I` take 3. Trailing
  OLC annotations are allowed after non-`V` commands.
- Reject unknown or lowercase command letters explicitly. Enforce the C
  parser's 79-byte limits for each `V` string so truncation is not mistaken for
  a faithful value.
- Treat `R` as its harmless effective 3-integer form, but guard the source
  dispatch discrepancy with a compatibility test. Diagnose the dummy integer
  required by current `I` parsing, non-functional `L`, and the five-integer
  `V` form emitted by `save_zone()` that shifts the variable name/value.
- Validate percentage ranges, wear positions, door directions and states,
  non-negative jump counts, and signed dependency offsets. Model the real
  127-entry result queue: `0` means unconditional, a positive offset tests an
  earlier result, and a negative offset inverts it.
- Simulate enough reset state to prove that `G`, `E`, `I`, `T`, and `V` have the
  required current mobile/object/script host. Account for `J` skips and for
  current commands that fail to push a result, rather than assuming every
  command is a simple Boolean chain.

**`.wld`**, against `src/db.c:parse_room()`,
`src/db.c:setup_dir()`, and `src/db.c:setup_moving_room()`:

- `#vnum`, name and description tilde strings, and a modern 6-field or accepted
  legacy 3-field flags/sector line. The first integer on that line is ignored
  by `parse_room()`; compare it with the range-selected owning zone and report
  a mismatch rather than trusting it for ownership.
- `D<dir>` exits with two strings and three integers. Direction must fit the
  supported Luminari direction table before indexing the exit array. A
  diagonal exit while `diagonal_dirs` is disabled is an error because
  `setup_dir()` returns without consuming its block and desynchronizes the
  room parser. Preserve the `-1`, `0`, and `65535` sentinel behavior for
  destinations and the `-1` and `65535` key sentinels.
- `E` extra descriptions; the two-line `C` coordinate construct; `M` moving
  room headers, messages, and connection list; `Z` spec-proc names; `S`; and
  post-`S` inline `T <trigger-vnum>` attachments.
- A moving-room block has five header integers, three message lines, and
  three-integer connections through a `~` sentinel. Mirror the source's 1-50
  repeat clamp and warn when it changes input. Treat an expanded connection
  count at or above `MAX_MOVING_ROOMS` as an error: the source's strict `<`
  storage check drops the boundary entry, and its later copy loop can read
  beyond the fixed temporary arrays when the count exceeds the limit.
- Diagnose malformed `C` coordinates even though the C loader's bare
  `sscanf()` leaves defaults without an error. Enforce line and tilde-string
  limits that the C buffers impose.
- Every room belongs to exactly one zone range. Room vnums must be globally
  strictly increasing in index load order. The current parser catches only
  some zone regressions; the validator catches all duplicates and inversions.

**Critical references delivered in this phase:**

- Every non-sentinel exit destination exists. This is the primary dangling
  exit check.
- Moving-room connection rooms exist, and their directions are valid.
- `M`, `O`, `D`, `R`, `T`, and `V` reset room fields resolve, with
  `O ... NOWHERE` handled as the source permits. Prototype and trigger targets
  wait for the full parsers in Phase 2.
- `D` names an exit that actually exists on the room.
- Room spec-proc names resolve through the registry in `src/spec_assign.c`.

Exit criteria:

- The tracked artifact and minimal zone/room fixtures parse without error.
- Deliberately broken fixtures cover header truncation, prescan desync,
  out-of-order rooms, malformed coordinates, invalid directions, reset queue
  offsets, the `I`/`L`/`V` traps, and dangling exits.
- `validate --all` over a read-only copy of the indexed development world
  reports all zone/room findings in one run without changing any file.

### Phase 2 - Mobiles, objects, triggers, shops, and full references

**`.mob`**, against `parse_mobile()`, `parse_simple_mob()`,
`parse_enhanced_mob()`, and `interpret_espec()`:

- Four tilde strings; modern 10-field or accepted legacy 4-field flag header;
  the simple-mob 9-conversion dice line, 2-integer gold/experience line, and
  3-integer position/sex line; `S` versus enhanced `E`; all recognized enhanced
  keywords and their source clamp ranges; enhanced terminator; and post-record
  trigger attachments.
- Diagnose unknown enhanced keywords, malformed `MFeat`, `Feat`, `Aff2`, and
  `Path` values, too many path rooms for `MAX_PATH`, invalid positions, sex,
  level, dice, and reserved flags. Values the server silently clamps get a
  warning showing stored versus effective value.

**`.obj`**, against `parse_object()` and `check_object()`:

- Four tilde strings; a modern first numeric line of 13 fields (type plus 12
  flag chunks) or 17 fields (plus four permanent-affect chunks), while still
  recognizing the accepted 4-field legacy form and diagnosing the unsafe
  3-field path described in the compatibility policy.
- Exactly 4 or 16 value integers, followed by 3-5 weight/cost/rent/level/timer
  integers with the same defaults and clamps as the C loader.
- All extension records actually accepted by the switch: `A`, `B`, `C`, `E`,
  `G`, `H`, `I`, `J`, `R`, `K`, `P`, `S`, `T`, and `Z`. Include the six-affect
  limit, spellbook slots, special abilities, extra descriptions, material,
  size, recipient, restring id, activated spells, weapon spells, trigger
  attachments, and spec-proc names.
- Record the exact extension payloads: `A` accepts 2-4 integers; `B` takes 2;
  `C` takes 7 plus an optional command word; `G`, `H`, `I`, and `J` take 1;
  `R` and `Z` take one following line; `K` takes 5; `S` takes 4; `T` is an
  inline attachment; and `P` consumes no payload. Preserve the source's shared
  `A`/`B` slot counter when checking bounds. Diagnose 2/3-integer `A` records:
  the loader accepts them but leaves `specific` in `t[3]` from earlier parsing,
  so only the 4-integer form initializes the complete affect deterministically.
- Object records have no `S` record terminator; the next `#` or `$` belongs to
  `discrete_load()`. Recovery and tests must reflect that. Diagnose the no-op
  `P` extension and more than `MAX_WEAPON_SPELLS` weapon spell entries even
  though the source's bound check is commented out.

**`.trg`**, against `src/dgscript/dg_db_scripts.c:parse_trigger()`:

- Name, 2/3-field numeric header, argument list, and command-list tilde
  strings; attach type 0-2; attach-specific flag table; optional numeric
  argument; non-empty command body; and strictly increasing unique vnums.

**`.shp`**, against `src/obj/shop.c:boot_the_shops()` and its read helpers:

- Tilde-string headers and `#vnum~` records, version-sensitive producing and
  buy-type lists, two profit values, seven checked message strings, temper,
  flags, keeper, customer restrictions, room list, four hours, and `$~`.
- Require the `v3.0` marker for modern variable-length lists. Validate list
  terminators and `MAX_SHOP_OBJ`, item-type names/numbers, message `%s`/`%d`
  ordering, hours, and unique sorted shop vnums. Accept the legacy fixed-five
  list form with a warning when the marker is absent and the record is
  structurally complete.

**Cross-file graph:**

- Strictly increasing, globally unique mobile, object, trigger, and shop vnums
  in index load order. A file's numeric zone basename disagreeing with the
  owning zone range is a packaging warning when that ownership is knowable.
- Reset targets: mobile/object prototypes, destination rooms, `P` containers,
  `E` slots and wear compatibility, `R` room/object pairs, trigger types and
  hosts, and variable hosts/scripts.
- Exit and moving-room keys exist; a non-`ITEM_KEY` key is a warning.
- Inline room/mobile/object triggers exist and their declared attach type
  matches the host. Reset `T` commands receive the same check.
- Shop keepers, rooms, products, and buy item types resolve. Missing products
  that the server silently drops are errors.
- Mobile `Path` rooms, object recipient mobs, spec-procs, and typed vnum slots
  in object value vectors and special abilities resolve where their source
  semantics are unambiguous.

Exit criteria:

- A synthetic complete package exercises every file type and extension record,
  including modern and legacy shop lists, enhanced mobs, every reset command,
  `V` strings, trigger host types, and normal/mini indexes.
- Every high-confidence reference kind has a missing-target and wrong-type
  test.
- The complete indexed development world validates in less than 10 seconds on
  the reference development host with a warm filesystem cache. Record file and
  record counts with the benchmark; do not hard-code a historical claim such
  as "579 zones" into acceptance criteria.

### Phase 3 - Semantic and topology lint

Warnings and info findings only unless a value is provably unaddressable or
silently changed.

- Decode four flag chunks using `global_bit = chunk * 32 + local_bit`. Report
  invalid characters, negative numeric masks, local bits above 31, and global
  bits beyond the corresponding source table. Numeric masks such as `12` are
  valid and common; do **not** warn merely because a token is all digits.
- Sector types must be in `0 .. NUM_ROOM_SECTORS - 1`. The loader's `>` rather
  than `>=` check does not make the terminator index valid.
- Reserved/system-set room, mobile, object, and affect flags in prototype data.
- Empty or known placeholder names/descriptions after stripping color codes.
- Rooms with no exits, physical one-way exits, key asymmetry, and duplicate
  exits. Exempt explicit sentinels and non-physical transports.
- Reachability roots are rooms with incoming exits from another zone. If a
  zone has none, use its lowest room only as an explicitly reported fallback.
  Report the root set with each unreachable-room finding so the result is
  reproducible.
- Compare levels of mobiles actually loaded by `M` resets into a zone against
  that destination zone's declared level band. Do not infer ownership solely
  from a `.mob` filename.
- `ROOM_DEATH` and `ROOM_STAFFROOM` placement in zones otherwise open to
  mortals. The former plan's `GODROOM` name does not exist.
- Port the high-confidence checks already present in `check_object()` and add
  item-type-specific value-vector bounds from the real consumers and
  `docs/world_game-data/OEDIT_GUIDE.md`. Avoid speculative rules for unused
  slots.
- Optional consistency findings for an exit whose reverse exit has a different
  key, keyword, or door capability.

Each semantic check receives a stable code and a focused fixture. A check that
cannot state its false-positive exemptions is not ready to ship.

### Phase 4 - Lookup, documentation drift gates, and adoption

1. Ship `flags`, `constants list`, `show`, and typed `refs` from the same
   parsed model used by validation. Flag encoding accepts unambiguous macro or
   display names and prints all four flat-file tokens.
2. Add `wtool docs --check`, limited to the world-building documentation set.
   It should verify:

   - coverage and indices for the room, mobile, object-extra, wear, item-type,
     and sector reference tables that the selected documents actually publish;
   - cited `src/...` paths and explicitly named source symbols;
   - documented OLC commands against `cmd_info[]` in `src/interpreter.c`; and
   - ASCII, UTF-8 validity, and LF line endings.
   Do not treat every identifier in illustrative prose as source code without
   an explicit marker or narrow allowlist.
3. Keep the useful human quick-reference tables in `ROOM_FLAGS.md`,
   `MOB_FLAGS.md`, and `OEDIT_GUIDE.md`; they were verified correct in the
   completed audit. Prevent drift with `docs --check` instead of deleting them.
4. Add the validation loop and command examples to
   `BUILDER_QUICKSTART.md`, `ZONE_FILE_FORMAT.md`, `SHOP_FILE_FORMAT.md`,
   `builder_manual.md`, and `OEDIT_GUIDE.md` as relevant. Update
   `docs/guides/TESTING_GUIDE.md` and the documentation master index.
5. Replace `lib/world/validate-zone.sh` with a compatibility wrapper that finds
   the repository from its own path, invokes `wtool validate --zone`, forwards
   options, and preserves the exit status even when called outside the repo.
6. Extend the Makefile, CTest, and CI gates established in Phase 0 with the
   wrapper and documentation checks. CI validates fixtures and
   `constants sync --check`; it does not pretend to validate ignored live data.
7. Keep the existing `scripts/development/generate-web-guides.sh --check` in
   the documentation gate rather than duplicating its HTML generation logic.

There is no in-game command in this project, so no helpfile is required. If a
future in-game wrapper is added, its help entry becomes part of that separate
change.

### Phase 5 - Emitter follow-on (do not start early)

`wtool emit SPEC.json --output STAGING_DIR` may produce correctly bit-packed,
column-compatible flat files, then run Phases 1-3 before completing. The
existing `scripts/world/populate_zone8000_from_south_of_waterdeep.py` is proof
that structured generation can work, but it is not a substitute for the
validator.

The emitter must use atomic files in an explicit staging directory. Any option
that writes to `lib/world/` requires a new reviewed plan, an unmistakable
opt-in flag, a pre-write backup, and validation before replacement. It is not
authorized by this document.

## 5. Testing strategy

Use standard-library `unittest`. Tests live under `scripts/world/tests/`, with
fixtures under `scripts/world/tests/fixtures/`.

Fixture classes:

- **Real tracked bundles:** `lib/world/artifacts/1699.*` and
  `lib/world/minimal/`.
- **Complete synthetic tree:** canonical `zon/wld/mob/obj/shp/trg`
  directories with normal and mini indexes, at least one valid record of every
  type, and every optional record construct.
- **Broken focused fixtures:** one primary defect per high-confidence finding
  code. Add a regression fixture whenever a live-world defect is confirmed.
- **Recovery fixtures:** multiple independent errors in one file, plus an
  unrecoverable tilde-string case that proves `complete: false` and avoids a
  cascade.

Required test layers:

1. Reader tests for physical line tracking, comments/blanks, CRLF handling,
   tilde termination, byte preservation, line-length limits, and C integer
   bounds.
2. Flag tests for numeric and alphabetic forms, four 32-bit chunks, aliases,
   invalid high local bits, and macro-prefix collisions.
3. One parser test module per file type, including every accepted legacy form
   and every OLC-emitted modern form.
4. Graph tests for duplicates, load order, each reference edge, reset state,
   dependency offsets, trigger host types, and normal versus mini membership.
5. Golden human and JSON tests for stable ordering, source positions, schema,
   summaries, clean stdout/stderr separation, and exit codes.
6. Read-only tests that hash or snapshot every input fixture before and after
   `validate`, `show`, `refs`, `flags`, and `docs --check`.
7. Wrapper smoke tests from the repository root and an unrelated working
   directory.
8. `constants sync --check` and world-documentation drift checks.

`make test-world-tools` runs the complete Python suite directly. The target
must not build `circle`, require MariaDB, leave a root-level binary, or write to
the ignored world directories. The equivalent CTest entry runs the same suite,
not a divergent subset.

An optional operational acceptance run may use a read-only copy of the
builder-owned world. It is not a unit-test fixture and must not connect to or
modify production. Record counts by finding code and tool version in a
separate dated report or in this working document until the project completes.

## 6. Sequencing and release gates

The former plan made all structural parsers one phase and all references the
next, which delayed the first useful result. The revised order is vertical:

1. Phase 0 establishes trustworthy constants, input selection, and output.
2. Phase 1 ships zone/room validation and dangling-exit detection as the first
   useful release.
3. Phase 2 completes the six-format graph and is the minimum full product.
4. Phase 3 can land check by check; no speculative lint blocks Phase 2.
5. Phase 4 makes the tool discoverable, prevents documentation drift, and
   wires fixture coverage into CI.

At each phase boundary, require clean unit tests, deterministic JSON, a
read-only assertion, and documentation for the commands actually available.
Do not advertise a later-phase command from the entry-point help before it is
implemented.

After Phase 2, run `wtool validate --all` against a read-only development-world
copy and record counts by code. Triage every error as one of: validator bug,
source-loader bug, or world-data defect. A booting world is not automatically
the expected-zero oracle because several checks exist specifically for silent
boot-time loss.

## 7. Completion

When Phases 0-4 are complete, move enduring usage and architecture material to
`docs/utilities/WORLD_VALIDATOR_CLI.md`, link it from
`docs/utilities/README.md` and the master index, document testing in
`docs/guides/TESTING_GUIDE.md`, and record the outcome in
`docs/CHANGELOG.md`. The working notes can then be deleted according to
`docs/ongoing-projects/README.md`.

Phase 5 remains a separately gated follow-on even after the validator plan is
closed.
