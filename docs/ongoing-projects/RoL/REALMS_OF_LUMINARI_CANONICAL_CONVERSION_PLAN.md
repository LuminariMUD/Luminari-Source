# Realms of Luminari Canonical Conversion Plan

- Status: Phase 6 corrected binding reconciliation in progress; Phase 6.5 is planned
- Consolidated: 2026-08-13
- Source corpus: `EXAMPLE/RealmsOfLuminari/`
- Target: this writable development checkout and its current `lib/world/`
- Production changes: prohibited
- Supersedes the active planning content in:
  - [Feature-first plan](plan-archive/REALMS_OF_LUMINARI_FEATURE_FIRST_CONVERSION_PLAN.md)
  - [Phase 6.5 VNUM rebase plan](plan-archive/PHASE6_5_CANONICAL_VNUM_REBASE_PLAN.md)
  - [Zone conversion scope](plan-archive/REALMS_OF_LUMINARI_ZONE_CONVERSION_SCOPE.md)
- Completed history and measured evidence:
  [RoL-Changelog.md](RoL-Changelog.md)

## 1. Objective and strategy

Convert the active, working Realms of Luminari (RoL) dependency closure into a
structurally valid and functionally playable Luminari world. Completion includes rooms,
mobiles, objects, shops, high-level quests (HLQs), automated mobile actions (SOC), zone
resets, traps, and every special procedure required by active content.

This is a reconciliation and migration project, not a file copy. The project compares:

1. the active RoL authoring corpus and its runtime;
2. the current Luminari development world, including prior conversions and OLC edits;
   and
3. the current Luminari runtime, configuration, documentation, and persistent state.

For each source record, the converter preserves useful target content, resolves one
final record action, and emits the result at the canonical RoL identity. Runtime work is
feature-first: trace one used behavior, reuse or extend the target implementation, add
tests, then convert the content that consumes it.

The delivery loop is:

1. trace source syntax through loader and runtime behavior;
2. trace the corresponding target data and runtime;
3. reconcile current target lineage and local edits as content evidence;
4. assign a record action and capability disposition;
5. resolve all identities and references through typed manifests;
6. implement only the missing transform, adapter, or mechanic;
7. emit an isolated deterministic bundle;
8. validate structure, behavior, references, preservation, and idempotency; and
9. expand only after a pilot or earlier batch proves the contract.

### 1.1 Canonical identity policy

Phase 6.5 changes the earlier identity policy. Existing target lineage still wins as
content and local-edit evidence, but it no longer wins as the destination VNUM for an
RoL-owned record. The final identity rules are:

```text
target zone VNUM   = normalized source zone VNUM + 20000
target entity VNUM = source entity VNUM + 2000000
```

Rooms, mobiles, and objects are entities. Shops, quests, and SOC remain attached to
their canonical keeper or host mobile. Synthetic DG triggers receive deterministic
zone-owned identities. External Luminari-owned dependencies keep their native VNUMs.

The mapping manifest remains authoritative for provenance, type checking, merges,
external dependencies, and reference closure. It may not replace the formulas with
legacy aliases or arbitrary exceptions.

This makes every expected RoL identity calculable. After Phase 6.5, a surviving source
VNUM, old `+1000`/`+100000` target VNUM, or other RoL destination exception is an error
to classify rather than a possible intentional alias.

## 2. Locked decisions and invariants

These decisions are final and are not implementation-time approval gates:

1. **Development only.** This checkout and its current `lib/world/` are the writable,
   authoritative development target. Never change production code, world data,
   services, or databases.
2. **No recovery artifacts.** Do not create or require backups, snapshots, preimages,
   rollback plans, compare-and-swap gates, or remote/production captures. Disposable
   staging and deterministic regeneration are correctness tools.
3. **Active scope only.** Include content selected by uncommented source manifests and
   its required active or target dependency closure. Disabled, unlisted, and
   demonstrably non-working source content receives no inventory beyond recorded raw
   counts, reconciliation, conversion, runtime port, acceptance work, or placeholder
   record merely to fill the canonical formula.
4. **Minimal dependency exclusion.** If an active instruction depends only on excluded
   source content, disable and log that instruction; do not pull the excluded package
   into scope.
5. **Behavior order.** Preserve equivalent Luminari behavior; otherwise implement clear
   intended gameplay, repair an obvious source defect, or disable and log the smallest
   irreducibly malformed unit. Never reproduce unsafe undefined behavior.
6. **Canonical RoL namespace.** Every active RoL-owned zone, room, mobile, and object
   uses the formulas in Section 1.1. Old and canonical copies may not remain active
   together, and permanent runtime VNUM aliases are prohibited.
7. **Content preservation.** Existing target records and OLC edits remain authoritative
   content evidence. Change them only through an explicit, evidence-backed `PATCH`,
   `MERGE`, replacement, or Phase 6.5 `REHOME` operation.
8. **Distinct identities.** Distinct active typed source VNUMs remain distinct at the
   target. Duplicate definitions of the same typed source VNUM may merge. Shared code
   or data profiles do not justify collapsing prototypes.
9. **Typed references.** Type every numeric reference before mapping it. Blind numeric
   search-and-replace is prohibited. Unknown syntax, ambiguous ownership, collisions,
   and unresolved required edges stop emission until the fixed repair or exclusion
   policy produces a deterministic result.
10. **Quests.** Every converted HLQ entry is pre-approved and receives the canonical
    `!` marker. There is no builder or human approval stage.
11. **Acceptance.** Automated structural tests, behavior tests, reset observation, and
    scripted walkthroughs replace builder sign-off.
12. **Rights and secrecy.** Full content rights and access are confirmed. Rights review
    is closed. World data is ignored by Git to protect exploration and spoilers, not
    because of licensing limits.
13. **Determinism.** Identical source, target, policy, and tool inputs produce
    byte-identical bundles. Reapplication is a no-op or fails a documented safe
    precondition.
14. **Repository rules.** Generated artifacts are ASCII text encoded as UTF-8 with LF
    endings. Runtime changes update tests, documentation/help, and both build manifests
    when source files are added or removed.
15. **Protected files.** Never edit `src/campaign.h`, `src/mud_options.h`, `src/vnums.h`,
    `lib/mysql_config`, or `lib/.env`. If policy requires a shipped configuration
    change, edit the appropriate example template.
16. **No unresolved final state.** `BLOCKED` is not a final record action. Investigation
    must produce a final action or the locked smallest-unit exclusion with a
    high-severity diagnostic.
17. **Engineering authority.** Resolve reset representation, SOC implementation,
    adapters, VNUM mechanics, and other technical routing from traced evidence and
    tests; do not return them as user or builder decisions.

## 3. Evidence boundary and measured baseline

### 3.1 Authoring inputs

The converter reads per-area active inputs so ownership, provenance, and diagnostics
remain package-specific:

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

Top-level `world.*` files are generated aggregates, not authoring inputs. Use them as
regression oracles for active membership, build order, and the source runtime's
assembled view. `areas/AREA` and `src/build_areas.c` define that relationship.

Primary source-runtime evidence includes `db.c`, `build_areas.c`, `interp.c`, `quest.c`,
`shop.c`, `shop_parallel.c`, `socials.c`, `structs.h`, `specs.assign.c`, and every
crossed `specs.*.c` call path. Names and file-format resemblance are not behavioral
evidence.

On the target, trace at least the current loaders, reset executor, OLC writers, shops,
HLQ, DG scripts, special-procedure assignments, command table, traps, persistent
bindings, and world validators. Current runtime code controls behavior; history and
comments provide lineage evidence only.

### 3.2 Raw corpus and active scope

The complete source corpus is about 45.9 MB and 1.73 million lines:

| Kind | Files | Bytes | Lines | Raw parsed records or structures |
|------|------:|------:|------:|--------------------------------:|
| `mob` | 260 | 6,448,241 | 200,972 | 13,505 mobiles |
| `obj` | 214 | 3,605,542 | 152,621 | 10,555 objects |
| `qst` | 261 | 2,390,960 | 81,688 | 5,081 blocks; 5,042 unique hosts |
| `shp` | 76 | 337,944 | 11,705 | 458 shops |
| `soc` | 91 | 703,602 | 32,554 | 1,758 lists; 4,284 actions |
| `wld` | 284 | 29,506,576 | 1,169,654 | 54,037 rooms; 54,019 unique VNUMs |
| `zon` | 284 | 2,931,956 | 84,134 | 287 records; 279 unique headers |
| **Total** | **1,470** | **45,924,821** | **1,733,328** | |

