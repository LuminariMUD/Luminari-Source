# RoL Conversion Accuracy Recovery - Remaining Work

Status: Active; this file contains unfinished work only

Last reviewed: 2026-08-17

Completed implementation, research, and verification evidence is recorded in
[ROL_CONVERSION_DONE.md](ROL_CONVERSION_DONE.md). Move work there as soon as it is
finished and verified.

## Remaining outcome

The converted world is not yet accepted as accurate or complete. The remaining
project must:

1. Add built-in write-ahead journaling, automatic restore, and an exact rollback
   command for future development applications.
2. Audit every active source record and meaningful directive for semantic fidelity.
3. Validate spawned-state and gameplay behavior of the boot-verified recovered
   candidate.
4. Repair any candidate or runtime defects found by that validation.
5. Validate, review, soak, and deploy an accepted reconversion release with tested
   rollback.

## Remaining priority problems

- Full-namespace generation, non-RoL preservation, development installation, and a
  verified full-tree backup are proven, but the maintained apply command still lacks
  built-in backup, a write-ahead journal, automatic restore, and explicit rollback.
- There is no complete all-record semantic-disposition ledger proving whether each
  source record and directive is exact, mapped, adapted, repaired, or excluded.
- The byte-identical recovered candidates pass syntax boot, full zone reset, bounded
  live runtime, and Phase 8 release sealing. Generated mobiles have not yet been
  spawned or compared with their generation ledgers.
- The automatic level-51+ tier results still need encounter-context review using zone
  and reset role, simultaneous population, procedures, spells, equipment, keys,
  progression role, and neighboring-creature rank. Any resulting rule adjustment
  must remain deterministic and versioned.
- Full-corpus gameplay, encounter balance, walkthrough, soak, release, rollback, and
  production evidence does not exist for an accuracy-recovery release.
- Tiamat's two generated profiles still require a booted encounter validation of
  cadence, control, transition, rewards, and expected group size.

Treat high-end mobile behavior as P1 until effective spawned-stat comparison and
representative encounter tests pass.

## Non-negotiable rules

- Never run conversion or reconversion writes against production.
- Never re-enable or reuse the destructive Phase 6.5 rehome.
- Never change the canonical RoL VNUM formulas as part of an accuracy repair.
- Never treat similar names, themes, or low VNUMs as proof that source and target
  records share identity.
- Do not permit builder or OLC changes in the RoL namespace until conversion accuracy
  is locked. Accidental edits in that namespace are disposable generated output.
- Treat the frozen RoL source, versioned rules, authoritative C calculator, and
  generated candidate as the recovery authorities. Never hand-edit generated output
  as a source of truth.
- Remove a generated RoL prototype only when the candidate proves it is absent from
  authoritative output and reference and persistence checks permit removal.
- Never modify existing non-RoL Luminari records to make RoL content fit.
- Never introduce per-mobile human classification. Any new identity or tier rule must
  remain deterministic, versioned, and fully evidenced.
- Every changed rule requires focused tests and regenerated evidence.
- Every apply requires an exact, tested rollback bundle.

## Remaining safe-release implementation

Full-namespace regeneration, mixed-file non-RoL preservation, deterministic output,
and the read-only persistence check are complete and recorded in
[ROL_CONVERSION_DONE.md](ROL_CONVERSION_DONE.md). The remaining tooling work is safe
application, exact rollback, and richer semantic release evidence.

### Reconversion commands

Keep the established command responsibilities and add the missing rollback path:

```text
rol-phase7             generate and validate the complete candidate overlay
rol-phase8             assemble and seal the release candidate
rol-phase8-apply       hash-guarded development replacement; backup and journaling remain
rol-phase8-rollback    outstanding command to restore the exact pre-apply tree
rol-phase8-completion  validate the applied tree and prove a repeat-apply no-op
```

Phase 7 and Phase 8 generation must not write to `lib/world`. Apply must retain the
`APP_ENV=development` guard and must gain built-in backup, journaling, and automatic
restore before future use without an independently verified external snapshot.
Production deployment is separately authorized and must install an accepted artifact
without regenerating it.

### Candidate scope manifest

Create one durable candidate artifact containing:

- every source record ID, kind, VNUM, path, line, and source hash;
- every canonical target type, VNUM, path, and generated record hash;
- generated triggers, attachments, indexes, special procedures, and compatibility
  behavior;
- every repair, adaptation, and exclusion disposition;
- every current RoL record to add, replace, retain unchanged, or remove;
- every non-RoL file and record that must remain byte-identical; and
- source-tree, converter-code, calculator, policy, candidate, and runtime hashes.

