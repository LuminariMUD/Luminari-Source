# RoL Conversion Accuracy Recovery - Remaining Work

Status: Active; this file contains unfinished work only

Last reviewed: 2026-08-16

Completed work and verification evidence have moved to
[ROL_CONVERSION_DONE.md](ROL_CONVERSION_DONE.md). When an item in this plan is
finished and verified, move its durable result to that file instead of leaving it
mixed into the active backlog.

## Remaining outcome

The converted world is not yet accepted as accurate or complete. The remaining
project must:

1. Teach the converter to replace the complete generated RoL namespace while proving
   that no non-RoL Luminari content changes.
2. Audit every active source record and meaningful directive for semantic fidelity.
3. Repair systemic conversion defects and then record-specific defects.
4. Convert mobiles by mapping level and identity; selecting class, race, up to three
   subraces, and tier; determining and retaining final size; running Luminari-native
   automatic stats; and then assigning the deterministically selected final size once
   before serialization.
5. Validate, review, soak, and deploy an accepted reconversion release with tested
   rollback.

## Remaining priority problems

The highest-priority unresolved problems are:

- There is no guarded full-namespace replacement path that proves the candidate owns
  only reserved RoL identities and leaves all non-RoL records byte-identical.
- There is no complete semantic-disposition ledger proving whether each source record
  is exact, mapped, adapted, repaired, excluded, or still unknown.
- The mobile converter still collapses broad high-level source ranges and retains raw
  source combat fields whose target meanings or encodings differ.
- Mobile identity conversion currently reads only the first RoL race-row token,
  applies one hard-coded broad-family integer, and emits neither subraces nor size.
  All 12,407 current converted mobiles lack `Size:` and meaningful subraces, so size
  falls through to Luminari's Medium default.
- The complete ordinary mobile autoroll calculation is not yet exposed as one
  side-effect-free authoritative C profile shared by OLC and conversion.
- The Python converter is not yet connected to a versioned C batch calculator.
- Active RoL level-51+ mobiles have not received deterministic automatic tier
  classifications.
- Named world bosses, beginning with Tiamat, do not yet have individual conversion
  profiles and encounter validation.
- Class-category modifiers, rewards, magic resistance, prestige behavior, authored
  overrides, automatic-stat Giant size mutation, and final spawned-stat ownership
  remain unreconciled.
- Full-corpus gameplay, persistence, soak, release, and production evidence does not
  yet exist for an accuracy-recovery release.

Treat the high-end mobile conversion defect as P1 major combat behavior until the
effective-stat audit and representative encounter tests prove otherwise.

## Tier selection rule for all remaining work

"Select tier" and "apply tier modifiers" are two different steps:

1. Map level and select race, up to three subraces, and class.
2. Determine final size automatically, but hold it outside automatic-stat calculation.
3. Explicitly classify and select Standard, Elite, Small Group, Big Group, Raid, or
   World Boss.
4. Pass that selected tier and the required calculation inputs into automatic-stat
   calculation.
5. Calculate the ordinary Luminari baseline, then apply the already selected tier's
   modifiers exactly once.
6. Assign the retained final size once, after automatic-stat calculation.
7. Persist the explicit tier, all three subrace slots, final size, and resulting
   statistics.
8. At load, read the persisted identity and tier; do not infer or replace them.

Level never chooses tier. The calculator never chooses tier. The loader never chooses
tier. New OLC mobiles already have Standard selected by default. The converter must
produce an explicit tier through versioned deterministic rules before invoking the
calculator. Converted mobiles require no per-mobile human classification or approval.

The only load-time exception is temporary compatibility for old records that have no
`Tier:` field. Those records may derive a Legacy effective tier from level so existing
content does not abruptly change. Legacy fallback is not a valid classification for
new conversion output, and every converted RoL level-51+ mobile must leave that state
before release.

## Race, subrace, and size accuracy rule for all remaining work

A Luminari mobile has one broad NPC race family and exactly three persisted NPC
subrace slots. A mobile may use zero through three meaningful subraces; unused slots
must be `SUBRACE_UNKNOWN`. The deterministic resolver must select and emit
`SubRace 1:`, `SubRace 2:`, and `SubRace 3:` instead of collapsing all source
identity into `Race:` alone. Race, subrace, and size conversion must require zero
per-mobile manual review.

The deterministic identity resolver owns final size. Current `autoroll_mob()` does
not use size in its combat-stat math. It nevertheless has two legacy size writes: its
Giant race branch tries to raise a mobile smaller than Large to Large, and its
live-mobile path copies current size back into real size. Neither write belongs in
the conversion calculation contract. Remove size ownership from the extracted
calculator and its wrapper after auditing existing callers.

Determine final size automatically from a versioned hierarchy: an exact identity
profile first, positive authored dimensions second, deterministic source-race
dimensions third, and a source-code fallback last. Retain that result before
calculation, but do not write it onto the calculation working record or destination
mobile. Current automatic-stat math does not use size to calculate combat statistics.
Automatic-stat calculation must neither read nor write size. After automatic-stat
and tier calculation, assign the retained final size to the destination exactly once
before serialization. Giant profiles must select Large or larger directly instead of
relying on the hidden Giant minimum. The calculator and wrapper must never accept,
invent, copy back, or return a replacement final size.

### Deterministic race and subrace research baseline

The remaining resolver must be based on what the two engines actually store and do,
not on guesses from creature names.

The authoritative RoL mobile race definition is the first token of the race row. The
lookup table in `EXAMPLE/RealmsOfLuminari/src/race_class.c` maps 75 codes to concrete
RoL race identifiers. RoL then uses the selected race for racial traits, automatic
height and weight, magic resistance, vision, movement, combat behavior, and several
race-specific procedures. This makes the code the strongest general evidence
available, but the active corpus proves that builders sometimes used an approximate
or incorrect code.

Use the runtime lookup table, not header comments, as source authority. For example,
the Ki-rin header comment says `OK`, while the actual loader table and active files use
`K`.

The remaining race-row fields have narrower meanings:

- token 1 is the mobile's RoL race code;
- tokens 2 and 3 are authored base height and weight;
- optional token 4 is a period-separated list of races that the mobile attacks; it
  describes aggression targets and must never be treated as the mobile's own race or
  subrace.

RoL calls `set_char_size()` when either height or weight is zero. That function uses
the source race table, sex, level for level-sized races, and randomized stat rolls.
It derives dimensions, not a Luminari race or subtype. In the active corpus only 65
mobiles have positive authored height and only 100 have positive authored weight, so
dimensions are mostly a size-validation input, not a reliable identity classifier.

Luminari stores a different model:

- `Race:` on an NPC is one broad family such as Humanoid, Undead, Dragon, Giant,
  Aberration, Construct, Elemental, Fey, Magical Beast, Monstrous Humanoid, Ooze,
  Outsider, Plant, Vermin, or Lycanthrope;
- exactly three persisted NPC subtype slots are available; and
- `HAS_SUBRACE()` checks all three slots equally, so slot order has no runtime
  priority.

Subtypes are mechanical, not decorative. Existing code uses them for elemental
resistances and vulnerabilities, holy and unholy damage, aligned weapons, dismissal,
incorporeal interaction, swarm combat restrictions, goblinoid and vampire checks,
and other targeting. A false subtype can therefore change an encounter substantially.