The locked active scope is smaller:

- 252 active physical `.zon` files containing 255 zone records;
- active companion-only data selected by source build lists and dependency closure;
- 30 disabled and 2 unlisted physical zone files permanently excluded; and
- six active manifest basenames without a same-named `.zon`, handled from build lists,
  aggregates, multi-header ownership, and references rather than by inventing records.

Physical packages and zone records are not one-to-one. The full corpus has 287 zone
records in 284 files; the active scope has 255 records in 252 files. Extra active
headers occur in `foggy_woods.zon` and `northern_highroad.zon`. Companion coverage is
also uneven: all 284 physical basenames have `wld` and `zon`, but only 258 have `mob`,
210 have `obj`, 255 have `qst`, 76 have `shp`, and 91 have `soc`; kind directories also
contain a few additional basenames.

### 3.3 Collision and defect evidence

Unshifted source VNUMs are unsafe. Raw-source collision upper bounds against the
assessed target were:

| Namespace | Unique source | Unique target | Same-number collisions |
|-----------|--------------:|--------------:|-----------------------:|
| Mobiles | 13,505 | 14,679 | 733 |
| Objects | 10,555 | 12,321 | 1,225 |
| Rooms | 54,019 | 54,390 | 17,191 |
| Zone headers | 279 | current zone table | 102 |

Most are unrelated records. Only 8 colliding mobiles and 12 colliding objects shared a
normalized primary name; all 102 colliding zone headers had different normalized
titles. Source zone `0`, for example, is `God Rooms`, while target zone `0` is
`Builder Academy`.

Known raw ambiguities include:

- rooms `45100-45117` duplicated in `dwarven_mines.wld` and `flesh.wld`;
- 39 duplicate quest-host headers;
- reused zone headers `466`, `509`, `757`, `903`, and `919`, with `466` in five files
  and nested `903` colliding with `mirar_ferry.zon`; and
- the malformed `mytheast.zon` header described in Section 4.2.

Only active instances require resolution. Resolve them through deterministic ownership,
merge, split, or minimal exclusion; never by guessed identity.

### 3.4 Existing lineage and validation baseline

The target is not empty. Verified examples of older conversions are:

| Source | Legacy target | Evidence |
|--------|---------------|----------|
| Zone `507`, Hulburg Trail | Zone `1507` | Matching title and package content |
| Mobile `50789` | Mobile `150789` | Matching normalized identity text |
| Zone `591`, Hulburg | Zone `1591` | Matching title and package content |
| Room `59433` | Room `159433` | Matching normalized identity text |
| Zone `960`, Jotunheim | Zone `1960` | Matching title and package content |
| Object `96001` | Object `196001` | Matching normalized identity text |

These records prove lineage and possible local improvement, not final identity or
quality. Phase 6.5 rehomes their content to canonical VNUMs. Pre-existing validation
findings, including findings previously observed in zone `1507` and 79 active findings
in the five-package staged baseline, are not waived: a selected or touched record must
be repaired in its owning batch, while unrelated findings remain baseline evidence.

Current world data and the RoL reference tree are intentionally ignored by Git. Review
therefore relies on hashed inventories, action ledgers, isolated output, and validation
reports instead of ordinary Git diffs.

### 3.5 Phase 6 discovery correction

Source special behavior spans 80 `specs.*.c` files and about 89,167 lines. Historical
raw scans found at least 926 direct mobile, 282 direct object, and 354 direct room
assignments plus helper-driven registrations, and about 2,769 three-to-six digit
numeric literals requiring type classification. These figures show scale, not active
completion denominators.

The repaired Phase 1 extractor follows the active boot path through all 53 reachable
registration wrappers, preserves each call path, honors the checked-in preprocessor
configuration, and resolves all 38 active numeric VNUM macros. The regenerated Phase 6
ledger measures:

- 1,813 static candidates, of which 92 are preprocessor-excluded and 1,721 are live;
- 1,098 mobile, 323 object, and 300 room bindings across 795 direct handler names;
- 1,606 resolved and 115 pending live static bindings, with 698 resolved and 97 pending
  direct handler names;
- two resolved dynamic paths representing 5,078 quest and 453 shop binding instances;
- 7,252 total active static and dynamic binding instances across 797 handler names; and
- 820 resolved and 28 pending records in the independent 848-record `ACT_SPEC`
  cross-check.

The corrected dependent evidence is Phase 1 run `rol-phase1-237602d3ade48138`, Phase 2
run `rol-phase2-c93b8c4610b36d1e`, Phase 5 run
`rol-phase5-audit-cec58661a4f21a2a`, and Phase 6 run
`rol-phase6-special-ccb5be8a975f9981`. The older 1,112/1,147 binding, 538/562 handler,
and 830/848 `ACT_SPEC` counts are historical checkpoints only.

## 4. Canonical identity and rehome contract

### 4.1 Zones and entities

For a normal source zone:

```text
canonical target zone = source zone + 20000
canonical target room = source room + 2000000
canonical target mob  = source mob  + 2000000
canonical target obj  = source obj  + 2000000
```

The active normal source zone range reaches `9999`, which maps to `29999`; reserve
target zone range `20000-29999`. The source kind remains part of an entity key, so a
room, mobile, and object may legally share a numeric VNUM in separate namespaces.

The target entity span `2000000-2999999` was empty in the locally assessed world on
2026-08-11. That is dated evidence, not a reservation. Phase 6.5 must reserve and
revalidate both canonical ranges against world data, code, configuration, synthetic
triggers, and relevant persistent state. A collision fails the canonical contract; it
does not authorize an exception VNUM.

A zone record may cover multiple 100-VNUM entity bands. Shift its top by the entity
offset while deriving its zone identity from the normalized source zone. Example:

```text
source Hulburg: #591, top 59599
target Hulburg: #20591, top 2059599
covered bands:  591-595 -> 20591-20595
```

Preserve sparse layout and internal spacing.

### 4.2 `mytheast.zon` normalization

`mytheast.zon` is not logical zone `81700`. Evidence shows:

- `areas/AREA` assigns `mytheast` to logical zones `817,818`;
- its header is incorrectly `#81700` with top `81899`;
- contained rooms, mobiles, objects, and resets use entity VNUMs `81700-81899`; and
- neighboring Myth Drannor packages use the same entity-scale convention.

Normalize its zone identity explicitly:

```text
source logical zones: 817-818
target zone record:    20817
target entities:       2081700-2081899
target top:            2081899
```

Do not retain fallback zone `20002`, allocate `101700`, widen a range to hide the
error, or generalize an unverified divide-by-100 rule. A future normalization requires
the same manifest, declared-range, contained-record, and typed-reference evidence.

### 4.3 Attached and synthetic identities

- Resolve a source `SHOP:` keeper through the canonical mobile formula; validate the
  separate target shop namespace independently. Preserve an existing shop identity
  when present; otherwise the resolved keeper VNUM is a possible deterministic shop
  identity, subject to typed collision checks.
- Treat a source quest header as an HLQ host mobile, not an independent quest VNUM.
- Attach SOC behavior to the canonical mobile.
- Resolve every room, mobile, and object embedded in generated DG behavior through the
  typed canonical manifest.
- Allocate synthetic DG trigger VNUMs deterministically in a collision-free, OLC-valid
  range owned by the canonical destination zone and record their provenance.
- Keep target-native external dependencies at their existing identities only when the
  reference ledger proves target ownership.

### 4.4 Merges, target improvements, and legacy retirement

- Duplicate definitions of one typed source VNUM may merge at its single canonical
  destination.
- Distinct source VNUMs may share a runtime handler or data profile but retain distinct
  canonical prototypes.
- Carry existing target-native improvements and OLC edits into canonical prototypes;
  do not use them to justify a legacy identity alias.
- Do not leave forwarding rooms, compatibility portals, duplicate prototypes, or
  runtime aliases.
- Remove a legacy definition only in the same validated cutover that supplies its
  canonical definition and rewrites all required incoming references.
- `REHOME` is a Phase 6.5 migration operation, not a replacement for final
  `KEEP/PATCH/ADD/MERGE/EXCLUDE` corpus actions. After rehome, regeneration evaluates
  the source record at its canonical target.

### 4.5 Dated Phase 6.5 planning baseline

