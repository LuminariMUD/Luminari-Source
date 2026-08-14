# Realms of Luminari Canonical Conversion Plan

- Status: Complete through Phase 8
- Consolidated: 2026-08-13
- Phase 6.5 completed: 2026-08-14
- Phase 7 plan streamlined: 2026-08-14
- Phase 7 completed: 2026-08-14
- Phase 8 completed: 2026-08-14
- Source: `EXAMPLE/RealmsOfLuminari/`
- Target: this development checkout and its current `lib/world/`
- Production changes: prohibited
- Superseded plans:
  [feature-first](plan-archive/REALMS_OF_LUMINARI_FEATURE_FIRST_CONVERSION_PLAN.md),
  [Phase 6.5 VNUM rebase](plan-archive/PHASE6_5_CANONICAL_VNUM_REBASE_PLAN.md), and
  [zone conversion scope](plan-archive/REALMS_OF_LUMINARI_ZONE_CONVERSION_SCOPE.md)
- History:
  [Phase 6 archive](changelog-archive/archive08_13-phase6-complete-RoL-Changelog.md)
  and
  [dated planning baselines](changelog-archive/archive08_13-canonical-plan-baselines.md)

## 1. Mission, scope, and evidence

Convert the active Realms of Luminari (RoL) dependency closure into a structurally
valid, functionally playable Luminari world. Scope includes rooms, mobiles, objects,
shops, high-level quests (HLQs), automated mobile actions (SOC), zone metadata and
resets, traps, and every special procedure required by active content.

This is reconciliation, not file copying. For each source record, compare the active
source data and runtime with the current target world, prior conversions, OLC edits,
runtime, configuration, documentation, and persistent state. Preserve useful target
content, choose one final record action, and emit the result at its canonical RoL
identity. Implement runtime capabilities before converting dependent content.

### 1.1 Active corpus

The current scope baseline is:

- 252 active physical `.zon` files containing 255 zone records;
- active companion-only data selected by source build lists and dependency closure;
- 30 disabled and 2 unlisted physical zone files excluded from inventory, conversion,
  runtime work, and acceptance denominators; and
- six active manifest basenames without same-named `.zon` files, resolved from build
  lists, aggregates, multi-header ownership, and typed references rather than invented
  records.

Physical packages and zone records are not one-to-one. `foggy_woods.zon` and
`northern_highroad.zon` contain extra active headers, and companion coverage differs by
record kind. Regenerate all counts before using them as acceptance denominators.

Known source ambiguities include duplicate rooms `45100-45117` in
`dwarven_mines.wld` and `flesh.wld`, 39 duplicate quest-host headers, reused zone
headers `466`, `509`, `757`, `903`, and `919` (`466` occurs in five files and nested
`903` collides with `mirar_ferry.zon`), and the malformed `mytheast.zon` header in
Section 3.2. Resolve active instances through evidence-backed ownership, merge, split,
or minimal exclusion; never guess identity.

### 1.2 Authoritative inputs

Read package-specific authoring inputs from:

```text
EXAMPLE/RealmsOfLuminari/areas/AREA
EXAMPLE/RealmsOfLuminari/areas/{mob,obj,qst,shp,soc,wld,zon}/
```

Top-level `world.*` files are generated aggregates. Use them only as regression oracles
for active membership, build order, and the assembled source view; `areas/AREA` and
`src/build_areas.c` define that relationship.

Trace source behavior through `db.c`, `build_areas.c`, `interp.c`, `quest.c`, `shop.c`,
`shop_parallel.c`, `socials.c`, `structs.h`, `specs.assign.c`, and every reached
`specs.*.c` call path. On the target, trace loaders, reset execution, OLC writers,
shops, HLQ, DG scripts, special assignments, commands, traps, persistent bindings, and
world validators. Current code controls behavior; names, format resemblance, history,
and comments are supporting evidence only.

The target world and RoL reference tree are ignored by Git. Review them through hashed
inventories, action ledgers, isolated output, and validation reports rather than Git
diffs.

## 2. Non-negotiable controls

1. **Development only.** Never change production code, world data, services, or
   databases. Read `lib/.env` before every apply session and stop if it identifies
   production.
2. **No recovery artifacts.** Do not create or require backups, snapshots, preimages,
   rollback plans, compare-and-swap gates, or remote captures. Disposable staging and
   deterministic regeneration provide safety.
3. **Active closure only.** Do not inventory, convert, port, accept, or create
   placeholders for disabled, unlisted, or demonstrably non-working source content.
   If an active instruction depends only on excluded content, disable and log that
   instruction instead of importing the excluded package.
4. **Behavior precedence.** Preserve equivalent Luminari behavior; otherwise implement
   clear intended gameplay, repair an obvious source defect, or disable the smallest
   irreducibly malformed unit. Never reproduce unsafe undefined behavior.
5. **Canonical namespace.** All active RoL-owned zones, rooms, mobiles, and objects use
   Section 3's formulas. Old and canonical copies may not coexist, and permanent VNUM
   aliases, compatibility portals, and forwarding records are prohibited.
6. **Preserve target work.** Existing target records and OLC edits are authoritative
   content evidence. Change them only through an explicit, evidence-backed `PATCH`,
   `MERGE`, replacement, or Phase 6.5 `REHOME`.
7. **Preserve identities.** Distinct active typed source VNUMs remain distinct.
   Duplicate definitions of the same typed source VNUM may merge; shared runtime code
   does not justify collapsing prototypes.
8. **Type every reference.** Blind numeric replacement is prohibited. Unknown syntax,
   ownership, collision, or required edge blocks emission unless the fixed repair or
   minimal-exclusion policy resolves it deterministically.
