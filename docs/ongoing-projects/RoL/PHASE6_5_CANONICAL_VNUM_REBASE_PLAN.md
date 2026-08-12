# Phase 6.5: Canonical RoL VNUM Rebase and Reference Closure

- Status: Planned, not started
- Plan date: 2026-08-12
- Placement: after Phase 6 and before Phase 7
- Initial estimate: 8 sessions, 16-32 focused engineering hours
- Source corpus: `EXAMPLE/RealmsOfLuminari/`
- Target: this writable development checkout and its current `lib/world/`
- Production changes: prohibited

## Purpose

Phase 6.5 establishes one predictable VNUM namespace for the complete active Realms of
Luminari corpus before Phase 7 begins package-wide conversion.

The final mapping rule is:

```text
target zone VNUM   = normalized source zone VNUM + 20000
target entity VNUM = source entity VNUM + 2000000
```

Rooms, mobiles, and objects are entities. Shops and HLQuest blocks remain attached to
their canonical keeper or host mobile. Generated DG triggers receive deterministic
identities owned by the canonical destination zone.

The mapping manifest remains required for provenance, typed reference conversion, and
validation. It is not allowed to replace the normal VNUM formula with a collection of
legacy identity aliases.

This phase changes the earlier reconciliation rule that an evidence-confirmed legacy
target VNUM always wins. Existing converted content and local edits still win as content
evidence, but their RoL-owned identities are rehomed into the canonical namespace.

## Why This Phase Exists

The current plan uses a hybrid identity model:

- most new RoL content uses zone offset `+20000` and entity offset `+2000000`;
- Trail, Hulburg, and Jotunheim retain an older `+1000` zone and `+100000` entity
  mapping;
- eleven source artifact records map to ten target-native objects in zone `1699`;
- the malformed `mytheast.zon` header currently receives a fallback zone allocation.

Typed manifest lookup can make that model work, but it makes every cross-zone
relationship dependent on a large exception table. A universal mapping provides a
stronger invariant for:

- exits that connect separate zones;
- keys used outside the zone that defines them;
- portals, vehicles, and teleport destinations;
- quests that require, load, or reward records from another package;
- shops selling objects defined elsewhere;
- resets loading cross-package mobiles or objects;
- SOC paths and DG scripts that move through multiple areas;
- special procedures and hardcoded VNUM consumers; and
- future RoL updates, debugging, builder navigation, and automated audits.

After Phase 6.5, a developer can calculate the expected Luminari identity directly from
the source identity. Any remaining active source VNUM or legacy RoL target VNUM becomes
an auditable error instead of a possible intentional alias.

## Locked Decisions

The following decisions are part of this plan and are not implementation-time approval
questions:

1. All in-scope RoL-owned zones use the canonical `+20000` zone namespace.
2. All in-scope RoL-owned rooms, mobiles, and objects use the canonical `+2000000`
   entity namespace.
3. Existing target content and OLC edits are preserved as content, not as legacy VNUMs.
4. A distinct active source entity VNUM receives its distinct canonical target VNUM.
   Shared runtime code and data profiles are allowed; identity collapse is not.
5. Duplicate definitions of the same typed source VNUM resolve to one canonical target
   VNUM through the existing `MERGE` rules.
6. Old and new copies must not remain simultaneously active after cutover.
7. Runtime aliases from old RoL VNUMs to canonical VNUMs are not part of the final state.
8. External Luminari-owned dependencies keep their existing identities. They are not
   RoL namespace exceptions because RoL does not own those records.
9. Disabled, unlisted, and irreducibly excluded source content remains out of scope and
   does not receive a placeholder record merely to fill the formula.
10. All reference rewriting is typed. Blind numeric search-and-replace is prohibited.
11. This phase runs only in the authorized development checkout and isolated test
    environments. It never writes production code, world data, or databases.
12. Phase 7 cannot start until every Phase 6.5 exit gate passes.

## Canonical Identity Contract

### Zones

For a normal source zone record:

```text
canonical_target_zone = source_zone + 20000
```

The active normal source zone range reaches zone `9999`, so the reserved target range
`20000-29999` is sufficient. Source zone `9999` maps to target zone `29999`.