The 2026-08-12 Phase 2 policy-2 artifact contained 64,395 core identity rows and 1,988
noncanonical mappings. Regenerate these denominators after Phase 6; they are planning
evidence, not acceptance values.

| Group | Noncanonical rows | Legacy state to retire |
|-------|------------------:|------------------------|
| Trail | 353 | `507 -> 1507`; 200 rooms, 151 mobiles, 1 object |
| Hulburg | 1,160 | `591 -> 1591`; 492 rooms, 400 mobiles, 267 objects |
| Jotunheim | 463 | `960 -> 1960`; 287 rooms, 89 mobiles, 86 objects |
| Modern artifacts | 11 | 11 source objects collapsed into `169901-169910` |
| Myth Drannor East | 1 | malformed header allocated to `20002` |
| **Total** | **1,988** | Must be zero at Phase 6.5 exit |

Canonical destinations include:

| Package | Canonical destination |
|---------|-----------------------|
| Trail | zone `20507`, entities `2050700+` |
| Hulburg | zone `20591`, entities `2059100+` |
| Jotunheim | zone `20960`, entities `2096000+` |
| Myth Drannor East | zone `20817`, entities `2081700-2081899` |

Six Jotunheim core records were already planned as canonical additions and are outside
its 463 rehome rows. Preserve them in the assembled package.

Eleven active source artifacts currently map to ten target-native objects. Source
objects `1007` and `1009` both converge on `169906`; Phase 6.5 must restore canonical
objects `2001007` and `2001009`. Select one evidence-backed successor for current
`169906` ownership and progression state, carry modern behavior to the appropriate
prototypes, and never duplicate persistent ownership.

## 5. Reconciliation and conversion controls

### 5.1 Final record actions

Every active source record receives exactly one final action:

| Action | Contract |
|--------|----------|
| `KEEP` | Canonical target record already provides the intended identity and behavior. |
| `PATCH` | Preserve canonical target content and local edits while applying a bounded change. |
| `ADD` | No acceptable canonical target record exists; create it at the formula identity. |
| `MERGE` | Combine duplicate or overlapping content deterministically. |
| `EXCLUDE` | Disable/log the smallest irreducibly invalid active unit. |

The action ledger records source path, kind, VNUM, hash, candidate target records and
hashes, lineage evidence, confidence, local-edit evidence, destination, dependencies,
action, and rationale. Filename, display text, or an old offset may nominate content
evidence but may not authorize overwrite or determine destination.

Use separate evidence thresholds for candidate nomination, `KEEP`, and bounded `PATCH`.
Matching text or an old VNUM formula alone is never enough for `KEEP` or overwrite.

Identity and content ambiguity are separate. The formula fixes an RoL-owned identity;
evidence decides what content belongs there. Preserve unrelated target-owned candidates.
If no safe active content can be formed, use the smallest-unit `EXCLUDE` fallback rather
than an alias, guessed merge, or unresolved action.

### 5.2 Capability dispositions

Classify behavior independently from record action:

| Code | Classification | Contract |
|------|----------------|----------|
| `N` | Native | Current Luminari behavior is equivalent. |
| `T` | Transform | A data or argument transform produces equivalent behavior. |
| `A` | Adapter | A bounded compatibility layer preserves used source semantics. |
| `P` | Port/new mechanic | Active content requires new runtime behavior. |
| `B` | Bug/fallback | Apply the fixed repair order and record evidence. |
| `X` | Minimal exclusion | Disable and log the smallest irreducible active unit. |

`A` here means adapter; RoL has no `A` reset opcode. A capability row records source
syntax and code path, target evidence, occurrences and consumers, semantic contract,
converter/runtime IDs, fixtures, acceptance tests, and loss policy. `B` and `X` rows
list affected records, impact, evidence, and automatic resolution. `Dead`, `unused`,
and `source no-op` are evidence tags, not capability dispositions.

Initial routing hypotheses become authoritative only after code-path evidence,
occurrence inventory, and fixtures:

| Construct | Initial route |
|-----------|---------------|
| Room/mobile/object grammar and values | `T` by traced grammar and symbol family |
| Base `M/O/P/G/E` resets and basic doors | `T` after chain/state proof |
| Extended/chance `D`; chance `R` | `A`, `P`, or `B` by used variant |
| Follow `F`; removal `X`; calendar `T` | `A` or `P`; never direct opcode mapping |
| Conversational quests | Per-direction `N/T/A/P/B` |
| Five SOC modes | Measured native-compatibility versus DG selection |
| Room extensions/dimensions | `T`, `P`, `B`, or locked `X` |
| Object traps | `T` or `A` after an end-to-end audit |
| Special procedures | Reuse/patch first; `A` or `P` only for active gaps |
| Numeric SOC commands; legacy colors | `T` through traced name/token maps |

### 5.3 Typed definition and reference graph

Build all definitions and edges before emission. Required edge families include:

- exits, exit keys, container keys, contents, portals, teleports, vehicles, and
  transports;
- reset rooms, mobiles, objects, containers, doors, equipment, followers, removals,
  calendar predicates, and trigger attachments;
- shop identities, keepers, rooms, and products;
- HLQ hosts, required items and item types, rewards, loaded entities, and destinations;
- SOC owners, commands, and path rooms;
- DG attachments and typed literals in trigger bodies;
- typed object value slots;
- special-procedure bindings and dependent records; and
- code, configuration, database, and non-database persistent consumers.

Typed namespaces may reuse numbers. Never flatten references into one numeric space.
Every external edge requires explicit target ownership. An unresolved required edge
blocks emission or triggers the fixed minimal exclusion for its malformed instruction.
Never emit `NOWHERE`, zero, or a guessed VNUM to conceal a missing required edge.

Raw numeric searches are discovery aids. Classify each match as an active typed
reference, immutable source/history, unrelated literal, documentation example, or
generated artifact to regenerate.

### 5.4 Required control artifacts

Maintain these machine-readable controls:

1. **Source and target inventories:** run ID, time, tool version, root identity without
   credentials, paths, sizes, cryptographic hashes, manifest/dependency membership,
   target index membership, missing/orphaned files, exact baseline validator commands,
   finding identities/counts, output and parse status, source/runtime revisions, and
   policy version.
2. **Grammar inventory:** every record version, extension, directive, terminator,
   package/record boundary, flag/enum/value position, zone header/reset variant, shop
   keyword, quest direction, SOC mode/action/path/command, special binding, and typed
   reference, with counts, locations, duplicate/companion coverage, and unknown-token
   reports.
3. **Record-action ledger:** the fields and final actions in Section 5.1.
4. **Capability matrix:** the fields and dispositions in Section 5.2.
5. **Canonical identity/rehome manifest:** normalized source zone, typed source and
   canonical VNUM, source/target hashes, formula or external rule, duplicate/merge
   evidence, old target if any, all consumers, validation, and legacy removal status.
6. **Collision report:** world definitions/ranges, hardcoded and generated consumers,
   configuration, synthetic triggers, and relevant persistent stores, all separated by
   typed namespace.
7. **Reference and reverse-reference reports:** all incoming/outgoing edges and every
   consumer of a retired identity.
8. **Conversion contract registry:** grammar, normalized IR, capability IDs, emitter
   behavior, required runtime version, ordering, validation, and diagnostics for every
   supported construct.
9. **Run and acceptance ledger:** converter build, input/policy/mapping hashes, selected
   records, emitted actions, output hashes, diagnostics, test evidence, engineering
   resolutions, and superseded run.

Regenerate the affected inventory and dependent artifacts whenever an input changes.

## 6. Semantic conversion contracts

### 6.1 Rooms

Source rooms encode a zone, legacy flags, sector, and dimensions; target rooms use a
zone VNUM, four flag vectors, sector, coordinates, and different optional records.

- Map VNUMs, exits, keys, external references, descriptions, extra descriptions,
  moving-room data, and named procedures.
- Map flags and sectors symbolically.
- Document the approximation from source length/width/height to target room-size flags;
  the target has no direct dimension fields.
- Supply deterministic target coordinates, required terminators, and index entries.
- Resolve the 18 raw duplicate room records before emission.

Rooms are the reference backbone; syntax-only validation is insufficient.

### 6.2 Mobiles

