# RoL Conversion Completed Work

Status: Completed-work record; active tasks are kept in the remaining-work plan

Last reviewed: 2026-08-17

## Purpose

This file records RoL conversion work that is already implemented and verified. It
does not contain future tasks. The unfinished project scope is maintained separately
in [ROL_CONVERSION_ACCURACY_RECOVERY_PLAN.md](ROL_CONVERSION_ACCURACY_RECOVERY_PLAN.md).

## Completed structural conversion

The first RoL conversion moved the selected source world into a separate Luminari
namespace. The accepted source inventory selected 258 packages and produced 71,680
record actions. The accepted Phase 8 overlay added 1,206 world-data files and replaced
the seven relevant world indexes.

The completed conversion established these safety properties:

- RoL zones use the reserved range 20000-29999.
- RoL rooms, mobiles, objects, and typed owners use 2000000-2999999.
- Existing Luminari identities are not aliases for similar RoL content.
- Generation occurs in a staging world before a development write.
- Release candidates are hash-guarded and development-only.
- Reapplying the same accepted bundle is a verified no-op.
- The rejected Phase 6.5 rehome and its persistence mutation/recovery tooling are
  absent from the active conversion system.
- The original overlay was additive and protected pre-existing Luminari records.

This work proved deterministic structure, namespace isolation, baseline-relative
validation, and safe additive application. It did not claim complete semantic
fidelity; that recovery remains in the active plan.

## Completed database-boundary cleanup

The active converter database surface was re-audited on 2026-08-17. Baseline capture,
discovery, planning, generation, capability analysis, special reconciliation, and
Phase 7 do not connect to MariaDB. The rejected rehome, migration, recovery, database
target-selection, backup, and completion-audit command paths and modules were removed.

The only maintained RoL database operation is the release persistence check. It is
fixed to `lib/mysql_config`, requires `APP_ENV=development`, accepts only one `SELECT`
or `SHOW`, starts every connection in a read-only session, and compares stored RoL
VNUMs with the assembled candidate without changing stored data. The current schema
exposes all 70 tracked world-VNUM bindings. The current development data contains one
RoL reference row, object 2019912, and both the installed world and the independently
assembled regenerated candidate define that object exactly once.

Two independent full Phase 7 runs produced the same run ID and byte-identical output
tree. The complete world-tool gate passed 427 tests, source-constant verification,
documentation validation, and the zone-wrapper smoke test after this cleanup and the
runtime-release audit correction described below.

## Completed full-namespace candidate regeneration

Phase 7 now copies the current development world into staging, removes every record
in the canonical RoL zone and entity ranges, preserves every other record block, and
regenerates the complete namespace from the frozen source and versioned rules. This
eliminates dependence on already-installed generated records and makes repeated runs
against an installed converted world idempotent.

Two independent 258-package runs emitted all 71,680 selected source records, 1,206
generated files, 1,228 SOC triggers, and 14 special triggers. They produced the same
run ID and the same output-tree SHA-256. The preservation audit covered 5,010 files,
found zero removed paths and zero undeclared changed paths, and passed. The connection
graph, runtime contract, staged validation delta, source parsing, reference-exception,
special-binding, and all-selected-record disposition gates also passed.

This completes candidate-side full-namespace replacement and non-RoL preservation.
Built-in apply journaling, automatic restore, an explicit rollback command,
spawned-state comparison, gameplay validation, soak, and deployment remain in the
active plan.

## Completed recovered-candidate runtime boot

The recovered mobile converter was rerun twice on 2026-08-17. Both Phase 7 runs used
258 packages, selected all 71,680 records, emitted 1,206 files, 1,228 SOC triggers,
and 14 special triggers, and produced run ID `rol-phase7-b12-2453b0a517fba84f`.
Their output overlays were byte-identical with SHA-256
`75aac0f865abe6f33d2e10f8dd6d3db6d55faf509b629348bde3b57d44d0dc94`.

The independently assembled complete candidate tree matched the sealed Phase 7 tree
SHA-256 `1fc9719d13799bcda756d71618c890e5252f32085b3647f07a27627073d3a5a3`.
It introduced zero active validator errors relative to the frozen development
baseline and contained zero active errors in the RoL namespace. The read-only
persistence audit used the one configured development database, found one persisted
RoL VNUM row, and proved that its object 2019912 target exists exactly once.