Use the current target only to report changes and guard against concurrent writes. A
difference inside proven RoL scope is a replacement, not a builder conflict. A
difference outside that scope blocks the run.

### Safe full replacement and rollback

Before a development write, apply must:

1. Verify candidate, runtime binary, policy, and current-target hashes.
2. Verify every changed identity and path belongs to proven RoL scope or a required
   shared index update.
3. Prove all non-RoL record blocks and unrelated paths remain byte-identical.
4. Save exact copies and hashes of every path that could change.
5. Write an apply journal before the first replacement.
6. Install the complete candidate RoL output and required index entries.
7. Validate the resulting tree against the accepted candidate.
8. Restore the backup automatically if application or validation fails.
9. Prove immediate reapplication changes zero paths.

Implement rollback as a tested command, not merely a backup directory.

### Removal inside the generated namespace

List every removal explicitly and prove typed-reference closure. Before production,
also prove that saved objects and other durable owners will not become unresolved, or
provide a separately tested migration. No removal may touch a non-RoL identity.

## Remaining semantic accuracy audit

### Disposition completion

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

Complete field- and behavior-level coverage for:

- package and record presence;
- rooms, exits, doors, keys, sectors, flags, sizes, and descriptions;
- zone ranges, reset order, dependencies, probabilities, limits, and schedules;
- objects, equipment, loot, containers, spells, affects, traps, and decay;
- shops, prices, products, restrictions, messages, hours, and keeper behavior;
- quests, dialogue, prerequisites, costs, rewards, commands, and host bindings;
- SOC modes, actions, paths, calendars, targets, and generated DG triggers;
- direct, dynamic, implicit, composite, periodic, combat, death, and object special
  procedures;
- typed references, portals, transported destinations, scripted loads, and
  attachments;
- bounded repairs and every converter diagnostic; and
- player-visible behavior under the target runtime.

Extend the capability audit from "a handler exists" to semantic fidelity. Convert
every remaining silent fallback, lossy path, broad default, and unsupported syntax
into an explicit result.

### Outstanding mobile semantic checks

Candidate and runtime mobile acceptance must:

- parse every generated mobile through the target loader and compare the loaded
  prototype with its generation ledger;
- spawn representative and risk-ranked mobiles and compare final hit points, attack,
  armor, damage, saves, abilities, spell resistance, gold, experience, size, class,
  race, subraces, and explicit tier with expected post-load values;
- prove class-category, race, tier, spell-slot, known-spell, special-procedure, and
  other runtime modifiers apply exactly once;
- validate source action and affect adaptations by behavior, not only persisted bits;
- validate visible text, position normalization, and enhanced-record termination in
  the loaded candidate records;
- review the player impact of all bounded aggression-list and prestige exclusions;
- validate target-native experience, group division, group bonuses, gold, equipment,
  loot, keys, and progression outcomes;
- review level-51+ tier decisions against complete encounter context and version any
  rule changes; and
- record a walkthrough or explicit acceptance for every remaining non-exact mobile
  behavior.

## Remaining mobile candidate and gameplay validation

### Complete loaded-state and spawn audit

Using the two verified byte-identical full candidates and completed boot evidence:

1. Compare parsed prototypes and spawned instances with the mobile conversion ledger.
2. Verify special-procedure and DG attachment ownership and all typed references.
3. Record discrepancies as findings rather than patching generated files.

### High-level tier review

For all active authored level-51+ mobiles, add or derive a deterministic encounter
context report covering:

- zone and reset role;
- simultaneous reset population and intended party size;
- special procedures, known spells, attacks, defenses, and control effects;
- source combat values and relative rank among neighboring creatures;
- rewards, equipment, keys, and progression role; and
- named-boss identity.

Compare that report with the existing explicit tier. Any changed classification must
be a versioned automatic rule with focused and full-corpus tests. No review queue or
load-time tier inference is permitted.

### Named world bosses

Validate both Tiamat forms and their combined encounter in a booted candidate. Record
total HP budget, regeneration, room-wide breath cadence, disabling-effect removal,
control immunity, summons, phase transition, rewards, and group-size expectations.
Compare the encounter with source behavior and relevant native Luminari encounters.

If further World Boss identities are found during the context review, add an
individual versioned profile and the same source-to-target runtime evidence before
release.

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

Repair or explicitly accept kill-XP division, group bonuses, serialized gold, loot,
and equipment behavior before approving reward factors. Version any balance-driven
coefficient change and replace its golden vectors.

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

