# RoL Conversion Accuracy Recovery and Maintenance Plan

Status: Active plan; implementation not started

Last reviewed: 2026-08-16

## Plain-language goal

The first RoL conversion successfully moved a very large old world into a separate,
safe Luminari namespace. It proved that the generated world was deterministic,
isolated from existing Luminari content, structurally valid relative to its baseline,
and safe to apply as an additive overlay.

It did not prove that every converted room, mobile, object, reset, shop, quest,
script, trap, and special behavior accurately preserves the old world's meaning.
A converter can consistently produce the same wrong answer. The converted content
is now reported to be substantially inaccurate and incomplete, so semantic fidelity
is reopened as active work.

This project has two connected goals:

1. Teach the converter to safely replace RoL content that it previously generated.
2. Audit and repair the conversion until every active source record has an explicit,
   reviewable accuracy result.

## Current state

The repository still contains the complete conversion toolchain and the ignored RoL
source tree. The current source inventory selects 258 packages and the accepted final
conversion contains 71,680 record actions. The accepted Phase 8 overlay added 1,206
world-data files and replaced the seven relevant world indexes.

The existing safety model remains valuable:

- RoL zones use the reserved range 20000-29999.
- RoL rooms, mobiles, objects, and typed owners use 2000000-2999999.
- Existing Luminari identities are not aliases for similar RoL content.
- Generation happens in a staging world before any development write.
- Release candidates are hash-guarded and development-only.
- Reapplying the same accepted bundle is a verified no-op.
- The superseded destructive Phase 6.5 rehome remains disabled.

The missing capability is safe *new* generation on top of an already-applied RoL
overlay. Phase 7 currently treats most generated records as new additions and rejects
collisions. That was correct for the first import, but it prevents routine correction
of converter-owned records already present in `lib/world`.

## Non-negotiable rules

- Never run conversion or maintenance writes against production.
- Never re-enable or reuse the destructive Phase 6.5 rehome.
- Never change the canonical RoL VNUM formulas as part of an accuracy repair.
- Never treat matching names, themes, or low VNUMs as identity evidence.
- Never silently overwrite a builder or OLC edit.
- Never hand-edit generated output as the only source of truth.
- Never delete a converted prototype automatically. Deletion requires a separate
  persistence and reference plan.
- Never modify existing Luminari records to make RoL content fit.
- Every changed conversion rule requires focused tests and regenerated evidence.
- Every apply must have an exact, tested rollback bundle.

## Three copies used by maintenance mode

Maintenance needs three versions of each converter-owned record:

| Name | Meaning |
|------|---------|
| Previous generated | What the last accepted converter release produced. |
| Current target | What is in the development world now, including later builder or OLC edits. |
| Newly generated | What the corrected converter wants to produce. |

The tool must compare all three before deciding what to do:

| Situation | Maintenance action |
|-----------|--------------------|
| Current equals previous; new differs | Safe converter replacement. |
| Current already equals new | No-op. |
| New equals previous; current differs | Preserve the builder edit. |
| Current and new both changed differently | Conflict; stop and request an explicit decision. |
| Record is new in the source | Add it at its canonical RoL identity. |
| Previously generated record is now missing from the source | Block; never infer deletion. |
| Previously generated record is missing from the target | Conflict; investigate the removal. |

This is the central safety rule. The converter may replace its own unchanged work, but
it may not guess when a human and the converter both changed the same record.

## Proposed maintenance commands

The final names may change during implementation, but responsibilities must remain
separate:

```text
rol-maintenance-plan       read-only ownership and three-way comparison
rol-maintenance-build      generate and validate an isolated corrected candidate
rol-maintenance-apply      hash-guarded development apply with backup and rollback
rol-maintenance-completion post-apply validation and repeat-apply no-op proof
```

`rol-maintenance-plan` and `rol-maintenance-build` must perform no live world writes.
`rol-maintenance-apply` must retain the existing `APP_ENV=development` guard.

## Required design

### 1. Accepted-output ownership

The maintenance system must know exactly what the prior converter owned. A high VNUM
alone is not enough because one physical world file can contain multiple records and
index files also contain non-record data.

Each accepted release must preserve:

- source record ID, kind, source VNUM, path, line, and source hash;
- canonical target type, VNUM, physical path, and record-block hash;
- generated triggers and attachment ownership;
- special-procedure or compatibility behavior ownership;
- accepted action and any repair, adaptation, merge, or exclusion disposition;
- full generated output needed for the next three-way comparison; and
- release, source-tree, converter-code, policy, and target-tree hashes.