A source zone record can cover more than one 100-VNUM entity band. Its target top value
is shifted by the entity offset while its target zone identity uses its normalized
source zone identity. For example:

```text
source Hulburg zone record:  #591, top 59599
target Hulburg zone record:  #20591, top 2059599
```

The covered source bands `591-595` therefore become target bands `20591-20595` without
changing their internal spacing.

### The `mytheast.zon` Header Anomaly

`mytheast.zon` is not a real zone `81700` whose entities begin at `8170000`.
Current source evidence is:

- `areas/AREA` assigns `mytheast` to logical zones `817,818`;
- `areas/zon/mytheast.zon` incorrectly begins with `#81700`;
- its declared top is `81899`;
- its rooms, mobiles, objects, and resets use entity VNUMs `81700-81899`; and
- neighboring Myth Drannor packages use the same entity-scale convention.

Phase 6.5 must classify `#81700` as a source header anomaly and normalize the zone
identity to `817`. Its canonical target is:

```text
source logical zones:  817-818
target zone record:     20817
target entity range:    2081700-2081899
target zone top:        2081899
```

The converter must recognize this case through explicit manifest, range, and record
evidence. It must not:

- treat `81700` as a logical zone and allocate zone `101700`;
- retain the current fallback allocation at zone `20002`;
- expand the target zone range to conceal the malformed header; or
- apply an unverified divide-by-100 heuristic to arbitrary future headers.

Any future header normalization requires the same corroborating evidence: active
manifest ownership, declared range, contained record ranges, and typed reset/reference
usage.

### Rooms, Mobiles, and Objects

For every active source-defined room, mobile, or object:

```text
canonical_target_entity = source_entity + 2000000
```

Examples:

| Source | Canonical target |
|--------|-----------------:|
| Room `0` | `2000000` |
| Object `1043` | `2001043` |
| Mobile `50789` | `2050789` |
| Room `59433` | `2059433` |
| Object `96001` | `2096001` |
| Room `81700` | `2081700` |

The source kind remains part of the identity key. A room, mobile, and object may legally
reuse the same numeric VNUM in their separate typed namespaces.

### Shops, Quests, SOC, and Generated Triggers

- A source `SHOP:` record is attached to its keeper mobile. The keeper resolves through
  the canonical mobile formula; the target shop namespace is validated independently.
- A source quest header is an HLQuest host mobile. It resolves through the canonical
  mobile formula and does not receive a fictitious independent quest VNUM.
- SOC behavior remains attached to its canonical mobile identity.
- Every source room, mobile, or object reference compiled into a DG trigger uses the
  canonical typed resolver.
- Synthetic DG trigger VNUMs have no direct source identity. Allocate them
  deterministically in a collision-free, OLC-valid range owned by the canonical target
  zone and record their provenance.

### Merges and Native Target Features

The universal rule applies to identity, not to code duplication.

- Duplicate definitions of the same typed source VNUM may merge at that VNUM's one
  canonical target identity.
- Distinct source VNUMs may share one runtime handler or data profile, but each retains
  its own canonical prototype when it is active and valid.
- Existing target-only improvements, such as modern artifact behavior, must be carried
  into the canonical prototypes rather than used to justify a legacy VNUM alias.
- A target-native record referenced as an external dependency remains at its native
  VNUM. The reference ledger must prove that the record is target-owned rather than an
  RoL entity being left behind.

## Current Rebase Baseline

These counts describe the latest inspected Phase 2 policy-2 identity artifact on
2026-08-12. Another agent is working on the current file set, so Session 01 must
regenerate the inventory after that work lands. The counts below are planning evidence,
not frozen acceptance denominators.

The core identity map contains 64,395 zone, room, mobile, and object rows. It currently
has 1,988 mapped identities that do not equal the universal formula:

| Group | Noncanonical identity rows | Current state |
|-------|---------------------------:|---------------|
| Trail | 353 | zone `507 -> 1507`; 200 rooms, 151 mobiles, 1 object use legacy identities |
| Hulburg | 1,160 | zone `591 -> 1591`; 492 rooms, 400 mobiles, 267 objects use legacy identities |
| Jotunheim | 463 | zone `960 -> 1960`; 287 rooms, 89 mobiles, 86 objects use legacy identities |
| Modern artifacts | 11 | eleven source identities collapse into target objects `169901-169910` |
| Myth Drannor East | 1 | anomalous zone header currently falls back to target zone `20002` |
| Total | 1,988 | must be zero at Phase 6.5 exit |

Jotunheim contains six additional core records already planned as canonical additions;
they are not included in its 463 rehome rows.

The major package destinations become:

| Package | Current target | Canonical target |
|---------|----------------|------------------|
| Trail | zone `1507`, entities `150700+` | zone `20507`, entities `2050700+` |
| Hulburg | zone `1591`, entities `159100+` | zone `20591`, entities `2059100+` |
| Jotunheim | zone `1960`, entities `196000+` | zone `20960`, entities `2096000+` |
| Myth Drannor East | fallback zone `20002` | zone `20817`, entities `2081700-2081899` |

The modern artifact rows require special handling because source objects `1007` and
`1009` currently converge on target object `169906`. Phase 6.5 must restore both direct
canonical identities (`2001007` and `2001009`) while sharing runtime implementation
where appropriate. Existing `169906` ownership and progression data must move to one
deterministically selected canonical successor; it must not be duplicated. The decision
must be based on source prototype behavior, current artifact semantics, active source
consumers, and persistent-state evidence, then recorded in the rehome ledger.

## Required Deliverables

Phase 6.5 produces the following implementation artifacts when executed:

1. A canonical namespace policy version that makes the formula authoritative.
2. A normalized source-zone identity table, including the explicit `mytheast` repair.
3. A complete typed old-to-canonical rehome ledger.
4. A collision report for world records, zone ranges, hardcoded consumers,
   configuration, generated triggers, and relevant persistent database columns.
5. A reverse-reference ledger for every legacy target definition.
6. Updated converter planning and identity-resolution behavior.
7. Deterministic generated bundles for Trail, Hulburg, Jotunheim, the modern artifacts,
   and the `mytheast` zone identity.
8. Updated generated Phase 6 profiles and bindings using canonical identities.
9. An idempotent development-database migration for relevant persistent VNUMs.
10. Structural, runtime, behavioral, reset, walkthrough, and persistence evidence.
11. A removal manifest for legacy RoL-owned definitions and references.
12. A final namespace audit proving zero unexplained noncanonical identities.
13. Updated operator, builder, conversion, testing, and system documentation after the
    implementation is complete.

The rehome ledger must record at least:

```text
source kind
source VNUM
source record ID and hash
old target VNUM, if any
canonical target VNUM
migration operation
content authority and merge evidence
all incoming typed references
all outgoing typed references
runtime/code consumers
persistent consumers
validation status
removal status for the old identity
```

`REHOME` may be used as a Phase 6.5 migration operation. It does not replace the final
corpus actions `KEEP/PATCH/ADD/MERGE/EXCLUDE`. After a successful rehome, regeneration
should observe the canonical target and give the source record its correct final corpus
action there.

## Reference-Closure Requirements

The rebase is complete only when every relevant reference crosses the old-to-new
boundary correctly. The audit must cover at least the following.

### World Data

- room exits and exit keys;
- container keys and contained-object references;
- portal, teleport, vehicle, and transport destinations;
- zone reset commands for rooms, mobiles, objects, containers, doors, equipment,
  followers, removal, and calendar behavior;
- shops, keepers, rooms, and products;
- quest hosts, required objects, reward objects, loaded mobiles, and load destinations;
- SOC mobile owners and path rooms;
- DG trigger attachments and VNUM literals in trigger bodies; and
- object value slots with typed room, mobile, or object semantics.

### Runtime, Configuration, and Documentation

- hardcoded special-procedure assignments;
- generated special, periodic, state-aware, weapon, death, and utility profiles;
- artifact registry constants and artifact prototype assignments;
- runtime tables containing room, mobile, object, zone, shop, quest-host, or trigger
  VNUMs;
- example configuration templates where a canonical identity is part of shipped policy;
- builder and player help that exposes a VNUM contract; and
- test fixtures and assertions that intentionally encode an RoL identity.