The target PC race registry is useful evidence for a species' broad family and normal
size, but its specific PC race identifiers cannot be written into an NPC `Race:`
field. For NPCs that field has the broad-family meaning above.

### Measured conversion and corpus facts

The 2026-08-16 development snapshot establishes this baseline:

- 12,407 active RoL mobile headers were parsed and 12,406 have all four expected
  numeric rows; they use 73 distinct active race codes;
- every active code is recognized by the current hard-coded table, so the main defect
  is semantic loss rather than unknown-code volume;
- 12,051 race rows have three tokens and 356 also have the aggression-list token;
- the current converter emits only `Race:` and `Class:` for identity, with no
  `SubRace 1:`, `SubRace 2:`, `SubRace 3:`, or `Size:`;
- all 12,407 generated RoL mobiles consequently have no meaningful subtype and no
  persisted size;
- among 14,660 native Luminari mobiles, only 282 have any meaningful subtype; 5,103
  omit subtype fields and 9,275 explicitly store three Unknown values; and
- native world data contains inconsistent and duplicate subtype assignments, so it
  can provide examples but cannot serve as a complete canonical registry.

The strongest target-side precedents are reviewed constructors in
`src/combat/encounters.c` and `src/quest/hunts.c`. They identify, among other cases,
Goblin as Humanoid/Goblinoid, Kobold as Humanoid/Reptilian, Ogre and Troll as Giant,
Naga as Aberration/Aquatic, Efreeti as Outsider/Extraplanar/Fire, Djinni as
Outsider/Extraplanar/Air, Golem as Construct, and Wraith as Undead/Incorporeal.

The current `RACE_CODE_MAP` is therefore not an acceptable final resolver. It uses
duplicated numeric constants, maps Ogre and Troll to Humanoid instead of Giant, maps
Naga to Monstrous Humanoid instead of the native Aberration/Aquatic profile, maps
Githyanki to Outsider despite RoL's humanoid trait, and gives unsafe uniform defaults
to mixed `OH`, `OP`, and `OU` buckets. It has no confidence, contradiction, or
record-override result.

### Required evidence order

Resolve every mobile through this deterministic order:

1. Apply a versioned exact-record rule keyed by stable source identity when the
   automatic corpus rules contain one. It must include its reason, evidence, rule ID,
   and source hash or release; it requires no run-time human approval.
2. Read the exact RoL race code through a complete versioned source-code registry.
   Unknown codes are errors, never Human or a silent broad default.
3. Run a closed, versioned identity classifier over normalized aliases and short
   description. It may use only whole words or phrases tied to complete atomic target
   profiles. Compound phrases such as `undead dragon` and `spirit naga` take priority
   over their component words. It must not use fuzzy matching or unrestricted
   description text.
4. If several rules match, select exactly one complete profile using explicit rule
   priority, then phrase specificity, then stable rule ID. Never union subtype lists
   from competing rules.
5. Use RoL racial traits, authored affects, dimensions, class, special procedure,
   zone role, and reset context as deterministic tie-break or corroboration inputs
   only where a versioned rule names that signal.
6. If no exception rule wins, apply the source code's mandatory base fallback. Every
   known code has one, including mixed buckets. Low confidence produces a diagnostic,
   not a manual-review gate.
7. Store the source code, source race name, selected target family, zero through three
   subtypes, winning rule ID, evidence, confidence, and fallback status in the
   conversion ledger.

There is no `REVIEW_REQUIRED` identity state. Unknown source syntax or an internally
invalid rule set still fails closed, but ordinary ambiguity resolves through the
documented automatic fallback and does not ask a person to classify a mobile.

Do not use these tempting shortcuts:

- do not derive Good, Evil, Lawful, or Chaotic subtypes from the numeric alignment
  field alone; Luminari already stores alignment, while these subtypes also grant
  aura, resistance, vulnerability, and weapon-targeting behavior;
- do not derive Aquatic merely from RoL `can_swim`; even ordinary source humanoids
  have that trait;
- do not derive Air from flight, Swarm from Insect/Arachnid/Parasite, Incorporeal
  from a temporary spell affect, or Shapechanger from descriptive prose;
- do not use the aggression race list as owner identity;
- do not treat equipment, nearby mobs, a zone theme, or a shared reset as proof of
  species; and
- do not use `SUBRACE_DARKLING` as a synonym for drow. Darkling is a distinct
  Luminari setting identity.

### Initial complete source-code profile matrix

The following is the initial base matrix. Names are symbolic target values, not
hard-coded integers. An exact identity rule may replace a base profile, but every
source code has a complete automatic fallback and no row creates a manual-review
queue.

| RoL code(s) | RoL meaning | Base Luminari family | Base subtype(s) | Decision |
|-------------|-------------|----------------------|-----------------|----------|
| `A AA AB AC AD AE AF AH B BR` | animal families | Animal | none | automatic base |
| `F` | fish | Animal | Aquatic | automatic base |
| `R RS` | reptile, snake | Animal | Reptilian | automatic base |
| `AP AS I` | parasite, arachnid, insect | Vermin | none | automatic; never infer Swarm |
| `AY` | hybrid animal | Magical Beast | none | automatic base |
| `D` | dragon | Dragon | none | automatic base |
| `E1` | efreeti | Outsider | Extraplanar, Fire | automatic base with known contradictions |
| `E2` | djinni | Outsider | Extraplanar, Air | automatic base |
| `EA` | air elemental | Elemental | Air, Extraplanar | automatic base |
| `EE` | earth elemental | Elemental | Earth, Extraplanar | automatic base |
| `EF` | fire elemental | Elemental | Fire, Extraplanar | automatic base |
| `EW` | water elemental | Elemental | Water, Extraplanar | automatic base |
| `G PO PT` | giant, ogre, troll | Giant | none | automatic base |
| `H H2 HH HO P2 PB PD PE PF PG PH PL PM PR` | humanoid and player-race families | Humanoid | none | automatic base |
| `HG` | goblin | Humanoid | Goblinoid | automatic base |
| `HK` | kobold | Humanoid | Reptilian | automatic base |
| `HC MS` | centaur, manscorpion | Monstrous Humanoid | none | automatic base |
| `PY RH` | yuan-ti, reptoid | Monstrous Humanoid | Reptilian | automatic base |
| `RT` | kuo-toa | Monstrous Humanoid | Aquatic | automatic base |
| `HS IX` | naga, ixzan | Aberration | Aquatic | automatic base with variant checks |
| `PI MH OB` | illithid, umber hulk, beholder | Aberration | none | automatic base |
| `HF` | faerie | Fey | none | automatic base |
| `HY` | githyanki | Humanoid | Extraplanar | automatic base with known contradictions |
| `L` | lycanthrope | Lycanthrope | Shapechanger | automatic base |
| `OG` | golem | Construct | none | automatic base |
| `OS` | slime | Ooze | none | automatic base |
| `PS VT` | myconid, tree | Plant | none | automatic base |
| `PZ U UH` | lich, undead, high undead | Undead | none | automatic broad base |
| `UG US` | ghost, spirit | Undead | Incorporeal | automatic base |
| `UV` | vampire | Undead | Vampire | automatic base with known contradictions |
| `X` | demon | Outsider | Chaotic, Evil, Extraplanar | automatic base |
| `Y` | devil | Outsider | Evil, Extraplanar, Lawful | automatic base |
| `Z` | angel | Outsider | Angelic, Extraplanar, Good | automatic base with known contradictions |
| `DK` | dragonkin | Dragon | none | automatic mixed-bucket fallback |
| `K` | ki-rin | Magical Beast | none | automatic; statue rules override to Construct |
| `N` | none | Unknown | none | automatic utility/unknown fallback |
| `OH` | humanoid other | Monstrous Humanoid | none | automatic mixed-bucket fallback |
| `OP` | possessed | Construct | none | automatic mixed-bucket fallback |
| `OU` | sessile | Plant | none | automatic mixed-bucket fallback |