9. **Pre-approve HLQs.** Every converted HLQ entry receives `!`; there is no builder or
   human approval stage. Automated structure, behavior, reset, and walkthrough evidence
   replaces builder sign-off.
10. **Determinism.** Identical source, target, policy, and tool inputs produce
    byte-identical bundles. Reapplication is a no-op or fails a documented safe
    precondition.
11. **No unresolved final state.** `BLOCKED` is not a final record action. Investigation
    must end in a final action or a logged, high-severity smallest-unit exclusion.
12. **Engineering decisions stay technical.** Resolve reset representation, SOC
    implementation, adapters, VNUM mechanics, and related routing from traced evidence
    and tests rather than returning them for builder approval.
13. **Repository rules apply.** Outputs are ASCII text encoded as UTF-8 with LF
    endings. Runtime changes update tests, documentation/help, and both build manifests
    when source files are added or removed.
14. **Protected files stay untouched.** Never edit `src/campaign.h`,
    `src/mud_options.h`, `src/vnums.h`, `lib/mysql_config`, or `lib/.env`; update the
    relevant example template when a shipped configuration change is required.
15. **Rights are settled.** Full content rights and access are confirmed. Git ignores
    world data to protect exploration and spoilers, not because of licensing limits.

## 3. Canonical identity contract

### 3.1 Formulas and ranges

For a normalized source zone and every source entity:

```text
target zone VNUM   = normalized source zone VNUM + 20000
target entity VNUM = source entity VNUM + 2000000
```

Rooms, mobiles, and objects are separate typed entity namespaces. Shops, quests, and
SOC attach to their canonical keeper or host mobile. Target-owned external dependencies
retain their native VNUMs only when the reference ledger proves ownership.

The active normal source zone range reaches `9999`. Reserve zone range `20000-29999`
and entity range `2000000-2999999`. The entity range was empty in the locally assessed
world on 2026-08-11, but that is evidence, not a reservation. Phase 6.5 must recheck
world data, code, configuration, generated and synthetic records, and relevant
persistent state. A collision fails the contract; it does not authorize an exception
VNUM.

A zone may span multiple 100-VNUM entity bands. Shift its top by `2000000`, derive its
zone identity from the normalized source zone, and preserve sparse layout. Example:

```text
source Hulburg: #591, top 59599
target Hulburg: #20591, top 2059599
covered bands:  591-595 -> 20591-20595
```

The identity manifest is authoritative for provenance, type checking, duplicates,
merges, external dependencies, and reference closure. It may not replace the formulas
with legacy aliases or arbitrary exceptions. Any surviving source VNUM, legacy
`+1000`/`+100000` destination, or other RoL destination exception is an error to
classify.

### 3.2 Required normalization and attached identities

`mytheast.zon` is logical zones `817-818`, not its malformed `#81700` header. Its top
is `81899`, its entities and resets use `81700-81899`, and its manifest membership and
neighboring packages establish the error. Normalize it exactly as follows:

```text
source logical zones: 817-818
target zone record:    20817
target entities:       2081700-2081899
target top:            2081899
```

Do not retain `20002`, allocate `101700`, widen a range to conceal the defect, or infer
a general divide-by-100 rule. Any future normalization needs equivalent manifest,
range, contained-record, and typed-reference evidence.

Additional identity rules:

- Resolve each `SHOP:` keeper through the canonical mobile mapping, then validate the
  target shop namespace independently. Preserve an existing shop identity when present;
  otherwise a resolved keeper VNUM is only a candidate deterministic identity.
- Treat source quest headers as host mobiles, not independent quest VNUMs.
- Attach SOC behavior to canonical mobiles.
- Resolve typed literals in generated DG behavior through the identity manifest.
- Allocate synthetic DG triggers deterministically in a collision-free, OLC-valid range
  owned by the canonical destination zone, and record provenance.

### 3.3 Rehome and retirement

Known rehome targets are:

| Package | Legacy state | Canonical state |
|---------|--------------|-----------------|
| Trail | zone `1507`, `150xxx` entities | zone `20507`, `2050700+` entities |
| Hulburg | zone `1591`, `159xxx` entities | zone `20591`, `2059100+` entities |
| Jotunheim | zone `1960`, `196xxx` entities | zone `20960`, `2096000+` entities |
| Myth Drannor East | fallback zone `20002` | zone `20817`, `2081700-2081899` entities |
| Modern artifacts | `169901-169910` | eleven direct canonical entity VNUMs |

Regenerate the dated counts in the
[planning baseline archive](changelog-archive/archive08_13-canonical-plan-baselines.md)
before rehome work. Preserve six already-canonical Jotunheim additions during package
assembly.

Eleven source artifacts currently converge on ten target objects. Source objects
`1007` and `1009` both map to `169906`; restore distinct objects `2001007` and
`2001009`. Choose one evidence-backed canonical successor for existing `169906`
ownership and progression, carry modern behavior to the appropriate prototypes, and
never clone persistent state.

Existing target improvements and OLC edits move into canonical prototypes; they do not
justify aliases. Remove a legacy definition only in the validated cutover that supplies
its canonical definition and rewrites every required incoming reference. `REHOME` is a
Phase 6.5 migration operation; later regeneration still assigns the source record one
final action from Section 4.1.

## 4. Reconciliation and conversion controls

### 4.1 Record actions and capability dispositions

Every active source record receives exactly one final action:

| Action | Contract |
|--------|----------|
| `KEEP` | The canonical target record already has the intended content and behavior. |
| `PATCH` | Preserve canonical target content and local edits while applying a bounded change. |
| `ADD` | Create a missing canonical target record. |
| `MERGE` | Deterministically combine duplicate or overlapping definitions. |
| `EXCLUDE` | Disable and log the smallest irreducibly invalid active unit. |

The action ledger records source identity/hash, candidate target identities/hashes,
lineage and local-edit evidence, confidence, destination, dependencies, action, and
rationale. Filename, display text, or an old offset may nominate a candidate but cannot
authorize overwrite or choose a destination. Candidate nomination, `KEEP`, and bounded
`PATCH` use separate evidence thresholds.

Identity and content ambiguity are independent: the formula fixes an RoL-owned
identity; evidence chooses its content. Preserve unrelated target-owned candidates. If
no safe active content can be formed, use minimal `EXCLUDE`, not an alias, guessed
merge, or unresolved action.

Classify behavior separately:

| Code | Classification | Contract |
|------|----------------|----------|
| `N` | Native | Current Luminari behavior is equivalent. |
| `T` | Transform | Data or argument conversion is sufficient. |
| `A` | Adapter | A bounded compatibility layer preserves used semantics. |
| `P` | Port/new mechanic | Active content needs new runtime behavior. |
| `B` | Bug/fallback | Apply the fixed repair order and record the evidence. |
| `X` | Minimal exclusion | Disable the smallest irreducible active unit. |

`A` means adapter; RoL has no `A` reset opcode. `Dead`, `unused`, and `source no-op`
are evidence tags, not dispositions. Each capability row records source syntax and call
path, target evidence, occurrences/consumers, semantic contract, converter/runtime IDs,
fixtures, acceptance tests, loss policy, and any affected repair or exclusion.

Initial routes require call-path evidence, occurrence inventory, and fixtures before
they become authoritative:

| Construct | Initial route |
|-----------|---------------|
| Record grammar, flags, values | `T` through traced grammar and symbols |
| Base `M/O/P/G/E` resets and basic doors | `T` after chain/state proof |
| Extended/chance `D`; chance `R` | `A`, `P`, or `B` by used variant |
| Follow `F`; removal `X`; calendar `T` | `A` or `P`; never map by opcode letter |
| Conversational quests | Per-direction `N/T/A/P/B` |
| Five SOC modes | Choose from measured native versus DG fidelity |
| Room extensions/dimensions | `T`, `P`, `B`, or locked `X` |
| Object traps | `T` or `A` after an end-to-end audit |
| Special procedures | Reuse/patch first; `A` or `P` only for active gaps |
| Numeric SOC commands; legacy colors | `T` through traced name/token maps |

### 4.2 Typed graph and machine-readable controls

Build all definitions and edges before emission. Cover:

- exits, keys, containers, contents, portals, teleports, vehicles, and transports;
- every reset argument and chain, including rooms, prototypes, equipment, followers,
  removals, calendar predicates, doors, and trigger attachments;
- shop identity, keeper, rooms, products, and rules;
- HLQ hosts, inputs, rewards, loaded entities, and destinations;
- SOC owners, commands, path rooms, and generated DG behavior;
- typed object values and DG body literals;
- special bindings and their dependent records; and
- code, configuration, database, and non-database persistent consumers.

Typed namespaces may reuse numbers; never flatten them into one numeric space. Every
external edge needs explicit target ownership. A required unresolved edge blocks
emission or invokes the fixed smallest-unit exclusion. Never emit `NOWHERE`, zero, or a
guessed VNUM to hide a missing dependency. Classify raw numeric matches as active typed
references, immutable source/history, unrelated literals, documentation examples, or
generated artifacts to regenerate.

Maintain these versioned, machine-readable controls:

1. source and target inventories with run/tool/policy versions, root identity without
   credentials, paths, hashes, active membership, missing/orphaned files, revisions,
   exact validator commands, parsed finding identities/counts, and parse status;
2. a grammar inventory covering all observed versions, directives, values, boundaries,
   terminators, resets, shops, quest directions, SOC modes/actions, special bindings,
   typed references, counts, duplicates, companion coverage, and unknown tokens;
3. the record-action ledger and capability matrix described above;
4. a canonical identity/rehome manifest with normalization, typed source/canonical
   VNUMs, hashes, mapping rule, merge evidence, prior target, consumers, validation, and
   retirement status;
5. typed collision, forward-reference, reverse-reference, and retired-consumer reports
   covering world data, code, configuration, generated records, and persistent stores;
6. a conversion-contract registry linking grammar, normalized IR, capability IDs,
   emitter behavior, runtime version, ordering, validation, and diagnostics; and
7. a run/acceptance ledger with input and output hashes, selected records, actions,
   diagnostics, tests, engineering resolutions, and superseded run.

Regenerate every affected control and its dependents when an input changes.

## 5. Semantic conversion contracts

### 5.1 Rooms

- Map VNUMs, zone ownership, exits, keys, external references, descriptions, extra
  descriptions, moving-room data, and named procedures.
- Map flags and sectors symbolically. Document the approximation from source
  length/width/height to target room-size flags; target rooms have no direct dimension
  fields.
- Supply deterministic coordinates, required terminators, record versions, and index
  entries.
- Resolve the 18 raw duplicate room records before emission.

Rooms are the reference backbone; parse success alone is insufficient.

### 5.2 Mobiles

Parse every observed legacy version and reject unknown grammar. Map flags, affects,
positions, sex, attacks, race, class, alignment, abilities, saves, dice, gold, and
experience by symbolic meaning. Record defaults and losses for target-only or
unrepresentable fields. Remap every reset, shop, quest, follow, SOC, script, and special
consumer.