The exact installed runtime passed 756 production-linked CuTests. A syntax boot
loaded 764 zones, 91,738 rooms, 27,067 mobiles, and 22,640 objects from the isolated
candidate and shut down normally. A bounded live boot reset the complete RoL zone
range, entered the game loop, ran for 71 seconds, and terminated normally. The syntax
and runtime logs contained zero converted-world diagnostics. Existing low-VNUM
`ITEM_AUTOPROC` diagnostics and unavailable optional Ollama and loopback I3 services
were outside the converted namespace and did not prevent startup.

A parsed-candidate field audit covered all 12,407 generated RoL mobiles. Every one
has nonempty explicit `Race:`, `SubRace 1:`, `SubRace 2:`, `SubRace 3:`, `Class:`,
`Size:`, `Tier:`, ability, save, and spell-resistance fields. No generated mobile uses
the signed `MOB_TIER_UNSPECIFIED` value. The tier distribution is 9,934 Standard, 153
Elite, 1,974 Small Group, 269 Big Group, 75 Raid, and the two World Boss forms.

The first Phase 8 release audit exposed two false recovery assumptions: it rejected
correctly namespaced mechanics already present in the development baseline and
source-internal merge destinations already present in the generated RoL namespace.
The audit now rejects only mechanics outside the reserved namespace, noncanonical
destinations, and blocking selected-record findings. Focused regression tests and the
427-test world-tool suite passed. The repeated Phase 8 release
`rol-phase8-release-873ca106400e38f0` passed every code, namespace, persistence,
connection-graph, mechanics-isolation, runtime-contract, validation-delta, and repeat
generation gate and reported `ready_to_apply: true`.

This completed isolated candidate generation, parser boot, full zone reset, bounded
runtime, and release sealing. The development installation and installed-world boot
are recorded below.

## Completed development installation and installed-world boot

Before installation, the complete 5,010-file `lib/world` tree was archived with
SHA-256 `e6ac48d64d3fdbda0de7a63734086ef345830ca4243a53a26ace8ff99e8eba0c`.
An independent archive-content audit reproduced the exact pre-apply tree SHA-256
`945c0be3c814542da3eddd4d0479b0cf78d0faca37e66d163a5224ed24772847`.

The accepted Phase 8 release was then applied to the normal development
`lib/world`. Of 1,206 guarded paths, 222 changed and 984 were already identical. The
installed tree exactly matches candidate SHA-256
`1fc9719d13799bcda756d71618c890e5252f32085b3647f07a27627073d3a5a3`.
A repeat apply changed zero paths, and the read-only persisted-VNUM check still found
object 2019912 exactly once.

Post-apply completion audit `rol-phase8-complete-fe12439a01d7b4e8` proved that the
development tree and normalized validation result match the accepted candidate, a
repeat apply is a no-op, documentation passes, and runtime conversion code is
unchanged from the gated release.

The installed world passed syntax boot from `bin/circle -c -d lib`. A normal
`bin/circle -d lib` live boot loaded the same 764 zones, 91,738 rooms, 27,067 mobiles,
and 22,640 objects, reset all 248 generated RoL zones across the sparse 20000-29999
namespace, entered the game loop, ran for 72 seconds, and terminated normally. The
installed-world log contains zero converted diagnostics. The process and its
auxiliary listeners were gone after shutdown, and only the original local MariaDB
process remained.

The verified archive makes this particular installation manually recoverable.
Built-in write-ahead journaling, automatic restoration after a partial failure, and
an explicit `rol-phase8-rollback` command remain unfinished.

## Completed recovery ownership decision

No building or OLC editing will occur in the Realms of Luminari namespace until the
conversion is accurate, validated, and locked. During recovery, the frozen RoL source,
versioned conversion rules, authoritative calculator, and generated candidate are the
only sources of truth.