Parse every observed legacy version and reject unknown grammar. Map flags, affects,
positions, sex, attacks, race, class, alignment, abilities, saves, dice, gold, and
experience by symbolic meaning. Record defaults and losses for target-only or
unrepresentable fields. Remap reset, shop, quest, follow, SOC, script, and special
procedure consumers.

### 6.3 Objects and traps

Map item types, wear/extra/anti flags, applies, bonus types, materials, sizes,
proficiencies, economy fields, descriptions, and affects by symbolic meaning. Interpret
value arrays per item type; a parseable wrong slot is a gameplay defect. Resolve spell
and skill references by registered name, not numeric equality. Remap resets, shops,
quests, containers, keys, traps, and special procedures. Select target trap
representation only after tracing loading, triggering, persistence, OLC, and combat
behavior end to end.

### 6.4 Shops

Convert source keeper-owned shop data into the target shop grammar. Preserve products,
rooms, buy/sell rules, hours, messages, and any source AI extension that active content
uses. Resolve keeper, room, and product identities independently through typed
namespaces; source `SHOP:` is not a standalone source shop VNUM.

### 6.5 High-level quests

Source `.qst` files are conversational high-level quests, not target AutoQuest files.
Compile them to canonical target `hlq` entries direction by direction:

- ask/topic matching and physical/runtime output order;
- source list-prepend/reversal behavior versus physical file order and target list
  ordering;
- item, item-type, and coin input;
- item, coin, random-object-range, prestige, and experience rewards;
- load mobile/object, attack, disappear, teach, and other action output;
- duplicate host headers, source lookup behavior, and target multi-block behavior; and
- unconditional `!` pre-approval on every emitted entry.

Do not hide gaps behind one blanket transform. Source experience code is a no-op, item
type input lacks a direct target equivalent, random ranges require an explicit contract,
and coin/disappearance/order behavior differs. The fixed resolution is to implement
configured experience, preserve exact coin costs, implement item-type input, random
rewards, disappearance, and ordering through transforms or bounded adapters, merge
distinct intended duplicate-host content, and remove only proven duplicates. Disable
only an irreducibly malformed direction so the host block can continue safely.

### 6.6 SOC automated mobile actions

Source `.soc` files are not player social commands. The five observed modes and raw
header counts are `LIST` 410, `PATH` 33, `PERIODIC` 1,099, `TIMED` 11, and `TRIGGER`
205; regenerate counts from the active inventory. The full raw corpus has 1,758 mobile
lists, 4,284 actions, and 144 numeric action codes. About 2,518 actions use special
indoor, outdoor, all-zone, room, or path behavior; 1,766 use old command-table indexes.

- Resolve source numeric command ID -> source command name/behavior -> target action.
  Never reuse the current command-table index.
- Preserve chance, delay, ordering, arguments, messages, list lifecycle, and path
  sequencing.
- Provide tested indoor/outdoor/all-zone and room echo semantics.
- Prototype both a native compatibility subsystem and DG compilation with bounded
  helpers. Select by measured fidelity, OLC ownership, maintainability, observability,
  performance, synthetic-record volume, and test cost.
- Attach behavior to canonical mobiles and allocate any triggers deterministically in
  their canonical owning zones.

### 6.7 Zone metadata and resets

RoL reset grammar uses `M/O/P/G/E/D/R/F/X/T`; it has no `A`. Target reset grammar uses
`M/O/G/E/P/D/R/T/V/J/I/L`, where target `T` attaches a DG trigger. Preserve effective
execution, conditional chains, limits, probability, and source order rather than opcode
letters or argument positions. Source `G` chance and target `G` arguments occupy
different positions; source `D` state/chance and target door states also use different
representations.

Raw full-corpus reset counts are planning evidence:

| Opcode | Count | Required treatment |
|--------|------:|--------------------|
| `M` | 32,112 | Map mobile, room, limits, chance, and chain behavior |
| `O` | 4,535 | Map object and room |
| `P` | 3,845 | Map object and container |
| `G` | 4,288 | Map object and source/target argument differences |
| `E` | 13,172 | Map object, mobile context, and equipment slot |
| `D` | 6,518 | Translate bitmask/extensions/chance to proven target behavior |
| `R` | 684 | Preserve chance-qualified variants, not only native unconditional removal |
| `F` | 1,485 | Implement follow/group/mount semantics; source `if_flag` is follow mode |
| `X` | 422 | Implement room/global mobile removal, combat, and chance behavior |
| `T` | 275 | Implement calendar predicate; never map directly to target trigger `T` |

Two loader-accepted `F2 ...` rows omit whitespace. Two un-commented headings,
`GROUPING*` and `GATE QUEST STUFF`, are misread by the source's first-character
dispatcher as malformed `G` rows. Lines such as `A Halruaan Airship 1~` are zone titles,
not reset commands. Preserve these as positive or negative grammar/source-defect
fixtures; do not silently execute or normalize malformed headings. The headings are not
part of the 4,288 intended `G` count.

Map lifespan, reset mode, active state, zone flags, bottom/top, and generated trigger
attachments by meaning. Every used reset variant needs an executable source fixture and
an expected target result.

### 6.8 Special procedures

Search current assignments, implementations, comments, and history before porting.
Existing target procedures may be equivalent, evolved, partial, or already attached to
converted identities. Reuse or patch them first. Only active dependency-closure
consumers justify work. Trace commands, spells, objects, rooms, globals, persistence,
scheduler assumptions, and all wrapper/dynamic registration paths. Process regular
families with strict source parsers and source-hashed generated profiles; keep irregular
mechanics in dependency-complete shared-runtime batches.

### 6.9 Cross-cutting transforms

- Convert the 461,060 raw legacy `&+X` color tokens with a token-aware lexer that also
  preserves literal ampersands.
- Map flags, item types, sectors, races, classes, spells, skills, affects, applies, wear
  positions, and commands through traced symbol tables; never assume numeric equality.
- Generate correct target record versions, `$`/`$~` terminators, and kind indexes.
- Resolve each external dependency to proven target-native content, a canonical active
  record, or a hard failure/minimal exclusion.
- Retain kind, source file, original VNUM, and source hash for every emitted record.
- Report every default, repair, dropped field, unsupported action, duplicate resolution,
  and engineering override.

## 7. Converter and release architecture

Use a standalone deterministic reconciler under a dedicated location such as
`scripts/world/`; confirm language, packaging, fixtures, and build integration in its
implementation spec.

Provide typed parsers for `wld`, `mob`, `obj`, `shp`, `qst`, `soc`, and `zon`, plus a
normalized typed IR. Provide symbolic mapping tables with tests and emitters for target
`wld`, `mob`, `obj`, `shp`, `hlq`, `trg`, and `zon` records, indexes, and terminators.
Also produce source/output structural comparisons, loss/exception reports, fixtures for
every observed grammar, per-zone acceptance checklists, and a corpus progress ledger.

```text
RoL active inputs ---------> typed source IR -----> grammar/capability inventory
                                  |
Current target inventory -------+-----> content lineage and record actions
                                  |
Current target runtime ----------+-----> conversion contracts
                                                    |
                                                    v
                                  canonical identity and typed reference resolver
                                                    |
                                                    v
                                        isolated change/rehome bundle
                                                    |
                                                    v
                                    structural and behavioral validation
                                                    |
                                                    v
                                      planned development application
```

Analysis, reconciliation, and generation precede writes to the development world.
Output first enters a unique isolated run directory. Application changes only declared
paths, rebuilds indexes, applies runtime and persistent dependencies in the required
order, removes only declared legacy records, and validates the result.

A run bundle contains equivalent machine-readable data to:

```text
run-manifest.json
source-inventory.json
target-inventory.json
reconciliation.jsonl
capabilities.jsonl
identity-map.jsonl
rehome.jsonl
reference-report.json
change-plan.jsonl
removals.jsonl
output/world/
validation/
```

Human summaries derive from canonical records. Never hand-edit ignored generated output
as the source of truth.

Each vertical capability unit includes traced source/target behavior, affected action
and capability rows, IR support, runtime work, deterministic conversion, diagnostics,
positive/negative/ambiguity fixtures, tests, a pilot demonstration, and relevant
documentation/help. Runtime capability lands before content that depends on it.

## 8. Phased execution plan

One session is one 2-4 hour objective and 12-25 concrete tasks. Phase summaries below
define scope and gates; each session spec expands them into its task checklist.

Phases 0-5 are complete. Their scope, run IDs, counts, commits, evidence, and prior
forecasts are in [RoL-Changelog.md](RoL-Changelog.md).