### 5.3 Objects and traps

Map item types, wear/extra/anti flags, applies, bonus types, materials, sizes,
proficiencies, economy fields, descriptions, and affects symbolically. Interpret value
slots by item type; a parseable value in the wrong slot is a gameplay defect. Resolve
spell and skill references by registered name, not number. Remap resets, shops, quests,
containers, keys, traps, and special consumers. Choose a trap representation only after
tracing load, trigger, persistence, OLC, and combat behavior end to end.

### 5.4 Shops

Convert keeper-owned source shops to target shop grammar. Preserve products, rooms,
buy/sell rules, hours, messages, and used AI extensions. Resolve keeper, room, product,
and shop identities independently through typed namespaces; source `SHOP:` is not a
standalone source shop VNUM.

### 5.5 High-level quests

Source `.qst` files are conversational HLQs, not target AutoQuests. Compile them by
direction, preserving:

- ask/topic matching, physical/runtime output order, and source list reversal versus
  target ordering;
- item, item-type, and coin inputs;
- item, coin, random-object-range, prestige, and experience rewards;
- load mobile/object, attack, disappear, teach, and other outputs;
- duplicate host headers, source lookup behavior, and target multi-block behavior; and
- `!` pre-approval on every entry.

Do not hide gaps behind one blanket transform. Source experience code is a no-op,
item-type input lacks a direct target equivalent, random ranges need an explicit
contract, and coin/disappearance/order behavior differs. Implement configured
experience, exact coin costs, item-type input, random rewards, disappearance, and
ordering through transforms or bounded adapters. Merge distinct intended duplicate-host
content; remove only proven duplicates. Exclude only an irreducibly malformed direction
so the rest of its host block remains usable.

### 5.6 SOC automated mobile actions

Source `.soc` files are mobile automation, not player social commands. Support all five
observed modes: `LIST`, `PATH`, `PERIODIC`, `TIMED`, and `TRIGGER`.

- Resolve numeric command ID to source command name/behavior and then to the target
  action; never reuse a current command-table index.
- Preserve chance, delay, ordering, arguments, messages, lifecycle, and path sequence.
- Test indoor, outdoor, all-zone, room-echo, and path semantics.
- Compare a native compatibility subsystem with DG compilation and bounded helpers by
  fidelity, OLC ownership, maintainability, observability, performance, synthetic
  record volume, and test cost.
- Attach behavior to canonical mobiles and own generated triggers canonically.

### 5.7 Zone metadata and resets

Source reset grammar is `M/O/P/G/E/D/R/F/X/T`; target grammar is
`M/O/G/E/P/D/R/T/V/J/I/L`, where target `T` attaches a DG trigger. Preserve effective
execution, conditional chains, limits, probability, source order, metadata, and active
state rather than opcode letters or argument positions.

| Source construct | Required treatment |
|------------------|--------------------|
| `M/O/P` | Map typed prototypes, rooms/containers, limits, chance, and chain state. |
| `G/E` | Map object, mobile context, equipment slot, and differing chance/argument positions. |
| `D` | Translate source bitmask/extensions/chance to proven target door behavior. |
| `R` | Preserve chance-qualified removal, not only native unconditional removal. |
| `F` | Implement follow/group/mount semantics; source `if_flag` is follow mode. |
| `X` | Implement room/global mobile removal, combat, and chance behavior. |
| `T` | Implement the calendar predicate; never map it to target trigger `T`. |

RoL has no `A` reset opcode. Preserve the two loader-accepted `F2 ...` rows without
whitespace as fixtures. Treat un-commented `GROUPING*` and `GATE QUEST STUFF` headings
as malformed source `G` rows, and `A Halruaan Airship 1~`-style lines as titles, not
commands. Do not silently execute or normalize malformed headings. Map lifespan, reset
mode, flags, bottom/top, and trigger attachments by meaning. Every used variant needs
an executable source fixture and expected target result.

### 5.8 Special procedures

Search current assignments, implementations, comments, and history before porting.
Reuse or patch equivalent/evolved target behavior first. Trace commands, spells,
objects, rooms, globals, persistence, scheduler assumptions, wrapper registrations, and
dynamic paths. Work only on active dependency-closure consumers. Use strict source
parsers and source-hashed profiles for regular families; keep irregular mechanics in
dependency-complete shared-runtime batches.

### 5.9 Cross-cutting transforms

- Convert legacy `&+X` colors with a token-aware lexer that preserves literal
  ampersands.
- Map flags, item types, sectors, races, classes, spells, skills, affects, applies, wear
  positions, and commands through traced symbol tables; never assume numeric equality.
- Emit correct record versions, `$`/`$~` terminators, and kind indexes.
- Resolve external dependencies to proven target-owned content, a canonical active
  record, or a hard failure/minimal exclusion.
- Retain kind, source path, original VNUM, and source hash for each emitted record.
- Report every default, repair, loss, dropped field, unsupported action, duplicate
  resolution, and engineering override.

## 6. Converter and release architecture

Use a standalone deterministic reconciler under `scripts/world/` or another dedicated
location chosen by its implementation spec. It must provide typed parsers for `wld`,
`mob`, `obj`, `shp`, `qst`, `soc`, and `zon`; a normalized typed IR; tested symbolic
maps; and emitters for target `wld`, `mob`, `obj`, `shp`, `hlq`, `trg`, `zon`, indexes,
and terminators.