Therefore the currently installed converted RoL records are disposable generated
output. Reconversion will replace the complete proven RoL namespace; it will not
preserve current RoL edits, perform three-way merges, or create builder conflicts.
Safety work is limited to proving the RoL regeneration boundary, preserving every
non-RoL Luminari byte, validating removals and references, and providing exact backup
and rollback. A later decision can establish builder workflow only after the
conversion is locked.

## Completed mobile mechanics study

The mobile-scaling investigation traced the source and target loaders, the in-game
`autoroll_mob()` path, runtime combat modifiers, world-file encodings, OLC, and the
current converted corpus.

The study established that:

- old Luminari levels 31-34 implicitly combined competence and encounter strength;
- the old ladder supplied increasing HP, armor, damage, attack, extra-attack,
  critical-confirmation, and critical-defense-bypass pressure;
- five mutually exclusive encounter roles would not fit safely in the two remaining
  mobile action bits;
- encounter role therefore belongs in one persisted scalar field instead of five
  physical `MOB_*` flags;
- level should represent competence while tier represents intended encounter size;
- the copied RoL hitroll uses a different meaning from Luminari's inverse-encoded
  mobile-file field;
- RoL source levels above 34 had been collapsed to target level 34; and
- automatic-stat class treatment and the configurable class-category table disagree
  for at least Sorcerer and Bard, although current 100-percent settings hide it.

The indexed-corpus study found 5,617 of 12,406 active source mobiles above level 34.
It also measured the approximate median spawned HP gap that motivated the tier
compatibility seed:

| Target level | Native Luminari mobiles | Converted RoL mobiles |
|-------------:|------------------------:|----------------------:|
| 31 | 2,800 | 2,300 |
| 32 | 5,800 | 2,600 |
| 33 | 9,700 | 2,900 |
| 34 | 14,700 | 7,900 |

The same study matched native high-level prototype HP additions to the existing
automatic-stat formula:

| Target level | Exact native autoroll matches | Share of native mobiles |
|-------------:|------------------------------:|------------------------:|
| 31 | 313 of 359 | 87.2 percent |
| 32 | 127 of 164 | 77.4 percent |
| 33 | 546 of 672 | 81.2 percent |
| 34 | 405 of 577 | 70.2 percent |

These measurements were used as compatibility evidence, not as final encounter
balance approval.

## Completed race, subrace, and size trace

The mobile identity trace established that Luminari supports one broad NPC race
family plus exactly three NPC subrace slots. The slots are stored in
`mob_specials.subrace[MAX_SUBRACES]`, loaded from `SubRace 1:` through `SubRace 3:`,
and written back by GenOLC. At the time of the trace, the RoL converter emitted only
the broad `Race:` mapping and did not emit those three subrace fields. The recovery
implementation below closes that gap.

The same trace established that `autoroll_mob()` is not a general size calculator.
It reads the current size only in the Giant race branch, where it tries to raise a
mobile smaller than Large to Large. Prototype and live-mobile calls do not handle
that mutation consistently because current size and real size are separate fields,
and the live path can overwrite the Giant assignment later in the function.

Accordingly, the implemented recovery design makes automatically resolved source identity
authoritative: the converter selects up to three subraces, determines and holds final
size outside the calculator, and assigns that retained final size once after automatic
stats. No converted mobile requires per-mobile human classification.

## Completed source-to-target mobile field inventory

The field audit traced the physical RoL record grammar through the RoL loader, the
current Python parser and emitter, and Luminari's loader and GenOLC serializer. It
established that a normal RoL mobile record contains:

- one vnum and four tilde-terminated text fields;
- one action mask, two affect masks, alignment, and the `S` format marker;
- race code, base height, base weight, and an optional race-aggression list;
- level, direct hitroll, bounded armor, HP dice, and damage dice;
- either aggregate money or four coin denominations, followed by experience; and
- current position, default position, sex, optional class, optional magic resistance,
  and optional prestige bonus.

The audit also established that a syntactically valid target record does not prove
value fidelity. At the time of the audit, the emitter ignored height, weight, the
optional aggression list, magic resistance, and prestige. It directly copied combat
fields whose target encodings differ, flattened currency without owning target
custom-gold behavior, and did not account for source loader changes to rewards and
spawned stats. The recovery implementation below resolves these ownership gaps.