The current ignored run directory is useful evidence but is not a durable deployment
contract by itself. The implementation must define an archived accepted-release
artifact and a small durable pointer/checksum that lets a new checkout retrieve and
verify it. Maintenance must stop if the prior accepted output cannot be proven.

### 2. Record-level comparison

Comparison must parse records instead of relying only on whole-file hashes. It must
understand `.zon`, `.wld`, `.mob`, `.obj`, `.shp`, `.trg`, `.qst`, and `.hlq` framing,
including files containing more than one record.

The comparison output must include:

- records safe to replace;
- records already current;
- preserved builder edits;
- true three-way conflicts;
- newly added records;
- suspicious removals;
- index changes;
- attachments, references, and special bindings affected by each action; and
- before and after hashes for every proposed write.

No conflict may be hidden inside a file-level `REPLACE` action.

### 3. Builder-edit decisions

Builder changes need explicit outcomes:

- `PRESERVE`: keep the current target record unchanged;
- `TAKE_GENERATED`: replace it after documented builder review;
- `PORT_EDIT`: carry the builder's intended change into a converter rule or source
  repair, then regenerate; or
- `DEFER`: omit the package from the release until the conflict is resolved.

These decisions belong in a reviewable ledger and must be inputs to candidate
generation. Command-line confirmation without a durable record is insufficient.

### 4. Safe apply and rollback

Before writing development data, the apply command must:

1. Verify the accepted prior release, new release, runtime binary, policy, and current
   target hashes.
2. Verify that every planned target remains in `lib/world`.
3. Save exact copies and hashes of every path that could change.
4. Write an apply journal before the first replacement.
5. Install only approved record/file changes and required index updates.
6. Validate the resulting tree against the accepted candidate.
7. Restore the backup automatically if application or validation fails.
8. Prove that immediate reapplication changes zero paths.

Rollback must be a tested command, not merely a directory containing old files.

### 5. No automatic deletion

The current policy has no delete action, and maintenance must keep that protection.
Removing a room, mobile, or object can break resets, exits, keys, quests, saved player
objects, houses, artifacts, and database references.

A requested retirement must become a separate reviewed change with typed reference
closure, persistent-owner evidence, replacement behavior, rollback, and deployment
instructions. Until then, the maintenance candidate must stop.

## Accuracy and completeness model

Structural validation answers, "Can Luminari read this?" Accuracy work must also
answer, "Did we preserve what RoL meant?"

Every active source record and meaningful directive needs one of these dispositions:

| Disposition | Meaning |
|-------------|---------|
| `EXACT` | Preserved without semantic change. |
| `MAPPED` | Converted through a reviewed, tested value or flag mapping. |
| `ADAPTED` | Implemented with a different Luminari mechanism that preserves intent. |
| `REPAIRED` | Source defect was corrected using explicit evidence. |
| `EXCLUDED` | Smallest unsafe or impossible unit was intentionally omitted with a reason. |
| `UNKNOWN` | Accuracy has not been established; this blocks final acceptance. |

The ledger must cover at least:

- package and record presence;
- room names, descriptions, sectors, flags, sizes, exits, doors, and keys;
- zone ranges, reset order, reset dependencies, probabilities, limits, and schedules;
- mobile text, flags, affects, race, class, level, combat values, positions, and loot;
- object text, type, flags, wear slots, values, applies, affects, spells, containers,
  traps, decay, and equipment behavior;
- shops, products, hours, restrictions, prices, messages, and keeper behavior;
- quests, dialogue, prerequisites, costs, rewards, commands, and host bindings;
- SOC modes, actions, paths, calendars, targets, and generated DG triggers;
- direct, dynamic, implicit, composite, periodic, combat, death, and object special
  procedures;
- typed references, portals, transported destinations, scripted loads, and attachments;
- bounded source repairs and every converter diagnostic; and
- player-visible behavior under the target runtime.

An `EXCLUDED` result is not automatically acceptable. It needs the smallest possible
scope, a player-impact statement, source evidence, and explicit review.

### High-priority mobile level and encounter-tier recovery

