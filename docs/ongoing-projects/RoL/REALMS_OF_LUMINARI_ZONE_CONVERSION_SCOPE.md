# RealmsOfLuminari Zone Conversion Scope

- Status: Phase 6 special-procedure reconciliation in progress
- Assessment date: 2026-08-11
- Source: `EXAMPLE/RealmsOfLuminari/areas/`
- Target: this writable development checkout and its current `lib/world/`

## Executive conclusion

A full conversion is a reconciliation and migration program, not a file-copy or
simple syntax rewrite.
The source contains about 45.9 MB and 1.73 million lines of area data across 284
physical zone packages. Its room, mobile, object, shop, quest, action-list, and
zone-reset formats differ materially from the current server. Large parts of the
source VNUM space also collide with unrelated target content.

The development target already contains earlier Homeland/RoL-lineage conversions and related
special-procedure ports. Verified examples include source zone `507` at target zone
`1507`, source mobile `50789` at target mobile `150789`, source zone `960` at target
zone `1960`, and source object `96001` at target object `196001`. These are lineage
candidates, not a universal formula. The project must reconcile active source records
with the current development target before adding anything.

The completion target is fixed: the active working source dependency closure must be
structurally reconciled and functionally playable, including its resets, shops, HLQs,
SOC behavior, traps, and required special procedures. Automated validation and scripted
walkthroughs provide acceptance evidence. Disabled, unlisted, and demonstrably
non-working RoL content is permanently out of scope.

The completed pilot measured target reuse, ambiguity, capability coverage, special
binding density, and validation throughput. Twenty completed Phase 6 delivery sessions
are now archived. The remaining evidence-based forecast is 76-136 sessions, or 152-544
focused engineering hours at 2-4 hours per session. This is a planning envelope rather
than a calendar promise; the measured basis is recorded in
[RoL-Changelog.md](RoL-Changelog.md).

## Locked project decisions

- The current checkout and `lib/world/` are the writable authoritative development
  target. Repository and world-data writes are authorized.
- Do not create or require backup copies, snapshots, preimage captures, rollback
  artifacts, recovery plans, compare-and-swap gates, or remote/production captures.
  Disposable staging and validation are correctness tools only.
- Only active working source content and its dependency closure are in scope. Disabled,
  unlisted, and demonstrably non-working content receives no further work.
- An active instruction that depends only on excluded source content is minimally
  disabled and logged; it never pulls that excluded package back into scope.
- Preserve equivalent Luminari/OLC behavior. Otherwise implement intended gameplay,
  repair obvious source bugs, and disable/log the smallest irreducibly malformed unit.
- Full content rights and access are confirmed. Rights review is closed and must not
  return as a project risk or release gate.
- World data is ignored by Git to preserve player discovery and avoid spoilers, not for
  licensing reasons.
- All converted HLQs are emitted pre-approved with the canonical `!` marker.
- No active builders are available or required. Automated tests, reset observation, and
  scripted walkthroughs replace builder approval.
- Technical choices are resolved from source/target traces and pilot evidence without
  returning them as user decisions.

## Raw source inventory and locked active scope

The source corpus contains 284 physical zone packages. The active build manifests select
252 physical `.zon` files containing 255 zone records. Thirty disabled physical files
and two unlisted files are excluded without deeper reconciliation, conversion, mechanic
ports, or QA.

Six active manifest basenames lack their own `.zon` file. They are not automatically
discarded: some supply active companion-only object or quest data, while multi-header
zone files carry the related zone records. The active source build lists, generated
aggregates, runtime references, and dependency closure decide inclusion. The converter
must not invent a missing file or record.

The following table describes the complete source corpus only; it is not the implementation
scope or a completion checklist:

| Kind | Files | Bytes | Lines | Parsed records or structures |
|------|------:|------:|------:|-----------------------------:|
| `mob` | 260 | 6,448,241 | 200,972 | 13,505 mobiles |
| `obj` | 214 | 3,605,542 | 152,621 | 10,555 objects |
| `qst` | 261 | 2,390,960 | 81,688 | 5,081 blocks; 5,042 unique hosts |
| `shp` | 76 | 337,944 | 11,705 | 458 shops |
| `soc` | 91 | 703,602 | 32,554 | 1,758 mobile lists; 4,284 actions |
| `wld` | 284 | 29,506,576 | 1,169,654 | 54,037 room records; 54,019 unique VNUMs |
| `zon` | 284 | 2,931,956 | 84,134 | 287 records; 279 unique header numbers |
| **Total** | **1,470** | **45,924,821** | **1,733,328** | |