The active-corpus inventory measured:

- 228 active mobile files: 227 declare source file version 1, while the empty
  `end.mob` sentinel declares version 0;
- 12,407 parsed mobile headers, of which 12,406 have all four expected numeric rows;
- 73 active race codes, 356 valid optional race-aggression lists, 65 positive authored
  heights, 100 positive authored weights, and only 61 records with both dimensions
  positive;
- 11,350 three-value final rows, 591 four-value rows, 344 five-value rows, and 121
  six-value rows; therefore 1,056 records author a class field, 465 author a
  magic-resistance field, and 121 author a prestige field;
- 286 positive effective explicit magic-resistance values, 108 positive prestige
  bonuses, and 13 negative prestige values that the source reward code ignores;
- five invalid sex values and four level-zero special or utility records; and
- one missing position row, two malformed race rows, and one money row with a trailing
  token ignored by the source loader.

All active action and affect bits have a current code-level disposition: mapped,
adapted, deferred to special reconciliation, or explicitly source-only. Their final
behavioral acceptance remains unfinished and is kept in the remaining-work plan.

The target-side inventory also covered every enhanced mobile field accepted by
`interpret_espec()` and emitted by GenOLC. It identified a persistence hole:
Luminari had runtime spell resistance and automatic stats could assign it, but mobile
records had no canonical enhanced field that loaded and saved the value. The recovery
implementation below closes that persistence hole.

## Completed encounter-tier design

The implemented scalar values are:

| Value | Name | Encounter role |
|------:|------|----------------|
| 0 | `MOB_TIER_STANDARD` | Ordinary same-level mobile. |
| 1 | `MOB_TIER_ELITE` | Tough solo encounter. |
| 2 | `MOB_TIER_SMALL_GROUP` | Intended for roughly two or three players. |
| 3 | `MOB_TIER_BIG_GROUP` | Intended for roughly four to six players. |
| 4 | `MOB_TIER_RAID` | Intended for a larger coordinated group. |
| 5 | `MOB_TIER_WORLD_BOSS` | Individually designed boss with the Raid result as its generic floor. |

`MOB_TIER_FORMULA_V1` preserves the former high-level combat shape while separating
it from level. Let `B` be the positive ordinary automatic-stat HP budget and let `t`
be 1 for Elite, 2 for Small Group, 3 for Big Group, or 4 for Raid:

```text
H = (2 * t * B) + 500
repeat t times:
    H = H + floor(H / 10)
```

Standard uses `H = B`. World Boss uses the Raid result as its generic floor. The
implemented non-HP modifiers are:

```text
conditional_attack_bonus(t)   = t + 1 + max(0, t - 2)
armor_class_bonus(t)           = t + 1
damage_bonus(t)                = t
extra_max_bab_attacks(t)       = t
critical_confirmation_bonus(t) = 2 * t
defense_bypass_percent(t)      = 20 + (10 * t)
saving_throw_bonus(t)          = 0
spell_resistance_bonus(t)      = 0
```

The resulting compatibility rows are:

| Tier | HP calculation | Conditional attack | Armor | Damage | Extra attacks | Critical confirm | Defense bypass |
|------|----------------|-------------------:|------:|-------:|--------------:|-----------------:|---------------:|
| Standard | `B` | +0 | +0 | +0 | +0 | +0 | 0 percent |
| Elite | compound `(2B + 500)` once | +2 | +2 | +1 | +1 | +2 | 30 percent |
| Small Group | compound `(4B + 500)` twice | +3 | +3 | +2 | +2 | +4 | 40 percent |
| Big Group | compound `(6B + 500)` three times | +5 | +4 | +3 | +3 | +6 | 50 percent |
| Raid | compound `(8B + 500)` four times | +7 | +5 | +4 | +4 | +8 | 60 percent |
| World Boss | Raid floor | +7 floor | +5 floor | +4 floor | +4 floor | +8 floor | 60 percent floor |

For the level-34 warrior baseline `B = 1,496`, the exact Standard through Raid HP
vectors are 1,496, 3,841, 7,845, 12,611, and 18,252. For the level-34 arcane baseline
`B = 598`, the Elite through Raid vectors are 1,865, 3,499, 5,439, and 7,735.