Mobile scaling is a priority semantic repair. The current converter does not perform
a real RoL-to-Luminari power conversion: it clamps every source level above 34 to 34
while retaining much of the source combat row. This destroys the ordering among
high-level mobiles and can combine RoL hit points and damage with unrelated Luminari
level, class, save, spell, and powerful-being behavior. The copied hitroll also uses
RoL's direct-bonus meaning in a target file field that Luminari loads as `20 - value`,
which is a format mismatch that must be corrected before balance judgments are made.
Treat this systemic high-end scaling defect as P1 major combat behavior until the
effective-stat audit and representative encounter tests prove otherwise.

The intended level boundary is:

- ordinary RoL levels 1-50 map into Luminari levels 1-30 using a reviewed mapping and
  Luminari-native automatic stats;
- RoL level 51 and higher maps into the Luminari 31-34 powerful-being range; and
- a source file value above RoL's runtime maximum must first receive the same clamp as
  the RoL loader. In particular, a source value of 60 behaves as 59 in RoL.

Special encounter strength is needed primarily for the source 51+ population. The
mechanics study found that Luminari already implements an implicit tier ladder for
levels 31-34. `autoroll_mob()` multiplies generated hit points by 2, 4, 6, or 8 and
adds 1-4 damage bonus at those levels. Runtime powerful-being code then adds 500 hit
points, applies one cumulative ten-percent increase per level from 31 through 34,
adds 2/3/4/5 armor class, adds 2/3/5/7 attack bonus when the defender lacks the two
specific wards, grants 1-4 additional attacks at maximum BAB, and gives a nominal
30/40/50/60 percent chance to bypass certain critical defenses. This makes the
effective high-level hit-point progression approximately x2.5, x5, x8, and x12 over
an ordinary same-level automatic-stat baseline.

Level must stop doing two jobs. Target level should determine creature competence,
including BAB, spell access, caster level, saves, and level-based class or racial
behavior. Encounter tier should separately determine how many players the encounter
is intended to challenge. The existing level-31-to-34 bonuses are compatibility
evidence for the first tier profiles, not another layer to stack on top of them.

Do not implement the five tiers as five physical mobile action flags. Mobile action
flags use four 32-bit words, indices 0-125 are already assigned, and only two storage
bits remain. Expanding `PM_ARRAY_MAX` would change shared player/mobile flag storage,
world-file grammar, OLC serialization, and the versioned saved-pet runtime payload.
Instead add one persisted scalar mobile field, provisionally serialized as `Tier:`
and exposed in OLC as one named, mutually exclusive choice:

| Value | Provisional name | Intended encounter role |
|------:|------------------|-------------------------|
| 0 | `MOB_TIER_STANDARD` | Ordinary same-level mobile with no encounter-size modifier. |
| 1 | `MOB_TIER_ELITE` | Tough solo encounter or challenge for one or two players. |
| 2 | `MOB_TIER_SMALL_GROUP` | Designed for the documented small group of two or three players. |
| 3 | `MOB_TIER_BIG_GROUP` | Designed for a full or large group of four to six players. |
| 4 | `MOB_TIER_RAID` | Designed for a larger coordinated group or multiple groups. |
| 5 | `MOB_TIER_WORLD_BOSS` | Individually reviewed, uniquely important world boss such as Tiamat. |

The names may be C enum constants even though the values are not action-bit flags.
The tier must be a general Luminari mechanic, not an RoL-only label. Every converted
RoL 51+ mobile must receive an explicit reviewed tier classification; a decision
based only on source level is not sufficient. Zone role, reset context, special
procedures, spellcasting, number of attacks, defenses, source combat values, loot,
and named-boss identity must inform the classification. World bosses require an
explicit identity list and individual builder review rather than an automatic level
rule.

Use this compatibility profile as the initial calculator input, not as final balance
approval:

| Tier | HP factor | Attack bonus | Armor class | Damage bonus | Extra max-BAB attacks | Defense bypass |
|------|----------:|-------------:|------------:|-------------:|----------------------:|---------------:|
| Elite | x2.5 | +2 | +2 | +1 | +1 | 30 percent |
| Small group | x5 | +3 | +3 | +2 | +2 | 40 percent |
| Big group | x8 | +5 | +4 | +3 | +3 | 50 percent |
| Raid | x12 | +7 | +5 | +4 | +4 | 60 percent |
| World boss | Raid floor plus a required named profile | Profile | Profile | Profile | Profile | Profile |

