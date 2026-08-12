# RealmsOfLuminari Reconciliation-First, Feature-First Conversion Plan

- Status: Phase 6 special-procedure reconciliation in progress
- Plan date: 2026-08-11
- Evidence audit: 2026-08-12
- Companion scope:
  [REALMS_OF_LUMINARI_ZONE_CONVERSION_SCOPE.md](REALMS_OF_LUMINARI_ZONE_CONVERSION_SCOPE.md)
- Source corpus root:
  `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/RealmsOfLuminari/`
- Destination: this writable development checkout and its current `lib/world/`

## Decision summary

Treat this as a reconciliation project, not a greenfield import. The current target
already contains earlier Homeland/RoL-lineage content and special-procedure ports.
Blindly shifting every source VNUM into a new range would duplicate content, bypass
existing builder work, and leave current code, database, and world references attached
to the older copies.

The project therefore compares three bodies of evidence:

1. the current RoL authoring corpus and its source runtime;
2. this development checkout's current Luminari world, including prior conversions and
   OLC edits; and
3. the current Luminari runtime, documentation, configuration, and persistent bindings.

For each in-scope source record, assign an explicit target action: keep an equivalent target
record, patch an existing record, add a genuinely absent record, merge overlapping
records, or exclude/disable the smallest irreducibly invalid unit under the locked
behavior policy. Runtime work remains feature-first and usage-driven.

The repeated delivery loop is:

1. trace one source behavior from syntax through its loader and runtime;
2. trace the corresponding target data and runtime behavior;
3. identify any existing target record or earlier port;
4. apply the locked record-action and semantic conversion rules;
5. implement only the missing adapter or runtime capability;
6. emit an isolated, reviewable change bundle;
7. validate structure, references, behavior, idempotency, and no-clobber guarantees; and
8. expand only after a walking skeleton or pilot proves the contract.

This checkout is the authorized writable development target. The implementation may
modify its code and `lib/world/` data directly. Do not create or require backup copies,
snapshots, preimage captures, rollback artifacts, recovery plans, or remote/production
captures for this project. Disposable staging and validation remain correctness tools,
not approval gates.

## Locked implementation decisions

These decisions are final and must not return as questions or approval gates:

1. **Writable environment:** the current checkout is the development port and may be
   modified freely. Conversion reads the current source and target and may write the
   validated result directly to this environment.
2. **Authoritative target:** the current runtime and current `lib/world/` in this checkout
   are the conversion baseline. Preserve existing Luminari/OLC work when equivalent.
3. **Active scope only:** include content assembled by uncommented active source manifests
   and its required active/target dependency closure. Ignore disabled, unlisted, and demonstrably
   non-working RoL content. Do not inventory it beyond the already recorded raw counts,
   reconcile it, convert it, port behavior solely for it, or include it in acceptance.
   An active instruction whose only source dependency is excluded is minimally disabled
   and logged; it does not pull the excluded package into scope.
4. **Behavior policy:** preserve equivalent Luminari behavior; otherwise implement clear
   intended gameplay, repair obvious source defects, and disable/log the smallest
   irreducibly malformed instruction or record. Never reproduce unsafe undefined behavior.
5. **Rights and secrecy:** full rights and access are confirmed. Rights review is out of
   scope. `lib/world/` remains uncommitted to protect player exploration and spoilers,
   not because of a licensing restriction.
6. **Quest availability:** all converted HLQ entries are pre-approved. Emit the canonical
   `!` marker automatically. There is no builder or human approval stage.
7. **Acceptance:** automated structural, behavioral, reset-observation, and scripted
   walkthrough evidence replaces builder sign-off. There are no active builders to audit
   converted content.
8. **Engineering authority:** VNUM placement, SOC implementation, reset representation,
   adapters, and other technical choices are resolved by traced evidence and the tests in
   this plan without returning them as user decisions.

## Evidence that changes the strategy