Reconcile and generate before writing the development world. Phase 7 batches emit to
isolated staging and update shared ledgers; they do not each recreate a complete release
bundle. Application changes only declared paths, rebuilds indexes, applies runtime and
persistent dependencies in order, removes only declared legacy records, and validates
the resulting target. Never hand-edit ignored generated output as the source of truth.

A normal Phase 7 batch records only its selected inputs and outputs, action delta,
reference diagnostics, exceptions, and targeted validation result. A milestone or
release bundle contains machine-readable equivalents of:

```text
run-manifest.json          source-inventory.json    target-inventory.json
reconciliation.jsonl      capabilities.jsonl       identity-map.jsonl
rehome.jsonl              reference-report.json    change-plan.jsonl
removals.jsonl            output/world/            validation/
```

At milestones, also emit source/output structural comparisons, loss/exception reports,
fixtures for newly supported grammar, and the cumulative corpus progress ledger. Human
summaries derive from canonical records. Do not duplicate unchanged inventories,
manifests, or evidence in every batch.

For established grammar and runtime capabilities, reuse the existing converter and
tests. A new or changed capability includes traced source/target behavior, IR or runtime
work, diagnostics, representative fixtures, focused tests, and relevant
documentation/help. Require a separate pilot only for a new high-risk mechanic. Land
runtime support before dependent data.

## 7. Execution roadmap

One session has one objective, 12-25 tasks, and a 2-4 hour limit. Split any scope that
exceeds those bounds.

### 7.1 Phase 6: Complete

All 1,721 active direct bindings, both dynamic registration paths, all 247 implicit-race
bindings, all 795 direct handlers, and all 848 `ACT_SPEC` records have terminal
dispositions, with zero pending rows and zero live target writes. Authoritative evidence
is `lib/rol-conversion/runs/phase6-special-20260813-complete`, run
`rol-phase6-special-49381a429d6f4224`. Completed behavior and exclusion details are in
the [Phase 6 archive](changelog-archive/archive08_13-phase6-complete-RoL-Changelog.md).

### 7.2 Phase 6.5: Complete

Phase 6.5 completed on the authoritative development target on 2026-08-14. The sealed
release is `lib/rol-conversion/runs/phase6-5-canonical-20260814-release3-a`, run
`rol-phase6-5-a11f8a8181c2dd49`; `release3-b` is byte-identical. The release applies
180 planned path changes, records 270 evidence-backed repairs, and closes all four
machine-checkable namespace and reference invariants at zero.

The cutover rehomes Trail to zone `20507`, Hulburg to `20591`, Jotunheim to `20960`,
and ten first-wave artifacts to their direct canonical identities. It restores the
distinct second Kelrarin identity at `2001009`, migrates 112 persistent object files,
and preserves 18 unique artifact-state rows without cloning ownership. Global
validation improves the inherited target baseline from 3,849 errors and 38,219
warnings to 3,770 errors and 38,028 warnings. Normalized added findings and touched
blocking findings are both zero.

The supplemental persistence audit classifies all 83 semantic database bindings, of
which 53 require migration. The development execution migrated 1,512 rows and the
final verification finds zero retired row; all 142 distinct canonical saved-object
VNUMs resolve to exactly one indexed live prototype. The record-level completion audit
is `lib/rol-conversion/runs/phase6-5-completion-20260814-final`; it covers all 1,994
rehome/normalization records and all 191 explicit deliverables, session tasks, and exit
requirements.

Final gates pass 396 world-tool tests and 698 production-linked CuTests, a warning-free
build and install, syntax boot, a private-MariaDB behavioral boot, reset observation,
and scripted component and cross-zone walkthroughs. The behavioral boot executed the
artifact and three rehomed zone resets and reached the game loop without a relevant
diagnostic. The walkthrough reached all 990 rehomed rooms across 16 components and
resolved all 1,730 reset commands, 2,215 exit-key uses, 20 portals, and 568 typed
record references. Reapplication reports all 180 paths already current and zero file
writes while the transactional database migration remains idempotent.

The session lists below are retained as the executed completion record.

**Prerequisites:** keep RoL policy, runtime bindings, generated profiles, and planning
inputs stable; confirm development; and regenerate all dated counts and hashes in the
first session.

Phase 6.5 initially has eight sessions. It must classify every edge in the active source
inventory as canonical, target-owned, or an owned repair/exclusion. The cutover must
also rewrite every active edge that crosses a retired identity. Phase 7 may emit later
records, but no edge may enter it with unknown type, identity, or ownership.

#### Session 6.5.1: Freeze the canonical baseline

- Pin Phase 6 and policy versions; regenerate source, target, Phase 1, and Phase 2
  inventories and artifacts.
- Group every noncanonical typed identity by package, subsystem, and destination;
  revalidate source and reserved target ranges.
- Record `mytheast` evidence and inventory all Trail, Hulburg, Jotunheim, artifact,
  profile, code, configuration, database, and other persistent consumers.
- Generate hashed typed rehome and reverse-reference ledgers.

**Exit:** every noncanonical identity and old consumer has an owned rehome,
normalization, external-target, or exclusion row; no identity or collision is unknown.

#### Session 6.5.2: Enforce the universal resolver

- Version the canonical policy and replace legacy destinations with formula
  destinations while retaining lineage as content evidence.
- Implement evidence-backed normalization, including `mytheast`; reject `20002` and
  `101700` for that package.
- Enforce typed entity mapping, distinct identities, same-VNUM duplicate merging,
  canonical keeper/host/SOC ownership, and canonical trigger ownership.
- Test full reserved ranges plus low, typical, maximum, multi-band, overflow,
  malformed-header, ambiguous-ownership, and legacy-override cases.