Apply hit-point factors multiplicatively to the class-adjusted automatic-stat
baseline. Apply attack bonus, armor class, damage bonus, and saving throw adjustments
additively because each point changes a d20 probability directly. Do not use a
percentage modifier for attack bonus, armor class, saves, or spell resistance. The
initial universal saving-throw and spell-resistance tier adjustments are zero:
class, race, size, and level already produce large differences, and an automatic
spell-resistance increase would punish caster groups without an equivalent martial
cost. Keep both fields in the profile so later measurements can justify a reviewed
nonzero value.

At level 34, the current warrior automatic-stat baseline is about 1,496 hit points,
four attacks, +48 attack bonus before the powerful-being adjustment, and roughly
31.5 average bare-hand damage before hit chance. The compatibility profile produces
approximately 3,740/7,480/11,968/17,952 generated hit points and 5/6/7/8 attacks for
Elite/Small Group/Big Group/Raid. These are deterministic comparison anchors. They
must be tested against representative martial, divine, rogue, and arcane mobiles and
against real level-30 player builds before they become balance policy.

The existing `autoroll_mob()` mechanic in `src/olc/medit.c` should become the
Luminari-native baseline instead of preserving raw RoL combat rows. Implementation
must first extract or expose one authoritative, deterministic stat-profile
calculation so OLC, conversion, validation, and tests cannot drift into separate
formulas. Tier profiles then apply reviewed modifiers to that baseline for hit
points, accuracy, armor class, saves, damage, spell resistance, rewards, and any
other affected mechanics. The tier must remain visible when a builder loads, edits,
saves, and inspects the mobile.

Do not independently duplicate the `autoroll_mob()` formulas in Python. Refactor the
calculation into a side-effect-free C function with explicit inputs and outputs. Its
inputs must include at least target level, race, subraces, size, class, and encounter
tier. Its outputs must contain every generated stat and enough expected post-load
state to detect later loader or runtime modifiers being applied twice. The existing
`autoroll_mob()` remains the in-game and OLC wrapper that applies this shared result
to a `char_data`.

Expose the same C calculator to the Python conversion pipeline through a small,
versioned batch utility. Python must send the mapped mobile identities and tiers to
one helper process and consume the returned target-native stat records; it must not
start one process per mobile. The helper protocol, calculator code, configuration,
and binary hash become release inputs. Conversion must stop if the expected helper
is missing, stale, malformed, or disagrees with its declared version. It must never
fall back to retaining the old RoL combat row.

The calculator must be deterministic for identical inputs. Remove generation-time
random rolls from the calculation or express randomness as a stable target dice
formula that is rolled only when the mobile loads. In particular,
`autoroll_mob()` currently calls `dice(1, level)` while selecting the stored hit-point
die size; that generation-time random choice must not enter the shared profile.

Use one explicit calculation and load order:

1. Map and clamp target level, then select reviewed class, race, subraces, and size.
2. Calculate the ordinary Luminari automatic-stat baseline.
3. Apply the persisted encounter-tier profile exactly once.
4. Apply any individually approved custom-stat profile or builder override, with a
   durable reason; never silently combine automatic and custom values.
5. Serialize runtime attack bonus and armor class through the inverse file encodings.
6. At spawn, roll only the stable persisted hit-point dice expression.
7. Apply configured class-category modifiers exactly once, or make their values
   explicit calculator inputs and remove the later duplicate application.
8. Derive non-persisted combat behavior such as tier attacks and defense bypass from
   the tier value without multiplying the persisted statistics again.

Remove the separate group-size hit-point and combat bonuses from the level-31-to-34
paths once tier-aware profiles own them. Other rules that genuinely mean "epic
creature," such as summon or teleport restrictions, may remain level-based after an
explicit review. This separation prevents a level-34 Raid mobile from receiving the
old level-34 raid bonuses and the new Raid bonuses together.

The configured class-category layer also needs reconciliation. Current configuration
values are all 100 percent, so the layer is dormant, but its category table classifies
Sorcerer and Bard as Warrior while `autoroll_mob()` treats Sorcerer as an arcane caster
and Bard as a lighter rogue-like caster. Tests must use non-100 configuration values
to expose category drift and double application before release.

If implementation adds a source or test file for the shared calculator or batch
utility, update both `Makefile.am` and `CMakeLists.txt`.

An independently maintained Python formula is allowed only as an explicitly approved
fallback design. It would require exhaustive golden-vector parity tests against the
C calculator for every supported level, class category, race category, size, and
tier, plus a gate that fails immediately on any disagreement. It is not the planned
implementation.