Companion data is not uniform. Of the 284 physical zone basenames, all have `wld`
and `zon` files, but only 258 have `mob`, 210 have `obj`, 255 have `qst`, 76 have
`shp`, and 91 have `soc`. The kind directories also contain a small number of
additional basenames not tied directly to a physical `.zon` basename.

Physical file and zone-record identity are not one-to-one. The full source corpus has 287 zone
records across 284 files; the active scope has 255 records across 252 files. The extra
active headers occur in `foggy_woods.zon` and `northern_highroad.zon`. Active inventory,
identity, and acceptance ledgers track both levels.

The source top-level `world.mob`, `world.obj`, and similar files are generated
aggregates. The converter consumes per-area active inputs so ownership, provenance,
and diagnostics remain package-specific. The aggregates are authoritative regression
oracles for active source build membership, order, and assembled runtime content.

## Blocking incompatibilities

### VNUM collisions

Unshifted source VNUMs cannot be preserved safely. The raw-source collision upper bounds
against the development target include:

| Namespace | Unique source records | Unique target records | Exact VNUM collisions |
|-----------|----------------------:|----------------------:|----------------------:|
| Mobiles | 13,505 | 14,679 | 733 |
| Objects | 10,555 | 12,321 | 1,225 |
| Rooms | 54,019 | 54,390 | 17,191 |
| Zone header numbers | 279 | current zone table | 102 |

The same-number collisions are mostly unrelated content. They do not reveal earlier
converted content stored under shifted VNUMs. Only 8 of the 733 colliding mobiles and
12 of the 1,225 colliding objects have the same normalized primary name. All 102
colliding zone header numbers have different normalized titles. For example, source
zone 0 is `God Rooms`, while target zone 0 is `Builder Academy`.

The raw source also contains these defects or ambiguities:

- Rooms 45100-45117 occur in both `dwarven_mines.wld` and `flesh.wld`.
- There are 39 duplicate quest host headers.
- Zone header numbers 466, 509, 757, 903, and 919 are reused; `466` occurs in five
  physical files, and nested header `903` collides with `mirar_ferry.zon`.
- `mytheast.zon` uses header `#81700` with a top value of 81899, unlike the normal
  source convention.

Only records selected by the active dependency closure require resolution. For those,
each duplicate receives deterministic ownership, merge, split, or smallest-unit exclusion,
and every non-duplicate receives a target-lineage search before new allocation.

### Unsupported source subsystems

Two source kinds do not have like-for-like target directories:

- Source `.qst` files are conversational high-level quests, not the target
  AutoQuest `.qst` format. They must be converted into target `hlq` records or a
  replacement system.
- Source `.soc` files are automated mobile action lists, not player social-command
  definitions. They require DG triggers or a deliberate compatibility subsystem.

The source `zon` format also contains reset commands whose letters conflict with
different target meanings. Copying them would produce incorrect behavior even when
the file parses.

### Code-dependent behavior

The source has 80 `specs.*.c` files totaling about 89,167 lines. Its main assignment
file contains at least 926 direct mobile, 282 direct object, and 354 direct room
special-procedure bindings, in addition to helper-driven zone assignments. An
approximate scan finds 2,769 unique three-to-six digit numeric literals across
those files; not every literal is a VNUM, but every behavioral reference must be
classified.

The target has its own named special-procedure registry, zone-specific
implementations, and numerous earlier Homeland-lineage ports. Some source behavior
already has a current equivalent or evolved target implementation, but names and
historical comments alone are not evidence of parity. Reuse and patch searches must
precede new ports. A data-only import can boot while still omitting zone behavior.

## Required conversion by data kind

### Rooms (`wld`)

The legacy room numeric line carries a zone value, legacy flags, sector, and room
dimensions. The target stores a zone VNUM, four flag vectors, sector, coordinates,
and different optional record types.

Required work:

- Remap room VNUMs, exit destinations, door keys, and all external room references.
- Convert descriptions, exits, extra descriptions, moving-room data, and named
  special-procedure bindings.
- Define a documented policy for legacy length/width/height values. The target does
  not store those dimensions directly; target room-size flags are only an
  approximation.
- Add required target coordinates and file terminators.
- Resolve the 18 duplicate room records before emission.

Risk: **High**, because rooms form the reference backbone for every other kind.

### Mobiles (`mob`)

The corpus contains 13,505 unique mobiles and several legacy format/version
variants. The target extended mobile format has a different flag layout and more
explicit race, class, size, saves, abilities, and optional fields.

Required work:

- Parse all observed source versions and report any record that does not match a
  known grammar.
- Map action flags, affect flags, positions, sex, attacks, race, class, alignment,
  abilities, saves, dice, gold, and experience by symbolic semantics.