Protected local configuration files remain untouched. If a template policy must change,
edit the appropriate `.example.h` file rather than `src/campaign.h`,
`src/mud_options.h`, or `src/vnums.h`.

### Persistent State

- every relevant VNUM-bearing database column found by the existing persistent binding
  inventory;
- artifact ownership, progression, cooldown, and account-binding state;
- stored player objects or inventories whose prototype identity is persisted;
- houses, mail, auctions, shops, quests, or other systems that persist world identities;
  and
- non-database persistence files consumed by the current runtime.

The database change must be scoped to the disposable isolated database during testing
and the authorized development database during final application. It must be
transactional where the storage engine permits, idempotent, and verified by before/after
typed counts. It must never target production.

### False Positives

Raw numeric searches are discovery evidence, not proof of a reference. Every match must
be classified as one of:

- active typed reference requiring rewrite;
- source or historical evidence that must remain unchanged;
- unrelated numeric literal;
- documentation example requiring update; or
- generated artifact that will be regenerated rather than hand-edited.

## Content-Preservation Rules

1. Do not rebuild legacy target packages blindly from source.
2. Treat the current development target record as authoritative for existing OLC edits
   and target-native improvements.
3. Treat the source record and conversion policy as authoritative for RoL identity,
   intended cross-zone relationships, and missing behavior.
4. Build the canonical record by applying the existing `KEEP/PATCH/MERGE` evidence to
   the current target content at the new identity.
5. Preserve record text, exits, flags, scripts, shops, quests, resets, and runtime
   attachments unless an explicit action and test justify a change.
6. Do not leave a forwarding room, duplicate prototype, compatibility portal, or runtime
   VNUM alias merely to make an incomplete reference audit pass.
7. Do not remove an old definition from the final development apply unless its canonical
   definition and all required incoming references are present in the same validated
   bundle.
8. Regenerate generated data from policy and source hashes; do not hand-edit ignored
   generated outputs as the source of truth.

## Execution Safety and Sequencing

- Recheck `lib/.env` at execution time and stop if the checkout is production. Read it
  only; do not modify it.
- Do not begin while another agent is changing the RoL converter, policy, runtime
  bindings, or active planning artifacts. Rebaseline after that work is complete.
- Finish Phase 6 first so the special-procedure inventory and generated profiles are
  stable before rebasing their identities.
- Perform analysis, generation, and all initial validation in disposable isolated roots.
- Use the source and target inventory hashes already required by the RoL conversion
  project. If inputs change before application, regenerate the affected bundle and
  evidence.
- Apply canonical definitions, rewritten references, persistent-state migrations, and
  legacy removals as one planned development cutover after the assembled isolated target
  passes every gate.
- The project does not require backup, snapshot, preimage, rollback, recovery, remote,
  or production-capture artifacts. Isolated staging and deterministic regeneration are
  the correctness mechanisms.
- Repeat the complete application against an already migrated disposable copy. The
  second application must write zero records or report an explicit safe no-op.

## Session Plan

Each session is one 2-4 hour objective with approximately 12-25 tasks. The session
boundaries may be split further if measured implementation work cannot fit that limit,
but no session may silently carry unfinished scope into Phase 7.

### Session 6.5.1: Freeze the Canonical Namespace Baseline

Objective: regenerate current evidence and create the complete canonical rehome inventory.

Tasks:

1. Confirm the checkout is non-production without modifying `lib/.env`.
2. Record the Phase 6 completion run and policy versions used as input.
3. Regenerate source and development-target inventories with hashes.
4. Regenerate the Phase 1 discovery and Phase 2 action/identity artifacts.
5. Count every identity that differs from the universal formula by typed kind.
6. Group noncanonical rows by package, runtime subsystem, and current destination.
7. Confirm the active logical source zone range and reserved target zone range.
8. Add explicit evidence for the `mytheast` header normalization.
9. Inventory every legacy Trail, Hulburg, and Jotunheim target definition.
10. Inventory every modern artifact source identity and target-native successor.
11. Inventory all currently generated Phase 6 profiles that contain target VNUMs.
12. Inventory all relevant database and non-database persistent VNUM consumers.
13. Generate the first typed old-to-canonical rehome ledger.
14. Generate a reverse-reference report for every old target identity.
15. Fail the inventory on an unclassified identity or collision.
16. Record the baseline counts and hashes in the Phase 6.5 run manifest.