Automatic stat generation must not silently erase a deliberate builder override.
The tier-aware auto-stat operation should run when generating or explicitly
requested by a builder, not blindly overwrite every mobile at every boot. Any
exception to the tier profile needs a durable per-mobile reason and focused review.
The converter must serialize the resulting target values in the exact representation
expected by the Luminari loader, including the inverse hitroll and armor-class file
encodings.

World-boss classification is not permission to apply one larger generic multiplier.
The World Boss profile inherits the Raid compatibility floor, then requires a named
profile and individual behavior review. Converted Tiamat is the first required case:
her runtime procedure pins the first form near 30,000 hit points, removes several
disabling effects, breathes across the room, regenerates, and creates a second
30,000-hit-point dracolich form. The native Prisoner encounter uses a different
multi-form, multi-life, healing, breath, and lethal-special design. Those encounters
show why phase count, regeneration, room-wide damage, control immunity, summoning,
and special-procedure cadence must be budgeted alongside raw statistics.

Do not freeze tier reward factors with the combat seed table. Stored mobile
experience is divided by three on a kill and then divided among present group
members. The current normal-campaign two-percent group bonus uses integer division,
so realistic group sizes receive no intended percentage multiplier. Serialized gold
is also normally replaced unless `MOB_CUSTOM_GOLD` is set. Repair or explicitly
preserve those reward semantics, then calculate experience, gold, and loot against
measured party effort rather than copying the hit-point factor.

A read-only parse of the current indexed world establishes the size of the immediate
balance gap. Using average prototype dice and the current powerful-being spawn
increase, the approximate median spawned hit points are:

| Target level | Native Luminari mobiles | Converted RoL mobiles |
|-------------:|------------------------:|----------------------:|
| 31 | 2,800 | 2,300 |
| 32 | 5,800 | 2,600 |
| 33 | 9,700 | 2,900 |
| 34 | 14,700 | 7,900 |

The converted level-31-to-33 medians are nearly flat and the converted level-34
median is about half the native value. The indexed parse also finds roughly 5,700
converted mobiles at level 34, confirming that this level currently mixes ordinary
source-51+ creatures with actual raid and world-boss candidates. Preserve the exact
audit command and machine-readable counts in the finding ledger when implementation
begins; the rounded values above are planning evidence, not a release artifact.

Initial mobile-scaling findings to enter in the finding ledger include:

- all source levels 35-60 currently collapse to target level 34, not a scaled ladder;
- 5,617 of 12,406 active source mobiles are above level 34 and currently collapse;
- direct RoL hitroll is copied into Luminari's inverse-encoded hitroll field;
- source hit-point and damage dice are retained and may then receive Luminari's
  level-31+ powerful-being hit-point increase;
- source armor values are not first bounded the way the RoL loader bounded them;
- source magic resistance and prestige bonus are not generally carried into the
  target mobile; and
- source gold is written, but Luminari normally generates replacement gold unless
  `MOB_CUSTOM_GOLD` is set;
- the converted level-31-to-33 hit-point progression is nearly flat and converted
  level-34 median spawned hit points are roughly half the native median;
- five physical tier flags do not fit in the remaining two mobile action bits without
  a broad persistence and file-format migration;
- generation-time random selection of the automatic hit-point die size prevents a
  deterministic calculator result; and
- configured class-category membership disagrees with automatic-stat treatment for
  at least Sorcerer and Bard, although current 100-percent values mask the difference.

This work is not complete when the tier field merely exists. It is complete when the tier
classification drives deterministic Luminari-native stats, survives OLC round trips,
is visible to builders and audits, and has been tested in representative encounters.

## Finding ledger

Create one durable ledger for reported and discovered inaccuracies. Each finding must
include:

- stable finding ID;
- severity and status;
- affected package;
- canonical target VNUM and record type;
- source VNUM, path, and line when known;
- observed target behavior;
- expected source behavior and supporting evidence;
- suspected conversion layer;
- whether the defect is systemic or record-specific;
- proposed disposition and owner;
- regression test or walkthrough needed; and
- release that fixes it.

Reports that name only an old source VNUM are incomplete. Reports should name the
canonical target VNUM and the specific room, mobile, object, reset, quest, shop,
script, trap, path, or special behavior.

Use these priorities:

| Priority | Examples |
|----------|----------|
| P0 | Crash, boot failure, corruption, unsafe command, broken persistence, or inaccessible required content. |
| P1 | Broken topology, resets, progression, required keys, quests, shops, portals, or major combat behavior. |
| P2 | Incorrect stats, flags, equipment, loot, prices, timing, secondary scripts, or noticeable mechanical fidelity. |
| P3 | Text, color, formatting, ambience, cosmetic timing, and low-impact fidelity. |