### 8.1 Phase 6: Special-procedure reconciliation

**Status:** discovery repair complete; corrected binding reconciliation in progress.

The call-path-aware extractor, macro resolution, dynamic shop/quest dispositions,
regression fixtures, and dependent Phase 1, 2, 5, and 6 regeneration are complete. The
remaining measured scope is 115 live static bindings across 97 direct handler names in
26 source files. The first corrected-denominator closure completed the four-handler,
four-binding Tarrasque encounter and added the missing typed mobile-death gateway. The second
reused the established target class-family guild adapters for 14 source callbacks and 37 room
bindings. The third reconciled the planar demon base layer: 25 explicitly abyss-forged mobiles
now dissolve their wielded weapons before death handling, while eight direct `standardDemon`
bindings compose with the already complete race-driven demon runtime. The fourth reconciled the
planar static initializers for Bar-lgura, Cambion, Lemure, Nupperibo, Dretch, Rutterkin, and
Alu-fiend while fixing owner-level composition of multiple prototype requirements. The fifth
reconciled the four Darkhold elemental death callbacks through the existing composable death
profile runtime, preserving their mapped reward drops and ordinary corpses. The sixth
reconciled the Seelie faerie combat family: 18 mobiles now compose exact prismatic, faerie-fire,
and hidden-target search profiles through one persistent target procedure. The seventh
reconciled 16 Hive manscorpion bindings across four venom handlers through a new typed
successful-mobile-hit gateway, preserving source chances, random mortal targeting, saves,
nonstacking Constitution loss, and the king's slow-poison-dependent lethal branch. The eighth
reconciled six successful-hit area handlers through that gateway, preserving Dobluth bladestorm
and wail behavior, Hive sandstorm damage and blindness, Greycloak wail and poison fumes, and
Aralesh's opponent and pet-owner execution while using target-native typed damage, saves,
resistance, safe area targeting, and hit-context invalidation. The ninth reconciled both Hive
Skriaxit bindings through the same persistent combat procedure. Their scheduled sandstorm runs
every three rounds while idle, fighting, or disabled, reaches the current and open orthogonally
adjacent rooms, preserves source eligibility and dispel behavior through target-native safety,
resistance, and Will saves, and deliberately deals zero damage because the bound source loop
resets its Skriaxit count before evaluating damage. The tenth reconciled 18 planar bindings
across eight handlers: Manes and Balor death behavior,
Balor whip and lightning-sword procs, independent Vrock screech and spore cooldowns, Spinagon
flaming spikes, and the source-inert Chasme buzz. The runtime uses target-native damage, saves,
area safety, affect state, corpse suppression, owner restrictions, and item invalidation while
preserving the source's actual `20d2` Spinagon roll despite its contradictory `2d20` comment.
The eleventh reconciled 11 planar bindings across five handlers: Glabrezu pincer capture,
Marilith tail capture, Succubus charm and captive commands, and the five-member Vrock dance of
ruin. The port preserves source thresholds, delayed attacks and kisses, Blackguard service,
command restrictions, three dance stages, shared 20d10 damage, and a one-day cooldown while
using target-native charm defenses, saves, and area safety. It also repairs the source's
wrong-variable defect so aborting or completing a dance clears the full cohort rather than
leaving peer Vrocks permanently disabled. The twelfth reconciled 22 Avernus bindings across
seven handlers: a Tiamat dragon's zone alert, Barbazu reactive rage and critical glaive blood
loss, Gelugon freezing tails, Meritos's caster-silencing bolt, Hanariel's disarm interception,
and the Gelugon freezing spear. The port adds the typed mobile-was-hit gateway, preserves the
source's actual one-in-seven tail chance despite its one-in-ten comment, keeps stacked recurring
blood-loss events and their nonlethal floor, and uses target-native saves, paralysis immunity,
resistance, affect state, waits, damage, and owner restrictions. The thirteenth reconciled the
remaining 21 active Avernus lifecycle bindings across 14 handlers. Twenty bindings now compose
exact mobile, object, and garden profiles for transformations, patrols, stealth, prisoner return,
room illusion, alignment-aware echoes, Bel's chamber lifecycle, the black altar, dancing daggers,
the infernal rod and sword, and the 16-room garden. The callback named `avernus_seal_unload` is
explicitly source-inert because it never parses an event and therefore never registers its
unload callback. The port replaces unsafe source-global patrol state with per-mobile state and
adds a narrowly scheduled room-activity gateway only for the authored garden. The fourteenth
reconciled all 18 active Scornubel bindings across 14 handlers. Seventeen mobile assignments now
use exact generated periodic profiles; Parchimil composes that behavior through its existing
guild-guard procedure, and fiery mace 2006084 preserves its one-in-36 fixed 100-point hit proc.
The fifteenth reconciled six regular Zhentil Keep periodic handlers through the same generated
runtime. It adds strictly validated zero-roll conditional generation for the little girl and
terrified merchant while leaving the multi-event gate guard pending for a complete port.
The sixteenth reconciled all 16 active Darkhold bindings across nine handlers. Eleven musical
skull and passage-gem objects use one exact-identity adapter, two weapons extend the typed weapon
runtime, and the shadow fiend and shadow dragon extend the monster-combat runtime with independent
cooldowns and a death-unlocked passage. The four elemental deaths remain independently composed
through the previously converted death profiles.
The seventeenth reconciled all 16 active `genericDrowEq` bindings through one exact object
runtime. It preserves the source's hourly decay event, Underdark stop and surface restart,
four-Hz jitter translated to the target clock, always-true time predicates, container and
sunlight rates, no-sell marking, source integer reductions, first-two-affect mutation, messages,
and terminal extraction. The preprocessor-disabled seventeenth assignment remains excluded.
The eighteenth reconciled 14 Undermountain bindings across nine handlers. Eight exact mobile
identities now use generated profiles that preserve all 61 source outcomes, zero-to-100 ranges,
awake and not-fighting gates, speech, room actions, and the expanded `frown` social. The six
troglodyte-stench registrations are explicitly source-inert because their event registration
and implementation are compile-disabled by `#if 0`.
The nineteenth reconciled eight death bindings across seven handlers from Trahern, Dobluth, and
Undermountain. The shared death-profile runtime now preserves both elemental-fire weevils, Lady
Aleanrahel's replacement and item transfer, Helmed Horror and Butcher Knife reward drops, two
corpse-suppressing shatter deaths, and the active white-pudding split. The source's single
`25d2` weevil roll and cumulative protection halving remain explicit. The twentieth reconciled
all 15 active Griffon's Nest `aggroNonBarbarian` bindings through the existing monster-combat
runtime. Exact converted guards attack the first visible mortal without a Berserker level,
remember eligible targets instead of changing opponents while occupied, exempt staff and NPCs,
and preserve the source contract that an intercepted command continues. The twenty-first
reconciled five Dusk Road and Undermountain bindings across three handlers through the same
runtime. Dusk Road basilisks scan eligible visible room occupants in source order after a
successful hit, with exact level-derived chances and save modifiers, and paralyze the first
failed target for ten rounds. Manscorpion and wyvern tail effects remain critical-only; the
former applies a random two-to-twelve-round paralysis, while a failed wyvern save is fatal.
Target-native paralysis immunity, saves, and hit-context invalidation protect later riders.
The twenty-second reconciled all six active Undermountain drow-conclave guard bindings. The
exact profiles preserve the source's one-per-boot alarm, visible mortal and staff gates,
11-room detect-invisibility sweep, first-three-guard and sergeant redeployment order, and six
one-in-20 combat lines. Target-native room-path movement replaces source hunt events, while
room-list traversal deliberately repairs the source's wrong global-character-list advance so
the alarm cannot mutate unrelated rooms.
The twenty-third reconciled the complete Undermountain Death's Head lifecycle across five
bindings and three handlers. One owner-aware runtime covers sapling, fruit, young, and mature
mobiles plus the implanted seed object. It preserves corpse-fed growth, larger-tree suppression,
head counts, fruit shedding, cries, bite implantation, carrier damage, and corpse germination.
The source's always-eleven mature regrowth, unreachable wood drop, existing-seed fruit gate, and
successful-sprout seed lifetime remain explicit, while object extraction and mobile replacement
use target invalidation and event ownership safely.
The twenty-fourth reconciled the seven remaining hit-only weapon bindings across six handlers.
Frostbite, the Trahern crystal and obsidian swords, the broadsword of dancing shadows, both drow
snake whips, and the searing rod now share the typed exact-identity weapon runtime. The port
preserves authored slots, one-in-22, one-in-33, and one-in-26 chances, critical-only poison and
fire, damage dice, daytime scorching ray, nighttime movement and armor penalties, layered shadow
reductions, source fire-shield save pressure, and target invalidation. Converter output persists
the procedure and required automatic-procedure flag together. This closes the hit callbacks only;
the separate Jotun passive apply-slot family remains pending.
The twenty-fifth reconciled Bhaal's warrior and rogue weapons plus the ordinary Seelie bard's
glaive. Both Torment weapons answer a target fire or cold shield with the triggering strike's
damage, retain source save pressure, and remain suppressed by the wielder's elemental protection
or Globe of Invulnerability. The bard's glaive preserves its level gate, Dexterity-weighted
one-in-2001 roll, direct two-round blindness, and saved `10d10` burst against an already-blinded
target. The source save modifiers are translated to the target API's opposite sign convention;
all three identities use the existing typed weapon gateway and target invalidation.
The twenty-sixth reconciled five Undermountain bindings across three object handlers. Astral-forged
weapons 2093191 and 2093195 now switch their first hitroll and damroll applies between +3 and +6
on exact source-Astral rooms, including while carried, worn, on the ground, or inside containers.
Room-level `ROOM_ROL_ASTRAL` conversion metadata preserves source sector 23 after its target sector
maps to generic Planes, without overmarking mixed-sector zones. Torin objects 2093446 and 2093447
restore source prototype state for valid owners, burn nonstaff players and pets who are neither a
Warrior nor Cleric Mountain Dwarf or Duergar, and retain source identification text. Object
2093447 additionally casts level-40 Chain Lightning on critical hits. The shared weapon pulse
contract now permits authored ground and contained-object events while retaining equipped/combat
requirements for weapon-hit callbacks.
The twenty-seventh reconciled the three Undermountain Vortex Knight death handlers through the
existing composable mobile-death runtime. Silver, Golden, and Platinum Knights 2093003-2093005
now suppress their ordinary corpses and create their mapped portals 2093006-2093008 only when no
same-prototype peer remains in the room. Each portal receives target-native one-tick decay;
the identity-specific source message, including its spelling variation, remains exact. Phase 7
must stage the converted portal prototypes and their mapped destinations before live testing.
The twenty-eighth reconciled three Trahern successful-hit handlers through the typed monster
combat runtime. Gakarak 2020217 now sends a one-in-three room quake and knocks standing targets
down when a `1..101` roll exceeds half their current Dexterity. Kazgoroth 2020234 tosses its
opponent to room 2020237, deals typed `10d10` bludgeoning damage, clears cross-room combat, and
leaves a survivor reclining with a three-round stun. Slothen 2020248 deals typed `10d15` acid to
its opponent, then applies a target-safe `20d15` acid burst to eligible room targets that fail
the source-equivalent Fortitude save. Target-native typed defenses and target invalidation protect
all three paths; Phase 7 must stage the toss destination before live testing.
The twenty-ninth reconciled the two remaining Trahern Erinyes callbacks on mobile 2020246
through an exact variant of the existing planar charm runtime. The Erinyes targets mortal PCs of
either sex, attempts charm one time in four, translates the source `-1` save modifier to the
target victim-bonus convention, preserves the source messages and command whitelist, and kills
one charmed follower after one to four MUD hours while deferring during combat or a new charm
attempt. Target-native spell resistance, mind blank, no-charm equipment, and charm immunity
remain protective. The shared path now also correctly translates the older Succubus source `-2`
save modifier instead of applying it with the target API's opposite meaning.
The thirtieth reconciled five Yawning Portal periodic handlers through the generated source
profile runtime and excluded three inert Undermountain callbacks at their smallest units.
Tamsil, Mhaere, the regular, the gambler, and Thorn preserve their exact one-to-10,
one-to-30, one-to-100, and one-to-20 ranges, awake and not-fighting gates, speech, room actions,
and Durnan or Kevlar targeted socials. The generator now resolves source-local target-name
arrays and correctly reads the leading `KISS` social record instead of silently omitting it.
The Blade of Paladins, High Duke Sword, and goblin-leader callbacks contain only comments and
return false without registering an event, so attaching target behavior would invent mechanics.
Next, select the highest-value dependency-complete pending combat or utility group.