## Completed encounter-tier implementation

### Data model and persistence

- `struct mob_special_data` contains a signed scalar tier field.
- `GET_MOB_TIER()` exposes the field to runtime and OLC code.
- Enhanced mobile records accept and emit `Tier: 0` through `Tier: 5`.
- Invalid explicit tier values stop the runtime loader.
- The Python world validator reports invalid and non-integer tier values.
- New MEDIT mobiles begin at Standard.
- Older records without `Tier:` use the signed `MOB_TIER_UNSPECIFIED` sentinel.

### Builder and inspection support

- MEDIT displays and edits one named, mutually exclusive encounter tier.
- `stat` displays the effective tier.
- Old omitted fields are visibly labeled `Legacy` instead of pretending they were
  explicitly classified.
- Builder documentation explains the intended player-count roles and the required
  race/subrace/size/class/level/tier autoroll order.

### Autoroll and runtime behavior

- `src/mob/mob_autoroll.c` contains the versioned tier calculations.
- Prototype autoroll stores exact generated HP as `1d1+(H-1)`.
- Tier armor and damage are applied to persisted autoroll results.
- Conditional attack bonus and its cap increase are derived from tier.
- Extra maximum-BAB attacks are derived from tier.
- Critical-confirmation bonus is derived from tier.
- Critical-defense bypass is derived from tier and uses an inclusive percentage
  boundary.
- Explicit tier results do not receive the former level-31-to-34 HP or armor bonuses
  a second time.

### Legacy compatibility

Records with no `Tier:` line temporarily derive Standard at level 30 or below and
Elite through Raid at levels 31 through 34. Their old spawn-time HP addition and
runtime armor behavior remain in place until explicit classification. Autorolling a
Legacy mobile resolves and stores its effective tier, after which the explicit tier
owns the modifiers exactly once.

This compatibility preserved existing high-level content during migration without
silently classifying every level-34 mobile as Raid.

### Build and test integration

- The new source and production-linked test are registered in both `Makefile.am` and
  `CMakeLists.txt`.
- Golden-vector tests cover warrior and arcane HP budgets.
- Modifier tests cover Standard through Raid and the World Boss Raid floor.
- Invalid tier, non-positive HP, null output, and overflow inputs are rejected.
- Legacy level fallback is tested.
- End-to-end autoroll tests cover every tier and prove exact HP serialization, armor,
  and damage application.
- Mobile parser tests cover valid and invalid `Tier:` records.
- The generated web mobile-flags guide was refreshed from its Markdown source.

## Completed mobile accuracy-recovery implementation

The mobile-only recovery slice was implemented on 2026-08-17 without writing a
candidate into `lib/world` and without deploying production. It replaces raw-source
combat copying with deterministic target-native selection and calculation.

### Strict source grammar and automatic repairs

- The source parser now enforces the actual four-row simple-mobile grammar instead of
  accepting arbitrary token rows.
- Exact, hash-bound policy rules own the one missing position row, two malformed race
  rows, one ignored trailing money token, five invalid sex values, and four authored
  level-zero utility records.
- The active 12,407-mobile corpus parses completely with exactly 13 warning-level
  repair diagnostics and no unowned syntax defect.

### Deterministic identity and encounter selection

- A frozen, hash-checked source registry extracts all 75 RoL race codes and their
  runtime names, dimensions, and magic-resistance caps from the source engine.
- The versioned policy contains one complete atomic base profile for every code,
  prioritized whole-phrase contradiction rules, and exact hash-bound record rules.
- Every mobile receives one symbolic broad target race, three explicit persisted
  subtype slots, one automatically selected final size, a mapped class, mapped level,
  and explicit encounter tier before calculation.
- The resolver covers all 12,407 active mobiles, including all 258 mixed-bucket
  records and all 2,542 authored level-51+ records, with only `AUTO_EXACT`,
  `AUTO_PHRASE`, or `AUTO_FALLBACK` outcomes. It creates no manual-review state.
- Aggression race lists are parsed and reported separately from owner identity. All
  356 active lists receive the reviewed bounded exclusion because the target has no
  safe equivalent race-list aggression primitive.