- Regenerate twice and run focused and full world-tool suites.

**Exit:** planning artifacts contain no unexplained noncanonical destination; every
normalization is evidence-backed and deterministic.

#### Session 6.5.3: Rehome Trail, Hulburg, and Jotunheim

- Verify source and legacy target records against frozen hashes.
- Generate Trail `20507`, Hulburg `20591`, and Jotunheim `20960`, including six existing
  canonical Jotunheim additions, correct tops, and sparse layout.
- Carry target/OLC edits through explicit actions; rewrite every internal, incoming, and
  outgoing world edge; regenerate owned triggers and Phase 6 profiles/bindings.
- Stage removals without applying them, then test structure, syntax boot, resets,
  walkthroughs, behavior, and repeat generation.

**Exit:** all three packages stage only at canonical identities with accepted content
preserved and no unresolved typed edge.

#### Session 6.5.4: Rehome modern artifacts

- Trace all eleven source and ten target prototypes, behaviors, world consumers, and
  persistence contracts.
- Create eleven canonical prototypes, including distinct `2001007` and `2001009`, while
  sharing runtime code only where semantics permit.
- Choose one canonical successor for `169906` state; preserve improvements without
  cloning ownership or progression.
- Update registries, assignments, commands, cooldown/progression/ownership lookup, and
  summon, travel, spell, combat, and called-effect dependencies.
- Generate idempotent persistent migrations, rewrite consumers, stage obsolete
  `1699xx` removals, extend production-linked tests, and run artifact regressions,
  world tools, `make test`, and `make install`.

**Exit:** eleven canonical artifacts preserve enhanced behavior and unique state; no
active or persistent RoL consumer needs `169901-169910`.

#### Session 6.5.5: Close world-data references

- Build the complete staged typed definition/reference graph.
- Validate exits and intended reverses, keys, containers, portals, teleports, vehicles,
  transports, resets, shops, HLQs, SOC paths, DG attachments/body literals, and typed
  object values.
- Classify target-owned dependencies; reject source/retired VNUMs, missing definitions,
  unexplained externals, and typed collisions.
- Publish per-package incoming/outgoing reports and cross-zone fixtures; strictly
  validate every selected or touched zone.

**Exit:** zero unresolved required, retired RoL, or unexplained external edge.

#### Session 6.5.6: Close runtime and persistent consumers

- Classify every source/legacy numeric match in runtime, templates, generated data,
  documentation, tests, databases, and other stores.
- Replace active consumers and regenerate all special, periodic, state-aware, combat,
  death, weapon, utility, and event profiles. Update example templates if needed.
- Build a transactional, idempotent development migration and test representative old
  rows in an isolated database.
- Verify typed counts, uniqueness, save/reload, artifact ownership, progression,
  cooldown/account binding, and every discovered house, mail, auction, shop, quest, or
  other persistent consumer.
- Run the migration twice, boot staged data against the migrated database, reject
  relevant diagnostics, and publish the consumer ledger.

**Exit:** no active runtime, configuration, generated, or persistent consumer uses a
retired RoL identity; migration is lossless and its second run is a no-op.

#### Session 6.5.7: Assemble and validate the rebase

- Assemble one isolated target with canonical definitions, rewritten references,
  runtime support, and persistent migration, excluding planned legacy definitions.
- Run global/per-zone strict validation and compare exact finding identities and parse
  completeness with baseline.
- Run syntax and isolated-database boots, eligible reset observation, component and
  cross-zone walkthroughs, artifact/reload paths, and focused regressions.
- Run `make test-world-tools`, `make test`, and `make install`; verify no root `circle`.
- Check documentation drift, ASCII/LF, deterministic regeneration, preservation, and
  second apply.

**Exit:** the isolated target passes structural, behavior, persistence, determinism,
no-clobber, and idempotency gates.

#### Session 6.5.8: Apply and close the development rebase

- Reconfirm development and regenerate inventories immediately before apply; rebuild
  and revalidate any changed input.
- Apply canonical definitions, rewritten references, runtime/configuration changes,
  development database migration, and legacy removals as one cutover.
- Regenerate indexes/profiles and rerun strict validation, syntax and bounded behavior
  boots, persistence reload, namespace/reference audits, `make test-world-tools`,
  `make test`, and `make install`.
- Require no unexplained noncanonical identity, retired active definition/reference, or
  unmapped source VNUM in an active target field.
- Update this plan, roadmap, archived
  [worknotes](plan-archive/REALMS_OF_LUMINARI_WORKNOTES.md), archived
  [manual tests](plan-archive/PHASE4_MANUAL_TESTING.md), testing guide, artifact docs,
  help, and [RoL-Changelog.md](RoL-Changelog.md) with final evidence.

**Exit:** Section 9.1 passes on the authoritative development target; only then may
Phase 7 begin.

### 7.3 Phase 7: Complete - lean canonical corpus conversion

Phase 7 completed in 12 dependency-complete batches on 2026-08-14. The final sealed
checkpoint is `lib/rol-conversion/runs/phase7-final-20260814`, run
`rol-phase7-b12-a20bbc98e3513f98`; the independently generated
`phase7-final-repeat-20260814` tree is byte-identical. The cumulative ledger accounts
for all 258 active packages and all 71,680 selected records with terminal actions,
including 1,228 SOC triggers, 14 multi-binding special profiles, and 160 patched
records. Source parsing, runtime contracts, reference closure, preservation, and all
selected-record gates pass with zero staged new active error and zero live target
write. Milestone checkpoints were sealed after batches 4 and 8 before final closeout.