Preserve the six already locked malformed or ignored rows as explicit, logged
smallest-unit exclusions when regenerating the evidence.

Then process dependency-complete groups of about 20-45 related handler families when
they share a regular shape. Run focused checks per group and the full build/test/install
gate at substantial checkpoints. Add a new source file to both `Makefile.am` and
`CMakeLists.txt`.

**Exit gate:** every active binding is kept, patched, adapted, ported, or minimally
excluded with behavior evidence. The measured remaining Phase 6 planning envelope is 10-20
sessions, or 20-80 focused engineering hours. Thirty corrected batches have closed 360
bindings across 160 handlers, leaving an arithmetic binding projection near 10 sessions and a
handler-diversity projection near 18. The published range allows shared runtime families to
outperform the handler projection while recognizing that 86 of the remaining 97 handlers are
singletons and many require source-specific tracing. Reforecast after another material batch or
any inventory correction.

### 8.2 Phase 6.5: Canonical VNUM rebase and reference closure

**Prerequisites:** Phase 6 has passed; no other work is changing RoL converter policy,
runtime bindings, generated profiles, or active planning inputs; the checkout is
confirmed as development; and the first session regenerates all dated counts and hashes.

**Estimate:** eight initial sessions, split further if a session cannot remain within
12-25 tasks and four hours. No unfinished rebase scope may leak into Phase 7.

Reference closure has two levels. Across the complete active source inventory, Phase
6.5 must type every edge and assign it a canonical destination, explicit target-native
dependency, or owned exclusion/repair action. In the development cutover, it must also
rewrite every currently active edge that crosses a retired identity. It does not emit
all remaining Phase 7 records; Phase 7 implements their already-owned record-specific
repairs and proves their emitted definitions. No edge may enter Phase 7 with unknown
type, identity, or ownership.

At the start of each apply session, read `lib/.env` without modifying it and stop if it
identifies production.

#### Session 6.5.1: Freeze the canonical baseline

- Record Phase 6 run/policy versions and regenerate source, target, Phase 1, and Phase 2
  inventories and artifacts.
- Count and group every noncanonical typed identity by package, subsystem, and current
  destination; verify source and reserved target ranges.
- Record explicit `mytheast` normalization evidence.
- Inventory legacy Trail, Hulburg, Jotunheim, artifact, generated-profile, code,
  configuration, database, and non-database consumers.
- Generate the typed rehome and reverse-reference ledgers with input hashes.

**Gate:** every noncanonical identity and old consumer has an owned rehome,
normalization, external-target, or exclusion row; no identity or collision is unknown.

#### Session 6.5.2: Enforce the universal resolver

- Version the canonical policy and replace legacy-destination selection with formula
  destinations while retaining lineage as content evidence.
- Implement evidence-backed zone normalization, including `mytheast`; reject `20002`
  and `101700` for that package.
- Enforce direct typed entity mapping, distinct source identities, same-VNUM duplicate
  merge behavior, canonical keeper/host/SOC ownership, and canonical trigger ownership.
- Check the complete reserved ranges and add low, typical, maximum, multi-band,
  overflow, malformed-header, ambiguous-ownership, and legacy-override fixtures.
- Regenerate twice and run focused plus full world-tool suites.

**Gate:** planning artifacts contain zero unexplained noncanonical destination and every
normalization is evidence-backed and deterministic.

#### Session 6.5.3: Rehome Trail, Hulburg, and Jotunheim

- Parse current source and legacy target records and verify them against frozen hashes.
- Generate Trail `20507`, Hulburg `20591`, and Jotunheim `20960`, including the six
  already-canonical Jotunheim additions, correct top values, and sparse layout.
- Carry bounded target/OLC edits through explicit rehome actions.
- Rewrite all internal, outgoing, and incoming world references; regenerate owned DG
  triggers and Phase 6 profiles/bindings.
- Stage removal entries without applying them; run strict structure, syntax boot, reset
  observation, component walkthroughs, behavioral comparison, and repeat-generation
  checks.