## Implementation phases

### Phase 0: Reopen acceptance and freeze evidence

Tasks:

- Preserve and checksum the most recent accepted source, Phase 7, Phase 8, and
  completion evidence.
- Identify the exact accepted overlay currently present in development.
- Capture the current development tree without changing it.
- Create the finding-ledger schema and enter all presently known inaccuracies.
- Record which checks demonstrated structural safety and which semantic questions
  remain unproved.
- Select one inaccurate but dependency-bounded package as the maintenance pilot.

Checkpoint:

- The old generated version, current target version, and source version of every pilot
  record can be reproduced and independently verified.
- No world or database write has occurred.

### Phase 1: Build accepted-output ownership

Tasks:

- Add record-block hashing and ownership metadata to release bundles.
- Recover ownership for the current accepted overlay from its Phase 7 action ledger,
  Phase 8 output, trigger attachments, and special ledgers.
- Detect shared physical files, duplicate source identities, merged HLQs, generated
  triggers, and composite special bindings.
- Reject records whose previous generated bytes cannot be proven.
- Define archival and retrieval rules for accepted release artifacts.

Checkpoint:

- Every converter-owned record and generated trigger in the pilot has exactly one
  ownership result.
- Ordinary Luminari records are classified as non-owned and cannot enter a replacement
  plan.

### Phase 2: Implement read-only three-way planning

Tasks:

- Add the maintenance-plan command.
- Compare previous generated, current target, and newly generated records.
- Emit the action, conflict, builder-edit, removal, reference-impact, and index ledgers.
- Add stable machine-readable output and concise human summaries.
- Add failure tests for missing prior evidence, changed hashes, path escape, type
  confusion, duplicate identities, ambiguous ownership, and malformed records.

Checkpoint:

- The command explains every pilot difference and performs zero writes.
- A deliberately edited pilot record becomes `PRESERVE` or `CONFLICT`, never a silent
  replacement.

### Phase 3: Implement maintenance build, apply, and rollback

Tasks:

- Generate a pilot candidate in an isolated staging tree.
- Carry explicit builder decisions into the candidate.
- Produce hash-preconditioned `ADD`, `REPLACE`, and no-op actions without adding a
  general delete action.
- Add pre-apply backup, journal, automatic failure recovery, explicit rollback, and
  completion evidence.
- Retain development-only and runtime-binary guards.
- Run apply twice and require the second run to change zero paths.

Checkpoint:

- The pilot can move from old accepted output to corrected output and back again.
- No non-RoL record changes.
- Interrupted or failed apply testing restores the exact original tree.

### Phase 4: Build the semantic accuracy audit

Tasks:

- Extend the capability audit from "a handler exists" to field- and behavior-level
  fidelity dispositions.
- Compare source semantics with the generated target model after accounting for
  reviewed mappings and adaptations.
- Turn all silent fallback paths and lossy diagnostics into ledger rows.
- Gate unknown syntax, default-to-human races, unsupported flags, removed references,
  dropped resets, unavailable spells, synthesized values, and broad exclusions.
- Add an effective-mobile-stat audit that compares RoL runtime meaning with the
  Luminari values after file loading, automatic stats, class/race behavior, and tier
  modifiers. Include level, hitroll, armor class, hit points, damage, saves, spell
  resistance, experience, gold, and special behavior.
- Require an explicit tier-classification result for every active RoL level-51+
  mobile and an explicit individually reviewed result for every world boss.
- Produce per-package summaries and global counts for `EXACT`, `MAPPED`, `ADAPTED`,
  `REPAIRED`, `EXCLUDED`, and `UNKNOWN`.

Checkpoint:

- The pilot has no unreported semantic loss.
- Every non-exact result links to a rule, evidence, test, and player-impact statement.

### Phase 5: Repair the corpus in dependency-complete waves

Fix systemic converter rules before patching individual records. A mapping bug that
affects 500 objects should be fixed once and regenerated, not patched 500 times.

Recommended waves:

1. Source inventory, parsing, missing records, identity, and typed references.
2. Rooms, exits, doors, keys, zone ranges, resets, and traversal.
3. Mobiles, beginning with level mapping, the shared auto-stat baseline, encounter
   tier persistence, level-51+ classification, hitroll/armor serialization, rewards, magic
   resistance, and named world bosses; then objects, equipment, loot, containers,
   spells, affects, and traps.