All four RoL elemental codes map to Extraplanar as the deterministic project policy.
This follows the strongest repeated native Luminari elemental profiles and avoids a
per-record planar-origin decision. An exact identity rule may replace that complete
profile if a source record explicitly represents something other than an elemental.

RoL marks Ghost and Spirit as immaterial, which supports Incorporeal directly. It
also gives immaterial behavior to Fire and Air elementals. Preserve that latter
behavior through a separate versioned behavior mapping if required; do not silently
turn a temporary or engine-specific behavior into an identity subtype.

### Known contradiction classes and automatic exception coverage

The source race code is strong but not infallible. Active examples include:

- Feline containing a displacer beast and other magical felines;
- Efreeti containing echo mobiles and magma creatures;
- Angel containing a djinni and a statue;
- Vampire containing a ghoul, a ghast, a vampiric warg, and generic undead;
- Naga containing a couatl, bone nagas, and spirit nagas;
- Githyanki containing an echo machine and unrelated `ruck` records;
- Dragonkin containing hydras, wyverns, pseudodragons, remorhazes, a tarrasque,
  element-themed drakes, undead dragons, and unrelated records;
- Humanoid Other containing zombies, mummies, otyughs, hook horrors, plants,
  owlbears, and actual humanoids;
- Possessed containing animated weapons and armor, machines, walls, wagons,
  monoliths, undead, mimics, and scenery; and
- Sessile containing plants, fungi, oozes, worms, leeches, ropers, mimics,
  aberrations, and zone-echo utilities.

The classifier must exercise all 254 active records in `DK`, `N`, `OH`, `OP`, and
`OU`, plus the four active `K` records, as a dedicated mixed-bucket corpus fixture.
Exact compound-phrase rules should recover the more specific profiles; each remaining
record receives the table's automatic base fallback. The same classifier handles
contradictions in otherwise uniform code families. Coverage, winning rule, and
confidence are reported, but none requires a human classification step.

### Representation ceiling

The current Luminari NPC schema cannot preserve every exact RoL species. Human,
Drow, Grey Elf, Mountain Dwarf, Duergar, Halfling, Gnome, Half-Elf, Orc, and similar
source identities all collapse to the Humanoid family because no matching NPC
subtype or species field exists. That is honest broad-family mapping, not exact
species conversion.

Preserve the exact RoL race code and name in the conversion decision ledger. If
species-level runtime mechanics are required, design a separate persisted NPC
species field or deliberately expand the subtype model after reviewing all gameplay
consumers. Do not stuff species into unrelated existing subtypes and do not claim
species-perfect accuracy from the present schema.

### Required resolver contract

Replace `RACE_CODE_MAP` with one versioned identity resolver that returns an atomic
decision containing:

- exact source code and authoritative source name;
- target broad family by symbolic constant;
- an ordered, de-duplicated list of zero through three symbolic subtypes;
- automatically selected final-size evidence, without writing size yet;
- rule ID and rule-set version;
- confidence and `AUTO_EXACT`, `AUTO_PHRASE`, or `AUTO_FALLBACK` status;
- all supporting and contradictory evidence; and
- the stable exact-record or phrase-rule identity when applicable.

Every rule returns one complete atomic profile containing no more than three
subtypes; rules never accumulate a fourth subtype. Preserve the winning profile's
declared subtype order and fill unused persisted slots with Unknown. Since runtime
lookup is order-independent, order exists only to make generated output stable and
inspectable.

Extend the generated constants manifest to export the target race, subtype, and size
symbols from the C headers. Python must not duplicate their numeric values. Add a
complete source-code registry test against the frozen RoL lookup table, plus active
corpus coverage tests. Correct the current unknown-code diagnostic, which emits target
Unknown (`0`) while claiming it used Human.

## Non-negotiable rules for remaining work

- Never run conversion or reconversion writes against production.
- Never re-enable or reuse the destructive Phase 6.5 rehome.
- Never change the canonical RoL VNUM formulas as part of an accuracy repair.
- Never treat similar source/target names, themes, or low VNUMs as proof that two
  world records share identity. The closed source-species phrase rules above classify
  mobile taxonomy; they do not alias a RoL record to a native Luminari record.
- Do not permit builder or OLC changes anywhere in the RoL namespace until conversion
  accuracy is locked. Any accidental edit in that namespace is disposable and will be
  overwritten by full regeneration.
- Treat the frozen RoL source, versioned conversion rules, and generated candidate as
  the only authorities during recovery; never hand-edit generated output as a source
  of truth.
- A generated RoL prototype may be removed during full regeneration only when the
  candidate proves it is absent from the authoritative source result and reference
  and persistence checks permit removal.
- Never modify existing Luminari records to make RoL content fit.
- Never require a per-mobile human race, subrace, or size decision; every valid known
  source record must resolve through one deterministic atomic profile.
- Every changed conversion rule requires focused tests and regenerated evidence.
- Every apply must have an exact, tested rollback bundle.

## Remaining full-regeneration design

### Exclusive RoL ownership during recovery

No building or OLC editing is allowed in the RoL namespace until conversion accuracy
is locked. During this recovery phase:

- the frozen RoL source tree, versioned conversion rules, authoritative C calculator,
  and generated candidate are the only sources of truth;
- the currently installed converted RoL records are disposable output;
- reconversion replaces generated RoL records instead of merging with them;
- there is no builder-preservation decision, three-way merge, or per-record conflict
  workflow; and
- non-RoL Luminari content remains outside converter ownership and must not change.

The regeneration boundary must be proved from canonical type and VNUM rules, target
paths, index membership, generated attachment ownership, and reference closure. A
reserved-looking filename alone is not sufficient. If a physical file mixes RoL and
non-RoL records, the tool must reconstruct it while preserving every non-RoL record
byte-for-byte or fail before writing.

The original importer's collision guard is not a conversion defect. It correctly
refuses to use an `ADD` action over an existing path or VNUM because the first import
was designed to protect an unknown target world. Reconversion does not need to weaken
that general protection. Build the candidate from the preserved non-RoL world plus a
freshly generated RoL namespace, then replace the old RoL namespace as one guarded
development operation.

### Reconversion commands

Implement separate read-only, build, apply, rollback, and completion responsibilities.
Working names are:

```text
rol-reconversion-plan       read-only scope and current-to-candidate difference report
rol-reconversion-build      generate and validate an isolated complete candidate
rol-reconversion-apply      hash-guarded development replacement with backup
rol-reconversion-rollback   restore the exact pre-apply development tree
rol-reconversion-completion validate and prove a repeat-apply no-op
```

Plan and build must not write to `lib/world`. Apply must retain the
`APP_ENV=development` guard. Production deployment remains a separately authorized
operation that installs an already accepted artifact and never regenerates it.

### Candidate scope manifest

Create one durable candidate artifact that records:

- every source record ID, kind, VNUM, path, line, and source hash;
- every canonical target type, VNUM, physical path, and generated record hash;
- generated triggers, attachments, indexes, special procedures, and compatibility
  behavior;
- every repair, adaptation, and exclusion disposition;
- every current RoL record to add, replace, retain unchanged, or remove;
- the complete set of non-RoL files and records that must remain byte-identical; and
- source-tree, converter-code, calculator, policy, candidate, and runtime hashes.

The current target is compared with the new candidate only to report changes and
guard against a concurrent write. A difference inside proven RoL scope is a normal
replacement, not a builder conflict. A difference outside that scope blocks the run.

### Safe full replacement and rollback

Before a development write, apply must:

1. Verify the candidate, runtime binary, policy, and current target hashes.
2. Verify every changed identity and path belongs to proven RoL scope or a required
   shared index update.
3. Prove all non-RoL record blocks and unrelated paths remain byte-identical.
4. Save exact copies and hashes of every path that could change.
5. Write an apply journal before the first replacement.
6. Install the complete candidate RoL output and required index entries.
7. Validate the resulting tree against the accepted candidate.
8. Restore the backup automatically if application or validation fails.
9. Prove that immediate reapplication changes zero paths.

Implement rollback as a tested command, not merely a backup directory.

### Removal inside the generated namespace

Full regeneration may remove a current converted RoL prototype when it is absent from
the authoritative new candidate. The plan must list every removal explicitly and
prove typed-reference closure. Before any production deployment, persistence checks
must also prove that saved objects or other durable owners will not become unresolved,
or provide a separately tested migration. No removal may touch a non-RoL identity.

## Remaining semantic accuracy audit

### Disposition model

Create one result for every active source record and meaningful directive:

| Disposition | Meaning |
|-------------|---------|
| `EXACT` | Preserved without semantic change. |
| `MAPPED` | Converted through a reviewed and tested mapping. |
| `ADAPTED` | A different Luminari mechanism preserves the source intent. |
| `REPAIRED` | An evidenced source defect was corrected. |
| `EXCLUDED` | The smallest unsafe or impossible unit was intentionally omitted. |
| `UNKNOWN` | Accuracy has not been established and acceptance is blocked. |

Every non-exact result needs source evidence, a player-impact statement, a reviewed
rule, and a regression test or walkthrough. An exclusion is not acceptable merely
because it is labeled.

### Required coverage

Audit at least:

- package and record presence;
- room names, descriptions, sectors, flags, sizes, exits, doors, and keys;
- zone ranges, reset order, dependencies, probabilities, limits, and schedules;
- mobile text, flags, affects, race, class, level, combat values, positions, and loot;
- object text, type, flags, wear slots, values, applies, affects, spells, containers,
  traps, decay, and equipment behavior;
- shops, products, hours, restrictions, prices, messages, and keeper behavior;
- quests, dialogue, prerequisites, costs, rewards, commands, and host bindings;
- SOC modes, actions, paths, calendars, targets, and generated DG triggers;
- direct, dynamic, implicit, composite, periodic, combat, death, and object special
  procedures;
- typed references, portals, transported destinations, scripted loads, and
  attachments;
- bounded source repairs and every converter diagnostic; and
- player-visible behavior under the target runtime.

Extend the current capability audit from "a handler exists" to field- and
behavior-level fidelity. Turn silent fallback, lossy conversion, broad default, and
unsupported syntax paths into explicit ledger results.

### Remaining source `.mob` field disposition matrix

The following matrix is the complete physical RoL mobile-record inventory traced
through `EXAMPLE/RealmsOfLuminari/src/db.c`, the current Python parser and emitter,
and Luminari's `src/db.c` and `src/olc/genmob.c`. A field is not covered merely
because the current emitter writes a syntactically valid target record.

| RoL file or record value | RoL meaning | Current conversion | Remaining required disposition |
|--------------------------|-------------|--------------------|--------------------------------|
| file-version line and `$` terminator | Source framing, not mobile state. | Parsed; target files use their native framing. | Prove every active file version and terminator has an explicit structural disposition. |
| `#vnum` | Mobile identity. | Deterministically remapped into the reserved RoL namespace. | Prove regeneration scope and typed-reference closure during reconversion. |
| aliases | Command lookup words. | Text and color syntax are converted. | Validate token preservation and any lossy byte replacement. |
| short description | Name shown in messages. | Text and color syntax are converted. | Validate visible text and retain it as identity-classifier evidence. |
| long description | Room-list description. | Text and color syntax are converted. | Validate capitalization, line framing, and visible color behavior. |
| detailed description | Look description. | Text and color syntax are converted. | Validate content, wrapping, terminators, and visible color behavior. |
| action mask | 32 source prototype behaviors. | Every action bit used by the active corpus is mapped, adapted, deferred to special reconciliation, or explicitly source-only. | Prove each mapped behavior in the target runtime and retain a per-bit disposition; never equate syntactic persistence with semantic equality. |
| first and second affect masks | 64 possible source affects. | Every affect bit used by the active corpus is mapped to `AFF`, mapped to `Aff2`, or marked transient/inert source-only. | Prove persistent behavior and explicit loss for every used bit. |
| alignment | Numeric moral alignment. | Copied into the target alignment field. | Add range and loader-equivalent round-trip tests; never invent alignment subtypes from it. |
| format letter `S` | Selects the RoL simple-mobile grammar. | Converted to target enhanced format `E`. | Reject unknown letters and prove the emitted enhanced section terminates correctly. |
| race code | One of the RoL runtime's 75 race identities. | Collapsed through an inaccurate broad numeric table. | Replace it with the deterministic family and zero-to-three-subrace resolver. |
| base height and base weight | Authored dimensions; RoL derives both from race when either is zero. | Parsed but ignored. | Use valid authored dimensions and deterministic source-race dimensions to select final Luminari size, then persist `Size:` once after automatic stats. |
| optional period-separated aggression-race list | Exact RoL race identities the mobile attacks. | Parsed as race-row text but completely ignored by emission. | Add a deterministic target aggression profile or a measured explicit adaptation; never silently drop it or treat it as owner identity. |
| level | Source competence on the RoL 1-59 runtime scale. | Raw values are clamped directly into target 1-34, collapsing every higher level. | Apply the versioned 1-59 to 1-34 formula before class, tier, and automatic stats. |
| hitroll | Direct RoL attack bonus. | Copied into a target field whose file encoding is `20 - hitroll`. | Stop copying it; serialize the authoritative target calculator result using target inverse encoding. |
| armor | RoL armor value bounded to -100 through 100 at load. | Copied into a target field whose loader computes `10 * (20 - file_AC)`. | Stop copying it; serialize the authoritative target calculator result using target armor encoding. |
| HP dice count, size, and bonus | RoL spawn HP, followed by source constitution adjustment. | Raw dice are copied despite different target runtime modifiers. | Replace them with the deterministic target automatic-stat result and prove final spawn HP. |
| damage dice count, size, and bonus | RoL base weaponless damage. | Raw dice are copied. | Replace them with the target automatic-stat and tier result unless a versioned exact-record profile owns an override. |
| aggregate money, or copper/silver/gold/platinum | Source carried currency, randomized downward at each spawn. | Denominations are flattened to copper-equivalent units, but target load ignores the value unless `MOB_CUSTOM_GOLD` is set. | Define one deterministic economy conversion, explicitly own `MOB_CUSTOM_GOLD`, and test the final spawned amount. |
| experience | Source kill reward, sometimes divided at load for mobiles without class powers. | Raw file value is copied, while a future autoroll would replace it. | Define a target reward policy from source effective reward, target level, tier, and group semantics; do not accidentally pay both policies. |
| current position and default position | RoL posture plus status bitfield. | Converted to a target position enum. | Exhaustively test all active combinations and deterministic repairs for malformed rows. |
| sex | Neutral, male, or female. | Valid values are copied and other values are silently changed to neutral. | Record an automatic repair rule for each invalid active value and emit its diagnostic. |
| class | Optional RoL class identity. | Mapped through a hard-coded class table, with action flags used when the field is zero. | Revalidate every source class and independent class-role flag, then select one target primary class without losing secondary behavior roles. |
| magic resistance | Optional explicit percentage; otherwise RoL derives a race-and-level value. | Ignored. Luminari has runtime spell resistance but no persisted mobile `SpellRes:` enhanced field. | Add and test canonical `SpellRes:` load/save/parser/OLC support, then convert the effective source value without duplicating target race bonuses. |
| prestige bonus | Extra source prestige currency awarded on a kill; negative authored values have no effect. | Ignored, and Luminari has no equivalent mobile kill-reward field. | Choose one versioned target-native reward adapter or explicit source-only exclusion for the mechanic; apply it automatically to all affected records. |