The target is not empty. Directly traced examples include:

| Source record | Existing target record | Evidence |
|---------------|------------------------|----------|
| Zone `507`, Hulburg Trail | Zone `1507`, Hulburg Trail | Matching zone title and package content |
| Mobile `50789` | Mobile `150789` | Matching normalized identity text |
| Zone `960`, Jotunheim | Zone `1960`, Jotunheim | Matching zone title and package content |
| Object `96001` | Object `196001` | Matching normalized identity text |
| Zone `591`, Hulburg | Zone `1591`, Hulburg | Matching zone title and package content |
| Room `59433` | Room `159433` | Matching normalized identity text |

These examples show an earlier zone `+1000` and entity `+100000` convention in some
packages. They are lineage candidates, not proof that every record follows that rule.
The reconciliation inventory must support one-to-one, one-to-many, many-to-one, and
edited matches.

A non-mutating strict validation of development zone `1507` on 2026-08-11 also reported
pre-existing parse, reference, and semantic findings. Existing lineage proves neither
quality nor `KEEP` eligibility; active defects require `PATCH` or the locked fallback.

The audit also corrected five planning assumptions:

- RoL does not have an `A` reset command. Lines beginning with `A` found by a raw text
  search are zone title lines such as `A Halruaan Airship 1~`, before the reset stream.
- RoL time-predicate `T` and target DG-trigger `T` are different operations.
- The 284 physical `.zon` files contain 287 zone records; package/file identity is not
  always one-to-one with a zone header.
- Current world data is intentionally ignored by Git because OLC edits are authoritative.
  Generated-data review therefore uses manifests, action ledgers, and validation results
  instead of ordinary Git diffs.
- A locally empty candidate VNUM span is evidence at one moment, not a reservation and
  not a reason to ignore existing lineage mappings.

## Non-negotiable invariants

- Repository and world-data writes are authorized throughout implementation.
- Converter analysis and staging precede writes to the development world.
- Existing target records and OLC edits are preserved unless their ledger action
  explicitly assigns a patch, merge, or replacement.
- No record is added merely because its source VNUM can be shifted without collision.
- Every numeric reference is typed before it is mapped.
- Unknown syntax, ambiguous identity, and unresolved required references stop emission
  until the locked repair/fallback rule produces a deterministic result.
- Source behavior is established from code paths, not names or format resemblance.
- No source field or behavior is discarded silently.
- Converter output is deterministic and idempotent for identical inputs and policy.
- Generated artifacts use ASCII text, UTF-8 encoding, and LF line endings.
- Runtime changes follow repository rules, including both build manifests for source
  additions and documentation/help updates for builder- or player-visible behavior.
- Never edit local protected configuration headers (`src/campaign.h`,
  `src/mud_options.h`, or `src/vnums.h`); change an example template only when a
  reviewed template policy actually requires it.
- Credentials and local configuration remain unchanged.

## Evidence boundaries and fixed baseline

### RoL inputs

The physical authoring inputs are:

```text
EXAMPLE/RealmsOfLuminari/areas/AREA
EXAMPLE/RealmsOfLuminari/areas/mob/
EXAMPLE/RealmsOfLuminari/areas/obj/
EXAMPLE/RealmsOfLuminari/areas/qst/
EXAMPLE/RealmsOfLuminari/areas/shp/
EXAMPLE/RealmsOfLuminari/areas/soc/
EXAMPLE/RealmsOfLuminari/areas/wld/
EXAMPLE/RealmsOfLuminari/areas/zon/
```

Per-area files preserve ownership and diagnostics. The generated `world.*` aggregates
are not authoring inputs, but they are useful regression oracles for active membership,
build order, and the source runtime's assembled view. The `AREA` file and
`src/build_areas.c` determine how to interpret that relationship.

Primary source-runtime evidence includes:

```text
EXAMPLE/RealmsOfLuminari/src/db.c
EXAMPLE/RealmsOfLuminari/src/build_areas.c
EXAMPLE/RealmsOfLuminari/src/interp.c
EXAMPLE/RealmsOfLuminari/src/quest.c
EXAMPLE/RealmsOfLuminari/src/shop.c
EXAMPLE/RealmsOfLuminari/src/shop_parallel.c
EXAMPLE/RealmsOfLuminari/src/socials.c
EXAMPLE/RealmsOfLuminari/src/structs.h
EXAMPLE/RealmsOfLuminari/src/specs.assign.c
EXAMPLE/RealmsOfLuminari/src/specs.*.c
```

Add every crossed call path to the evidence ledger. Naming similarity is never enough.

### Luminari development baseline

The current checkout's runtime and `lib/world/` are the fixed authoritative development
target. Writes are authorized. Each conversion run records source and target inventories
with content hashes for provenance and deterministic diagnostics; those inventories do
not copy data or restrict writes.

Most world records and the RoL reference tree are ignored by Git. This is intentional:
the live world is hidden to preserve exploration and prevent spoilers. The run manifest,
reconciliation ledger, output bundle, and validation report provide review evidence
instead of ordinary Git diffs.

### Target-runtime evidence

At minimum, trace the current loaders, reset executor, OLC writers, shops, HLQ, DG
scripts, special-procedure assignments, command table, traps, persistent database
references, and world validation tools. Historical comments and commits can establish
lineage, but current runtime code determines current behavior.

## Required project artifacts

### 1. Input inventory manifests

Create separate source and target manifests containing:

- run identifier, creation time, and tool version;
- root identity without embedding credentials;
- every included path, byte size, and cryptographic hash;
- active manifest membership and dependency-closure status;
- target index membership and missing/orphaned files;
- exact baseline validator commands, output, finding counts, and parse-completeness state;
- relevant source/runtime revision identifiers; and
- the locked active-scope and behavior-policy version.

Regenerate the applicable inventory when an input changes.

### 2. Grammar-aware corpus inventory

Inventory every construct observed in the active manifest/dependency closure:

- record version, extension, optional directive, and terminator;
- physical package membership and every contained record boundary;
- flag, enum, bit vector, and value-array position;
- zone header variant and reset operation;
- shop keyword and AI extension;
- quest input, output, reward, and action direction;
- SOC mode, action code, chance, delay, flag, path, and command identity;
- special-procedure binding; and
- typed reference to a room, mobile, object, zone, shop, trigger, command, spell,
  skill, quest host, or other entity.

Record counts and exact locations. Parsers must distinguish file headers, titles,
comments, strings, and command streams; the false `A` reset is a mandatory negative
fixture.

### 3. Target-lineage and record-action ledger

Each source record receives one action:

| Action | Meaning |
|--------|---------|
| `KEEP` | An evidence-confirmed target record provides the intended identity and behavior. |
| `PATCH` | Preserve the target identity and local edits while applying a bounded change. |
| `ADD` | No acceptable target record exists; create a new mapped record. |
| `MERGE` | Multiple source or target records require a deterministic combined result. |
| `EXCLUDE` | The smallest invalid active unit is safely disabled under the locked policy. |

The ledger must include source path/type/VNUM/hash, all target candidates and hashes,
match evidence and confidence, local-edit evidence, selected action, destination VNUM,
dependencies, and rationale. Filename or text similarity may nominate a candidate but
cannot justify an automatic overwrite.

`BLOCKED` is not a final record action. Investigation must resolve ambiguity through
evidence; if no stable behavior exists, apply the locked smallest-unit `EXCLUDE` fallback
and emit a high-severity diagnostic. Disabled, unlisted, and non-working source content
never enters this ledger.

### 4. Behavior capability matrix

Keep behavior classification separate from record action:

| Code | Classification | Meaning |
|------|----------------|---------|
| `N` | Native | Current Luminari provides equivalent behavior. |
| `T` | Transform | Existing behavior is equivalent after a data or argument transformation. |
| `A` | Adapter | A bounded compatibility layer can preserve the used source semantics. |
| `P` | Port/new mechanic | Active content requires new target runtime behavior. |
| `B` | Bug/fallback | Apply the locked repair or smallest-unit disable policy. |
| `X` | Minimal exclusion | Disable/log the smallest irreducible active unit. |

`A` in this table means adapter; it is not a reset opcode. "Dead," "unused," and
"source no-op" are evidence tags. A `B` row follows the fixed order: keep equivalent
Luminari behavior, implement clear source intent, repair an obvious defect, or disable
the smallest unsafe unit and log it. It does not wait for product approval.

Each row records source syntax and runtime evidence, target evidence, occurrence count,
consuming records, classification, semantic contract, converter and runtime IDs,
fixtures, acceptance tests, and loss policy.

### 5. Typed reference graph

Build definitions and references before emission. Typical edges include:

- room exit -> room;
- reset -> room/mobile/object/trigger;
- object content -> object;
- shop -> shop/keeper/room/product;
- HLQ block -> host/item/mobile/reward;
- SOC action -> mobile/command/path;
- special procedure -> assigned entity and dependent records; and
- code/config/database binding -> world entity.

Typed namespaces may legally reuse a number, so collision and resolution checks must
not flatten all VNUMs into one namespace. External references need explicit ownership;
required unresolved edges block emission.

### 6. Identity and provenance manifest

Resolve identity in this order:

1. an evidence-confirmed existing target-lineage mapping;
2. a deterministically resolved merge or explicit exception mapping;
3. a reserved allocation for a genuinely new or still-ambiguous active record; and
4. smallest-record `EXCLUDE` with a high-severity diagnostic if no safe identity exists.

The previously proposed allocation remains only a candidate for new records:

```text
CANDIDATE_NEW_ENTITY_OFFSET = 2000000
CANDIDATE_NEW_ZONE_OFFSET   =   20000
```

Its assessed span was empty and reserved against the development target on 2026-08-11.
Revalidate it before every batch and again at apply time.
Collision discovery must cover world records, zone ranges, hardcoded assignments,
configuration, and persistent database references that participate at runtime.

Special identity rules are:

- Preserve evidence-confirmed existing room, mobile, object, zone, shop, and trigger
  identities.
- New rooms, mobiles, objects, and zones may use the candidate offsets only after
  reservation and full typed-reference validation.
- A RoL `.qst` header identifies a host mobile; target HLQ remains attached to the
  resolved host mobile rather than receiving a fictitious independent quest VNUM.
- RoL `SHOP:` identifies the keeper. Preserve an existing shop identity when present;
  otherwise the resolved keeper VNUM is a possible deterministic shop VNUM in the
  separate shop namespace, subject to collision checks.
- If SOC compilation creates DG triggers, allocate deterministic trigger identities
  from an explicitly reserved, OLC-valid range owned by the resolved destination zone.
  Record all synthetic provenance.

Every mapping records source and target identity, record type, hashes, rule, confidence,
exception, and all records that consume it. The manifest, not an offset formula, is the
canonical mapping.

### 7. Conversion contract registry

For every supported construct, record its parser grammar, normalized intermediate
representation, capability IDs, target emitter behavior, required runtime version,
validation rules, source-order semantics, and reversible diagnostics. An emitter may
not handle a construct without a resolved record action and capability disposition.

### 8. Acceptance and conversion-run ledger

Each conversion run records converter revision/build ID, input manifest hashes, policy
and mapping versions, selected records, emitted actions, output hashes, diagnostics,
test evidence, engineering resolutions, and superseded run ID. This replaces the false
assumption that ignored world data will yield useful Git diffs.

## Initial capability hypotheses