- Define explicit defaults for target-only fields and a loss report for source
  fields with no equivalent.
- Remap shop keepers, quest hosts, resets, follow relationships, scripts, and
  special-procedure assignments.

Risk: **High**, due to enum drift and behavioral dependencies.

### Objects (`obj`)

The legacy object format has fewer main fields and value slots. Target object
meaning depends heavily on item type, flags, apply types, materials, spells, and
skills.

Required work:

- Map item types, wear and extra flags, anti-restrictions, apply locations, bonus
  types, materials, sizes, proficiencies, and economy fields by symbolic meaning.
- Interpret the legacy value array by item type and emit the correct target value
  slots.
- Map spell and skill references by registered name, not old numeric ID.
- Convert extra descriptions and affects.
- Remap reset, shop, quest, container, key, and special-procedure references.

Risk: **High**, because a syntactically valid but incorrectly mapped value array can
silently change gameplay.

### High-level quests (`qst` to `hlq`)

Source quest blocks are keyed by a host mobile and contain keyword/reply,
completion, disappearance, required-give, and reward directives. They are
semantically closer to the current high-level quest system documented in
[HLQUEST_FILE_FORMAT.md](../../world_game-data/HLQUEST_FILE_FORMAT.md) than to the
AutoQuest format documented in
[QUEST_FILE_FORMAT.md](../../world_game-data/QUEST_FILE_FORMAT.md).

Required work:

- Convert source keyword/reply entries to target ask entries.
- Compile required items, rewards, and completion behavior into target quest
  directions and input/output commands.
- Remap mobile, object, room, spell, and skill references.
- Resolve the 39 duplicate host headers by tracing effective source lookup and target
  multi-block behavior; do not assume duplicates are simply invalid.
- Classify each directive independently. Item-type input, random object ranges,
  prestige, no-op source experience rewards, coin handling, ordering, and
  disappearance semantics contain known gaps or ambiguities.
- Emit the canonical target `!` marker on every converted entry. All converted HLQs
  are pre-approved; there is no builder review state.

Risk: **Very high**, because this is behavior compilation rather than record
translation.

### Automated mobile actions (`soc`)

The source has 1,758 mobile action lists and 4,284 actions using 144 numeric action
codes. Of these actions, 2,518 use special codes for indoor-zone echo,
outdoor-zone echo, all-zone echo, room echo, or path movement. The remaining 1,766
refer to old numeric command-table positions.

Required work:

- Resolve every old numeric command ID against the source interpreter table and map
  it to command name and behavior. Current command-table indexes must not be used.
- Preserve chance, delay, ordering, messages, arguments, and action-list lifecycle.
- Generate DG mobile triggers and attach them through zone resets, or implement and
  document a compatibility subsystem.
- Add support for zone-wide indoor/outdoor echo and path sequencing where current DG
  commands cannot express the source behavior.
- Reserve collision-free trigger VNUMs and include them in the mapping manifest.

Risk: **Very high**. This subsystem needs execution-level tests, not just parse
validation.

### Zone metadata and resets (`zon`)

The target zone header requires explicit bottom/top ranges and a different flag
layout. A grammar-aware scan of all source reset streams observed these raw upper bounds:

| Command | Count | Conversion concern |
|---------|------:|--------------------|
| `M` | 32,112 | Load remapped mobile into remapped room |
| `O` | 4,535 | Load remapped object into remapped room |
| `P` | 3,845 | Put remapped object in remapped container |
| `G` | 4,288 | Give remapped object to mobile |
| `E` | 13,172 | Equip remapped object on mobile |
| `D` | 6,518 | Map exit and door state semantics |
| `R` | 684 | Remove remapped object from room |
| `F` | 1,485 | Legacy follow/group/mount behavior; no direct target reset |
| `X` | 422 | Legacy mobile removal; no direct target reset |
| `T` | 275 | Legacy time predicate; target `T` attaches a trigger |

Required work:

- Preserve evidence-confirmed existing ranges; generate canonical, explicit, non-overlapping
  ranges for records that are added or patched.
- Map base `M/O/P/G/E` commands with target argument order, conditional-chain, and
  probability semantics.
- Split `D` and `R` by used variant: source `D` bitmask/extensions/chance and
  chance-qualified `R` are not blanket conventional transforms.
- Compile legacy `F`, `X`, and time-predicate `T` behavior into DG/reset support or
  a bounded target extension selected by tests.
- Preserve lifespan, reset mode, active state, and supported zone flags by meaning.
- Add generated trigger attachment resets without confusing them with legacy `T`.