The field ledger must also cover the source loader consequences of those values. RoL
clamps level at 59; creates level-based spell-circle budgets; rolls level- and
race-dependent abilities; derives race-and-level magic resistance when no override
exists; adds infravision to undead and animals; randomizes every coin denomination;
derives both dimensions when either is zero; adds memory to most non-animals; reduces
experience for mobiles without class powers; adds a constitution-based HP adjustment;
adds detect-invisibility to the exact alias `elite`; removes scavenging from witnesses
and shopkeepers; derives psionic mana; attaches standard Demon, Devil, and Umber Hulk
procedures; and initializes periodic and path behavior from assigned procedures.
Each consequence needs an `EXACT`, `MAPPED`, `ADAPTED`, `REPAIRED`, or `EXCLUDED`
result even when target autoroll or special reconciliation replaces its implementation.

The final target audit must observe Luminari after parse and spawn, not just emitted
text. Its loader inverse-decodes hitroll and armor, may discard gold, rolls persisted
HP dice, normalizes fighting positions, applies Legacy tier compatibility only when
`Tier:` is absent, applies class-category modifiers, and initializes NPC spell slots.
Acceptance compares the resulting live mobile after those steps so no source or
target modifier is omitted or applied twice.

The source parser must also enforce the loader's real grammar instead of merely
collecting four arbitrary lines. The active corpus contains known bounded defects
that require deterministic rules and diagnostics:

- mobile 51348 in `llyrath.mob` has no position row;
- mobiles 10632 and 10633 in `griffon.mob` have malformed dimension/aggression rows;
- mobile 50619 in `neshkal.mob` has one trailing money token that the source loader
  ignores after reading experience `1`;
- five records contain invalid sex values outside 0 through 2; and
- four special or utility records use authored level 0, so the generic positive-level
  mapping must fail closed unless an exact automatic noncombat or repair profile owns
  the record.

No parser default may silently turn these defects into ordinary data. Each record
must receive an automatic exact repair, a behavior-preserving special profile, or a
generation-blocking syntax result. None creates a per-mobile human review queue.

### Target-only `.mob` fields

Luminari enhanced mobile records can persist values that have no direct physical RoL
`.mob` field. Their absence from RoL is not itself a conversion defect, but the
converter must populate them when target mechanics or another traced RoL source
requires them:

| Target field group | Conversion ownership |
|--------------------|----------------------|
| `BareHandAttack`, `Str`, `StrAdd`, `Int`, `Wis`, `Dex`, `Con`, `Cha`, and the five canonical saves | Produced only by the authoritative target automatic-stat calculator or a versioned exact-record override. |
| `Race`, `SubRace 1`, `SubRace 2`, `SubRace 3`, `Class`, `Size`, and `Tier` | Selected deterministically before calculation, except final size is retained outside the calculator and assigned once afterward. |
| damage resistances, `DR_MOD`, `Feat`, `MFeat`, and `KnownSpell` | Emitted only when a traced source race, affect, class role, special procedure, or exact profile requires a tested target mechanic. |
| `Aff2` | Owns source affects that require the target's secondary persistent bitset. |
| `SpecProc` and inline `T` trigger attachments | Come from the separate source assignment, special-procedure, SOC, and trigger reconciliation; they are not invented from the base `.mob` row. |
| `Walkin`, `Walkout`, `EchoZone`, `EchoFreq`, `EchoCount`, `EchoSequential`, `Echo`, and `Path` | Have no direct RoL base-mobile counterpart; populate only from separately traced source behavior such as SOC data. |

Emit canonical target names, not deprecated save aliases. Every emitted target-only
value must identify the source rule or target calculation that owns it. An omitted
target-only field must be proven unnecessary rather than assumed unnecessary.

## Remaining mobile conversion work

### Level mapping

Implement and test one explicit competence mapping for positive combat mobiles. A
non-positive authored source level must fail the generic rule and may proceed only
through a versioned exact automatic repair or noncombat profile. First apply the RoL
runtime upper clamp, then map into Luminari:

```text
S = min(authored_source_level, 59)
L = min(34, ceil(3 * S / 5))
```

For positive integer arithmetic, use `(3 * S + 4) / 5`. This maps source 50 to target
30, source 51 to 31, source 52-53 to 32, source 54-55 to 33, and source 56-59 to 34.
Level selects competence only and must not automatically choose encounter tier.

### Authoritative full automatic-stat calculator

Finish extracting the ordinary Luminari automatic-stat baseline from the OLC wrapper
into a side-effect-free C function. The tier modifier functions are not a substitute
for the full profile. The calculator must:

- accept explicit target level, race, class, a preselected encounter tier, versioned
  configuration, and any approved custom profile inputs;
- reject a missing, Legacy-unspecified, or invalid tier instead of deriving one;
- never classify, infer, or return a replacement tier;
- return every generated persisted stat and enough expected post-load state to detect
  duplicate runtime modifiers;
- calculate the ordinary baseline before tier pressure;
- apply the already selected tier exactly once;
- exclude final size from calculation input and output while current automatic-stat
  math does not use it;
- avoid hidden global state and generation-time randomness;
- use checked intermediate arithmetic and reject out-of-range serialization;
- preserve deliberate custom stats only through an explicit versioned override; and
- remain the one production implementation used by OLC, conversion, validation, and
  tests.

Keep `autoroll_mob()` as the in-game wrapper that applies a calculator result to
`char_data`. Do not create an independent production Python copy of the formulas.
The conversion bridge, not the calculator, must assign the retained final size once
after applying the calculator result. Remove the wrapper's Giant minimum-size write
and live-mobile size copy-back after auditing and updating callers that relied on
those side effects; callers must assign identity-owned size explicitly.

### Versioned converter bridge

Create a small versioned batch utility around the C calculator. The converter must
finish level, identity, race, all three subrace slots, final-size, class, and tier
selection before sending the actual calculation inputs to one helper process, then
consume target-native results. Retain subraces and final size outside the helper. Do
not start one process per mobile. Do not allow the helper response to select or change
race, subraces, final size, class, or tier.

