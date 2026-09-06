# World Validator, Lookup, and RoL Reconciliation CLI

`wtool` is the standalone validator, lookup, source-inventory, and RoL reconciliation utility for
LuminariMUD flat world data. It parses the same eight formats used by the server
without starting the game or compiling `luminari`:

- zones (`.zon`)
- rooms (`.wld`)
- mobiles (`.mob`)
- objects (`.obj`)
- shops (`.shp`)
- DG Script triggers (`.trg`)
- quests (`.qst`)
- high-level quests (`.hlq`)

Validation, lookup, RoL inventory, flag conversion, documentation checks, and
`rol-persistence-check` are read-only. The persistence check is the only RoL command
that connects to MariaDB; it uses `lib/mysql_config`, requires `APP_ENV=development`,
and executes only `SELECT` statements. Evidence and generation commands write new,
explicit run directories but never modify the source corpus. `rol-phase8-apply` is
the only maintained world-data apply path. The maintainer operation
`constants sync --write` replaces the checked-in derived constants manifest.

## Requirements and Entry Point

Run commands from the repository root with Python 3.10 or newer:

```sh
python3 scripts/world/wtool.py --help
python3 scripts/world/wtool.py --version
```

The current release reports `wtool 0.9.0`.

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

The RoL inventory path reuses the byte-preserving source reader for source
manifest lines and zone headers. It then hashes and classifies physical files
without invoking a source or target game runtime.

The RoL baseline path hashes target indexes and files, reproduces the source
aggregate builder byte for byte, checks typed candidate ranges in world and code,
captures full target diagnostics, and writes a unique evidence bundle. It does not
connect to a database. Its versioned policy is
`scripts/world/rol_conversion_policy.json`.

The Phase 1 discovery path parses all seven active source grammars, resolves
typed dependencies, extracts command and special-procedure bindings, generates
non-destructive lineage candidates, and records an owned capability disposition for
every observed construct. It does not connect to a database. The Phase 2 planner
verifies those artifacts before assigning every active record a deterministic
`KEEP`, `PATCH`, `ADD`, `MERGE`, or `EXCLUDE` action. Neither command writes
`lib/world/`. The Phase 3 walking skeleton verifies a confirmed `KEEP`, copies
the target into an isolated staging tree, validates both trees with the same
configuration, and proves two applies are zero-write no-ops.

The Phase 4 selection path verifies the Phase 1 and Phase 2 artifact chains before
extracting the manually selected representative packages, their source hashes,
record actions, identities, references, capabilities, SOC modes/action codes, and
special-procedure bindings. It enforces the complete pilot coverage contract without
automating the engineering priority decision.

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

## Canonical RoL Namespace and Maintenance Contract

Conversion status: complete through Phase 8. The accepted import is an additive
overlay in the dedicated RoL namespace. Existing Luminari zones, records, and
persistent identities are preserved in place. The phase commands remain
available to audit or deterministically regenerate the conversion; they are
not an alternate builder workflow for assigning new VNUMs.

Treat installed records in the canonical RoL namespace as generated output. A
maintenance regeneration removes that namespace only in its staging copy, recreates
it from the selected source manifests and versioned policy, and preserves non-RoL
record blocks. Builder or OLC edits to generated RoL records are not merge inputs and
can be overwritten by the next accepted regeneration.

Canonical RoL identities use these formulas:

```text
target zone VNUM   = normalized source zone VNUM + 20000
target entity VNUM = source entity VNUM + 2000000
```

Rooms, mobiles, and objects remain separate typed namespaces. Shops, HLQs,
SOC behavior, generated triggers, and special bindings attach through typed
canonical owners; a matching number in another namespace is not evidence of
identity. A package may span multiple 100-VNUM entity bands, so its top and
sparse layout must be derived from its records rather than from a single
band assumption.

The only source-zone normalization in the completed corpus is
`mytheast.zon`: malformed source header `#81700` represents logical zones
817-818 and therefore maps to zone 20817, entities 2081700-2081899, and top
2081899. This is an evidence-backed package exception, not a general
divide-by-100 rule.

The similarly named packages coexist; similarity is never lineage evidence:

| Existing Luminari content, unchanged | Imported RoL content, additive |
|--------------------------------------|--------------------------------|
| Trail zone 1507 and 150xxx entities | RoL source zone 507 at zone 20507 and 20507xx entities |
| Hulburg zone 1591 and 159xxx entities | RoL source zone 591 at zone 20591 and 20591xx entities |
| Jotunheim zone 1960 and 196xxx entities | RoL source zone 960 at zone 20960 and 20960xx entities |
| Luminari artifacts 169901-169910 | RoL objects at their independent 2000000+ identities |

No Luminari VNUM is retired by the RoL import. Do not create forwarding
records, compatibility aliases, low-VNUM fallback lookup, or package-name
matching. Regenerate converted data from the selected source manifests and
`scripts/world/rol_conversion_policy.json`; do not hand-edit generated output
as its source of truth. Converter-owned RoL flags and reset, shop, or object
extensions preserve imported behavior, while new builder content should use
native mechanics unless it intentionally extends the converted compatibility
contract.

The exact artifact identities and persistence rules are maintained in
[ARTIFACT_SYSTEM.md](../systems/ARTIFACT_SYSTEM.md).

Any maintenance regeneration must leave these checks at zero:

```text
noncanonical active RoL zone identities       = 0
noncanonical active RoL entity identities     = 0
modified preserved Luminari records           = 0
cross-world typed references                   = 0
missing reserved-namespace typed targets       = 0
```

The accepted result must also preserve target/OLC edits through explicit
record actions, add no normalized validation finding, resolve persistent
objects to one live prototype, produce byte-identical output from identical
inputs, and make repeat application a no-op. See the canonical maintenance
gate in `docs/guides/TESTING_GUIDE.md` for the complete acceptance list.

## Realms of Luminari Source Inventory

Inventory the ignored reference corpus at its default repository location:

```sh
python3 scripts/world/wtool.py rol-inventory
python3 scripts/world/wtool.py --json rol-inventory > /tmp/rol-inventory.json
```

Use another checkout or a tracked fixture with an explicit source root:

```sh
python3 scripts/world/wtool.py --json rol-inventory \
  --source-root /srv/reference/RealmsOfLuminari
```

The source root must contain `areas/`. The command follows the selection paths
in `EXAMPLE/RealmsOfLuminari/src/build_areas.c`:

| Manifest | Selected kinds |
|----------|----------------|
| `areas/AREA` | `zon`, `wld`, `soc` |
| `areas/AREA.mobobj` | `mob`, `obj` |
| `areas/SHOP` | `shp` |
| `areas/QUEST` | `qst` |

These are source build-list rules, not Luminari index rules. A `*` is a comment
marker only in column zero. The first whitespace-delimited token on every
other line is the basename, and later columns are annotations. File order and
manifest source lines are retained in the inventory. Blank active lines,
unsafe or non-ASCII basenames, duplicate active entries, and entries too long
for the source build buffers return status 2 with exact manifest paths and line
numbers.

Every physical file has one primary status:

| Status | Meaning |
|--------|---------|
| `active` | Its kind-specific manifest selects the basename; the file is included. |
| `disabled` | A column-zero disabled entry names the file; it is inventoried but excluded. |
| `unlisted` | No active or disabled entry names the file; it is inventoried but excluded. |

Package records add `missing-companion` when an active selection has no file
of that kind, and `multi-zone` when a physical `.zon` contains more than one
zone header. Missing companions are reported rather than invented. An active
object, quest, or other companion remains included even when its basename has
no physical `.zon`.

JSON output is deterministic and contains:

- inventory and tool schema versions plus a normalized root label;
- all four manifest hashes, selected kinds, basename order, and source lines;
- all seven data kinds with path, byte size, SHA-256, manifest membership,
  inclusion state, and zone header identities where applicable;
- package-level present, selected, included, missing, disabled, and unlisted
  kind sets; and
- direct classification lists and aggregate counts used by conversion gates.

No timestamp, modification time, absolute in-repository path, or directory
enumeration order enters the payload. Identical bytes therefore produce
identical human and JSON output. The command never creates an aggregate,
repairs a companion, or writes source data.

## Realms of Luminari Phase 0 Baseline

Generate the complete Phase 0 evidence bundle against the writable
development target:

```sh
python3 scripts/world/wtool.py --world-root lib/world rol-baseline \
  --source-root EXAMPLE/RealmsOfLuminari \
  --output-dir lib/rol-conversion/runs/phase0-YYYYMMDD
```

The output directory must not already exist. Generated runs are ignored by
Git because their inventories and diagnostics describe builder-owned world
data. The command never connects to MariaDB.

Use `--created-at` with an ISO-8601 timestamp when reproducing a run manifest
with a controlled creation time. All other artifact content is derived from
input bytes, policy, tool version, source revision, or target revision.

The bundle contains:

| Path | Evidence |
|------|----------|
| `run-manifest.json` | Run identity, revisions, policy, artifact hashes, and acceptance summary |
| `source-inventory.json` | Active source manifests, packages, hashes, and exclusions |
| `target-inventory.json` | Target indexes, hashes, missing entries, and orphaned files |
| `source-aggregate-reconciliation.json` | Exact per-kind rebuild and aggregate comparison |
| `collision-evidence.json` | Typed world and code-binding range checks |
| `policies.json` | The exact versioned conversion policy used by the run |
| `validation/baseline.json` | Full `validate --all` result, finding identities, and parse state |

The aggregate comparison mirrors the source C reader, including its behavior
of dropping an unterminated final fragment. It records the path, size, and
hash of each such fragment rather than silently treating a naive
concatenation as authoritative.

## Realms of Luminari Phase 1 Discovery

Generate grammar, closure, binding, lineage, and capability evidence:

```sh
python3 scripts/world/wtool.py --world-root lib/world rol-discover \
  --source-root EXAMPLE/RealmsOfLuminari \
  --output-dir lib/rol-conversion/runs/phase1-YYYYMMDD
```

The unique output contains source and target inventories, the grammar summary,
all normalized source records, one lineage-candidate row per active record, one
resolution row per typed reference, source/target special bindings, the capability
matrix, byte and semantic aggregate reconciliation, the locked policy, and a manifest
hashing every artifact. The command never connects to MariaDB.

Unknown syntax is an operational error. Known source-loader losses remain explicit
warnings or smallest-unit exclusions. Aggregate semantic differences are accepted
only when they are bounded by a recorded unterminated tail from the exact byte
reconciliation.

## Realms of Luminari Phase 2 Planning

Build a complete, non-writing action and identity plan from a verified discovery run:

```sh
python3 scripts/world/wtool.py rol-plan \
  --discovery-dir lib/rol-conversion/runs/phase1-YYYYMMDD \
  --output-dir lib/rol-conversion/runs/phase2-YYYYMMDD
```

The planner verifies every input hash and discovery acceptance gate. It emits the
record-action ledger, canonical identity map, capability rows, apply-oriented change
plan, schemas, summary, and run manifest. Target package similarity and matching low
VNUMs never establish lineage. Every imported identity is emitted in the reserved
namespace. `MERGE` in the ledger means only that duplicate records inside the RoL
source share one RoL identity; it never means merging an RoL record into an existing
Luminari record. Known source-invalid dependent instructions remain explicit smallest-
unit exclusions.

## Realms of Luminari Phase 3 Walking Skeleton

Exercise the complete no-clobber delivery path for the smallest confirmed
prior-lineage zone slice:

```sh
python3 scripts/world/wtool.py --world-root lib/world rol-skeleton \
  --plan-dir lib/rol-conversion/runs/phase2-REVISION \
  --output-dir lib/rol-conversion/runs/phase3-REVISION
```

`rol-skeleton` is retained only to replay frozen Phase 3 evidence; it must not
select current destinations or authorize target lineage. When the skeleton
consumes its historical Phase 2 bundle, it verifies every
artifact hash and target-file precondition, inventories the target tree,
creates an isolated staging copy, and validates both trees with identical
grammar configuration. Its two historical `KEEP` applies must perform zero
writes, preserve the authoritative target hash, and add no finding.

Use a different `--basename` only when the Phase 2 reconciliation contains
exactly one confirmed zone `KEEP` for that package. The output directory must
not exist. `--created-at` controls only the manifest timestamp; with the same
inputs and timestamp, independent runs produce byte-identical evidence artifacts
and the same run ID.

## Realms of Luminari Phase 4 Pilot Selection

Create the measured pilot selection from verified Phase 1 and Phase 2 evidence:

```sh
python3 scripts/world/wtool.py --json rol-pilot-select \
  --discovery-dir lib/rol-conversion/runs/phase1-REVISION \
  --plan-dir lib/rol-conversion/runs/phase2-REVISION \
  --output-dir lib/rol-conversion/runs/phase4-select-REVISION
```

The locked selection contains `swamp_two`, `hulburg`, `muspel`, `theswamp`, and
`cemetery`. It was chosen manually from measured coverage and is rejected unless it
continues to cover a compact conventional-reset package, a shop/quest settlement,
all five SOC modes, all five special SOC action codes, follow/calendar/removal
resets, uncommon room/object extensions, significant special-procedure dependencies,
and confirmed-lineage reuse.

The bundle contains the selected source oracle, all selected normalized records,
the exact Phase 2 actions and core identities, all outgoing reference resolutions,
active special-binding candidates, aggregate coverage, and a manifest hashing every
artifact. It performs zero live target writes.

## Realms of Luminari Phase 5 and Phase 6 Audits

Audit every planned capability and reconcile every active special binding:

```sh
python3 scripts/world/wtool.py rol-capability-audit \
  --plan-dir lib/rol-conversion/runs/phase2-REVISION \
  --output-dir lib/rol-conversion/runs/phase5-REVISION

python3 scripts/world/wtool.py rol-special-reconcile \
  --discovery-dir lib/rol-conversion/runs/phase1-REVISION \
  --plan-dir lib/rol-conversion/runs/phase2-REVISION \
  --capability-audit-dir lib/rol-conversion/runs/phase5-REVISION \
  --output-dir lib/rol-conversion/runs/phase6-REVISION
```

Both commands verify the manifests they consume. Phase 6 accounts for direct
bindings, dynamic registrations, implicit race bindings, handlers, and every
active `ACT_SPEC` record. Its accepted bundle has no pending binding.

## RoL object-format conversion contract