| RoL construct | Traced target situation | Initial disposition |
|---------------|--------------------------|---------------------|
| Prior converted records | Target content exists | Reconcile records; no capability code |
| Room/mobile/object formats | Same concepts; different grammar | `T` by grammar/value family |
| Base `M/O/P/G/E` resets | Native operations differ | `T` after chain tracing |
| Basic door `D` states | Native door reset support | `T` for proven equivalent states |
| Extended/chance `D` | Source bitmask/chance differs | `A`, `P`, or `B` by used case |
| Unconditional `R` | Native remove-object reset | `T` |
| Chance-qualified `R` | Target reset lacks source chance field | `A` or `P` |
| Follow `F` | No equivalent target opcode | `A` or `P` |
| Removal `X` | No equivalent target opcode | `A` or `P` |
| Time-predicate `T` | Target `T` attaches a trigger | `A` or `P`; never direct mapping |
| Conversational `.qst` | Target HLQ is not identical | Per-direction `N/T/A/P/B` |
| Five `.soc` modes | No like-for-like target kind | Measured native vs. DG selection |
| Room extensions/dimensions | Partial or absent | `T`, `P`, `B`, or locked `X` |
| Object traps | Multiple current trap representations | `T` or `A` after end-to-end audit |
| Special procedures | Prior ports plus gaps | Reuse/patch first; `A` or `P` for gaps |
| Numeric SOC command IDs | Command indexes differ | `T`: source ID -> name -> target action |
| Legacy color markup | Target markup differs | `T` with a token-aware lexer |

This is a hypothesis table. Rows become authoritative only after code-path evidence,
occurrence inventory, and fixtures support them.

## High-risk semantic contracts

### Zone resets

The source reset grammar includes `M/O/P/G/E/D/R/F/X/T`; it does not include `A`.
The target reset grammar includes `M/O/G/E/P/D/R/T/V/J/I/L`, where target `T` attaches
a DG trigger.

The source corpus also contains two loader-accepted no-space `F2 ...` rows and two
un-commented headings (`GROUPING*` and `GATE QUEST STUFF`) that the first-character
source dispatcher misreads as malformed `G` commands. Preserve all four as explicit
grammar/source-defect fixtures; do not silently normalize or execute the malformed rows.

The converter must preserve effective execution semantics, not copy opcode letters or
argument positions. In particular:

- source `G` chance and target `G` arguments occupy different positions;
- source `D` uses a state/extension bitmask and may include chance, while target `D`
  uses a different state enumeration and has no equivalent chance argument;
- source `R` may be chance-qualified, while target `R` is unconditional;
- source `F` overloads the source `if_flag` field as follow mode;
- source `X` removes mobiles in a room or globally and has combat/chance behavior;
- source `T` is a calendar predicate, not trigger attachment.

Every used variant needs an executable source fixture and expected target outcome.

### High-level quests

Compile quest behavior direction by direction. Required contract rows include:

- ask/topic matching and output ordering;
- item input, item-type input, and coin input;
- item, coin, random-object-range, prestige, and experience rewards;
- load mobile/object, attack, disappear, teach, and other action outputs;
- duplicate source host headers and effective source lookup behavior;
- source list-prepend/reversal behavior versus physical file order;
- target list ordering and duplicate host-block behavior; and
- unconditional emission of the target `!` marker for mortal availability.

Known gaps or ambiguities must not be hidden by calling all `.qst` records a transform.
For example, source experience reward code is present but intentionally does not award
experience, item-type input lacks a direct target equivalent, random object ranges need
a policy, and source/target coin and disappearance behavior are not identical. Duplicate
host headers follow the locked behavior policy: preserve equivalent Luminari content,
otherwise merge distinct intended content and remove only proven duplicates.

Every emitted entry is canonical and pre-approved with `!`. There is no builder review
state or later approval batch.

### SOC action lists