The helper protocol, calculator code, configuration, executable identity, and binary
hash must become release inputs. Generation must stop on a missing, stale, malformed,
nonzero, version-mismatched, or inconsistent helper. It must never fall back to raw
RoL combat rows.

If a new source or test file is added, update both `Makefile.am` and `CMakeLists.txt`.

### Selection, calculation, serialization, and load order

Implement this generation order:

1. Clamp and map level.
2. Select race, zero through three meaningful subraces, and class deterministically.
3. Determine final size automatically and retain it outside the calculator without
   assigning it to the calculation working record.
4. Classify and select an explicit encounter tier. Standard is still an explicit
   choice; World Boss also requires a named profile.
5. Invoke the calculator with the fields its math consumes: level, race, class,
   tier, versioned configuration, and approved custom-profile inputs.
6. Calculate the ordinary Luminari automatic-stat baseline and configured
   class-category treatment.
7. Apply modifiers from the already selected tier exactly once.
8. Apply an exact-record custom-stat profile selected by a versioned automatic rule,
   if any, with a durable reason.
9. Assign the retained automatically selected final size exactly once.
10. Serialize `Race:`, all three `SubRace` slots, `Class:`, final `Size:`, explicit
   `Tier:`, and every generated stat, including exact hitroll and armor-class file
   encodings.

Implement this load and runtime order:

1. Read the persisted tier and statistics without changing the explicit tier.
2. Reject or report missing explicit tier in newly converted RoL output; reserve
   level-derived Legacy behavior for pre-tier records only.
3. At spawn, roll only stable persisted dice expressions.
4. Apply any configured class-category or load-time modifier not already owned by the
   calculator exactly once.
5. Derive non-persisted combat behavior from the loaded tier without multiplying
   persisted statistics again.

Autoroll and conversion must follow the same generation order. Mob load must never be
used as a deferred tier-classification step.

Audit all remaining level-based "powerful being" rules. Keep only rules that truly
mean epic creature rather than encounter size.

### Class-category reconciliation

Reconcile every class between automatic-stat treatment and configured category
modifiers. Use non-100 configuration values in tests so category drift and duplicate
application cannot remain hidden. Resolve at least the known Sorcerer and Bard
disagreements.

### RoL 51+ classification

Give every active source level-51+ mobile an explicit automatically selected tier.
The versioned classifier must consider:

- zone and reset role;
- simultaneous reset population and intended party size;
- special procedures, known spells, attacks, defenses, and control effects;
- source combat values and relative rank among neighboring creatures;
- rewards, equipment, keys, and progression role; and
- named-boss identity.

Do not infer tier from level alone. Source levels 1-50 remain Standard unless a
versioned lower-level exception rule selects another tier. Complete classification
before invoking the automatic-stat calculator, include the selected tier in its
input, and persist that same tier in the generated mobile. The classifier must have
complete automatic fallback behavior and request no per-mobile human classification.
Eliminate Legacy-unspecified tier state from the converted RoL 51+ population before
release; mob load must not be expected to repair it.

### Effective-stat and field repairs

Repair and audit:

- inverse target hitroll encoding;
- armor bounds and target armor encoding;
- removal of raw source HP and damage rows when automatic stats own those values;
- save progression and class/race effects;
- source magic resistance and prestige semantics;
- experience, gold, loot, and group reward semantics;
- `MOB_CUSTOM_GOLD` ownership;
- authored stat overrides and their durable versioned reasons;
- special-procedure and known-spell interaction with automatic stats; and
- final spawned values after loader and runtime modifiers.

No converted mobile may rely on a raw RoL value whose target encoding and semantic
disposition have not been proven.

### Named world bosses

Build an explicit identity list and an individual automatic profile for every world
boss. Start with Tiamat. Her profile validation must cover both forms, total HP budget, regeneration,
room-wide breath cadence, disabling-effect removal, control immunity, summons,
phase transition, rewards, and group-size expectations.

Compare each converted boss with its source behavior and relevant native Luminari
encounters. A World Boss tier is only a Raid floor; it is not permission to apply one
larger universal multiplier. Select World Boss and its named profile before invoking
automatic stats; do not promote a Raid to World Boss during load or combat.

### Encounter balance and rewards

Run a development-only combat matrix using representative level-30 builds:

- solo martial;
- solo caster;
- two-or-three-player small group;
- four-to-six-player full group; and
- larger coordinated raid.

Exercise martial, divine, rogue, and arcane mobiles at each relevant tier. Separate
plain automatic-stat encounters from special-procedure and known-spell encounters.
Record time to kill, deaths, incoming and outgoing damage per round, hit rates, save
rates, spell-resistance failures, healing pressure, actions per round, control uptime,
regeneration, summons, and rewards per present player.

Repair or explicitly preserve kill-XP division, group bonuses, serialized gold, and
loot behavior before approving reward factors. Version any balance-driven coefficient
change and replace its golden vectors.

## Remaining finding ledger

Create one durable ledger with:

- stable finding ID;
- priority, severity, and status;
- affected package and canonical target identity;
- source VNUM, path, and line when known;
- observed target behavior;
- expected source behavior and evidence;
- suspected conversion layer;
- systemic or record-specific classification;
- proposed disposition and owner;
- required regression test or walkthrough; and
- release that resolves the finding.

Enter the known systemic mobile findings immediately, including level collapse,
hitroll encoding, raw-stat retention, armor bounds, magic resistance, prestige,
rewards, class categories, missing size, missing subraces, incorrect broad race
mappings, ambiguous source race buckets, and unclassified high-level encounters.

Use these priorities:

| Priority | Examples |
|----------|----------|
| P0 | Crash, boot failure, corruption, unsafe command, broken persistence, or inaccessible required content. |
| P1 | Broken topology, resets, progression, keys, quests, shops, portals, or major combat behavior. |
| P2 | Incorrect stats, flags, equipment, loot, prices, timing, secondary scripts, or noticeable mechanical fidelity. |
| P3 | Text, color, formatting, ambience, cosmetic timing, and low-impact fidelity. |

A report containing only an old source VNUM is incomplete. Record the canonical
target VNUM and the exact room, mobile, object, reset, quest, shop, script, trap,
path, or behavior.

## Remaining implementation phases

### Phase 0: Freeze inputs and select a pilot

Tasks:

- Preserve and checksum the authoritative source tree, conversion policy, calculator,
  current generated world, and existing Phase 7 and Phase 8 evidence.
- Identify the exact current RoL namespace present in development.
- Capture the current development tree without changing it.
- Create the finding-ledger schema and enter known defects.
- Record which old checks proved structure and which semantic claims remain unknown.
- Select one inaccurate but dependency-bounded package as the reconversion pilot.

Checkpoint:

- The source, current target, and newly generated form of every pilot record can be
  independently reproduced.
- No world or database write has occurred.

### Phase 1: Prove the regeneration boundary

Tasks:

- Add record-block hashing and scope metadata to candidate bundles.
- Prove RoL scope from canonical type/VNUM formulas, generated paths, attachments,
  indexes, and special-procedure evidence.
- Detect shared files, duplicate source identities, merged HLQs, generated triggers,
  and composite special bindings.
- Record every non-RoL record block that shares a physical file or index and require
  byte-identical preservation.
- Define candidate archival and retrieval rules.