- Source prestige has an explicit exclusion policy for all 108 positive and 13
  negative authored values. Target-native automatic rewards own experience and gold,
  and every emitted mobile explicitly owns `MOB_CUSTOM_GOLD`.

### One authoritative automatic-stat implementation

- `mob_autoroll_calculate()` is a side-effect-free C profile for the complete ordinary
  automatic-stat calculation. It accepts explicit level, race, class, tier,
  versioned configuration, and a symbolic custom profile.
- The function rejects missing or invalid tier and identity inputs, uses checked
  percentage and tier arithmetic, applies tier exactly once, and returns both the
  persisted profile and expected post-load state.
- `autoroll_mob()` remains the production `char_data` wrapper and now delegates to
  that profile. The hidden Giant minimum-size mutation and live size copy-back were
  removed; callers retain ownership of final size. Dynamic wilderness encounters now
  restore their table-selected size explicitly after autoroll.
- Sorcerer and Bard now consistently use the Arcane configuration category in both
  calculation and load-time category treatment.
- The standalone `rol_mob_calculator` utility uses a strict versioned line protocol
  and one persistent process. It echoes and validates level, race, class, tier, and
  custom-profile inputs, reports its executable hash, and fails closed on identity,
  protocol, process, or response errors.
- Python contains no production copy or fallback for the stat formulas. The converter
  inverse-encodes calculator hitroll and armor, persists exact HP as `1d1+(H-1)`, and
  serializes every generated ability and save.

### Persistence, ledgers, and named profiles

- `SpellRes:` is a canonical enhanced mobile field with range-checked runtime load,
  GenOLC save, MEDIT display/edit, Python validation, and converter emission.
- Source explicit magic resistance wins when valid; otherwise the converter derives
  the same race-and-level value as the source loader and persists it once without
  stacking the target Dragon baseline.
- Each emitted result carries calculator version/hash evidence, the complete identity
  decision, repairs, rewards, aggression and prestige dispositions, physical-field
  dispositions, serialization inverses, and source/target loader consequences.
- Living Tiamat and the dracolich phase have separate exact, hash-bound World Boss
  profiles. Their 29,999 and 30,000 HP budgets are owned inside the C calculator,
  emitted with `MOB_CUSTOM_MOB_STATS`, and checked against the existing two-phase
  runtime behavior and special-procedure bindings.
- Target race, subtype, size, class, tier, and custom-profile numbers come from the
  generated C constants manifest; Python policy stores symbolic names rather than
  duplicated numeric values.

### Completed deterministic generation contract

The implementation follows the generation contract that previously lived in the
active plan:

1. Clamp and map positive source level with
   `min(34, (3 * min(source_level, 59) + 4) / 5)`; exact automatic profiles own the
   four level-zero utility records.
2. Select symbolic broad race, zero through three meaningful subraces, and class.
3. Determine final size from exact identity, complete positive authored dimensions,
   source-race average dimensions, or a source-code fallback, in that order.
4. Select an explicit encounter tier before calculation. Level does not itself choose
   tier, and World Boss requires a named profile.
5. Invoke the authoritative calculator with level, race, class, selected tier,
   versioned configuration, and any approved custom profile.
6. Calculate the ordinary profile and apply the submitted tier exactly once.
7. Apply a hash-bound custom profile when the exact-record rule requires one.
8. Assign the retained final size after calculation and serialize the complete target
   identity and generated statistics.

The calculator neither accepts nor returns size or subraces. The bridge verifies that
the helper echoes level, race, class, tier, and custom profile unchanged. Generated
output persists explicit tier and three subrace slots; unused slots are
`SUBRACE_UNKNOWN`. Legacy level-derived tier behavior remains only as compatibility
for old records that omit `Tier:`.

Identity resolution uses this fixed evidence order:

1. A stable, hash-bound exact-record rule.
2. The complete frozen RoL runtime race-code registry.
3. A closed whole-word or whole-phrase rule, with compound phrases winning through
   explicit priority, phrase specificity, and stable rule ID.
4. The source code's mandatory atomic base fallback.