Success gate: every noncanonical identity and every old target consumer has an owned
rehome or normalization row; there are no unknown rows.

### Session 6.5.2: Enforce the Universal Resolver and Zone Normalization

Objective: make the planner and all converter identity consumers enforce the canonical
formula.

Tasks:

1. Version the conversion policy for the universal namespace contract.
2. Replace legacy-lineage-first identity selection with canonical-first selection.
3. Retain lineage matches as content/rehome evidence rather than destination selection.
4. Implement normalized source-zone identity support.
5. Implement the explicit `mytheast` normalization with corroborating evidence checks.
6. Reject `mytheast` destinations `20002` and `101700`.
7. Enforce direct entity mapping for rooms, mobiles, and objects.
8. Prevent distinct active source VNUMs from collapsing into one target identity.
9. Preserve same-typed-VNUM duplicate merges at the single canonical destination.
10. Update shop, quest-host, and SOC owner resolution.
11. Update synthetic trigger ownership to canonical destination zones.
12. Update collision checks for the complete canonical ranges.
13. Add positive formula tests for low, typical, and maximum source VNUMs.
14. Add multi-zone record tests for source top-value translation.
15. Add negative tests for malformed headers, ambiguous ownership, and range overflow.
16. Add tests proving legacy target candidates cannot override canonical identity.
17. Regenerate twice and prove byte-identical planning artifacts.
18. Run the focused and complete world-tool suites.

Success gate: the planner produces zero unexplained noncanonical destination identities
and all normalized zone identities are evidence-backed.

### Session 6.5.3: Rehome Trail, Hulburg, and Jotunheim

Objective: generate canonical packages that preserve the three legacy conversions and
their local target work.

Tasks:

1. Load old target records and current source records through typed parsers.
2. Reconcile old target hashes against the Session 01 inventory.
3. Generate Trail zone `20507` and its canonical entity records.
4. Generate Hulburg zone `20591` and its canonical entity records.
5. Generate Jotunheim zone `20960` and its canonical entity records.
6. Carry the six already-canonical Jotunheim additions into the assembled package.
7. Preserve package top values and sparse entity layout under the entity offset.
8. Preserve bounded target and OLC edits through explicit rehome actions.
9. Rewrite all intra-package exits, keys, resets, shops, quests, and scripts.
10. Rewrite outgoing references from the three packages to other canonical packages.
11. Rewrite incoming world-data references from other packages to these packages.
12. Regenerate attached DG triggers in canonical owning-zone ranges.
13. Regenerate Phase 6 profiles and binding ledgers for the new identities.
14. Produce old-definition removal entries without applying them yet.
15. Validate each isolated package structurally and by syntax boot.
16. Run reset observation and scripted walkthroughs for every rehomed room component.
17. Compare behavior evidence with the accepted pilot/Phase 6 evidence.
18. Prove repeated package generation is byte-identical.

Success gate: all three packages stage exclusively at canonical identities, preserve
their accepted content and behavior, and have no unresolved typed reference.

### Session 6.5.4: Rehome Modern Artifact Identities

Objective: move all RoL-owned artifact prototypes and behavior into their direct
canonical identities without duplicating persistent ownership.

Tasks:

1. Trace all eleven source prototypes and the ten target-native artifact prototypes.
2. Record the current content, runtime behavior, and persistence contract for each.
3. Map each distinct source object to its direct `+2000000` identity.
4. Split source objects `1007` and `1009` into canonical objects `2001007` and
   `2001009` while reusing shared runtime code where correct.
5. Select and document the one canonical successor for existing `169906` persistent
   state using traced evidence and a deterministic tie-break.