Checkpoint:

- Every pilot record and generated trigger has exactly one proven RoL scope result.
- Non-RoL Luminari records cannot enter a replacement action.

### Phase 2: Implement read-only planning

Tasks:

- Add the reconversion-plan command.
- Compare the current target with the newly generated candidate.
- Emit add, replace, unchanged, removal, reference-impact, scope, and index ledgers.
- Add stable machine output and concise human summaries.
- Add failure tests for missing evidence, changed hashes, path escape, type confusion,
  duplicate identities, ambiguous scope, non-RoL changes, and malformed records.

Checkpoint:

- Every pilot difference is explained with zero writes.
- A deliberately changed RoL record becomes a normal candidate replacement, while a
  deliberately changed non-RoL record is excluded from the action set.

### Phase 3: Implement build, apply, and rollback

Tasks:

- Generate a pilot candidate in isolated staging.
- Produce hash-preconditioned `ADD`, `REPLACE`, `REMOVE`, and no-op actions within
  proven RoL scope.
- Add backup, journal, automatic failure recovery, explicit rollback, and completion
  evidence.
- Retain environment and runtime-binary guards.
- Apply twice and require the second run to change zero paths.

Checkpoint:

- The pilot can move to corrected output and back exactly.
- No non-RoL record changes.
- Interrupted or failed apply restores the original tree.

### Phase 4: Build the semantic audit

Tasks:

- Produce field- and behavior-level dispositions.
- Compare source runtime meaning with the generated target model.
- Turn every silent fallback and lossy diagnostic into a ledger row.
- Export symbolic target race, subtype, and size constants for converter use.
- Build the complete RoL race-code registry, base-profile matrix, contradiction
  report, automatic exact-phrase classifier, and exact-record rule schema.
- Resolve every mixed-bucket record and contradiction to `AUTO_EXACT`, `AUTO_PHRASE`,
  or `AUTO_FALLBACK`; create no per-mobile manual-review queue.
- Add effective mobile-stat comparison after loading and runtime modification.
- Require tier results for all active level-51+ mobiles and individual world-boss
  results.
- Produce per-package and global disposition counts.

Checkpoint:

- The pilot has no unreported semantic loss.
- Every non-exact result links to a rule, evidence, test, and impact statement.

### Phase 5: Repair the corpus in dependency-complete waves

Repair systemic rules before individual records. Use these waves:

1. Source inventory, parsing, missing records, identity, and typed references.
2. Rooms, exits, doors, keys, zone ranges, resets, and traversal.
3. Mobiles: level, race, up to three subraces, final size, and class mapping;
   automatic mixed-bucket classification; tier classification before calculator
   invocation; full shared calculator; converter bridge; encodings; rewards;
   resistance; custom profiles; and world bosses. Then repair objects, equipment,
   loot, containers, spells, affects, and traps.
4. Shops, quests, rewards, dialogue, prerequisites, and progression.
5. SOC, DG triggers, paths, schedules, portals, and attachments.
6. Native and adapted special procedures, including combat, death, periodic, and
   composite behavior.
7. Text, color, formatting, ambience, and cosmetic fidelity.

For each wave:

- group findings by common root cause;
- add focused failing tests;
- repair the production conversion or runtime layer;
- regenerate only through the isolated reconversion path;
- build twice and compare bytes;
- validate dependency closure;
- perform targeted gameplay walkthroughs without editing the RoL zones; and
- update finding and semantic ledgers.

Checkpoint:

- No open P0 or P1 remains in the wave scope.
- No new normalized world finding or unresolved typed edge is introduced.

### Phase 6: Validate a full-corpus release

Tasks:

- Generate two independent full-corpus candidates and require identical bytes.
- Prove the complete current RoL namespace is replaced by or already equals the
  candidate, with no preservation exceptions.
- Run world-tool, documentation, production-linked CuTest, install, syntax-boot, and
  bounded runtime gates.
- Walk every converted zone entrance and risk-based samples of resets, quests, shops,
  portals, combat, death behavior, and scripted paths.
- Verify saved objects resolve uniquely at unchanged canonical identities.
- Apply to development, validate, and prove repeat apply is a no-op.
- Complete a development soak covering scheduled and periodic behavior.

Checkpoint:

- All acceptance criteria below pass.
- Production has not been modified.

### Phase 7: Deploy and close out

Production deployment remains a separate authorized operation.

Tasks:

- Back up every production world path that will change.
- Verify production is on the expected prior accepted release.
- Deploy the accepted development artifact without regenerating it.
- Run syntax, runtime, reference, persistence-resolution, and smoke checks.
- Retain a tested rollback artifact and decision window.
- Update permanent converter, builder, testing, help, and changelog documentation.
- Move every newly completed item from this plan into the completed-work record.

Checkpoint:

- Production matches accepted artifact hashes and passes deployment checks.
- Every resolved finding records the shipped release.

## Remaining test requirements

### Reconversion tooling

Add tests for:

- canonical RoL versus non-RoL scope classification for every record type;
- complete replacement of a deliberately changed RoL record;
- byte-identical preservation of non-RoL records in shared files and indexes;
- new, replaced, unchanged, and explicitly removed RoL records;
- reference and persistence rejection for unsafe removals;
- unchanged and changed indexes;
- hash drift, missing evidence, path escape, wrong environment, and changed runtime;
- backup, rollback, interrupted apply, and repeat no-op behavior;
- semantic-disposition completeness; and
- deterministic independent builds.

### Mobile conversion

Add remaining coverage for:

- one complete source-to-target disposition for every physical field in the mobile
  matrix, plus its loader-derived consequences;
- exact source grammar and arity validation, including the known missing position
  row, malformed race rows, trailing money token, invalid sex values, and level-zero
  records;
- all 75 RoL race codes exactly once in the source registry and every active source
  code in the corpus;
- symbolic target-constant extraction so renumbering a C race, subtype, or size
  constant either updates the manifest or makes its freshness check fail;
- every base profile in the matrix, including corrected Ogre, Troll, Naga, and
  Githyanki results;
- rejection of unknown source syntax, duplicate subtypes, invalid target constants,
  malformed atomic profiles, and non-deterministic rule-priority ties;
- automatic base-fallback results for every mixed bucket and deterministic winning
  profiles for contradictory evidence;
- proof that the optional source aggression list never changes owner race or subtype;
- preservation or explicit automatic adaptation of all 356 active aggression lists,
  including exact target-selection behavior;
- exact-phrase contradiction fixtures for known Feline, Efreeti, Angel, Vampire,
  Naga, Githyanki, Dragonkin, Humanoid Other, Possessed, Sessile, Ki-rin, and None
  failures, plus near-match cases that must not trigger;
- a full-corpus report proving every mobile is `AUTO_EXACT`, `AUTO_PHRASE`, or
  `AUTO_FALLBACK`, with zero unresolved identity decisions and zero requested manual
  classifications;
- persistence round trips for zero, one, two, and three independent subtypes, with
  all unused slots explicitly Unknown;
- proof that alignment, flight, swimming, temporary affects, equipment, zone theme,
  and reset neighbors cannot invent a subtype;
- source levels 1, 49, 50, 51, 52, 53, 54, 55, 56, 59, and authored 60 after the
  source clamp;
- ward-conditioned attack bonus and cap behavior with Iron Skin, Epic Warding, both,
  and neither;