RoL has no `A` reset command. The two apparent matches from a line-oriented scan are
zone title lines such as `A Halruaan Airship 1~`, before command parsing begins. A
grammar-aware negative fixture must prevent this false positive.

Two `F2 ...` rows omit whitespace after the opcode but are accepted by the source
loader and are included in the `F` total. Separately, `GROUPING*` and
`GATE QUEST STUFF` are un-commented headings that the source first-character
dispatcher misreads as malformed `G` commands. They are source-defect fixtures and
are not included in the 4,288 intended `G` rows.

Risk: **Very high**, because a letter-for-letter conversion would be wrong.

## Cross-cutting transformations

The converter must also handle these corpus-wide concerns:

- **Color markup:** 461,060 legacy `&+X` color tokens occur in the relevant source
  files, while current world data does not use that markup. Conversion needs a
  token-aware map and literal-ampersand handling, not blind replacement.
- **ASCII and line endings:** generated data and reports must comply with repository
  requirements: ASCII-compatible UTF-8 text and LF endings.
- **Terminator and index generation:** source per-area inputs are aggregated by the
  old build process and generally omit target file terminators. Target per-zone
  files need correct `$`/`$~` endings and entries in each kind's `index`.
- **Symbol tables:** flags, item types, sectors, races, classes, spells, skills,
  affects, applies, wear positions, and commands must be mapped from the traced
  source and target definitions. Numeric equality must never be assumed.
- **External references:** references outside the imported corpus need an explicit
  map to existing target content, a generated replacement, or a hard conversion
  failure.
- **Provenance:** every emitted record should retain source kind, filename, original
  VNUM, and source hash in a machine-readable manifest.

## Recommended identity strategy

Use a reconciliation manifest, not a universal offset, as the canonical map. Resolve
each typed identity in this order:

1. preserve an evidence-confirmed existing target-lineage identity;
2. apply a deterministically resolved merge or explicit exception;
3. allocate a reserved identity for a genuinely absent or still-ambiguous active record;
   or
4. preserve target candidates and minimally exclude/log the active unit only when no
   safe identity can be emitted.

The earlier offsets remain candidates only for new records:

```text
CANDIDATE_NEW_ENTITY_OFFSET = 2000000
CANDIDATE_NEW_ZONE_OFFSET   =   20000
```

The resulting entity span `2000000-2999999` was empty in the locally assessed world
on 2026-08-11 and fits the target index type. That is dated evidence, not an
reservation. Check the current development world, declared zone ranges,
hardcoded assignments, configuration, and runtime-relevant database references.
Reserve the range before use, then revalidate it before each batch and at apply time.

Rules:

1. Existing evidence-confirmed room, mobile, object, zone, shop, and trigger mappings take
   precedence over candidate allocation.
2. Map references by typed manifest identity; never perform untyped textual
   replacement.
3. Preserve the source sparse layout for a new package only when the candidate range
   is reserved and the resulting target zone range is valid.
4. Treat shop and quest identities according to their actual grammars: source
   `SHOP:` is a keeper mobile, and a source quest header is an HLQ host mobile.
5. Allocate generated DG triggers deterministically inside an explicitly reserved,
   OLC-valid range owned by the resolved destination zone.
6. Record formulas, existing mappings, exceptions, source/target hashes, and all
   consuming references in the mapping manifest.
7. Resolve duplicate zones, rooms, quest hosts, and the anomalous `mytheast.zon`
   header explicitly.
8. Fail on an unresolved required reference instead of emitting `NOWHERE`, zero, or
   a guessed VNUM.

## Converter architecture and deliverables

The implementation is a standalone, deterministic reconciler/converter operating on the
current source and target inventories. It writes to an isolated run directory first for
validation, then may apply a validated bundle directly to the writable development world.
It is not embedded in server boot logic.

Required deliverables:

- Source and development-target input inventory manifests.
- A baseline validation report with finding identities and parse completeness.
- A grammar-aware corpus inventory command that produces counts, duplicates,
  companion coverage, format variants, and unknown-token reports.
- Typed parsers for all seven legacy kinds.
- A normalized intermediate representation with typed references and provenance.
- A target-lineage matcher and explicit `KEEP/PATCH/ADD/MERGE/EXCLUDE`
  record-action ledger.
- Source-to-target symbolic mapping tables with tests.
- An existing-mapping-first identity resolver, reserved new-record allocator, typed
  collision checker, and mapping manifest.
- Emitters for target `wld`, `mob`, `obj`, `shp`, `hlq`, `trg`, and `zon` files plus
  indexes and terminators.