The source implements five modes: `LIST`, `PATH`, `PERIODIC`, `TIMED`, and `TRIGGER`.
The audit observed 410, 33, 1099, 11, and 205 headers respectively; regenerate these
counts from the current active source inventory. Special action codes for indoor, outdoor, all-zone,
room, and path behavior require explicit semantic fixtures.

Prototype both choices on representative data:

- a native compatibility subsystem retaining source mobile/mode/list semantics; and
- a DG compiler using deterministic, zone-owned trigger prototypes plus any bounded
  helpers needed for path and zone-echo behavior.

Compare fidelity, OLC ownership and maintainability, observability, performance,
generated-record volume, and testing cost before choosing.

### Special procedures

Search current assignments, implementations, comments, and history before porting.
Existing target procedures may be equivalent, locally evolved, partially ported, or
attached to already converted identities. Reuse or patch those first. Port only behavior
consumed by active records and only after tracing dependent commands, spells, objects,
rooms, globals, persistence, and scheduler assumptions.

## Converter and release architecture

```text
Current RoL inputs ----------+
                              |
                              v
                       Typed source IR ------> Capability inventory
                              |
Current target inventory --->+--> Lineage matcher --> Record-action ledger
                              |                         |
Current target runtime ------+--> Contract registry ---+
                                                        |
                                                        v
                                            Identity/reference resolver
                                                        |
                                                        v
                                             Isolated change bundle
                                           /        |         \
                                  KEEP evidence   PATCHes     ADDs
                                           \        |         /
                                                        v
                                             Staging and parity checks
```

The converter is standalone and deterministic. It reads the current source and target
inventories. Inventory and reconciliation run before emitters. Output first goes to a
unique run directory for validation; an apply step may then update the writable
development world directly.

A release bundle contains at least:

```text
run-manifest.json
source-inventory.json
target-inventory.json
reconciliation.jsonl
capabilities.jsonl
identity-map.jsonl
reference-report.json
change-plan.jsonl
output/world/
validation/
```

Exact names may change in the implementation spec, but equivalent machine-readable
evidence is required. Human-readable summaries are derived from canonical records.

Application is a separate operation. It applies only planned paths, rebuilds indexes when
required, and validates the result. Never overwrite an OLC-edited record based only on
its path or VNUM; any such change requires an explicit `PATCH` or `MERGE` action in the
record ledger.

The likely implementation home is a dedicated subtree under `scripts/world/`. Confirm
language, packaging, fixtures, and build integration in the implementation spec before
creating files.

## Vertical capability unit

Each runtime/converter feature lands as one reviewable unit containing:

1. traced source and target behavior;
2. affected record-action and capability rows;
3. normalized model support;
4. target runtime changes, if required;
5. deterministic conversion and diagnostics;
6. positive, negative, and ambiguity fixtures;
7. unit/integration tests;
8. one walking-skeleton or pilot demonstration; and
9. documentation/help updates when behavior is builder- or player-visible.

Do not accumulate a long-lived mechanics branch or generate the whole corpus before
the supporting runtime lands.

## Phased session plan

One session is one 2-4 hour spec with one objective and 12-25 tasks. Session ranges
below are evidence-based planning envelopes, not delivery promises. The measured
Phase 4 pilot replaced the provisional ranges. Twenty-four completed Phase 6 delivery
sessions are archived in the changelog. The remaining Phases 6-8 forecast is 72-132
sessions, or 144-528 focused engineering hours at the defined session size.

Phases 0-5 are complete and have been removed from this active plan. Their delivered
scope, run identities, counts, acceptance evidence, commits, and reforecast basis are
recorded in [RoL-Changelog.md](RoL-Changelog.md). Active implementation continues with
the remaining Phase 6 families.

### Phase 6: Special-procedure reconciliation (24-56 remaining sessions)

Process by shared behavior family and consuming package. Reuse equivalent target ports,
patch bounded differences, adapt native systems, and port only remaining selected
behavior. A new source file must be added to both `Makefile.am` and `CMakeLists.txt`.