6. Preserve target-native artifact improvements in the appropriate canonical prototype.
7. Update artifact registry constants and typed lookup tables.
8. Update object special assignments and generated reconciliation profiles.
9. Update artifact commands, ownership checks, cooldowns, and progression lookups.
10. Update summon, travel, spell, combat, and called-effect dependencies.
11. Generate persistent-state migration rows for `169901-169910`.
12. Prove the migration neither duplicates nor loses ownership/progression rows.
13. Rewrite every world reset, quest, shop, script, and code consumer of the old objects.
14. Generate removal entries for obsolete `1699xx` RoL-owned prototypes.
15. Extend production-linked tests for every canonical artifact identity.
16. Run artifact behavior regressions and the complete world-tool suite.
17. Run the production-linked CuTest suite and install gate.
18. Record behavioral equivalence and intentional source-variant differences.

Success gate: all eleven active source artifact identities exist at their direct
canonical VNUMs, current enhanced behavior is preserved, and no active or persistent
consumer requires `169901-169910` as an RoL identity.

### Session 6.5.5: Close All World-Data References

Objective: prove that every world-data edge resolves through the canonical identity
graph.

Tasks:

1. Build the complete typed definition and reference graph for the staged corpus.
2. Validate every cross-zone exit and reverse exit where one is intended.
3. Validate every exit and container key against its canonical object.
4. Validate every portal, teleport, vehicle, and transport destination.
5. Validate every zone reset argument by target type.
6. Validate every shop keeper, room, and product.
7. Validate every quest host, required item, reward, mobile, and load destination.
8. Validate every SOC owner and path room.
9. Validate every DG attachment and typed VNUM literal in trigger bodies.
10. Validate all typed object value references.
11. Classify references to target-native external dependencies explicitly.
12. Reject active source VNUMs that survive in emitted target fields.
13. Reject references to retired Trail, Hulburg, Jotunheim, or artifact VNUMs.
14. Reject missing canonical definitions and namespace collisions.
15. Produce per-package incoming and outgoing reference reports.
16. Add fixtures for cross-zone keys, quests, resets, portals, and scripts.
17. Run strict validation for every selected or touched zone.
18. Record zero unresolved required reference edges.

Success gate: the complete staged typed graph has no unresolved edge, old RoL edge, or
unexplained external edge.

### Session 6.5.6: Close Runtime and Persistent Consumers

Objective: remove all non-world dependencies on old RoL identities and prove persistent
state migration.

Tasks:

1. Search current runtime code for every old and source identity.
2. Classify each match as active, historical, generated, documentation, or unrelated.
3. Replace active hardcoded consumers with canonical constants or generated profiles.
4. Regenerate special-procedure and event binding tables.
5. Regenerate periodic, state-aware, combat, death, weapon, and utility profiles.
6. Update relevant example configuration templates without touching protected local
   headers.
7. Build the idempotent development-database migration.
8. Apply the migration to an isolated database populated with representative old rows.
9. Verify typed before/after row counts and uniqueness constraints.
10. Verify player inventory and object persistence across save and reload.
11. Verify artifact ownership, progression, cooldown, and account binding across reload.
12. Verify quest, shop, house, mail, auction, or other discovered persistent consumers.
13. Verify non-database persistence files with old identities are migrated or regenerated.
14. Run a second migration and prove it is a no-op.
15. Boot the staged world against the migrated isolated database.
16. Reject relevant invalid-record, reference, reset, trigger, persistence, or `SYSERR`
    diagnostics.
17. Run focused persistence and runtime regressions.
18. Produce the final code/configuration/persistence consumer ledger.

Success gate: zero active runtime, configuration, or persistent consumer depends on a
retired RoL identity, and the migration is lossless and idempotent.

### Session 6.5.7: Assemble and Validate the Complete Rebase

Objective: prove the entire canonical rebase as one isolated, release-quality bundle.

Tasks:

1. Assemble the canonical target from the current development baseline and all Phase
   6.5 bundles.