Alignment, flight, swimming, temporary affects, equipment, zone theme, reset
neighbors, and aggression targets do not invent owner identity. Rules return one
complete profile rather than unioning subtype lists. The ledger retains the exact RoL
code and name because the target NPC schema cannot preserve every source species
below its broad family and three-subrace representation ceiling.

### Completed source-code base profile matrix

The policy now contains the complete symbolic fallback matrix below. Exact and phrase
rules may replace a row atomically, but no known source code falls through to a silent
Human default or a manual-review state.

| RoL code(s) | Target family | Target subtype(s) |
|-------------|---------------|-------------------|
| `A AA AB AC AD AE AF AH B BR` | Animal | none |
| `F` | Animal | Aquatic |
| `R RS` | Animal | Reptilian |
| `AP AS I` | Vermin | none |
| `AY` | Magical Beast | none |
| `D` | Dragon | none |
| `E1` | Outsider | Extraplanar, Fire |
| `E2` | Outsider | Extraplanar, Air |
| `EA` | Elemental | Air, Extraplanar |
| `EE` | Elemental | Earth, Extraplanar |
| `EF` | Elemental | Fire, Extraplanar |
| `EW` | Elemental | Water, Extraplanar |
| `G PO PT` | Giant | none; Large minimum |
| `H H2 HH HO P2 PB PD PE PF PG PH PL PM PR` | Humanoid | none |
| `HG` | Humanoid | Goblinoid |
| `HK` | Humanoid | Reptilian |
| `HC MS` | Monstrous Humanoid | none |
| `PY RH` | Monstrous Humanoid | Reptilian |
| `RT` | Monstrous Humanoid | Aquatic |
| `HS IX` | Aberration | Aquatic |
| `PI MH OB` | Aberration | none |
| `HF` | Fey | none |
| `HY` | Humanoid | Extraplanar |
| `L` | Lycanthrope | Shapechanger |
| `OG` | Construct | none |
| `OS` | Ooze | none |
| `PS VT` | Plant | none |
| `PZ U UH` | Undead | none |
| `UG US` | Undead | Incorporeal |
| `UV` | Undead | Vampire |
| `X` | Outsider | Chaotic, Evil, Extraplanar |
| `Y` | Outsider | Evil, Extraplanar, Lawful |
| `Z` | Outsider | Angelic, Extraplanar, Good |
| `DK` | Dragon | none |
| `K` | Magical Beast | none |
| `N` | Unknown | none; Medium fallback |
| `OH` | Monstrous Humanoid | none |
| `OP` | Construct | none |
| `OU` | Plant | none |

The contradiction classifier covers Feline, Efreeti, Angel, Vampire, Naga,
Githyanki, Dragonkin, Humanoid Other, Possessed, Sessile, Ki-rin, and None records.
The 258 active mixed-bucket records resolve through exact, phrase, or base rules with
stable evidence and no per-record approval.

### Completed mobile field dispositions

Every physical source mobile value now has a converter-side disposition and a ledger
entry. Candidate runtime validation remains separate work.

| Source value | Completed converter disposition |
|--------------|----------------------------------|
| file version and terminator | Mapped to canonical target enhanced-record framing. |
| VNUM | Mapped through the canonical typed RoL identity formula. |
| aliases and three descriptions | Converted through canonical text and color handling. |
| action mask | Each active bit is mapped, adapted, deferred to special reconciliation, or explicitly source-only. |
| two affect masks | Each active bit is mapped to `AFF`, mapped to `Aff2`, or explicitly transient or inert. |
| alignment | Preserved as target alignment and excluded from subtype inference. |
| format letter | Strict source `S` grammar is emitted as target enhanced `E`. |
| race code | Resolved through the frozen 75-code symbolic identity policy. |
| height and weight | Used by the deterministic size hierarchy and then represented by one persisted target size. |
| aggression-race list | Parsed independently of owner identity and given a bounded exclusion with player-impact evidence. |
| level | Mapped from the source 1-59 scale to target 1-34 competence. |
| hitroll and armor | Replaced by calculator values and serialized with target inverse encodings. |
| HP and damage dice | Replaced by deterministic calculator and tier output; exact named profiles own approved overrides. |
| money and experience | Adapted to target-native calculated rewards with explicit `MOB_CUSTOM_GOLD` ownership. |
| positions | Mapped from the source posture/status bitfield. |
| sex | Preserved when valid; five invalid active values have exact hash-bound repairs. |
| class | Mapped from source class plus class-role action evidence. |
| magic resistance | Explicit source value wins; otherwise the source race-and-level value is derived and persisted once as `SpellRes:`. |
| prestige bonus | Explicitly excluded because the target has no equivalent mobile kill-reward field. |