**Gate:** all three packages stage only at canonical identities with preserved accepted
content and no unresolved typed edge.

#### Session 6.5.4: Rehome modern artifacts

- Trace all eleven source and ten current target prototypes, runtime behavior, world
  consumers, and persistence contracts.
- Create direct canonical prototypes, including separate `2001007` and `2001009`, while
  sharing runtime implementation only where behavior permits.
- Select one deterministic canonical successor for existing `169906` state; preserve
  target improvements without cloning ownership or progression.
- Update registries, assignments, commands, cooldown/progression/ownership lookup, and
  summon, travel, spell, combat, and called-effect dependencies.
- Generate idempotent persistent migrations, rewrite world/code consumers, and stage
  removal of obsolete `1699xx` RoL prototypes.
- Extend production-linked tests; run artifact regressions, world tools, `make test`,
  and `make install`.

**Gate:** all eleven active source artifacts exist at direct canonical identities,
enhanced behavior and unique state are preserved, and no active or persistent RoL
consumer needs `169901-169910`.

#### Session 6.5.5: Close world-data references

- Build the complete staged typed definition/reference graph.
- Validate exits and intended reverse exits, keys, containers, portals, teleports,
  vehicles, transports, reset arguments, shops, HLQs, SOC paths, DG attachments/body
  literals, and typed object values.
- Classify every target-native external dependency.
- Reject surviving source VNUMs, retired RoL VNUMs, missing canonical definitions,
  unexplained external edges, and typed collisions.
- Produce per-package incoming/outgoing reports and cross-zone fixtures; run strict
  validation for every selected or touched zone.

**Gate:** zero unresolved required edge, old RoL edge, or unexplained external edge.

#### Session 6.5.6: Close runtime and persistent consumers

- Classify every source/legacy numeric match in runtime, templates, generated data,
  documentation, tests, databases, and non-database stores.
- Replace active consumers and regenerate all special, periodic, state-aware, combat,
  death, weapon, utility, and event profiles.
- Update example templates when required without touching protected local files.
- Build a transactional, idempotent development migration and test representative old
  rows in an isolated database.
- Verify typed counts, uniqueness, player inventory save/reload, artifact ownership,
  progression, cooldown/account binding, and every discovered house, mail, auction,
  shop, quest, or other persistent consumer.
- Run the migration twice, boot the staged world against the migrated database, reject
  relevant diagnostics, and publish the consumer ledger.

**Gate:** no active runtime, configuration, generated, or persistent consumer depends
on a retired RoL identity; migration is lossless and the second run is a no-op.

#### Session 6.5.7: Assemble and validate the rebase

- Assemble one isolated target containing canonical definitions, rewritten references,
  runtime support, and persistent migration while excluding planned legacy definitions.
- Run global and per-zone strict validation and compare exact finding identities and
  parse completeness with the recorded baseline.
- Run syntax boot, isolated-database boot, eligible reset observation, component
  walkthroughs, representative cross-zone interactions, all artifact/reload paths, and
  focused regressions.
- Run `make test-world-tools`, `make test`, and `make install`; verify no root `circle`
  remains.
- Run documentation drift, ASCII/LF, deterministic regeneration, preservation, and
  second-apply checks.

**Gate:** the isolated target passes every structural, behavioral, persistence,
determinism, no-clobber, and idempotency requirement.

#### Session 6.5.8: Apply and close the development rebase

- Reconfirm development environment; regenerate inventories immediately before apply
  and rebuild/revalidate anything whose input changed.
- Apply canonical definitions, rewritten references, runtime/configuration changes,
  the development-database migration, and legacy removals as one planned cutover.
- Regenerate indexes and profiles; run strict development validation, syntax boot,
  bounded behavioral boot, persistence reload checks, namespace/reference audits,
  `make test-world-tools`, `make test`, and `make install`.
- Require zero unexplained noncanonical identity, retired active definition/reference,
  or unmapped source VNUM in an active target field.
- Update the roadmap, worknotes, testing guide, artifact docs, help, and changelog with
  final run IDs, hashes, counts, exclusions, migrations, and results. Named project
  targets include this plan,
  [REALMS_OF_LUMINARI_WORKNOTES.md](REALMS_OF_LUMINARI_WORKNOTES.md),
  [PHASE4_MANUAL_TESTING.md](PHASE4_MANUAL_TESTING.md), and
  [RoL-Changelog.md](RoL-Changelog.md).

**Gate:** the authoritative development target uses the canonical namespace and every
Phase 6.5 exit criterion in Section 10.1 passes. Only then is Phase 7 unblocked.

### 8.3 Phase 7: Canonical action-based corpus batches

**Updated role after Phase 6.5:** convert and accept all remaining active packages
against the already-applied canonical development baseline. Phase 7 no longer chooses
legacy destinations, reserves ad hoc RoL identity ranges, performs the Trail/Hulburg/
Jotunheim/artifact rebase, or creates compatibility aliases.

An unexpected legacy RoL identity, canonical collision, or formula exception is a
failed Phase 6.5 invariant. Repair and revalidate the canonical baseline before the
affected Phase 7 batch proceeds; never route around it with a local exception.

Batch by dependency closure, shared runtime capability, and record action rather than
by a fixed count of numbered zones. Each batch must:

1. pin the post-Phase-6.5 policy, identity map, target inventory, and input hashes;
2. select an active package closure, including companion-only records and cross-package
   dependencies;
3. assign every contained record its canonical `KEEP/PATCH/ADD/MERGE/EXCLUDE` action;
4. implement and validate the pre-owned dispositions for package-specific missing
   references, including the previously measured 804 gaps, without changing the
   universal identity rule;
5. preserve accepted Phase 6.5 content and touch it only for an explicit incoming or
   outgoing dependency action;
6. implement runtime capabilities before dependent data;
7. regenerate deterministically and report before/after hashes, action/capability
   coverage, identities, references, repairs, exclusions, and companion mismatches;
8. run target syntax checks, exact baseline finding comparison, isolated syntax and
   database boots, reset observation, scripted walkthroughs, focused regressions, and
   preservation/idempotency checks; and
9. emit an isolated reviewable validation bundle for Phase 8 application.

Selected or touched target-baseline findings must be repaired in their owning batch;
they cannot be waived because they predate conversion. The 30 disabled and 2 unlisted
physical files remain outside every ledger and acceptance denominator.

**Exit gate:** every in-scope package and companion mismatch has evidence; every record
has a final action at its canonical identity; every capability and required reference is
resolved; and every package has structural, reset, behavior, walkthrough, preservation,
and deterministic regeneration evidence.

**Estimate:** retain 42-66 sessions only as the pre-rebase planning envelope. Reforecast
after Phase 6.5 from measured canonical batch throughput and remaining record actions;
do not treat 42-66 as a delivery promise or silently subtract work without evidence.

### 8.4 Phase 8: Final canonical integration and release evidence

**Updated role after Phase 6.5:** integrate and apply the accepted Phase 7 corpus
bundles, prove complete-corpus behavior, and close project documentation. Do not repeat
the Phase 6.5 namespace rebase or its legacy persistent-state migration.

Workstreams, split into 6-10 sessions as measured scope requires:

1. freeze the post-Phase-6.5 development baseline and every accepted Phase 7 bundle;
2. assemble a complete isolated target with runtime dependencies before data bundles;
3. reconcile full-corpus counts, actions, capabilities, canonical identities, typed
   references, external dependencies, exclusions, hashes, and output indexes;
4. run global/per-zone structure, syntax boot, isolated-database boot, reset,
   walkthrough, quest, shop, SOC, trap, special-procedure, persistence, and regression
   gates;
5. apply only the validated Phase 7/final integration plan to development, preserving
   the Phase 6.5 canonical baseline and revalidating changed inputs;
6. rerun the universal namespace and retired-reference audits after final apply;
7. finalize converter, operator, builder, testing, apply, system, help, and changelog
   documentation; and
8. publish reproducible final run evidence and acceptance results.

Any newly discovered legacy identity or unresolved rebase consumer returns to Phase 6.5
remediation; it is not deferred as release debt. Any package defect returns to its Phase
7 batch.

**Exit gate:** the complete active source closure is reproducibly represented at
canonical identities, applied to development, functionally playable, and supported by
all evidence in the Definition of Done.