4. Shops, quests, rewards, dialogue, prerequisites, and progression.
5. SOC, DG triggers, paths, schedules, portals, and attachments.
6. Native and adapted special procedures, including combat, death, periodic, and
   composite behavior.
7. Text, color, formatting, ambience, and remaining cosmetic fidelity.

For each wave:

- triage findings by common root cause;
- add a focused failing test or source/target fixture;
- fix the parser, mapping, emitter, compiler, explicit repair, or runtime adapter;
- regenerate only through the maintenance pipeline;
- review all three-way conflicts;
- build the same candidate twice and compare bytes;
- validate the dependency closure, not only the named package;
- perform targeted builder walkthroughs; and
- update the finding and semantic-disposition ledgers.

Checkpoint:

- The wave has no open P0 or P1 finding in its declared scope.
- It introduces no new normalized world finding and no unresolved typed edge.

### Phase 6: Full-corpus release validation

Tasks:

- Generate two independent full-corpus maintenance candidates.
- Require byte-identical generated output for identical inputs.
- Review every builder edit and conflict.
- Run the complete world-tool, documentation, production-linked CuTest, install,
  syntax-boot, and bounded runtime gates.
- Walk every converted zone entrance and a risk-based sample of resets, quests, shops,
  portals, combat encounters, death behavior, and scripted paths.
- Verify that saved objects still resolve uniquely at unchanged canonical identities.
- Apply to development, run post-apply validation, and prove repeat apply is a no-op.
- Hold a development soak period long enough to exercise scheduled and periodic
  behavior.

Checkpoint:

- The maintenance release satisfies the acceptance criteria below.
- Production deployment has not yet occurred.

### Phase 7: Production deployment and closeout

Production deployment is a separate authorized operation, not a converter command.

Tasks:

- Back up every production world path that will change.
- Verify production is on the expected prior accepted RoL release.
- Deploy the already accepted development artifact without regenerating it.
- Run syntax, runtime, reference, persistence-resolution, and smoke checks.
- Retain a tested rollback artifact and a clear rollback decision window.
- Update permanent converter, builder, testing, help, and changelog documentation.
- Move durable rules out of this working plan when the project completes.

Checkpoint:

- Production matches the accepted artifact hashes and passes the deployment checks.
- The finding ledger records the shipped release for every resolved defect.

## Test requirements

### World-tool tests

Add focused tests for:

- all three-way decision-table outcomes;
- record ownership in shared files;
- builder edit preservation and true conflicts;
- new records, suspicious removals, and forbidden deletions;
- unchanged and changed index files;
- hash drift, missing evidence, path escape, wrong environment, and changed runtime;
- backup, rollback, interrupted apply, and repeat no-op behavior;
- semantic disposition completeness; and
- deterministic output from independent builds.

### Converter fixtures

Each repaired rule needs positive, negative, boundary, and loss-diagnostic fixtures.
Fixtures should use small source records while production-scale audits prove full-corpus
coverage. Do not duplicate production parser or emitter logic in tests.

Mobile conversion fixtures must additionally cover:

- the source level boundaries 50, 51, 59, and file value 60;
- each encounter tier, invalid scalar values, and attempted ambiguous classifications;
- direct golden-vector tests of the side-effect-free C calculator;
- batch-helper protocol versioning, malformed input/output, stale binary, nonzero
  exit, and deterministic repeated requests;
- proof that the Python converter consumes calculator output without implementing a
  second stat formula or falling back to source combat rows;
- target hitroll and armor-class serialization followed by a loader-equivalent round
  trip;
- deterministic auto-stat results for each level, class/race category, and tier;
- final spawned-stat tests proving group, powerful-being, and class-category
  modifiers are applied exactly once;
- non-100 class-category configuration tests, including Sorcerer and Bard;
- OLC display, edit, save, reload, and rejection tests for the scalar tier field;
- preservation and explicit review of manual stat overrides;
- loss or adaptation of source magic resistance, prestige, rewards, and special
  behavior; and
- at least one named world-boss fixture, with Tiamat as the required first case.

### Runtime tests

Use production-linked CuTests for native special adapters, compatibility flags, combat,
death, periodic behavior, traps, resets, object values, and other C runtime contracts.
Use isolated syntax and bounded runtime boots for complete candidate worlds.

### Builder and gameplay tests