Exit gate: every active binding is kept, patched, adapted, ported, or minimally excluded
under the locked malformed-content rule, with behavioral evidence.

### Phase 7: Action-based corpus batches (42-66 sessions)

Batch by dependency closure and record actions, not by blindly adding 5-10 numbered
zones. Each batch includes deterministic regeneration, before/after hashes, capability
coverage, identity/reference validation, target syntax checks, isolated boot/reset logs,
scripted walkthrough evidence, and a validation bundle.

Reconcile only the active working dependency closure. The current evidence identifies
252 active physical zone files containing 255 zone records, plus active companion-only
data selected by the source build lists. The 30 disabled and 2 unlisted physical files,
and other demonstrably non-working content, are permanently out of scope.

Exit gate: every in-scope package has a disposition, every contained record has a
resolved action, and every active companion-file mismatch has evidence.

### Phase 8: Integration and release evidence (6-10 sessions)

1. Validate a complete isolated target assembled from the development baseline and
   planned bundles.
2. Run full structural, syntax-boot, behavioral, and regression gates.
3. Reconcile counts, identities, references, capabilities, exclusions, and output content.
4. Finalize builder, operator, testing, conversion, and apply documentation.
5. Integrate runtime dependencies before their data bundles.
6. Apply the validated bundles directly to the writable development world.

Exit gate: the chosen parity level is supported by reproducible evidence.

## Priority model

Rank work using measured evidence:

```text
priority = pilot blocking + dependency fan-out + corpus frequency + reuse value
```

Do not automate the ranking. A rare construct can be first if it protects identity,
blocks safe staging, or affects many downstream references. Prefer proving reusable
existing work over implementing a duplicate subsystem.

## Locked engineering resolution rules

### Environment and application

This development checkout is writable and authoritative for the project. Every apply
may write its validated plan directly to the current files. The project has no backup,
snapshot, preimage, rollback, recovery, compare-and-swap, remote-capture, or
production-capture requirement. Disposable staging exists only to test converter output.

### Lineage confidence

Use separate evidence thresholds for candidate generation, `KEEP`, and bounded `PATCH`.
Exact VNUM formulas or matching display text alone are insufficient. If lineage remains
ambiguous after traceable evidence, preserve every target candidate untouched and `ADD`
the required active source record in the reserved range. Never pause for builder review
or destructively guess.

### Identity allocation

Evidence-confirmed existing mappings win. Reserve and revalidate new ranges only for
true `ADD` records. Typed world, code, configuration, synthetic-trigger, and relevant
database references all participate in collision checks. Exceptions are deterministic
manifest entries, not approval items.

### Reset semantics

Implement contracts for extended/chance `D`, chance `R`, follow `F`, removal `X`, calendar
`T`, and conditional-chain behavior according to intended gameplay. Malformed commands
use the smallest-unit disable/log fallback. There is no source `A` reset.

### Quest semantics

All converted entries are emitted pre-approved with `!`. Merge distinct intended content
from duplicate host blocks, remove only proven duplicate entries, implement configured
experience instead of preserving the source no-op bug, preserve exact coin costs, and
implement item-type input, random rewards, disappearance, and ordering semantics through
target transforms or bounded adapters. Irreducibly malformed directions are individually
disabled and logged; the host block continues when safe.

### SOC implementation

Pilot both native compatibility and DG compilation. Select the result with higher tested
fidelity, OLC maintainability, observability, and lower synthetic-record burden. This is
an engineering measurement, not a user decision or pause.

### Special procedures

Only active dependency-closure consumers justify work. Trace dependencies and search the
current target for reuse before any port. Disabled/unlisted-only procedures are ignored.

### Loss and source defects

Preserve evidence for every repair. Apply the fixed order: equivalent Luminari behavior,
clear intended source behavior, obvious repair, then smallest-unit disable with a
high-severity diagnostic. Every `B` or `X` row lists affected active records, impact,
evidence, and the automatic resolution. No builder or product approval is required.