The source loader and native object loader have different grammars and enum
namespaces. `rol_source.py` parses source records; `rol_transform.py` emits the
native representation. The [native serialization reference](../world_game-data/OEDIT_GUIDE.md#native-object-file-serialization)
describes the target. Keep conversions explicit in the policy and diagnostics.

Native validation counts `A` applies and `B` spellbook entries independently,
matching the server loader. Legacy two/three-field applies receive zero for
omitted bonus type/specific fields. Each extension still enforces its own capacity.

- Source object text uses tilde strings and RoL `&` color syntax. The shared text
  converter preserves `&+x` foreground, `&-x` background, and `&=xy` combined colors,
  including source background blinking, using existing target protocol tokens.
  Source black `L/l` maps to target `D/d`; target `L/l` would display lime.
  `&&` becomes a literal ampersand. Unknown complete escapes remain literal and
  incomplete escapes are dropped as in the source output loop; both are diagnosed.
  Literal at-sign escaping, ASCII, LF, and tilde framing apply to every format.
  Source flags
  are decimal words, with type/extra/wear and optional anti flags. Source values
  have eight fields; target records have sixteen. Economy is weight/cost/
  durability, not the target weight/cost/rent/level/timer line. Source whitespace
  can cross line boundaries, so do not infer field boundaries from presentation.
- Optional source affect words encode one-based bits 1..64. The source loader
  clears AFF_HIDE. Type, flag, apply and spell IDs require semantic maps; raw
  integer passthrough across these namespaces is unsafe.
- Source extensions are extra descriptions, up to two applies, then at most one
  six-integer `T` record. Source `T` is not a native DG attachment. Content after
  that source record can be ignored by the source loader and is diagnosed as
  `ROLOBJ004`; missing action-description recovery uses `ROLOBJ005`.
- Source ability modifiers are not a reason for a blanket 4.5 multiplier in the
  D20 target. Preserve the established direct mapping unless a named conversion
  policy requires otherwise. Source drink-container weight uses a /4 conversion.
- Source armor value 0 has positive protective AC, while its AC apply uses a
  different sign convention. Other armor values describe warmth/prestige/proc,
  not a target armor-family index. Preserve protection while making the target
  family/slot decision explicitly; AC magnitude alone cannot identify armor.
- Armor-typed nonstandard wearables become `ITEM_WORN`. Their value-0 protection
  becomes signed `APPLY_AC_NEW`: divide magnitude by ten, round toward zero,
  and retain magnitude one for a nonzero value. Universal bonus type 23 stacks
  with preserved authored applies and other equipment. Clear value 0 to prevent
  double-counting, including rings worn on tails. Preserve normalized wear flags
  and takeability; take-only placeholders gain no inferred equipment slot.
  Standard armor slots, mixed masks containing them, and dedicated-tail armor
  remain outside this rule and require their separate family/disposition review.
  Nonstandard warmth/prestige and unbound procedure-state losses are diagnosed
  per record. Assigned special adapters retain procedure state and effect ownership;
  for example, the tattered cloak still uses value 3 for its recharge counter.
- Weapon profiles distinguish melee, launchers, thrown weapons, ammunition and
  quivers. Javelin names alone do not establish throwing intent; thrown darts and
  blowgun ammunition are distinct. Use the classifier audit and reviewed
  overrides, not renamed enum numbers or ad hoc emitted-file edits.
- Source ship loading forces `ITEM_LIT` even when its file omits that flag.
  Converted ships retain boat behavior and receive target `ITEM_MAGLIGHT`.
  Magical item light follows direct room, inventory, and equipment placement;
  containers conceal it, and magical darkness takes precedence.
- Armor, object-level policy, and structured extension losses still require
  explicit review before claiming complete object equivalence.

Track armor inference in [#113](https://github.com/LuminariMUD/Luminari-Source/issues/113), other field policies in
[#114](https://github.com/LuminariMUD/Luminari-Source/issues/114), and weapon release review in [#115](https://github.com/LuminariMUD/Luminari-Source/issues/115).
Audit counts describe a particular source package; regenerate them for each
reviewed conversion. Preserve converter source manifests and evidence hashes
when changing output-affecting code.

The policy's `class_map` converts world/mobile class statistics, not player
archetype progression. RoL's single-class level-50 kits cannot be equated with
Luminari's multiclass level-30 classes solely by name. Current converted totem
support preserves 21 identities using Cleric level 21/Wisdom; it is not a native
Shaman progression. Player-kit and spell-assignment decisions are tracked in
[#117](https://github.com/LuminariMUD/Luminari-Source/issues/117).

## RoL Mobile Conversion Contract

RoL mobile combat rows are not copied directly because the source and target use
different encodings and runtime adjustments. The converter instead:

1. maps a positive source level with
   `min(34, (3 * min(source_level, 59) + 4) / 5)` using integer division;
2. resolves one broad race, up to three subraces, final size, class, and explicit
   encounter tier through hash-checked exact-record, phrase, and source-code fallback
   rules in `scripts/world/rol_conversion_policy.json`;
3. submits that identity to the native `mob_autoroll_calculate()` implementation
   through `util/rol_mob_calculator`; and
4. serializes target-native hitroll, armor, HP, damage, abilities, saves, rewards,
   `Tier:`, subrace, size, and `SpellRes:` fields with their source and target
   dispositions in the generation ledger.

The calculator bridge checks its protocol and profile versions, executable hash, and
echoed inputs. Python has no production formula fallback. A valid explicit source
magic-resistance value is persisted; otherwise the source race-and-level result is
derived and persisted once. Target-native rewards own experience and gold, including
explicit `MOB_CUSTOM_GOLD` ownership. Ordinary generated mobiles retain variable HP as
one deterministic `1dY+H` roll; exact custom profiles use fixed `1d1+(H-1)` HP. Source
race-list aggression and source prestige bonuses remain explicit bounded exclusions
because Luminari has no equivalent safe race-list aggression primitive or mobile
prestige kill-reward field. Named World Boss overrides require exact, hash-bound custom
profiles.

Encounter Tier affects only values saved by autoroll; it does not add loader-time or
live-combat bonuses. See the
[Mobile Flags guide](../world_game-data/MOB_FLAGS.md#encounter-tier-separate-scalar-field)
for the builder and runtime contract.

## RoL Database Boundary

World generation, baseline capture, discovery, planning, capability analysis,
special reconciliation, and Phase 7 compilation do not connect to MariaDB.

Persisted player, pet, house, quest, vessel, and subsystem state can contain world
VNUMs. Before release, validate those references against the candidate:

```sh
python3 scripts/world/wtool.py \
  --world-root <candidate-lib>/world \
  --json rol-persistence-check \
  --development-lib-root <development-lib>
```

The command is deliberately narrow:

- it reads the repository's `lib/mysql_config` by default, or the configuration under an
  explicitly supplied `--development-lib-root`;
- it refuses to run unless the selected lib root's `.env` identifies the environment exactly as
  `development`;
- its query runner accepts only one `SELECT` or `SHOW` statement and rejects
  semicolons and every write-shaped statement, and every connection sets the
  database session to `READ ONLY` before running the query;
- it reports only typed VNUMs, counts, definition status, and a database-identity
  hash, never credentials or serialized object contents; and
- it fails unless every persisted RoL VNUM resolves to exactly one indexed candidate
  definition.

Phase 8 runs this same check against its assembled candidate. Use
`--development-lib-root` when the candidate is being sealed from an isolated worktree whose
protected credential files are intentionally absent. The converter cannot create, copy, migrate,
recover, or select a database.

## Realms of Luminari Phase 7 and Phase 8

Generate cumulative Phase 7 milestones after batches 4, 8, and 12. Each invocation
regenerates from the frozen, authoritative Luminari development baseline; it
does not modify that baseline.

```sh
python3 scripts/world/wtool.py --world-root lib/world rol-phase7 \
  --discovery-dir <phase1-directory> \
  --plan-dir <phase2-directory> \
  --capability-audit-dir <phase5-directory> \
  --phase6-dir <phase6-directory> \
  --through-batch 12 \
  --prior-milestone-dir <batch4-directory> \
  --prior-milestone-dir <batch8-directory> \
  --output-dir <phase7-final-directory>
```

The command freezes the input trees, derives dependency-complete package batches,
converts every selected record, compiles SOC and special behavior, patches preserved
canonical records, validates the full assembled world, and records preservation and
runtime-contract evidence. The final milestone requires 258 packages, 71,680 record
actions, no new normalized active error, and a byte-identical independent repeat.

Conversion selection is package- and dependency-based, not extension-based. There is
no supported `.mob`-only, `.obj`-only, `.wld`-only, or similar release mode. Intermediate
milestones may stop at a cumulative dependency batch, but Phase 8 requires the complete
final Phase 7 corpus. Use `validate --zone` or `validate --paths` for scoped diagnosis;
the accepted release still validates a complete assembled world.

After the code suites, install, candidate syntax boot, and bounded runtime boot pass,
assemble the Phase 8 release candidate:

```sh
python3 scripts/world/wtool.py --world-root lib/world rol-phase8 \
  --phase7-dir <phase7-final-directory> \
  --repeat-phase7-dir <phase7-repeat-directory> \
  --world-tools-log <world-tools-log> \
  --cutest-log <cutest-log> \
  --install-log <install-log> \
  --syntax-log <syntax-log> \
  --runtime-log <runtime-log> \
  --development-lib-root <development-lib> \
  --output-dir <phase8-release-directory>
```

`rol-phase8` reproduces the complete candidate from the frozen baseline and Phase 7
overlay, reconciles counts and actions, proves that source-internal `MERGE` actions
target no existing Luminari record, rejects every cross-world typed reference,
audits RoL mechanics markers and code identities, checks persisted VNUMs read-only
against the candidate, and emits a hash-preconditioned additive apply plan. Apply and
seal it only in development:

```sh
python3 scripts/world/wtool.py --json rol-phase8-apply \
  --bundle-dir <phase8-release-directory> \
  --lib-root lib

python3 scripts/world/wtool.py --json rol-phase8-completion \
  --bundle-dir <phase8-release-directory> \
  --lib-root lib \
  --output-dir <phase8-completion-directory>
```

The apply command rejects production, changed inputs, changed runtime binaries, and
modified bundle artifacts. Reapplication is a verified no-op. The completion command
requires the development tree and post-apply diagnostics to match the accepted
candidate, rechecks documentation, and records the no-op reapplication.

`rol-phase8-apply` copies only apply-plan paths whose current hashes differ, but it does
not create a backup or write-ahead journal, automatically restore a partial application,
or provide a `rol-phase8-rollback` command. Before applying, create and independently
verify an exact snapshot of the complete development world tree. Do not use the apply
command without that external recovery point.

Candidate boots use the repository's local development database through
`lib/mysql_config`.

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

### Retired Semantic Checks

Two semantic checks are permanently disabled. Their code is retained, commented
out, in `scripts/world/wtool_lib/semantics.py` so the rationale stays with the
implementation. Neither code can be emitted any more, so `--ignore-code` entries
for them are unnecessary.

| Code | Was | Why it is retired |
|------|-----|-------------------|
| `SEM004` | Warning on empty or known-placeholder descriptive text | Fired on every in-progress builder record and on legitimate short-form text, drowning the actionable findings. |
| `SEM006` | Warning on a physical exit with no reverse exit | One-way connections are a deliberate, widespread mechanic in the legacy zones. |

Removing `SEM006` does not weaken the paired-exit checks. `SEM007` (key
mismatch), `SEM008` (keyword mismatch), and `SEM009` (door-capability mismatch)
still run over every genuine reciprocal pair.

### Room Reachability (`SEM011`)

`SEM011` reports a room that no player can enter by any modelled route. The
check only warns after every entrance has been ruled out:

- **Exits.** The room is not walkable from any zone entrance. Entrances are the
  rooms targeted by incoming cross-zone exits.
- **Portal objects.** No `ITEM_PORTAL` object names the room. Both
  `PORTAL_NORMAL`/`PORTAL_CHECKFLAGS` exact destinations and `PORTAL_RANDOM`
  destination ranges are honored, whichever zone the portal object lives in.
- **DG scripts.** No trigger teleports into the room or wires a runtime exit to
  it. The parser reads literal room vnums out of `mteleport`/`oteleport`/
  `wteleport`, `mat`/`oat`/`wat`, `mgoto`, and the `*door <room> <dir> room
  <vnum>` form, in both the type-specific spelling and the portable `%teleport%`
  / `%at%` / `%goto%` / `%door%` spelling. A vnum supplied through a script
  variable cannot be resolved and is not counted.
- **Zone flags.** The owning zone is neither `ZONE_CLOSED` nor
  `ZONE_WILDERNESS`. A locked zone is deliberately sealed off, and wilderness
  rooms are entered through the coordinate navigator rather than through static
  exits, so an unreachable room in either is expected. Every room in such a zone
  is skipped, which also suppresses the `SEM010` fallback-root note for it.

Portal and script destinations are full reachability roots, not just
exemptions: rooms walkable onward from them are reachable too.

Room level-range records use two additional stable findings:

| Code | Severity | Contract |
|------|----------|----------|
| `WLD036` | Error | Room `R` record is malformed or repeated. |
| `SEM034` | Error | Room entry level bounds are outside `-1` or `1..LVL_IMPL`, or reversed. |
| `SEM035` | Error | An `ITEM_TRAPPED` RoL compatibility payload in object values 10-15 is invalid. |

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

Version 0.3.0 keeps validation JSON schema version 1 and gives the RoL inventory
its own schema version 1 payload. Quest and HLQ records, references, and
findings remain additive; the payloads for the original six record types are
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

## Version 0.3.0 RoL Inventory Acceptance

The 2026-08-11 development-corpus acceptance result is:

```text
Active zone scope: 252 files, 255 records
Excluded zone files: 30 disabled, 2 unlisted
Active basenames without .zon: 6
Active multi-zone files: 2
Included active companion-only files: 9
```

The missing-zone basenames are `foggy_woods2`, `god_items`,
`northern_highroad2`, `northern_highroad3`, `quest_1`, and `quest_2`.
`foggy_woods.zon` supplies records 900 and 901;
`northern_highroad.zon` supplies records 902, 903, and 904. The nine included
companion-only inputs are three object files and six quest files spread across
those six basenames.

The tracked fixtures cover all five classification labels and malformed
manifest entries. The ignored reference-corpus test locks the counts above
when `EXAMPLE/RealmsOfLuminari` is installed and skips only when that ignored
corpus is absent, such as in a source-only CI checkout.

## Version 0.2.0 Acceptance Snapshot

The quest-system expansion was accepted against the development world on
2026-08-05. The version and JSON contract were:

```text
$ python3 scripts/world/wtool.py --version
wtool 0.2.0
```

JSON output remained at schema version 1 because quest and HLQ records,
references, and findings are additive. The clean tracked gates passed all 170
Python tests through both Make and CMake, all four focused CTest entries, all
399 production-linked CuTests, and the required `make install` step.

The completion audit accepted commit
`954d7c1ece58f24867c1ac5c907eb2cb3a8d7457`. Its complete
[GitHub Actions run](https://github.com/LuminariMUD/Luminari-Source/actions/runs/30960033702)
passed all six jobs: world-data tools, `make test-all`, the supported Luminari
behavioral build, ASan/UBSan plus bounded protocol fuzzing, exact Valgrind, and
coverage. The coverage job passed 399 production tests and 22 protocol tests,
reported 24,785 of 235,600 lines and 15,473 of 215,463 branches, enforced the
stable 10.50-percent line and 7.16-percent branch floors, and uploaded the HTML,
XML, and JSON reports as a GitHub artifact.

The representative development-world lookup commands were:

```sh
python3 scripts/world/wtool.py validate --zone 3
python3 scripts/world/wtool.py show quest 300
python3 scripts/world/wtool.py refs quest 300
python3 scripts/world/wtool.py show hlquest 374
python3 scripts/world/wtool.py refs hlquest 374
```

Zone 3 contained no quest-system finding. The zone command still returned
status 1 because other selected world formats had existing errors. All four
lookups returned status 0: QST 300 had one outgoing edge, and HLQ host 374 had
19. Full-world lookup JSON also reported `lookup_parse_errors: 3849`; a found
record does not imply that unrelated world data is clean.

The hash-guarded `validate --all` audit parsed an inventory of 182 QST files
and 320 HLQ files. It completed in 12.13 seconds at 301,620 KiB peak RSS and
reported 41,468 whole-world findings: 3,849 errors, 37,413 warnings, and 206
info findings. The quest-system subset was 372 findings: 210 errors and 162
warnings. Its exact classification was:

- 57 `IDX008` unindexed-file warnings and one `REF010` package warning;
- one `HLQ014` missing-terminator error and three `HLQ015`
  post-terminator-content errors;
- five `REF032`, 143 `REF034`, and 36 `REF035` missing or wrong-type
  reference errors;
- three `SEM026` required-field errors;
- 40 `SEM028` chain reciprocity warnings;
- 19 errors and 31 warnings under `SEM032`; and
- 33 `SEM033` ignored-parameter warnings.

Every quest-system finding mapped to a source-demonstrated runtime contract or
a builder-owned data issue. No validator defect remained after classification,
and the audit did not repair live data. `validate --mini` reproduced the
expected `IDX009` error for the absent development `hlq/index.mini`.

The path-and-content aggregate hashes were identical before validation and
after validation, lookups, server boot, gameplay, and the final 2026-08-05
completion audit:

```text
QST  9d80ee4d90c360c10d5c4b38eb939516b7928a2fb5cef76a61c8511393ce0655
HLQ  7f1647e1d55c3404c348a3cb967cc6722bb764fcae518fb256e55d1a058b7bfe
```

After all static gates, the development service was restarted on the newly
installed `bin/luminari` and entered the game loop. The final installed and
running executable hashes both equal
`4253a7d2a077e1fadf6d75ff6c6b7bd235bbf162dae8a6cd727da24be25ed3c0`.
QST 300 was listed at its
questmaster, joined, displayed through both progress views, and left. HLQ host
374 answered its stored `hi` and `lumber` ASK entries. A second login confirmed
the test character was back in its original room with all quest slots free and
unchanged quest points.

That playtest proves loader integration, reset availability for the selected
NPCs, questmaster special dispatch, quest queue save/clear behavior, and HLQ
ASK dispatch for those records. It does not prove QST autocraft completion or
rewards, HLQ GIVE/ROOM/output command effects, combat, spell effects, dialogue
rolls, or the correctness of builder-authored text and balance.

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
but not MariaDB or a `luminari` build.

GitHub Actions validates tracked fixtures and the constants/documentation
contracts. The live `lib/world/{wld,mob,obj,zon,shp,trg,qst,hlq}` data is ignored by
Git and is not available to CI, so a green workflow is not a claim that the
complete builder-owned world is clean.