The ledger also records source loader consequences: ability generation, spell-circle
budgets, derived resistance and dimensions, infravision, coin randomization, memory,
classless experience reduction, constitution HP adjustment, elite-alias behavior,
scavenger suppression, psionic resources, race procedures, and periodic/path
behavior. Each is mapped, adapted, or excluded rather than silently assumed.

Target-only generated ownership is explicit:

- abilities and saves come only from the C calculator or an exact custom profile;
- race, three subraces, class, size, and tier come from deterministic selection;
- `Aff2`, resistances, feats, known spells, procedures, and trigger attachments come
  only from separately traced source behavior;
- hitroll, armor, HP, damage, experience, gold, and spell resistance use target-native
  encodings and ownership; and
- unrelated target echo and path fields are not invented from the base mobile row.

### Completed focused regression coverage

Focused and production-scale tests now cover:

- strict four-row grammar and every known bounded repair class;
- all 75 registry codes, complete symbolic policy coverage, and manifest freshness;
- unknown source codes, invalid target symbols, duplicate subtypes, stale exact hashes,
  and malformed calculator responses;
- exact, phrase, and fallback identity outcomes across the complete active corpus;
- all 356 aggression lists remaining independent from owner identity;
- mapped level boundaries from source 1 through the authored level-60 clamp case;
- all supported calculator levels, classes, races, categories, tiers, non-100
  configuration, invalid tiers, and custom profiles;
- Giant and non-Giant size ownership outside the calculator and wrapper;
- one persistent helper process, protocol/version identity, input echo validation,
  executable hashing, stale-hash rejection, and absence of a Python formula fallback;
- hitroll, armor, fixed-HP, abilities, saves, tier, subrace, final-size, reward, and
  custom-profile serialization;
- `SpellRes:` parser, validator, runtime load, GenOLC save, MEDIT ownership, and
  explicit-versus-derived conversion;
- positive and negative prestige dispositions and exact repair diagnostics; and
- separate living and dracolich Tiamat conversion profiles.

### Mobile recovery verification

The implementation passed these focused gates on 2026-08-17:

- all 756 production-linked CuTests, including full calculator input coverage,
  non-100 category configuration, wrapper size ownership, named profiles, and
  `SpellRes:` load/save;
- all 427 world-tool tests;
- full-corpus identity selection and one-process native serialization for all 12,407
  active mobiles;
- constants-manifest freshness, Python bytecode compilation, C format checks for new
  source files, and `git diff --check`.

Spawn observation, gameplay balance, walkthrough, soak, automatic apply recovery,
explicit rollback tooling, and production deployment remain release work and are kept
in the active recovery plan.

## Completed verification

The completed tier slice passed these gates on 2026-08-16:

- warning-free GNU C23 production and CuTest builds;
- all 749 production-linked CuTests;
- all 420 world-tool tests;
- world documentation drift, ASCII, UTF-8, and LF checks;
- Python mobile parser tests;
- `git diff --check`;
- `make install`; and
- root-level `circle` artifact removal after installation.

The verified binary was installed through the repository's versioned installation
path. No production deployment or converted-world rewrite was performed.

## Authoritative implementation references

- `src/mob/mob_autoroll.c`
- `src/mob/mob_autoroll.h`
- `src/olc/medit.c`
- `src/olc/genmob.c`
- `src/db.c`
- `src/combat/fight.c`
- `src/utils.c`
- `unittests/CuTest/test_mob_autoroll.c`
- `scripts/world/wtool_lib/mobiles.py`
- `docs/world_game-data/builder_manual.md`
- `docs/world_game-data/MOB_FLAGS.md`