Automated structural checks cannot establish whether dialogue, encounter flow, quest
intent, shop behavior, or ambience feels correct. Each repaired package needs a short
walkthrough script with expected observations and a recorded result.

Mobile tier sign-off needs a development-only combat matrix using representative
level-30 builds rather than one synthetic average character. Include at least a solo
martial, solo caster, two-or-three-player small group, four-to-six-player full group,
and a larger coordinated raid. Exercise martial, divine, rogue, and arcane mobiles at
each relevant tier. Record time to kill, player deaths, incoming and outgoing damage
per round, mobile hit rate, player hit rate, save success, spell-resistance failure,
healing pressure, actions taken per round, control uptime, regeneration, summons,
and rewards per present player. Run special-procedure and known-spell encounters
separately from plain automatic-stat mobiles so scripted cadence is not mistaken for
a tier-stat effect. Compatibility factors may be adjusted only from preserved results
and must remain versioned calculator inputs.

## Acceptance criteria

The maintenance mechanism is ready only when:

- every proposed write belongs to a proven converter-owned record or required index;
- every current target difference is classified, with zero silent builder overwrite;
- non-RoL Luminari records change by zero bytes;
- removal and identity changes are blocked by default;
- apply failure can restore the exact prior tree;
- reapplying an accepted maintenance bundle changes zero paths; and
- two builds from identical inputs produce byte-identical generated output.

The accuracy recovery is complete only when:

- every active source record and meaningful directive has a non-`UNKNOWN` disposition;
- every `MAPPED`, `ADAPTED`, `REPAIRED`, or `EXCLUDED` result has evidence and tests;
- no unexplained whole-record exclusion remains;
- no P0 or P1 accuracy finding remains open;
- all P2 findings are resolved or explicitly accepted with player-impact review;
- every active RoL level-51+ mobile has a reviewed encounter-tier classification;
- every tier value has one documented mechanical profile, rejects invalid or ambiguous
  classification, survives OLC save/reload, and produces deterministic automatic stats;
- OLC and conversion use the same authoritative C stat calculator, with no independent
  production Python copy of its formulas;
- the conversion batch-helper identity and hash are recorded in the release evidence,
  and helper failure stops generation without a raw-stat fallback;
- no converted mobile relies on an unreviewed raw RoL combat value in a differently
  encoded Luminari field;
- every named world boss, beginning with Tiamat, has an individual source-to-target
  combat and behavior review;
- zero cross-world typed references and zero missing reserved-namespace targets remain;
- existing Luminari identities and bytes remain protected;
- the candidate adds no normalized world validation finding;
- runtime, persistence-resolution, build, test, install, and documentation gates pass;
- targeted package walkthroughs pass; and
- development apply, soak, completion, repeat apply, and rollback evidence are sealed.

P3 cosmetic findings may remain only in a visible backlog with explicit acceptance.
They must not be mislabeled as verified fidelity.

## Immediate milestone

The first milestone is deliberately read-only:

1. Preserve the last accepted Phase 8 output as the proven previous-generated input.
2. Create the finding ledger and enter the known inaccurate or incomplete examples.
3. Select one dependency-bounded pilot package with at least one real reported defect.
4. Produce record-level ownership for that package.
5. Implement the three-way maintenance plan for that package.
6. Demonstrate that a builder-edited record is preserved or reported as a conflict.
7. Produce a corrected staged candidate twice with identical bytes.

No apply command should be implemented or run until that milestone passes review.

## Risks and controls

| Risk | Control |
|------|---------|
| Builder or OLC work is erased | Mandatory record-level three-way comparison and explicit conflicts. |
| Existing Luminari content is changed | Proven ownership, reserved identities, preservation audit, and zero-byte gate. |
| A structurally valid but wrong conversion is accepted again | Semantic disposition ledger, source evidence, focused tests, and walkthroughs. |
| A removed prototype breaks saved state | No automatic deletion; separate persistence and reference closure. |
| A partial apply leaves a mixed world | Pre-write backup, journal, candidate hash, automatic restore, and tested rollback. |
| Old evidence is missing or stale | Archived accepted artifact with a durable checksum; fail closed. |
| Production is modified accidentally | Development environment guard and separate authorized deployment process. |
| The evidence store grows without bound | Define retention for scratch runs while permanently preserving accepted releases and checksums. |
| One-off patches hide a systemic defect | Group findings by converter rule and repair systemic causes first. |

## Related references

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
- `src/spec/spec_rol_*.c`