2. Include canonical definitions and all rewritten references.
3. Include the persistent-state migration in the isolated runtime setup.
4. Exclude every planned legacy RoL definition from the assembled result.
5. Run `wtool.py validate --all --strict` against the explicit isolated world root.
6. Run strict per-zone validation for every touched zone.
7. Compare finding identities and parse completeness with the recorded baseline.
8. Require zero new global finding and zero unresolved finding on touched records.
9. Run `bin/circle -c -d <isolated-lib-root>`.
10. Boot with an isolated MariaDB test instance.
11. Observe eligible resets for all rehomed packages.
12. Run scripted walkthroughs covering every rehomed room component.
13. Exercise representative cross-zone exits, keys, portals, shops, quests, and scripts.
14. Exercise all canonical artifacts and persistent reload paths.
15. Run `make test-world-tools`.
16. Run `make test` followed by `make install`.
17. Verify no root-level `circle` artifact remains.
18. Run the documentation drift and ASCII/LF checks.
19. Apply the bundle a second time to a disposable migrated copy.
20. Require byte-identical generation and zero second-apply writes.

Success gate: the assembled isolated target passes every structural, behavioral,
persistence, determinism, no-clobber, and idempotency gate.

### Session 6.5.8: Apply and Close the Development Rebase

Objective: apply the validated canonical rebase to development and publish final Phase
6.5 evidence.

Tasks:

1. Reconfirm the checkout and database are development, not production.
2. Regenerate current input inventories immediately before application.
3. Rebuild and revalidate any bundle whose recorded input changed.
4. Apply canonical world definitions to the development world.
5. Apply every rewritten world-data reference.
6. Apply runtime and configuration changes required by the canonical identities.
7. Apply the validated development-database migration.
8. Remove legacy RoL-owned definitions as declared by the removal manifest.
9. Regenerate all indexes and generated profiles.
10. Run strict validation against the resulting development world.
11. Run the syntax boot and a bounded development behavioral boot.
12. Verify the development database and persistence reload checks.
13. Run the universal-namespace audit against all active source identities.
14. Require zero unexplained noncanonical destination identity.
15. Require zero active definition or reference at a retired RoL identity.
16. Require zero active target field containing an unmapped source VNUM.
17. Re-run `make test-world-tools`, `make test`, and `make install`.
18. Update the roadmap, scope, worknotes, manual testing guide, artifact documentation,
   help files, and changelog with the completed policy and evidence.
19. Record final run IDs, hashes, counts, exclusions, migrations, and validation results.
20. Mark Phase 7 unblocked only after every exit gate below passes.

Success gate: the authoritative development target uses the canonical RoL namespace and
Phase 7 can build every remaining package against one stable identity contract.

## Validation Commands

Use explicit roots. Never rely on the default development world during isolated gates.

```bash
python3 scripts/world/wtool.py \
  --world-root <isolated-lib-root>/world validate --all --strict

lib/world/validate-zone.sh <zone-vnum> \
  --world-root <isolated-lib-root>/world --strict

bin/circle -c -d <isolated-lib-root>

make test-world-tools
make test
make install
```

Full behavioral boots use an isolated MariaDB instance prepared through the existing
test-runtime safety tooling. Validation must inspect parsed finding counts and identities,
not only process exit status.

## Phase Exit Gates

Phase 6.5 is complete only when all of the following are true:

1. Every active source zone has one evidence-backed normalized source zone identity.
2. Every active source zone destination equals normalized source zone plus `20000`.
3. Every active non-excluded source room, mobile, and object destination equals source
   VNUM plus `2000000`.
4. Every distinct active typed source identity remains distinct at the target, except
   duplicate definitions of the same typed source VNUM.
5. The `mytheast` zone maps to `20817` and its entities map to
   `2081700-2081899`.
6. Trail, Hulburg, and Jotunheim exist only at their canonical RoL identities.
7. All eleven modern-artifact source identities exist at their direct canonical VNUMs.
8. No active world, runtime, configuration, generated, or persistent reference requires
   the retired RoL-owned identities.
9. Every cross-zone exit, key, quest, shop, reset, portal, SOC path, and DG reference
   resolves through the typed canonical manifest.
10. Every canonical allocation is collision-free in its typed namespace.
11. Existing target and OLC edits are preserved or changed only by an explicit,
    evidence-backed action.
12. Persistent migrations preserve ownership, progression, inventory, and other
    discovered state without duplication.
13. The complete isolated target adds no baseline diagnostic and touched records have
    no unresolved finding.