Seed it with unresolved candidate, runtime, regeneration, and semantic findings. Use
these priorities:

| Priority | Examples |
|----------|----------|
| P0 | Crash, boot failure, corruption, unsafe write, broken rollback, or dangling required identity. |
| P1 | Major combat, progression, topology, quest, shop, reset, or special-behavior defect. |
| P2 | Localized semantic loss, malformed text, missing optional behavior, or economy mismatch. |
| P3 | Cosmetic difference with no mechanical or progression effect. |

Every finding must carry the canonical target VNUM and exact affected owner. Do not
track by basename alone.

## Remaining implementation sequence

### Complete semantic and mobile runtime audits

- Produce remaining field- and behavior-level dispositions for all record types.
- Turn every remaining silent fallback and lossy diagnostic into a ledger row.
- Add target-loader and spawned-state comparison for generated mobiles.
- Complete level-51+ encounter-context review and Tiamat runtime validation.
- Produce per-package and global disposition counts.

Checkpoint: the pilot has no unreported loss and every non-exact result links to a
rule, evidence, test, impact statement, and walkthrough where runtime behavior is
involved.

### Repair the corpus in dependency-complete waves

Repair systemic rules before records, then process:

1. Source inventory, parsing, missing records, identity, and typed references.
2. Rooms, exits, doors, keys, zone ranges, resets, and traversal.
3. Candidate-loaded and spawned mobile defects, encounter tiers, rewards, equipment,
   loot, spells, and world-boss behavior.
4. Remaining objects, containers, affects, traps, and decay.
5. Shops, quests, rewards, dialogue, prerequisites, and progression.
6. SOC, DG triggers, paths, schedules, portals, and attachments.
7. Native and adapted special procedures.
8. Text, color, formatting, ambience, and cosmetic fidelity.

For each wave, group common causes, add focused tests, repair production conversion or
runtime code, regenerate only through isolated reconversion, build twice, validate
dependency closure, run targeted walkthroughs, and update ledgers.

Checkpoint: no P0 or P1 remains in wave scope and no new normalized world finding or
unresolved typed edge is introduced.

### Implement safe development apply and rollback

- Extend the current Phase 8 apply plan with exact pre-apply copies and hashes.
- Write a journal before the first replacement.
- Restore automatically on any copy or post-apply validation failure.
- Add `rol-phase8-rollback` and prove exact restoration after a successful apply.
- Retain the environment, target-tree, bundle, and runtime-binary guards.
- Apply twice and require the second run to change zero paths.

Checkpoint: development can move to the accepted candidate and back exactly, an
interrupted apply restores the prior tree, and no non-RoL record changes.

### Validate a full-corpus release

- Walk every converted zone entrance and risk-based samples of resets, quests, shops,
  portals, combat, death behavior, and scripted paths.
- Repeat the deterministic generation, validation, persistence, build, test, install,
  syntax-boot, and bounded runtime gates after any remaining semantic or gameplay
  repair.
- Complete a development soak covering scheduled and periodic behavior.

Checkpoint: all remaining acceptance criteria pass and production is unchanged.

### Deploy and close out

Production deployment remains separately authorized.

- Back up every production path that will change.
- Verify production is on the expected prior accepted release.
- Deploy the accepted development artifact without regenerating it.
- Run syntax, runtime, reference, persistence-resolution, and smoke checks.
- Retain a tested rollback artifact and decision window.
- Update permanent converter, builder, testing, help, and changelog documentation.
- Move newly completed work from this plan into the completed record.

Checkpoint: production matches accepted artifact hashes, passes deployment checks,
and every resolved finding records the shipped release.

## Remaining test requirements

### Reconversion tooling

Add tests for:

- replacement of a deliberately changed RoL record;
- new, replaced, unchanged, and explicitly removed RoL records;
- reference and persistence rejection for unsafe removals;
- hash drift, missing evidence, path escape, wrong environment, and changed runtime;
- backup, rollback, interrupted apply, and repeat no-op behavior;
- all-record semantic-disposition completeness.

### Mobile candidate and runtime

Add tests or recorded runtime evidence for:

- loader-equivalent comparison of every generated mobile with its ledger;
- spawned values proving every modifier applies exactly once;
- full level-51+ encounter-context reports and any resulting tier rule changes;
- target behavior for action and affect adaptations and documented exclusions;
- target reward, group, equipment, loot, key, and progression outcomes;
- special-procedure and known-spell interaction with generated statistics;
- actual in-game MEDIT display, edit, save, reload, and rejection paths for recovered
  fields;