The execution rules below are retained as the completed Phase 7 record.

Convert all remaining active packages against the applied canonical baseline. A legacy
identity, canonical collision, or formula exception is a failed Phase 6.5 invariant;
repair that baseline instead of adding a local exception.

Freeze the post-Phase-6.5 policy, identity map, target inventory, and input hashes once
at Phase 7 start. Refresh them only when an authoritative input changes. Group packages
by dependency closure and shared runtime capability rather than zone-number order.

Each batch:

1. selects a dependency-complete group, including companion-only and cross-package
   dependencies;
2. assigns each selected record `KEEP/PATCH/ADD/MERGE/EXCLUDE` at its canonical
   identity and resolves its required references;
3. lands any required runtime capability before its dependent data;
4. generates isolated output while preserving Phase 6.5 and untouched target content;
5. runs targeted syntax, identity, reference, range, and no-clobber checks on selected
   and touched records;
6. runs focused reset, walkthrough, or behavior tests only for new, changed, or
   high-risk mechanics; and
7. updates one cumulative action, exception, and progress ledger instead of producing
   a full validation bundle per package.

Run a milestone check after every four to six batch sessions, and immediately after a
loader, persistence, or broadly shared runtime change. A milestone regenerates the
completed corpus from the frozen baseline, runs full structural validation and an
isolated syntax/database boot, samples the affected resets and walkthroughs, runs the
full code suites when code changed, verifies deterministic output and preservation,
and stores one checkpoint bundle. Phase 8 owns exhaustive full-corpus behavior,
persistence, integration, and post-apply testing.

Repair selected or touched baseline findings in their owning batch. Phase 7 exits when
every in-scope package, companion mismatch, record action, capability, and required
reference is resolved; targeted checks pass; and the final milestone can regenerate
the cumulative Phase 7 output from the sealed Phase 6.5 baseline.

The initial lean planning target is 18-30 sessions, or 36-120 focused engineering
hours at 2-4 hours per session. This includes baseline grouping, conversion batches,
milestone checks, and Phase 7 closeout, but not Phase 8. Reforecast after three
representative batches covering straightforward data, cross-package references, and a
runtime-heavy package. This is a working envelope, not a calendar promise.

### 7.4 Phase 8: Complete - final integration and release evidence

Phase 8 completed on the authoritative development target on 2026-08-14. The sealed
release bundle is `lib/rol-conversion/runs/phase8-release-20260814`, run
`rol-phase8-release-5992b9c59dd3055e`, with candidate tree
`39c05c7427b941e715491d129d83b17b73f89c332219ec577ea5bf2dc4662b20`.
Its hash-guarded plan applied all 1,201 paths and the repeat apply is a zero-write
no-op. The release reconciles 258 packages and 71,680 records with zero new active
error, a clean namespace audit, byte-identical repeat generation, and passing
preservation and runtime contracts.

Full-corpus behavior evidence covers 761 zones, 90,722 rooms, 26,427 mobiles, 22,273
objects, 1,148 shops, 6,296 HLQs, 3,216 triggers, 115,074 reset commands, 3,432 trigger
attachments, 77 object traps, and 34 exit traps. The production binary passes 409
world-tool tests, 699 CuTests, install, syntax boot, and bounded private-MariaDB runtime
boot with zero converted-zone diagnostic. The post-apply completion audit is
`lib/rol-conversion/runs/phase8-completion-20260814`.

The integration steps below are retained as the completed Phase 8 record.

Integrate the accepted cumulative Phase 7 output without repeating the namespace rebase
or legacy persistent-state migration. Split the following work into 6-10 sessions if
measured scope still supports that range:

1. freeze the canonical development baseline and accepted Phase 7 checkpoint;
2. assemble an isolated complete target with runtime before data;
3. reconcile full-corpus counts, actions, capabilities, identities, typed references,
   externals, exclusions, hashes, and indexes;
4. run global/per-zone structure, syntax/database boots, resets, walkthroughs, quests,
   shops, SOC, traps, specials, persistence, and regressions;
5. apply only the validated Phase 7 and integration plan to development and revalidate
   changed inputs;
6. rerun namespace and retired-reference audits;
7. finalize converter, operator, builder, testing, apply, system, help, and changelog
   documentation; and
8. publish reproducible final evidence and acceptance results.

A legacy identity or unresolved rebase consumer returns to Phase 6.5 remediation; a
package defect returns to its Phase 7 batch. Exit when the complete active closure is
reproducible at canonical identities, applied to development, playable, and satisfies
Section 9.2.

## 8. Validation and application gates

### 8.1 Structural validation

For an ordinary batch, record parsed findings for selected and touched records. Run the
full-corpus commands below at Phase 7 milestones and in Phase 8. Exit status alone is
insufficient because a wrapper may print errors while returning success.

```bash
python3 scripts/world/wtool.py \
  --world-root <isolated-lib-root>/world validate --all --strict

lib/world/validate-zone.sh <zone-vnum> \
  --world-root <isolated-lib-root>/world --strict

bin/circle -c -d <isolated-lib-root>
```

Validate indexes, versions, terminators, ranges, typed collisions, ASCII, and LF. Every
batch must leave its selected or touched records without an unresolved error. A
milestone must add no normalized global finding. The `validate` command always
requires a selector such as `--all`.

### 8.2 Runtime and behavior validation