14. Syntax boot, isolated database boot, reset observation, scripted walkthroughs,
    focused regressions, world-tool tests, CuTests, and installation all pass.
15. Repeated generation is byte-identical and repeated application is a safe no-op.
16. The development target passes the same namespace and structural audits after apply.
17. Final documentation describes the universal mapping and no active plan still states
    that legacy target identity takes precedence for RoL-owned records.
18. No unresolved decision, unexplained exception, or `BLOCKED` identity remains.

The decisive machine-checkable invariant is:

```text
noncanonical active RoL zone identities   = 0
noncanonical active RoL entity identities = 0
active references to retired RoL VNUMs    = 0
unresolved required typed references       = 0
```

## Risks and Mitigations

### Existing OLC Edits Could Be Lost

Mitigation: generate canonical content from the reconciled current target plus source
evidence. Do not overwrite it with a raw shifted source record. Compare semantic fields
and hashes through the rehome ledger.

### Cross-Zone References Could Be Missed

Mitigation: build a complete typed reference graph, maintain reverse references for
every old identity, and fail on any old or unresolved edge. Use raw search only as a
secondary discovery mechanism.

### Old and New Copies Could Both Load

Mitigation: the final assembled and development targets must contain only canonical
RoL-owned definitions. Index and definition audits reject simultaneous active copies.

### Persistent Rows Could Point to Removed Prototypes

Mitigation: inventory all VNUM-bearing stores, test an idempotent migration against an
isolated database, and verify save/reload behavior before development application.

### Artifact Identity Splitting Could Duplicate Ownership

Mitigation: migrate the current merged artifact state to one evidence-backed canonical
successor, create the second source identity without cloning persistent ownership, and
test uniqueness and account-binding rules.

### Generated Phase 6 Data Could Reintroduce Old VNUMs

Mitigation: update generators and policy inputs, regenerate all profiles, and reject old
VNUMs in generated output. Do not patch generated files manually.

### The Parallel Phase 6 Work Could Change the Baseline

Mitigation: make Phase 6 completion and agent handoff prerequisites. Session 01
regenerates all denominators and hashes; this plan's current counts are advisory.

### A Source Zone Header Could Be Misclassified

Mitigation: normalize only with corroborating manifest, range, contained-record, and
typed-reference evidence. Keep `mytheast` as an explicit tested anomaly rather than a
generic arithmetic shortcut.

## Out of Scope

- Converting the remaining Phase 7 packages beyond what is necessary to prove reference
  closure for the rebase.
- Changing gameplay balance unrelated to identity migration.
- Importing disabled, unlisted, or demonstrably non-working RoL content.
- Creating compatibility duplicates or permanent runtime aliases.
- Renumbering target-native Luminari content that is not RoL-owned.
- Modifying production files, services, or databases.
- Editing protected local configuration headers or credential files.
- Creating backup, rollback, recovery, or remote-capture artifacts.

## Deferred Documentation Synchronization

Only this plan file is created now because another agent is working with the current RoL
file set. When Phase 6.5 implementation begins after that work is complete, the final
session must reconcile at least:

- `REALMS_OF_LUMINARI_FEATURE_FIRST_CONVERSION_PLAN.md`;
- `REALMS_OF_LUMINARI_ZONE_CONVERSION_SCOPE.md`;
- `REALMS_OF_LUMINARI_WORKNOTES.md`;
- `PHASE4_MANUAL_TESTING.md`;
- `RoL-Changelog.md`;
- artifact system and placement documentation;
- relevant builder/player help; and
- any generated documentation or tests that expose legacy RoL identities.

Until that synchronization occurs, this document is the authoritative proposed Phase
6.5 policy. Existing files continue to describe the currently implemented hybrid policy
and must not be mistaken for the Phase 6.5 end state.

## Handoff

Prerequisite sequence:

```text
finish current Phase 6 work
-> stop concurrent RoL identity/policy edits
-> execute Session 6.5.1 baseline
-> complete Sessions 6.5.2 through 6.5.8
-> pass all Phase 6.5 exit gates
-> begin Phase 7 action-based corpus batches
```

Phase 7 must consume only the canonical Phase 6.5 policy, identity map, and validated
development baseline.