- representative encounters across class categories and tiers; and
- the complete two-form Tiamat runtime encounter.

Every new repair needs positive, negative, boundary, and loss-diagnostic fixtures.

### Runtime and gameplay

Use production-linked CuTests for C contracts and staged-candidate syntax/runtime boots
against the repository's configured local development database. Give every repaired
package a walkthrough with expected observations and a recorded result. Automated
structure alone cannot prove dialogue, encounter flow, quest intent, shop behavior,
or ambience.

## Remaining acceptance criteria

The reconversion mechanism is not ready until:

- every write belongs to proven RoL scope or a required shared index update;
- every current-to-candidate difference is classified;
- every current RoL edit is replaced without a preservation exception;
- non-RoL Luminari records change by zero bytes;
- every removal has reference and persistence evidence;
- apply failure restores the exact prior tree;
- identical inputs produce byte-identical builds.

Accuracy recovery is not complete until:

- every active source record and meaningful directive has a non-`UNKNOWN` result;
- every non-exact result has evidence, impact, and a test or walkthrough;
- no unexplained whole-record exclusion remains;
- no P0 or P1 finding remains open;
- every P2 is resolved or explicitly accepted with reviewed impact;
- every generated mobile parses and spawns with ledger-matching effective state;
- every level-51+ tier has completed encounter-context evidence;
- every named World Boss has complete source-to-target runtime validation;
- helper protocol identity and hash are sealed in release evidence;
- zero cross-world typed references and missing reserved targets remain;
- existing non-RoL Luminari identities and bytes remain protected;
- the candidate adds no normalized world validation finding;
- final post-repair runtime, persistence, build, test, install, and documentation
  gates are rerun and pass;
- package walkthroughs pass; and
- development soak, automatic failure recovery, and rollback evidence are sealed.

P3 findings may remain only in a visible, explicitly accepted backlog.

## Immediate remaining milestone

The next milestone is candidate spawned-state validation:

1. Compare all loaded generated mobile prototypes with their generation ledgers.
2. Spawn the risk-ranked sample, including both Tiamat forms, and record effective
   runtime state and encounter findings.
3. Seed the durable finding ledger and repair any systemic conversion defects found.
4. Regenerate twice and require identical bytes after every repair.

Do not run another apply after a conversion repair until the regenerated candidate
passes the complete deterministic, semantic, persistence, and runtime gates.

## Risks and controls

| Risk | Required control |
|------|------------------|
| Someone edits RoL content during recovery | Enforce the no-building freeze and document that regeneration overwrites the edit. |
| Existing non-RoL content changes | Proven scope, reserved identities, and a zero-byte gate. |
| Structurally valid but wrong output is accepted | Semantic ledgers, source evidence, runtime tests, and walkthroughs. |
| Removed prototypes break saved state | Explicit removal ledgers plus persistence and reference closure. |
| Partial apply leaves a mixed world | Backup, journal, candidate hash, automatic restore, and rollback. |
| Evidence is missing or stale | Archived accepted artifact with durable checksums; fail closed. |
| Production is modified accidentally | Development guard and separately authorized deployment. |
| Evidence storage grows without bound | Retention rules plus permanent accepted-release checksums. |
| One-off patches hide systemic defects | Group findings by converter rule and repair common causes first. |

## Related references

- [Completed RoL conversion work](ROL_CONVERSION_DONE.md)
- [World Validator, Lookup, and RoL Reconciliation CLI](../utilities/WORLD_VALIDATOR_CLI.md)
- [Testing Guide - canonical RoL maintenance gate](../guides/TESTING_GUIDE.md#canonical-rol-maintenance-gate)
- [Realms of Luminari help entry](../../lib/text/help/realms_of_luminari.hlp)
- `scripts/world/wtool_lib/rol_source.py`
- `scripts/world/wtool_lib/rol_transform.py`
- `scripts/world/wtool_lib/rol_mobile_identity.py`
- `scripts/world/wtool_lib/rol_mob_calculator.py`
- `scripts/world/wtool_lib/rol_soc.py`
- `scripts/world/wtool_lib/rol_special.py`
- `scripts/world/wtool_lib/rol_phase7.py`
- `scripts/world/wtool_lib/rol_phase8.py`
- `scripts/world/rol_conversion_policy.json`
- `src/mob/mob_autoroll_profile.c`
- `src/spec/spec_rol_*.c`