Use the smallest safe focused harness for an ordinary batch. Use an isolated lib root
and isolated MariaDB instance at milestones, or sooner when a batch changes loaders,
persistence, shops, HLQs, SOC, traps, specials, or other database-backed behavior.
Reuse or adapt `scripts/ci/prepare_test_runtime.sh` and its safety checks; never run
destructive fixtures against development or production databases. Reject relevant
invalid-record, reference, reset, trigger, persistence, extraction, and `SYSERR`
diagnostics.

Test representative changed behavior in each batch. Expand reset and scripted
walkthrough coverage at milestones, prioritizing new mechanics and cross-zone paths.
Phase 8 provides exhaustive full-corpus coverage for doors, keys, containers,
equipment, shops, HLQs, SOC, paths, traps, specials, artifacts, and persistent reloads.
No manual builder sign-off is required.

### 8.3 Code, preservation, and apply validation

When runtime or converter code changes, run focused tests immediately. Run the full
suites below at the next milestone and before the Phase 7 handoff:

```bash
make test-world-tools
make test
make install
```

`make install` must remove the root-level `circle`. Add or remove source files in both
`Makefile.am` and `CMakeLists.txt`.

For every batch generation:

- untouched paths retain hashes and target/OLC content changes only by declared action;
- canonical and synthetic identities are collision-free and all required references
  resolve through the typed manifest;
- runtime precedes dependent data; and
- canonical definitions and rewritten references precede legacy retirement.

At each milestone, identical frozen inputs must produce byte-identical cumulative
output. Repeat application to a disposable copy must be a no-op or an explicit safe
stop. If persistent data is touched, verify scoped, transactional where supported,
idempotent save/reload behavior. Phase 8 repeats the complete structure, namespace,
reference, syntax, behavior, persistence, preservation, and code gates against the
assembled development candidate.

## 9. Acceptance criteria

### 9.1 Phase 6.5 exit

Phase 6.5 passes only when:

1. each active source zone has one evidence-backed normalized identity at source zone
   plus `20000`;
2. each active, non-excluded room/mobile/object is at source VNUM plus `2000000`, with
   distinct typed identities except duplicate definitions of the same source identity;
3. `mytheast` is zone `20817` with entities `2081700-2081899`;
4. Trail, Hulburg, and Jotunheim exist only at their canonical RoL identities;
5. eleven modern artifacts exist at direct canonical VNUMs with unique persistent
   state;
6. no world, runtime, configuration, generated, test, policy documentation, database,
   or other persistent consumer needs a retired RoL identity;
7. every typed cross-zone, key, quest, shop, reset, portal, SOC, and DG edge resolves,
   and all canonical/synthetic identities are collision-free;
8. target/OLC edits change only through evidence-backed actions;
9. migration loses or duplicates no ownership, progression, inventory, or discovered
   persistent state;
10. the isolated target adds no baseline finding and touched records have no unresolved
    finding;
11. syntax/database boots, resets, walkthroughs, focused tests, world tools, CuTests,
    and installation pass;
12. generation is byte-identical, repeat apply is safe, and the applied development
    target passes the same audits;
13. current documentation states the canonical rule and gives no legacy identity
    precedence; and
14. no unexplained exception, unresolved decision, or final `BLOCKED` identity remains.

Machine-checkable summary:

```text
noncanonical active RoL zone identities   = 0
noncanonical active RoL entity identities = 0
active references to retired RoL VNUMs    = 0
unresolved required typed references      = 0
```

This gate passed on 2026-08-14 in run `rol-phase6-5-a11f8a8181c2dd49` and again
against the applied development target. All four values above are zero; generation is
byte-identical and repeat application performs zero file writes.

### 9.2 Project definition of done

The conversion is complete only when:

1. reproducible inventories account for every active package, companion, record, and
   run input;
2. every source record has a final canonical action, and every active behavior has a
   tested capability disposition;
3. every canonical/synthetic identity and typed reference resolves without collision
   or legacy alias, while target edits remain preserved or explicitly changed;
4. every repair, default, loss, and minimal exclusion is explicit and tested;
5. generated grammar, ranges, indexes, terminators, ASCII, and LF are valid, touched
   records have no unresolved finding, and the assembled target adds no baseline
   diagnostic;
6. Phase 7 targeted checks and milestones pass, and Phase 8 full-corpus syntax/database
   boots, behavior, resets, walkthroughs, persistence, regressions, no-clobber,
   determinism, and idempotency pass with a cumulative ledger and final validation
   bundle;
7. the complete result is applied to development and passes post-apply audits;
8. converter, operator, builder, testing, apply, system, help, and changelog
   documentation matches behavior; and
9. recorded inputs and versioned policy can regenerate and safely reapply the result
   with no unresolved decision or final `BLOCKED` action.

## 10. Priority, exclusions, and handoff

Prioritize work by:

```text
priority = pilot blocking + dependency fan-out + corpus frequency + reuse value
```

Do not automate the ranking. A rare construct may lead when it protects identity,
blocks safe staging, or has high fan-out.

Out of scope:

- disabled, unlisted, or demonstrably non-working source content;
- balance changes unrelated to conversion correctness;
- compatibility duplicates, forwarding records, or permanent VNUM aliases;
- renumbering target-owned Luminari content;
- production files, services, or databases;
- protected local configuration or credential changes; and
- backup, rollback, recovery, or remote-capture artifacts.

Current handoff:

```text
sealed Phase 6.5 canonical baseline
-> sealed and byte-identical Phase 7 cumulative corpus
-> applied Phase 8 development release
-> reproducible completion evidence and a zero-write repeat apply
```

The conversion through Phase 8 is complete. Future maintenance must preserve the
canonical identity contract, regenerate from the recorded inputs and policy, and may
not restore legacy identity precedence.