**Estimate:** 6-10 sessions. The combined post-Phase-6 envelope is therefore initially
56-84 sessions for Phases 6.5-8 (112-336 focused hours). With the measured Phase 6
envelope, the current remaining project range is 66-104 sessions, or 132-416 focused
hours. This supersedes the old 49-79-session, 74-114-session, and corresponding hour
forecasts. Replace the post-rebase envelope after the Phase 6.5 measured reforecast.

## 9. Validation and application gates

### 9.1 Structural validation

Record exact commands, parsed findings, identities, and parse-completeness status for
both baseline and staged results. Process exit status alone is insufficient because the
development wrapper can report errors while returning success.

For an explicit isolated root:

```bash
python3 scripts/world/wtool.py \
  --world-root <isolated-lib-root>/world validate --all --strict

lib/world/validate-zone.sh <zone-vnum> \
  --world-root <isolated-lib-root>/world --strict

bin/circle -c -d <isolated-lib-root>
```

The `validate` command requires a selector such as `--all`. A batch adds no new global
finding, and every selected/touched record has no unresolved error. Validate indexes,
record versions, terminators, zone ranges, typed collisions, ASCII, and LF endings.

### 9.2 Runtime and behavior validation

Use an isolated lib root and isolated MariaDB instance. Reuse or adapt
`scripts/ci/prepare_test_runtime.sh` and its safety checks; never run destructive
fixtures against the development or production database. Boot logs must contain no
relevant invalid-record, reference, reset, trigger, persistence, extraction, or
`SYSERR` diagnostic.

Automated evidence covers resets, doors, keys, containers, equipment, shops, HLQs,
SOC, paths, traps, special procedures, cross-zone travel, artifacts, and persistent
reload. Every converted room component receives reset observation and a scripted
walkthrough. There is no human builder sign-off.

### 9.3 Code integration validation

When runtime or converter code changes, run the focused suites plus:

```bash
make test-world-tools
make test
make install
```

`make install` must remove any root-level `circle` artifact. Every source-file addition
or removal appears in both `Makefile.am` and `CMakeLists.txt`.

### 9.4 Determinism, preservation, and apply validation

- Repeated conversion from identical inventories is byte-identical.
- Untouched target paths retain their recorded hashes.
- Existing target/OLC content changes only through declared actions.
- Every new/canonical identity is collision-free in its typed namespace.
- Every required reference resolves through the canonical manifest.
- Repeated application to a disposable migrated copy writes nothing or reports a safe
  explicit no-op.
- Persistent migrations are scoped, transactional where supported, idempotent, and
  verified with typed before/after counts and save/reload behavior.
- Apply runtime dependencies before data, canonical definitions and rewritten
  references before retiring old definitions, and all dependent parts in one validated
  plan.
- After development apply, rerun structural, namespace, reference, syntax, bounded
  behavior, persistence, and code test gates against the resulting state.

## 10. Exit criteria

### 10.1 Phase 6.5 exit

Phase 6.5 passes only when:

1. every active source zone has one evidence-backed normalized identity;
2. every active zone destination is normalized source zone plus `20000`;
3. every active non-excluded room, mobile, and object destination is source VNUM plus
   `2000000`;
4. distinct active typed identities remain distinct except duplicate definitions of the
   same typed source VNUM;
5. `mytheast` is zone `20817` with entities `2081700-2081899`;
6. Trail, Hulburg, and Jotunheim exist only at canonical RoL identities;
7. all eleven modern artifacts exist at direct canonical VNUMs with unique persistent
   state;
8. no world, runtime, configuration, generated, test, documentation-policy, database,
   or non-database consumer requires a retired RoL identity;
9. every cross-zone exit, key, quest, shop, reset, portal, SOC path, and DG reference
   resolves through the typed canonical manifest;
10. canonical ranges and every synthetic identity are collision-free by type;
11. target/OLC edits are preserved or changed only by explicit evidence-backed action;
12. persistent migration loses or duplicates no ownership, progression, inventory, or
   discovered state;
13. the isolated target adds no baseline finding and touched records have no unresolved
   finding;
14. syntax/database boots, resets, walkthroughs, focused tests, world tools, CuTests,
   and installation pass;
15. generation is byte-identical and repeated apply is a safe no-op;
16. the applied development target passes the same namespace, reference, structure,
   persistence, and behavior audits;
17. current documentation states the canonical rule and no active plan gives legacy
   RoL identity precedence; and
18. no unexplained exception, unresolved decision, or `BLOCKED` identity remains.

Machine-checkable summary:

```text
noncanonical active RoL zone identities   = 0
noncanonical active RoL entity identities = 0
active references to retired RoL VNUMs    = 0
unresolved required typed references       = 0
```

### 10.2 Project definition of done

The full conversion is complete only when:

1. source and development-target inventories reproducibly identify every run input;
2. every active package, companion record, and contained record is accounted for;
3. every in-scope source record has a final canonical
   `KEEP/PATCH/ADD/MERGE/EXCLUDE` action;
4. every observed active behavior has a final capability disposition and tested target
   contract;
5. every canonical identity, synthetic identity, and required typed reference resolves
   without collision or legacy alias;
6. every existing target edit is preserved or changed by an explicit action;
7. every source repair, default, loss, or smallest-unit exclusion is explicit and
   tested;
8. generated files have valid target grammar, ranges, indexes, terminators, ASCII, and
   LF endings;
9. selected/touched records have no unresolved finding and the assembled target adds no
   baseline diagnostic;
10. full syntax, isolated database, behavior, reset, walkthrough, persistence,
    regression, no-clobber, determinism, and idempotency gates pass;
11. each in-scope package has a validation bundle and acceptance evidence;
12. the complete result is applied to this development checkout and passes post-apply
    audits;
13. converter, operator, builder, testing, apply, system, help, and changelog
    documentation match the final behavior; and
14. the result can be regenerated and safely re-applied from recorded inputs and
    versioned policy with no unresolved decision or final `BLOCKED` action.

## 11. Priority, risks, and exclusions

Prioritize by measured evidence:

```text
priority = pilot blocking + dependency fan-out + corpus frequency + reuse value
```

Do not automate this ranking. A rare construct may lead when it protects identity,
blocks safe staging, or has high fan-out.

Required risk controls:

- **OLC edits:** build canonical content from reconciled target plus source evidence and
  compare semantic fields and hashes.
- **Missed cross-zone reference:** use a complete typed graph and reverse references;
  fail on old, ambiguous, or unresolved edges.
- **Old and canonical copies both load:** audit definitions and indexes; assembled and
  applied targets contain only canonical RoL-owned records.
- **Persistent rows reference retired prototypes:** inventory all stores, test an
  idempotent isolated migration, and verify save/reload before development apply.
- **Artifact ownership is duplicated by the split:** move merged state to one
  deterministic successor and test uniqueness and account binding.
- **Generated Phase 6 data restores old VNUMs:** update generators/policy, regenerate
  from hashes, and reject old VNUMs in output.
- **Inputs change before cutover:** hash inputs, stop concurrent policy/binding work,
  and regenerate affected evidence before apply.
- **A zone header is normalized incorrectly:** require manifest, range,
  contained-record, and typed-reference evidence; keep `mytheast` explicit.
- **The canonical range acquires a target collision:** fail and repair/revalidate the
  reserved namespace; never create a local alias or exception allocation.
- **A source defect becomes silent data loss:** record every repair or minimal exclusion
  with affected records, impact, fixtures, and high-severity diagnostics.

Out of scope:

- disabled, unlisted, or demonstrably non-working source content;
- gameplay balance changes unrelated to conversion correctness;
- compatibility duplicates, forwarding records, or permanent VNUM aliases;
- renumbering target-native Luminari content not owned by RoL;
- production files, services, or databases;
- protected local configuration or credential changes; and
- backup, rollback, recovery, or remote-capture artifacts.

## 12. Handoff sequence

```text
repair and complete Phase 6 discovery and bindings
-> freeze concurrent RoL identity/policy work
-> execute Phase 6.5 Sessions 1-8
-> pass every canonical namespace and reference-closure gate
-> reforecast and execute Phase 7 canonical package batches
-> integrate and apply Phase 7 bundles in Phase 8
-> pass the full project Definition of Done
```

Phase 7 consumes only the canonical Phase 6.5 policy, identity manifest, generated
profiles, and validated development baseline. Phase 8 preserves that baseline while
integrating the remaining accepted corpus; neither phase may reintroduce legacy RoL
identity precedence.