- A loss/exception report listing every default, dropped field, unsupported action,
  unresolved dependency, duplicate resolution, and engineering override.
- Structural comparison reports for source records versus emitted records.
- Focused fixtures covering every observed version, extension, reset command, and
  malformed edge case.
- A zone acceptance checklist and a corpus progress ledger.
- A change bundle with input/output hashes, planned actions, and validation results.
- Documentation for rerunning the converter and reviewing ignored world-data changes.

For a bootable staging root `<isolated-lib-root>`, use the actual validator forms:

```bash
python3 scripts/world/wtool.py \
  --world-root <isolated-lib-root>/world validate --all --strict

lib/world/validate-zone.sh <zone-vnum> \
  --world-root <isolated-lib-root>/world --strict

bin/circle -c -d <isolated-lib-root>
```

Full behavioral boots additionally require an isolated MariaDB test instance.

## Phased implementation plan

Each session below should remain a bounded 2-4 hour spec with roughly 12-25 concrete
tasks. Session counts include implementation and focused verification, not unattended
calendar time.

| Phase | Scope | Sessions |
|------:|-------|---------:|
| 6 | Reconcile/reuse/patch/port special procedures | 28-60 remaining |
| 7 | Action-based batches, zone QA, and validation bundles | 42-66 |
| 8 | Isolated integration, development apply, and documentation | 6-10 |

Completed Phases 0-5 and the first twenty Phase 6 delivery sessions have been removed from
this active scope; their delivery, acceptance evidence, and reforecast basis are in
[RoL-Changelog.md](RoL-Changelog.md). Active work continues with the remaining Phase 6
families and must preserve the pilot's deterministic, no-clobber, structural, reset,
walkthrough, and isolated runtime gates.

## Acceptance criteria

### Corpus and structure

- Every record and companion file in the active build/dependency closure is accounted
  for. The 30 disabled and 2 unlisted physical files require no ledger entries.
- Active companion-only basenames contribute existing data selected by the source build;
  no absent `.zon` or other record is fabricated.
- Every in-scope record has a final record action; every duplicate room, quest host,
  zone header, and target-lineage ambiguity has a deterministic resolution.
- Generated files contain valid target syntax, terminators, and indexes.
- Generated text passes ASCII/UTF-8 and LF checks.
- Record counts and every intentional omission are reconciled in a conversion report.

### References and VNUMs

- No generated VNUM collides with existing target content or another generated
  record in the same typed namespace.
- Every internal typed reference resolves to the intended kept, patched, merged, or
  added record.
- Every external reference is explicitly mapped or rejected.
- A stable source-to-target mapping and provenance record exists for every in-scope
  entity, including reused target identities.

### Runtime behavior

- The complete staged world is checked with `wtool.py validate --all --strict` and
  strict per-zone commands using the explicit staging world root.
- Validation compares finding identities and parse completeness, not process status
  alone: no new global finding is allowed, and selected/touched records have no
  unresolved errors.
- `bin/circle -c -d <isolated-lib-root>` passes before full behavioral boot.
- The server boots staged data with an isolated test database and without relevant
  invalid-record, unresolved-reference, reset, trigger, or `SYSERR` messages.
- Representative resets, doors, containers, equipment, shops, quests, action lists,
  and special procedures have automated tests.
- Each in-scope zone has automated reset observation and scripted walkthrough evidence.
- Every converted HLQ entry contains the canonical `!` pre-approval marker.
- `make test` succeeds, followed by `make install`, before integration is considered
  complete.

### Conversion and application correctness

- Conversion and boot testing use an isolated staging world and isolated test database.
- Source and development-target inventories record the inputs and content hashes used by
  each conversion run.
- Baseline validator output identifies pre-existing findings so they remain
  distinguishable from conversion regressions.
- No existing target content or OLC edit is overwritten implicitly.
- Integration is reviewable through record-action ledgers and validation bundles;
  ordinary Git diffs are insufficient because live world data is intentionally ignored.
- Validated bundles may update this development world directly.

## Open-decision status

There are no open authority, write-permission, active-scope, behavior-policy, rights,
quest-approval, builder-review, or technical-routing decisions. Engineering findings
must resolve through the locked rules and tests rather than returning as questions.

## Recommended next step

Continue Phase 6 with the 580 pending direct bindings and 289 pending `ACT_SPEC`
records. Process the next reusable families, then proceed by consuming package, reusing
current target procedures before adapting or porting source behavior. Preserve the six
explicit source-defect or ignored content rows as logged smallest-unit exclusions.
Completed Phase 5 work and Phase 6 evidence checkpoints are recorded in
[RoL-Changelog.md](RoL-Changelog.md).