- exact runtime defense-bypass percentage boundaries;
- full calculator results across supported levels, class/race categories, every size,
  zero through three subraces, configurations, and tiers;
- round trips proving all three subrace slots preserve independent combinations and
  unused slots remain `SUBRACE_UNKNOWN`;
- Small, Medium, Large, and larger Giant cases proving automatic stats return no size
  and the bridge assigns the deterministically selected final size afterward in every
  case;
- proof that OLC and conversion select an explicit tier before calculator invocation;
- rejection of missing, Legacy-unspecified, or invalid calculator tier input;
- batch-helper versioning, malformed I/O, stale identity, nonzero exit, and repeated
  deterministic requests;
- proof that the helper cannot choose or change the submitted tier;
- proof that Python consumes C results without a second formula or raw-stat fallback;
- hitroll and armor serialization followed by loader-equivalent round trips;
- `SpellRes:` parser, runtime load, OLC save, validator, and converter round trips,
  including explicit and race-derived source magic resistance;
- source denomination randomization, target `MOB_CUSTOM_GOLD`, effective experience,
  and tier/group reward outcomes;
- deterministic automatic disposition of all 108 positive source prestige bonuses
  and proof that the 13 negative authored values grant no source reward;
- final spawned stats proving every modifier applies exactly once;
- non-100 class-category configuration, including Sorcerer and Bard;
- actual OLC display, edit, save, reload, and rejection round trips;
- proof that load preserves an explicit tier unchanged and does not classify from
  level;
- proof that Legacy load fallback is limited to old omitted-tier records and is
  rejected from newly generated RoL output;
- preservation and deterministic disposition of authored stat overrides;
- loss or adaptation of source resistance, prestige, rewards, and special behavior;
  and
- named world-boss conversion and runtime fixtures, beginning with Tiamat.

Every repaired rule also needs positive, negative, boundary, and loss-diagnostic
fixtures. Use small fixtures for rule behavior and production-scale audits for corpus
coverage.

### Runtime and gameplay

Use production-linked CuTests for C runtime contracts and isolated syntax/runtime
boots for complete candidate worlds. Give every repaired package a walkthrough with
expected observations and a recorded result. Automated structure alone cannot prove
dialogue, encounter flow, quest intent, shop behavior, or ambience.

## Remaining acceptance criteria

The reconversion mechanism is not ready until:

- every write belongs to proven RoL scope or a required shared index update;
- every current-to-candidate difference is classified as add, replace, unchanged, or
  explicit removal;
- every current RoL edit is replaced without a preservation exception;
- non-RoL Luminari records change by zero bytes;
- every removal has reference and persistence evidence;
- apply failure restores the exact prior tree;
- repeat apply changes zero paths; and
- identical inputs produce byte-identical builds.

Accuracy recovery is not complete until:

- every active source record and meaningful directive has a non-`UNKNOWN` result;
- every physical RoL `.mob` value and every loader-derived consequence in the field
  matrix has a tested target disposition, with no silently ignored value;
- every converted mobile has an automatically selected broad race, three explicit
  subtype slots, one automatically selected final size, and a ledger entry preserving
  its exact RoL race code;
- every mixed race bucket and contradiction records the exact winning automatic rule
  or fallback, while unknown syntax and invalid atomic profiles fail generation;
- all 258 mixed-bucket records (`DK`, `K`, `N`, `OH`, `OP`, and `OU`) and every
  additional contradiction resolve without per-mobile manual review;
- every non-exact result has evidence and tests;
- no unexplained whole-record exclusion remains;
- no P0 or P1 accuracy finding remains open;
- every P2 is resolved or explicitly accepted with reviewed impact;
- every active level-51+ mobile has an automatically selected explicit tier and
  durable classifier evidence;
- every named world boss has an individual automatic profile and source-to-target
  validation;
- OLC and conversion select tier before invoking automatic stats;
- the calculator consumes but never selects or changes tier;
- generated files persist the selected tier with its calculated statistics;
- loading an explicit tier never recalculates it from level;
- OLC and conversion use the same authoritative full C calculator;
- no independent production Python stat formula exists;
- helper protocol identity and hash are sealed in release evidence;
- helper failure blocks generation without raw-stat fallback;
- no mobile retains a differently encoded source combat field without a proven
  target disposition;
- every explicit tier survives OLC save/reload and produces deterministic stats;
- zero cross-world typed references and missing reserved targets remain;
- existing Luminari identities and bytes remain protected;
- the candidate adds no normalized world validation finding;
- runtime, persistence, build, test, install, and documentation gates pass;
- package walkthroughs pass; and
- development apply, soak, completion, repeat-apply, and rollback evidence are sealed.

P3 findings may remain only in a visible, explicitly accepted backlog.

## Immediate remaining milestone

The next milestone is read-only:

1. Freeze and hash the source, conversion rules, calculator, and current development
   world.
2. Create the finding ledger and enter known defects.
3. Select one dependency-bounded pilot package with a real defect.
4. Prove the RoL/non-RoL regeneration boundary for the pilot.
5. Produce a current-to-candidate replacement plan for the pilot.
6. Demonstrate that an arbitrary current RoL edit is replaced and every non-RoL byte
   is preserved.
7. Produce the corrected staged candidate twice with identical bytes.

Do not implement or run apply until this milestone passes review.

## Risks and controls for remaining work

| Risk | Required control |
|------|------------------|
| Someone edits RoL content during recovery | Enforce the no-building freeze and document that regeneration overwrites the edit. |
| Existing non-RoL Luminari content changes | Proven scope, reserved identities, and a zero-byte gate. |
| Structurally valid but wrong output is accepted | Semantic ledgers, source evidence, tests, and walkthroughs. |
| Removed prototypes break saved state | Require explicit removal ledgers plus persistence and reference closure. |
| Partial apply leaves a mixed world | Backup, journal, candidate hash, automatic restore, and rollback. |
| Evidence is missing or stale | Archived accepted artifact with durable checksum; fail closed. |
| Production is modified accidentally | Development guard and separately authorized deployment. |
| Evidence storage grows without bound | Retention rules plus permanent accepted-release checksums. |
| One-off patches hide systemic defects | Group by converter rule and repair common causes first. |

## Related references

- [Completed RoL conversion work](ROL_CONVERSION_DONE.md)
- [World Validator, Lookup, and RoL Reconciliation CLI](../utilities/WORLD_VALIDATOR_CLI.md)
- [Testing Guide - canonical RoL maintenance gate](../guides/TESTING_GUIDE.md#canonical-rol-maintenance-gate)
- [Realms of Luminari help entry](../../lib/text/help/realms_of_luminari.hlp)
- `scripts/world/wtool_lib/rol_source.py`
- `scripts/world/wtool_lib/rol_transform.py`
- `scripts/world/wtool_lib/rol_soc.py`
- `scripts/world/wtool_lib/rol_special.py`
- `scripts/world/wtool_lib/rol_phase7.py`
- `scripts/world/wtool_lib/rol_phase8.py`
- `scripts/world/rol_conversion_policy.json`
- `EXAMPLE/RealmsOfLuminari/src/race_class.h`
- `EXAMPLE/RealmsOfLuminari/src/race_class.c`
- `EXAMPLE/RealmsOfLuminari/src/db.c`
- `src/structs.h`
- `src/combat/encounters.c`
- `src/quest/hunts.c`
- `src/spec/spec_rol_*.c`