## Validation strategy

### Inventory and contract validation

- Every token is grammar-classified and counted.
- Every grammar variant has positive and negative fixtures.
- Every definition/reference is typed.
- Every active source record has a final record action.
- Every capability row cites source and target runtime evidence.
- Aggregate-source oracles reconcile with active per-area inputs.

### Reconciliation and conversion validation

- Evidence-confirmed existing identities win over candidate offsets.
- Every new allocation is reserved and collision-free in all relevant typed stores.
- Every required reference resolves to the manifest identity; an irreducibly malformed
  dependent instruction is disabled and logged under the fixed fallback.
- Source, target, and output record counts reconcile by action.
- No diagnostic is new relative to the recorded target baseline.
- Findings attached to selected or touched records are resolved; they cannot be waived
  merely because they predate the conversion.
- Untouched target paths retain their recorded hashes.
- Repeated conversion produces byte-identical bundles.
- Repeated application is a no-op or is safely rejected by an explicit precondition.
- Output indexes, versions, terminators, ASCII text, and LF endings are valid.

### Target structural validation

For an isolated bootable root `<isolated-lib-root>`:

```bash
python3 scripts/world/wtool.py \
  --world-root <isolated-lib-root>/world validate --all --strict

lib/world/validate-zone.sh <zone-vnum> \
  --world-root <isolated-lib-root>/world --strict
```

The `validate` subcommand requires an explicit selector such as `--all`; omitting it is
not a valid project gate.

Run and record the same commands against the current target before staging changes.
Compare finding identity and parse completeness, not only process exit status. The
current development wrapper can report errors while returning success, so automation
must assert the parsed summary. A batch must add no finding; selected or touched records
must have no unresolved errors. Unrelated baseline findings remain outside this active
conversion scope.

### Runtime validation

Run the fast syntax/world boot before behavioral boots:

```bash
bin/circle -c -d <isolated-lib-root>
```

For full behavior, use an isolated lib root and isolated MariaDB test instance. Reuse or
adapt `scripts/ci/prepare_test_runtime.sh` and its safety checks rather than using the
development database for destructive fixtures. Boot logs must have no relevant invalid
record, reset, trigger, reference, or `SYSERR` diagnostics.

Behavioral fixtures cover resets, shops, quests, SOC, traps, and shared special
procedures. Pilots and batches require automated reset observation and scripted
walkthroughs; there is no human builder sign-off.

### Code integration validation

When runtime code changes:

```bash
make test
make install
```

Verify that `make install` removes any root-level `circle` artifact. Runtime capabilities
must land before bundles that depend on them, and both supported build manifests must
contain every source-file addition/removal.

### Development application validation

- Assemble the planned output in the isolated test root.
- Re-run structural and runtime gates on the assembled result.
- Apply the validated plan directly to the writable development world.
- Re-run structural checks against the resulting development world.

## Definition of done

The locked active conversion scope is complete only when:

1. source and development-target inventories reproducibly identify the run inputs;
2. every in-scope source record has a final `KEEP/PATCH/ADD/MERGE/EXCLUDE` action;
3. every observed in-scope behavior has a final capability disposition;
4. every identity and required reference resolves through the canonical manifest;
5. every existing target edit is preserved or changed by an explicit manifest action;
6. every required runtime gap has tested target behavior;
7. selected/touched records have no unresolved validation errors, the assembled target
   adds no baseline diagnostic, and all output passes syntax-boot, behavioral,
   idempotency, and no-clobber validation;
8. every source repair or smallest-unit disable is explicit and tested;
9. every in-scope package has acceptance evidence; and
10. the result can be regenerated and safely re-applied to this development checkout
    from the recorded inputs and versioned policy.

No unresolved decision or `BLOCKED` action is permitted at completion. Engineering must
trace it to a final action or apply the locked smallest-unit fallback.
